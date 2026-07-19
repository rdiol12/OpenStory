using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Net.Http;
using System.Net.Security;
using System.Security.Cryptography;
using System.Text.Json;

namespace AugurLauncher;

public enum ServerState { Checking, Online, Offline }

/// <summary>Snapshot of progress, marshalled to the UI thread.</summary>
public sealed class ProgressState
{
    public ServerState Server = ServerState.Checking;
    public string Status = "Checking for updates…";
    public string CurrentFile = "";
    public double FilePercent;
    public double OverallPercent;
    public double SpeedBps;
}

/// <summary>Result of the check phase: what (if anything) needs downloading.</summary>
public sealed class CheckResult
{
    public ServerState Server = ServerState.Checking;
    public List<PendingFile> Pending = new();
    public string? Error;
}

/// <summary>One file that differs from the server — shown in the picker (checkbox bound to Selected).</summary>
public sealed class PendingFile : INotifyPropertyChanged
{
    internal readonly Updater.Job Job;
    public PendingFile(Updater.Job job, bool selected) { Job = job; _selected = selected; }

    public string Name => Job.File.Name;
    public long Size => Job.File.Size;
    public string SizeText => FormatSize(Size);
    public bool IsNew => Job.IsNew;
    public string Tag => Job.IsNew ? "new" : "update";

    private bool _selected;
    public bool Selected
    {
        get => _selected;
        set { if (_selected != value) { _selected = value; PropertyChanged?.Invoke(this, new(nameof(Selected))); } }
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    public static string FormatSize(long b)
    {
        string[] u = { "B", "KB", "MB", "GB" };
        double v = b; int i = 0;
        while (v >= 1024 && i < u.Length - 1) { v /= 1024; i++; }
        return i == 0 ? $"{b} B" : $"{v:0.0} {u[i]}";
    }
}

/// <summary>
/// Syncs the two manifests (game-data + client-build). Self-signed TLS is
/// trusted only for host 72.60.176.12.
/// </summary>
public sealed class Updater
{
    private const string TrustedHost = "72.60.176.12";
    private const int MaxAttempts = 3;

    private readonly LauncherConfig _cfg;
    private readonly HttpClient _http;
    private readonly HashCache _cache;
    private string? _sessionDir;
    private bool _forceHash;   // Repair/Verify: re-hash every file, ignore the cache

    public Updater(LauncherConfig cfg)
    {
        _cfg = cfg;
        _cache = HashCache.Load(cfg.BaseDir);

        var handler = new SocketsHttpHandler
        {
            SslOptions = new SslClientAuthenticationOptions
            {
                RemoteCertificateValidationCallback = (sender, cert, chain, errors) =>
                {
                    string host = (sender as SslStream)?.TargetHostName ?? "";
                    if (string.Equals(host, TrustedHost, StringComparison.OrdinalIgnoreCase)) return true;
                    return errors == SslPolicyErrors.None;
                }
            },
            PooledConnectionLifetime = TimeSpan.FromMinutes(5),
        };
        _http = new HttpClient(handler) { Timeout = Timeout.InfiniteTimeSpan };
        _http.DefaultRequestHeaders.UserAgent.ParseAdd("AugurMS-Launcher/1.0");
    }

    // ---- GenAI live feed (best-effort; never throws) ----
    public async Task<List<FeedEntry>> FetchFeedAsync(CancellationToken ct)
    {
        try
        {
            if (string.IsNullOrWhiteSpace(_cfg.FeedUrl)) return new();
            using var cts = CancellationTokenSource.CreateLinkedTokenSource(ct);
            cts.CancelAfter(TimeSpan.FromSeconds(15));
            string json = await _http.GetStringAsync(_cfg.FeedUrl, cts.Token);
            var feed = Feed.Parse(json);
            // newest first
            return feed.Entries
                .OrderByDescending(e => e.Time ?? DateTimeOffset.MinValue)
                .ToList();
        }
        catch { return new(); }
    }

    // ---- Live GM activity from the dashboard (drives News + GenAI's Log) ----
    public async Task<List<DashActivity>> FetchActivityAsync(CancellationToken ct)
    {
        try
        {
            if (string.IsNullOrWhiteSpace(_cfg.NewsUrl)) return new();
            using var cts = CancellationTokenSource.CreateLinkedTokenSource(ct);
            cts.CancelAfter(TimeSpan.FromSeconds(15));
            var items = DashResponse.Parse(await _http.GetStringAsync(_cfg.NewsUrl, cts.Token));
            return items
                .OrderByDescending(a => a.Date ?? DateTimeOffset.MinValue)
                .ToList();
        }
        catch { return new(); }
    }

    // ---- Self-update descriptor (best-effort) ----
    public async Task<LauncherRelease?> FetchLauncherReleaseAsync(CancellationToken ct)
    {
        try
        {
            if (string.IsNullOrWhiteSpace(_cfg.LauncherManifestUrl)) return null;
            using var cts = CancellationTokenSource.CreateLinkedTokenSource(ct);
            cts.CancelAfter(TimeSpan.FromSeconds(15));
            string json = await _http.GetStringAsync(_cfg.LauncherManifestUrl, cts.Token);
            return JsonSerializer.Deserialize<LauncherRelease>(json,
                new JsonSerializerOptions { PropertyNameCaseInsensitive = true });
        }
        catch { return null; }
    }

    // Download the new launcher exe to a temp path next to the target, verified by hash.
    public async Task<string> DownloadLauncherAsync(LauncherRelease rel, string url, CancellationToken ct)
    {
        string dir = AppContext.BaseDirectory;
        string tmp = Path.Combine(dir, "GenMs.new.exe");
        using (var resp = await _http.GetAsync(url, HttpCompletionOption.ResponseHeadersRead, ct))
        {
            resp.EnsureSuccessStatusCode();
            await using var fs = new FileStream(tmp, FileMode.Create, FileAccess.Write, FileShare.None, 1 << 20);
            await resp.Content.CopyToAsync(fs, ct);
        }
        if (!string.IsNullOrWhiteSpace(rel.Sha256))
        {
            string got = HashCache.Sha256File(tmp);
            if (!string.Equals(got, rel.Sha256, StringComparison.OrdinalIgnoreCase))
            {
                try { File.Delete(tmp); } catch { }
                throw new InvalidDataException("launcher update checksum mismatch");
            }
        }
        return tmp;
    }

    public sealed class Job
    {
        public required Manifest Manifest;
        public required ManifestFile File;
        public required string DestPath;
        public bool IsNew; // file missing locally (vs. changed)
    }

    // ---- Phase 1: check what needs downloading (no writes) ----
    public async Task<CheckResult> CheckAsync(SelectionStore selection, IProgress<ProgressState> report, CancellationToken ct, bool force = false)
    {
        _forceHash = force;
        var r = new CheckResult { Server = ServerState.Checking };
        report.Report(new ProgressState { Server = ServerState.Checking, Status = force ? "Verifying game files…" : "Checking for updates…" });

        Manifest? dataM = await TryFetchManifest(_cfg.DataManifestUrl, ct);
        Manifest? clientM = await TryFetchManifest(_cfg.ClientManifestUrl, ct);

        if (dataM is null && clientM is null)
        {
            r.Server = ServerState.Offline;
            r.Error = "Could not reach the update server. You can still play with the installed files.";
            return r;
        }

        r.Server = ServerState.Online;
        report.Report(new ProgressState { Server = ServerState.Online, Status = "Verifying game files…" });

        var all = new List<Job>();
        if (dataM is not null) CollectJobs(dataM, _cfg.DataDirFull, all);
        if (clientM is not null) CollectJobs(clientM, _cfg.InstallDirFull, all);

        foreach (var job in all)
        {
            ct.ThrowIfCancellationRequested();
            if (NeedsDownload(job.File, job.DestPath, out bool isNew))
            {
                job.IsNew = isNew;
                // default checked, unless the user unchecked it before
                bool sel = selection.IsSelected(job.File.Name, defaultValue: true);
                r.Pending.Add(new PendingFile(job, sel));
            }
        }
        _cache.Save();
        return r;
    }

    // ---- Phase 2: download the chosen files ----
    public async Task DownloadAsync(IReadOnlyList<PendingFile> files, IProgress<ProgressState> report, CancellationToken ct)
    {
        long totalBytes = files.Sum(f => Math.Max(0, f.Size));
        long doneBytes = 0;
        var sw = Stopwatch.StartNew();
        long sampleBytes = 0; var sampleTime = TimeSpan.Zero;

        for (int i = 0; i < files.Count; i++)
        {
            ct.ThrowIfCancellationRequested();
            var job = files[i].Job;
            string shortName = Path.GetFileName(job.File.Name);

            report.Report(new ProgressState
            {
                Server = ServerState.Online,
                Status = $"Downloading {shortName} ({i + 1}/{files.Count})…",
                CurrentFile = shortName,
                OverallPercent = totalBytes > 0 ? doneBytes * 100.0 / totalBytes : 0,
            });

            long fileStart = doneBytes;
            await DownloadWithRetry(job, ct, (fileRead, fileSize) =>
            {
                doneBytes = fileStart + fileRead;
                var now = sw.Elapsed;
                double dt = (now - sampleTime).TotalSeconds;
                if (dt >= 0.15)
                {
                    report.Report(new ProgressState
                    {
                        Server = ServerState.Online,
                        Status = $"Downloading {shortName} ({i + 1}/{files.Count})…",
                        CurrentFile = shortName,
                        FilePercent = fileSize > 0 ? fileRead * 100.0 / fileSize : 0,
                        OverallPercent = totalBytes > 0 ? doneBytes * 100.0 / totalBytes : 0,
                        SpeedBps = (doneBytes - sampleBytes) / dt,
                    });
                    sampleBytes = doneBytes; sampleTime = now;
                }
            });
            doneBytes = fileStart + Math.Max(0, job.File.Size);
        }

        _cache.Save();
        report.Report(new ProgressState
        {
            Server = ServerState.Online, Status = "Update complete — ready to play!",
            OverallPercent = 100, FilePercent = 100,
        });
    }

    private void CollectJobs(Manifest m, string targetDir, List<Job> into)
    {
        foreach (var f in m.Files)
        {
            if (string.IsNullOrWhiteSpace(f.Name)) continue;
            into.Add(new Job { Manifest = m, File = f, DestPath = SafeDestPath(targetDir, f.Name) });
        }
    }

    private static string SafeDestPath(string targetDir, string name)
    {
        string rel = name.Replace('\\', '/').TrimStart('/');
        string full = Path.GetFullPath(Path.Combine(targetDir, rel));
        string root = Path.GetFullPath(targetDir);
        if (!full.StartsWith(root.TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase)
            && !string.Equals(full, root, StringComparison.OrdinalIgnoreCase))
            throw new InvalidDataException($"manifest entry '{name}' escapes the target folder");
        return full;
    }

    private bool NeedsDownload(ManifestFile f, string dest, out bool isNew)
    {
        isNew = !File.Exists(dest);
        if (isNew) return true;
        var fi = new FileInfo(dest);
        if (f.Size > 0 && fi.Length != f.Size) return true;
        if (string.IsNullOrWhiteSpace(f.Sha256)) return false;
        string local = _forceHash ? HashCache.Sha256File(dest) : _cache.GetOrCompute(dest, fi);
        return !string.Equals(local, f.Sha256, StringComparison.OrdinalIgnoreCase);
    }

    private async Task<Manifest?> TryFetchManifest(string url, CancellationToken ct)
    {
        try
        {
            using var cts = CancellationTokenSource.CreateLinkedTokenSource(ct);
            cts.CancelAfter(TimeSpan.FromSeconds(20));
            return Manifest.Parse(await _http.GetStringAsync(url, cts.Token));
        }
        catch { return null; }
    }

    private async Task DownloadWithRetry(Job job, CancellationToken ct, Action<long, long> onChunk)
    {
        string url = job.Manifest.ResolveUrl(job.File);
        Exception? last = null;

        for (int attempt = 1; attempt <= MaxAttempts; attempt++)
        {
            ct.ThrowIfCancellationRequested();
            string tmp = job.DestPath + ".part-" + Guid.NewGuid().ToString("N")[..8];
            try
            {
                Directory.CreateDirectory(Path.GetDirectoryName(job.DestPath)!);
                using (var resp = await _http.GetAsync(url, HttpCompletionOption.ResponseHeadersRead, ct))
                {
                    resp.EnsureSuccessStatusCode();
                    long size = resp.Content.Headers.ContentLength ?? job.File.Size;
                    await using var net = await resp.Content.ReadAsStreamAsync(ct);
                    await using (var fs = new FileStream(tmp, FileMode.Create, FileAccess.Write, FileShare.None, 1 << 20))
                    {
                        var buf = new byte[1 << 16];
                        long read = 0; int n;
                        while ((n = await net.ReadAsync(buf, ct)) > 0)
                        {
                            await fs.WriteAsync(buf.AsMemory(0, n), ct);
                            read += n;
                            onChunk(read, size);
                        }
                    }
                }
                if (!string.IsNullOrWhiteSpace(job.File.Sha256))
                {
                    string got = HashCache.Sha256File(tmp);
                    if (!string.Equals(got, job.File.Sha256, StringComparison.OrdinalIgnoreCase))
                        throw new InvalidDataException($"checksum mismatch for {job.File.Name}");
                }
                BackupExisting(job.DestPath);   // save old version first
                AtomicReplace(tmp, job.DestPath);
                _cache.Update(job.DestPath);
                return;
            }
            catch (OperationCanceledException) { TryDelete(tmp); throw; }
            catch (Exception ex)
            {
                last = ex; TryDelete(tmp);
                if (attempt < MaxAttempts) await Task.Delay(TimeSpan.FromSeconds(attempt), ct);
            }
        }
        throw new IOException($"Failed to download {job.File.Name} after {MaxAttempts} attempts: {last?.Message}", last);
    }

    private void BackupExisting(string destPath)
    {
        try
        {
            if (!File.Exists(destPath)) return;
            string session = EnsureBackupSession();
            string rel = Path.GetRelativePath(_cfg.BaseDir, destPath);
            if (rel.StartsWith("..", StringComparison.Ordinal) || Path.IsPathRooted(rel))
                rel = Path.GetFileName(destPath);
            string backupPath = Path.Combine(session, rel);
            Directory.CreateDirectory(Path.GetDirectoryName(backupPath)!);
            File.Copy(destPath, backupPath, overwrite: true);
        }
        catch { }
    }

    private string EnsureBackupSession()
    {
        if (_sessionDir != null) return _sessionDir;
        string root = _cfg.BackupDirFull;
        if (_cfg.BackupKeep > 0) PruneOldBackups(root, _cfg.BackupKeep - 1);
        _sessionDir = Path.Combine(root, DateTime.Now.ToString("yyyy-MM-dd_HH-mm-ss"));
        Directory.CreateDirectory(_sessionDir);
        return _sessionDir;
    }

    private static void PruneOldBackups(string root, int keep)
    {
        try
        {
            if (!Directory.Exists(root)) return;
            var dirs = new DirectoryInfo(root).GetDirectories().OrderBy(d => d.Name, StringComparer.Ordinal).ToList();
            int remove = dirs.Count - Math.Max(0, keep);
            for (int i = 0; i < remove; i++) try { dirs[i].Delete(recursive: true); } catch { }
        }
        catch { }
    }

    private static void AtomicReplace(string tmp, string dest)
    {
        if (File.Exists(dest)) File.Replace(tmp, dest, null, ignoreMetadataErrors: true);
        else File.Move(tmp, dest);
    }

    private static void TryDelete(string p) { try { if (File.Exists(p)) File.Delete(p); } catch { } }
}

/// <summary>Caches SHA-256 by (size, mtime) so big .nx files aren't re-hashed each launch.</summary>
public sealed class HashCache
{
    private sealed class Entry { public long Size { get; set; } public long Mtime { get; set; } public string Sha { get; set; } = ""; }
    private readonly string _path;
    private Dictionary<string, Entry> _map = new(StringComparer.OrdinalIgnoreCase);
    private bool _dirty;

    private HashCache(string path) => _path = path;

    public static HashCache Load(string baseDir)
    {
        var c = new HashCache(Path.Combine(baseDir, ".augur_hashcache.json"));
        try { if (File.Exists(c._path)) c._map = JsonSerializer.Deserialize<Dictionary<string, Entry>>(File.ReadAllText(c._path)) ?? new(StringComparer.OrdinalIgnoreCase); }
        catch { }
        return c;
    }

    public string GetOrCompute(string path, FileInfo fi)
    {
        long mtime = fi.LastWriteTimeUtc.Ticks;
        if (_map.TryGetValue(path, out var e) && e.Size == fi.Length && e.Mtime == mtime) return e.Sha;
        string sha = Sha256File(path);
        _map[path] = new Entry { Size = fi.Length, Mtime = mtime, Sha = sha };
        _dirty = true;
        return sha;
    }

    public void Update(string path)
    {
        try { var fi = new FileInfo(path); _map[path] = new Entry { Size = fi.Length, Mtime = fi.LastWriteTimeUtc.Ticks, Sha = Sha256File(path) }; _dirty = true; }
        catch { }
    }

    public void Save() { if (!_dirty) return; try { File.WriteAllText(_path, JsonSerializer.Serialize(_map)); _dirty = false; } catch { } }

    public static string Sha256File(string path)
    {
        using var sha = SHA256.Create();
        using var fs = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read, 1 << 20, FileOptions.SequentialScan);
        return Convert.ToHexString(sha.ComputeHash(fs)).ToLowerInvariant();
    }
}

namespace AugurLauncher;

/// <summary>Reads config.ini (next to the launcher exe) with sensible defaults.</summary>
public sealed class LauncherConfig
{
    public string BaseDir { get; init; } = AppContext.BaseDirectory;

    // [paths]
    public string ClientDataDir { get; set; } = ".";
    public string ClientInstallDir { get; set; } = ".";
    public string ClientExe { get; set; } = "MapleStory.exe";
    public string ClientSettingsFile { get; set; } = "Settings"; // the client's Settings file

    // [server]
    public string DataManifestUrl { get; set; } = "https://72.60.176.12/updates/manifest.json";
    public string ClientManifestUrl { get; set; } = "https://72.60.176.12/updates/client/manifest.json";
    public string FeedUrl { get; set; } = "https://72.60.176.12/updates/feed.json"; // GenAI live feed
    public int FeedRefreshSeconds { get; set; } = 45;
    public string NewsUrl { get; set; } = "https://genms.fun/api/launcher/news"; // live GM activity (dashboard)
    public string LauncherManifestUrl { get; set; } = "https://72.60.176.12/updates/launcher.json"; // self-update
    // Live game-server status probe (login port). Host defaults to the client's
    // configured ServerIP; GameHost here is only the fallback.
    public string GameHost { get; set; } = "72.60.176.12";
    public int GamePort { get; set; } = 8484;
    public int StatusPollSeconds { get; set; } = 15;

    // [backup]
    public string BackupDir { get; set; } = "backups";
    public int BackupKeep { get; set; } = 5; // number of update backups to retain

    /// <summary>scheme://host[:port] the NX/data updates are downloaded from.</summary>
    public string UpdateOrigin =>
        Uri.TryCreate(DataManifestUrl, UriKind.Absolute, out var u) ? u.GetLeftPart(UriPartial.Authority) : "";

    /// <summary>Normalize "host", "host:port" or "https://host[:port]" to an http(s) origin.</summary>
    public static bool TryParseOrigin(string input, out string origin)
    {
        origin = "";
        input = (input ?? "").Trim().TrimEnd('/');
        if (input.Length == 0) return false;
        if (!input.Contains("://")) input = "https://" + input;
        if (!Uri.TryCreate(input, UriKind.Absolute, out var u) ||
            (u.Scheme != Uri.UriSchemeHttp && u.Scheme != Uri.UriSchemeHttps) ||
            u.Host.Length == 0)
            return false;
        origin = u.GetLeftPart(UriPartial.Authority);
        return true;
    }

    /// <summary>
    /// Point every update URL (data/client/feed/launcher manifests) at a new
    /// server, keeping each URL's path. NewsUrl (the dashboard) is untouched.
    /// </summary>
    public bool TrySetUpdateServer(string input)
    {
        if (!TryParseOrigin(input, out var baseUrl))
            return false;

        DataManifestUrl = Rebase(DataManifestUrl, baseUrl, "/updates/manifest.json");
        ClientManifestUrl = Rebase(ClientManifestUrl, baseUrl, "/updates/client/manifest.json");
        FeedUrl = Rebase(FeedUrl, baseUrl, "/updates/feed.json");
        LauncherManifestUrl = Rebase(LauncherManifestUrl, baseUrl, "/updates/launcher.json");
        return true;
    }

    private static string Rebase(string url, string baseUrl, string defaultPath) =>
        baseUrl + (Uri.TryCreate(url, UriKind.Absolute, out var u) ? u.PathAndQuery : defaultPath);

    /// <summary>Persist the current update URLs into config.ini, keeping every other line.</summary>
    public void SaveUpdateServer()
    {
        var path = Path.Combine(BaseDir, "config.ini");
        var lines = File.Exists(path) ? File.ReadAllLines(path).ToList() : new List<string>();

        // find the [server] section (or create it)
        int serverStart = -1, serverEnd = lines.Count;
        for (int i = 0; i < lines.Count; i++)
        {
            var t = lines[i].Trim();
            if (t.StartsWith('[') && t.EndsWith(']'))
            {
                if (serverStart >= 0) { serverEnd = i; break; }
                if (t[1..^1].Trim().Equals("server", StringComparison.OrdinalIgnoreCase))
                    serverStart = i;
            }
        }
        if (serverStart < 0)
        {
            lines.Add("[server]");
            serverStart = lines.Count - 1;
            serverEnd = lines.Count;
        }

        var keys = new (string Key, string Value)[]
        {
            ("DataManifestUrl", DataManifestUrl),
            ("ClientManifestUrl", ClientManifestUrl),
            ("FeedUrl", FeedUrl),
            ("LauncherManifestUrl", LauncherManifestUrl),
        };

        foreach (var (key, value) in keys)
        {
            int found = -1;
            for (int i = serverStart + 1; i < serverEnd; i++)
            {
                var t = lines[i].TrimStart();
                int eq = t.IndexOf('=');
                if (eq > 0 && t[..eq].Trim().Equals(key, StringComparison.OrdinalIgnoreCase)) { found = i; break; }
            }
            string line = $"{key} = {value}";
            if (found >= 0) lines[found] = line;
            else { lines.Insert(serverStart + 1, line); serverEnd++; }
        }

        File.WriteAllLines(path, lines);
    }

    public string DataDirFull => ResolveDir(ClientDataDir);
    public string InstallDirFull => ResolveDir(ClientInstallDir);
    public string BackupDirFull => ResolveDir(BackupDir);
    public string ClientExeFull => Path.GetFullPath(Path.Combine(InstallDirFull, ClientExe));
    public string ClientSettingsFull => Path.GetFullPath(Path.Combine(InstallDirFull, ClientSettingsFile));

    private string ResolveDir(string p) =>
        Path.GetFullPath(Path.Combine(BaseDir, string.IsNullOrWhiteSpace(p) ? "." : p));

    public static LauncherConfig Load(string baseDir)
    {
        var cfg = new LauncherConfig { BaseDir = baseDir };
        var path = Path.Combine(baseDir, "config.ini");
        if (!File.Exists(path))
            return cfg; // defaults are fine

        string section = "";
        foreach (var raw in File.ReadAllLines(path))
        {
            var line = StripComment(raw).Trim();
            if (line.Length == 0) continue;

            if (line.StartsWith('[') && line.EndsWith(']'))
            {
                section = line[1..^1].Trim().ToLowerInvariant();
                continue;
            }

            int eq = line.IndexOf('=');
            if (eq <= 0) continue;
            var key = line[..eq].Trim().ToLowerInvariant();
            var val = line[(eq + 1)..].Trim();

            switch (section, key)
            {
                case ("paths", "clientdatadir"): cfg.ClientDataDir = val; break;
                case ("paths", "clientinstalldir"): cfg.ClientInstallDir = val; break;
                case ("paths", "clientexe"): cfg.ClientExe = val; break;
                case ("paths", "clientsettingsfile"): cfg.ClientSettingsFile = val; break;
                case ("server", "datamanifesturl"): cfg.DataManifestUrl = val; break;
                case ("server", "clientmanifesturl"): cfg.ClientManifestUrl = val; break;
                case ("server", "feedurl"): cfg.FeedUrl = val; break;
                case ("server", "newsurl"): cfg.NewsUrl = val; break;
                case ("server", "launchermanifesturl"): cfg.LauncherManifestUrl = val; break;
                case ("server", "feedrefreshseconds"): if (int.TryParse(val, out var fr)) cfg.FeedRefreshSeconds = Math.Max(10, fr); break;
                case ("server", "gamehost"): cfg.GameHost = val; break;
                case ("server", "gameport"): if (int.TryParse(val, out var gp)) cfg.GamePort = gp; break;
                case ("server", "statuspollseconds"): if (int.TryParse(val, out var sp)) cfg.StatusPollSeconds = Math.Max(5, sp); break;
                case ("backup", "dir"): cfg.BackupDir = val; break;
                case ("backup", "keep"): if (int.TryParse(val, out var k)) cfg.BackupKeep = Math.Max(0, k); break;
            }
        }
        return cfg;
    }

    // Strip a trailing ';' comment, but not one inside a URL/value quote.
    private static string StripComment(string s)
    {
        int i = s.IndexOf(';');
        return i >= 0 ? s[..i] : s;
    }
}

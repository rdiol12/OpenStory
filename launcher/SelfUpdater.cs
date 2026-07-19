using System.Diagnostics;

namespace AugurLauncher;

/// <summary>
/// Updates the launcher's own exe. Windows allows renaming a running .exe (just
/// not overwriting the open image), so we: download the new build → move the
/// running exe aside → move the new one into place → relaunch it. On next start
/// the leftover ".old" file is cleaned up.
/// </summary>
public static class SelfUpdater
{
    private static string ExePath => Environment.ProcessPath ?? Path.Combine(AppContext.BaseDirectory, "GenMs.exe");
    private static string OldPath => ExePath + ".old";

    /// <summary>Delete the aside copy left by a previous self-update. Call on startup.</summary>
    public static void CleanupOld()
    {
        try { if (File.Exists(OldPath)) File.Delete(OldPath); } catch { /* still locked; next launch */ }
    }

    /// <summary>True if the server's published launcher differs from the one running.</summary>
    public static bool NeedsUpdate(LauncherRelease rel)
    {
        if (string.IsNullOrWhiteSpace(rel.Sha256)) return false;
        try
        {
            string mine = HashCache.Sha256File(ExePath);
            return !string.Equals(mine, rel.Sha256, StringComparison.OrdinalIgnoreCase);
        }
        catch { return false; }
    }

    /// <summary>
    /// Swap in the already-downloaded new exe (verified) and relaunch it.
    /// Returns true if the swap succeeded and the caller should shut down.
    /// </summary>
    public static bool ApplyAndRelaunch(string downloadedExe)
    {
        try
        {
            if (File.Exists(OldPath)) File.Delete(OldPath);
            File.Move(ExePath, OldPath);          // move the running exe aside (allowed on Windows)
            File.Move(downloadedExe, ExePath);    // put the new one where it belongs

            Process.Start(new ProcessStartInfo
            {
                FileName = ExePath,
                UseShellExecute = true,
                WorkingDirectory = AppContext.BaseDirectory,
            });
            return true;
        }
        catch
        {
            // Roll back so the launcher still works this session.
            try { if (!File.Exists(ExePath) && File.Exists(OldPath)) File.Move(OldPath, ExePath); } catch { }
            return false;
        }
    }
}

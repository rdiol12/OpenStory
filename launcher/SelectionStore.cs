using System.Text.Json;

namespace AugurLauncher;

/// <summary>
/// Remembers the player's per-file download choices across launches
/// (.augur_selection.json next to the launcher). Keyed by manifest file name.
/// </summary>
public sealed class SelectionStore
{
    private readonly string _path;
    private Dictionary<string, bool> _map = new(StringComparer.OrdinalIgnoreCase);

    private SelectionStore(string path) => _path = path;

    public static SelectionStore Load(string baseDir)
    {
        var s = new SelectionStore(Path.Combine(baseDir, ".augur_selection.json"));
        try
        {
            if (File.Exists(s._path))
                s._map = JsonSerializer.Deserialize<Dictionary<string, bool>>(File.ReadAllText(s._path))
                         ?? new(StringComparer.OrdinalIgnoreCase);
        }
        catch { /* corrupt file -> defaults */ }
        return s;
    }

    /// <summary>Whether this file is chosen for download (defaults to <paramref name="defaultValue"/>).</summary>
    public bool IsSelected(string name, bool defaultValue) =>
        _map.TryGetValue(name, out var v) ? v : defaultValue;

    public void Set(string name, bool selected) => _map[name] = selected;

    public void Save()
    {
        try { File.WriteAllText(_path, JsonSerializer.Serialize(_map)); }
        catch { /* non-fatal */ }
    }
}

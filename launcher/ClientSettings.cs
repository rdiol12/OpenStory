namespace AugurLauncher;

/// <summary>
/// Reads/writes the game client's "Settings" file (Key = Value lines with
/// '#'-comment section headers). Only known keys are touched; the file's
/// order, comments and spacing are preserved.
/// </summary>
public sealed class ClientSettings
{
    private readonly string _path;
    private readonly List<string> _lines;

    public ClientSettings(string path)
    {
        _path = path;
        _lines = File.Exists(path) ? File.ReadAllLines(path).ToList() : new List<string>();
    }

    public bool FileExists => File.Exists(_path);

    private static bool IsKeyLine(string line, string key, out int eq)
    {
        eq = line.IndexOf('=');
        if (eq <= 0) return false;
        if (line.TrimStart().StartsWith('#') || line.TrimStart().StartsWith(';')) return false;
        return string.Equals(line[..eq].Trim(), key, StringComparison.OrdinalIgnoreCase);
    }

    public string? Get(string key)
    {
        foreach (var line in _lines)
            if (IsKeyLine(line, key, out int eq))
                return line[(eq + 1)..].Trim();
        return null;
    }

    public void Set(string key, string value)
    {
        for (int i = 0; i < _lines.Count; i++)
        {
            if (IsKeyLine(_lines[i], key, out int eq))
            {
                _lines[i] = $"{_lines[i][..eq].TrimEnd()} = {value}";
                return;
            }
        }
        _lines.Add($"{key} = {value}"); // not present -> append
    }

    public int GetInt(string key, int def) => int.TryParse(Get(key), out var v) ? v : def;
    public bool GetBool(string key, bool def) => bool.TryParse(Get(key), out var v) ? v : def;
    public void SetInt(string key, int v) => Set(key, v.ToString());
    public void SetBool(string key, bool v) => Set(key, v ? "true" : "false");

    public void Save() => File.WriteAllLines(_path, _lines);
}

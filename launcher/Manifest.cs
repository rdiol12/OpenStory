using System.Text.Json;
using System.Text.Json.Serialization;

namespace AugurLauncher;

/// <summary>One entry in an update manifest.</summary>
public sealed class ManifestFile
{
    // Relative path (may contain sub-folders for the client manifest).
    public string Name { get; set; } = "";
    // Absolute download URL. If empty/relative it is resolved against BaseUrl.
    public string Url { get; set; } = "";
    public long Size { get; set; }
    public string Sha256 { get; set; } = "";
}

/// <summary>A whole manifest (game-data or client-build).</summary>
public sealed class Manifest
{
    public string? GeneratedAt { get; set; }
    public string? BaseUrl { get; set; }
    public List<ManifestFile> Files { get; set; } = new();

    private static readonly JsonSerializerOptions Opts = new()
    {
        PropertyNameCaseInsensitive = true,
        NumberHandling = JsonNumberHandling.AllowReadingFromString,
    };

    public static Manifest Parse(string json)
    {
        var m = JsonSerializer.Deserialize<Manifest>(json, Opts)
                ?? throw new InvalidDataException("manifest was empty");
        m.Files ??= new();
        return m;
    }

    /// <summary>Resolve a file's absolute download URL (handles relative urls).</summary>
    public string ResolveUrl(ManifestFile f)
    {
        if (!string.IsNullOrWhiteSpace(f.Url) &&
            (f.Url.StartsWith("http://", StringComparison.OrdinalIgnoreCase) ||
             f.Url.StartsWith("https://", StringComparison.OrdinalIgnoreCase)))
            return f.Url;

        var baseUrl = (BaseUrl ?? "").TrimEnd('/');
        var rel = (string.IsNullOrWhiteSpace(f.Url) ? f.Name : f.Url).Replace('\\', '/').TrimStart('/');
        return baseUrl.Length > 0 ? $"{baseUrl}/{rel}" : rel;
    }
}

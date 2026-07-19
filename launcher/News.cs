using System.Globalization;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace AugurLauncher;

/// <summary>One patch-note / announcement entry.</summary>
public sealed class NewsItem
{
    public DateTimeOffset? Date { get; set; }
    public string Title { get; set; } = "";
    public string Body { get; set; } = "";
    public string Tag { get; set; } = ""; // e.g. "Patch", "Event", "Notice"

    [JsonIgnore] public string DateText => Date?.ToLocalTime().ToString("MMM d, yyyy", CultureInfo.InvariantCulture) ?? "";
    [JsonIgnore] public bool HasTag => !string.IsNullOrWhiteSpace(Tag);
}

/// <summary>The news document served at NewsUrl.</summary>
public sealed class News
{
    public List<NewsItem> Items { get; set; } = new();

    private static readonly JsonSerializerOptions Opts = new()
    {
        PropertyNameCaseInsensitive = true,
        NumberHandling = JsonNumberHandling.AllowReadingFromString,
    };

    public static News Parse(string json)
    {
        var n = JsonSerializer.Deserialize<News>(json, Opts) ?? new News();
        n.Items ??= new();
        return n;
    }
}

/// <summary>Self-update descriptor served at LauncherManifestUrl (launcher.json).</summary>
public sealed class LauncherRelease
{
    public string Sha256 { get; set; } = "";  // hash of the current published GenMs.exe
    public string Url { get; set; } = "";      // where to download it
    public string Version { get; set; } = "";  // optional, informational
}

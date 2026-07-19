using System.Text.Json;
using System.Text.Json.Serialization;

namespace AugurLauncher;

/// <summary>One line in the GenAI live feed (a rate change, event, drop tweak…).</summary>
public sealed class FeedEntry
{
    public DateTimeOffset? Time { get; set; }
    public string Type { get; set; } = "info";   // rate | drop | event | boss | info
    public string Title { get; set; } = "";
    public string Text { get; set; } = "";

    // ---- display helpers (bound by the UI) ----
    [JsonIgnore] public string Icon => Type.ToLowerInvariant() switch
    {
        "rate" => "⚡",   // ⚡
        "drop" => "🎁", // 🎁
        "event" => "🎉", // 🎉
        "boss" => "⚔",   // ⚔
        _ => "◆",        // ◆
    };

    [JsonIgnore] public string Accent => Type.ToLowerInvariant() switch
    {
        "rate" => "#E0A82E",   // amber
        "drop" => "#3E9E5A", // green
        "event" => "#E07B39",  // maple orange
        "boss" => "#C64B3A",   // red
        _ => "#8A7B63",        // muted ink
    };

    [JsonIgnore] public string Heading => string.IsNullOrWhiteSpace(Title)
        ? Type.ToUpperInvariant() : Title;

    [JsonIgnore] public string When => Relative(Time);

    private static string Relative(DateTimeOffset? t)
    {
        if (t is null) return "";
        var d = DateTimeOffset.UtcNow - t.Value;
        if (d.TotalSeconds < 60) return "just now";
        if (d.TotalMinutes < 60) return $"{(int)d.TotalMinutes}m ago";
        if (d.TotalHours < 24) return $"{(int)d.TotalHours}h ago";
        return $"{(int)d.TotalDays}d ago";
    }
}

/// <summary>The whole feed document served at FeedUrl.</summary>
public sealed class Feed
{
    public List<FeedEntry> Entries { get; set; } = new();

    private static readonly JsonSerializerOptions Opts = new()
    {
        PropertyNameCaseInsensitive = true,
        NumberHandling = JsonNumberHandling.AllowReadingFromString,
    };

    public static Feed Parse(string json)
    {
        var f = JsonSerializer.Deserialize<Feed>(json, Opts) ?? new Feed();
        f.Entries ??= new();
        return f;
    }
}

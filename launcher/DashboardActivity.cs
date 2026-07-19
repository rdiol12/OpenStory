using System.Text.Json;
using System.Text.Json.Serialization;

namespace AugurLauncher;

/// <summary>One GM action from the dashboard's /api/launcher/news endpoint.</summary>
public sealed class DashActivity
{
    public string Type { get; set; } = "update";   // update | rates | drops | event
    public string Text { get; set; } = "";
    public DateTimeOffset? Date { get; set; }
    public string? Tool { get; set; }
    public DashDetail? Detail { get; set; }

    [JsonIgnore] public string Summary => Detail?.Summary ?? "";

    // --- map the dashboard category onto the launcher's feed/news vocabulary ---
    [JsonIgnore] public string FeedType => Type.ToLowerInvariant() switch
    {
        "rates" => "rate",
        "drops" => "drop",
        "event" => "event",
        "boss" => "boss",
        _ => "info",
    };

    [JsonIgnore] public string Tag => (Tool ?? "").ToLowerInvariant() switch
    {
        "update_rates" => "Rates",
        "add_mob_drop" or "remove_mob_drop" or "batch_update_drops" or "add_reactor_drop" => "Drops",
        "create_event" or "add_map_spawn" or "add_map_reactor" => "Event",
        "propose_content" => "Proposed",
        "delegate_code_change" => "Dev",
        "update_goal" => "Goal",
        _ => Type.ToLowerInvariant() switch { "rates" => "Rates", "drops" => "Drops", "event" => "Event", _ => "GM" },
    };
}

public sealed class DashDetail
{
    public string? Summary { get; set; }
}

/// <summary>The /api/launcher/news response: { "news": [ ... ] }.</summary>
public sealed class DashResponse
{
    public List<DashActivity> News { get; set; } = new();

    private static readonly JsonSerializerOptions Opts = new()
    {
        PropertyNameCaseInsensitive = true,
        NumberHandling = JsonNumberHandling.AllowReadingFromString,
    };

    public static List<DashActivity> Parse(string json)
    {
        var r = JsonSerializer.Deserialize<DashResponse>(json, Opts);
        return r?.News ?? new();
    }
}

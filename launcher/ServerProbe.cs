using System.Net.Sockets;

namespace AugurLauncher;

/// <summary>
/// Lightweight "is the game server up?" check — opens a TCP connection to the
/// login port and reports whether it accepts. No data is sent.
/// </summary>
public static class ServerProbe
{
    public static async Task<bool> IsOnlineAsync(string host, int port, int timeoutMs, CancellationToken ct)
    {
        if (string.IsNullOrWhiteSpace(host) || port <= 0) return false;
        try
        {
            using var client = new TcpClient();
            using var cts = CancellationTokenSource.CreateLinkedTokenSource(ct);
            cts.CancelAfter(timeoutMs);
            await client.ConnectAsync(host, port, cts.Token);
            return client.Connected;
        }
        catch
        {
            return false;
        }
    }
}

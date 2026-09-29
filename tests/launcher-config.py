#!/usr/bin/env python3
"""Check launcher defaults, local configuration, and standard TLS validation."""
from pathlib import Path
import subprocess
import tempfile
from xml.sax.saxutils import escape

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='openstory-launcher-') as directory:
    work = Path(directory)
    project = escape(str(root / 'launcher/AugurLauncher.csproj'))
    (work / 'check.csproj').write_text(f'''<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup><OutputType>Exe</OutputType><TargetFramework>net8.0-windows</TargetFramework>
  <UseWPF>true</UseWPF><ImplicitUsings>enable</ImplicitUsings>
  <RuntimeIdentifier>win-x64</RuntimeIdentifier><SelfContained>true</SelfContained></PropertyGroup>
  <ItemGroup><ProjectReference Include="{project}" /></ItemGroup>
</Project>''')
    (work / 'Program.cs').write_text(r'''
using AugurLauncher;
using System.Net.Http;
using System.Reflection;
static void Check(bool condition, string message) {
    if (!condition) throw new Exception(message);
}
var config = LauncherConfig.Load(Environment.CurrentDirectory);
Check(config.GameHost == "127.0.0.1", "Default game endpoint must be loopback");
foreach (var url in new[] { config.DataManifestUrl, config.ClientManifestUrl,
    config.FeedUrl, config.NewsUrl, config.LauncherManifestUrl })
    Check(new Uri(url).Host.EndsWith(".example"), "Default URLs must be examples");
Check(config.TrySetUpdateServer("https://updates.test:9443"), "Custom origin rejected");
Check(config.UpdateOrigin == "https://updates.test:9443", "Custom origin lost");
Check(config.LauncherManifestUrl == "https://updates.test:9443/updates/launcher.json",
      "Self-update origin did not follow configuration");
config.SaveUpdateServer();
var loaded = LauncherConfig.Load(Environment.CurrentDirectory);
Check(loaded.DataManifestUrl == config.DataManifestUrl, "Local configuration lost");
var updater = new Updater(loaded);
var http = (HttpClient)typeof(Updater).GetField("_http", BindingFlags.NonPublic | BindingFlags.Instance).GetValue(updater);
var handler = (SocketsHttpHandler)typeof(HttpMessageInvoker).GetField("_handler", BindingFlags.NonPublic | BindingFlags.Instance).GetValue(http);
Check(handler.SslOptions.RemoteCertificateValidationCallback is null,
      "Updater must use platform certificate validation");
http.Dispose();
Console.WriteLine("PASS: example defaults, local configuration and standard TLS validation");
''')
    subprocess.run(['dotnet', 'run', '--project', str(work / 'check.csproj'),
                    '--configuration', 'Release'], cwd=work, check=True)

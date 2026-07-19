using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Media.Imaging;
using System.Windows.Threading;

namespace AugurLauncher;

public partial class MainWindow : Window
{
    private enum UiState { Checking, Choosing, Ready, Offline }

    private readonly LauncherConfig _cfg;
    private readonly CancellationTokenSource _cts = new();
    private readonly ObservableCollection<PendingFile> _pending = new();
    private readonly ObservableCollection<FeedEntry> _feed = new();          // GenAI log (right panel)
    private readonly ObservableCollection<NewsItem> _news = new();           // patch notes (right panel)
    private DispatcherTimer? _feedTimer;
    private DispatcherTimer? _statusTimer;
    private SelectionStore _selection = null!;
    private Updater _updater = null!;
    private Storyboard? _pulse;
    private UiState _state = UiState.Checking;

    public MainWindow()
    {
        InitializeComponent();
        _cfg = LauncherConfig.Load(AppContext.BaseDirectory);
        FileList.ItemsSource = _pending;
        FeedList.ItemsSource = _feed;
        NewsList.ItemsSource = _news;
        Loaded += OnLoaded;
        Closing += (_, _) => _cts.Cancel();
    }

    private async void OnLoaded(object sender, RoutedEventArgs e)
    {
        LoadLogo();
        StartLeaves();
        VersionText.Text = "GenMs Launcher  v1.0";
        LoadFullscreenState();
        _selection = SelectionStore.Load(AppContext.BaseDirectory);
        _updater = new Updater(_cfg);

        SelfUpdater.CleanupOld();
        StartServerStatus();
        StartLive();

        // Self-update the launcher itself before checking game files.
        if (await MaybeSelfUpdateAsync()) return; // relaunching with the new build

        var progress = new Progress<ProgressState>(ApplyProgress);
        try
        {
            var result = await _updater.CheckAsync(_selection, progress, _cts.Token);
            OnChecked(result);
        }
        catch (OperationCanceledException) { }
        catch (Exception ex)
        {
            StatusText.Text = "Something went wrong while checking for updates.";
            ShowError(ex.Message);
            EnterReady(offline: true);
        }
    }

    // ==== check finished ====
    private void OnChecked(CheckResult r)
    {
        // Note: the status dot is driven by the live game-server probe, not the
        // update-server reachability — see ProbeServerAsync.
        if (r.Server == ServerState.Offline)
        {
            StatusText.Text = "Update server unreachable.";
            if (r.Error != null) ShowError(r.Error);
            EnterReady(offline: true);
            return;
        }

        if (r.Pending.Count == 0)
        {
            StatusText.Text = "Up to date — ready to play!";
            Progress.Value = 100;
            EnterReady(offline: false);
            return;
        }

        // there are updates — let the player choose
        foreach (var p in r.Pending)
        {
            p.PropertyChanged += OnFileToggled;
            _pending.Add(p);
        }
        EnterChoosing();
    }

    private void EnterChoosing()
    {
        _state = UiState.Choosing;
        long total = _pending.Sum(p => p.Size);
        StatusText.Text = $"{_pending.Count} update(s) available — choose what to download.";
        PickerPanel.Visibility = Visibility.Visible;
        SkipBtn.Visibility = Visibility.Visible;
        VerifyBtn.Visibility = Visibility.Collapsed;
        ActionBtn.Content = "⬇  DOWNLOAD";
        ActionBtn.IsEnabled = true;
        RefreshSummary();
    }

    private void EnterReady(bool offline)
    {
        _state = offline ? UiState.Offline : UiState.Ready;
        PickerPanel.Visibility = Visibility.Collapsed;
        SkipBtn.Visibility = Visibility.Collapsed;
        VerifyBtn.Visibility = offline ? Visibility.Collapsed : Visibility.Visible;
        FileText.Text = "";
        SpeedText.Text = "";
        ActionBtn.Content = "▶  PLAY";
        ActionBtn.IsEnabled = true;
    }

    // ==== picker interactions ====
    private void OnFileToggled(object? sender, PropertyChangedEventArgs e) => RefreshSummary();

    private void SelectAll_Click(object sender, RoutedEventArgs e)
    {
        bool on = SelectAll.IsChecked == true;
        foreach (var p in _pending) p.Selected = on;
        RefreshSummary();
    }

    private void RefreshSummary()
    {
        int sel = _pending.Count(p => p.Selected);
        long bytes = _pending.Where(p => p.Selected).Sum(p => p.Size);
        SelSummary.Text = $"{sel} of {_pending.Count} · {PendingFile.FormatSize(bytes)}";
        SelectAll.IsChecked = sel == _pending.Count && sel > 0;
        if (_state == UiState.Choosing)
            ActionBtn.Content = sel > 0 ? "⬇  DOWNLOAD" : "▶  PLAY";
    }

    private void SaveSelections()
    {
        foreach (var p in _pending) _selection.Set(p.Name, p.Selected);
        _selection.Save();
    }

    // ==== primary action ====
    private async void Action_Click(object sender, RoutedEventArgs e)
    {
        if (_state == UiState.Choosing)
        {
            SaveSelections();
            var chosen = _pending.Where(p => p.Selected).ToList();
            if (chosen.Count == 0) { EnterReady(offline: false); return; }
            await DownloadAsync(chosen);
        }
        else
        {
            LaunchGame();
        }
    }

    private void Skip_Click(object sender, RoutedEventArgs e)
    {
        SaveSelections();               // remember the unchecked items for next time
        StatusText.Text = "Skipped updates — playing with current files.";
        EnterReady(offline: false);
    }

    private async Task DownloadAsync(System.Collections.Generic.List<PendingFile> chosen)
    {
        _state = UiState.Ready;
        PickerPanel.Visibility = Visibility.Collapsed;
        SkipBtn.Visibility = Visibility.Collapsed;
        VerifyBtn.Visibility = Visibility.Collapsed;
        ActionBtn.IsEnabled = false;
        ActionBtn.Content = "⬇  DOWNLOADING…";
        HideError();

        var progress = new Progress<ProgressState>(ApplyProgress);
        try
        {
            await _updater.DownloadAsync(chosen, progress, _cts.Token);
            StatusText.Text = "Update complete — ready to play!";
        }
        catch (OperationCanceledException) { return; }
        catch (Exception ex)
        {
            StatusText.Text = "Some files failed to download.";
            ShowError(ex.Message + "  You can still play or retry.");
        }
        EnterReady(offline: false);
    }

    // ==== progress -> UI ====
    private void ApplyProgress(ProgressState s)
    {
        StatusText.Text = s.Status;
        Progress.Value = Math.Clamp(s.OverallPercent, 0, 100);
        FileText.Text = string.IsNullOrEmpty(s.CurrentFile) ? "" : $"{s.CurrentFile}  —  {s.FilePercent:0}%";
        SpeedText.Text = s.SpeedBps > 1 ? FormatSpeed(s.SpeedBps) : "";
    }

    private void SetServer(ServerState state)
    {
        switch (state)
        {
            case ServerState.Online:
                StatusDot.Fill = (Brush)FindResource("Green");
                ServerText.Text = "Server Online";
                ServerText.Foreground = (Brush)FindResource("Green");
                StartPulse();
                break;
            case ServerState.Offline:
                StopPulse();
                StatusDot.Fill = (Brush)FindResource("Red");
                ServerText.Text = "Server Offline";
                ServerText.Foreground = (Brush)FindResource("Red");
                break;
            default:
                StatusDot.Fill = (Brush)FindResource("Grey");
                ServerText.Text = "Connecting…";
                ServerText.Foreground = (Brush)FindResource("InkSecondary");
                break;
        }
    }

    // ==== live game-server status (TCP probe of the login port) ====
    private void StartServerStatus()
    {
        _ = ProbeServerAsync();
        _statusTimer = new DispatcherTimer { Interval = TimeSpan.FromSeconds(Math.Max(5, _cfg.StatusPollSeconds)) };
        _statusTimer.Tick += async (_, _) => await ProbeServerAsync();
        _statusTimer.Start();
    }

    private async Task ProbeServerAsync()
    {
        // Probe exactly the endpoint the client will connect to: ServerIP/ServerPort
        // from the client's Settings, falling back to config.
        string host = _cfg.GameHost;
        int port = _cfg.GamePort;
        try
        {
            var s = new ClientSettings(_cfg.ClientSettingsFull);
            var ip = s.Get("ServerIP");
            if (!string.IsNullOrWhiteSpace(ip)) host = ip.Trim();
            int p = s.GetInt("ServerPort", 0);
            if (p > 0) port = p;
        }
        catch { }

        bool online = await ServerProbe.IsOnlineAsync(host, port, 5000, _cts.Token);
        SetServer(online ? ServerState.Online : ServerState.Offline);
    }

    // ==== Live GM activity: drives both News (detailed) and GenAI's Log (compact) ====
    private void StartLive()
    {
        _ = RefreshLiveAsync();
        _feedTimer = new DispatcherTimer { Interval = TimeSpan.FromSeconds(Math.Max(10, _cfg.FeedRefreshSeconds)) };
        _feedTimer.Tick += async (_, _) => await RefreshLiveAsync();
        _feedTimer.Start();
    }

    private async Task RefreshLiveAsync()
    {
        List<DashActivity> activity;
        List<FeedEntry> curated;
        try
        {
            activity = await _updater.FetchActivityAsync(_cts.Token);   // dashboard GM activity
            curated = await _updater.FetchFeedAsync(_cts.Token);        // curated feed.json (may be empty)
        }
        catch { return; }

        // News & Patch Notes: activity as detailed cards (headline + the GM's reasoning).
        _news.Clear();
        foreach (var a in activity)
            _news.Add(new NewsItem { Date = a.Date, Tag = a.Tag, Title = a.Text, Body = a.Summary });
        NewsEmpty.Visibility = _news.Count > 0 ? Visibility.Collapsed : Visibility.Visible;

        // GenAI's Log: curated live-ops announcements + GM activity, newest first.
        var log = curated
            .Concat(activity.Select(a => new FeedEntry { Time = a.Date, Type = a.FeedType, Title = a.Tag, Text = a.Text }))
            .OrderByDescending(e => e.Time ?? DateTimeOffset.MinValue)
            .Take(40)
            .ToList();

        _feed.Clear();
        foreach (var e in log) _feed.Add(e);
        FeedEmpty.Visibility = _feed.Count > 0 ? Visibility.Collapsed : Visibility.Visible;
    }

    // ==== Repair / Verify: re-hash every managed file and re-download mismatches ====
    private async void Verify_Click(object sender, RoutedEventArgs e)
    {
        VerifyBtn.Visibility = Visibility.Collapsed;
        _pending.Clear();
        StatusText.Text = "Verifying game files…";
        Progress.Value = 0;
        ActionBtn.IsEnabled = false;
        HideError();

        var progress = new Progress<ProgressState>(ApplyProgress);
        try
        {
            var result = await _updater.CheckAsync(_selection, progress, _cts.Token, force: true);
            if (result.Server == ServerState.Offline)
            {
                StatusText.Text = "Couldn't reach the update server to verify.";
                EnterReady(offline: false);
                return;
            }
            if (result.Pending.Count == 0)
            {
                StatusText.Text = "All files verified — everything is intact. ✓";
                Progress.Value = 100;
                EnterReady(offline: false);
                return;
            }
            // Something is missing/corrupted — repair it all.
            StatusText.Text = $"Repairing {result.Pending.Count} file(s)…";
            foreach (var p in result.Pending) p.Selected = true;
            await DownloadAsync(result.Pending.ToList());
        }
        catch (OperationCanceledException) { }
        catch (Exception ex)
        {
            ShowError(ex.Message);
            EnterReady(offline: false);
        }
    }

    // ==== Launcher self-update (rename-swap of the running exe) ====
    private async Task<bool> MaybeSelfUpdateAsync()
    {
        LauncherRelease? rel;
        try { rel = await _updater.FetchLauncherReleaseAsync(_cts.Token); }
        catch { return false; }
        if (rel is null || !SelfUpdater.NeedsUpdate(rel)) return false;

        var choice = MessageBox.Show(
            "A launcher update is available. Update now?\n\nThe launcher will restart.",
            "GenMs", MessageBoxButton.YesNo, MessageBoxImage.Information);
        if (choice != MessageBoxResult.Yes) return false;

        try
        {
            StatusText.Text = "Updating launcher…";
            string url = !string.IsNullOrWhiteSpace(rel.Url)
                ? rel.Url
                : "https://72.60.176.12/updates/client/GenMs.exe";
            string newExe = await _updater.DownloadLauncherAsync(rel, url, _cts.Token);

            if (SelfUpdater.ApplyAndRelaunch(newExe))
            {
                Application.Current.Shutdown();
                return true;
            }
            StatusText.Text = "Launcher update failed — continuing with this version.";
            return false;
        }
        catch (Exception ex)
        {
            ShowError("Launcher update failed: " + ex.Message);
            return false;
        }
    }

    private void LaunchGame()
    {
        try
        {
            string exe = _cfg.ClientExeFull;
            if (!File.Exists(exe))
            {
                MessageBox.Show($"Could not find the game client:\n{exe}\n\nCheck ClientInstallDir / ClientExe in config.ini.",
                    "GenMs", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            Process.Start(new ProcessStartInfo { FileName = exe, WorkingDirectory = _cfg.InstallDirFull, UseShellExecute = true });
            _cts.Cancel();
            Application.Current.Shutdown();
        }
        catch (Exception ex)
        {
            MessageBox.Show("Failed to launch the game:\n\n" + ex.Message, "GenMs", MessageBoxButton.OK, MessageBoxImage.Error);
        }
    }

    // ==== animated drifting maple-leaf background ====
    private static readonly Point[] LeafPts = BuildLeaf();

    private static Point[] BuildLeaf()
    {
        (double x, double y)[] rp =
        {
            (50,4),(60,25),(71,23),(68,35),(83,39),(76,49),(90,57),(78,63),(73,78),(63,72),(60,88),(55,84),(55,100),(50,100)
        };
        var pts = new List<Point>();
        foreach (var p in rp) pts.Add(new Point(p.x, p.y));
        for (int i = rp.Length - 2; i >= 1; i--) pts.Add(new Point(100 - rp[i].x, rp[i].y));
        return pts.ToArray();
    }

    private static Geometry MakeLeafGeometry()
    {
        var fig = new PathFigure { StartPoint = LeafPts[0], IsClosed = true, IsFilled = true };
        for (int i = 1; i < LeafPts.Length; i++) fig.Segments.Add(new LineSegment(LeafPts[i], false));
        var g = new PathGeometry();
        g.Figures.Add(fig);
        g.Freeze();
        return g;
    }

    private void StartLeaves()
    {
        double w = Width, h = Height;
        LeafCanvas.Clip = new RectangleGeometry(new Rect(0, 0, w, h), 8, 8);
        var geo = MakeLeafGeometry();
        var rnd = new Random();

        // Dark (Nexon-style) theme: fewer, fainter leaves so the UI stays clean
        for (int i = 0; i < 8; i++)
        {
            double size = rnd.Next(14, 26);
            byte a = (byte)rnd.Next(16, 44);
            Color c = rnd.Next(3) == 0 ? Color.FromArgb(a, 0xf4, 0xc5, 0x42) : Color.FromArgb(a, 0xf6, 0x87, 0x1f);

            var leaf = new System.Windows.Shapes.Path
            {
                Data = geo,
                Fill = new SolidColorBrush(c),
                Width = size, Height = size, Stretch = Stretch.Uniform,
                RenderTransformOrigin = new Point(0.5, 0.5),
            };
            var rot = new RotateTransform();
            var move = new TranslateTransform();
            var tg = new TransformGroup();
            tg.Children.Add(rot);
            tg.Children.Add(move);
            leaf.RenderTransform = tg;
            Canvas.SetLeft(leaf, rnd.NextDouble() * w);
            LeafCanvas.Children.Add(leaf);

            double dur = rnd.Next(9, 18);
            var fall = new DoubleAnimation(-40, h + 40, TimeSpan.FromSeconds(dur))
            { RepeatBehavior = RepeatBehavior.Forever, BeginTime = TimeSpan.FromSeconds(-rnd.NextDouble() * dur) };
            move.BeginAnimation(TranslateTransform.YProperty, fall);

            double sway = rnd.Next(20, 60);
            var swayA = new DoubleAnimation(-sway, sway, TimeSpan.FromSeconds(rnd.Next(3, 6)))
            { AutoReverse = true, RepeatBehavior = RepeatBehavior.Forever, EasingFunction = new SineEase { EasingMode = EasingMode.EaseInOut } };
            move.BeginAnimation(TranslateTransform.XProperty, swayA);

            var spin = new DoubleAnimation(0, rnd.Next(2) == 0 ? 360 : -360, TimeSpan.FromSeconds(rnd.Next(6, 14)))
            { RepeatBehavior = RepeatBehavior.Forever };
            rot.BeginAnimation(RotateTransform.AngleProperty, spin);
        }
    }

    private void LoadLogo()
    {
        // Prefer a logo.png next to the exe (lets you rebrand without rebuilding),
        // otherwise use the copy embedded in the exe so a bare download still shows it.
        try
        {
            var path = Path.Combine(AppContext.BaseDirectory, "logo.png");
            if (File.Exists(path))
            {
                var bmp = new BitmapImage();
                bmp.BeginInit(); bmp.CacheOption = BitmapCacheOption.OnLoad; bmp.UriSource = new Uri(path); bmp.EndInit();
                LogoImage.Source = bmp;
                return;
            }
        }
        catch { }

        try
        {
            LogoImage.Source = new BitmapImage(new Uri("pack://application:,,,/logo.png"));
            return;
        }
        catch { }

        LogoImage.Visibility = Visibility.Collapsed;
        Wordmark.Visibility = Visibility.Visible;
    }

    private void StartPulse()
    {
        if (_pulse != null) return;
        var anim = new DoubleAnimation(1.0, 0.35, TimeSpan.FromSeconds(0.9))
        { AutoReverse = true, RepeatBehavior = RepeatBehavior.Forever, EasingFunction = new SineEase { EasingMode = EasingMode.EaseInOut } };
        _pulse = new Storyboard();
        _pulse.Children.Add(anim);
        Storyboard.SetTarget(anim, StatusDot);
        Storyboard.SetTargetProperty(anim, new PropertyPath(OpacityProperty));
        _pulse.Begin();
    }

    private void StopPulse() { _pulse?.Stop(); _pulse = null; StatusDot.Opacity = 1; }

    private void ShowError(string msg) { ErrorText.Text = msg; ErrorText.Visibility = Visibility.Visible; }
    private void HideError() { ErrorText.Visibility = Visibility.Collapsed; }

    private static string FormatSpeed(double bps)
    {
        string[] u = { "B/s", "KB/s", "MB/s", "GB/s" };
        int i = 0; while (bps >= 1024 && i < u.Length - 1) { bps /= 1024; i++; }
        return $"{bps:0.0} {u[i]}";
    }

    // ==== settings ====
    private void Settings_Click(object sender, RoutedEventArgs e)
    {
        var s = new ClientSettings(_cfg.ClientSettingsFull);

        ServerAddrBox.Text = s.Get("ServerIP") ?? "";
        ServerPortBox.Text = s.Get("ServerPort") ?? "";
        UpdateServerBox.Text = _cfg.UpdateOrigin;

        string res = $"{s.GetInt("Width", 1920)}x{s.GetInt("Height", 1080)}";
        foreach (var child in ResPanel.Children)
            if (child is RadioButton rb) rb.IsChecked = (rb.Tag as string) == res;

        FullscreenCheck.IsChecked = s.GetBool("Fullscreen", false);
        _syncingFs = true;
        FullscreenQuick.IsChecked = FullscreenCheck.IsChecked;   // keep the main-screen toggle in sync
        _syncingFs = false;
        VsyncCheck.IsChecked = s.GetBool("VSync", false);
        ShakeCheck.IsChecked = s.GetBool("ScreenShake", true);
        BgmSlider.Value = Math.Clamp(s.GetInt("BGMVolume", 50), 0, 100);
        SfxSlider.Value = Math.Clamp(s.GetInt("SFXVolume", 50), 0, 100);

        string fps = s.GetInt("FPSCap", 120).ToString();
        foreach (var child in FpsPanel.Children)
            if (child is RadioButton rb) rb.IsChecked = (rb.Tag as string) == fps;
        GfxSlider.Value = Math.Clamp(s.GetInt("GraphicsQuality", 50), 0, 100);
        EfxSlider.Value = Math.Clamp(s.GetInt("EffectsQuality", 50), 0, 100);
        ShowFpsCheck.IsChecked = s.GetBool("ShowFPS", false);

        HpSlider.Value = Math.Clamp(s.GetInt("HPWarning", 50), 0, 100);
        MpSlider.Value = Math.Clamp(s.GetInt("MPWarning", 50), 0, 100);
        MouseSlider.Value = Math.Clamp(s.GetInt("MouseSpeed", 50), 0, 100);
        ChatOpenCheck.IsChecked = s.GetBool("Chatopen", false);
        SaveLoginCheck.IsChecked = s.GetBool("SaveLogin", false);

        SettingsHint.Text = s.FileExists
            ? "Changes apply the next time the game starts."
            : "No Settings file yet — this will create one for the client.";

        SettingsOverlay.Visibility = Visibility.Visible;
    }

    // Guards the two fullscreen checkboxes (main-screen quick toggle + the one
    // in the Settings overlay) from re-triggering each other while syncing.
    private bool _syncingFs;

    // Reflect the client's saved Fullscreen setting on the main-screen toggle.
    private void LoadFullscreenState()
    {
        try
        {
            var s = new ClientSettings(_cfg.ClientSettingsFull);
            _syncingFs = true;
            FullscreenQuick.IsChecked = s.GetBool("Fullscreen", false);
        }
        catch { /* toggle just stays unchecked if Settings can't be read */ }
        finally { _syncingFs = false; }
    }

    // Main-screen toggle: write the client Settings immediately so it applies on
    // the next launch, and keep the Settings-overlay checkbox in sync.
    private void FullscreenQuick_Toggled(object sender, RoutedEventArgs e)
    {
        if (_syncingFs) return;
        bool on = FullscreenQuick.IsChecked == true;
        try
        {
            var s = new ClientSettings(_cfg.ClientSettingsFull);
            s.SetBool("Fullscreen", on);
            s.Save();
        }
        catch (Exception ex)
        {
            MessageBox.Show("Could not save the fullscreen setting:\n\n" + ex.Message,
                "GenMs", MessageBoxButton.OK, MessageBoxImage.Warning);
        }

        // Mirror onto the overlay checkbox if it's been created.
        if (FullscreenCheck != null)
        {
            _syncingFs = true;
            FullscreenCheck.IsChecked = on;
            _syncingFs = false;
        }
    }

    private void SettingsSave_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            // Update server (NX downloads) — validate before saving anything.
            // Blank keeps the current server.
            string updInput = UpdateServerBox.Text;
            string newOrigin = "";
            if (!string.IsNullOrWhiteSpace(updInput) && !LauncherConfig.TryParseOrigin(updInput, out newOrigin))
            {
                MessageBox.Show("Update server address not recognized.\n\nUse host, host:port or https://host[:port].",
                    "GenMs", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            var s = new ClientSettings(_cfg.ClientSettingsFull);

            if (!string.IsNullOrWhiteSpace(ServerAddrBox.Text)) s.Set("ServerIP", ServerAddrBox.Text.Trim());
            if (!string.IsNullOrWhiteSpace(ServerPortBox.Text)) s.Set("ServerPort", ServerPortBox.Text.Trim());

            foreach (var child in ResPanel.Children)
                if (child is RadioButton rb && rb.IsChecked == true && rb.Tag is string tag)
                {
                    var p = tag.Split('x');
                    if (p.Length == 2 && int.TryParse(p[0], out int w) && int.TryParse(p[1], out int h))
                    { s.SetInt("Width", w); s.SetInt("Height", h); }
                }
            s.SetBool("Fullscreen", FullscreenCheck.IsChecked == true);
            _syncingFs = true;
            FullscreenQuick.IsChecked = FullscreenCheck.IsChecked == true;  // reflect overlay change on the main toggle
            _syncingFs = false;
            s.SetBool("VSync", VsyncCheck.IsChecked == true);
            s.SetBool("ScreenShake", ShakeCheck.IsChecked == true);
            s.SetInt("BGMVolume", (int)BgmSlider.Value);
            s.SetInt("SFXVolume", (int)SfxSlider.Value);

            foreach (var child in FpsPanel.Children)
                if (child is RadioButton rb && rb.IsChecked == true && rb.Tag is string t && int.TryParse(t, out int fps))
                    s.SetInt("FPSCap", fps);
            s.SetInt("GraphicsQuality", (int)GfxSlider.Value);
            s.SetInt("EffectsQuality", (int)EfxSlider.Value);
            s.SetBool("ShowFPS", ShowFpsCheck.IsChecked == true);

            s.SetInt("HPWarning", (int)HpSlider.Value);
            s.SetInt("MPWarning", (int)MpSlider.Value);
            s.SetInt("MouseSpeed", (int)MouseSlider.Value);
            s.SetBool("Chatopen", ChatOpenCheck.IsChecked == true);
            s.SetBool("SaveLogin", SaveLoginCheck.IsChecked == true);
            s.Save();

            // Apply a changed update server: persist to config.ini and re-check
            // against the new host right away (the Updater reads _cfg live).
            if (newOrigin.Length > 0 && !newOrigin.Equals(_cfg.UpdateOrigin, StringComparison.OrdinalIgnoreCase))
            {
                _cfg.TrySetUpdateServer(newOrigin);
                _cfg.SaveUpdateServer();
                RecheckUpdates();
            }

            SettingsOverlay.Visibility = Visibility.Collapsed;
        }
        catch (Exception ex)
        {
            MessageBox.Show("Could not save settings:\n\n" + ex.Message, "GenMs", MessageBoxButton.OK, MessageBoxImage.Warning);
        }
    }

    // Fresh forced check after the update server changed in Settings.
    private async void RecheckUpdates()
    {
        _pending.Clear();
        PickerPanel.Visibility = Visibility.Collapsed;
        SkipBtn.Visibility = Visibility.Collapsed;
        StatusText.Text = "Checking for updates…";
        Progress.Value = 0;
        ActionBtn.IsEnabled = false;
        HideError();

        var progress = new Progress<ProgressState>(ApplyProgress);
        try
        {
            var result = await _updater.CheckAsync(_selection, progress, _cts.Token, force: true);
            OnChecked(result);
        }
        catch (OperationCanceledException) { }
        catch (Exception ex)
        {
            StatusText.Text = "Something went wrong while checking for updates.";
            ShowError(ex.Message);
            EnterReady(offline: true);
        }
    }

    private void SettingsCancel_Click(object sender, RoutedEventArgs e) => SettingsOverlay.Visibility = Visibility.Collapsed;

    private void Window_Drag(object sender, MouseButtonEventArgs e) { if (e.ButtonState == MouseButtonState.Pressed) DragMove(); }
    private void Min_Click(object sender, RoutedEventArgs e) => WindowState = WindowState.Minimized;
    private void Close_Click(object sender, RoutedEventArgs e) { _cts.Cancel(); Close(); }
}

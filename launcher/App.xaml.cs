using System.Windows;
using System.Windows.Threading;

namespace AugurLauncher;

public partial class App : Application
{
    protected override void OnStartup(StartupEventArgs e)
    {
        // Never crash to a bare dialog — surface any unexpected error nicely.
        DispatcherUnhandledException += (s, ex) =>
        {
            MessageBox.Show(
                "GenMs Launcher hit an unexpected error:\n\n" + ex.Exception.Message,
                "GenMs", MessageBoxButton.OK, MessageBoxImage.Error);
            ex.Handled = true;
        };

        base.OnStartup(e);
    }
}

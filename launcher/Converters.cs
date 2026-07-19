using System.Globalization;
using System.Windows.Data;
using System.Windows.Media;

namespace AugurLauncher;

/// <summary>true (new file) -> green badge, false (update) -> blue badge.</summary>
public sealed class NewTagBrushConverter : IValueConverter
{
    private static readonly Brush New = new SolidColorBrush(Color.FromRgb(0x57, 0xb3, 0x68));   // green
    private static readonly Brush Upd = new SolidColorBrush(Color.FromRgb(0x4a, 0x9f, 0xd8));   // blue

    public object Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
        => (value is bool b && b) ? New : Upd;

    public object ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        => throw new NotSupportedException();
}

/// <summary>Hex string like "#E0A82E" -> SolidColorBrush (for GenAI feed accents).</summary>
public sealed class HexBrushConverter : IValueConverter
{
    public object Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
    {
        try
        {
            if (value is string s && !string.IsNullOrWhiteSpace(s))
                return new SolidColorBrush((Color)ColorConverter.ConvertFromString(s));
        }
        catch { }
        return new SolidColorBrush(Color.FromRgb(0x8A, 0x7B, 0x63));
    }

    public object ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        => throw new NotSupportedException();
}

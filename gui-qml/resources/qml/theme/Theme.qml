pragma Singleton
import QtQuick
import QtQuick.Controls.Material

QtObject {
    property bool darkTheme: appConfig?.theme === "dark" ?? false

    // ── Base palette ──────────────────────────────────────────────────────────
    property color background:    darkTheme ? "#121212" : "#f0f2f5"
    property color textColor:     darkTheme ? "#ECEFF1" : "#212121"
    property color hintColor:     darkTheme ? "#B0BEC5" : "#757575"
    property color dividerColor:  darkTheme ? "#424242" : "#d0d4da"
    property color cardBackground:darkTheme ? "#222222" : "#ffffff"

    property color accentColor: {
        switch ((appConfig?.accentColor ?? "blue").toLowerCase()) {
        case "red":        return Material.color(Material.Red)
        case "pink":       return Material.color(Material.Pink)
        case "purple":     return Material.color(Material.Purple)
        case "deeppurple": return Material.color(Material.DeepPurple)
        case "indigo":     return Material.color(Material.Indigo)
        case "blue":       return Material.color(Material.Blue)
        case "lightblue":  return Material.color(Material.LightBlue)
        case "cyan":       return Material.color(Material.Cyan)
        case "teal":       return Material.color(Material.Teal)
        case "green":      return Material.color(Material.Green)
        case "lightgreen": return Material.color(Material.LightGreen)
        case "lime":       return Material.color(Material.Lime)
        case "yellow":     return Material.color(Material.Yellow)
        case "amber":      return Material.color(Material.Amber)
        case "orange":     return Material.color(Material.Orange)
        case "deeporange": return Material.color(Material.DeepOrange)
        case "brown":      return Material.color(Material.Brown)
        case "bluegrey":   return Material.color(Material.BlueGrey)
        default:           return Material.color(Material.Blue)
        }
    }

    property color errorColor:   Material.color(Material.Red)
    property color successColor: Material.color(Material.Green)
    property color warningColor: Material.color(Material.Orange)
    property color againColor:   Material.color(Material.Red)
    property color hardColor:    Material.color(Material.BlueGrey)
    property color goodColor:    Material.color(Material.Green)
    property color easyColor:    Material.color(Material.Blue)

    property color surfaceHover:    darkTheme ? Qt.rgba(1,1,1,0.07) : Qt.rgba(0,0,0,0.05)
    property color surfacePress:    darkTheme ? Qt.rgba(1,1,1,0.12) : Qt.rgba(0,0,0,0.08)
    property color surfaceSubtle:   darkTheme ? Qt.rgba(1,1,1,0.06) : Qt.rgba(0,0,0,0.05)
    property color surfaceBorder:   darkTheme ? Qt.rgba(1,1,1,0.10) : Qt.rgba(0,0,0,0.12)
    property color surfaceInactive: darkTheme ? Qt.rgba(1,1,1,0.25) : Qt.rgba(0,0,0,0.20)
    property color surfaceClear:    darkTheme ? Qt.rgba(1,1,1,0.10) : Qt.rgba(0,0,0,0.08)
    property color surfaceInput:    darkTheme ? Qt.rgba(1,1,1,0.05) : Qt.rgba(0,0,0,0.04)

    function srsStateColor(state, fallback) {
        switch (state ?? "") {
            case "New":        return "#4A9EFF"
            case "Learning":   return "#FFB83F"
            case "Relearning": return "#FF7043"
            case "Review":     return "#4CAF7D"
            case "Suspended":  return "#9E9E9E"
            default:           return fallback !== undefined ? fallback : hintColor
        }
    }

    function srsStateIcon(state) {
        switch (state ?? "") {
            case "New":        return "✦"
            case "Learning":   return "◎"
            case "Relearning": return "↺"
            case "Review":     return "✓"
            case "Suspended":  return "⏸"
            default:           return ""
        }
    }

    property real   fontScale:  appConfig?.fontScale ?? 1.0
    property string fontFamily: (appConfig?.fontFamily ?? "default") === "default" ? "" : appConfig?.fontFamily ?? ""

    property real fontSizeTiny:    Math.round(10 * fontScale)
    property real fontSizeXSmall:  Math.round(11 * fontScale)
    property real fontSizeSmall:   Math.round(12 * fontScale)
    property real fontSizeBody:    Math.round(13 * fontScale)
    property real fontSizeBase:    Math.round(14 * fontScale)
    property real fontSizeIcon:    Math.round(15 * fontScale)
    property real fontSizeMedium:  Math.round(16 * fontScale)
    property real fontSizeItem:    Math.round(17 * fontScale)
    property real fontSizeLarge:   Math.round(18 * fontScale)
    property real fontSizeTitle:   Math.round(22 * fontScale)
    property real fontSizeBack:    Math.round(32 * fontScale)
    property real fontSizeHero:    Math.round(34 * fontScale)
    property real fontSizeDisplay: Math.round(36 * fontScale)
    property real fontSizeCard:    Math.round(42 * fontScale)
    property real fontSizeGlyph:   Math.round(48 * fontScale)

    readonly property int minTapTarget: 48
}

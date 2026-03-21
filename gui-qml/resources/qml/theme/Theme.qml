pragma Singleton
import QtQuick
import QtQuick.Controls.Material

QtObject {
    property bool darkTheme: appConfig.theme === "dark"

    property color background:    darkTheme ? "#121212" : "#fafafa"
    property color textColor:     darkTheme ? "#ECEFF1" : "#212121"
    property color hintColor:     darkTheme ? "#B0BEC5" : "#757575"
    property color accentColor: {
        switch (appConfig.accentColor.toLowerCase()) {
        case "blue":   return Material.color(Material.Blue)
        case "red":    return Material.color(Material.Red)
        case "green":  return Material.color(Material.Green)
        case "indigo": return Material.color(Material.Indigo)
        default:       return Material.color(Material.Blue)
        }
    }
    property color dividerColor:   darkTheme ? "#424242" : "#E0E0E0"
    property color cardBackground: darkTheme ? "#222222" : "#ffffff"
    property color errorColor:     Material.color(Material.Red)
    property color successColor:   Material.color(Material.Green)
    property color warningColor:   Material.color(Material.Orange)
    property color againColor:     Material.color(Material.Red)
    property color hardColor:      Material.color(Material.BlueGrey)
    property color goodColor:      Material.color(Material.Green)
    property color easyColor:      Material.color(Material.Blue)

    // ── Font scale ────────────────────────────────────────────────────────────
    // Comes from appConfig.fontScale (0.85 | 1.0 | 1.15)
    // All font sizes in the app derive from these — never hardcode pixelSize.
    property real   fontScale:  appConfig.fontScale
    property string fontFamily: appConfig.fontFamily === "default" ? "" : appConfig.fontFamily
    // fontFamily="" means Qt uses the system default — no override needed

    // Semantic scale — map every hardcoded value to a named token:
    //
    //  10 → fontSizeTiny    (stat card sub-labels, interval text in rating buttons)
    //  11 → fontSizeXSmall  (badge text, pill labels, section headers uppercase)
    //  12 → fontSizeSmall   (action buttons, back button, filter pills)
    //  13 → fontSizeBody    (search fields, list glosses, body copy)
    //  14 → fontSizeBase    (show answer button, dashboard action labels)
    //  15 → fontSizeIcon    (search icon glyphs)
    //  16 → fontSizeMedium  (card meaning/answer, stat values secondary)
    //  17 → fontSizeItem    (dictionary list headword)
    //  18 → fontSizeLarge   (stat card values, details headword secondary)
    //  22 → fontSizeTitle   (dashboard section title)
    //  32 → fontSizeBack    (back chevron "‹")  — intentionally large
    //  34 → fontSizeHero    (dashboard stat numbers)
    //  36 → fontSizeDisplay (stat card number in SrsLibrary delegate)
    //  42 → fontSizeCard    (flashcard word during study)
    //  48 → fontSizeGlyph   (empty state decorative kanji)

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
}
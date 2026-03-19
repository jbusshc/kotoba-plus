pragma Singleton

import QtQuick
import QtQuick.Controls.Material

QtObject {

    property bool darkTheme: appConfig.theme === "dark"
    property color background: darkTheme ? "#121212" : "#fafafa"

    property color textColor: darkTheme ? "#ECEFF1" : "#212121"
    property color hintColor: darkTheme ? "#B0BEC5" : "#757575"

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

    property color errorColor:   Material.color(Material.Red)
    property color successColor: Material.color(Material.Green)
    property color warningColor: Material.color(Material.Orange)

    property color againColor: Material.color(Material.Red)
    property color hardColor:  Material.color(Material.BlueGrey)
    property color goodColor:  Material.color(Material.Green)
    property color easyColor:  Material.color(Material.Blue)
}
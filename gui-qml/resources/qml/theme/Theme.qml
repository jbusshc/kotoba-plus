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
        case "blue": return Material.Blue
        case "red": return Material.Red
        case "green": return Material.Green
        case "indigo": return Material.Indigo
        default: return Material.Blue
        }
    }


    property color dividerColor: darkTheme ? "#424242" : "#E0E0E0"
    property color cardBackground: darkTheme ? "#222222" : "#ffffff"    

    property color errorColor: Material.Red
    property color successColor: Material.Green
    property color warningColor: Material.Orange

    property color againColor: Material.Red
    property color hardColor: Material.Orange
    property color goodColor: Material.Green
    property color easyColor: Material.Blue

}
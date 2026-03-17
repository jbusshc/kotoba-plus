import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import "pages"
import Kotoba 1.0

ApplicationWindow {
    id: root
    width: 900
    height: 650
    visible: true
    title: "Kotoba Plus"
    color: Theme.background

    Material.theme: appConfig.theme === "dark" ? Material.Dark : Material.Light

    Material.primary: {
        switch (appConfig.primaryColor.toLowerCase()) {
            case "indigo": return Material.Indigo
            case "blue":   return Material.Blue
            case "red":    return Material.Red
            case "green":  return Material.Green
            default:       return Material.Indigo
        }
    }

    Material.accent: {
        switch (appConfig.accentColor.toLowerCase()) {
            case "blue":   return Material.Blue
            case "red":    return Material.Red
            case "green":  return Material.Green
            case "indigo": return Material.Indigo
            default:       return Material.Blue
        }
    }

    property int currentTab: 0

    header: TabBar {
        id: tabBar
        currentIndex: root.currentTab

        TabButton {
            text: "Dictionary"
            onClicked: {
                if (root.currentTab !== 0) {
                    root.currentTab = 0
                    stack.replace(dictionaryPageComponent)
                }
            }
        }

        TabButton {
            text: "SRS"
            onClicked: {
                if (root.currentTab !== 1) {
                    root.currentTab = 1
                    stack.replace(srsDashboardComponent)
                }
            }
        }
    }

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: dictionaryPageComponent
    }

    Component { id: dictionaryPageComponent; DictionaryPage {} }
    Component { id: srsDashboardComponent;   SrsDashboard {} }
}
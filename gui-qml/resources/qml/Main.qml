import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import "pages"

ApplicationWindow {
    id: root
    width: 900
    height: 650
    visible: true
    title: "Kotoba Plus"

    // tema dinámico
    Material.theme: appConfig.theme === "dark" ? Material.Dark : Material.Light

    // colores dinámicos
    Material.primary: {
        switch (appConfig.primaryColor.toLowerCase()) {
            case "indigo": return Material.Indigo
            case "blue": return Material.Blue
            case "red": return Material.Red
            case "green": return Material.Green
            default: return Material.Indigo
        }
    }

    Material.accent: {
        switch (appConfig.accentColor.toLowerCase()) {
            case "blue": return Material.Blue
            case "red": return Material.Red
            case "green": return Material.Green
            case "indigo": return Material.Indigo
            default: return Material.Blue
        }
    }

    header: TabBar {
        id: tabBar
        currentIndex: stack.currentIndex || 0

        TabButton {
            text: "Dictionary"
            onClicked: stack.replace(dictionaryPageComponent)
        }

        TabButton {
            text: "SRS"
            onClicked: stack.replace(srsDashboardComponent)
        }
    }

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: dictionaryPageComponent
    }

    Component { id: dictionaryPageComponent; DictionaryPage {} }
    Component { id: srsDashboardComponent; SrsDashboard {} }
}
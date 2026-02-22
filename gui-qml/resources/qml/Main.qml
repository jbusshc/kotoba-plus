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

    Material.theme: Material.Dark
    Material.primary: Material.Indigo   // azul sobrio
    Material.accent: Material.Blue

    header: TabBar {
        id: tabBar
        currentIndex: stack.currentIndex

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

    Component {
        id: dictionaryPageComponent
        DictionaryPage {}
    }

    Component {
        id: srsDashboardComponent
        SrsDashboard {}
    }
}
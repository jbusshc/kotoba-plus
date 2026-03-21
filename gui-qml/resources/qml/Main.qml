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

    // 0 = Dictionary, 1 = SRS, 2 = Settings
    property int currentTab: 0

    function switchTab(index) {
        if (root.currentTab === index) return
        root.currentTab = index
        switch (index) {
            case 0: stack.replace(dictionaryPageComponent); break
            case 1: stack.replace(srsDashboardComponent);   break
            case 2: stack.replace(settingsPageComponent);   break
        }
    }

    header: Item {
        width: parent.width
        height: tabBar.implicitHeight

        TabBar {
            id: tabBar
            width: parent.width - gearBtn.width
            currentIndex: root.currentTab === 2 ? -1 : root.currentTab

            TabButton {
                text: "Dictionary"
                onClicked: root.switchTab(0)
            }
            TabButton {
                text: "SRS"
                onClicked: root.switchTab(1)
            }
        }

        // Gear — mirrors TabButton active style
        Item {
            id: gearBtn
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 48

            // Hover background
            Rectangle {
                anchors.fill: parent
                color: gearMouse.containsMouse ? Qt.rgba(1,1,1,0.08) : "transparent"
                Behavior on color { ColorAnimation { duration: 120 } }
            }

            Text {
                anchors.centerIn: parent
                anchors.verticalCenterOffset: -1
                text: "⚙"
                font.pixelSize: 17
                color: root.currentTab === 2 ? "white" : Qt.rgba(1, 1, 1, 0.60)
                Behavior on color { ColorAnimation { duration: 120 } }
            }

            // Active underline — same style as Material TabButton
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                width: root.currentTab === 2 ? 24 : 0
                height: 2
                radius: 1
                color: "white"
                Behavior on width { NumberAnimation { duration: 150; easing.type: Easing.OutQuad } }
            }

            MouseArea {
                id: gearMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.switchTab(2)
            }
        }
    }

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: dictionaryPageComponent
    }

    Component { id: dictionaryPageComponent; DictionaryPage {}    }
    Component { id: srsDashboardComponent;   SrsDashboard {}      }
    Component { id: settingsPageComponent;   SettingsPage {}      }
}
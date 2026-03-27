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
    property int pendingTab: -1   // tab al que se quiere ir mientras hay dirty

    // Lee dirty directamente del item vivo en el stack
    readonly property bool settingsDirty: {
        if (stack.currentItem && typeof stack.currentItem.dirty !== "undefined")
            return stack.currentItem.dirty
        return false
    }

    // Intento de cambio de tab — pasa por el guard de dirty
    function requestTabSwitch(index) {
        if (root.currentTab === index) return
        if (root.settingsDirty) {
            root.pendingTab = index
            unsavedDialog.open()
            return
        }
        root.switchTab(index)
    }

    // Cambio efectivo, sin guardar
    function switchTab(index) {
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
                onClicked: root.requestTabSwitch(0)
            }
            TabButton {
                text: "SRS"
                onClicked: root.requestTabSwitch(1)
            }
        }

        Item {
            id: gearBtn
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 48

            Rectangle {
                anchors.fill: parent
                color: gearMouse.containsMouse ? Theme.surfaceHover : "transparent"
                Behavior on color { ColorAnimation { duration: 120 } }
            }

            Text {
                anchors.centerIn: parent
                anchors.verticalCenterOffset: -1
                text: "⚙"
                font.pixelSize: 17
                color: root.currentTab === 2 ? Theme.textColor : Theme.hintColor
                Behavior on color { ColorAnimation { duration: 120 } }
            }

            Rectangle {
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                width: root.currentTab === 2 ? 24 : 0
                height: 2
                radius: 1
                color: Theme.accentColor
                Behavior on width { NumberAnimation { duration: 150; easing.type: Easing.OutQuad } }
            }

            MouseArea {
                id: gearMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.requestTabSwitch(2)
            }
        }
    }

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: dictionaryPageComponent
    }

    Component { id: dictionaryPageComponent; DictionaryPage {}  }
    Component { id: srsDashboardComponent;   SrsDashboard {}    }
    Component { id: settingsPageComponent;   SettingsPage {}    }

    // ── Dialog: cambios sin aplicar ──────────────────────────────────────────
    Dialog {
        id: unsavedDialog
        title: "Unsaved changes"
        width: 340
        anchors.centerIn: Overlay.overlay
        modal: true
        closePolicy: Popup.CloseOnEscape

        Text {
            text: "You have unsaved settings changes.\nDo you want to discard them and leave?"
            wrapMode: Text.Wrap
            width: 292
            color: Theme.textColor
            font.pixelSize: Theme.fontSizeBody
            lineHeight: 1.4
        }

        footer: DialogButtonBox {
            alignment: Qt.AlignRight
            spacing: 8
            leftPadding: 16; rightPadding: 16
            bottomPadding: 12; topPadding: 8

            // Botón "Keep editing" — queda en settings
            Button {
                text: "Keep Editing"
                flat: true
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
                contentItem: Text {
                    text: parent.text
                    font.pixelSize: Theme.fontSizeSmall
                    font.weight: Font.Medium
                    color: Theme.hintColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 6
                    color: parent.hovered
                        ? Theme.surfaceHover
                        : "transparent"
                    border.width: 1
                    border.color: Theme.surfaceBorder
                    Behavior on color { ColorAnimation { duration: 100 } }
                }
            }

            // Botón "Discard" — descarta y navega
            Button {
                text: "Discard & Leave"
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                contentItem: Text {
                    text: parent.text
                    font.pixelSize: Theme.fontSizeSmall
                    font.weight: Font.Medium
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 6
                    color: parent.hovered ? "#e53935" : "#f44336"
                    Behavior on color { ColorAnimation { duration: 100 } }
                }
            }
        }

        onRejected: {
            root.pendingTab = -1
            // Vuelve a seleccionar el gear visualmente (ya estaba en settings)
            root.currentTab = 2
        }

        onAccepted: {
            // Revierte appConfig al último estado guardado
            appConfig.reloadFromDisk()
            var target = root.pendingTab
            root.pendingTab = -1
            root.switchTab(target)
        }
    }
}
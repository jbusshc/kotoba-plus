import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import "theme"
import "pages"

ApplicationWindow {
    id: root

    readonly property bool isDesktop: Qt.platform.os === "windows"
                                || Qt.platform.os === "linux"
                                || Qt.platform.os === "osx"

    visible: true
    width:   isDesktop ? 900 : Screen.width
    height:  isDesktop ? 600 : Screen.height
    title:   "Kotoba+"

    Material.theme: Theme.darkTheme ? Material.Dark : Material.Light
    Material.accent: Theme.accentColor

    property int currentTab:  0
    property int pendingTab:  -1

    function navigate(url, props) {
        stack.push(url, props ?? {})
    }

    function requestTabSwitch(index) {
        if (index === currentTab) return
        if (currentTab === 2) {
            const settingsItem = stack.currentItem
            if (settingsItem && settingsItem.dirty) {
                pendingTab = index
                unsavedDialog.open()
                return
            }
        }
        switchTab(index)
    }

    function switchTab(index) {
        currentTab = index
        switch (index) {
        case 0: stack.replace(dictionaryPageComponent); break
        case 1: stack.replace(srsDashboardComponent);   break
        case 2: stack.replace(settingsPageComponent);   break
        }
    }

    header: Rectangle {
        width:  parent ? parent.width : root.width
        height: tabBar.implicitHeight
        color:  Theme.headerBarColor

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: Theme.dividerColor
        }

        TabBar {
            id: tabBar
            width: parent.width - gearBtn.width
            currentIndex: root.currentTab === 2 ? -1 : root.currentTab
            background: Rectangle { color: "transparent" }

            TabButton {
                text: "Dictionary"
                onClicked: root.requestTabSwitch(0)

                contentItem: Text {
                    text: parent.text
                    color: parent.checked ? Theme.textColor : Theme.hintColor
                    font.pixelSize: 16
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            TabButton {
                text: "SRS"
                onClicked: root.requestTabSwitch(1)

                contentItem: Text {
                    text: parent.text
                    color: parent.checked ? Theme.textColor : Theme.hintColor
                    font.pixelSize: 16
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        Item {
            id: gearBtn
            anchors { right: parent.right; top: parent.top; bottom: parent.bottom }
            width: Theme.minTapTarget

            Rectangle {
                anchors.fill: parent
                color: gearMouse.containsMouse ? Theme.surfaceHover : "transparent"
                Behavior on color { ColorAnimation { duration: 120 } }
            }

            Text {
                anchors.centerIn: parent
                text: "⚙"
                font.pixelSize: 17
                color: root.currentTab === 2 ? Theme.textColor : Theme.hintColor
            }

            Rectangle {
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                width: root.currentTab === 2 ? 24 : 0
                height: 2
                radius: 1
                color: Theme.accentColor
                Behavior on width { NumberAnimation { duration: 150 } }
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

        initialItem: Qt.platform.os === "android"
                     ? splashPageComponent
                     : dictionaryPageComponent

        pushEnter: null
        pushExit: null
        popEnter: null
        popExit: null
        replaceEnter: null
        replaceExit: null
    }

    // ✅ OVERLAY CORREGIDO (CLAVE)
    Rectangle {
        id: resumeOverlay
        anchors.fill: parent
        color: Theme.background
        z: 9999

        // visible inmediatamente cuando app no está activa
        visible: !appController.appActive || fadeOut.running

        opacity: (!appController.appActive) ? 1.0 : (fadeOut.running ? 1.0 : 0.0)

        Behavior on opacity {
            NumberAnimation { duration: 150; easing.type: Easing.OutQuad }
        }

        Timer {
            id: fadeOut
            interval: 120
            onTriggered: resumeOverlay.opacity = 0.0
        }
    }

    Component { id: splashPageComponent;     SplashPage     {} }
    Component { id: dictionaryPageComponent; DictionaryPage {} }
    Component { id: srsDashboardComponent;   SrsDashboard   {} }
    Component { id: settingsPageComponent;   SettingsPage   {} }

    Connections {
        target: appController

        function onAppReady() {
            stack.replace(dictionaryPageComponent)
            root.currentTab = 0
        }

        function onAppActiveChanged(active) {
            if (active) {
                fadeOut.restart()
            }
        }
    }

    Dialog {
        id: unsavedDialog
        title: "Unsaved changes"
        width: 340
        anchors.centerIn: Overlay.overlay
        modal: true

        Text {
            text: "You have unsaved settings changes.\nDiscard them and leave?"
            wrapMode: Text.Wrap
            width: 296
            color: Theme.textColor
        }

        footer: DialogButtonBox {
            alignment: Qt.AlignRight

            Button {
                text: "Keep Editing"
                flat: true
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            }
            Button {
                text: "Discard & Leave"
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            }
        }

        onRejected: {
            root.pendingTab = -1
            root.currentTab = 2
        }

        onAccepted: {
            if (typeof appConfig !== "undefined" && appConfig)
                appConfig.reloadFromDisk()
            const target = root.pendingTab
            root.pendingTab = -1
            root.switchTab(target)
        }
    }
}
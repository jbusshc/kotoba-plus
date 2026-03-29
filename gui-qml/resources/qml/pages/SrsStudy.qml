import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Kotoba 1.0
import "../theme"
import "../components"

Page {
    id: page
    padding: 0

    property color textColor:      Theme.textColor
    property color hintColor:      Theme.hintColor
    property color accentColor:    Theme.accentColor
    property color dividerColor:   Theme.dividerColor
    property color cardBackground: Theme.cardBackground

    property color againColor: Theme.againColor
    property color hardColor:  Theme.hardColor
    property color goodColor:  Theme.goodColor
    property color easyColor:  Theme.easyColor

    property bool answerShown:  false
    property bool contentReady: false

    background: Rectangle { color: Theme.background }

    Component.onCompleted: {
        if (srsVM) srsVM.startSession()
        page.contentReady = true
    }

    Connections {
        target: srsVM
        function onCurrentChanged() {
            page.contentReady = false
            page.answerShown  = false
            Qt.callLater(() => { page.contentReady = true })
        }
        function onNoMoreCards() {
            if (srsVM) srsVM.saveProfile()
            if (stack) stack.pop()
        }
    }

    // ── Rating button ─────────────────────────────────────────────────────────
    component RatingButton: Rectangle {
        property string label:    ""
        property string interval: ""
        property color  accent:   "white"
        signal clicked()

        radius: 8
        color: btnMouse.pressed
            ? Qt.rgba(accent.r, accent.g, accent.b, 0.30)
            : btnMouse.containsMouse
                ? Qt.rgba(accent.r, accent.g, accent.b, 0.22)
                : Qt.rgba(accent.r, accent.g, accent.b, 0.12)
        border.width: 1
        border.color: Qt.rgba(accent.r, accent.g, accent.b, 0.40)
        Behavior on color { ColorAnimation { duration: 100 } }

        Column {
            anchors.centerIn: parent
            spacing: 3
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: label
                font.pixelSize: Theme.fontSizeBody; font.weight: Font.DemiBold
                color: accent
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: interval
                font.pixelSize: Theme.fontSizeTiny
                color: Qt.rgba(accent.r, accent.g, accent.b, 0.65)
                visible: interval.length > 0
            }
        }

        MouseArea {
            id: btnMouse; anchors.fill: parent
            hoverEnabled: true; cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
        }
    }

    Item {
        anchors.fill: parent
        anchors.margins: 24

        // ── Top bar ───────────────────────────────────────────────────────────
        RowLayout {
            id: topBar
            anchors.top: parent.top
            anchors.left: parent.left; anchors.right: parent.right
            height: Theme.minTapTarget

            // Back button
            Item {
                width: Math.max(backLabel.implicitWidth + 24, Theme.minTapTarget)
                height: Theme.minTapTarget

                Rectangle {
                    anchors.fill: parent; radius: 6
                    color: backMouse.pressed
                        ? Theme.surfacePress
                        : backMouse.containsMouse ? Theme.surfaceHover : "transparent"
                    Behavior on color { ColorAnimation { duration: 120 } }
                }
                RowLayout {
                    anchors.centerIn: parent; spacing: 4
                    Text {
                        text: "‹"
                        font.pixelSize: Theme.fontSizeMedium; color: hintColor
                    }
                    Text {
                        id: backLabel
                        text: "Back"
                        font.pixelSize: Theme.fontSizeSmall
                        font.weight: Font.Medium; color: hintColor
                    }
                }
                MouseArea {
                    id: backMouse; anchors.fill: parent
                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: stack.pop()
                }
            }

            Item { Layout.fillWidth: true }

            // Undo button
            Item {
                width: Math.max(undoLabel.implicitWidth + 24, Theme.minTapTarget)
                height: Theme.minTapTarget
                visible: srsVM && srsVM.canUndo && !page.answerShown

                Rectangle {
                    anchors.fill: parent; radius: 6
                    color: undoMouse.pressed
                        ? Theme.surfacePress
                        : undoMouse.containsMouse ? Theme.surfaceHover : "transparent"
                    Behavior on color { ColorAnimation { duration: 120 } }
                }
                RowLayout {
                    anchors.centerIn: parent; spacing: 4
                    Text {
                        text: "↩"
                        font.pixelSize: Theme.fontSizeSmall; color: hintColor
                    }
                    Text {
                        id: undoLabel
                        text: "Undo"
                        font.pixelSize: Theme.fontSizeSmall
                        font.weight: Font.Medium; color: hintColor
                    }
                }
                MouseArea {
                    id: undoMouse; anchors.fill: parent
                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: if (srsVM) srsVM.undoLastAnswer()
                }
            }
        }

        // ── Action bar ────────────────────────────────────────────────────────
        Item {
            id: actionBar
            anchors.bottom: parent.bottom
            anchors.left: parent.left; anchors.right: parent.right
            height: Theme.minTapTarget + 20   // 68px

            // Show Answer button
            Rectangle {
                anchors.fill: parent
                visible: !page.answerShown
                radius: 8
                color: showMouse.pressed
                    ? Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.24)
                    : showMouse.containsMouse
                        ? Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.18)
                        : Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.10)
                border.width: 1
                border.color: Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.35)
                enabled: srsVM && srsVM.hasCard
                opacity: enabled ? 1.0 : 0.4
                Behavior on color { ColorAnimation { duration: 120 } }

                Text {
                    anchors.centerIn: parent
                    text: "Show Answer"
                    font.pixelSize: Theme.fontSizeBase; font.weight: Font.Medium
                    color: accentColor
                }
                MouseArea {
                    id: showMouse; anchors.fill: parent
                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: page.answerShown = true
                }
            }

            RowLayout {
                anchors.fill: parent
                spacing: 8
                visible: page.answerShown

                RatingButton {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    label: "Again"; interval: srsVM ? srsVM.againInterval : ""
                    accent: againColor
                    onClicked: if (srsVM) srsVM.answerAgain()
                }
                RatingButton {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    label: "Hard"; interval: srsVM ? srsVM.hardInterval : ""
                    accent: hardColor
                    onClicked: if (srsVM) srsVM.answerHard()
                }
                RatingButton {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    label: "Good"; interval: srsVM ? srsVM.goodInterval : ""
                    accent: goodColor
                    onClicked: if (srsVM) srsVM.answerGood()
                }
                RatingButton {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    label: "Easy"; interval: srsVM ? srsVM.easyInterval : ""
                    accent: easyColor
                    onClicked: if (srsVM) srsVM.answerEasy()
                }
            }
        }

        // ── Flash card ────────────────────────────────────────────────────────
        Item {
            anchors.top: topBar.bottom
            anchors.bottom: actionBar.top
            anchors.left: parent.left; anchors.right: parent.right
            anchors.topMargin: 16; anchors.bottomMargin: 16

            Rectangle {
                anchors.centerIn: parent
                width: Math.min(620, parent.width)
                height: Math.min(parent.height, cardScroll.contentHeight + 56)
                Behavior on height { NumberAnimation { duration: 180; easing.type: Easing.OutQuad } }
                radius: 12
                color: cardBackground
                border.width: 1; border.color: dividerColor

                Rectangle {
                    anchors.top: parent.top
                    anchors.left: parent.left; anchors.right: parent.right
                    height: 2; radius: 1
                    color: accentColor; opacity: 0.6
                }

                ScrollView {
                    id: cardScroll
                    anchors.fill: parent; anchors.margins: 28
                    clip: true
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                    opacity: page.contentReady ? 1.0 : 0.0
                    Behavior on opacity { NumberAnimation { duration: 120 } }

                    Loader {
                        anchors.fill: parent
                        active: page.contentReady

                        sourceComponent: EntryView {
                            width: parent.width
                            entryData: srsVM ? srsVM.currentEntryData : ({})
                            mode: "srs"
                            revealed: page.answerShown
                            textColor:    page.textColor
                            hintColor:    page.hintColor
                            accentColor:  page.accentColor
                            dividerColor: page.dividerColor
                        }
                    }
                }
            }
        }
    }
}

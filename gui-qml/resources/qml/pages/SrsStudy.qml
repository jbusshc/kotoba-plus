import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Kotoba 1.0
import "../theme"

Page {
    id: page
    padding: 0

    property bool darkTheme: Theme.darkTheme
    property color textColor: Theme.textColor
    property color hintColor: Theme.hintColor
    property color accentColor: Theme.accentColor
    property color dividerColor: Theme.dividerColor
    property color cardBackground: Theme.cardBackground

    property color againColor: Theme.againColor
    property color hardColor:  Theme.hardColor
    property color goodColor:  Theme.goodColor
    property color easyColor:  Theme.easyColor

    property bool answerShown: false

    background: Rectangle { color: Theme.background }

    Component.onCompleted: {
        if (srsVM) srsVM.startSession()
    }

    Connections {
        target: srsVM
        function onCurrentChanged() { page.answerShown = false }
        function onNoMoreCards() {
            if (srsVM) srsVM.saveProfile()
            if (stack) stack.pop()
        }
    }

    component RatingButton: Rectangle {
        property string label: ""
        property string interval: ""
        property color  accent: "white"
        signal clicked()

        radius: 8
        color: btnMouse.containsMouse
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
                font.pixelSize: 13
                font.weight: Font.DemiBold
                color: accent
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: interval
                font.pixelSize: 10
                color: Qt.rgba(accent.r, accent.g, accent.b, 0.65)
                visible: interval.length > 0
            }
        }

        MouseArea {
            id: btnMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
        }
    }

    Item {
        anchors.fill: parent
        anchors.margins: 24

        // ── Top bar ──────────────────────────────────────────────────────────
        RowLayout {
            id: topBar
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 32

            Item {
                width: backLabel.implicitWidth + 20
                height: 32
                Rectangle {
                    anchors.fill: parent; radius: 6
                    color: backMouse.containsMouse ? Qt.rgba(1,1,1,0.06) : "transparent"
                    Behavior on color { ColorAnimation { duration: 120 } }
                }
                RowLayout {
                    anchors.centerIn: parent; spacing: 4
                    Text { text: "‹"; font.pixelSize: 16; color: hintColor }
                    Text { id: backLabel; text: "Back"; font.pixelSize: 12; font.weight: Font.Medium; color: hintColor }
                }
                MouseArea {
                    id: backMouse; anchors.fill: parent
                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: stack.pop()
                }
            }

            Item { Layout.fillWidth: true }

            Item {
                width: undoLabel.implicitWidth + 20
                height: 32
                visible: srsVM && srsVM.canUndo && !page.answerShown
                Rectangle {
                    anchors.fill: parent; radius: 6
                    color: undoMouse.containsMouse ? Qt.rgba(1,1,1,0.06) : "transparent"
                    Behavior on color { ColorAnimation { duration: 120 } }
                }
                RowLayout {
                    anchors.centerIn: parent; spacing: 4
                    Text { text: "↩"; font.pixelSize: 12; color: hintColor }
                    Text { id: undoLabel; text: "Undo"; font.pixelSize: 12; font.weight: Font.Medium; color: hintColor }
                }
                MouseArea {
                    id: undoMouse; anchors.fill: parent
                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: if (srsVM) srsVM.undoLastAnswer()
                }
            }
        }

        // ── Action bar — always pinned to bottom ─────────────────────────────
        Item {
            id: actionBar
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 60

            Rectangle {
                anchors.fill: parent
                visible: !page.answerShown
                radius: 8
                color: showMouse.containsMouse
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
                    font.pixelSize: 14; font.weight: Font.Medium
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
                    label: "Again"; interval: srsVM ? srsVM.againInterval : ""; accent: againColor
                    onClicked: if (srsVM) srsVM.answerAgain()
                }
                RatingButton {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    label: "Hard"; interval: srsVM ? srsVM.hardInterval : ""; accent: hardColor
                    onClicked: if (srsVM) srsVM.answerHard()
                }
                RatingButton {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    label: "Good"; interval: srsVM ? srsVM.goodInterval : ""; accent: goodColor
                    onClicked: if (srsVM) srsVM.answerGood()
                }
                RatingButton {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    label: "Easy"; interval: srsVM ? srsVM.easyInterval : ""; accent: easyColor
                    onClicked: if (srsVM) srsVM.answerEasy()
                }
            }
        }

        // ── Flash card — fills space between top bar and action bar ──────────
        Item {
            anchors.top: topBar.bottom
            anchors.bottom: actionBar.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 16
            anchors.bottomMargin: 16

            Rectangle {
                anchors.centerIn: parent
                width: Math.min(580, parent.width)
                height: cardContent.implicitHeight + 56
                radius: 12
                color: cardBackground
                border.width: 1
                border.color: dividerColor

                Rectangle {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 2; radius: 1
                    color: accentColor; opacity: 0.6
                }

                Column {
                    id: cardContent
                    anchors.centerIn: parent
                    width: parent.width - 56
                    spacing: 20

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: srsVM ? srsVM.currentWord : ""
                        font.pixelSize: 42; font.weight: Font.Bold
                        color: textColor
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap; width: parent.width
                    }

                    Rectangle {
                        width: parent.width * 0.3; height: 1
                        color: dividerColor
                        anchors.horizontalCenter: parent.horizontalCenter
                        visible: page.answerShown; opacity: 0.5
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        opacity: page.answerShown ? 1.0 : 0.0
                        visible: opacity > 0
                        text: srsVM ? srsVM.currentMeaning : ""
                        font.pixelSize: 16; color: hintColor
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap; width: parent.width
                        Behavior on opacity { NumberAnimation { duration: 180 } }
                    }
                }
            }
        }
    }
}
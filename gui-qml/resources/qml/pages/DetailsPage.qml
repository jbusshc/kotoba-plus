import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Kotoba 1.0
import "../components"

Page {
    id: page

    property bool darkTheme: Theme.darkTheme
    property color textColor: Theme.textColor
    property color hintColor: Theme.hintColor
    property color accentColor: Theme.accentColor
    property color dividerColor: Theme.dividerColor

    property int docId: -1
    property var entryData: ({})

    property bool inSrs: false

    function updateSrsState() {
        if (srsVM && docId !== -1)
            inSrs = srsVM.contains(docId)
    }

    Component.onCompleted: {
        if (docId !== -1)
            entryData = detailsVM.mapEntry(docId)
        updateSrsState()
    }

    Connections {
        target: srsVM
        function onContainsChanged(changedId) {
            if (changedId === docId)
                updateSrsState()
        }
    }

    background: Rectangle { color: Theme.background }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 14

        // ── Top bar ──────────────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: 0

            // Back button — minimal, text-only
            Item {
                width: backLabel.implicitWidth + 24
                height: 36

                Rectangle {
                    id: backBg
                    anchors.fill: parent
                    radius: 6
                    color: backMouse.containsMouse ? Theme.surfaceHover : "transparent"
                    Behavior on color { ColorAnimation { duration: 120 } }
                }

                RowLayout {
                    anchors.centerIn: parent
                    spacing: 5
                    Text {
                        text: "‹"
                        font.pixelSize: Theme.fontSizeLarge
                        color: hintColor
                    }
                    Text {
                        id: backLabel
                        text: "Back"
                        font.pixelSize: Theme.fontSizeBody
                        font.weight: Font.Medium
                        color: hintColor
                    }
                }

                MouseArea {
                    id: backMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: stack.pop()
                }
            }

            Item { Layout.fillWidth: true }

            // SRS toggle button
            Rectangle {
                width: srsRowLayout.implicitWidth + 24
                height: 34
                radius: 6
                color: inSrs
                    ? Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.15)
                    : Theme.surfaceSubtle
                border.width: 1
                border.color: inSrs
                    ? Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.5)
                    : Theme.surfaceBorder

                Behavior on color { ColorAnimation { duration: 180 } }

                RowLayout {
                    id: srsRowLayout
                    anchors.centerIn: parent
                    spacing: 6

                    Rectangle {
                        width: 7; height: 7; radius: 4
                        color: inSrs ? accentColor : Theme.surfaceInactive
                        Behavior on color { ColorAnimation { duration: 180 } }
                    }

                    Text {
                        text: inSrs ? "In SRS" : "Add to SRS"
                        font.pixelSize: Theme.fontSizeSmall
                        font.weight: Font.Medium
                        color: inSrs ? accentColor : hintColor
                        Behavior on color { ColorAnimation { duration: 180 } }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (inSrs) { srsVM.remove(docId); inSrs = false }
                        else       { srsVM.add(docId);    inSrs = true  }
                    }
                }
            }
        }

        // ── Entry content area ───────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 10
            color: Theme.cardBackground
            border.width: 1
            border.color: dividerColor

            ScrollView {
                anchors.fill: parent
                anchors.margins: 20
                clip: true

                EntryView {
                    width: parent.width
                    entryData: page.entryData
                    mode: "dictionary"
                    textColor:    page.textColor
                    hintColor:    page.hintColor
                    accentColor:  page.accentColor
                    dividerColor: page.dividerColor
                }
            }
        }
    }
}

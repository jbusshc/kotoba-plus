import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../theme"

Page {
    id: page

    property int docId:    -1
    property var entryData: ({})
    property bool inSrs:   false

    background: Rectangle { color: Theme.background }

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
            if (changedId === docId) updateSrsState()
        }
    }

    ColumnLayout {
        anchors { fill: parent; margins: 20 }
        spacing: 14

        // ── Top bar ───────────────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: 0

            BackButton { onClicked: stack.pop() }

            Item { Layout.fillWidth: true }

            // SRS toggle
            Rectangle {
                width:  srsRow.implicitWidth + 24
                height: 34; radius: 6
                color: page.inSrs
                    ? Qt.rgba(Theme.accentColor.r, Theme.accentColor.g, Theme.accentColor.b, 0.15)
                    : Theme.surfaceSubtle
                border.width: 1
                border.color: page.inSrs
                    ? Qt.rgba(Theme.accentColor.r, Theme.accentColor.g, Theme.accentColor.b, 0.5)
                    : Theme.surfaceBorder
                Behavior on color { ColorAnimation { duration: 180 } }

                RowLayout {
                    id: srsRow
                    anchors.centerIn: parent
                    spacing: 6

                    Rectangle {
                        width: 7; height: 7; radius: 4
                        color: page.inSrs ? Theme.accentColor : Theme.surfaceInactive
                        Behavior on color { ColorAnimation { duration: 180 } }
                    }
                    Text {
                        text:           page.inSrs ? "In SRS" : "Add to SRS"
                        font.pixelSize: Theme.fontSizeSmall
                        font.weight:    Font.Medium
                        color: page.inSrs ? Theme.accentColor : Theme.hintColor
                        Behavior on color { ColorAnimation { duration: 180 } }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape:  Qt.PointingHandCursor
                    onClicked: {
                        if (page.inSrs) { srsVM.remove(page.docId); page.inSrs = false }
                        else            { srsVM.add(page.docId);    page.inSrs = true  }
                    }
                }
            }
        }

        // ── Entry content ─────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth:  true
            Layout.fillHeight: true
            radius: 10
            color:  Theme.cardBackground
            border.width: 1; border.color: Theme.dividerColor

            ScrollView {
                anchors { fill: parent; margins: 20 }
                clip: true

                EntryView {
                    width:     parent.width
                    entryData: page.entryData
                    mode:      "dictionary"
                    onNavigateTo: (url, props) => stack.push(url, props)
                }
            }
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../theme"

Page {
    id: page

    property int docId:    -1
    property var entryData: ({})

    // Estado reactivo por tipo de carta
    property bool inSrsRecognition: false
    property bool inSrsRecall:      false

    background: Rectangle { color: Theme.background }

    function updateSrsState() {
        if (!srsVM || docId === -1) return
        inSrsRecognition = srsVM.containsRecognition(docId)
        inSrsRecall      = srsVM.containsRecall(docId)
    }

    Component.onCompleted: {
        if (docId !== -1)
            entryData = detailsVM.mapEntry(docId)
        updateSrsState()
    }

    // containsChanged(entryId, cardTypeMask) — 1=Recognition, 2=Recall, 3=ambos
    Connections {
        target: srsVM
        function onContainsChanged(changedId, mask) {
            if (changedId === docId) updateSrsState()
        }
    }

    ColumnLayout {
        anchors { fill: parent; margins: 20 }
        spacing: 14

        // ── Top bar ───────────────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            BackButton { onClicked: stack.pop() }

            Item { Layout.fillWidth: true }

            // ── Recognition badge ─────────────────────────────────────────────
            Rectangle {
                id: recognitionBadge
                implicitWidth:  recRow.implicitWidth + Theme.fontSizeSmall * 1.5
                implicitHeight: Math.max(Theme.fontSizeSmall * 2.4, 28)
                radius: 6

                color: page.inSrsRecognition
                    ? Qt.rgba(Theme.accentColor.r, Theme.accentColor.g, Theme.accentColor.b, 0.15)
                    : Theme.surfaceSubtle
                border.width: 1
                border.color: page.inSrsRecognition
                    ? Qt.rgba(Theme.accentColor.r, Theme.accentColor.g, Theme.accentColor.b, 0.5)
                    : Theme.surfaceBorder
                Behavior on color { ColorAnimation { duration: 180 } }

                RowLayout {
                    id: recRow
                    anchors.centerIn: parent
                    spacing: 6

                    Rectangle {
                        width: 7; height: 7; radius: 4
                        color: page.inSrsRecognition ? Theme.accentColor : Theme.surfaceInactive
                        Behavior on color { ColorAnimation { duration: 180 } }
                    }
                    Text {
                        text: page.inSrsRecognition ? "Recog. ✓" : "+ Recog."
                        font.pixelSize: Theme.fontSizeSmall
                        font.weight:    Font.Medium
                        color: page.inSrsRecognition ? Theme.accentColor : Theme.hintColor
                        Behavior on color { ColorAnimation { duration: 180 } }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape:  Qt.PointingHandCursor
                    onClicked: {
                        if (page.inSrsRecognition)
                            srsVM.remove(page.docId)
                        else
                            srsVM.add(page.docId)
                    }
                }
            }

            // ── Recall badge ──────────────────────────────────────────────────
            Rectangle {
                id: recallBadge
                implicitWidth:  recallRow.implicitWidth + Theme.fontSizeSmall * 1.5
                implicitHeight: Math.max(Theme.fontSizeSmall * 2.4, 28)
                radius: 6

                readonly property color recallAccent: Qt.rgba(0.96, 0.62, 0.25, 1)

                color: page.inSrsRecall
                    ? Qt.rgba(recallAccent.r, recallAccent.g, recallAccent.b, 0.15)
                    : Theme.surfaceSubtle
                border.width: 1
                border.color: page.inSrsRecall
                    ? Qt.rgba(recallAccent.r, recallAccent.g, recallAccent.b, 0.5)
                    : Theme.surfaceBorder
                Behavior on color { ColorAnimation { duration: 180 } }

                RowLayout {
                    id: recallRow
                    anchors.centerIn: parent
                    spacing: 6

                    Rectangle {
                        width: 7; height: 7; radius: 4
                        color: page.inSrsRecall
                            ? recallBadge.recallAccent
                            : Theme.surfaceInactive
                        Behavior on color { ColorAnimation { duration: 180 } }
                    }
                    Text {
                        text: page.inSrsRecall ? "Recall ✓" : "+ Recall"
                        font.pixelSize: Theme.fontSizeSmall
                        font.weight:    Font.Medium
                        color: page.inSrsRecall ? recallBadge.recallAccent : Theme.hintColor
                        Behavior on color { ColorAnimation { duration: 180 } }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape:  Qt.PointingHandCursor
                    onClicked: {
                        if (page.inSrsRecall)
                            srsVM.removeRecall(page.docId)
                        else
                            srsVM.addRecall(page.docId)
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
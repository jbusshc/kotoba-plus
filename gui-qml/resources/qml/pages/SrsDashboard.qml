import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../theme"

Page {
    id: page
    padding: 0

    background: Rectangle { color: Theme.background }

    ColumnLayout {
        anchors { fill: parent; margins: 24 }
        spacing: 0

        Item { Layout.fillHeight: true }

        // ── Header ────────────────────────────────────────────────────────────
        Column {
            Layout.fillWidth:    true
            Layout.bottomMargin: 24
            spacing: 4

            Text {
                text:           "Study Queue"
                font.pixelSize: Theme.fontSizeTitle
                font.weight:    Font.Bold
                color:          Theme.textColor
            }
            Text {
                text: {
                    if (!srsVM) return ""
                    const due = srsVM.dueCount
                    if (due === 0) return "You're all caught up for now."
                    return due + " card" + (due === 1 ? "" : "s") + " ready to review."
                }
                font.pixelSize: Theme.fontSizeBody
                color:          Theme.hintColor
            }
        }

        // ── Stats: Due + New on top row, Reviewed full-width below ────────────
        Column {
            Layout.fillWidth:    true
            Layout.bottomMargin: 20
            spacing: 10

            Row {
                width:   parent.width
                spacing: 10

                StatCard {
                    heroStyle: true
                    width:  (parent.width - 10) / 2
                    label:  "Due"
                    value:  srsVM ? String(srsVM.dueCount ?? 0) : "0"
                    accent: "#F87171"
                }
                StatCard {
                    heroStyle: true
                    width:  (parent.width - 10) / 2
                    label:  "New"
                    value:  srsVM ? String(srsVM.newCount ?? 0) : "0"
                    accent: "#60A5FA"
                }
            }

            StatCard {
                heroStyle: true
                width:  parent.width
                label:  "Reviewed Today"
                value:  srsVM ? String(srsVM.reviewTodayCount ?? 0) : "0"
                accent: "#34D399"
            }
        }

        // ── Buttons ───────────────────────────────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 10

            ActionButton {
                Layout.fillWidth: true
                primary:     true
                accent:      Theme.accentColor
                label:       srsVM && srsVM.dueCount > 0 ? "Start Study Session" : "Nothing Due"
                sublabel:    srsVM && srsVM.dueCount > 0
                    ? srsVM.dueCount + " card" + (srsVM.dueCount === 1 ? "" : "s") + " in queue"
                    : "Check back later"
                interactive: srsVM ? srsVM.dueCount > 0 : false
                onClicked: {
                    if (srsVM && srsVM.dueCount > 0) {
                        srsVM.startSession()
                        stack.push("qrc:/qml/pages/SrsStudy.qml")
                    }
                }
            }

            ActionButton {
                Layout.fillWidth: true
                label:       "Browse Cards"
                sublabel:    srsVM ? (String(parseInt(srsVM.totalCount) || 0) + " cards in deck") : ""
                interactive: true
                onClicked:   stack.push("qrc:/qml/pages/SrsLibrary.qml")
            }
        }

        Item { Layout.fillHeight: true }
    }
}
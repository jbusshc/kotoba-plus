import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../theme"

Page {
    id: page

    property int entryId:  -1
    property var entryData: ({})

    // ── Reactive stats ────────────────────────────────────────────────────────
    property string statDue:          "—"
    property int    statReps:         0
    property int    statLapses:       0
    property int    statTotalReviews: 0
    property real   statStability:    0
    property real   statDifficulty:   0
    property string statLastReview:   "Never"
    property string statState:        ""

    function refreshStats() {
        if (entryId < 0 || !srsLibraryVM) return
        srsLibraryVM.refresh()
        statDue          = srsLibraryVM.getDue(entryId)
        statReps         = srsLibraryVM.getReps(entryId)
        statLapses       = srsLibraryVM.getLapses(entryId)
        statTotalReviews = srsLibraryVM.getTotalReviews(entryId)
        statStability    = srsLibraryVM.getStability(entryId)
        statDifficulty   = srsLibraryVM.getDifficulty(entryId)
        statLastReview   = srsLibraryVM.getLastReview(entryId)
        statState        = srsLibraryVM.getState(entryId)
    }

    background: Rectangle { color: Theme.background }

    ColumnLayout {
        anchors { fill: parent; margins: 16 }
        spacing: 12

        // ── Top bar ───────────────────────────────────────────────────────────
        Item {
            Layout.fillWidth: true
            height: Theme.minTapTarget

            BackButton {
                anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                onClicked: stack.pop()
            }

            // State badge
            Rectangle {
                anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                height: 28
                width:  badgeRow.implicitWidth + 28
                radius: height / 2
                color:  Theme.srsStateColorBg(statState, 0.18)
                border.width: 1
                border.color: Qt.rgba(
                    Theme.srsStateColor(statState).r,
                    Theme.srsStateColor(statState).g,
                    Theme.srsStateColor(statState).b, 0.40)
                Behavior on color { ColorAnimation { duration: 200 } }

                Row {
                    id: badgeRow
                    anchors.centerIn: parent
                    spacing: 5

                    Text {
                        text:           Theme.srsStateIcon(statState)
                        color:          Theme.srsStateColor(statState)
                        font.pixelSize: Theme.fontSizeXSmall
                        visible:        statState !== ""
                    }
                    Text {
                        text:           statState
                        color:          Theme.srsStateColor(statState)
                        font.pixelSize: Theme.fontSizeSmall; font.weight: Font.Medium
                    }
                }
            }
        }

        // ── Entry view card ───────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth:        true
            Layout.preferredHeight:  parent.height * 0.42
            radius: 10
            border.width: 1; border.color: Theme.dividerColor
            color: Theme.cardBackground

            ScrollView {
                anchors { fill: parent; margins: 16 }
                clip: true

                EntryView {
                    width:     parent.width
                    entryData: page.entryData
                    mode:      "srs"
                    onNavigateTo: (url, props) => stack.push(url, props)
                }
            }
        }

        // ── Stats grid ────────────────────────────────────────────────────────
        Text {
            text:               "Card Stats"
            color:              Theme.hintColor
            font.pixelSize:     Theme.fontSizeXSmall; font.weight: Font.Medium
            font.letterSpacing: 1.2
            Layout.leftMargin:  2
        }

        GridLayout {
            Layout.fillWidth: true
            columns:       3
            columnSpacing: 10; rowSpacing: 10

            StatCard {
                Layout.fillWidth: true
                label: "Due";     value: statDue;  icon: "⏰"
                accent: Theme.accentColor
            }
            StatCard {
                Layout.fillWidth: true
                label: "Reviews"; value: statTotalReviews.toString(); icon: "🔁"
                accent: "#4CAF7D"
            }
            StatCard {
                Layout.fillWidth: true
                label: "Lapses";  value: statLapses.toString(); icon: "⚡"
                accent: statLapses > 0 ? "#FF7043" : Theme.hintColor
            }
            StatCard {
                Layout.fillWidth: true
                label: "Stability"
                value: statState === "New" ? "—" : statStability.toFixed(1) + "d"
                icon:  "◈"; accent: "#60A5FA"
            }
            StatCard {
                Layout.fillWidth: true
                label: "Difficulty"
                value: statState === "New" ? "—" : statDifficulty.toFixed(2)
                icon:  "◇"
                accent: statDifficulty > 7 ? "#FF7043" : statDifficulty > 4 ? "#FFB83F" : "#4CAF7D"
            }
            StatCard {
                Layout.fillWidth: true
                label: "Last seen"; value: statLastReview; icon: "◷"
                accent: Theme.hintColor
            }
        }

        // ── Actions ───────────────────────────────────────────────────────────
        Text {
            text:               "Actions"
            color:              Theme.hintColor
            font.pixelSize:     Theme.fontSizeXSmall; font.weight: Font.Medium
            font.letterSpacing: 1.2
            Layout.leftMargin:  2
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            // Reusable action button for this page
            component CardActionBtn: Rectangle {
                property string label:      ""
                property color  labelColor: Theme.textColor
                signal tapped()

                Layout.fillWidth: true
                height: Theme.minTapTarget - 8
                radius: 7
                color: ma.pressed ? Theme.surfacePress
                     : ma.containsMouse ? Theme.surfaceHover : Theme.surfaceSubtle
                border.width: 1; border.color: Theme.surfaceBorder
                Behavior on color { ColorAnimation { duration: 100 } }

                Text {
                    anchors.centerIn: parent
                    width:            parent.width - 8
                    text:             parent.label
                    font.pixelSize:   Theme.fontSizeSmall; font.weight: Font.Medium
                    color:            parent.labelColor
                    horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight
                }
                MouseArea {
                    id: ma; anchors.fill: parent
                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: parent.tapped()
                }
            }

            CardActionBtn {
                label:      statState === "Suspended" ? "Unsuspend" : "Suspend"
                labelColor: statState === "Suspended" ? "#34D399" : "#FFB83F"
                onTapped: { suspendDialog.unsuspending = (statState === "Suspended"); suspendDialog.open() }
            }
            CardActionBtn {
                label: "Reset"; labelColor: "#60A5FA"
                onTapped: resetDialog.open()
            }
            CardActionBtn {
                label: "Delete"; labelColor: "#F87171"
                onTapped: deleteDialog.open()
            }
        }

        Item { Layout.fillHeight: true }
    }

    // ── Dialogs ───────────────────────────────────────────────────────────────
    Dialog {
        id: resetDialog
        title: "Reset card?"
        width: 320; anchors.centerIn: Overlay.overlay; modal: true

        Text {
            text: "This will move the card back to New state and clear all progress."
            wrapMode: Text.Wrap; width: 260
            color: Theme.textColor; font.pixelSize: Theme.fontSizeBody
        }
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: { srsLibraryVM.reset(entryId); refreshStats() }
    }

    Dialog {
        id: suspendDialog
        width: 320; anchors.centerIn: Overlay.overlay; modal: true
        property bool unsuspending: false
        title: unsuspending ? "Unsuspend card?" : "Suspend card?"

        Text {
            text: suspendDialog.unsuspending
                ? "The card will return to its previous state and become available for review."
                : "The card will be removed from all queues and won't appear in reviews."
            wrapMode: Text.Wrap; width: 260
            color: Theme.textColor; font.pixelSize: Theme.fontSizeBody
        }
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            if (unsuspending) srsLibraryVM.unsuspend(entryId)
            else              srsLibraryVM.suspend(entryId)
            refreshStats()
        }
    }

    Dialog {
        id: deleteDialog
        title: "Delete card?"
        width: 320; anchors.centerIn: Overlay.overlay; modal: true
        property int deletedEntryId: -1

        Text {
            text: "This will permanently remove the card and all its progress from your deck."
            wrapMode: Text.Wrap; width: 260
            color: Theme.textColor; font.pixelSize: Theme.fontSizeBody
        }
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            deleteDialog.deletedEntryId = entryId
            srsLibraryVM.remove(entryId)
            readdDialog.open()
            stack.pop()
        }
    }

    Dialog {
        id: readdDialog
        title: "Card deleted"
        width: 320; anchors.centerIn: Overlay.overlay; modal: true

        Text {
            text: "The card was removed. Would you like to add it back as new?"
            wrapMode: Text.Wrap; width: 260
            color: Theme.textColor; font.pixelSize: Theme.fontSizeBody
        }
        standardButtons: Dialog.Yes | Dialog.No
        onAccepted: {
            if (deleteDialog.deletedEntryId >= 0)
                srsVM.add(deleteDialog.deletedEntryId)
        }
    }

    Connections {
        target: srsVM
        function onStatsChanged() { refreshStats() }
    }

    Component.onCompleted: {
        if (entryId >= 0) {
            entryData = detailsVM.mapEntry(entryId)
            refreshStats()
        }
    }
}

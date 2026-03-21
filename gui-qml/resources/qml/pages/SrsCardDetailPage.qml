import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Kotoba 1.0
import "../components"

Page {
    id: page

    property int entryId: -1
    property var entryData: ({})

    // ── reactive stats ───────────────────────────────────────────────────────
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

    property color textColor:    Theme.textColor
    property color hintColor:    Theme.hintColor
    property color accentColor:  Theme.accentColor
    property color dividerColor: Theme.dividerColor

    function stateColor(state) {
        switch (state) {
            case "New":        return "#4A9EFF"
            case "Learning":   return "#FFB83F"
            case "Relearning": return "#FF7043"
            case "Review":     return "#4CAF7D"
            case "Suspended":  return "#9E9E9E"
            default:           return hintColor
        }
    }

    function stateIcon(state) {
        switch (state) {
            case "New":        return "✦"
            case "Learning":   return "◎"
            case "Relearning": return "↺"
            case "Review":     return "✓"
            case "Suspended":  return "⏸"
            default:           return ""
        }
    }

    // ── root layout ──────────────────────────────────────────────────────────
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // ── top bar ──────────────────────────────────────────────────────────
        Item {
            Layout.fillWidth: true
            height: 36

            Button {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                text: "← Back"
                onClicked: stack.pop()
            }

            // Badge anchored to right — not in flow, so it never affects layout
            Rectangle {
                id: stateBadgeRect
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                height: 28
                width: badgeRow.implicitWidth + 28
                radius: height / 2
                color: Qt.rgba(
                    stateColor(statState).r,
                    stateColor(statState).g,
                    stateColor(statState).b, 0.18)
                border.width: 1
                border.color: Qt.rgba(
                    stateColor(statState).r,
                    stateColor(statState).g,
                    stateColor(statState).b, 0.40)

                Behavior on color { ColorAnimation { duration: 200 } }

                Row {
                    id: badgeRow
                    spacing: 5

                    Text {
                        text: stateIcon(statState)
                        color: stateColor(statState)
                        font.pixelSize: Theme.fontSizeXSmall
                        visible: statState !== ""
                    }
                    Text {
                        text: statState
                        color: stateColor(statState)
                        font.pixelSize: Theme.fontSizeSmall
                        font.weight: Font.Medium
                    }
                }
            }
        }

        // ── entry view card ───────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: parent.height * 0.42
            radius: 10
            border.width: 1
            border.color: dividerColor
            color: Theme.cardBackground

            ScrollView {
                anchors.fill: parent
                anchors.margins: 16
                clip: true

                EntryView {
                    width: parent.width
                    entryData: page.entryData
                    mode: "srs"
                    textColor:    page.textColor
                    hintColor:    page.hintColor
                    accentColor:  page.accentColor
                    dividerColor: page.dividerColor
                }
            }
        }

        // ── SRS stats grid ────────────────────────────────────────────────────
        Text {
            text: "Card Stats"
            color: hintColor
            font.pixelSize: Theme.fontSizeXSmall
            font.weight: Font.Medium
            font.letterSpacing: 1.2
            Layout.leftMargin: 2
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 3
            columnSpacing: 10
            rowSpacing: 10

            StatCard {
                Layout.fillWidth: true
                label: "Due"
                value: statDue
                icon:  "⏰"
                accent:      accentColor
                bg:          Theme.cardBackground
                borderColor: dividerColor
                hint:        hintColor
                fg:          textColor
            }

            StatCard {
                Layout.fillWidth: true
                label: "Reviews"
                value: statTotalReviews.toString()
                icon:  "🔁"
                accent:      "#4CAF7D"
                bg:          Theme.cardBackground
                borderColor: dividerColor
                hint:        hintColor
                fg:          textColor
            }

            StatCard {
                Layout.fillWidth: true
                label: "Lapses"
                value: statLapses.toString()
                icon:  "⚡"
                accent:      statLapses > 0 ? "#FF7043" : hintColor
                bg:          Theme.cardBackground
                borderColor: dividerColor
                hint:        hintColor
                fg:          textColor
            }

            StatCard {
                Layout.fillWidth: true
                label: "Stability"
                value: statState === "New" ? "—" : statStability.toFixed(1) + "d"
                icon:  "◈"
                accent:      "#60A5FA"
                bg:          Theme.cardBackground
                borderColor: dividerColor
                hint:        hintColor
                fg:          textColor
            }

            StatCard {
                Layout.fillWidth: true
                label: "Difficulty"
                value: statState === "New" ? "—" : statDifficulty.toFixed(2)
                icon:  "◇"
                accent:      statDifficulty > 7 ? "#FF7043" : statDifficulty > 4 ? "#FFB83F" : "#4CAF7D"
                bg:          Theme.cardBackground
                borderColor: dividerColor
                hint:        hintColor
                fg:          textColor
            }

            StatCard {
                Layout.fillWidth: true
                label: "Last seen"
                value: statLastReview
                icon:  "◷"
                accent:      hintColor
                bg:          Theme.cardBackground
                borderColor: dividerColor
                hint:        hintColor
                fg:          textColor
            }
        }

        // ── actions ───────────────────────────────────────────────────────────
        Text {
            text: "Actions"
            color: hintColor
            font.pixelSize: Theme.fontSizeXSmall
            font.weight: Font.Medium
            font.letterSpacing: 1.2
            Layout.leftMargin: 2
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            // Suspend / Unsuspend toggle based on current state
            Rectangle {
                Layout.fillWidth: true
                height: 36
                radius: 7
                color: actionSuspendMouse.containsMouse
                    ? Qt.rgba(1,1,1,0.08) : Qt.rgba(1,1,1,0.04)
                border.width: 1
                border.color: Qt.rgba(1,1,1,0.10)
                Behavior on color { ColorAnimation { duration: 100 } }

                Text {
                    anchors.centerIn: parent
                    width: parent.width - 8
                    text: statState === "Suspended" ? "Unsuspend" : "Suspend"
                    font.pixelSize: Theme.fontSizeSmall
                    font.weight: Font.Medium
                    color: statState === "Suspended" ? "#34D399" : "#FFB83F"
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                }

                MouseArea {
                    id: actionSuspendMouse
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (statState === "Suspended")
                            suspendDialog.unsuspending = true
                        else
                            suspendDialog.unsuspending = false
                        suspendDialog.open()
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 36
                radius: 7
                color: actionResetMouse.containsMouse
                    ? Qt.rgba(1,1,1,0.08) : Qt.rgba(1,1,1,0.04)
                border.width: 1
                border.color: Qt.rgba(1,1,1,0.10)
                Behavior on color { ColorAnimation { duration: 100 } }

                Text {
                    anchors.centerIn: parent
                    width: parent.width - 8
                    text: "Reset"
                    font.pixelSize: Theme.fontSizeSmall
                    font.weight: Font.Medium
                    color: "#60A5FA"
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                }

                MouseArea {
                    id: actionResetMouse
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: resetDialog.open()
                }
            }


            Rectangle {
                Layout.fillWidth: true
                height: 36
                radius: 7
                color: actionDeleteMouse.containsMouse
                    ? Qt.rgba(1,1,1,0.08) : Qt.rgba(1,1,1,0.04)
                border.width: 1
                border.color: Qt.rgba(1,1,1,0.10)
                Behavior on color { ColorAnimation { duration: 100 } }

                Text {
                    anchors.centerIn: parent
                    width: parent.width - 8
                    text: "Delete"
                    font.pixelSize: Theme.fontSizeSmall
                    font.weight: Font.Medium
                    color: "#F87171"
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                }

                MouseArea {
                    id: actionDeleteMouse
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: deleteDialog.open()
                }
            }
        }

        Item { Layout.fillHeight: true }
    }

    // ── StatCard component ────────────────────────────────────────────────────
    component StatCard: Rectangle {
        property string label:       ""
        property string value:       "—"
        property string icon:        ""
        property color  accent:      "white"
        property color  bg:          "transparent"
        property color  borderColor: "#333"
        property color  hint:        "#888"
        property color  fg:          "white"

        height: 72
        radius: 8
        color: bg
        border.width: 1
        border.color: borderColor

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 2

            RowLayout {
                spacing: 4
                Text { text: icon; font.pixelSize: Theme.fontSizeBody }
                Text {
                    text: label
                    color: hint
                    font.pixelSize: Theme.fontSizeXSmall
                    font.weight: Font.Medium
                    font.letterSpacing: 0.8
                }
            }

            Text {
                text: value
                color: accent
                font.pixelSize: Theme.fontSizeLarge
                font.weight: Font.Bold
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }
    }

    // ── dialogs ───────────────────────────────────────────────────────────────

    // Reset confirmation
    Dialog {
        id: resetDialog
        title: "Reset card?"
        width: 320
        anchors.centerIn: Overlay.overlay
        modal: true


        Text {
            text: "This will move the card back to New state and clear all progress."
            wrapMode: Text.Wrap
            width: 260
            color: page.textColor
        }

        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            srsLibraryVM.reset(entryId)
            refreshStats()
        }
    }


    // Suspend / Unsuspend confirmation
    Dialog {
        id: suspendDialog
        width: 320
        anchors.centerIn: Overlay.overlay
        modal: true
        property bool unsuspending: false

        title: unsuspending ? "Unsuspend card?" : "Suspend card?"

        Text {
            text: suspendDialog.unsuspending
                ? "The card will return to its previous state and become available for review."
                : "The card will be removed from all queues and won't appear in reviews."
            wrapMode: Text.Wrap
            width: 260
            color: page.textColor
        }

        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            if (unsuspending)
                srsLibraryVM.unsuspend(entryId)
            else
                srsLibraryVM.suspend(entryId)
            refreshStats()
        }
    }

    // Delete confirmation — offers re-add after deletion
    Dialog {
        id: deleteDialog
        title: "Delete card?"
        width: 320
        anchors.centerIn: Overlay.overlay
        modal: true

        property int deletedEntryId: -1

        Text {
            text: "This will permanently remove the card and all its progress from your deck."
            wrapMode: Text.Wrap
            width: 260
            color: page.textColor
        }

        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            deleteDialog.deletedEntryId = entryId
            srsLibraryVM.remove(entryId)
            readdDialog.open()
            stack.pop()
        }
    }

    // Re-add prompt shown immediately after deletion
    Dialog {
        id: readdDialog
        title: "Card deleted"
        width: 320
        anchors.centerIn: Overlay.overlay
        modal: true


        Text {
            text: "The card was removed. Would you like to add it back as new?"
            wrapMode: Text.Wrap
            width: 260
            color: page.textColor
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

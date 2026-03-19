import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Kotoba 1.0
import "../components"

Page {
    id: page

    property int entryId: -1
    property var entryData: ({})

    property color textColor:    Theme.textColor
    property color hintColor:    Theme.hintColor
    property color accentColor:  Theme.accentColor
    property color dividerColor: Theme.dividerColor

    // ── helpers ────────────────────────────────────────────────────────────────

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
            default:           return "?"
        }
    }

    // ── root layout ────────────────────────────────────────────────────────────

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // ── top bar ──────────────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true

            Button {
                text: "← Back"
                onClicked: stack.pop()
            }

            Item { Layout.fillWidth: true }

            // Live state badge
            Rectangle {
                id: stateBadge
                height: 28
                width: stateLabel.implicitWidth + 28
                radius: height / 2
                color: stateColor(srsLibraryVM.getState ? srsLibraryVM.getState(entryId) : "")
                opacity: 0.15
                visible: false  // used as background; actual badge below
            }

            Rectangle {
                height: 28
                width: badgeRow.implicitWidth + 28
                radius: height / 2
                color: Qt.rgba(stateColorValue.r, stateColorValue.g, stateColorValue.b, 0.18)

                ColorAnimation on color { duration: 300 }

                // helper to expose parsed color
                property color stateColorValue: stateColor(cardState.text)

                RowLayout {
                    id: badgeRow
                    anchors.centerIn: parent
                    spacing: 4

                    Text {
                        text: stateIcon(cardState.text)
                        color: stateColor(cardState.text)
                        font.pixelSize: 11
                    }
                    Text {
                        id: cardState
                        text: srsLibraryVM.getState ? srsLibraryVM.getState(entryId) : "—"
                        color: stateColor(text)
                        font.pixelSize: 12
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
            font.pixelSize: 11
            font.weight: Font.Medium
            font.letterSpacing: 1.2
            Layout.leftMargin: 2
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 3
            columnSpacing: 10
            rowSpacing: 10

            // ── Due ──
            StatCard {
                Layout.fillWidth: true
                label: "Due"
                value: srsLibraryVM.getDue(entryId)
                icon:  "⏰"
                accent:      accentColor
                bg:          Theme.cardBackground
                borderColor: dividerColor
                hint:        hintColor
                fg:          textColor
            }

            // ── Reps ──
            StatCard {
                Layout.fillWidth: true
                label: "Reviews"
                value: srsLibraryVM.getReps(entryId).toString()
                icon:  "🔁"
                accent:      "#4CAF7D"
                bg:          Theme.cardBackground
                borderColor: dividerColor
                hint:        hintColor
                fg:          textColor
            }

            // ── Lapses ──
            StatCard {
                Layout.fillWidth: true
                label: "Lapses"
                value: srsLibraryVM.getLapses(entryId).toString()
                icon:  "⚡"
                accent:      srsLibraryVM.getLapses(entryId) > 0 ? "#FF7043" : hintColor
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
            font.pixelSize: 11
            font.weight: Font.Medium
            font.letterSpacing: 1.2
            Layout.leftMargin: 2
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            // Suspend / Unsuspend toggle
            Button {
                Layout.fillWidth: true
                text: "Suspend"
                onClicked: {
                    srsLibraryVM.suspend(entryId)
                }
            }

            Button {
                Layout.fillWidth: true
                text: "Reset"
                onClicked: {
                    resetDialog.open()
                }
            }

            Button {
                Layout.fillWidth: true
                text: "Delete"
                onClicked: {
                    deleteDialog.open()
                }
            }
        }

        Item { Layout.fillHeight: true }
    }

    // ── inline StatCard component ─────────────────────────────────────────────
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
                Text { text: icon;  font.pixelSize: 13 }
                Text {
                    text: label
                    color: hint
                    font.pixelSize: 11
                    font.weight: Font.Medium
                    font.letterSpacing: 0.8
                }
            }

            Text {
                text: value
                color: accent
                font.pixelSize: 18
                font.weight: Font.Bold
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }
    }

    // ── confirmation dialogs ──────────────────────────────────────────────────
    Dialog {
        id: resetDialog
        title: "Reset card?"
        modal: true
        anchors.centerIn: parent

        Text {
            text: "This will move the card back to New state and clear all progress."
            wrapMode: Text.Wrap
            width: 260
            color: page.textColor
        }

        standardButtons: Dialog.Ok | Dialog.Cancel

        onAccepted: {
            srsLibraryVM.reset(entryId)
        }
    }

    Dialog {
        id: deleteDialog
        title: "Delete card?"
        modal: true
        anchors.centerIn: parent

        Text {
            text: "This will permanently remove the card from your deck."
            wrapMode: Text.Wrap
            width: 260
            color: page.textColor
        }

        standardButtons: Dialog.Ok | Dialog.Cancel

        onAccepted: {
            srsLibraryVM.remove(entryId)
            stack.pop()
        }
    }

    // ── init ──────────────────────────────────────────────────────────────────
    Component.onCompleted: {
        if (entryId >= 0)
            entryData = detailsVM.mapEntry(entryId)
    }
}
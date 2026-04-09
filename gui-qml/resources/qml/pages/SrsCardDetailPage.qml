import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../theme"

Page {
    id: page
    padding: 0

    property int entryId:   -1
    property var entryData: ({})

    property var recog: ({
        present: false, state: "", due: "—",
        reps: 0, lapses: 0, totalReviews: 0,
        stability: 0, difficulty: 0, lastReview: "Never"
    })
    property var recall: ({
        present: false, state: "", due: "—",
        reps: 0, lapses: 0, totalReviews: 0,
        stability: 0, difficulty: 0, lastReview: "Never"
    })

    function refreshStats() {
        if (entryId < 0 || !srsLibraryVM) return
        srsLibraryVM.refresh()
        recog = {
            present:      srsVM.containsRecognition(entryId),
            state:        srsLibraryVM.getState(entryId, 0),
            due:          srsLibraryVM.getDue(entryId, 0),
            reps:         srsLibraryVM.getReps(entryId, 0),
            lapses:       srsLibraryVM.getLapses(entryId, 0),
            totalReviews: srsLibraryVM.getTotalReviews(entryId, 0),
            stability:    srsLibraryVM.getStability(entryId, 0),
            difficulty:   srsLibraryVM.getDifficulty(entryId, 0),
            lastReview:   srsLibraryVM.getLastReview(entryId, 0)
        }
        recall = {
            present:      srsVM.containsRecall(entryId),
            state:        srsLibraryVM.getState(entryId, 1),
            due:          srsLibraryVM.getDue(entryId, 1),
            reps:         srsLibraryVM.getReps(entryId, 1),
            lapses:       srsLibraryVM.getLapses(entryId, 1),
            totalReviews: srsLibraryVM.getTotalReviews(entryId, 1),
            stability:    srsLibraryVM.getStability(entryId, 1),
            difficulty:   srsLibraryVM.getDifficulty(entryId, 1),
            lastReview:   srsLibraryVM.getLastReview(entryId, 1)
        }
    }

    background: Rectangle { color: Theme.background }

    // ── Flickable raíz — todo el contenido scrollea junto ────────────────────
    Flickable {
        id:          flick
        anchors.fill: parent
        contentWidth: width
        contentHeight: mainCol.implicitHeight + 32
        clip:          true

        // Columna principal: sin ColumnLayout, sin anchors en hijos críticos
        Column {
            id:      mainCol
            x:       16
            y:       16
            width:   flick.width - 32
            spacing: 16

            // ── Back button ───────────────────────────────────────────────────
            Item {
                width:  parent.width
                height: Theme.minTapTarget
                BackButton {
                    anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                    onClicked: stack.pop()
                }
            }

            // ── EntryView — sin altura fija, crece con el contenido ───────────
            Rectangle {
                width:  parent.width
                // La altura la dicta el EntryView interior + padding
                height: entryViewItem.implicitHeight + 32
                radius: 10
                border.width: 1; border.color: Theme.dividerColor
                color: Theme.cardBackground

                EntryView {
                    id:       entryViewItem
                    x:        16; y: 16
                    width:    parent.width - 32
                    entryData: page.entryData
                    mode:     "srs"
                    onNavigateTo: (url, props) => stack.push(url, props)
                }
            }

            // ── Sección Recognition ───────────────────────────────────────────
            CardSectionBox {
                width:     parent.width
                cardLabel: "Recognition"
                cardType:  0
                stats:     page.recog
            }

            // ── Sección Recall ────────────────────────────────────────────────
            CardSectionBox {
                width:     parent.width
                cardLabel: "Recall"
                cardType:  1
                stats:     page.recall
            }

            // padding bottom
            Item { width: 1; height: 8 }
        }

        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
    }

    // ── CardSectionBox — componente inline ────────────────────────────────────
    // Rectangle cuya altura = sectionCol.implicitHeight + padding.
    // sectionCol es un Column (no ColumnLayout) posicionado con x/y/width,
    // así implicitHeight fluye correctamente al padre.
    component CardSectionBox: Rectangle {
        id: sectionBox

        property string cardLabel: ""
        property int    cardType:  0
        property var    stats:     ({})

        readonly property bool present: stats.present ?? false

        radius:       10
        border.width: 1; border.color: Theme.dividerColor
        color:        Theme.cardBackground
        // Altura derivada del Column interior
        height:       sectionCol.implicitHeight + 28

        Column {
            id:      sectionCol
            x:       14; y: 14
            width:   parent.width - 28
            spacing: 10

            // ── Cabecera ──────────────────────────────────────────────────────
            Row {
                width:   parent.width
                spacing: 8

                Text {
                    text:               sectionBox.cardLabel.toUpperCase()
                    color:              Theme.hintColor
                    font.pixelSize:     Theme.fontSizeXSmall
                    font.weight:        Font.Medium
                    font.letterSpacing: 1.2
                    anchors.verticalCenter: parent.verticalCenter
                }

                Item { width: parent.width - sectionBox.cardLabel.length * 8 - stateBadge.width - notAddedTxt.width - 16; height: 1 }

                // Badge estado
                Rectangle {
                    id:      stateBadge
                    visible: sectionBox.present && (sectionBox.stats.state ?? "") !== ""
                    height:  22
                    width:   visible ? sbRow.implicitWidth + 16 : 0
                    radius:  height / 2
                    color:   Theme.srsStateColorBg(sectionBox.stats.state ?? "", 0.18)
                    border.width: 1
                    border.color: Qt.rgba(
                        Theme.srsStateColor(sectionBox.stats.state ?? "").r,
                        Theme.srsStateColor(sectionBox.stats.state ?? "").g,
                        Theme.srsStateColor(sectionBox.stats.state ?? "").b, 0.40)

                    Row {
                        id: sbRow
                        anchors.centerIn: parent
                        spacing: 4
                        Text {
                            text:           Theme.srsStateIcon(sectionBox.stats.state ?? "")
                            color:          Theme.srsStateColor(sectionBox.stats.state ?? "")
                            font.pixelSize: Theme.fontSizeTiny
                            visible:        (sectionBox.stats.state ?? "") !== ""
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text:           sectionBox.stats.state ?? ""
                            color:          Theme.srsStateColor(sectionBox.stats.state ?? "")
                            font.pixelSize: Theme.fontSizeXSmall
                            font.weight:    Font.Medium
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }

                Text {
                    id:      notAddedTxt
                    visible: !sectionBox.present
                    text:    "Not added"
                    color:   Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.45)
                    font.pixelSize: Theme.fontSizeXSmall
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            // ── Stats grid (solo si presente) ─────────────────────────────────
            Grid {
                visible:  sectionBox.present
                width:    parent.width
                columns:  3
                spacing:  8

                StatCard {
                    width:  (parent.width - 16) / 3
                    label: "Due";    value: sectionBox.stats.due ?? "—"; icon: "⏰"
                    accent: Theme.accentColor
                }
                StatCard {
                    width:  (parent.width - 16) / 3
                    label: "Reviews"; value: (sectionBox.stats.totalReviews ?? 0).toString(); icon: "🔁"
                    accent: "#4CAF7D"
                }
                StatCard {
                    width:  (parent.width - 16) / 3
                    label: "Lapses";  value: (sectionBox.stats.lapses ?? 0).toString(); icon: "⚡"
                    accent: (sectionBox.stats.lapses ?? 0) > 0 ? "#FF7043" : Theme.hintColor
                }
                StatCard {
                    width:  (parent.width - 16) / 3
                    label: "Stability"
                    value: (sectionBox.stats.state ?? "") === "New" ? "—"
                         : (sectionBox.stats.stability ?? 0).toFixed(1) + "d"
                    icon:  "◈"; accent: "#60A5FA"
                }
                StatCard {
                    width:  (parent.width - 16) / 3
                    label: "Difficulty"
                    value: (sectionBox.stats.state ?? "") === "New" ? "—"
                         : (sectionBox.stats.difficulty ?? 0).toFixed(2)
                    icon:  "◇"
                    accent: (sectionBox.stats.difficulty ?? 0) > 7 ? "#FF7043"
                          : (sectionBox.stats.difficulty ?? 0) > 4 ? "#FFB83F" : "#4CAF7D"
                }
                StatCard {
                    width:  (parent.width - 16) / 3
                    label: "Last seen"; value: sectionBox.stats.lastReview ?? "Never"; icon: "◷"
                    accent: Theme.hintColor
                }
            }

            // ── Botones de acción ─────────────────────────────────────────────
            Row {
                width:   parent.width
                spacing: 8

                // Suspend / Unsuspend
                Rectangle {
                    visible: sectionBox.present
                    width:   (parent.width - 16) / 3
                    height:  Theme.minTapTarget - 8
                    radius:  7
                    color:   suspMa.pressed ? Theme.surfacePress
                           : suspMa.containsMouse ? Theme.surfaceHover : Theme.surfaceSubtle
                    border.width: 1; border.color: Theme.surfaceBorder
                    Behavior on color { ColorAnimation { duration: 100 } }
                    Text {
                        anchors.centerIn: parent
                        text:           (sectionBox.stats.state ?? "") === "Suspended" ? "Unsuspend" : "Suspend"
                        font.pixelSize: Theme.fontSizeSmall; font.weight: Font.Medium
                        color:          (sectionBox.stats.state ?? "") === "Suspended" ? "#34D399" : "#FFB83F"
                        horizontalAlignment: Text.AlignHCenter
                    }
                    MouseArea {
                        id: suspMa; anchors.fill: parent
                        hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            actionDialog.cardType   = sectionBox.cardType
                            actionDialog.cardLabel  = sectionBox.cardLabel
                            actionDialog.actionKind = (sectionBox.stats.state ?? "") === "Suspended"
                                                      ? "unsuspend" : "suspend"
                            actionDialog.open()
                        }
                    }
                }

                // Reset
                Rectangle {
                    visible: sectionBox.present
                    width:   (parent.width - 16) / 3
                    height:  Theme.minTapTarget - 8
                    radius:  7
                    color:   resetMa.pressed ? Theme.surfacePress
                           : resetMa.containsMouse ? Theme.surfaceHover : Theme.surfaceSubtle
                    border.width: 1; border.color: Theme.surfaceBorder
                    Behavior on color { ColorAnimation { duration: 100 } }
                    Text {
                        anchors.centerIn: parent
                        text: "Reset"; font.pixelSize: Theme.fontSizeSmall; font.weight: Font.Medium
                        color: "#60A5FA"
                    }
                    MouseArea {
                        id: resetMa; anchors.fill: parent
                        hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            actionDialog.cardType   = sectionBox.cardType
                            actionDialog.cardLabel  = sectionBox.cardLabel
                            actionDialog.actionKind = "reset"
                            actionDialog.open()
                        }
                    }
                }

                // Delete
                Rectangle {
                    visible: sectionBox.present
                    width:   (parent.width - 16) / 3
                    height:  Theme.minTapTarget - 8
                    radius:  7
                    color:   deleteMa.pressed ? Theme.surfacePress
                           : deleteMa.containsMouse ? Theme.surfaceHover : Theme.surfaceSubtle
                    border.width: 1; border.color: Theme.surfaceBorder
                    Behavior on color { ColorAnimation { duration: 100 } }
                    Text {
                        anchors.centerIn: parent
                        text: "Delete"; font.pixelSize: Theme.fontSizeSmall; font.weight: Font.Medium
                        color: "#F87171"
                    }
                    MouseArea {
                        id: deleteMa; anchors.fill: parent
                        hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            actionDialog.cardType   = sectionBox.cardType
                            actionDialog.cardLabel  = sectionBox.cardLabel
                            actionDialog.actionKind = "delete"
                            actionDialog.open()
                        }
                    }
                }

                // Add (si no está presente)
                Rectangle {
                    visible: !sectionBox.present
                    width:   parent.width
                    height:  Theme.minTapTarget - 8
                    radius:  7
                    color:   addMa.pressed ? Theme.surfacePress
                           : addMa.containsMouse ? Theme.surfaceHover : Theme.surfaceSubtle
                    border.width: 1; border.color: Theme.surfaceBorder
                    Behavior on color { ColorAnimation { duration: 100 } }
                    Text {
                        anchors.centerIn: parent
                        text:           "Add " + sectionBox.cardLabel
                        font.pixelSize: Theme.fontSizeSmall; font.weight: Font.Medium
                        color:          Theme.accentColor
                    }
                    MouseArea {
                        id: addMa; anchors.fill: parent
                        hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (sectionBox.cardType === 1) srsVM.addRecall(entryId)
                            else                          srsVM.add(entryId)
                            refreshStats()
                        }
                    }
                }
            }
        }
    }

    // ── Diálogo único de acción ───────────────────────────────────────────────
    Dialog {
        id: actionDialog
        width: 320; anchors.centerIn: Overlay.overlay; modal: true

        property int    cardType:   0
        property string cardLabel:  ""
        property string actionKind: ""

        title: {
            switch (actionKind) {
                case "suspend":   return "Suspend "   + cardLabel + "?"
                case "unsuspend": return "Unsuspend " + cardLabel + "?"
                case "reset":     return "Reset "     + cardLabel + "?"
                case "delete":    return "Delete "    + cardLabel + "?"
                default:          return ""
            }
        }

        Text {
            width: 260; wrapMode: Text.Wrap
            color: Theme.textColor; font.pixelSize: Theme.fontSizeBody
            text: {
                switch (actionDialog.actionKind) {
                    case "suspend":   return "The card will be removed from all queues."
                    case "unsuspend": return "The card will return to its previous state."
                    case "reset":     return "The card will be moved back to New state."
                    case "delete":    return "This will permanently remove the card and its progress."
                    default:          return ""
                }
            }
        }

        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            const id = entryId
            const ct = actionDialog.cardType
            switch (actionDialog.actionKind) {
                case "suspend":   srsLibraryVM.suspend(id, ct);   break
                case "unsuspend": srsLibraryVM.unsuspend(id, ct); break
                case "reset":     srsLibraryVM.reset(id, ct);     break
                case "delete":
                    srsLibraryVM.remove(id, ct)
                    readdDialog.deletedEntryId  = id
                    readdDialog.deletedCardType = ct
                    readdDialog.open()
                    stack.pop()
                    return
            }
            refreshStats()
        }
    }

    Dialog {
        id: readdDialog
        title: "Card deleted"
        width: 320; anchors.centerIn: Overlay.overlay; modal: true
        property int deletedEntryId:  -1
        property int deletedCardType: 0
        Text {
            text: "Would you like to add it back as new?"
            wrapMode: Text.Wrap; width: 260
            color: Theme.textColor; font.pixelSize: Theme.fontSizeBody
        }
        standardButtons: Dialog.Yes | Dialog.No
        onAccepted: {
            if (deletedEntryId >= 0) {
                if (deletedCardType === 1) srsVM.addRecall(deletedEntryId)
                else                      srsVM.add(deletedEntryId)
            }
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
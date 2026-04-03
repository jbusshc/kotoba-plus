import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../theme"

Page {
    id: page
    title:   "SRS Library"
    padding: 0

    background: Rectangle { color: Theme.background }

    Component.onCompleted: {
        if (srsLibraryVM) srsLibraryVM.setFilter("All")
    }

    // ── Filter pill (library-specific chip, not a generic component) ──────────
    component FilterPill: Rectangle {
        property string label:  ""
        property bool   active: false
        signal clicked()

        height: Theme.minTapTarget - 20
        width:  lbl.implicitWidth + 20
        radius: height / 2

        readonly property color _stateColor: Theme.srsStateColor(label, Theme.accentColor)

        color: active
            ? Qt.rgba(_stateColor.r, _stateColor.g, _stateColor.b, 0.20)
            : Theme.surfaceSubtle
        border.width: 1
        border.color: active
            ? Qt.rgba(_stateColor.r, _stateColor.g, _stateColor.b, 0.55)
            : Theme.surfaceBorder
        Behavior on color { ColorAnimation { duration: 130 } }

        Text {
            id: lbl
            anchors.centerIn: parent
            text:           parent.label
            font.pixelSize: Theme.fontSizeXSmall; font.weight: Font.Medium
            color: parent.active
                ? parent._stateColor
                : Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.7)
            Behavior on color { ColorAnimation { duration: 130 } }
        }

        MouseArea {
            anchors.centerIn: parent
            width: parent.width; height: Theme.minTapTarget
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
        }
    }

    // ── Layout ────────────────────────────────────────────────────────────────
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Top bar ───────────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            color:  Theme.cardBackground
            height: topCol.implicitHeight + 20

            Rectangle {
                anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
                height: 1; color: Theme.dividerColor
            }

            Column {
                id: topCol
                anchors { fill: parent; margins: 14 }
                spacing: 10

                RowLayout {
                    width: parent.width

                    BackButton {
                        onClicked: stack.pop()
                        implicitWidth: Math.max(implicitWidth, Theme.minTapTarget)
                    }
                    Item { Layout.fillWidth: true }
                }

                // Search field
                Rectangle {
                    width: parent.width; height: 36; radius: 7
                    color: Theme.surfaceInput
                    border.width: 1
                    border.color: searchField.activeFocus
                        ? Qt.rgba(Theme.accentColor.r, Theme.accentColor.g, Theme.accentColor.b, 0.5)
                        : Theme.surfaceBorder
                    Behavior on border.color { ColorAnimation { duration: 150 } }

                    RowLayout {
                        anchors { fill: parent; leftMargin: 10; rightMargin: 10 }
                        spacing: 8

                        Text {
                            text:           "⌕"
                            font.pixelSize: Theme.fontSizeIcon
                            color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.45)
                        }

                        TextField {
                            id: searchField
                            Layout.fillWidth: true
                            placeholderText:  "Search word or meaning…"
                            color:            Theme.textColor
                            placeholderTextColor: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.4)
                            font.pixelSize:   Theme.fontSizeBody
                            background:       Rectangle { color: "transparent" }
                            leftPadding:      0
                            onTextChanged:    if (srsLibraryVM) srsLibraryVM.setSearch(text)
                        }

                        Rectangle {
                            width: 18; height: 18; radius: 9
                            color:   Theme.surfaceClear
                            visible: searchField.text.length > 0
                            Text {
                                anchors.centerIn: parent
                                text:           "×"
                                font.pixelSize: Theme.fontSizeXSmall
                                color:          Theme.hintColor
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape:  Qt.PointingHandCursor
                                onClicked:    searchField.text = ""
                            }
                        }
                    }
                }

                // Filter pills
                Row {
                    spacing: 6
                    Repeater {
                        model: ["All", "New", "Learning", "Review", "Suspended"]
                        FilterPill {
                            label:  modelData
                            active: filterState.current === modelData
                            onClicked: {
                                filterState.current = modelData
                                if (srsLibraryVM) srsLibraryVM.setFilter(modelData)
                            }
                        }
                    }
                }
            }
        }

        // Filter state holder
        QtObject { id: filterState; property string current: "All" }

        // Count header
        Rectangle {
            Layout.fillWidth: true
            height: 32; color: "transparent"
            RowLayout {
                anchors { fill: parent; leftMargin: 16; rightMargin: 16 }
                Text {
                    font.pixelSize:     Theme.fontSizeXSmall
                    font.weight:        Font.Medium
                    font.letterSpacing: 0.5
                    color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.55)
                    text: cardList.count + (cardList.count === 1 ? " CARD" : " CARDS")
                }
                Item { Layout.fillWidth: true }
            }
            Rectangle {
                anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
                height: 1; color: Theme.dividerColor; opacity: 0.4
            }
        }

        // ── Card list ─────────────────────────────────────────────────────────
        Item {
            Layout.fillWidth:  true
            Layout.fillHeight: true

            ListView {
                id: cardList
                anchors.fill:  parent
                model:         srsLibraryVM
                clip:          true
                spacing:       0
                reuseItems:    false
                cacheBuffer:   400

                delegate: SrsCardDelegate {
                    width:      cardList.width
                    word:       model.word
                    meaning:    model.meaning
                    cardState:  model.state
                    due:        model.due
                    entryId:    model.entryId
                    variants:   model.variants
                    readings:   model.readings

                    highlightEnabled: appConfig.highlightMatches
                    activeQuery:      srsLibraryVM.activeSearch
                    highlightFunc:    function(text) { return srsLibraryVM.highlightField(text) }

                    onOpenDetails: function(id) {
                        stack.push("qrc:/qml/pages/SrsCardDetailPage.qml", { entryId: id })
                    }
                }

                // Fade-out gradient at bottom
                Rectangle {
                    anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
                    height: 40
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "transparent" }
                        GradientStop { position: 1.0; color: Theme.background }
                    }
                    visible: cardList.contentHeight > cardList.height
                    z: 1
                }
            }

            // Empty state
            Column {
                anchors.centerIn: parent
                spacing: 10
                visible: cardList.count === 0

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text:           searchField.text.length > 0 ? "∅" : "○"
                    font.pixelSize: Theme.fontSizeDisplay
                    color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.15)
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text:           searchField.text.length > 0 ? "No cards found" : "No cards in deck"
                    font.pixelSize: Theme.fontSizeBody
                    color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.35)
                }
            }
        }
    }
}

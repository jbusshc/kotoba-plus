import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import Kotoba 1.0

Page {
    id: page
    title: "SRS Library"
    padding: 0

    property bool darkTheme: Theme.darkTheme
    property color textColor: Theme.textColor
    property color hintColor: Theme.hintColor
    property color accentColor: Theme.accentColor
    property color dividerColor: Theme.dividerColor

    background: Rectangle { color: Theme.background }

    Component.onCompleted: {
        if (srsLibraryVM)
            srsLibraryVM.setFilter("All")
    }

    // ── Filter pill component ────────────────────────────────────────────────
    component FilterPill: Rectangle {
        property string label: ""
        property bool   active: false
        signal clicked()

        function pillColor(state) {
            switch (state) {
                case "New":       return "#4A9EFF"
                case "Learning":  return "#FFB83F"
                case "Review":    return "#34D399"
                case "Suspended": return "#9CA3AF"
                default:          return accentColor
            }
        }

        height: 28
        width: pillLabel.implicitWidth + 20
        radius: height / 2

        color: active
            ? Qt.rgba(pillColor(label).r, pillColor(label).g, pillColor(label).b, 0.20)
            : Qt.rgba(1, 1, 1, 0.05)

        border.width: 1
        border.color: active
            ? Qt.rgba(pillColor(label).r, pillColor(label).g, pillColor(label).b, 0.55)
            : Qt.rgba(1, 1, 1, 0.10)

        Behavior on color { ColorAnimation { duration: 130 } }

        Text {
            id: pillLabel
            anchors.centerIn: parent
            text: label
            font.pixelSize: Theme.fontSizeXSmall
            font.weight: Font.Medium
            color: active ? pillColor(label) : Qt.rgba(hintColor.r, hintColor.g, hintColor.b, 0.7)
            Behavior on color { ColorAnimation { duration: 130 } }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Top bar: search + filters ────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            color: Theme.cardBackground
            height: searchCol.implicitHeight + 20

            // Bottom border
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: dividerColor
            }

            Column {
                id: searchCol
                anchors.fill: parent
                anchors.margins: 14
                spacing: 10

                // Back button
                RowLayout {
                    width: parent.width

                    Item {
                        width: backLabel.implicitWidth + 20
                        height: 32

                        Rectangle {
                            anchors.fill: parent
                            radius: 6
                            color: backMouse.containsMouse ? Qt.rgba(1,1,1,0.06) : "transparent"
                            Behavior on color { ColorAnimation { duration: 120 } }
                        }
                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 4
                            Text { text: "‹"; font.pixelSize: Theme.fontSizeMedium; color: hintColor }
                            Text { id: backLabel; text: "Back"; font.pixelSize: Theme.fontSizeSmall; font.weight: Font.Medium; color: hintColor }
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
                }

                // Search row
                Rectangle {
                    width: parent.width
                    height: 36
                    radius: 7
                    color: Qt.rgba(1,1,1,0.05)
                    border.width: 1
                    border.color: searchField.activeFocus
                        ? Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.5)
                        : Qt.rgba(1,1,1,0.08)

                    Behavior on border.color { ColorAnimation { duration: 150 } }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        spacing: 8

                        Text {
                            text: "⌕"
                            font.pixelSize: Theme.fontSizeIcon
                            color: Qt.rgba(hintColor.r, hintColor.g, hintColor.b, 0.45)
                        }

                        TextField {
                            id: searchField
                            Layout.fillWidth: true
                            placeholderText: "Search word or meaning…"
                            color: textColor
                            placeholderTextColor: Qt.rgba(hintColor.r, hintColor.g, hintColor.b, 0.4)
                            font.pixelSize: Theme.fontSizeBody
                            background: Rectangle { color: "transparent" }
                            leftPadding: 0
                            onTextChanged: if (srsLibraryVM) srsLibraryVM.setSearch(text)
                        }

                        Rectangle {
                            width: 18; height: 18; radius: 9
                            color: Qt.rgba(1,1,1,0.12)
                            visible: searchField.text.length > 0

                            Text {
                                anchors.centerIn: parent
                                text: "×"
                                font.pixelSize: Theme.fontSizeXSmall
                                color: hintColor
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: searchField.text = ""
                            }
                        }
                    }
                }

                // Filter pills row
                Row {
                    spacing: 6

                    Repeater {
                        model: ["All", "New", "Learning", "Review", "Suspended"]

                        FilterPill {
                            label: modelData
                            active: filterBox.currentFilter === modelData
                            onClicked: {
                                filterBox.currentFilter = modelData
                                if (srsLibraryVM) srsLibraryVM.setFilter(modelData)
                            }
                        }
                    }
                }
            }
        }

        // Hidden state tracker for filters (replaces ComboBox)
        QtObject {
            id: filterBox
            property string currentFilter: "All"
        }

        // ── Count bar ────────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 32
            color: "transparent"

            // We put this inside a RowLayout
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16

                Text {
                    font.pixelSize: Theme.fontSizeXSmall
                    font.weight: Font.Medium
                    color: Qt.rgba(hintColor.r, hintColor.g, hintColor.b, 0.55)
                    font.letterSpacing: 0.5
                    text: cardList.count + (cardList.count === 1 ? " CARD" : " CARDS")
                }

                Item { Layout.fillWidth: true }
            }

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: dividerColor
                opacity: 0.4
            }
        }

        // ── Card list ────────────────────────────────────────────────────────
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: cardList
                anchors.fill: parent
                model: srsLibraryVM
                clip: true
                spacing: 0
                reuseItems: false
                cacheBuffer: 400

                delegate: SrsCardDelegate {
                    width: cardList.width
                    word:      model.word
                    meaning:   model.meaning
                    cardState: model.state
                    due:       model.due
                    entryId:   model.entryId

                    onOpenDetails: function(id) {
                        stack.push("qrc:/qml/pages/SrsCardDetails.qml", { entryId: id })
                    }
                }

                // Scroll fade at bottom
                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
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
                    text: searchField.text.length > 0 ? "∅" : "○"
                    font.pixelSize: Theme.fontSizeDisplay
                    color: Qt.rgba(hintColor.r, hintColor.g, hintColor.b, 0.15)
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: searchField.text.length > 0 ? "No cards found" : "No cards in deck"
                    font.pixelSize: Theme.fontSizeBody
                    color: Qt.rgba(hintColor.r, hintColor.g, hintColor.b, 0.35)
                }
            }
        }
    }
}

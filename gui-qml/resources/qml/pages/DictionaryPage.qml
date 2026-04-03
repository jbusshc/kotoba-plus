import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Page {
    id: page
    padding: 0

    background: Rectangle { color: Theme.background }

    // ── Helpers ───────────────────────────────────────────────────────────────
    function _dotSep(alpha) {
        const c = Theme.hintColor
        return "<span style=\"color:" + Qt.rgba(c.r, c.g, c.b, alpha) + "\"> ・ </span>"
    }

    function _escape(s) {
        return s.replace(/&/g, "&amp;").replace(/</g, "&lt;")
    }

    function _hl(s, doHl) {
        return (doHl && searchVM) ? searchVM.highlightField(s) : _escape(s)
    }

    // ── Layout ────────────────────────────────────────────────────────────────
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Search bar ────────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 56
            color: Theme.cardBackground

            Rectangle {
                anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
                height: 1; color: Theme.dividerColor
            }

            RowLayout {
                anchors { fill: parent; leftMargin: 16; rightMargin: 16 }
                spacing: 10

                Text {
                    text:           "⌕"
                    font.pixelSize: Theme.fontSizeLarge
                    color: searchField.activeFocus
                        ? Theme.accentColor
                        : Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.5)
                    Behavior on color { ColorAnimation { duration: 150 } }
                }

                TextField {
                    id: searchField
                    Layout.fillWidth: true
                    placeholderText:  "Search in Japanese, kana, or English…"
                    text:             searchVM ? searchVM.query : ""
                    color:            Theme.textColor
                    placeholderTextColor: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.45)
                    font.pixelSize:   Theme.fontSizeBase
                    background:       Rectangle { color: "transparent" }
                    leftPadding:      0
                    onTextChanged:    if (searchVM) searchVM.query = text
                }

                Rectangle {
                    width: 20; height: 20; radius: 10
                    color:   Theme.surfaceClear
                    visible: searchField.text.length > 0

                    Text {
                        anchors.centerIn: parent
                        text:           "×"
                        font.pixelSize: Theme.fontSizeSmall
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

        // ── Results list ──────────────────────────────────────────────────────
        ListView {
            id: listView
            model:             searchModel
            Layout.fillWidth:  true
            Layout.fillHeight: true
            clip:           true
            boundsBehavior: Flickable.StopAtBounds
            spacing:        0

            delegate: Item {
                id: del
                width: listView.width

                readonly property bool hasSubline: variants.length > 0 || readings.length > 0
                height: hasSubline ? 72 : 56

                readonly property bool doHl: appConfig.highlightMatches
                                          && searchVM
                                          && searchVM.activeQuery.length > 0

                property bool hovered: false
                property bool pressed: false

                // ── Background ────────────────────────────────────────────
                Rectangle {
                    anchors.fill: parent
                    color: del.pressed
                        ? Qt.rgba(Theme.accentColor.r, Theme.accentColor.g, Theme.accentColor.b, 0.10)
                        : del.hovered ? Theme.surfaceHover : "transparent"
                    Behavior on color { ColorAnimation { duration: 100 } }
                }

                // ── Content ───────────────────────────────────────────────
                RowLayout {
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter
                              leftMargin: 16; rightMargin: 16 }
                    spacing: 12

                    Rectangle {
                        width:  3
                        height: contentCol.implicitHeight
                        radius: 2
                        color:  Theme.accentColor
                        opacity: del.hovered ? 1 : 0
                        Behavior on opacity { NumberAnimation { duration: 120 } }
                    }

                    Column {
                        id: contentCol
                        Layout.fillWidth: true
                        spacing: 2

                        Text {
                            width:            parent.width
                            textFormat:       Text.RichText
                            font.pixelSize:   Theme.fontSizeItem
                            font.weight:      Font.Medium
                            color:            Theme.textColor
                            elide:            Text.ElideRight
                            maximumLineCount: 1
                            text: {
                                const hw = page._hl(headword, del.doHl)
                                if (variants.length === 0) return hw
                                const sep    = page._dotSep(0.45)
                                const vParts = variants.split("・").map(v => page._hl(v, del.doHl))
                                return hw + sep + vParts.join(sep)
                            }
                        }

                        Text {
                            visible:          readings.length > 0
                            width:            parent.width
                            textFormat:       Text.RichText
                            font.pixelSize:   Theme.fontSizeSmall
                            color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.6)
                            elide:            Text.ElideRight
                            maximumLineCount: 1
                            text: {
                                if (!readings || readings.length === 0) return ""
                                const sep   = page._dotSep(0.35)
                                const parts = readings.split("・").map(r => page._hl(r, del.doHl))
                                return parts.join(sep)
                            }
                        }

                        Text {
                            width:            parent.width
                            textFormat:       Text.RichText
                            font.pixelSize:   Theme.fontSizeSmall
                            color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.8)
                            elide:            Text.ElideRight
                            maximumLineCount: 1
                            text: del.doHl ? page._hl(gloss, true) : gloss
                        }
                    }

                    Text {
                        text:           "›"
                        font.pixelSize: Theme.fontSizeMedium
                        color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.35)
                        opacity: del.hovered ? 1 : 0
                        Behavior on opacity { NumberAnimation { duration: 120 } }
                    }
                }

                Rectangle {
                    anchors { bottom: parent.bottom; left: parent.left; right: parent.right; leftMargin: 16 }
                    height: 1; color: Theme.dividerColor; opacity: 0.5
                }

                // ── Single MouseArea — no competing TapHandlers ────────────
                MouseArea {
                    anchors.fill:  parent
                    hoverEnabled:  true
                    cursorShape:   Qt.PointingHandCursor
                    onEntered:     del.hovered = true
                    onExited:      del.hovered = false
                    onPressed:     del.pressed = true
                    onReleased:    del.pressed = false
                    onClicked:     stack.push("qrc:/qml/pages/DetailsPage.qml", { docId: docId })
                }
            }

            // ── Pagination ────────────────────────────────────────────────
            onContentYChanged: {
                if (contentY + height >= contentHeight - 150)
                    searchVM.needMore()
            }

            // ── Empty state ───────────────────────────────────────────────
            Item {
                anchors.centerIn: parent
                visible: listView.count === 0 && searchField.text.length > 0
                width:   parent.width

                Column {
                    anchors.centerIn: parent
                    spacing: 8
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "∅"; font.pixelSize: Theme.fontSizeBack
                        color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.25)
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "No results found"; font.pixelSize: Theme.fontSizeBody
                        color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.4)
                    }
                }
            }

            // ── Prompt state ──────────────────────────────────────────────
            Item {
                anchors.centerIn: parent
                visible: listView.count === 0 && searchField.text.length === 0
                width:   parent.width

                Column {
                    anchors.centerIn: parent
                    spacing: 8
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "日"; font.pixelSize: Theme.fontSizeGlyph
                        color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.10)
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "Type to search"; font.pixelSize: Theme.fontSizeBody
                        color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.3)
                    }
                }
            }
        }
    }
}
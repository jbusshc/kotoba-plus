import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../theme"

Page {
    id: page
    padding: 0

    property color textColor:    Theme.textColor
    property color hintColor:    Theme.hintColor
    property color accentColor:  Theme.accentColor
    property color dividerColor: Theme.dividerColor

    background: Rectangle { color: Theme.background }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Search bar ───────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 56
            color: Theme.cardBackground

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width; height: 1
                color: dividerColor
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16; anchors.rightMargin: 16
                spacing: 10

                Text {
                    text: "⌕"
                    font.pixelSize: Theme.fontSizeLarge
                    color: searchField.activeFocus
                        ? accentColor
                        : Qt.rgba(hintColor.r, hintColor.g, hintColor.b, 0.5)
                    Behavior on color { ColorAnimation { duration: 150 } }
                }

                TextField {
                    id: searchField
                    Layout.fillWidth: true
                    placeholderText: "Search in Japanese, kana, or English…"
                    text: searchVM ? searchVM.query : ""
                    color: textColor
                    placeholderTextColor: Qt.rgba(hintColor.r, hintColor.g, hintColor.b, 0.45)
                    font.pixelSize: Theme.fontSizeBase
                    background: Rectangle { color: "transparent" }
                    leftPadding: 0
                    onTextChanged: if (searchVM) searchVM.query = text
                }

                Rectangle {
                    width: 20; height: 20; radius: 10
                    color: Theme.surfaceClear
                    visible: searchField.text.length > 0

                    Text {
                        anchors.centerIn: parent
                        text: "×"
                        font.pixelSize: Theme.fontSizeSmall
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

        // ── Results list ─────────────────────────────────────────────────────
        ListView {
            id: listView
            model: searchModel
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            spacing: 0

            delegate: Item {
                id: delegateRoot
                width: listView.width

                readonly property bool hasSubline: variants.length > 0 || readings.length > 0
                height: hasSubline ? 72 : 56

                property bool hovered: false
                property bool pressed: false

                readonly property string aq: searchVM ? searchVM.activeQuery : ""
                readonly property bool doHighlight: appConfig.highlightMatches && aq.length > 0

                Rectangle {
                    anchors.fill: parent
                    color: delegateRoot.pressed
                        ? Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.10)
                        : delegateRoot.hovered
                            ? Theme.surfaceHover
                            : "transparent"
                    Behavior on color { ColorAnimation { duration: 100 } }
                }

                RowLayout {
                    anchors.left: parent.left; anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 16; anchors.rightMargin: 16
                    spacing: 12

                    Rectangle {
                        width: 3
                        height: contentCol.implicitHeight
                        radius: 2
                        color: accentColor
                        opacity: delegateRoot.hovered ? 1 : 0
                        Behavior on opacity { NumberAnimation { duration: 120 } }
                    }

                    Column {
                        id: contentCol
                        Layout.fillWidth: true
                        spacing: 2

                        // Fila 1: headword + variants kanji
                        Text {
                            id: wordText
                            width: parent.width
                            textFormat: Text.RichText
                            text: {
                                const hw = delegateRoot.doHighlight
                                    ? searchVM.highlightField(headword)
                                    : headword

                                if (variants.length === 0) return hw

                                const sep = "<span style=\"color:" + Qt.rgba(hintColor.r, hintColor.g, hintColor.b, 0.45) + "\"> ・ </span>"
                                const vParts = variants.split("・").map(v => {
                                    return delegateRoot.doHighlight
                                        ? searchVM.highlightField(v)
                                        : v.replace(/&/g,"&amp;").replace(/</g,"&lt;")
                                })
                                return hw + sep + vParts.join(sep)
                            }
                            font.pixelSize: Theme.fontSizeItem
                            font.weight: Font.Medium
                            color: textColor
                            elide: Text.ElideRight
                            maximumLineCount: 1
                        }

                        // Fila 2: readings
                        Text {
                            id: readingsText
                            visible: readings.length > 0
                            width: parent.width
                            textFormat: Text.RichText
                            text: {
                                if (!readings || readings.length === 0) return ""
                                const parts = readings.split("・").map(r => {
                                    return delegateRoot.doHighlight
                                        ? searchVM.highlightField(r)
                                        : r.replace(/&/g,"&amp;").replace(/</g,"&lt;")
                                })
                                return parts.join("<span style=\"color:" + Qt.rgba(hintColor.r, hintColor.g, hintColor.b, 0.35) + "\"> ・ </span>")
                            }
                            font.pixelSize: Theme.fontSizeSmall
                            color: Qt.rgba(hintColor.r, hintColor.g, hintColor.b, 0.6)
                            elide: Text.ElideRight
                            maximumLineCount: 1
                        }

                        // Fila 3: gloss
                        Text {
                            id: glossText
                            width: parent.width
                            textFormat: Text.RichText
                            text: delegateRoot.doHighlight
                                ? searchVM.highlightField(gloss)
                                : gloss
                            font.pixelSize: Theme.fontSizeSmall
                            color: Qt.rgba(hintColor.r, hintColor.g, hintColor.b, 0.8)
                            elide: Text.ElideRight
                            maximumLineCount: 1
                        }
                    }

                    Text {
                        text: "›"
                        font.pixelSize: Theme.fontSizeMedium
                        color: Qt.rgba(hintColor.r, hintColor.g, hintColor.b, 0.35)
                        opacity: delegateRoot.hovered ? 1 : 0
                        Behavior on opacity { NumberAnimation { duration: 120 } }
                    }
                }

                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left; anchors.right: parent.right
                    anchors.leftMargin: 16
                    height: 1
                    color: dividerColor; opacity: 0.5
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onEntered:  delegateRoot.hovered = true
                    onExited:   delegateRoot.hovered = false
                    onPressed:  delegateRoot.pressed = true
                    onReleased: delegateRoot.pressed = false
                    onClicked:  stack.push("qrc:/qml/pages/DetailsPage.qml", { docId: docId })
                }
            }

            onContentYChanged: {
                if (contentY + height >= contentHeight - 150)
                    searchVM.needMore()
            }

            // Empty state
            Item {
                anchors.centerIn: parent
                visible: listView.count === 0 && searchField.text.length > 0
                width: parent.width

                Column {
                    anchors.centerIn: parent
                    spacing: 8
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "∅"
                        font.pixelSize: Theme.fontSizeBack
                        color: Qt.rgba(hintColor.r, hintColor.g, hintColor.b, 0.25)
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "No results found"
                        font.pixelSize: Theme.fontSizeBody
                        color: Qt.rgba(hintColor.r, hintColor.g, hintColor.b, 0.4)
                    }
                }
            }

            // Prompt state
            Item {
                anchors.centerIn: parent
                visible: listView.count === 0 && searchField.text.length === 0
                width: parent.width

                Column {
                    anchors.centerIn: parent
                    spacing: 8
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "日"
                        font.pixelSize: Theme.fontSizeGlyph
                        color: Qt.rgba(hintColor.r, hintColor.g, hintColor.b, 0.10)
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "Type to search"
                        font.pixelSize: Theme.fontSizeBody
                        color: Qt.rgba(hintColor.r, hintColor.g, hintColor.b, 0.3)
                    }
                }
            }
        }
    }
}
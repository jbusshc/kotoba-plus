import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Item {
    id: root

    property string word
    property string meaning
    property string cardState
    property string due
    property int    entryId
    property string variants: ""
    property string readings: ""

    property string activeQuery: ""
    property bool highlightEnabled: false
    property var highlightFunc

    readonly property bool doHighlight: highlightEnabled && activeQuery.length > 0

    function escapeHtml(s) {
        return s.replace(/&/g,"&amp;").replace(/</g,"&lt;")
    }

    function doHighlightText(s) {
        return highlightFunc ? highlightFunc(s) : escapeHtml(s)
    }

    signal openDetails(int entryId)

    readonly property bool hasReadings: readings.length > 0

    height: hasReadings ? 72 : 56

    property bool hovered: false
    property bool pressed: false

    Rectangle {
        anchors.fill: parent
        color: root.pressed
            ? Qt.rgba(Theme.accentColor.r, Theme.accentColor.g, Theme.accentColor.b, 0.10)
            : root.hovered ? Theme.surfaceHover : "transparent"
        Behavior on color { ColorAnimation { duration: 100 } }
    }

    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 16
        height: 1
        color: Theme.dividerColor
        opacity: 0.5
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 12
        spacing: 12

        Rectangle {
            width: 3
            Layout.fillHeight: true
            Layout.topMargin: 8
            Layout.bottomMargin: 8
            radius: 2
            color: Theme.accentColor
            opacity: root.hovered ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 120 } }
        }

        Column {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 2

            Text {
                width: parent.width
                textFormat: Text.RichText
                text: {
                    const hw = root.doHighlight
                        ? doHighlightText(word)
                        : escapeHtml(word)

                    if (variants.length === 0)
                        return hw

                    const dimColor = Qt.rgba(
                        Theme.hintColor.r,
                        Theme.hintColor.g,
                        Theme.hintColor.b,
                        0.45
                    )

                    const sep = "<span style=\"color:" + dimColor + "\"> ・ </span>"

                    const vParts = variants.split("・").map(v =>
                        root.doHighlight ? doHighlightText(v) : escapeHtml(v)
                    )

                    return hw + sep + vParts.join(sep)
                }
                font.pixelSize: Theme.fontSizeItem
                font.weight: Font.Medium
                color: Theme.textColor
                elide: Text.ElideRight
                maximumLineCount: 1
            }

            Text {
                visible: root.hasReadings
                width: parent.width
                textFormat: Text.RichText
                text: {
                    if (!root.hasReadings)
                        return ""

                    const dimColor = Qt.rgba(
                        Theme.hintColor.r,
                        Theme.hintColor.g,
                        Theme.hintColor.b,
                        0.35
                    )

                    const sep = "<span style=\"color:" + dimColor + "\"> ・ </span>"

                    const parts = readings.split("・").map(r =>
                        root.doHighlight ? doHighlightText(r) : escapeHtml(r)
                    )

                    return parts.join(sep)
                }
                font.pixelSize: Theme.fontSizeXSmall
                color: Qt.rgba(
                    Theme.hintColor.r,
                    Theme.hintColor.g,
                    Theme.hintColor.b,
                    0.6
                )
                elide: Text.ElideRight
                maximumLineCount: 1
            }

            Text {
                width: parent.width
                textFormat: Text.RichText
                text: root.doHighlight
                    ? doHighlightText(meaning)
                    : escapeHtml(meaning)

                font.pixelSize: Theme.fontSizeSmall
                color: Qt.rgba(
                    Theme.hintColor.r,
                    Theme.hintColor.g,
                    Theme.hintColor.b,
                    0.8
                )
                elide: Text.ElideRight
                maximumLineCount: 1
            }
        }

        Rectangle {
            Layout.alignment: Qt.AlignVCenter
            width: 76
            height: 22
            radius: 4
            color: {
                switch (cardState) {
                case "New":        return Qt.rgba(0.29, 0.62, 1.0,  0.15)
                case "Learning":
                case "Relearning": return Qt.rgba(1.0,  0.72, 0.25, 0.15)
                case "Review":     return Qt.rgba(0.20, 0.83, 0.60, 0.15)
                case "Suspended":  return Qt.rgba(0.61, 0.64, 0.69, 0.15)
                default:           return "transparent"
                }
            }

            Text {
                anchors.centerIn: parent
                text: cardState === "Relearning" ? "Relearn" : cardState
                font.pixelSize: Theme.fontSizeXSmall
                font.weight: Font.Medium
                color: {
                    switch (cardState) {
                    case "New":        return "#4A9EFF"
                    case "Learning":
                    case "Relearning": return "#FFB83F"
                    case "Review":     return "#34D399"
                    case "Suspended":  return "#9CA3AF"
                    default:           return Theme.hintColor
                    }
                }
            }
        }

        Rectangle {
            Layout.alignment: Qt.AlignVCenter
            width: 58
            height: 28
            radius: 6
            color: detailsMa.pressed
                ? Theme.surfacePress
                : detailsMa.containsMouse
                    ? Theme.surfaceHover
                    : Theme.surfaceSubtle

            border.width: 1
            border.color: Theme.surfaceBorder

            Behavior on color { ColorAnimation { duration: 80 } }

            Text {
                anchors.centerIn: parent
                text: "Details"
                font.pixelSize: Theme.fontSizeXSmall
                font.weight: Font.Medium
                color: Qt.rgba(
                    Theme.hintColor.r,
                    Theme.hintColor.g,
                    Theme.hintColor.b,
                    0.8
                )
            }

            MouseArea {
                id: detailsMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.openDetails(root.entryId)
            }
        }

        Text {
            text: "›"
            font.pixelSize: Theme.fontSizeMedium
            color: Qt.rgba(
                Theme.hintColor.r,
                Theme.hintColor.g,
                Theme.hintColor.b,
                0.35
            )
            opacity: root.hovered ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 120 } }
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor

        onEntered:  root.hovered = true
        onExited:   root.hovered = false
        onPressed:  root.pressed = true
        onReleased: root.pressed = false
        onClicked:  root.openDetails(root.entryId)
    }
}
// StatCard.qml
// Shared stat card used by SrsDashboard (heroStyle:true) and detail pages.
//
// Dashboard usage:
//   StatCard { heroStyle: true; label: "Due"; value: "3"; accent: "#F87171" }
//
// Detail page usage:
//   StatCard { label: "Due"; value: statDue; icon: "⏰"; accent: accentColor }
import QtQuick
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: root

    property string label:       ""
    property string value:       "—"
    property string icon:        ""
    property color  accent:      Theme.accentColor
    property bool   heroStyle:   false

    // heroStyle dims these automatically; only needed for compact mode.
    // Left here for backward compat but they now default to Theme values.
    property color bg:          Theme.cardBackground
    property color borderColor: Theme.dividerColor
    property color hint:        Theme.hintColor
    property color fg:          Theme.textColor

    height: heroStyle ? 110 : 72
    radius: 8
    color:  bg
    border.width: 1
    border.color: heroStyle
        ? Qt.rgba(accent.r, accent.g, accent.b, 0.25)
        : borderColor

    // Top accent line (hero style only)
    Rectangle {
        visible:        root.heroStyle
        anchors.top:    parent.top
        anchors.left:   parent.left
        anchors.right:  parent.right
        height: 2; radius: 1
        color:   root.accent
        opacity: 0.75
    }

    ColumnLayout {
        anchors.fill:    parent
        anchors.margins: root.heroStyle ? 16 : 10
        spacing:         root.heroStyle ? 6  : 2

        // Icon + label row (compact style only)
        RowLayout {
            visible: !root.heroStyle
            spacing: 4

            Text {
                visible:        root.icon !== ""
                text:           root.icon
                font.pixelSize: Theme.fontSizeBody
                color:          root.fg
            }
            Text {
                text:            root.label
                color:           root.hint
                font.pixelSize:  Theme.fontSizeXSmall
                font.weight:     Font.Medium
                font.letterSpacing: 0.8
            }
        }

        // Value
        Text {
            text:            root.value
            color:           root.accent
            font.pixelSize:  root.heroStyle ? Theme.fontSizeHero : Theme.fontSizeLarge
            font.weight:     Font.Bold
            font.letterSpacing: root.heroStyle ? -1 : 0
            elide:           Text.ElideRight
            Layout.fillWidth: true
        }

        // Label (hero style — shown below value)
        Text {
            visible:            root.heroStyle
            text:               root.label.toUpperCase()
            font.pixelSize:     Theme.fontSizeSmall
            font.weight:        Font.Medium
            font.letterSpacing: 0.6
            color: Qt.rgba(root.hint.r, root.hint.g, root.hint.b, 0.7)
        }
    }
}

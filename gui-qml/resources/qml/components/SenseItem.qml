import QtQuick
import QtQuick.Controls

Column {
    id: root

    property var sense
    property int senseIndex: 0
    property string mode: "dictionary"

    property color textColor
    property color hintColor
    property color accentColor
    property color dividerColor

    width: parent ? parent.width : 400
    spacing: 6

    Row {
        spacing: 8

        Text {
            text: (root.senseIndex + 1) + "."
            font.bold: true
            font.pixelSize: 14
            color: root.textColor
        }

        Text {
            visible: (root.sense.pos || []).length > 0
            text: "(" + (root.sense.pos || []).join(", ") + ")"
            font.pixelSize: 13
            color: root.accentColor
        }

        Text {
            visible: root.mode === "dictionary" &&
                     (root.sense.field || []).length > 0
            text: "[" + (root.sense.field || []).join(", ") + "]"
            font.pixelSize: 12
            color: root.hintColor
        }
    }

    Text {
        visible: root.mode === "dictionary" &&
                 (root.sense.stagk || []).length > 0
        text: "Only for kanji: " + (root.sense.stagk || []).join(", ")
        font.pixelSize: 12
        color: root.hintColor
    }

    Text {
        visible: root.mode === "dictionary" &&
                 (root.sense.stagr || []).length > 0
        text: "Only for reading: " + (root.sense.stagr || []).join(", ")
        font.pixelSize: 12
        color: root.hintColor
    }

    Repeater {
        model: root.sense.gloss || []

        delegate: Text {
            width: parent.width
            wrapMode: Text.WordWrap
            text: "• " + modelData
            font.pixelSize: 15
            color: root.textColor
        }
    }

    Column {
        visible: root.mode === "dictionary"
        spacing: 2

        Text {
            visible: (root.sense.misc || []).length > 0
            text: (root.sense.misc || []).join(", ")
            font.pixelSize: 12
            color: root.hintColor
        }

        Text {
            visible: (root.sense.dial || []).length > 0
            text: "(" + (root.sense.dial || []).join(", ") + " dialect)"
            font.pixelSize: 12
            color: root.hintColor
        }

        Text {
            visible: (root.sense.s_inf || []).length > 0
            text: (root.sense.s_inf || []).join("; ")
            font.pixelSize: 12
            font.italic: true
            color: root.hintColor
        }

        Text {
            visible: (root.sense.lsource || []).length > 0
            text: "From: " + (root.sense.lsource || []).join(", ")
            font.pixelSize: 12
            font.italic: true
            color: root.hintColor
        }
    }

    Rectangle {
        width: parent.width
        height: 1
        color: root.dividerColor
        opacity: 0.5
    }
}
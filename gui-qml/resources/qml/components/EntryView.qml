import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
Column {
    id: root

    property var    entryData
    property string mode:     "dictionary"
    property bool   revealed: true          

    property color textColor
    property color hintColor
    property color accentColor
    property color dividerColor

    width: parent ? parent.width : 400
    spacing: 18

    // ── DATA NORMALIZADA ─────────────────────────────────

    property var kanjiList: entryData.k_elements || []
    property var readingList: entryData.r_elements || []
    property bool hasKanji: kanjiList.length > 0

    property string headword:
        hasKanji
        ? (kanjiList[0] ? kanjiList[0].keb : "")
        : (readingList.length > 0 ? readingList[0].reb : "")

    // ── HEADWORD ─────────────────────────────────────────

    Text {
        width: parent.width
        text: root.headword

        font.pixelSize: 48
        font.bold: true
        color: root.textColor
        horizontalAlignment: Text.AlignHCenter
    }

    // ── VARIANTES KANJI ─────────────────────────────────

    Flow {
        visible: (root.mode !== "srs" || root.revealed)
                 && root.hasKanji
                 && root.kanjiList.length > 1

        width: parent.width
        spacing: 8

        opacity: root.revealed ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { duration: 180 } }

        Repeater {
            model: root.kanjiList.slice(1)
            delegate: Text {
                text: modelData.keb
                font.pixelSize: 18
                font.weight: Font.Medium
                color: Qt.rgba(root.hintColor.r, root.hintColor.g, root.hintColor.b, 0.7)
            }
        }
    }

    // ── LECTURAS ────────────────────────────────────────

    Column {
        visible: (root.mode !== "srs" || root.revealed)
                 && root.readingList.length > 0

        spacing: 4

        opacity: root.revealed ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { duration: 180 } }

        Text {
            text: "Reading"
            font.pixelSize: 12
            font.italic: true
            color: root.hintColor
            opacity: 0.7
        }

        Flow {
            width: parent.width
            spacing: 6

            Repeater {
                model: root.hasKanji
                       ? root.readingList
                       : root.readingList.slice(1)

                delegate: Text {
                    text: modelData.reb
                    font.pixelSize: 16
                    font.italic: true
                    color: root.hintColor
                }
            }
        }
    }

    // ── DIVIDER ─────────────────────────────────────────

    Rectangle {
        width: parent.width
        height: 1
        color: root.dividerColor

        opacity: root.revealed ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { duration: 180 } }
    }

    // ── SENSES ───────────────────────────────────────────

    Repeater {
        model: root.entryData.senses || []

        delegate: SenseItem {
            sense:      modelData
            senseIndex: index
            mode:       root.mode
            revealed:   root.revealed

            visible: root.revealed

            textColor:    root.textColor
            hintColor:    root.hintColor
            accentColor:  root.accentColor
            dividerColor: root.dividerColor
        }
    }
}
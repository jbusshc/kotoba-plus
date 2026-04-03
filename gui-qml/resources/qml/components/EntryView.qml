import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../theme"

Column {
    id: root

    property var    entryData
    property string mode:     "dictionary"
    property bool   revealed: true

    // Bubble navigation requests up — parent connects to StackView.push
    signal navigateTo(string page, var props)

    width:   parent ? parent.width : 400
    spacing: 18

    // ── Normalised data ───────────────────────────────────────────────────────
    property var  kanjiList:   entryData.k_elements || []
    property var  readingList: entryData.r_elements || []
    property bool hasKanji:    kanjiList.length > 0

    property string headword:
        hasKanji
        ? (kanjiList[0] ? kanjiList[0].keb : "")
        : (readingList.length > 0 ? readingList[0].reb : "")

    // ── Headword ──────────────────────────────────────────────────────────────
    Text {
        width:               parent.width
        text:                root.headword
        font.pixelSize:      48
        font.bold:           true
        color:               Theme.textColor
        horizontalAlignment: Text.AlignHCenter
    }

    // ── Kanji variants ────────────────────────────────────────────────────────
    Flow {
        visible: (root.mode !== "srs" || root.revealed)
                 && root.hasKanji
                 && root.kanjiList.length > 1

        width:   parent.width
        spacing: 8

        opacity: root.revealed ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { duration: 180 } }

        Repeater {
            model: root.kanjiList.slice(1)
            delegate: Text {
                text:           modelData.keb
                font.pixelSize: 18
                font.weight:    Font.Medium
                color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.7)
            }
        }
    }

    // ── Readings ──────────────────────────────────────────────────────────────
    Column {
        visible: (root.mode !== "srs" || root.revealed)
                 && root.readingList.length > 0

        spacing: 4

        opacity: root.revealed ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { duration: 180 } }

        Text {
            text:           "Reading"
            font.pixelSize: 12
            font.italic:    true
            color:          Theme.hintColor
            opacity:        0.7
        }

        Flow {
            width:   parent.width
            spacing: 6

            Repeater {
                model: root.hasKanji
                       ? root.readingList
                       : root.readingList.slice(1)

                delegate: Text {
                    text:           modelData.reb
                    font.pixelSize: 16
                    font.italic:    true
                    color:          Theme.hintColor
                }
            }
        }
    }

    // ── Divider ───────────────────────────────────────────────────────────────
    Rectangle {
        width:  parent.width
        height: 1
        color:  Theme.dividerColor

        opacity: root.revealed ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { duration: 180 } }
    }

    // ── Senses ────────────────────────────────────────────────────────────────
    Repeater {
        model: root.entryData.senses || []

        delegate: SenseItem {
            sense:       modelData
            senseIndex:  index
            mode:        root.mode
            revealed:    root.revealed
            visible:     root.revealed

            onNavigateTo: (page, props) => root.navigateTo(page, props)
        }
    }
}

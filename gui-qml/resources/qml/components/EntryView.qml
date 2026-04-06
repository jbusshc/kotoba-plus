import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import "../theme"

Column {
    id: root

    property var    entryData:  ({})
    property string mode:       "dictionary"   // "dictionary" | "srs_recognition" | "srs" (legacy) | "srs_recall"
    property bool   revealed:   true

    signal navigateTo(string page, var props)

    width:   parent ? parent.width : 400
    spacing: 18

    // ── Datos normalizados — bindings reactivos sobre entryData ───────────────
    property var  kanjiList:   entryData.k_elements  || []
    property var  readingList: entryData.r_elements  || []
    property var  senseList:   entryData.senses      || []
    property bool hasKanji:    kanjiList.length > 0

    property string headword:
        hasKanji
            ? (kanjiList[0] ? kanjiList[0].keb : "")
            : (readingList.length > 0 ? readingList[0].reb : "")

    // Glosa del primer sense — binding reactivo directo
    property string firstGloss:
        senseList.length > 0 && (senseList[0].gloss || []).length > 0
            ? (senseList[0].gloss || []).join("; ")
            : ""

    // Todas las glosas de todos los senses — binding reactivo directo
    property string allGloss: {
        const parts = []
        for (var i = 0; i < senseList.length; i++) {
            const g = senseList[i].gloss || []
            if (g.length > 0) parts.push(g.join("; "))
        }
        return parts.join(" / ")
    }

    // ── Mode helpers ──────────────────────────────────────────────────────────
    readonly property bool isRecall:      mode === "srs_recall"
    readonly property bool isRecognition: mode === "srs_recognition" || mode === "srs"
    readonly property bool isSrs:         isRecall || isRecognition
    readonly property bool isDictionary:  !isSrs

    // ══════════════════════════════════════════════════════════════════════════
    // VISTA RECALL
    // Front (revealed=false): glosas visibles, kanji/lecturas ocultos
    // Back  (revealed=true):  glosas + kanji + lecturas + senses completos
    // ══════════════════════════════════════════════════════════════════════════

    // ── [Recall] Glosas — visibles solo en el front (ocultas al revelar) ────────
    Text {
        visible:             root.isRecall && !root.revealed
        width:               parent.width
        text:                root.allGloss
        wrapMode:            Text.Wrap
        font.pixelSize:      22
        font.weight:         Font.Medium
        color:               Theme.textColor
        horizontalAlignment: Text.AlignHCenter
    }

    // ── [Recall] Separador ────────────────────────────────────────────────────
    Rectangle {
        visible: root.isRecall && !root.revealed
        width:   parent.width
        height:  1
        color:   Theme.dividerColor
        opacity: 0.4
    }

    // ── [Recall] Hint antes de revelar ────────────────────────────────────────
    Text {
        visible:             root.isRecall && !root.revealed
        width:               parent.width
        text:                "↑ Recall the Japanese word"
        font.pixelSize:      Theme.fontSizeXSmall
        color:               Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.35)
        horizontalAlignment: Text.AlignHCenter
    }

    // ── [Recall] Respuesta revelada: kanji + lecturas ─────────────────────────
    Column {
        visible: root.isRecall && root.revealed
        width:   parent.width
        spacing: 10

        Text {
            visible:             root.hasKanji
            width:               parent.width
            text:                root.headword
            font.pixelSize:      48
            font.bold:           true
            color:               Theme.textColor
            horizontalAlignment: Text.AlignHCenter
        }

        Flow {
            visible:  root.hasKanji && root.kanjiList.length > 1
            width:    parent.width
            spacing:  8
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

        Column {
            visible:  root.readingList.length > 0
            width:    parent.width
            spacing:  4

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
                    model: root.hasKanji ? root.readingList : root.readingList.slice(1)
                    delegate: Text {
                        text:           modelData.reb
                        font.pixelSize: 16
                        font.italic:    true
                        color:          Theme.hintColor
                    }
                }
            }
        }
    }

    // ── [Recall] Senses completos al revelar (para comparar) ──────────────────
    Repeater {
        model: root.isRecall && root.revealed ? root.senseList : []
        delegate: SenseItem {
            sense:       modelData
            senseIndex:  index
            mode:        root.mode
            revealed:    true
            onNavigateTo: (page, props) => root.navigateTo(page, props)
        }
    }

    // ══════════════════════════════════════════════════════════════════════════
    // VISTA RECOGNITION / DICTIONARY
    // ══════════════════════════════════════════════════════════════════════════

    // ── [Recog/Dict] Headword ─────────────────────────────────────────────────
    Text {
        visible:             !root.isRecall
        width:               parent.width
        text:                root.headword
        font.pixelSize:      48
        font.bold:           true
        color:               Theme.textColor
        horizontalAlignment: Text.AlignHCenter
    }

    // ── [Recog/Dict] Kanji variantes ──────────────────────────────────────────
    Flow {
        visible: !root.isRecall && root.hasKanji && root.kanjiList.length > 1
        width:   parent.width
        spacing: 8

        opacity: (root.isDictionary || root.revealed) ? 1.0 : 0.0
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

    // ── [Recog/Dict] Lecturas ─────────────────────────────────────────────────
    Column {
        visible: !root.isRecall && root.readingList.length > 0
        spacing: 4

        opacity: (root.isDictionary || root.revealed) ? 1.0 : 0.0
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
                model: root.hasKanji ? root.readingList : root.readingList.slice(1)
                delegate: Text {
                    text:           modelData.reb
                    font.pixelSize: 16
                    font.italic:    true
                    color:          Theme.hintColor
                }
            }
        }
    }

    // ── [Recog/Dict] Divider ──────────────────────────────────────────────────
    Rectangle {
        visible: !root.isRecall
        width:   parent.width
        height:  1
        color:   Theme.dividerColor

        opacity: (root.isDictionary || root.revealed) ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { duration: 180 } }
    }

    // ── [Recog/Dict] Senses ───────────────────────────────────────────────────
    Repeater {
        model: !root.isRecall ? root.senseList : []
        delegate: SenseItem {
            sense:       modelData
            senseIndex:  index
            mode:        root.mode
            revealed:    root.revealed
            visible:     root.isDictionary || root.revealed
            onNavigateTo: (page, props) => root.navigateTo(page, props)
        }
    }
}
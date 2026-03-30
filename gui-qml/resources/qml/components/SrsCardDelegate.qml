import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: root

    property string word
    property string meaning
    property string cardState
    property string due
    property int    entryId

    // Variants kanji y readings vienen del modelo de SrsLibrary como strings "a・b・c".
    // El delegate los recibe opcionalmente — si no se asignan quedan vacíos.
    property string variants: ""
    property string readings: ""

    signal openDetails(int entryId)

    // Altura fija según si hay sublinea (readings), para evitar inflado al cambiar
    // el formato de texto (el mismo problema que en DictionaryPage).
    readonly property bool hasSubline: readings.length > 0
    height: hasSubline ? 72 : 56

    color: "transparent"   // fondo lo maneja el ListView o el highlight de hover

    // ── Separador inferior ────────────────────────────────────────────────────
    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left; anchors.right: parent.right
        anchors.leftMargin: 12
        height: 1
        color: Theme.dividerColor
        opacity: 0.4
    }

    // ── Layout principal ──────────────────────────────────────────────────────
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 14; anchors.rightMargin: 8
        anchors.topMargin: 6;   anchors.bottomMargin: 6
        spacing: 10

        // ── Bloque izquierdo: word + readings ─────────────────────────────────
        Column {
            Layout.preferredWidth: 130
            Layout.minimumWidth:   90
            spacing: 2

            // Headword + variants kanji (misma línea, elided)
            Text {
                width: parent.width
                textFormat: Text.RichText
                text: {
                    // Guard: srsLibraryVM puede ser null al inicio
                    const q = srsLibraryVM ? (typeof srsLibraryVM.activeSearch !== "undefined"
                                              ? srsLibraryVM.activeSearch : "") : ""
                    const doHL = appConfig.highlightMatches && q.length > 0

                    // Para SrsLibrary no tenemos highlightField en el VM todavía;
                    // hacemos el match básico en JS mientras tanto.
                    function hl(str) {
                        if (!doHL || str.length === 0) return str.replace(/&/g,"&amp;").replace(/</g,"&lt;")
                        const idx = str.toLowerCase().indexOf(q.toLowerCase())
                        if (idx < 0) return str.replace(/&/g,"&amp;").replace(/</g,"&lt;")
                        const color = Theme.accentColor
                        return str.slice(0,idx).replace(/&/g,"&amp;").replace(/</g,"&lt;")
                             + "<b style=\"color:" + color + "\">"
                             + str.slice(idx, idx+q.length).replace(/&/g,"&amp;").replace(/</g,"&lt;")
                             + "</b>"
                             + str.slice(idx+q.length).replace(/&/g,"&amp;").replace(/</g,"&lt;")
                    }

                    const hw = hl(word)
                    if (variants.length === 0) return hw

                    const sep = "<span style=\"color:" + Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.4) + "\"> ・ </span>"
                    const vParts = variants.split("・").map(v => hl(v))
                    return hw + sep + vParts.join(sep)
                }
                font.pixelSize: Theme.fontSizeItem
                font.weight: Font.Medium
                color: Theme.textColor
                elide: Text.ElideRight
                maximumLineCount: 1
            }

            // Readings (kana)
            Text {
                visible: readings.length > 0
                width: parent.width
                textFormat: Text.RichText
                text: {
                    if (readings.length === 0) return ""
                    const q = srsLibraryVM ? (typeof srsLibraryVM.activeSearch !== "undefined"
                                              ? srsLibraryVM.activeSearch : "") : ""
                    const doHL = appConfig.highlightMatches && q.length > 0

                    function hl(str) {
                        if (!doHL || str.length === 0) return str.replace(/&/g,"&amp;").replace(/</g,"&lt;")
                        const idx = str.toLowerCase().indexOf(q.toLowerCase())
                        if (idx < 0) return str.replace(/&/g,"&amp;").replace(/</g,"&lt;")
                        return str.slice(0,idx).replace(/&/g,"&amp;").replace(/</g,"&lt;")
                             + "<b style=\"color:" + Theme.accentColor + "\">"
                             + str.slice(idx, idx+q.length).replace(/&/g,"&amp;").replace(/</g,"&lt;")
                             + "</b>"
                             + str.slice(idx+q.length).replace(/&/g,"&amp;").replace(/</g,"&lt;")
                    }

                    const sep = "<span style=\"color:" + Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.35) + "\"> ・ </span>"
                    return readings.split("・").map(r => hl(r)).join(sep)
                }
                font.pixelSize: Theme.fontSizeXSmall
                color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.6)
                elide: Text.ElideRight
                maximumLineCount: 1
            }
        }

        // ── Meaning ───────────────────────────────────────────────────────────
        Text {
            Layout.fillWidth: true
            text: meaning
            font.pixelSize: Theme.fontSizeSmall
            color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.85)
            elide: Text.ElideRight
            maximumLineCount: 2
            wrapMode: Text.Wrap
        }

        // ── State pill ────────────────────────────────────────────────────────
        Rectangle {
            width: stateLbl.implicitWidth + 12
            height: 22
            radius: 4
            color: {
                switch (cardState) {
                case "New":        return Qt.rgba(0.29, 0.62, 1.0,  0.15)
                case "Learning":   return Qt.rgba(1.0,  0.72, 0.25, 0.15)
                case "Relearning": return Qt.rgba(1.0,  0.72, 0.25, 0.15)
                case "Review":     return Qt.rgba(0.20, 0.83, 0.60, 0.15)
                case "Suspended":  return Qt.rgba(0.61, 0.64, 0.69, 0.15)
                default:           return "transparent"
                }
            }
            Text {
                id: stateLbl
                anchors.centerIn: parent
                text: cardState
                font.pixelSize: Theme.fontSizeXSmall
                font.weight: Font.Medium
                color: {
                    switch (cardState) {
                    case "New":        return "#4A9EFF"
                    case "Learning":   return "#FFB83F"
                    case "Relearning": return "#FFB83F"
                    case "Review":     return "#34D399"
                    case "Suspended":  return "#9CA3AF"
                    default:           return Theme.hintColor
                    }
                }
            }
        }

        // ── Due date ──────────────────────────────────────────────────────────
        Text {
            Layout.preferredWidth: 72
            text: due
            font.pixelSize: Theme.fontSizeXSmall
            color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.55)
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignRight
        }

        // ── Details button ────────────────────────────────────────────────────
        Rectangle {
            width: 60; height: 30; radius: 6
            color: detailsMa.pressed
                ? Theme.surfacePress
                : detailsMa.containsMouse ? Theme.surfaceHover : Theme.surfaceSubtle
            border.width: 1; border.color: Theme.surfaceBorder
            Behavior on color { ColorAnimation { duration: 80 } }

            Text {
                anchors.centerIn: parent
                text: "Details"
                font.pixelSize: Theme.fontSizeXSmall
                font.weight: Font.Medium
                color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.8)
            }
            MouseArea {
                id: detailsMa
                anchors.fill: parent
                hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                onClicked: root.openDetails(root.entryId)
            }
        }
    }
}
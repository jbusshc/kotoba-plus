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

    // Recibe el texto activo del campo de búsqueda de SrsLibrary.
    // La página lo pasa con: activeQuery: searchField.text
    property string activeQuery: ""

    // Convierte el nombre de color Material a hex CSS para inline style.
    // Misma tabla que en SearchViewModel.cpp::accentToHex().
    readonly property string accentColorHex: {
        const t = {
            "red":"#F44336","pink":"#E91E63","purple":"#9C27B0",
            "deeppurple":"#673AB7","indigo":"#3F51B5","blue":"#2196F3",
            "lightblue":"#03A9F4","cyan":"#00BCD4","teal":"#009688",
            "green":"#4CAF50","lightgreen":"#8BC34A","lime":"#CDDC39",
            "yellow":"#FFEB3B","amber":"#FFC107","orange":"#FF9800",
            "deeporange":"#FF5722","brown":"#795548","bluegrey":"#607D8B"
        }
        return t[appConfig.accentColor] || "#2196F3"
    }


    signal openDetails(int entryId)

    readonly property bool hasReadings:  readings.length > 0
    readonly property bool doHighlight:  appConfig.highlightMatches && activeQuery.length > 0

    // Altura fija — igual que DictionaryPage para consistencia visual
    height: hasReadings ? 72 : 56

    // Inline highlight: no depende de searchVM, funciona para SRS
    function hl(str) {
        if (!doHighlight || str.length === 0)
            return str.replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;")
        const q   = activeQuery.toLowerCase()
        const idx = str.toLowerCase().indexOf(q)
        if (idx < 0)
            return str.replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;")
        function esc(s) {
            return s.replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;")
        }
        return esc(str.slice(0, idx))
             + "<b style=\"color:" + accentColorHex + "\">"
             + esc(str.slice(idx, idx + q.length))
             + "</b>"
             + esc(str.slice(idx + q.length))
    }

    // ── Hover / press ─────────────────────────────────────────────────────────
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
        anchors.left: parent.left; anchors.right: parent.right
        anchors.leftMargin: 16
        height: 1
        color: Theme.dividerColor; opacity: 0.5
    }

    // ── Layout ────────────────────────────────────────────────────────────────
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16; anchors.rightMargin: 12
        spacing: 12

        // Barra accent (igual que DictionaryPage)
        Rectangle {
            width: 3
            Layout.fillHeight: true
            Layout.topMargin: 8; Layout.bottomMargin: 8
            radius: 2
            color: Theme.accentColor
            opacity: root.hovered ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 120 } }
        }

        // ── Texto: headword / readings / gloss ────────────────────────────────
        Column {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 2

            // Headword + variants kanji
            Text {
                width: parent.width
                textFormat: Text.RichText
                text: {
                    const hw = root.hl(word)
                    if (variants.length === 0) return hw
                    const dimColor = Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.45)
                    const sep = "<span style=\"color:" + dimColor + "\"> ・ </span>"
                    return hw + sep + variants.split("・").map(v => root.hl(v)).join(sep)
                }
                font.pixelSize: Theme.fontSizeItem
                font.weight: Font.Medium
                color: Theme.textColor
                elide: Text.ElideRight
                maximumLineCount: 1
            }

            // Readings kana
            Text {
                visible: root.hasReadings
                width: parent.width
                textFormat: Text.RichText
                text: {
                    if (!root.hasReadings) return ""
                    const dimColor = Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.35)
                    const sep = "<span style=\"color:" + dimColor + "\"> ・ </span>"
                    return readings.split("・").map(r => root.hl(r)).join(sep)
                }
                font.pixelSize: Theme.fontSizeXSmall
                color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.6)
                elide: Text.ElideRight
                maximumLineCount: 1
            }

            // Gloss / meaning
            Text {
                width: parent.width
                text: meaning
                font.pixelSize: Theme.fontSizeSmall
                color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.8)
                elide: Text.ElideRight
                maximumLineCount: 1
            }
        }

        // ── State pill — ancho fijo para que no baile ─────────────────────────
        Rectangle {
            Layout.alignment: Qt.AlignVCenter
            // "Relearning" es el label más largo (~68px en fontSizeXSmall Medium)
            width: 76; height: 22; radius: 4
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
                font.pixelSize: Theme.fontSizeXSmall; font.weight: Font.Medium
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

        // ── Details button ────────────────────────────────────────────────────
        Rectangle {
            Layout.alignment: Qt.AlignVCenter
            width: 58; height: 28; radius: 6
            color: detailsMa.pressed
                ? Theme.surfacePress
                : detailsMa.containsMouse ? Theme.surfaceHover : Theme.surfaceSubtle
            border.width: 1; border.color: Theme.surfaceBorder
            Behavior on color { ColorAnimation { duration: 80 } }

            Text {
                anchors.centerIn: parent
                text: "Details"
                font.pixelSize: Theme.fontSizeXSmall; font.weight: Font.Medium
                color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.8)
            }
            MouseArea {
                id: detailsMa
                anchors.fill: parent
                hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                onClicked: root.openDetails(root.entryId)
            }
        }

        // Flecha hover
        Text {
            text: "›"
            font.pixelSize: Theme.fontSizeMedium
            color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.35)
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
        // Click en el área general va a details igual que dictionary va a entry
        onClicked:  root.openDetails(root.entryId)
    }
}
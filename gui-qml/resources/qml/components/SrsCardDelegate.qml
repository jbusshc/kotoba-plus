import QtQuick
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

    property string activeQuery:      ""
    property bool   highlightEnabled: false
    property var    highlightFunc

    readonly property bool doHighlight: highlightEnabled && activeQuery.length > 0

    signal openDetails(int entryId)

    // Altura dinámica: el contenido manda
    height: contentCol.implicitHeight + 20

    property bool hovered: false
    property bool pressed: false

    // ── Helpers ───────────────────────────────────────────────────────────────
    function _escape(s) {
        return s.replace(/&/g, "&amp;").replace(/</g, "&lt;")
    }

    function _hl(s) {
        return (highlightFunc && root.doHighlight) ? highlightFunc(s) : _escape(s)
    }

    function _dotSep(alpha) {
        const c = Theme.hintColor
        return "<span style=\"color:" + Qt.rgba(c.r, c.g, c.b, alpha) + "\"> ・ </span>"
    }

    function _wordText() {
        const hw = _hl(word)
        if (variants.length === 0) return hw
        const sep    = _dotSep(0.45)
        const vParts = variants.split("・").map(v => _hl(v))
        return hw + sep + vParts.join(sep)
    }

    function _readingsText() {
        if (!readings || readings.length === 0) return ""
        const sep   = _dotSep(0.35)
        const parts = readings.split("・").map(r => _hl(r))
        return parts.join(sep)
    }

    // ── Background ────────────────────────────────────────────────────────────
    Rectangle {
        anchors.fill: parent
        color: root.pressed
            ? Qt.rgba(Theme.accentColor.r, Theme.accentColor.g, Theme.accentColor.b, 0.10)
            : root.hovered ? Theme.surfaceHover : "transparent"
        Behavior on color { ColorAnimation { duration: 100 } }
    }

    Rectangle {
        anchors { bottom: parent.bottom; left: parent.left; right: parent.right; leftMargin: 16 }
        height: 1; color: Theme.dividerColor; opacity: 0.5
    }

    // ── Content ───────────────────────────────────────────────────────────────
    RowLayout {
        anchors {
            left: parent.left; right: parent.right
            verticalCenter: parent.verticalCenter
            leftMargin: 16; rightMargin: 12
        }
        spacing: 12

        // Hover accent bar
        Rectangle {
            width:  3
            Layout.fillHeight:   true
            Layout.topMargin:    8
            Layout.bottomMargin: 8
            radius: 2
            color:   Theme.accentColor
            opacity: root.hovered ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 120 } }
        }

        // Text column
        Column {
            id: contentCol
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 2

            Text {
                width:          parent.width
                textFormat:     Text.RichText
                text:           root._wordText()
                font.pixelSize: Theme.fontSizeItem
                font.weight:    Font.Medium
                color:          Theme.textColor
                wrapMode:       Text.NoWrap
                elide:          Text.ElideRight
            }

            Text {
                visible:        readings.length > 0
                width:          parent.width
                textFormat:     Text.RichText
                text:           root._readingsText()
                font.pixelSize: Theme.fontSizeXSmall
                color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.6)
                wrapMode:       Text.NoWrap
                elide:          Text.ElideRight
            }

            Text {
                width:          parent.width
                textFormat:     Text.RichText
                text:           root._hl(meaning)
                font.pixelSize: Theme.fontSizeSmall
                color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.8)
                wrapMode:       Text.NoWrap
                elide:          Text.ElideRight
            }
        }

        // State chip — tamaño proporcional a la fuente
        Rectangle {
            Layout.alignment: Qt.AlignVCenter
            implicitWidth:  stateChipTxt.implicitWidth + Theme.fontSizeSmall
            implicitHeight: Theme.fontSizeSmall * 1.8
            radius: 4
            color: Theme.srsStateColorBg(cardState, 0.15)

            Text {
                id: stateChipTxt
                anchors.centerIn: parent
                text:           cardState === "Relearning" ? "Relearn" : cardState
                font.pixelSize: Theme.fontSizeXSmall
                font.weight:    Font.Medium
                color:          Theme.srsStateColor(cardState)
            }
        }

        // Details button — tamaño proporcional
        Rectangle {
            Layout.alignment: Qt.AlignVCenter
            implicitWidth:  detailsTxt.implicitWidth + Theme.fontSizeSmall * 1.2
            implicitHeight: Theme.fontSizeSmall * 2.0
            radius: 6
            color: detailsMa.pressed
                ? Theme.surfacePress
                : detailsMa.containsMouse ? Theme.surfaceHover : Theme.surfaceSubtle
            border.width: 1; border.color: Theme.surfaceBorder
            Behavior on color { ColorAnimation { duration: 80 } }

            Text {
                id: detailsTxt
                anchors.centerIn: parent
                text:           "Details"
                font.pixelSize: Theme.fontSizeXSmall
                font.weight:    Font.Medium
                color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.8)
            }

            MouseArea {
                id:           detailsMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape:  Qt.PointingHandCursor
                onClicked:    root.openDetails(root.entryId)
            }
        }

        // Hover chevron
        Text {
            text:           "›"
            font.pixelSize: Theme.fontSizeMedium
            color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.35)
            opacity: root.hovered ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 120 } }
        }
    }

    // ── Full-row tap ──────────────────────────────────────────────────────────
    MouseArea {
        anchors.fill:  parent
        hoverEnabled:  true
        cursorShape:   Qt.PointingHandCursor
        onEntered:     root.hovered = true
        onExited:      root.hovered = false
        onPressed:     root.pressed = true
        onReleased:    root.pressed = false
        onClicked:     root.openDetails(root.entryId)
    }
}
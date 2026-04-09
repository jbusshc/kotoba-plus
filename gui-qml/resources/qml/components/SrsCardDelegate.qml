import QtQuick
import QtQuick.Layouts
import "../theme"

Item {
    id: root

    property string word
    property string meaning
    property string variants: ""
    property string readings: ""
    property int    entryId:  -1

    property bool   recogPresent: false
    property string recogState:   ""
    property string recogDue:     ""

    property bool   recallPresent: false
    property string recallState:   ""
    property string recallDue:     ""

    property string activeQuery:      ""
    property bool   highlightEnabled: false
    property var    highlightFunc

    readonly property bool doHighlight: highlightEnabled && activeQuery.length > 0

    signal openDetails(int entryId)

    property bool hovered: false
    property bool pressed: false

    // El Item se dimensiona exactamente igual que el Column + padding vertical
    width:          parent ? parent.width : 0
    implicitHeight: contentCol.implicitHeight + 20
    height:         implicitHeight

    // ── Helpers ───────────────────────────────────────────────────────────────
    function _escape(s) {
        return s.replace(/&/g, "&amp;").replace(/</g, "&lt;")
    }
    function _hl(s) {
        return (highlightFunc && doHighlight) ? highlightFunc(s) : _escape(s)
    }
    function _dotSep(alpha) {
        const c = Theme.hintColor
        return "<span style=\"color:" + Qt.rgba(c.r, c.g, c.b, alpha) + "\"> ・ </span>"
    }
    function _wordText() {
        const hw = _hl(word)
        if (!variants || variants.length === 0) return hw
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

    // ── Content Column — SIN anchors, solo x/y/width ──────────────────────────
    // Esto garantiza que implicitHeight fluya sin interferencia de bindings
    // de posición.
    Column {
        id:      contentCol
        x:       16
        y:       10
        width:   parent.width - 28   // 16 izq + 12 der
        spacing: 3

        // ── Headword row ──────────────────────────────────────────────────────
        Item {
            width:          parent.width
            implicitHeight: wordText.implicitHeight

            // Hover accent bar
            Rectangle {
                x:      -8
                y:      0
                width:  3
                height: parent.implicitHeight
                radius: 2
                color:   Theme.accentColor
                opacity: root.hovered ? 1 : 0
                Behavior on opacity { NumberAnimation { duration: 120 } }
            }

            Text {
                id:             wordText
                width:          parent.width - detailsBtn.width - chevron.width - 16
                textFormat:     Text.RichText
                text:           root._wordText()
                font.pixelSize: Theme.fontSizeItem
                font.weight:    Font.Medium
                color:          Theme.textColor
                wrapMode:       Text.NoWrap
                elide:          Text.ElideRight
            }

            // Details button
            Rectangle {
                id: detailsBtn
                anchors { right: chevron.left; rightMargin: 4; verticalCenter: parent.verticalCenter }
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
                    id: detailsMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape:  Qt.PointingHandCursor
                    onClicked: root.openDetails(root.entryId)
                }
            }

            // Chevron
            Text {
                id:             chevron
                anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                text:           "›"
                font.pixelSize: Theme.fontSizeMedium
                color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.35)
                opacity: root.hovered ? 1 : 0
                Behavior on opacity { NumberAnimation { duration: 120 } }
            }
        }

        // ── Readings ──────────────────────────────────────────────────────────
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

        // ── Meaning ───────────────────────────────────────────────────────────
        Text {
            width:          parent.width
            textFormat:     Text.RichText
            text:           root._hl(meaning)
            font.pixelSize: Theme.fontSizeSmall
            color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.8)
            wrapMode:       Text.NoWrap
            elide:          Text.ElideRight
        }

        // ── Badges ────────────────────────────────────────────────────────────
        Row {
            visible: recogPresent || recallPresent
            spacing: 6
            topPadding: 2

            // Recognition
            Rectangle {
                visible:      recogPresent
                height:       rRow.implicitHeight + 6
                width:        rRow.implicitWidth  + 12
                radius:       4
                color:        Theme.srsStateColorBg(recogState, 0.15)
                border.width: 1
                border.color: Theme.srsStateColorBg(recogState, 0.40)

                Row {
                    id: rRow
                    anchors.centerIn: parent
                    spacing: 3

                    Text {
                        text:           Theme.srsStateIcon(recogState)
                        font.pixelSize: Theme.fontSizeTiny
                        color:          Theme.srsStateColor(recogState)
                        visible:        recogState !== ""
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text:           "Recog · " + (recogDue !== "" ? recogDue : recogState)
                        font.pixelSize: Theme.fontSizeTiny
                        font.weight:    Font.Medium
                        color:          Theme.srsStateColor(recogState)
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            // Recall
            Rectangle {
                visible:      recallPresent
                height:       rcRow.implicitHeight + 6
                width:        rcRow.implicitWidth  + 12
                radius:       4
                color:        Theme.srsStateColorBg(recallState, 0.15)
                border.width: 1
                border.color: Theme.srsStateColorBg(recallState, 0.40)

                Row {
                    id: rcRow
                    anchors.centerIn: parent
                    spacing: 3

                    Text {
                        text:           Theme.srsStateIcon(recallState)
                        font.pixelSize: Theme.fontSizeTiny
                        color:          Theme.srsStateColor(recallState)
                        visible:        recallState !== ""
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text:           "Recall · " + (recallDue !== "" ? recallDue : recallState)
                        font.pixelSize: Theme.fontSizeTiny
                        font.weight:    Font.Medium
                        color:          Theme.srsStateColor(recallState)
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
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
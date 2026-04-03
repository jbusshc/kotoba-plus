// ActionButton.qml
// primary=true  → filled accent background
// primary=false → ghost / subtle background
import QtQuick
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: root

    property string label:       ""
    property string sublabel:    ""
    property color  accent:      Theme.accentColor
    property bool   primary:     false
    property bool   interactive: true

    signal clicked()

    implicitHeight: Theme.minTapTarget + 12   // 60 px at scale 1
    radius: 8

    color: root.primary
        ? (ma.pressed && root.interactive
            ? Qt.darker(root.accent, 1.10)
            : ma.containsMouse && root.interactive
                ? Qt.lighter(root.accent, 1.15)
                : root.accent)
        : (ma.pressed && root.interactive
            ? Theme.surfacePress
            : ma.containsMouse && root.interactive
                ? Theme.surfaceHover
                : Theme.surfaceSubtle)

    border.width: root.primary ? 0 : 1
    border.color: Theme.surfaceBorder

    opacity: root.interactive ? 1.0 : 0.35

    Behavior on color { ColorAnimation { duration: 120 } }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 2

        Text {
            Layout.alignment: Qt.AlignHCenter
            text:           root.label
            font.pixelSize: Theme.fontSizeBody
            font.weight:    Font.DemiBold
            color: root.primary ? "white" : Theme.textColor
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text:           root.sublabel
            font.pixelSize: Theme.fontSizeTiny
            color: root.primary ? Qt.rgba(1,1,1,0.65) : Theme.hintColor
            visible: root.sublabel.length > 0
        }
    }

    MouseArea {
        id: ma
        anchors.fill: parent
        hoverEnabled: true
        cursorShape:  root.interactive ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked:    if (root.interactive) root.clicked()
    }
}

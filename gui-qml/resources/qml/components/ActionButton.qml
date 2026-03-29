import QtQuick
import QtQuick.Layouts
import Kotoba 1.0

// ── ActionButton ──────────────────────────────────────────────────────────────
// Used in SrsDashboard for "Start Study Session" / "Browse Cards".
// primary=true  → filled accent background
// primary=false → subtle ghost style

Rectangle {
    id: root

    property string label:    ""
    property string sublabel: ""
    property color  accent:   Theme.accentColor
    property bool   primary:  false
    property bool   enabled:  true

    signal clicked()

    height: Theme.minTapTarget + 12   // 60px at scale 1
    radius: 8

    color: root.primary
        ? (btnMouse.pressed && root.enabled
            ? Qt.darker(root.accent, 1.10)
            : btnMouse.containsMouse && root.enabled
                ? Qt.lighter(root.accent, 1.15)
                : root.accent)
        : (btnMouse.pressed && root.enabled
            ? Theme.surfacePress
            : btnMouse.containsMouse && root.enabled
                ? Theme.surfaceHover
                : Theme.surfaceSubtle)

    border.width: root.primary ? 0 : 1
    border.color: Theme.surfaceBorder

    opacity: root.enabled ? 1.0 : 0.35

    Behavior on color { ColorAnimation { duration: 120 } }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 2

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: root.label
            font.pixelSize: Theme.fontSizeBody
            font.weight: Font.DemiBold
            color: root.primary ? "white" : Theme.textColor
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: root.sublabel
            font.pixelSize: Theme.fontSizeTiny
            color: root.primary
                ? Qt.rgba(1,1,1,0.65)
                : Theme.hintColor
            visible: root.sublabel.length > 0
        }
    }

    MouseArea {
        id: btnMouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: if (root.enabled) root.clicked()
    }
}

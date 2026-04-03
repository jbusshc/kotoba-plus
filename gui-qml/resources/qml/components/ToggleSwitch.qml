// ToggleSwitch.qml
import QtQuick
import "../theme"

Rectangle {
    id: root
    property bool checked: false
    signal toggled(bool newValue)

    implicitWidth:  48
    implicitHeight: 28
    radius:         height / 2

    color: checked
        ? Qt.rgba(Theme.accentColor.r, Theme.accentColor.g, Theme.accentColor.b, 0.25)
        : Theme.surfaceSubtle
    border.width: 1
    border.color: checked
        ? Qt.rgba(Theme.accentColor.r, Theme.accentColor.g, Theme.accentColor.b, 0.6)
        : Theme.surfaceBorder
    Behavior on color        { ColorAnimation { duration: 150 } }
    Behavior on border.color { ColorAnimation { duration: 150 } }

    Rectangle {
        id: knob
        width: 20; height: 20; radius: 10
        anchors.verticalCenter: parent.verticalCenter
        x:     root.checked ? parent.width - width - 6 : 6
        color: root.checked ? Theme.accentColor : Theme.surfaceInactive
        Behavior on x     { NumberAnimation { duration: 150; easing.type: Easing.OutQuad } }
        Behavior on color { ColorAnimation  { duration: 150 } }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape:  Qt.PointingHandCursor
        onClicked:    root.toggled(!root.checked)
    }
}

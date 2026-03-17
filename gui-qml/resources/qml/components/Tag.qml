import QtQuick
import "../theme"
import Kotoba 1.0

Rectangle {
    id: root

    property string label: ""
    property color tagColor: Theme.hintColor

    radius: 6

    // fondo suave
    color: Qt.rgba(
        root.tagColor.r,
        root.tagColor.g,
        root.tagColor.b,
        Theme.darkTheme ? 0.25 : 0.12
    )

    border.color: Qt.rgba(
        root.tagColor.r,
        root.tagColor.g,
        root.tagColor.b,
        0.4
    )
    border.width: 1

    height: textItem.implicitHeight + 6
    width: textItem.implicitWidth + 12

    // 🔥 TEXTO INTELIGENTE
    property color textOnTag: Theme.darkTheme
        ? "#121212"   // oscuro en dark mode
        : "#212121"   // oscuro en light mode (tu textColor ya es oscuro)

    Text {
        id: textItem
        anchors.centerIn: parent
        text: root.label
        font.pixelSize: 11
        color: root.textOnTag
    }
}
import QtQuick
import "../theme"

import Kotoba 1.0


Rectangle {
    id: root
    property string label: ""
    property color tagColor: Theme.hintColor

    radius: 6
    height: textItem.implicitHeight + 6
    width: textItem.implicitWidth + 10
    color: tagColor
    border.color: tagColor
    border.width: 1

    Text {
        id: textItem
        anchors.centerIn: parent
        text: root.label
        font.pixelSize: 12
        color: Theme.background
    }
}
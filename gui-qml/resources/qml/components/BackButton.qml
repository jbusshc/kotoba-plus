// BackButton.qml
// Reusable "‹ Back" button that pops the nearest StackView.
// Usage:  BackButton {}
//         BackButton { onClicked: myStack.pop() }  // optional override
import QtQuick
import QtQuick.Layouts
import "../theme"

Item {
    id: root

    signal clicked()

    implicitWidth:  backLabel.implicitWidth + 24
    implicitHeight: Theme.minTapTarget

    Rectangle {
        anchors.fill: parent
        radius: 6
        color: ma.containsMouse ? Theme.surfaceHover : "transparent"
        Behavior on color { ColorAnimation { duration: 120 } }
    }

    RowLayout {
        anchors.centerIn: parent
        spacing: 5

        Text {
            text:           "‹"
            font.pixelSize: Theme.fontSizeLarge
            color:          Theme.hintColor
        }
        Text {
            id:             backLabel
            text:           "Back"
            font.pixelSize: Theme.fontSizeBody
            font.weight:    Font.Medium
            color:          Theme.hintColor
        }
    }

    MouseArea {
        id: ma
        anchors.fill: parent
        hoverEnabled:  true
        cursorShape:   Qt.PointingHandCursor
        onClicked:     root.clicked()
    }
}

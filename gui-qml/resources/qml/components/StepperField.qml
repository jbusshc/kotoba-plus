// StepperField.qml
// Numeric stepper: [−] [value] [+] [unit]
import QtQuick
import QtQuick.Layouts
import "../theme"

RowLayout {
    id: root

    property int    value:     0
    property int    min:       0
    property int    max:       99999
    property string zeroLabel: ""
    property string unit:      ""

    // Internal step button
    component StepBtn: Rectangle {
        property string lbl: "+"
        signal tapped()

        implicitWidth:  Math.max(28, Theme.minTapTarget - 16)
        implicitHeight: 28
        radius: 6
        color: ma.pressed
            ? Theme.surfacePress
            : ma.containsMouse ? Theme.surfaceHover : Theme.surfaceSubtle
        border.width: 1; border.color: Theme.surfaceBorder
        Behavior on color { ColorAnimation { duration: 100 } }

        Text {
            anchors.centerIn: parent
            text:           parent.lbl
            font.pixelSize: Theme.fontSizeBody
            color:          Theme.hintColor
        }
        MouseArea {
            id: ma; anchors.fill: parent
            hoverEnabled: true; cursorShape: Qt.PointingHandCursor
            onClicked: parent.tapped()
        }
    }

    spacing: 0

    StepBtn {
        lbl: "−"
        onTapped: if (root.value > root.min) root.value--
    }

    Rectangle {
        implicitWidth:  72
        implicitHeight: 28
        color:          Theme.surfaceInput
        border.width: 1; border.color: Theme.surfaceBorder

        TextInput {
            anchors.centerIn: parent
            width: parent.width - 8
            font.pixelSize:      Theme.fontSizeSmall
            font.weight:         Font.Medium
            horizontalAlignment: Text.AlignHCenter
            color: (root.value === 0 && root.zeroLabel !== "") ? Theme.accentColor : Theme.textColor
            validator: IntValidator { bottom: root.min; top: root.max }
            text: (root.value === 0 && root.zeroLabel !== "") ? root.zeroLabel
                                                              : root.value.toString()
            onEditingFinished: {
                if (root.zeroLabel !== "" && text === root.zeroLabel) {
                    root.value = 0
                } else {
                    const v = parseInt(text)
                    if (!isNaN(v)) root.value = Math.max(root.min, Math.min(root.max, v))
                }
            }
        }
    }

    StepBtn {
        lbl: "+"
        onTapped: if (root.value < root.max) root.value++
    }

    Text {
        visible:             root.unit !== ""
        Layout.preferredWidth: 52
        text:                root.unit
        font.pixelSize:      Theme.fontSizeXSmall
        color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.55)
        verticalAlignment: Text.AlignVCenter
    }
}

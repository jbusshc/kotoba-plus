import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

Page {
    property int docId: -1
    padding: 12

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        Button {
            text: "< Back"
            onClicked: stack.pop()
        }

        Text {
            id: heading
            font.pixelSize: 24
            font.bold: true
            color: palette.text
        }

        Text {
            id: readings
            font.pixelSize: 14
            color: Material.hintTextColor
        }

        Repeater {
            id: sensesRepeater
            model: []

            delegate: Text {
                text: modelData
                wrapMode: Text.WordWrap
                color: palette.text
            }
        }
    }

    Component.onCompleted: {
        if (docId !== -1) {
            var map = detailsVM.buildDetails(docId)
            heading.text = map.mainWord || ""
            readings.text = map.readings || ""
            sensesRepeater.model = map.senses || []
        }
    }
}
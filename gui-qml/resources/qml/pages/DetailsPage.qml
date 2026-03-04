import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

Page {
    id: page
    property int docId: -1
    padding: 12

    property bool darkTheme: appConfig && appConfig.config ? appConfig.config.theme === "dark" : true
    property color textColor: darkTheme ? "white" : "black"
    property color hintColor: darkTheme ? "#B0BEC5" : "#757575"

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
            color: textColor
        }

        Text {
            id: readings
            font.pixelSize: 14
            color: hintColor
        }

        Repeater {
            id: sensesRepeater
            model: []

            delegate: Text {
                text: modelData || ""
                wrapMode: Text.WordWrap
                color: textColor
            }
        }
    }

    Component.onCompleted: {
        if (docId !== -1 && detailsVM) {
            var map = detailsVM.buildDetails(docId)
            heading.text = map.mainWord || ""
            readings.text = map.readings || ""
            sensesRepeater.model = map.senses || []
        }
    }

    // reset selection when returning
    onVisibleChanged: if (visible) {
        heading.text = ""
        readings.text = ""
        sensesRepeater.model = []
        docId = -1
    }
}
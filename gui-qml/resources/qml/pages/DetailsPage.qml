import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

Page {
    id: page
    property int docId: -1

    property bool darkTheme: appConfig && appConfig.config ? appConfig.config.theme === "dark" : true
    property color textColor: darkTheme ? "white" : "black"
    property color hintColor: darkTheme ? "#B0BEC5" : "#757575"

    ScrollView {
        anchors.fill: parent
        clip: true

        ColumnLayout {
            id: contentLayout
            width: parent.width
            spacing: 6
            anchors.margins: 12   // ✅ reemplaza padding

            Button {
                text: "< Back"
                onClicked: stack.pop()
                Layout.alignment: Qt.AlignLeft
            }

            Text {
                id: heading
                font.pixelSize: 24
                font.bold: true
                color: textColor
                wrapMode: Text.WordWrap
            }

            Text {
                id: readings
                font.pixelSize: 14
                color: hintColor
                wrapMode: Text.WordWrap
            }

            Repeater {
                id: sensesRepeater
                model: []

                delegate: Text {
                    text: modelData || ""
                    wrapMode: Text.WordWrap
                    color: textColor
                    font.pixelSize: 14
                }
            }

            Item { Layout.fillHeight: true }
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

    onVisibleChanged: if (visible) {
        heading.text = ""
        readings.text = ""
        sensesRepeater.model = []
        docId = -1
    }
}
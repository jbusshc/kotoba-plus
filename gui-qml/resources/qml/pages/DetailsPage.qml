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
    property color dividerColor: darkTheme ? "#424242" : "#E0E0E0"

    function formatGlosses(text) {
        if (!text)
            return ""

        var parts = text.split(";")

        for (var i = 0; i < parts.length; i++)
            parts[i] = "• " + parts[i].trim()

        return parts.join("\n")
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 6

    RowLayout {
        Layout.fillWidth: true

        Button {
            text: "< Back"
            onClicked: stack.pop()
        }

        Item {
            Layout.fillWidth: true
        }

        Button {
            id: addSrsButton
            text: srsVM.contains(docId) ? "Already in SRS" : "Add to SRS"
            enabled: !srsVM.contains(docId)

            onClicked: {
                if (docId !== -1) {
                    srsVM.add(docId)
                    addSrsButton.enabled = false
                    addSrsButton.text = "Already in SRS"
                }
            }
        }
    }
        Text {
            id: heading
            font.pixelSize: 24
            font.bold: true
            color: textColor
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Text {
            id: readings
            font.pixelSize: 14
            color: hintColor
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        // separación extra entre readings y senses
        Item {
            Layout.preferredHeight: 12
        }

        ListView {
            id: sensesList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 12

            model: []

            delegate: Column {
                width: sensesList.width
                spacing: 8

                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: textColor
                    font.pixelSize: 14
                    text: page.formatGlosses(modelData)
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: dividerColor
                    visible: index < sensesList.count - 1
                }
            }
        }
    }

    Component.onCompleted: {
        if (docId !== -1 && detailsVM) {
            var map = detailsVM.buildDetails(docId)

            heading.text = map.mainWord || ""
            readings.text = map.readings || ""
            sensesList.model = map.senses || []
        }
    }

    onVisibleChanged: if (visible) {
        heading.text = ""
        readings.text = ""
        sensesList.model = []
        docId = -1
    }
}
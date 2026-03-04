import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

Page {
    id: page
    padding: 20

    property bool darkTheme: appConfig && appConfig.config ? appConfig.config.theme === "dark" : true
    property color textColor: darkTheme ? "white" : "black"
    property color hintColor: darkTheme ? "#B0BEC5" : "#757575"

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        Text {
            text: "SRS Dashboard"
            font.pixelSize: 24
            font.bold: true
            color: textColor
        }

        Button {
            text: "Start Study"
            onClicked: {
                if (stack && srsVM) {
                    stack.push("qrc:/qml/pages/SrsStudy.qml")
                    srsVM.startSession()
                }
            }
        }
    }
}
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

Page {
    padding: 20

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        Text {
            text: "SRS Dashboard"
            font.pixelSize: 24
            font.bold: true
            color: palette.text
        }

        Button {
            text: "Start Study"
            onClicked: {
                stack.push("qrc:/qml/pages/SrsStudy.qml")
                srsVM.startSession()
            }
        }
    }
}
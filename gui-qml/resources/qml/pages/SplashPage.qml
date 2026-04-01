// resources/qml/pages/SplashPage.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import "../theme"

Item {
    id: splashRoot

    Rectangle {
        anchors.fill: parent
        color: Theme.background

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 28

            Image {
                Layout.alignment: Qt.AlignHCenter
                source: "qrc:/images/icon.svg"
                width: 96; height: 96
                fillMode: Image.PreserveAspectFit
                smooth: true
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "Kotoba+"
                font.pixelSize: 26
                font.weight: Font.Medium
                color: Theme.textColor
            }

            ProgressBar {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 180
                indeterminate: true
            }

            Text {
                id: statusLabel
                Layout.alignment: Qt.AlignHCenter
                text: "Loading…"
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.hintColor
            }
        }
    }

    Connections {
        target: appController

        function onStatusChanged(message) {
            statusLabel.text = message
        }
    }
}
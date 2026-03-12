import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

Page {
    id: page
    padding: 32

    property bool darkTheme: appConfig && appConfig.config ? appConfig.config.theme === "dark" : true
    property color textColor: darkTheme ? "white" : "black"

    component DashboardCard: Rectangle {
        property string label
        property int value
        property color accent

        radius: 12
        color: Material.backgroundColor
        border.color: accent
        border.width: 2

        Layout.fillWidth: true
        Layout.preferredHeight: 140

        Column {
            anchors.centerIn: parent
            width: parent.width
            spacing: 8

            Text {
                text: value
                font.pixelSize: 36
                font.bold: true
                color: accent
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                text: label
                font.pixelSize: 14
                opacity: 0.8
                color: textColor
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 28

        GridLayout {
            columns: 3
            columnSpacing: 20
            rowSpacing: 20
            Layout.fillWidth: true

            DashboardCard {
                label: "Due"
                value: srsVM ? srsVM.dueCount : 0
                accent: Material.color(Material.Red)
            }

            DashboardCard {
                label: "Learning"
                value: srsVM ? srsVM.learningCount : 0
                accent: Material.color(Material.Blue)
            }

            DashboardCard {
                label: "Lapsed"
                value: srsVM ? srsVM.lapsedCount : 0
                accent: Material.color(Material.Orange)
            }
        }

        Item { Layout.fillHeight: true }

        // Botones
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 16

            Button {
                text: enabled ? "Start Study" : "Nothing to study"
                enabled: srsVM && srsVM.dueCount > 0
                font.pixelSize: 20
                padding: 16
                Material.background: Material.Blue
                Material.foreground: "white"

                onClicked: {
                    if (stack && srsVM && srsVM.dueCount > 0) {
                        stack.push("qrc:/qml/pages/SrsStudy.qml")
                        srsVM.startSession()
                    }
                }
            }

            Button {
                text: "Library"
                font.pixelSize: 20
                padding: 16
                Material.background: Material.Gray
                Material.foreground: "white"

                onClicked: {
                    if (stack) {
                        stack.push("qrc:/qml/pages/SrsLibrary.qml")
                    }
                }
            }
        }

        Text {
            visible: srsVM && srsVM.dueCount === 0
            Layout.alignment: Qt.AlignHCenter
            text: {
                if (!srsVM) return ""

                if (srsVM.learningCount > 0)
                    return "You are currently learning cards. Come back later for reviews."

                return "No reviews available right now. New cards will appear later."
            }
            color: Material.hintTextColor
            font.pixelSize: 14
            horizontalAlignment: Text.AlignHCenter
        }

        Item { Layout.fillHeight: true }
    }
}
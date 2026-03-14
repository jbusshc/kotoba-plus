import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

Page {
    id: page
    padding: 32

    property bool darkTheme: appConfig && appConfig.config ? appConfig.config.theme === "dark" : true
    property color textColor: darkTheme ? "white" : "black"
    property color cardBackground: darkTheme ? "#222222" : "#ffffff"

    component DashboardCard: Rectangle {
        property string label
        property int    value
        property color  accent

        radius: 12
        color: cardBackground
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
                label: "Total Cards"
                value: srsVM ? srsVM.totalCount : 0
                accent: Material.color(Material.Blue)
            }

            DashboardCard {
                label: "Reviewed Today"
                value: srsVM ? srsVM.reviewTodayCount : 0
                accent: Material.color(Material.Green)
            }
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 16

            /* ---- Start Study ---- */
            Button {
                id: studyButton
                text: srsVM && srsVM.dueCount > 0 ? "Start Study" : "Nothing to study"
                enabled: srsVM && srsVM.dueCount > 0
                font.pixelSize: 20
                padding: 16
                Layout.preferredWidth: 180

                background: Rectangle {
                    color: studyButton.enabled
                           ? Material.color(Material.Blue)
                           : (page.darkTheme ? "#555" : "#aaa")
                    radius: 8
                }

                contentItem: Text {
                    text: parent.text
                    color: "white"
                    font.pixelSize: 20
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    if (stack && srsVM && srsVM.dueCount > 0) {
                        srsVM.startSession()
                        stack.push("qrc:/qml/pages/SrsStudy.qml")
                    }
                }
            }

            /* ---- Cards List ----
             * BUG FIX: el color antes dependía de studyButton.enabled,
             * así que se ponía gris cuando no había cartas pendientes aunque
             * el botón siempre esté habilitado. Ahora tiene su propio color. */
            Button {
                id: libraryButton
                text: "Cards list"
                font.pixelSize: 20
                padding: 16
                Layout.preferredWidth: 180

                background: Rectangle {
                    color: Material.color(Material.BlueGrey)
                    radius: 8
                }

                contentItem: Text {
                    text: parent.text
                    color: "white"
                    font.pixelSize: 20
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    if (stack)
                        stack.push("qrc:/qml/pages/SrsLibrary.qml")
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
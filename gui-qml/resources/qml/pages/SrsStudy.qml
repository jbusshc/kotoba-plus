import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

import "../theme"

import Kotoba 1.0

Page {
    id: page
    padding: 24

    property bool darkTheme: Theme.darkTheme
    property color textColor: Theme.textColor
    property color hintColor: Theme.hintColor
    property color accentColor: Theme.accentColor
    property color dividerColor: Theme.dividerColor
    property color cardBackground: Theme.cardBackground
    property bool answerShown: false

    property color againColor: Theme.againColor
    property color hardColor: Theme.hardColor
    property color goodColor: Theme.goodColor
    property color easyColor: Theme.easyColor

    property int buttonWidth: 120
    property int buttonHeight: 65

    /*  Iniciar sesión cuando la pantalla ya existe */
    Component.onCompleted: {
        if (srsVM) {
            srsVM.startSession()
        }
    }

    /* Reset visual cuando cambias de carta */
    Connections {
        target: srsVM
        function onCurrentChanged() {
            page.answerShown = false
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 20

        Item { Layout.fillHeight: true }

        Frame {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 640
            Layout.preferredHeight: 260

            background: Rectangle {
                radius: 14
                color: cardBackground
                border.color: dividerColor
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 16

                Item { Layout.fillHeight: true }

                /*  Pregunta desde estado */
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    font.pixelSize: 36
                    font.bold: true
                    color: textColor
                    text: srsVM ? srsVM.currentWord : ""
                }

                /*  Respuesta desde estado */
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    visible: page.answerShown
                    font.pixelSize: 18
                    color: hintColor
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    width: parent.width * 0.9
                    text: srsVM ? srsVM.currentMeaning : ""
                }

                Item { Layout.fillHeight: true }
            }
        }
        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter

            Item {
                Layout.preferredHeight: buttonHeight
                Layout.fillWidth: false

                /* Botón reveal */
                Button {
                    anchors.centerIn: parent
                    visible: !page.answerShown
                    enabled: srsVM && srsVM.hasCard
                    text: "Show Answer"
                    Layout.preferredHeight: buttonHeight

                    onClicked: {
                        page.answerShown = true
                    }
                }

                /* Botones de respuesta */
                RowLayout {
                    anchors.centerIn: parent
                    spacing: 12
                    visible: page.answerShown

                    Button {
                        text: "Again\n" + (srsVM ? srsVM.againInterval : "")
                        Material.background: againColor
                        onClicked: { if (srsVM) srsVM.answerAgain() }
                        Layout.preferredWidth: buttonWidth
                        Layout.preferredHeight: buttonHeight
                    }

                    Button {
                        text: "Hard\n" + (srsVM ? srsVM.hardInterval : "")
                        Material.background: hardColor
                        onClicked: { if (srsVM) srsVM.answerHard() }
                        Layout.preferredWidth: buttonWidth
                        Layout.preferredHeight: buttonHeight
                    }

                    Button {
                        text: "Good\n" + (srsVM ? srsVM.goodInterval : "")
                        Material.background: goodColor
                        onClicked: { if (srsVM) srsVM.answerGood() }
                        Layout.preferredWidth: buttonWidth
                        Layout.preferredHeight: buttonHeight
                    }

                    Button {
                        text: "Easy\n" + (srsVM ? srsVM.easyInterval : "")
                        Material.background: easyColor
                        onClicked: { if (srsVM) srsVM.answerEasy() }
                        Layout.preferredWidth: buttonWidth
                        Layout.preferredHeight: buttonHeight
                    }
                }
            }
        }

        /*  Undo */
        Button {
            Layout.alignment: Qt.AlignHCenter
            visible: srsVM && srsVM.canUndo && !page.answerShown
            text: "↩ Undo"
            flat: true
            Material.foreground: page.hintColor
            onClicked: { if (srsVM) srsVM.undoLastAnswer() }
        }

        Item { Layout.fillHeight: true }
    }

    /*  Fin de sesión */
    Connections {
        target: srsVM

        function onNoMoreCards() {
            if (srsVM) srsVM.saveProfile()
            if (stack) stack.pop()
        }
    }
}
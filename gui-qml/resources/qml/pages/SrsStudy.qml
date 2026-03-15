import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

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

    /* Limpiar estado al entrar a la pantalla */
    onVisibleChanged: {
        if (visible) {
            questionText.text  = ""
            answerText.text    = ""
            page.answerShown   = false
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

                Text {
                    id: questionText
                    Layout.alignment: Qt.AlignHCenter
                    font.pixelSize: 36
                    font.bold: true
                    color: textColor
                    text: ""
                }

                Text {
                    id: answerText
                    Layout.alignment: Qt.AlignHCenter
                    visible: page.answerShown
                    font.pixelSize: 18
                    color: hintColor
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    width: parent.width * 0.9
                }

                Item { Layout.fillHeight: true }
            }
        }

        Button {
            id: revealButton
            Layout.alignment: Qt.AlignHCenter
            enabled: questionText.text !== ""
            text: "Show Answer"

            onClicked: {
                page.answerShown = true
                if (srsVM) srsVM.revealAnswer()
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 12
            visible: page.answerShown

            Button {
                text: "Again\n" + (srsVM ? srsVM.againInterval : "")
                Material.background: page.againColor
                onClicked: { if (srsVM) srsVM.answerAgain() }
            }

            Button {
                text: "Hard\n" + (srsVM ? srsVM.hardInterval : "")
                Material.background: page.hardColor
                onClicked: { if (srsVM) srsVM.answerHard() }
            }

            Button {
                text: "Good\n" + (srsVM ? srsVM.goodInterval : "")
                Material.background: page.goodColor
                onClicked: { if (srsVM) srsVM.answerGood() }
            }

            Button {
                text: "Easy\n" + (srsVM ? srsVM.easyInterval : "")
                Material.background: page.easyColor
                onClicked: { if (srsVM) srsVM.answerEasy() }
            }
        }

        Item { Layout.fillHeight: true }
    }

    Connections {
        target: srsVM

        function onShowQuestion(word) {
            questionText.text = word || ""
            answerText.text   = ""
            page.answerShown  = false
        }

        function onShowAnswer(ans) {
            answerText.text  = ans || ""
            page.answerShown = true
        }

        /*
         * BUG FIX: antes se hacía stack.pop() directamente sin guardar.
         * Ahora llamamos saveProfile() para que el progreso de la sesión
         * quede persistido antes de salir de la pantalla de estudio.
         */
        function onNoMoreCards() {
            if (srsVM) srsVM.saveProfile()
            if (stack) stack.pop()
        }
    }
}

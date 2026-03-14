import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

Page {
    id: page
    padding: 24

    property bool darkTheme: appConfig && appConfig.config ? appConfig.config.theme === "dark" : true
    property color textColor: darkTheme ? "white" : "black"
    property color hintColor: darkTheme ? "#B0BEC5" : "#757575"

    property bool answerShown: false

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
                color: darkTheme ? "#1E1E1E" : "#FAFAFA"
                border.color: darkTheme ? "#333" : "#DDD"
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
                Material.background: "#D32F2F"
                onClicked: { if (srsVM) srsVM.answerAgain() }
            }

            Button {
                text: "Hard\n" + (srsVM ? srsVM.hardInterval : "")
                Material.background: "#F57C00"
                onClicked: { if (srsVM) srsVM.answerHard() }
            }

            Button {
                text: "Good\n" + (srsVM ? srsVM.goodInterval : "")
                Material.background: "#388E3C"
                onClicked: { if (srsVM) srsVM.answerGood() }
            }

            Button {
                text: "Easy\n" + (srsVM ? srsVM.easyInterval : "")
                Material.background: "#1976D2"
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

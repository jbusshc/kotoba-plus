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
            id: q
            font.pixelSize: 28
            font.bold: true
            color: textColor
        }

        Text {
            id: a
            visible: false
            font.pixelSize: 16
            color: hintColor
            wrapMode: Text.WordWrap
        }

        Button {
            text: "Show Answer"
            onClicked: if (srsVM) srsVM.revealAnswer()
        }

        RowLayout {
            spacing: 8

            Button { text: "Again"; onClicked: if (srsVM) srsVM.answerAgain() }
            Button { text: "Hard"; onClicked: if (srsVM) srsVM.answerHard() }
            Button { text: "Good"; onClicked: if (srsVM) srsVM.answerGood() }
            Button { text: "Easy"; onClicked: if (srsVM) srsVM.answerEasy() }
        }
    }

    Connections {
        target: srsVM
        function onShowQuestion(word) {
            q.text = word || ""
            a.visible = false
        }
        function onShowAnswer(ans) {
            a.text = ans || ""
            a.visible = true
        }
        function onNoMoreCards() {
            q.text = "No more cards"
            a.visible = false
        }
    }

    // reset view on back
    onVisibleChanged: if (visible) {
        q.text = ""
        a.text = ""
        a.visible = false
    }
}
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
            id: q
            font.pixelSize: 28
            font.bold: true
            color: palette.text
        }

        Text {
            id: a
            visible: false
            font.pixelSize: 16
            color: Material.hintTextColor
            wrapMode: Text.WordWrap
        }

        Button {
            text: "Show Answer"
            onClicked: srsVM.revealAnswer()
        }

        RowLayout {
            spacing: 8

            Button { text: "Again"; onClicked: srsVM.answerAgain() }
            Button { text: "Hard"; onClicked: srsVM.answerHard() }
            Button { text: "Good"; onClicked: srsVM.answerGood() }
            Button { text: "Easy"; onClicked: srsVM.answerEasy() }
        }
    }

    Connections {
        target: srsVM

        function onShowQuestion(word) {
            q.text = word
            a.visible = false
        }

        function onShowAnswer(ans) {
            a.text = ans
            a.visible = true
        }

        function onNoMoreCards() {
            q.text = "No more cards"
            a.visible = false
        }
    }
}
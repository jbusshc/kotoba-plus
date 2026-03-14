import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {

    property string word
    property string meaning
    property string state
    property string due
    property int entryId

    signal openDetails(int entryId)

    height: 60
    radius: 6
    color: "#202020"

    RowLayout {

        anchors.fill: parent
        anchors.margins: 8
        spacing: 10

        Label {
            text: word
            font.bold: true
            Layout.preferredWidth: 160
            elide: Label.ElideRight
        }

        Label {
            text: meaning
            Layout.fillWidth: true
            elide: Label.ElideRight
        }

        Label {
            text: state
            Layout.preferredWidth: 90
        }

        Label {
            text: due
            Layout.preferredWidth: 100
        }

        Button {
            text: "Details"

            onClicked: {
                openDetails(entryId)
            }
        }
    }
}
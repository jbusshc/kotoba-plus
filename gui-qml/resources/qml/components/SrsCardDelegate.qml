import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Kotoba 1.0

Rectangle {

    property string word
    property string meaning
    property string cardState
    property string due
    property int entryId

    signal openDetails(int entryId)

    height: 60
    radius: 6
    color: Theme.cardBackground
    border.color: Theme.dividerColor
    border.width: 1

    RowLayout {

        anchors.fill: parent
        anchors.margins: 8
        spacing: 10

        Label {
            text: word
            font.bold: true
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            elide: Label.ElideRight
        }

        Label {
            text: meaning
            Layout.fillWidth: true
            Layout.preferredWidth: 2
            elide: Label.ElideRight
        }

        Label {
            text: cardState
            Layout.preferredWidth: 80
        }

        Label {
            text: due
            Layout.preferredWidth: 90
        }

        Button {
            text: "Details"
            onClicked: openDetails(entryId)
        }
    }
}
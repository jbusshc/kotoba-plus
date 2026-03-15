import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Kotoba 1.0

Rectangle {

    property string word
    property string meaning
    property string state
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
            Layout.preferredWidth: 160
            elide: Label.ElideRight
            color: Theme.textColor
        }

        Label {
            text: meaning
            Layout.fillWidth: true
            elide: Label.ElideRight
            color: Theme.hintColor
        }

        Label {
            text: state
            Layout.preferredWidth: 90
            color: Theme.hintColor
        }

        Label {
            text: due
            Layout.preferredWidth: 100
            color: Theme.hintColor
        }

        Button {
            text: "Details"
            onClicked: openDetails(entryId)
        }
    }
}
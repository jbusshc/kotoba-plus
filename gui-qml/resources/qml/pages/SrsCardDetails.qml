import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Kotoba 1.0
import "../components"

Page {
    id: page

    property int entryId: -1
    property var entryData: ({})

    property color textColor: Theme.textColor
    property color hintColor: Theme.hintColor
    property color accentColor: Theme.accentColor
    property color dividerColor: Theme.dividerColor

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16

        RowLayout {
            Layout.fillWidth: true

            Button {
                text: "← Back"
                onClicked: stack.pop()
            }

            Item { Layout.fillWidth: true }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true

            radius: 8
            border.width: 1
            border.color: dividerColor
            color: Theme.cardBackground

            ScrollView {

                anchors.fill: parent
                anchors.margins: 16

                EntryView {
                    width: parent.width
                    entryData: page.entryData
                    mode: "srs"

                    textColor: page.textColor
                    hintColor: page.hintColor
                    accentColor: page.accentColor
                    dividerColor: page.dividerColor
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            Item { Layout.fillWidth: true }

            Button {
                text: "Suspend"
                onClicked: srsLibraryVM.suspend(entryId)
            }

            Button {
                text: "Reset"
                onClicked: srsLibraryVM.reset(entryId)
            }

            Button {
                text: "Delete"
                onClicked: {
                    srsLibraryVM.remove(entryId)
                    stack.pop()
                }
            }
        }
    }

    Component.onCompleted: {
        if (entryId >= 0)
            entryData = detailsVM.mapEntry(entryId)
    }
}
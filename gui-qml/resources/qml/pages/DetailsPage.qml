import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Kotoba 1.0
import "../components"

Page {
    id: page

    property int docId: -1
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

            Button {
                text: srsVM && srsVM.contains(docId) ? "In SRS" : "Add to SRS"
                enabled: srsVM && !srsVM.contains(docId)

                onClicked: srsVM.add(docId)
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            EntryView {
                width: parent.width
                entryData: page.entryData
                mode: "dictionary"

                textColor: page.textColor
                hintColor: page.hintColor
                accentColor: page.accentColor
                dividerColor: page.dividerColor
            }
        }
    }

    Component.onCompleted: {
        if (docId !== -1)
            entryData = detailsVM.mapEntry(docId)
    }
}
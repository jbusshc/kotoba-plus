import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import Kotoba 1.0

Page {
    id: page
    padding: 12

    property bool darkTheme: Theme.darkTheme
    property color textColor: Theme.textColor
    property color hintColor: Theme.hintColor
    property color accentColor: Theme.accentColor
    property color dividerColor: Theme.dividerColor

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        TextField {
            id: searchField
            Layout.fillWidth: true
            placeholderText: "Search Japanese word..."
            text: searchVM ? searchVM.query : ""
            color: textColor
            placeholderTextColor: hintColor
            onTextChanged: if (searchVM) searchVM.query = text
        }

        ListView {
            id: listView
            model: searchModel
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            boundsBehavior: Flickable.StopAtBounds

        delegate: Rectangle {
            id: delegateRect
            width: ListView.view.width
            height: contentColumn.implicitHeight + 16
            color: "transparent"

            Column {
                id: contentColumn
                anchors.fill: parent
                anchors.margins: 10
                spacing: 4

                Text { text: headword; font.pixelSize: 18; font.bold: true; color: textColor; wrapMode: Text.Wrap }
                Text { text: gloss; font.pixelSize: 13; color: hintColor; wrapMode: Text.Wrap }
            }

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                onClicked: {
                    // Trigger the flash
                    flashAnim.running = true
                    // Navigate to details page
                    stack.push("qrc:/qml/pages/DetailsPage.qml", { docId: docId })
                }
            }

            Rectangle {
                id: flashRect
                anchors.fill: parent
                color: accentColor.lighter(1.5)
                opacity: 0
            }

            // Animation for the flash effect
            Behavior on opacity {
                NumberAnimation { duration: 300; easing.type: Easing.InOutQuad }
            }

            SequentialAnimation {
                id: flashAnim
                running: false
                PropertyAnimation { target: flashRect; property: "opacity"; to: 1; duration: 100 }
                PropertyAnimation { target: flashRect; property: "opacity"; to: 0; duration: 300 }
            }

            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: dividerColor }
        }

            onContentYChanged: { if (contentY + height >= contentHeight - 150) searchVM.needMore() }

            Label { anchors.centerIn: parent; visible: listView.count === 0 && searchField.text.length > 0; text: "No results found"; color: hintColor }
        }
    }
}
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

Page {
    padding: 12

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

    TextField {
        id: searchField
        Layout.fillWidth: true
        placeholderText: "Search Japanese word..."

        text: searchVM ? searchVM.query : ""

        onTextChanged: {
            if (searchVM)
                searchVM.query = text
        }
    }

        ListView {
            id: listView
            model: searchModel

            Layout.fillWidth: true
            Layout.fillHeight: true

            clip: true
            boundsBehavior: Flickable.StopAtBounds

            delegate: Rectangle {
                width: ListView.view.width
                height: contentColumn.implicitHeight + 16
                color: mouseArea.pressed
                       ? Material.accentColor
                       : "transparent"

                Column {
                    id: contentColumn
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 4

                    Text {
                        text: headword
                        font.pixelSize: 18
                        font.bold: true
                        color: palette.text
                        wrapMode: Text.Wrap
                    }

                    Text {
                        text: gloss
                        font.pixelSize: 13
                        color: Material.hintTextColor
                        wrapMode: Text.Wrap
                    }
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    onClicked: {
                        listView.currentIndex = index
                        stack.push("qrc:/qml/pages/DetailsPage.qml", {
                            docId: docId
                        })
                    }
                }

                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: 1
                    color: Material.dividerColor
                }
            }

            onContentYChanged: {
                if (contentY + height >= contentHeight - 150)
                    searchVM.needMore()
            }

            Label {
                anchors.centerIn: parent
                visible: listView.count === 0 && searchField.text.length > 0
                text: "No results found"
                color: Material.hintTextColor
            }
        }
    }
}
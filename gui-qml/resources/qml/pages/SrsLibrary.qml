import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

Page {
    id: page

    property bool darkTheme: appConfig && appConfig.config ? appConfig.config.theme === "dark" : true
    property color textColor: darkTheme ? "white" : "black"
    property color hintColor: darkTheme ? "#B0BEC5" : "#757575"
    property color cardColor: darkTheme ? "#2C2C2C" : "#FAFAFA"
    property color cardBorder: darkTheme ? "#555" : "#DDD"

    // Header con back y título
    RowLayout {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 12
        anchors.margins: 24

        Button {
            text: "< Back"
            onClicked: stack.pop()
        }
    }

    // Lista scrolleable de tarjetas
    ListView {
        id: cardList
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 24

        model: srsLibraryVM
        clip: true
        spacing: 12

        Component.onCompleted: {
            if (srsLibraryVM) srsLibraryVM.reloadCards()
        }

        delegate: Rectangle {
            width: ListView.view.width
            height: 80
            radius: 12
            color: cardColor
            border.color: cardBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12

                // Columna principal
                ColumnLayout {
                    Layout.fillWidth: true
                    Text { text: word; font.pixelSize: 18; color: textColor; elide: Text.ElideRight }
                    Text { text: meaning; font.pixelSize: 12; color: "#AAA"; wrapMode: Text.WordWrap }
                }

                // Columna secundaria
                ColumnLayout {
                    spacing: 2
                    Layout.alignment: Qt.AlignRight
                    Text { text: state; font.pixelSize: 12; color: "#FF9800" }
                    Text { text: "Next: " + interval; font.pixelSize: 12; color: "#4CAF50" }
                    Text { text: "Reps: " + reps + " / Lapses: " + lapses; font.pixelSize: 12; color: "#9E9E9E" }
                }
            }
        }
    }
}
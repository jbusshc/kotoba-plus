import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Column {
    id: root

    property var entryData
    property string mode: "dictionary"

    property color textColor
    property color hintColor
    property color accentColor
    property color dividerColor

    width: parent ? parent.width : 400
    spacing: 18

    Repeater {
        model: (root.entryData.k_elements || []).length > 0
               ? root.entryData.k_elements
               : []

        delegate: Text {
            width: parent.width
            text: modelData.keb

            font.pixelSize: index === 0 ? 48 : 28
            font.bold: true

            color: root.textColor
            horizontalAlignment: Text.AlignHCenter
        }
    }

    Text {
        visible: (root.entryData.k_elements || []).length === 0 &&
                 (root.entryData.r_elements || []).length > 0

        width: parent.width
        text: ((root.entryData.r_elements || [])[0] || {}).reb || ""

        font.pixelSize: 48
        font.bold: true

        color: root.textColor
        horizontalAlignment: Text.AlignHCenter
    }

    Flow {
        visible: (root.entryData.k_elements || []).length > 0
        width: parent.width
        spacing: 8

        Repeater {
            model: root.entryData.r_elements || []

            delegate: Text {
                text: modelData.reb
                font.pixelSize: 20
                font.weight: Font.Medium
                color: root.hintColor
            }
        }
    }

    Rectangle {
        width: parent.width
        height: 1
        color: root.dividerColor
    }

    Repeater {
        model: root.entryData.senses || []

        delegate: SenseItem {
            sense: modelData
            senseIndex: index
            mode: root.mode

            textColor: root.textColor
            hintColor: root.hintColor
            accentColor: root.accentColor
            dividerColor: root.dividerColor
        }
    }

    Component.onCompleted: {
        console.log("ENTRY DATA:", JSON.stringify(entryData))
    }
}
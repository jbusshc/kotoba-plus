import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

Page {
    id: page
    property int entryId: -1

    property bool darkTheme: appConfig && appConfig.config ? appConfig.config.theme === "dark" : true
    property color textColor: darkTheme ? "white" : "black"
    property color hintColor: darkTheme ? "#B0BEC5" : "#757575"
    property color dividerColor: darkTheme ? "#424242" : "#E0E0E0"

    function formatGlosses(text) {
        if (!text) return ""
        var parts = text.split(";")
        for (var i = 0; i < parts.length; i++)
            parts[i] = "• " + parts[i].trim()
        return parts.join("\n")
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        // ── Top bar ─────────────────────────────
        RowLayout {
            Layout.fillWidth: true

            Button {
                text: "< Back"
                onClicked: stack.pop()
            }

            Item { Layout.fillWidth: true }
        }

        // ── Card container ──────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true

            radius: 8
            border.width: 1
            border.color: dividerColor
            color: darkTheme ? "#2b2b2b" : "white"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                // ── Palabra principal ─────────────
                Text {
                    id: heading
                    Layout.fillWidth: true
                    font.pixelSize: 26
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    color: textColor
                    wrapMode: Text.WordWrap
                }

                // ── Lecturas ──────────────────────
                Text {
                    id: readings
                    Layout.fillWidth: true
                    font.pixelSize: 15
                    horizontalAlignment: Text.AlignHCenter
                    color: hintColor
                    wrapMode: Text.WordWrap
                }

                // ── Stats SRS ─────────────────────
                RowLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 30

                    Repeater {
                        model: [
                            { label: "Reps",   value: entryId >= 0 && srsLibraryVM ? String(srsLibraryVM.getReps(entryId))   : "–" },
                            { label: "Lapses", value: entryId >= 0 && srsLibraryVM ? String(srsLibraryVM.getLapses(entryId)) : "–" },
                            { label: "Due",    value: entryId >= 0 && srsLibraryVM ? srsLibraryVM.getDue(entryId)            : "–" }
                        ]

                        delegate: Column {
                            spacing: 2

                            Text {
                                text: modelData.label
                                font.pixelSize: 11
                                color: hintColor
                            }

                            Text {
                                text: modelData.value
                                font.pixelSize: 16
                                font.bold: true
                                color: textColor
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: dividerColor
                }

                // ── Significados (scroll) ─────────
                ListView {
                    id: sensesList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 12
                    model: []

                    delegate: Column {
                        width: sensesList.width
                        spacing: 8

                        Text {
                            width: parent.width
                            wrapMode: Text.WordWrap
                            color: textColor
                            font.pixelSize: 14
                            text: page.formatGlosses(modelData)
                        }

                        Rectangle {
                            width: parent.width
                            height: 1
                            color: dividerColor
                            visible: index < sensesList.count - 1
                        }
                    }
                }

                // ── Acciones ──────────────────────
                RowLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 10

                    Button {
                        text: "Suspend"
                        onClicked: if (srsLibraryVM) srsLibraryVM.suspend(entryId)
                    }

                    Button {
                        text: "Reset"
                        onClicked: if (srsLibraryVM) srsLibraryVM.reset(entryId)
                    }

                    Button {
                        text: "Delete"
                        onClicked: {
                            if (srsLibraryVM) srsLibraryVM.remove(entryId)
                            stack.pop()
                        }
                    }

                    Button {
                        text: "View in Dictionary"
                        onClicked: stack.push("qrc:/qml/pages/DetailsPage.qml", { docId: entryId })
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        if (entryId >= 0 && detailsVM) {
            var map = detailsVM.buildDetails(entryId)
            heading.text     = map.mainWord || ""
            readings.text    = map.readings || ""
            sensesList.model = map.senses   || []
        }
    }

    onVisibleChanged: if (!visible) {
        heading.text     = ""
        readings.text    = ""
        sensesList.model = []
    }
}
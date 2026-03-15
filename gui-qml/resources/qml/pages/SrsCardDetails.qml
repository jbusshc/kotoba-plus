import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import Kotoba 1.0

Page {
    id: page
    property int entryId: -1
    property var entryData: ({})

    property bool darkTheme: Theme.darkTheme
    property color textColor: Theme.textColor
    property color hintColor: Theme.hintColor
    property color accentColor: Theme.accentColor
    property color dividerColor: Theme.dividerColor

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        // ── Fila superior: Back ────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true

            Button {
                text: "← Back"
                onClicked: stack.pop()
            }

            Item { Layout.fillWidth: true }
        }

        // ── Tarjeta principal ──────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true

            radius: 8
            border.width: 1
            border.color: dividerColor
            color: Theme.cardBackground

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                // ── Headword ───────────────────────────────────────────────
                // Si hay k_elements, mostramos el kanji; si no, el primer reading
                // es el headword y NO mostramos sección de readings aparte.
                Text {
                    id: heading
                    Layout.fillWidth: true
                    font.pixelSize: 36
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    color: textColor
                    wrapMode: Text.WordWrap
                }

                // ── Readings (solo si hay kanji) ───────────────────────────
                Flow {
                    id: readingsFlow
                    Layout.fillWidth: true
                    spacing: 8
                    visible: (entryData.k_elements || []).length > 0

                    Repeater {
                        model: entryData.r_elements || []
                        delegate: Text {
                            text: modelData.reb
                            font.pixelSize: 16
                            color: hintColor
                        }
                    }
                }

                // ── Stats SRS ──────────────────────────────────────────────
                RowLayout {
                    spacing: 24
                    Layout.fillWidth: true

                    Repeater {
                        model: [
                            { label: "Reps",   value: entryId >= 0 && srsLibraryVM ? String(srsLibraryVM.getReps(entryId))   : "–" },
                            { label: "Lapses", value: entryId >= 0 && srsLibraryVM ? String(srsLibraryVM.getLapses(entryId)) : "–" },
                            { label: "Due",    value: entryId >= 0 && srsLibraryVM ? srsLibraryVM.getDue(entryId)            : "–" }
                        ]
                        delegate: Column {
                            required property var modelData
                            spacing: 2
                            Text { text: modelData.label; font.pixelSize: 11; color: hintColor }
                            Text { text: modelData.value; font.pixelSize: 15; font.bold: true; color: textColor }
                        }
                    }

                    Item { Layout.fillWidth: true }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: dividerColor }

                // ── Senses ─────────────────────────────────────────────────
                ListView {
                    id: sensesList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 14
                    model: entryData.senses || []

                    delegate: Column {
                        width: sensesList.width
                        spacing: 6

                        // Número + POS
                        Row {
                            spacing: 8
                            Text {
                                text: (index + 1) + "."
                                font.bold: true
                                font.pixelSize: 13
                                color: textColor
                            }
                            Text {
                                text: (modelData.pos || []).join(", ")
                                font.pixelSize: 12
                                color: hintColor
                            }
                        }

                        // Glosses
                        Repeater {
                            model: modelData.gloss || []
                            delegate: Text {
                                width: sensesList.width
                                wrapMode: Text.WordWrap
                                text: "• " + modelData
                                color: textColor
                                font.pixelSize: 14
                            }
                        }

                        // Field
                        Text {
                            visible: (modelData.field || []).length > 0
                            text: "[" + (modelData.field || []).join(", ") + "]"
                            color: accentColor
                            font.pixelSize: 12
                        }

                        // Misc
                        Text {
                            visible: (modelData.misc || []).length > 0
                            text: (modelData.misc || []).join(", ")
                            color: hintColor
                            font.pixelSize: 12
                        }

                        // Dialect
                        Text {
                            visible: (modelData.dial || []).length > 0
                            text: "(" + (modelData.dial || []).join(", ") + " dialect)"
                            color: hintColor
                            font.pixelSize: 12
                        }

                        // Sense info
                        Text {
                            visible: (modelData.s_inf || []).length > 0
                            text: (modelData.s_inf || []).join("; ")
                            color: hintColor
                            font.pixelSize: 12
                            font.italic: true
                        }

                        // Xref
                        Text {
                            visible: (modelData.xref || []).length > 0
                            text: "See also: " + (modelData.xref || []).join(", ")
                            color: accentColor
                            font.pixelSize: 12
                        }

                        // Antonym
                        Text {
                            visible: (modelData.ant || []).length > 0
                            text: "Antonym: " + (modelData.ant || []).join(", ")
                            color: accentColor
                            font.pixelSize: 12
                        }

                        Rectangle {
                            width: sensesList.width
                            height: 1
                            color: dividerColor
                            visible: index < sensesList.count - 1
                        }
                    }
                }
            }
        }

        // ── Acciones ───────────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true

            Item { Layout.fillWidth: true }

            Flow {
                spacing: 8

                Button {
                    text: "Suspend"
                    onClicked: { if (srsLibraryVM) srsLibraryVM.suspend(entryId) }
                }

                Button {
                    text: "Reset"
                    onClicked: { if (srsLibraryVM) srsLibraryVM.reset(entryId) }
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

            Item { Layout.fillWidth: true }
        }
    }

    Component.onCompleted: {
        if (entryId >= 0 && detailsVM) {
            entryData = detailsVM.mapEntry(entryId)

            var kElems = entryData.k_elements || []
            var rElems = entryData.r_elements || []

            if (kElems.length > 0) {
                // Hay kanji: headword = kanji, readings se muestran debajo
                heading.text = kElems[0].keb
            } else if (rElems.length > 0) {
                // Sin kanji: headword = primer reading, no hay sección readings
                heading.text = rElems[0].reb
            }
        }
    }
}
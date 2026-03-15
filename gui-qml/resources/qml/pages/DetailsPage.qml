import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

import Kotoba 1.0

Page {
    id: page
    property int docId: -1
    property var entryData: ({})

    property bool darkTheme: Theme.darkTheme
    property color textColor: Theme.textColor
    property color hintColor: Theme.hintColor
    property color accentColor: Theme.accentColor
    property color dividerColor: Theme.dividerColor

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 14

        RowLayout {
            Layout.fillWidth: true

            Button {
                text: "← Back"
                onClicked: stack.pop()
            }

            Item { Layout.fillWidth: true }

            Button {
                id: addButton
                text: srsVM.contains(docId) ? "In SRS" : "Add to SRS"
                enabled: !srsVM.contains(docId)
                onClicked: {
                    srsVM.add(docId)
                    enabled = false
                    text = "In SRS"
                }
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Column {
                width: parent.width
                spacing: 18

                // ── Headword ───────────────────────────────────────────────
                // Si hay k_elements → mostrar todos los kanji
                // Si no              → el headword es el primer reading
                Repeater {
                    model: (entryData.k_elements || []).length > 0
                           ? entryData.k_elements || []
                           : []
                    delegate: Text {
                        width: parent.width
                        text: modelData.keb
                        font.pixelSize: 42
                        font.bold: true
                        color: textColor
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                // Headword cuando no hay kanji (primer reading como título)
                Text {
                    visible: (entryData.k_elements || []).length === 0 &&
                             (entryData.r_elements || []).length > 0
                    width: parent.width
                    text: ((entryData.r_elements || [])[0] || {}).reb || ""
                    font.pixelSize: 42
                    font.bold: true
                    color: textColor
                    horizontalAlignment: Text.AlignHCenter
                }

                // ── Readings (solo si hay kanji) ───────────────────────────
                Flow {
                    visible: (entryData.k_elements || []).length > 0
                    width: parent.width
                    spacing: 8

                    Repeater {
                        model: entryData.r_elements || []
                        delegate: Text {
                            text: modelData.reb
                            font.pixelSize: 18
                            color: hintColor
                        }
                    }
                }

                // Readings adicionales cuando no hay kanji (desde el 2º)
                Flow {
                    visible: (entryData.k_elements || []).length === 0 &&
                             (entryData.r_elements || []).length > 1
                    width: parent.width
                    spacing: 8

                    Repeater {
                        // slice(1) no disponible en QML directamente, usamos index
                        model: (entryData.r_elements || []).length > 1
                               ? (entryData.r_elements || []).slice(1)
                               : []
                        delegate: Text {
                            text: modelData.reb
                            font.pixelSize: 16
                            color: hintColor
                        }
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: dividerColor
                }

                // ── Senses ─────────────────────────────────────────────────
                Repeater {
                    model: entryData.senses || []

                    delegate: Column {
                        width: parent.width
                        spacing: 8

                        // Número + POS
                        Row {
                            spacing: 10
                            Text {
                                text: (index + 1) + "."
                                font.bold: true
                                color: textColor
                            }
                            Text {
                                text: (modelData.pos || []).join(", ")
                                color: hintColor
                                font.pixelSize: 13
                            }
                        }

                        // Glosses
                        Repeater {
                            model: modelData.gloss || []
                            delegate: Text {
                                width: parent.width
                                wrapMode: Text.WordWrap
                                text: "• " + modelData
                                color: textColor
                            }
                        }

                        // Field
                        Text {
                            visible: (modelData.field || []).length > 0
                            text: "[" + (modelData.field || []).join(", ") + "]"
                            color: accentColor
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
                        }

                        // Sense info (s_inf) — faltaba antes
                        Text {
                            visible: (modelData.s_inf || []).length > 0
                            text: (modelData.s_inf || []).join("; ")
                            color: hintColor
                            font.pixelSize: 12
                            font.italic: true
                        }

                        // Lsource — faltaba antes
                        Text {
                            visible: (modelData.lsource || []).length > 0
                            text: "From: " + (modelData.lsource || []).join(", ")
                            color: hintColor
                            font.pixelSize: 12
                            font.italic: true
                        }

                        // Xref
                        Text {
                            visible: (modelData.xref || []).length > 0
                            text: "See also: " + (modelData.xref || []).join(", ")
                            color: accentColor
                        }

                        // Antonym
                        Text {
                            visible: (modelData.ant || []).length > 0
                            text: "Antonym: " + (modelData.ant || []).join(", ")
                            color: accentColor
                        }

                        Rectangle {
                            width: parent.width
                            height: 1
                            color: dividerColor
                        }
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        if (docId !== -1) {
            entryData = detailsVM.mapEntry(docId)
        }
    }
}
import QtQuick
import QtQuick.Controls
import "../components"   // 👈 importa Tag.qml

Column {
    id: root

    property var sense
    property int senseIndex: 0
    property string mode: "dictionary"

    // 🔥 fallback a Theme (mejor práctica)
    property color textColor: Theme.textColor
    property color hintColor: Theme.hintColor
    property color accentColor: Theme.accentColor
    property color dividerColor: Theme.dividerColor

    width: parent ? parent.width : 400
    spacing: 6

    /* -------------------------
     * HEADER
     * ------------------------- */
    Row {
        spacing: 8

        Text {
            text: (root.senseIndex + 1) + "."
            font.bold: true
            font.pixelSize: 14
            color: root.textColor
        }

        // POS → TAGS
        Flow {
            visible: (root.sense.pos || []).length > 0
            spacing: 4

            Repeater {
                model: root.sense.pos || []

                delegate: Tag {
                    label: modelData
                    color: root.hintColor
                }
            }
        }

        // FIELD → TAGS
        Flow {
            visible: root.mode === "dictionary" &&
                     (root.sense.field || []).length > 0
            spacing: 4

            Repeater {
                model: root.sense.field || []

                delegate: Tag {
                    label: modelData
                    color: root.accentColor
                }
            }
        }
    }

    /* -------------------------
     * STAGK / STAGR
     * ------------------------- */
    Flow {
        visible: root.mode === "dictionary" &&
                 (root.sense.stagk || []).length > 0
        spacing: 4

        Repeater {
            model: root.sense.stagk || []

            delegate: Tag {
                label: modelData
                color: root.hintColor
            }
        }
    }

    Flow {
        visible: root.mode === "dictionary" &&
                 (root.sense.stagr || []).length > 0
        spacing: 4

        Repeater {
            model: root.sense.stagr || []

            delegate: Tag {
                label: modelData
                color: root.hintColor
            }
        }
    }

    /* -------------------------
     * GLOSS
     * ------------------------- */
    Repeater {
        model: root.sense.gloss || []

        delegate: Text {
            width: parent.width
            wrapMode: Text.WordWrap
            text: "• " + modelData
            font.pixelSize: 15
            color: root.textColor
        }
    }

    /* -------------------------
     * EXTRA INFO (tags)
     * ------------------------- */
    Column {
        visible: root.mode === "dictionary"
        spacing: 4

        // MISC
        Flow {
            visible: (root.sense.misc || []).length > 0
            spacing: 4

            Repeater {
                model: root.sense.misc || []

                delegate: Tag {
                    label: modelData
                    color: root.hintColor
                }
            }
        }

        // DIALECT
        Flow {
            visible: (root.sense.dial || []).length > 0
            spacing: 4

            Repeater {
                model: root.sense.dial || []

                delegate: Tag {
                    label: modelData
                    color: root.hintColor
                }
            }
        }

        // S_INF (esto queda mejor como texto)
        Text {
            visible: (root.sense.s_inf || []).length > 0
            text: (root.sense.s_inf || []).join("; ")
            font.pixelSize: 12
            font.italic: true
            color: root.hintColor
        }

        // LSOURCE
        Text {
            visible: (root.sense.lsource || []).length > 0
            text: "From: " + (root.sense.lsource || []).join(", ")
            font.pixelSize: 12
            font.italic: true
            color: root.hintColor
        }
    }

    /* -------------------------
     * XREF (CLICKABLE TAGS)
     * ------------------------- */
    Column {
        visible: (root.sense.xref || []).length > 0
        spacing: 2

        Text {
            text: "See also:"
            font.pixelSize: 12
            color: root.accentColor
        }

        Flow {
            width: parent.width
            spacing: 6

            Repeater {
                model: root.sense.xref || []

                delegate: Tag {
                    label: modelData.label
                    color: root.accentColor

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        hoverEnabled: true

                        onClicked: {
                            stack.push("pages/DetailsPage.qml", {
                                docId: modelData.id
                            })
                        }

                        onEntered: parent.opacity = 0.6
                        onExited: parent.opacity = 1.0
                    }
                }
            }
        }
    }

    /* -------------------------
     * ANT (CLICKABLE TAGS)
     * ------------------------- */
    Column {
        visible: (root.sense.ant || []).length > 0
        spacing: 2

        Text {
            text: "Antonym:"
            font.pixelSize: 12
            color: root.hintColor
        }

        Flow {
            width: parent.width
            spacing: 6

            Repeater {
                model: root.sense.ant || []

                delegate: Tag {
                    label: modelData.label
                    color: root.hintColor

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        hoverEnabled: true

                        onClicked: {
                            stack.push("pages/DetailsPage.qml", {
                                docId: modelData.id
                            })
                        }

                        onEntered: parent.opacity = 0.6
                        onExited: parent.opacity = 1.0
                    }
                }
            }
        }
    }

    /* -------------------------
     * DIVIDER
     * ------------------------- */
    Rectangle {
        width: parent.width
        height: 1
        color: root.dividerColor
        opacity: 0.5
    }
}
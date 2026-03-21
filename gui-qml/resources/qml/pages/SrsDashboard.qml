import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Kotoba 1.0

Page {
    id: page
    padding: 0

    property bool darkTheme: Theme.darkTheme
    property color textColor: Theme.textColor
    property color hintColor: Theme.hintColor
    property color accentColor: Theme.accentColor
    property color dividerColor: Theme.dividerColor

    background: Rectangle { color: Theme.background }

    // ── Stat card component ──────────────────────────────────────────────────
    component StatCard: Rectangle {
        property string label: ""
        property string value: "0"
        property color  accent: "white"
        property string sublabel: ""

        radius: 10
        color: Theme.cardBackground
        border.width: 1
        border.color: Qt.rgba(accent.r, accent.g, accent.b, 0.25)

        Layout.fillWidth: true
        Layout.preferredHeight: 110

        // Top accent line
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 2
            radius: 1
            color: accent
            opacity: 0.7
        }

        Column {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 6

            Text {
                text: value
                font.pixelSize: Theme.fontSizeHero
                font.weight: Font.Bold
                font.letterSpacing: -1
                color: accent
            }

            Text {
                text: label.toUpperCase()
                font.pixelSize: Theme.fontSizeSmall
                font.weight: Font.Medium
                font.letterSpacing: 0.6
                color: Qt.rgba(hintColor.r, hintColor.g, hintColor.b, 0.7)
            }
        }
    }

    // ── Action button component ──────────────────────────────────────────────
    component ActionButton: Rectangle {
        property string label: ""
        property string sublabel: ""
        property color  accent: accentColor
        property bool   primary: false
        property bool   enabled: true
        signal clicked()

        height: 60
        radius: 8
        color: primary
            ? (btnMouse.containsMouse && enabled ? Qt.lighter(accent, 1.15) : accent)
            : (btnMouse.containsMouse && enabled ? Qt.rgba(1,1,1,0.08) : Qt.rgba(1,1,1,0.04))

        border.width: primary ? 0 : 1
        border.color: Qt.rgba(1,1,1,0.10)

        opacity: enabled ? 1.0 : 0.35

        Behavior on color { ColorAnimation { duration: 120 } }

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 2

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: label
                font.pixelSize: Theme.fontSizeBody
                font.weight: Font.DemiBold
                color: primary ? "white" : textColor
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: sublabel
                font.pixelSize: Theme.fontSizeTiny
                color: primary ? Qt.rgba(1,1,1,0.65) : hintColor
                visible: sublabel.length > 0
            }
        }

        MouseArea {
            id: btnMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
            onClicked: if (parent.enabled) parent.clicked()
        }
    }

    // ── Layout ───────────────────────────────────────────────────────────────
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 24

            Item { Layout.fillHeight: true }

        // Header
        Column {
            Layout.fillWidth: true
            spacing: 4

            Text {
                text: "Study Queue"
                font.pixelSize: Theme.fontSizeTitle
                font.weight: Font.Bold
                color: textColor
            }

            Text {
                text: {
                    if (!srsVM) return ""
                    const due = srsVM.dueCount
                    if (due === 0) return "You're all caught up for now."
                    return due + " card" + (due === 1 ? "" : "s") + " ready to review."
                }
                font.pixelSize: Theme.fontSizeBody
                color: hintColor
            }
        }

        // Stats row
        GridLayout {
            Layout.fillWidth: true
            columns: 3
            columnSpacing: 12
            rowSpacing: 12

            StatCard {
                label: "Due"
                value: srsVM ? String(srsVM.dueCount ?? 0) : "0"
                accent: "#F87171"
            }

            StatCard {
                label: "Total"
                value: srsVM ? String(srsVM.totalCount ?? 0) : "0"
                accent: "#60A5FA"
            }

            StatCard {
                label: "Today"
                value: srsVM ? String(srsVM.reviewTodayCount ?? 0) : "0"
                accent: "#34D399"
            }
        }

        // Action buttons
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 10

            ActionButton {
                Layout.fillWidth: true
                primary: true
                label: srsVM && srsVM.dueCount > 0 ? "Start Study Session" : "Nothing Due"
                sublabel: srsVM && srsVM.dueCount > 0 ? srsVM.dueCount + " cards in queue" : "Check back later"
                accent: accentColor
                enabled: srsVM && srsVM.dueCount > 0

                onClicked: {
                    if (stack && srsVM && srsVM.dueCount > 0) {
                        srsVM.startSession()
                        stack.push("qrc:/qml/pages/SrsStudy.qml")
                    }
                }
            }

            ActionButton {
                Layout.fillWidth: true
                label: "Browse Cards"
                sublabel: srsVM ? ((parseInt(srsVM.totalCount) || 0) + " cards in deck") : ""
                enabled: true

                onClicked: {
                    if (stack) stack.push("qrc:/qml/pages/SrsLibrary.qml")
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}

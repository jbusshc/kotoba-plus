import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Kotoba 1.0

Page {
    id: page
    padding: 0

    property color textColor:    Theme.textColor
    property color hintColor:    Theme.hintColor
    property color accentColor:  Theme.accentColor
    property color dividerColor: Theme.dividerColor

    background: Rectangle { color: Theme.background }

    // ── Stat card ─────────────────────────────────────────────────────────────
    component StatCard: Rectangle {
        property string label:  ""
        property string value:  "0"
        property color  accent: Theme.accentColor

        Layout.fillWidth: true
        implicitHeight: 96
        radius: 10
        color: Theme.cardBackground
        border.width: 1
        border.color: Qt.rgba(accent.r, accent.g, accent.b, 0.22)

        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left; anchors.right: parent.right
            height: 2; radius: 1
            color: parent.accent; opacity: 0.75
        }

        Column {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 5

            Text {
                text: value
                font.pixelSize: Theme.fontSizeHero
                font.weight: Font.Bold
                font.letterSpacing: -1
                color: accent
            }
            Text {
                text: label.toUpperCase()
                font.pixelSize: Theme.fontSizeXSmall
                font.weight: Font.Medium
                font.letterSpacing: 0.8
                color: Qt.rgba(hintColor.r, hintColor.g, hintColor.b, 0.65)
            }
        }
    }

    // ── Action button ─────────────────────────────────────────────────────────
    component ActionButton: Rectangle {
        id: actionBtn
        property string label:    ""
        property string sublabel: ""
        property color  accent:   accentColor
        property bool   primary:  false
        property bool   enabled:  true
        signal clicked()

        implicitHeight: 60
        radius: 8
        opacity: actionBtn.enabled ? 1.0 : 0.35

        color: actionBtn.primary
            ? (btnMa.pressed && actionBtn.enabled
                ? Qt.darker(actionBtn.accent, 1.08)
                : btnMa.containsMouse && actionBtn.enabled
                    ? Qt.lighter(actionBtn.accent, 1.12)
                    : actionBtn.accent)
            : (btnMa.pressed && actionBtn.enabled
                ? Theme.surfacePress
                : btnMa.containsMouse && actionBtn.enabled
                    ? Theme.surfaceHover
                    : Theme.surfaceSubtle)

        border.width: actionBtn.primary ? 0 : 1
        border.color: Theme.surfaceBorder

        Behavior on color { ColorAnimation { duration: 120 } }

        Column {
            anchors.centerIn: parent
            spacing: 3

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: actionBtn.label
                font.pixelSize: Theme.fontSizeBody
                font.weight: Font.DemiBold
                color: actionBtn.primary ? "white" : textColor
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: actionBtn.sublabel
                font.pixelSize: Theme.fontSizeTiny
                color: actionBtn.primary ? Qt.rgba(1, 1, 1, 0.65) : hintColor
                visible: actionBtn.sublabel.length > 0
            }
        }

        MouseArea {
            id: btnMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: actionBtn.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
            onClicked: if (actionBtn.enabled) actionBtn.clicked()
        }
    }

    // ── Layout ────────────────────────────────────────────────────────────────
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 0

        Item { Layout.fillHeight: true }

        // Header
        Column {
            Layout.fillWidth: true
            Layout.bottomMargin: 24
            spacing: 4

            Text {
                text: "Study Session"
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

        // Stats row: Due | New | Reviewed today
        GridLayout {
            Layout.fillWidth: true
            Layout.bottomMargin: 20
            columns: 3
            columnSpacing: 10
            rowSpacing: 10

            StatCard {
                label:  "Due"
                value:  srsVM ? String(srsVM.dueCount        ?? 0) : "0"
                accent: "#F87171"
            }
            StatCard {
                // Rename srsVM.newCount to match your VM property if needed
                label:  "New"
                value:  srsVM ? String(srsVM.newCount        ?? 0) : "0"
                accent: "#60A5FA"
            }
            StatCard {
                label:  "Reviewed"
                value:  srsVM ? String(srsVM.reviewTodayCount ?? 0) : "0"
                accent: "#34D399"
            }
        }

        // Buttons
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 10

            ActionButton {
                Layout.fillWidth: true
                primary:  true
                label:    srsVM && srsVM.dueCount > 0 ? "Start Study Session" : "Nothing Due"
                sublabel: srsVM && srsVM.dueCount > 0
                    ? srsVM.dueCount + " card" + (srsVM.dueCount === 1 ? "" : "s") + " in queue"
                    : "Check back later"
                accent:  accentColor
                enabled: srsVM ? srsVM.dueCount > 0 : false
                onClicked: {
                    if (stack && srsVM && srsVM.dueCount > 0) {
                        srsVM.startSession()
                        stack.push("qrc:/qml/pages/SrsStudy.qml")
                    }
                }
            }

            ActionButton {
                Layout.fillWidth: true
                label:    "Browse Cards"
                sublabel: srsVM ? (String(parseInt(srsVM.totalCount) || 0) + " cards in deck") : ""
                enabled:  true
                onClicked: { if (stack) stack.push("qrc:/qml/pages/SrsLibrary.qml") }
            }
        }

        Item { Layout.fillHeight: true }
    }
}

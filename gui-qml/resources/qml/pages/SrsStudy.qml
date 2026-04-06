import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import "../components"

Page {
    id: page
    padding: 0

    property bool answerShown:  false
    property bool contentReady: false

    background: Rectangle { color: Theme.background }

    Component.onCompleted: {
        if (srsVM) srsVM.startSession()
        page.contentReady = true
    }

    Connections {
        target: srsVM
        function onCurrentChanged() {
            page.contentReady = false
            page.answerShown  = false
            Qt.callLater(() => { page.contentReady = true })
        }
        function onNoMoreCards() {
            if (srsVM) srsVM.saveProfile()
            stack.pop()
        }
    }

    // ── Rating button ─────────────────────────────────────────────────────────
    component RatingButton: Rectangle {
        property string label:    ""
        property string interval: ""
        property color  accent:   "white"
        signal clicked()

        radius: 8
        color: ma.pressed
            ? Qt.rgba(accent.r, accent.g, accent.b, 0.30)
            : ma.containsMouse
                ? Qt.rgba(accent.r, accent.g, accent.b, 0.22)
                : Qt.rgba(accent.r, accent.g, accent.b, 0.12)
        border.width: 1
        border.color: Qt.rgba(accent.r, accent.g, accent.b, 0.40)
        Behavior on color { ColorAnimation { duration: 100 } }

        Column {
            anchors.centerIn: parent
            spacing: 3
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text:           label
                font.pixelSize: Theme.fontSizeBody
                font.weight:    Font.DemiBold
                color:          accent
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text:           interval
                font.pixelSize: Theme.fontSizeTiny
                color: Qt.rgba(accent.r, accent.g, accent.b, 0.65)
                visible: interval.length > 0
            }
        }

        MouseArea {
            id: ma; anchors.fill: parent
            hoverEnabled: true; cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
        }
    }

    // ── Root ──────────────────────────────────────────────────────────────────
    Item {
        anchors { fill: parent; margins: 24 }

        // ── Top bar ───────────────────────────────────────────────────────────
        RowLayout {
            id: topBar
            anchors { top: parent.top; left: parent.left; right: parent.right }
            height: Theme.minTapTarget

            BackButton { onClicked: stack.pop() }

            Item { Layout.fillWidth: true }

            // Card-type badge — visual distintivo de Recognition vs Recall
            Rectangle {
                visible: srsVM && srsVM.hasCard
                implicitHeight: Math.max(Theme.fontSizeXSmall * 2.0, 22)
                implicitWidth:  typeBadgeTxt.implicitWidth + Theme.fontSizeXSmall * 1.2
                radius: 5
                color: {
                    const t = srsVM ? srsVM.currentCardType : "Recognition"
                    return t === "Recall"
                        ? Qt.rgba(0.96, 0.62, 0.25, 0.18)
                        : Qt.rgba(Theme.accentColor.r, Theme.accentColor.g, Theme.accentColor.b, 0.15)
                }
                border.width: 1
                border.color: {
                    const t = srsVM ? srsVM.currentCardType : "Recognition"
                    return t === "Recall"
                        ? Qt.rgba(0.96, 0.62, 0.25, 0.55)
                        : Qt.rgba(Theme.accentColor.r, Theme.accentColor.g, Theme.accentColor.b, 0.45)
                }

                Text {
                    id: typeBadgeTxt
                    anchors.centerIn: parent
                    text: srsVM ? srsVM.currentCardType : ""
                    font.pixelSize: Theme.fontSizeXSmall
                    font.weight:    Font.Medium
                    color: {
                        const t = srsVM ? srsVM.currentCardType : "Recognition"
                        return t === "Recall" ? Qt.rgba(0.96, 0.62, 0.25, 1) : Theme.accentColor
                    }
                }
            }

            // Undo button
            Item {
                width:   Math.max(undoLbl.implicitWidth + 24, Theme.minTapTarget)
                height:  Theme.minTapTarget
                visible: srsVM && srsVM.canUndo && !page.answerShown

                Rectangle {
                    anchors.fill: parent; radius: 6
                    color: undoMa.pressed
                        ? Theme.surfacePress
                        : undoMa.containsMouse ? Theme.surfaceHover : "transparent"
                    Behavior on color { ColorAnimation { duration: 120 } }
                }
                RowLayout {
                    anchors.centerIn: parent; spacing: 4
                    Text { text: "↩"; font.pixelSize: Theme.fontSizeSmall; color: Theme.hintColor }
                    Text {
                        id: undoLbl; text: "Undo"
                        font.pixelSize: Theme.fontSizeSmall; font.weight: Font.Medium
                        color: Theme.hintColor
                    }
                }
                MouseArea {
                    id: undoMa; anchors.fill: parent
                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: if (srsVM) srsVM.undoLastAnswer()
                }
            }
        }

        // ── Prompt hint (encima de la card) ───────────────────────────────────
        // Le recuerda al user qué se espera que recuerde según el tipo de carta
        Text {
            id: promptHint
            anchors { top: topBar.bottom; left: parent.left; right: parent.right }
            anchors.topMargin: 8
            height: visible ? implicitHeight : 0
            visible: srsVM && srsVM.hasCard

            text: {
                if (!srsVM) return ""
                return srsVM.currentCardType === "Recall"
                    ? "Write / recall the Japanese word"
                    : "Recall the reading and meaning"
            }
            font.pixelSize: Theme.fontSizeXSmall
            color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.5)
            horizontalAlignment: Text.AlignHCenter
        }

        // ── Flash card ────────────────────────────────────────────────────────
        Item {
            anchors {
                top:    promptHint.bottom
                bottom: actionBar.top
                left:   parent.left
                right:  parent.right
            }
            anchors.topMargin:    8
            anchors.bottomMargin: 16

            Rectangle {
                anchors.centerIn: parent
                width:  Math.min(620, parent.width)
                height: Math.min(parent.height, cardScroll.contentHeight + 56)
                Behavior on height { NumberAnimation { duration: 180; easing.type: Easing.OutQuad } }
                radius: 12
                color:  Theme.cardBackground
                border.width: 1; border.color: Theme.dividerColor

                // Borde superior: distinto color para Recall vs Recognition
                Rectangle {
                    anchors { top: parent.top; left: parent.left; right: parent.right }
                    height: 2; radius: 1
                    color: (srsVM && srsVM.currentCardType === "Recall")
                        ? Qt.rgba(0.96, 0.62, 0.25, 0.8)
                        : Theme.accentColor
                    opacity: 0.7
                    Behavior on color { ColorAnimation { duration: 200 } }
                }

                ScrollView {
                    id: cardScroll
                    anchors { fill: parent; margins: 28 }
                    clip: true
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                    opacity: page.contentReady ? 1.0 : 0.0
                    Behavior on opacity { NumberAnimation { duration: 120 } }

                    Loader {
                        anchors.fill: parent
                        active: page.contentReady

                        sourceComponent: EntryView {
                            width:     parent.width
                            entryData: srsVM ? srsVM.currentEntryData : ({})
                            // Pasamos el tipo de carta para que EntryView sepa
                            // qué campo ocultar antes de revelar la respuesta.
                            // "srs_recognition" → oculta gloss hasta revelar
                            // "srs_recall"      → oculta headword/lectura hasta revelar
                            mode:     srsVM
                                ? (srsVM.currentCardType === "Recall" ? "srs_recall" : "srs_recognition")
                                : "srs_recognition"
                            revealed: page.answerShown
                            onNavigateTo: (url, props) => stack.push(url, props)
                        }
                    }
                }
            }
        }

        // ── Action bar ────────────────────────────────────────────────────────
        Item {
            id: actionBar
            anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
            height: Math.max(Theme.minTapTarget + Theme.fontSizeBase, 56)

            Rectangle {
                anchors.fill: parent
                visible: !page.answerShown
                radius: 8

                readonly property color _btnAccent: (srsVM && srsVM.currentCardType === "Recall")
                    ? Qt.rgba(0.96, 0.62, 0.25, 1)
                    : Theme.accentColor

                color: showMa.pressed
                    ? Qt.rgba(_btnAccent.r, _btnAccent.g, _btnAccent.b, 0.24)
                    : showMa.containsMouse
                        ? Qt.rgba(_btnAccent.r, _btnAccent.g, _btnAccent.b, 0.18)
                        : Qt.rgba(_btnAccent.r, _btnAccent.g, _btnAccent.b, 0.10)
                border.width: 1
                border.color: Qt.rgba(_btnAccent.r, _btnAccent.g, _btnAccent.b, 0.35)
                enabled: srsVM && srsVM.hasCard
                opacity: enabled ? 1.0 : 0.4
                Behavior on color { ColorAnimation { duration: 120 } }

                Text {
                    anchors.centerIn: parent
                    text:           "Show Answer"
                    font.pixelSize: Theme.fontSizeBase; font.weight: Font.Medium
                    color: parent._btnAccent
                }
                MouseArea {
                    id: showMa; anchors.fill: parent
                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: page.answerShown = true
                }
            }

            RowLayout {
                anchors.fill: parent
                spacing: 8
                visible: page.answerShown

                RatingButton {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    label: "Again"; interval: srsVM ? srsVM.againInterval : ""
                    accent: Theme.againColor
                    onClicked: if (srsVM) srsVM.answerAgain()
                }
                RatingButton {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    label: "Hard"; interval: srsVM ? srsVM.hardInterval : ""
                    accent: Theme.hardColor
                    onClicked: if (srsVM) srsVM.answerHard()
                }
                RatingButton {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    label: "Good"; interval: srsVM ? srsVM.goodInterval : ""
                    accent: Theme.goodColor
                    onClicked: if (srsVM) srsVM.answerGood()
                }
                RatingButton {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    label: "Easy"; interval: srsVM ? srsVM.easyInterval : ""
                    accent: Theme.easyColor
                    onClicked: if (srsVM) srsVM.answerEasy()
                }
            }
        }
    }
}
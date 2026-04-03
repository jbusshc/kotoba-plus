import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Effects

import "../components"
import "../theme"

Page {
    id: page
    padding: 0

    background: Rectangle { color: Theme.background }

    // ── Layout constants ──────────────────────────────────────────────────────
    readonly property int rowH:  56
    readonly property int ctrlH: 32
    readonly property int ctrlW: 220
    readonly property int unitW: 52
    readonly property int secGap: 24

    // ── Dirty detection ───────────────────────────────────────────────────────
    property bool ready:    false
    property bool dirty:    false
    property var  snapshot: ({})

    function takeSnapshot() {
        snapshot = {
            theme:             appConfig.theme,
            accentColor:       appConfig.accentColor,
            fontScale:         appConfig.fontScale,
            glossLanguages:    appConfig.glossLanguages,
            fallbackLanguage:  appConfig.fallbackLanguage,
            interfaceLanguage: appConfig.interfaceLanguage,
            searchOnTyping:    appConfig.searchOnTyping,
            searchDelayMs:     appConfig.searchDelayMs,
            newCardsPerDay:    appConfig.newCardsPerDay,
            reviewsPerDay:     appConfig.reviewsPerDay,
            desiredRetention:  appConfig.desiredRetention,
            maximumInterval:   appConfig.maximumInterval,
            leechThreshold:    appConfig.leechThreshold,
            dayOffset:         appConfig.dayOffset,
            enableFuzz:        appConfig.enableFuzz,
            orderMode:         appConfig.orderMode,
            showRomaji:        appConfig.showRomaji,
            maxResults:        appConfig.maxResults,
            pageSize:          appConfig.pageSize,
        }
    }

    function _snapEq(key) {
        const a = snapshot[key], b = appConfig[key]
        if (typeof a === "number" && typeof b === "number")
            return Math.abs(a - b) <= 0.001
        return a === b
    }

    function checkDirty() {
        if (!ready || Object.keys(snapshot).length === 0) { dirty = false; return }
        dirty = !["theme","accentColor","fontScale","glossLanguages","fallbackLanguage",
                  "interfaceLanguage","searchOnTyping","searchDelayMs","newCardsPerDay",
                  "reviewsPerDay","desiredRetention","maximumInterval","leechThreshold",
                  "dayOffset","enableFuzz","orderMode","showRomaji","maxResults","pageSize"]
                 .every(k => _snapEq(k))
    }

    function markDirty() { if (ready) checkDirty() }

    function applySettings() {
        appConfig.saveToDisk()
        takeSnapshot()
        dirty = false
        appConfig.applyToServices()
    }

    function resetToDefaults() {
        appConfig.theme             = "dark"
        appConfig.accentColor       = "blue"
        appConfig.fontScale         = 1.0
        appConfig.glossLanguages    = "en"
        appConfig.fallbackLanguage  = "en"
        appConfig.interfaceLanguage = "en"
        appConfig.searchOnTyping    = true
        appConfig.searchDelayMs     = 150
        appConfig.newCardsPerDay    = 20
        appConfig.reviewsPerDay     = 200
        appConfig.desiredRetention  = 0.90
        appConfig.maximumInterval   = 36500
        appConfig.leechThreshold    = 8
        appConfig.dayOffset         = 14400
        appConfig.enableFuzz        = true
        appConfig.orderMode         = 0
        appConfig.showRomaji        = false
        appConfig.maxResults        = 20000
        appConfig.pageSize          = 20
        checkDirty()
        applySettings()
    }

    Timer {
        id: initTimer; interval: 150
        onTriggered: { takeSnapshot(); page.ready = true }
    }

    Component.onCompleted: initTimer.start()

    onVisibleChanged: {
        if (visible) {
            page.ready = false; page.dirty = false
            initTimer.restart()
        }
    }

    // ═════════════════════════════════════════════════════════════════════════
    // INLINE COMPONENTS  (settings-page–specific, not needed elsewhere)
    // ═════════════════════════════════════════════════════════════════════════

    component InfoBadge: Item {
        id: badge
        property string tip: ""
        implicitWidth: Theme.minTapTarget; implicitHeight: Theme.minTapTarget

        Rectangle {
            anchors.centerIn: parent
            width: 18; height: 18; radius: 9
            color: badgeMa.containsMouse
                ? Qt.rgba(Theme.accentColor.r, Theme.accentColor.g, Theme.accentColor.b, 0.25)
                : Theme.surfaceSubtle
            border.width: 1; border.color: Theme.surfaceBorder
            Behavior on color { ColorAnimation { duration: 120 } }

            Text {
                anchors.centerIn: parent
                text: "?"
                font.pixelSize: Theme.fontSizeXSmall
                color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.7)
            }
        }

        Rectangle {
            visible: badgeMa.containsMouse
            z: 999
            width:  Math.min(tipTxt.implicitWidth + 20, 260)
            height: tipTxt.implicitHeight + 14
            radius: 6
            color:  Theme.cardBackground
            border.width: 1; border.color: Theme.dividerColor
            anchors { right: parent.right; bottom: parent.top; bottomMargin: 6 }

            Text {
                id: tipTxt
                anchors { fill: parent; margins: 10 }
                text:           badge.tip
                font.pixelSize: Theme.fontSizeXSmall
                color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.9)
                wrapMode: Text.Wrap
            }
        }

        MouseArea { id: badgeMa; anchors.fill: parent; hoverEnabled: true }
    }

    component SectionHeader: Item {
        property string title: ""
        Layout.fillWidth: true
        implicitHeight:   44

        Text {
            anchors { left: parent.left; bottom: parent.bottom; bottomMargin: 6 }
            text:               parent.title.toUpperCase()
            font.pixelSize:     Theme.fontSizeXSmall; font.weight: Font.Medium
            font.letterSpacing: 1.2
            color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.55)
        }
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width; height: 1
            color: Theme.dividerColor; opacity: 0.5
        }
    }

    component RowDivider: Rectangle {
        Layout.fillWidth: true
        implicitHeight:   1
        color:   Theme.dividerColor; opacity: 0.28
    }

    component SettingRow: Item {
        id: sr
        property string label:    ""
        property string subtitle: ""
        property string tip:      ""
        property bool   compact:  false

        default property alias content: slot.data

        Layout.fillWidth: true
        implicitHeight:   subtitle !== "" ? 64 : (compact ? 44 : page.rowH)

        RowLayout {
            anchors.fill: parent
            spacing: 16

            Column {
                Layout.fillWidth: true; Layout.alignment: Qt.AlignVCenter
                spacing: 3

                Text {
                    width:          parent.width
                    text:           sr.label
                    font.pixelSize: Theme.fontSizeBody; font.weight: Font.Medium
                    color:          Theme.textColor; elide: Text.ElideRight
                }
                Text {
                    visible:        sr.subtitle !== ""
                    width:          parent.width
                    text:           sr.subtitle
                    font.pixelSize: Theme.fontSizeXSmall
                    color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.55)
                    wrapMode: Text.Wrap
                }
            }

            InfoBadge {
                tip:     sr.tip
                visible: sr.tip !== ""
                Layout.alignment: Qt.AlignVCenter
            }
            Item {
                visible:              sr.tip === ""
                Layout.preferredWidth: Theme.minTapTarget
                Layout.preferredHeight: 1
                Layout.alignment: Qt.AlignVCenter
            }

            Item {
                id: slot
                Layout.preferredWidth:  page.ctrlW
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                implicitHeight: page.ctrlH
            }
        }
    }

    component Pill: Rectangle {
        property string pillLabel:    ""
        property bool   pillSelected: false
        signal pillClicked()

        implicitWidth:  pillTxt.implicitWidth + 20
        implicitHeight: 30
        radius: 6
        color: pillSelected
            ? Qt.rgba(Theme.accentColor.r, Theme.accentColor.g, Theme.accentColor.b, 0.20)
            : Theme.surfaceSubtle
        border.width: 1
        border.color: pillSelected
            ? Qt.rgba(Theme.accentColor.r, Theme.accentColor.g, Theme.accentColor.b, 0.55)
            : Theme.surfaceBorder
        Behavior on color { ColorAnimation { duration: 120 } }

        Text {
            id: pillTxt
            anchors.centerIn: parent
            text:           parent.pillLabel
            font.pixelSize: Theme.fontSizeSmall; font.weight: Font.Medium
            color: parent.pillSelected ? Theme.accentColor
                : Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.7)
        }
        MouseArea {
            anchors.centerIn: parent
            width: parent.width; height: Theme.minTapTarget
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.pillClicked()
        }
    }

    component ColorChip: Rectangle {
        property string chipValue:    ""
        property color  chipColor:    "gray"
        property bool   chipSelected: false
        property string chipLabel:    ""
        signal chipClicked()

        width: 30; height: 30; radius: 15
        color: chipColor
        border.width: chipSelected ? 2 : 0
        border.color: Theme.textColor
        opacity: chipSelected ? 1.0 : 0.5
        Behavior on opacity      { NumberAnimation { duration: 120 } }
        Behavior on border.width { NumberAnimation { duration: 120 } }

        Rectangle {
            anchors.centerIn: parent
            width: 8; height: 8; radius: 4
            color:   Theme.textColor
            visible: parent.chipSelected
        }

        // Hover label
        Rectangle {
            anchors { horizontalCenter: parent.horizontalCenter; top: parent.bottom; topMargin: 4 }
            width:  hoverLbl.implicitWidth + 8; height: 16; radius: 3
            color:  Theme.cardBackground
            border.width: 1; border.color: Theme.dividerColor
            visible: chipMa.containsMouse; z: 10

            Text {
                id: hoverLbl
                anchors.centerIn: parent
                text:           parent.parent.chipLabel
                font.pixelSize: 9; color: Theme.hintColor
            }
        }

        MouseArea {
            id: chipMa
            anchors.centerIn: parent
            width: Theme.minTapTarget; height: Theme.minTapTarget
            hoverEnabled: true; cursorShape: Qt.PointingHandCursor
            onClicked: parent.chipClicked()
        }
    }

    // ═════════════════════════════════════════════════════════════════════════
    // ROOT LAYOUT
    // ═════════════════════════════════════════════════════════════════════════
    Item {
        anchors.fill: parent

        ScrollView {
            anchors { fill: parent; bottomMargin: page.dirty ? 56 : 0 }
            Behavior on anchors.bottomMargin { NumberAnimation { duration: 200; easing.type: Easing.OutQuad } }
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            Item {
                width: parent.width
                implicitHeight: col.implicitHeight

                ColumnLayout {
                    id: col
                    anchors.horizontalCenter: parent.horizontalCenter
                    width:   Math.min(parent.width - 32, 680)
                    spacing: 0

                    Item { Layout.fillWidth: true; implicitHeight: 20 }

                    // ── APPEARANCE ────────────────────────────────────────────
                    SectionHeader { title: "Appearance" }

                    SettingRow {
                        label: "Theme"
                        Row {
                            anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                            spacing: 8
                            Repeater {
                                model: [{ label: "Dark", value: "dark" }, { label: "Light", value: "light" }]
                                Pill {
                                    required property var modelData
                                    pillLabel:    modelData.label
                                    pillSelected: appConfig.theme === modelData.value
                                    onPillClicked: { appConfig.theme = modelData.value; page.markDirty() }
                                }
                            }
                        }
                    }
                    RowDivider {}

                    ColumnLayout {
                        Layout.fillWidth: true; spacing: 10

                        Item {
                            Layout.fillWidth: true; implicitHeight: 44
                            Text {
                                anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                                text:           "Accent Color"
                                font.pixelSize: Theme.fontSizeBody; font.weight: Font.Medium
                                color:          Theme.textColor
                            }
                        }

                        Flow {
                            Layout.fillWidth:    true
                            Layout.bottomMargin: 20
                            spacing: 8
                            Repeater {
                                model: [
                                    { label: "Red",         value: "red",        hex: "#F44336" },
                                    { label: "Pink",        value: "pink",       hex: "#E91E63" },
                                    { label: "Purple",      value: "purple",     hex: "#9C27B0" },
                                    { label: "Deep Purple", value: "deeppurple", hex: "#673AB7" },
                                    { label: "Indigo",      value: "indigo",     hex: "#3F51B5" },
                                    { label: "Blue",        value: "blue",       hex: "#2196F3" },
                                    { label: "Light Blue",  value: "lightblue",  hex: "#03A9F4" },
                                    { label: "Cyan",        value: "cyan",       hex: "#00BCD4" },
                                    { label: "Teal",        value: "teal",       hex: "#009688" },
                                    { label: "Green",       value: "green",      hex: "#4CAF50" },
                                    { label: "Light Green", value: "lightgreen", hex: "#8BC34A" },
                                    { label: "Lime",        value: "lime",       hex: "#CDDC39" },
                                    { label: "Yellow",      value: "yellow",     hex: "#FFEB3B" },
                                    { label: "Amber",       value: "amber",      hex: "#FFC107" },
                                    { label: "Orange",      value: "orange",     hex: "#FF9800" },
                                    { label: "Deep Orange", value: "deeporange", hex: "#FF5722" },
                                    { label: "Brown",       value: "brown",      hex: "#795548" },
                                    { label: "Blue Grey",   value: "bluegrey",   hex: "#607D8B" },
                                ]
                                ColorChip {
                                    required property var modelData
                                    chipValue:    modelData.value
                                    chipColor:    modelData.hex
                                    chipLabel:    modelData.label
                                    chipSelected: appConfig.accentColor === modelData.value
                                    onChipClicked: { appConfig.accentColor = modelData.value; page.markDirty() }
                                }
                            }
                        }
                    }
                    RowDivider {}

                    SettingRow {
                        label:    "Text Scale"
                        subtitle: "Current: " + Math.round(appConfig.fontScale * 100) + "%  (default: 100%)"
                        tip:      "Scales all text proportionally."
                        StepperField {
                            anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                            value: Math.round(appConfig.fontScale * 100)
                            min: 70; max: 200; unit: " %"
                            onValueChanged: { appConfig.fontScale = value / 100.0; page.markDirty() }
                        }
                    }

                    // ── LANGUAGE ──────────────────────────────────────────────
                    SectionHeader { title: "Language" }

                    ColumnLayout {
                        Layout.fillWidth: true; spacing: 10

                        RowLayout {
                            Layout.fillWidth: true; Layout.minimumHeight: 36; spacing: 16
                            Text {
                                text: "Gloss Languages"
                                font.pixelSize: Theme.fontSizeBody; font.weight: Font.Medium
                                color: Theme.textColor
                                Layout.fillWidth: true; Layout.alignment: Qt.AlignVCenter
                            }
                            InfoBadge {
                                tip: "Languages shown for word definitions. Multi-select."
                                Layout.alignment: Qt.AlignVCenter
                            }
                        }

                        Flow {
                            id: glossFlow
                            Layout.fillWidth: true; spacing: 6

                            readonly property var langs:  ["de","en","es","fr","hu","nl","ru","slv","sv"]
                            readonly property var labels: ["DE","EN","ES","FR","HU","NL","RU","SLV","SV"]

                            function active(lang) {
                                return appConfig.glossLanguages
                                    .split(",").map(s => s.trim()).indexOf(lang) >= 0
                            }
                            function toggle(lang) {
                                const set = {}
                                appConfig.glossLanguages.split(",").forEach(t => {
                                    const r = t.trim()
                                    if (r.length > 0) set[r] = true
                                })
                                if (set[lang]) {
                                    if (Object.keys(set).length > 1) delete set[lang]
                                } else {
                                    set[lang] = true
                                }
                                appConfig.glossLanguages = Object.keys(set).join(", ")
                                page.markDirty()
                            }

                            Repeater {
                                model: glossFlow.langs.length
                                Pill {
                                    required property int index
                                    pillLabel:    glossFlow.labels[index]
                                    pillSelected: glossFlow.active(glossFlow.langs[index])
                                    onPillClicked: glossFlow.toggle(glossFlow.langs[index])
                                }
                            }
                        }
                    }

                    Item { Layout.fillWidth: true; implicitHeight: 12 }
                    RowDivider {}

                    ColumnLayout {
                        Layout.fillWidth: true; spacing: 10

                        RowLayout {
                            Layout.fillWidth: true; Layout.minimumHeight: 36; spacing: 16
                            Text {
                                text: "Fallback Language"
                                font.pixelSize: Theme.fontSizeBody; font.weight: Font.Medium
                                color: Theme.textColor
                                Layout.fillWidth: true; Layout.alignment: Qt.AlignVCenter
                            }
                            InfoBadge {
                                tip: "Used when a definition isn't available in your selected gloss languages."
                                Layout.alignment: Qt.AlignVCenter
                            }
                        }

                        Flow {
                            Layout.fillWidth: true; spacing: 6
                            Repeater {
                                model: ["de","en","es","fr","hu","nl","ru","slv","sv"]
                                Pill {
                                    required property string modelData
                                    pillLabel:    modelData.toUpperCase()
                                    pillSelected: appConfig.fallbackLanguage === modelData
                                    onPillClicked: { appConfig.fallbackLanguage = modelData; page.markDirty() }
                                }
                            }
                        }
                    }

                    Item { Layout.fillWidth: true; implicitHeight: 12 }
                    RowDivider {}

                    ColumnLayout {
                        Layout.fillWidth: true; spacing: 10

                        RowLayout {
                            Layout.fillWidth: true; Layout.minimumHeight: 36; spacing: 16
                            Column {
                                Layout.fillWidth: true; Layout.alignment: Qt.AlignVCenter; spacing: 2
                                Text {
                                    text: "Interface Language"
                                    font.pixelSize: Theme.fontSizeBody; font.weight: Font.Medium
                                    color: Theme.textColor
                                }
                                Text {
                                    text: "Requires restart"
                                    font.pixelSize: Theme.fontSizeXSmall
                                    color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.5)
                                }
                            }
                            InfoBadge {
                                tip: "Language used for the app UI."
                                Layout.alignment: Qt.AlignVCenter
                            }
                        }

                        Flow {
                            id: ifaceFlow
                            Layout.fillWidth: true; spacing: 6

                            readonly property var langs:  ["en","fr","de","ru","es","pt","it","nl","hu","sv","cs","pl","ro","he","ar","tr","th","vi","id","ms","ko","zh","zh-cn","zh-tw","fa","eo","slv"]
                            readonly property var labels: ["EN","FR","DE","RU","ES","PT","IT","NL","HU","SV","CS","PL","RO","HE","AR","TR","TH","VI","ID","MS","KO","ZH","ZH-CN","ZH-TW","FA","EO","SLV"]

                            Repeater {
                                model: ifaceFlow.langs.length
                                Pill {
                                    required property int index
                                    pillLabel:    ifaceFlow.labels[index]
                                    pillSelected: appConfig.interfaceLanguage === ifaceFlow.langs[index]
                                    onPillClicked: { appConfig.interfaceLanguage = ifaceFlow.langs[index]; page.markDirty() }
                                }
                            }
                        }
                    }

                    Item { Layout.fillWidth: true; implicitHeight: page.secGap }

                    // ── DICTIONARY ────────────────────────────────────────────
                    SectionHeader { title: "Dictionary" }

                    SettingRow {
                        label: "Search on Typing"
                        ToggleSwitch {
                            anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                            checked: appConfig.searchOnTyping
                            onToggled: (v) => { appConfig.searchOnTyping = v; page.markDirty() }
                        }
                    }
                    RowDivider {}

                    SettingRow {
                        label: "Search Delay"
                        tip:   "Milliseconds after typing before triggering search.\n\nDefault: 150 ms"
                        StepperField {
                            anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                            value: appConfig.searchDelayMs; min: 0; max: 2000; unit: " ms"
                            onValueChanged: { appConfig.searchDelayMs = value; page.markDirty() }
                        }
                    }
                    RowDivider {}

                    SettingRow {
                        label: "Show Romaji"
                        ToggleSwitch {
                            anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                            checked: appConfig.showRomaji
                            onToggled: (v) => { appConfig.showRomaji = v; page.markDirty() }
                        }
                    }
                    RowDivider {}

                    SettingRow {
                        label: "Max Search Results"
                        tip:   "Maximum dictionary entries shown in search results.\n\nDefault: 20000"
                        StepperField {
                            anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                            value: appConfig.maxResults; min: 1; max: 100000; unit: " results"
                            onValueChanged: { appConfig.maxResults = value; page.markDirty() }
                        }
                    }
                    RowDivider {}

                    SettingRow {
                        label: "Results Per Page"
                        tip:   "Number of results shown per page in the dictionary view.\n\nDefault: 20"
                        StepperField {
                            anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                            value: appConfig.pageSize; min: 1; max: 100; unit: " per page"
                            onValueChanged: { appConfig.pageSize = value; page.markDirty() }
                        }
                    }

                    // ── SPACED REPETITION ─────────────────────────────────────
                    SectionHeader { title: "Spaced Repetition" }

                    SettingRow {
                        label: "New Cards / Day"
                        tip:   "Maximum new cards per day. 0 = unlimited.\n\nDefault: 20"
                        StepperField {
                            anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                            value: appConfig.newCardsPerDay; min: 0; max: 9999; zeroLabel: "∞"
                            onValueChanged: { appConfig.newCardsPerDay = value; page.markDirty() }
                        }
                    }
                    RowDivider {}

                    SettingRow {
                        label: "Reviews / Day"
                        tip:   "Maximum review cards per day. 0 = unlimited.\n\nDefault: 200"
                        StepperField {
                            anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                            value: appConfig.reviewsPerDay; min: 0; max: 9999; zeroLabel: "∞"
                            onValueChanged: { appConfig.reviewsPerDay = value; page.markDirty() }
                        }
                    }
                    RowDivider {}

                    SettingRow {
                        label:    "Desired Retention"
                        subtitle: Math.round(appConfig.desiredRetention * 100) + "% recall target"
                        tip:      "Target recall probability at review time. Higher = more reviews.\n\nRecommended: 85–92%. Default: 90%."
                        Slider {
                            anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
                            from: 0.70; to: 0.97; stepSize: 0.01
                            value: appConfig.desiredRetention
                            onMoved: { appConfig.desiredRetention = value; page.markDirty() }
                        }
                    }
                    RowDivider {}

                    SettingRow {
                        label: "Maximum Interval"
                        tip:   "Longest gap between reviews in days. Default: 36500 (≈100 years)."
                        StepperField {
                            anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                            value: appConfig.maximumInterval; min: 1; max: 99999; unit: " days"
                            onValueChanged: { appConfig.maximumInterval = value; page.markDirty() }
                        }
                    }
                    RowDivider {}

                    SettingRow {
                        label: "Leech Threshold"
                        tip:   "Lapses before a card is flagged as a leech.\n\nDefault: 8"
                        StepperField {
                            anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                            value: appConfig.leechThreshold; min: 2; max: 99; unit: " lapses"
                            onValueChanged: { appConfig.leechThreshold = value; page.markDirty() }
                        }
                    }
                    RowDivider {}

                    SettingRow {
                        label: "Day Offset"
                        tip:   "Seconds after midnight when the SRS day starts. 14400 = 4:00 AM.\n\nDefault: 14400"
                        StepperField {
                            anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                            value: appConfig.dayOffset; min: 0; max: 86399; unit: " sec"
                            onValueChanged: { appConfig.dayOffset = value; page.markDirty() }
                        }
                    }
                    RowDivider {}

                    SettingRow {
                        label:   "Interval Fuzz"
                        tip:     "Randomizes intervals slightly to avoid review clustering."
                        compact: true
                        ToggleSwitch {
                            anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                            checked: appConfig.enableFuzz
                            onToggled: (v) => { appConfig.enableFuzz = v; page.markDirty() }
                        }
                    }
                    RowDivider {}

                    SettingRow {
                        label:   "Scheduler Order"
                        tip:     "Controls the order in which cards are reviewed first."
                        compact: true

                        ComboBox {
                            id: combo
                            anchors.verticalCenter: parent.verticalCenter
                            width: parent.width; height: 30
                            model: ["Mixed", "Review First", "New First"]
                            currentIndex: appConfig.orderMode
                            onCurrentIndexChanged: { appConfig.orderMode = currentIndex; page.markDirty() }

                            background: Rectangle {
                                radius: 6
                                color: combo.pressed ? Theme.surfacePress
                                     : combo.hovered ? Theme.surfaceHover : Theme.surfaceSubtle
                                border.width: 1
                                border.color: combo.hovered
                                    ? Qt.rgba(Theme.accentColor.r, Theme.accentColor.g, Theme.accentColor.b, 0.5)
                                    : Theme.surfaceBorder
                                Behavior on color        { ColorAnimation { duration: 100 } }
                                Behavior on border.color { ColorAnimation { duration: 120 } }
                            }

                            contentItem: Text {
                                text:              combo.displayText
                                font.pixelSize:    Theme.fontSizeSmall
                                color:             Theme.textColor
                                verticalAlignment: Text.AlignVCenter
                                elide:             Text.ElideRight
                                leftPadding: 10; rightPadding: 24
                            }

                            indicator: Text {
                                text: "▾"; font.pixelSize: 12; color: Theme.hintColor
                                anchors { right: parent.right; rightMargin: 8; verticalCenter: parent.verticalCenter }
                            }

                            popup: Popup {
                                y: combo.height + 4
                                width: combo.width
                                height: combo.count * 36
                                padding: 4
                                closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

                                background: Rectangle {
                                    radius: 6
                                    color: Theme.cardBackground
                                    border.width: 1; border.color: Theme.dividerColor
                                    layer.enabled: true
                                    layer.effect: MultiEffect {
                                        shadowEnabled: true; shadowColor: "#60000000"
                                        shadowBlur: 0.6; shadowVerticalOffset: 4
                                    }
                                }

                                contentItem: ListView {
                                    clip: true; interactive: false
                                    model: combo.delegateModel
                                    highlight: Rectangle {
                                        radius: 4
                                        color: Qt.rgba(Theme.accentColor.r, Theme.accentColor.g, Theme.accentColor.b, 0.20)
                                        Behavior on y { SmoothedAnimation { velocity: 200 } }
                                    }
                                    highlightFollowsCurrentItem: true
                                    currentIndex: combo.highlightedIndex
                                }
                            }

                            delegate: ItemDelegate {
                                width: combo.width - 8; height: 36
                                onClicked: { combo.currentIndex = index; combo.popup.close() }

                                background: Rectangle {
                                    radius: 4
                                    color: hovered ? Theme.surfaceHover : "transparent"
                                    Behavior on color { ColorAnimation { duration: 80 } }
                                }
                                contentItem: Row {
                                    spacing: 8; leftPadding: 6
                                    anchors.verticalCenter: parent.verticalCenter
                                    Text {
                                        text: index === combo.currentIndex ? "✓" : ""
                                        font.pixelSize: Theme.fontSizeSmall; color: Theme.accentColor
                                        width: 14; verticalAlignment: Text.AlignVCenter
                                    }
                                    Text {
                                        text: modelData; font.pixelSize: Theme.fontSizeSmall
                                        color: index === combo.currentIndex ? Theme.accentColor : Theme.textColor
                                        verticalAlignment: Text.AlignVCenter
                                        Behavior on color { ColorAnimation { duration: 80 } }
                                    }
                                }
                            }
                        }
                    }

                    // ── ABOUT ─────────────────────────────────────────────────
                    SectionHeader { title: "About" }

                    SettingRow {
                        label: "Version"
                        Text {
                            anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                            text:           appConfig.appVersion
                            font.pixelSize: Theme.fontSizeBody
                            color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.6)
                        }
                    }
                    RowDivider {}

                    SettingRow {
                        label:    "Device ID"
                        subtitle: "Used for sync identification"
                        Text {
                            anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                            text:           appConfig.deviceId.toString(16).toUpperCase().substring(0, 8) + "…"
                            font.pixelSize: Theme.fontSizeSmall; font.family: "monospace"
                            color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.5)
                        }
                    }
                    RowDivider {}

                    SettingRow {
                        label:    "Reset to Defaults"
                        subtitle: "Restore all settings to their original values"

                        Rectangle {
                            anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                            width:  rstLbl.implicitWidth + 20; height: 30; radius: 6
                            color: rstMa.containsMouse
                                ? Qt.rgba(0.97, 0.44, 0.44, 0.22)
                                : Qt.rgba(0.97, 0.44, 0.44, 0.10)
                            border.width: 1; border.color: Qt.rgba(0.97, 0.44, 0.44, 0.40)
                            Behavior on color { ColorAnimation { duration: 100 } }

                            Text {
                                id: rstLbl; anchors.centerIn: parent
                                text: "Reset"; font.pixelSize: Theme.fontSizeSmall; font.weight: Font.Medium
                                color: "#F87171"
                            }
                            MouseArea {
                                id: rstMa; anchors.fill: parent
                                hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                onClicked: resetConfirmDialog.open()
                            }
                        }
                    }

                    Item { Layout.fillWidth: true; implicitHeight: 40 }
                }
            }
        }

        // ── Apply / Discard bar ───────────────────────────────────────────────
        Rectangle {
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
            height: page.dirty ? 56 : 0
            clip: true; color: Theme.cardBackground; z: 10
            Behavior on height { NumberAnimation { duration: 200; easing.type: Easing.OutQuad } }

            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.dividerColor }

            RowLayout {
                anchors { fill: parent; leftMargin: 24; rightMargin: 24 }
                spacing: 12
                visible: page.dirty

                Text {
                    text:           "Unsaved changes"
                    font.pixelSize: Theme.fontSizeSmall
                    color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.65)
                    Layout.fillWidth: true; Layout.alignment: Qt.AlignVCenter
                }

                Rectangle {
                    width: dscLbl.implicitWidth + 20; height: 30; radius: 6
                    color: dscMa.containsMouse ? Theme.surfaceHover : "transparent"
                    border.width: 1; border.color: Theme.surfaceBorder
                    Layout.alignment: Qt.AlignVCenter
                    Behavior on color { ColorAnimation { duration: 100 } }
                    Text {
                        id: dscLbl; anchors.centerIn: parent; text: "Discard"
                        font.pixelSize: Theme.fontSizeSmall; font.weight: Font.Medium
                        color: Qt.rgba(Theme.hintColor.r, Theme.hintColor.g, Theme.hintColor.b, 0.7)
                    }
                    MouseArea {
                        id: dscMa; anchors.fill: parent
                        hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: { appConfig.reloadFromDisk(); page.dirty = false }
                    }
                }

                Rectangle {
                    width: aplLbl.implicitWidth + 24; height: 30; radius: 6
                    color: aplMa.containsMouse ? Qt.lighter(Theme.accentColor, 1.15) : Theme.accentColor
                    Layout.alignment: Qt.AlignVCenter
                    Behavior on color { ColorAnimation { duration: 100 } }
                    Text {
                        id: aplLbl; anchors.centerIn: parent; text: "Apply"
                        font.pixelSize: Theme.fontSizeSmall; font.weight: Font.Medium
                        color: "white"
                    }
                    MouseArea {
                        id: aplMa; anchors.fill: parent
                        hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: page.applySettings()
                    }
                }
            }
        }
    }

    Dialog {
        id: resetConfirmDialog
        title: "Reset to defaults?"
        width: 320; anchors.centerIn: Overlay.overlay; modal: true

        Text {
            text: "All settings will be restored to their default values."
            wrapMode: Text.Wrap; width: 260
            color: Theme.textColor; font.pixelSize: Theme.fontSizeBody
        }
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: page.resetToDefaults()
    }
}

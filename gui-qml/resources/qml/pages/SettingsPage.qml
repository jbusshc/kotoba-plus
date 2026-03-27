import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import Qt5Compat.GraphicalEffects
import Kotoba 1.0


Page {
    id: page
    padding: 0

    // ── Aliases into Theme so child components don't need to qualify ──────────
    readonly property color textColor:    Theme.textColor
    readonly property color hintColor:    Theme.hintColor
    readonly property color accentColor:  Theme.accentColor
    readonly property color dividerColor: Theme.dividerColor

    // ── Layout constants ──────────────────────────────────────────────────────
    readonly property int rowH:    56   // standard row height
    readonly property int ctrlH:   32   // control height (stepper, toggle, pill row)
    readonly property int ctrlW:  220   // right-column width
    readonly property int unitW:   52   // unit label width (ms / days / sec / %)
    readonly property int secGap:  24   // vertical gap between sections

    background: Rectangle { color: Theme.background }

    // ── Snapshot-based dirty detection ───────────────────────────────────────
    property bool ready:    false
    property bool dirty:    false
    property var  snapshot: ({})

    function takeSnapshot() {
        snapshot = {
            theme:             appConfig.theme,
            accentColor:       appConfig.accentColor,
            fontScale:         appConfig.fontScale,
            showFurigana:      appConfig.showFurigana,
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
        }
    }

    function checkDirty() {
        if (!ready || Object.keys(snapshot).length === 0) { dirty = false; return }
        dirty = snapshot.theme             !== appConfig.theme
             || snapshot.accentColor       !== appConfig.accentColor
             || Math.abs(snapshot.fontScale      - appConfig.fontScale)      > 0.001
             || snapshot.showFurigana      !== appConfig.showFurigana
             || snapshot.glossLanguages    !== appConfig.glossLanguages
             || snapshot.fallbackLanguage  !== appConfig.fallbackLanguage
             || snapshot.interfaceLanguage !== appConfig.interfaceLanguage
             || snapshot.searchOnTyping    !== appConfig.searchOnTyping
             || snapshot.searchDelayMs     !== appConfig.searchDelayMs
             || snapshot.newCardsPerDay    !== appConfig.newCardsPerDay
             || snapshot.reviewsPerDay     !== appConfig.reviewsPerDay
             || Math.abs(snapshot.desiredRetention - appConfig.desiredRetention) > 0.001
             || snapshot.maximumInterval   !== appConfig.maximumInterval
             || snapshot.leechThreshold    !== appConfig.leechThreshold
             || snapshot.dayOffset         !== appConfig.dayOffset
             || snapshot.enableFuzz        !== appConfig.enableFuzz
             || snapshot.orderMode         !== appConfig.orderMode
             || snapshot.showRomaji        !== appConfig.showRomaji
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
        appConfig.showFurigana      = true
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
            page.ready = false
            page.dirty = false
            initTimer.restart()
        }
    }

    // ══════════════════════════════════════════════════════════════════════════
    // REUSABLE COMPONENTS
    // ══════════════════════════════════════════════════════════════════════════

    // ── Tooltip badge ─────────────────────────────────────────────────────────
    component InfoBadge: Item {
        id: badge
        property string tip: ""
        implicitWidth: 18; implicitHeight: 18

        Rectangle {
            anchors.fill: parent; radius: 9
            color: ma.containsMouse
                ? Qt.rgba(page.accentColor.r, page.accentColor.g, page.accentColor.b, 0.25)
                : Qt.rgba(1, 1, 1, 0.08)
            border.width: 1; border.color: Qt.rgba(1, 1, 1, 0.15)
            Behavior on color { ColorAnimation { duration: 120 } }

            Text {
                anchors.centerIn: parent
                text: "?"
                font.pixelSize: Theme.fontSizeXSmall
                color: Qt.rgba(page.hintColor.r, page.hintColor.g, page.hintColor.b, 0.7)
            }
        }

        // Floating tooltip bubble
        Rectangle {
            visible: ma.containsMouse
            z: 999
            width: Math.min(tipText.implicitWidth + 20, 260)
            height: tipText.implicitHeight + 14
            radius: 6
            color: Theme.cardBackground
            border.width: 1; border.color: page.dividerColor
            anchors.right: parent.right
            anchors.bottom: parent.top
            anchors.bottomMargin: 6

            Text {
                id: tipText
                anchors.fill: parent; anchors.margins: 10
                text: badge.tip
                font.pixelSize: Theme.fontSizeXSmall
                color: Qt.rgba(page.hintColor.r, page.hintColor.g, page.hintColor.b, 0.9)
                wrapMode: Text.Wrap
            }
        }

        MouseArea { id: ma; anchors.fill: parent; hoverEnabled: true }
    }

    // ── Section header with label + rule ─────────────────────────────────────
    component SectionHeader: Item {
        property string title: ""
        Layout.fillWidth: true
        implicitHeight: 44

        Text {
            anchors.left: parent.left
            anchors.bottom: parent.bottom; anchors.bottomMargin: 6
            text: parent.title.toUpperCase()
            font.pixelSize: Theme.fontSizeXSmall; font.weight: Font.Medium
            font.letterSpacing: 1.2
            color: Qt.rgba(page.hintColor.r, page.hintColor.g, page.hintColor.b, 0.55)
        }
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width; height: 1
            color: page.dividerColor; opacity: 0.5
        }
    }

    // ── Thin row divider ──────────────────────────────────────────────────────
    component RowDivider: Rectangle {
        Layout.fillWidth: true
        implicitHeight: 1
        color: page.dividerColor; opacity: 0.28
    }

    // ── Toggle switch ─────────────────────────────────────────────────────────
    //    Self-contained; bind `checked` two-way via onCheckedChanged.
    component ToggleSwitch: Rectangle {
        id: toggle
        property bool checked: false
        signal toggled(bool newValue)

        implicitWidth: 48; implicitHeight: page.ctrlH
        radius: height / 2
        color: checked
            ? Qt.rgba(page.accentColor.r, page.accentColor.g, page.accentColor.b, 0.25)
            : Qt.rgba(1, 1, 1, 0.08)
        border.width: 1
        border.color: checked
            ? Qt.rgba(page.accentColor.r, page.accentColor.g, page.accentColor.b, 0.6)
            : Qt.rgba(1, 1, 1, 0.15)
        Behavior on color       { ColorAnimation { duration: 150 } }
        Behavior on border.color { ColorAnimation { duration: 150 } }

        Rectangle {
            id: knob
            width: 20; height: 20; radius: 10
            anchors.verticalCenter: parent.verticalCenter
            x: toggle.checked ? parent.width - width - 6 : 6
            color: toggle.checked ? page.accentColor : Qt.rgba(1, 1, 1, 0.4)
            Behavior on x     { NumberAnimation { duration: 150; easing.type: Easing.OutQuad } }
            Behavior on color { ColorAnimation  { duration: 150 } }
        }

        MouseArea {
            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
            onClicked: toggle.toggled(!toggle.checked)
        }
    }

    // ── Single stepper button (−/+) ───────────────────────────────────────────
    component StepButton: Rectangle {
        property string label: "+"
        signal clicked()

        implicitWidth: 28; implicitHeight: page.ctrlH
        radius: 6
        color: stepMa.containsMouse ? Qt.rgba(1, 1, 1, 0.10) : Qt.rgba(1, 1, 1, 0.05)
        border.width: 1; border.color: Qt.rgba(1, 1, 1, 0.10)
        Behavior on color { ColorAnimation { duration: 100 } }

        Text {
            anchors.centerIn: parent
            text: parent.label
            font.pixelSize: Theme.fontSizeBody
            color: page.hintColor
        }
        MouseArea {
            id: stepMa; anchors.fill: parent
            hoverEnabled: true; cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
        }
    }

    // ── Numeric stepper (−  [value]  +) with optional unit label ─────────────
    //    Usage: StepperField { value: ...; min: 0; max: 9999; unit: " ms";
    //                          zeroLabel: "∞"; onValueChanged: ... }
    component StepperField: RowLayout {
        id: sf
        property int    value:      0
        property int    min:        0
        property int    max:        99999
        property string zeroLabel:  ""      // displayed when value === 0
        property string unit:       ""      // optional trailing unit label

        spacing: 0

        StepButton {
            label: "−"
            onClicked: if (sf.value > sf.min) sf.value--
        }

        // Value display / text input
        Rectangle {
            implicitWidth: 72; implicitHeight: page.ctrlH
            color: Qt.rgba(1, 1, 1, 0.04)
            border.width: 1; border.color: Qt.rgba(1, 1, 1, 0.10)

            TextInput {
                anchors.centerIn: parent
                width: parent.width - 8
                font.pixelSize: Theme.fontSizeSmall; font.weight: Font.Medium
                horizontalAlignment: Text.AlignHCenter
                color: (sf.value === 0 && sf.zeroLabel !== "") ? page.accentColor : page.textColor
                validator: IntValidator { bottom: sf.min; top: sf.max }
                text: (sf.value === 0 && sf.zeroLabel !== "") ? sf.zeroLabel
                                                              : sf.value.toString()
                onEditingFinished: {
                    if (sf.zeroLabel !== "" && text === sf.zeroLabel) {
                        sf.value = 0
                    } else {
                        var v = parseInt(text)
                        if (!isNaN(v))
                            sf.value = Math.max(sf.min, Math.min(sf.max, v))
                    }
                }
            }
        }

        StepButton {
            label: "+"
            onClicked: if (sf.value < sf.max) sf.value++
        }

        // Optional unit label, right-aligned within ctrlW
        Text {
            visible: sf.unit !== ""
            Layout.preferredWidth: page.unitW
            text: sf.unit
            font.pixelSize: Theme.fontSizeXSmall
            color: Qt.rgba(page.hintColor.r, page.hintColor.g, page.hintColor.b, 0.55)
            verticalAlignment: Text.AlignVCenter
        }
    }

    // ── Generic setting row ───────────────────────────────────────────────────
    //    Item wrapper with explicit height — the inner RowLayout just handles
    //    horizontal distribution. This is the only reliable way to fix height
    //    in a parent ColumnLayout: RowLayout children ignore preferredHeight.
    component SettingRow: Item {
        id: sr
        property string label:    ""
        property string subtitle: ""
        property string tip:      ""
        property bool   compact:  false

        default property alias content: controlSlot.data

        Layout.fillWidth: true
        // Height is owned by THIS Item, not by the inner RowLayout.
        implicitHeight: subtitle !== "" ? 64 : (compact ? 44 : page.rowH)

        RowLayout {
            anchors.fill: parent
            spacing: 16

            // Left: label + optional subtitle
            Column {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                spacing: 3

                Text {
                    width: parent.width
                    text: sr.label
                    font.pixelSize: Theme.fontSizeBody; font.weight: Font.Medium
                    color: page.textColor
                    elide: Text.ElideRight
                }
                Text {
                    visible: sr.subtitle !== ""
                    width: parent.width
                    text: sr.subtitle
                    font.pixelSize: Theme.fontSizeXSmall
                    color: Qt.rgba(page.hintColor.r, page.hintColor.g, page.hintColor.b, 0.55)
                    wrapMode: Text.Wrap
                }
            }

            // Centre: tooltip badge — always 18 px wide to keep columns aligned
            InfoBadge {
                tip:     sr.tip
                visible: sr.tip !== ""
                Layout.alignment: Qt.AlignVCenter
            }
            Item {
                visible: sr.tip === ""
                Layout.preferredWidth: 18; Layout.preferredHeight: 1
                Layout.alignment: Qt.AlignVCenter
            }

            // Right: fixed-width control slot
            Item {
                id: controlSlot
                Layout.preferredWidth: page.ctrlW
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                implicitHeight: page.ctrlH
            }
        }
    }

    // ── Pill toggle button (for multi-select / single-select groups) ──────────
    component Pill: Rectangle {
        property string pillLabel:    ""
        property bool   pillSelected: false
        signal pillClicked()

        implicitWidth:  pillTxt.implicitWidth + 20
        implicitHeight: 30
        radius: 6
        color: pillSelected
            ? Qt.rgba(page.accentColor.r, page.accentColor.g, page.accentColor.b, 0.20)
            : Qt.rgba(1, 1, 1, 0.05)
        border.width: 1
        border.color: pillSelected
            ? Qt.rgba(page.accentColor.r, page.accentColor.g, page.accentColor.b, 0.55)
            : Qt.rgba(1, 1, 1, 0.10)
        Behavior on color { ColorAnimation { duration: 120 } }

        Text {
            id: pillTxt
            anchors.centerIn: parent
            text: parent.pillLabel
            font.pixelSize: Theme.fontSizeSmall; font.weight: Font.Medium
            color: parent.pillSelected ? page.accentColor
                                       : Qt.rgba(page.hintColor.r, page.hintColor.g, page.hintColor.b, 0.7)
        }
        MouseArea {
            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
            onClicked: parent.pillClicked()
        }
    }

    // ── Color chip ────────────────────────────────────────────────────────────
    component ColorChip: Rectangle {
        property string chipValue:    ""
        property color  chipColor:    "gray"
        property bool   chipSelected: false
        property string chipLabel:    ""
        signal chipClicked()

        width: 30; height: 30; radius: 15
        color: chipColor
        border.width: chipSelected ? 2 : 0; border.color: "white"
        opacity: chipSelected ? 1.0 : 0.5
        Behavior on opacity       { NumberAnimation { duration: 120 } }
        Behavior on border.width  { NumberAnimation { duration: 120 } }

        // Selected dot
        Rectangle {
            anchors.centerIn: parent
            width: 8; height: 8; radius: 4
            color: "white"
            visible: parent.chipSelected
        }

        // Hover label
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.bottom; anchors.topMargin: 4
            width: hoverLbl.implicitWidth + 8; height: 16; radius: 3
            color: Theme.cardBackground
            border.width: 1; border.color: page.dividerColor
            visible: chipMa.containsMouse; z: 10

            Text {
                id: hoverLbl
                anchors.centerIn: parent
                text: parent.parent.chipLabel
                font.pixelSize: 9
                color: page.hintColor
            }
        }

        MouseArea {
            id: chipMa; anchors.fill: parent
            hoverEnabled: true; cursorShape: Qt.PointingHandCursor
            onClicked: parent.chipClicked()
        }
    }

    // ══════════════════════════════════════════════════════════════════════════
    // ROOT LAYOUT
    // ══════════════════════════════════════════════════════════════════════════

    Item {
        anchors.fill: parent

        // ── Scrollable content ────────────────────────────────────────────────
        ScrollView {
            anchors.fill: parent
            anchors.bottomMargin: page.dirty ? 56 : 0
            Behavior on anchors.bottomMargin { NumberAnimation { duration: 200; easing.type: Easing.OutQuad } }
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            Item {
                width: parent.width
                implicitHeight: col.implicitHeight

                ColumnLayout {
                    id: col
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: Math.min(parent.width - 64, 680)
                    spacing: 0

                    Item { Layout.fillWidth: true; implicitHeight: 20 }

                    // ── APPEARANCE ────────────────────────────────────────────
                    SectionHeader { title: "Appearance" }

                    // Theme
                    SettingRow {
                        label: "Theme"

                        Row {
                            anchors.right:          parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 8

                            Repeater {
                                model: [
                                    { label: "Dark",  value: "dark"  },
                                    { label: "Light", value: "light" },
                                ]
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

                    // Accent color — free-height layout (chips wrap naturally)
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Item {
                            Layout.fillWidth: true; implicitHeight: 44
                            Text {
                                anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter
                                text: "Accent Color"
                                font.pixelSize: Theme.fontSizeBody; font.weight: Font.Medium
                                color: page.textColor
                            }
                        }

                        Flow {
                            Layout.fillWidth: true
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

                    // Text Scale
                    SettingRow {
                        label:    "Text Scale"
                        subtitle: "Current: " + Math.round(appConfig.fontScale * 100) + "%  (default: 100%)"
                        tip:      "Scales all text proportionally. 85% = smaller, 100% = default, 115% = larger."

                        StepperField {
                            anchors.left:           parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            value: Math.round(appConfig.fontScale * 100)
                            min:   70
                            max:   200
                            unit:  " %"
                            onValueChanged: { appConfig.fontScale = value / 100.0; page.markDirty() }
                        }
                    }
                    RowDivider {}

                    // Furigana
                    SettingRow {
                        label:   "Show Furigana"

                        ToggleSwitch {
                            anchors.right:          parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            checked: appConfig.showFurigana
                            onToggled: (v) => { appConfig.showFurigana = v; page.markDirty() }
                        }
                    }


                    // ── LANGUAGE ──────────────────────────────────────────────
                    SectionHeader { title: "Language" }

                    // Gloss languages (multi-select pill group)
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.minimumHeight: 36
                            spacing: 16

                            Text {
                                text: "Gloss Languages"
                                font.pixelSize: Theme.fontSizeBody; font.weight: Font.Medium
                                color: page.textColor
                                Layout.fillWidth: true; Layout.alignment: Qt.AlignVCenter
                            }
                            InfoBadge {
                                tip: "Languages shown for word definitions. Multi-select."
                                Layout.alignment: Qt.AlignVCenter
                            }
                        }

                        Flow {
                            id: glossFlow
                            Layout.fillWidth: true
                            spacing: 6

                            readonly property var langs:  ["de","en","es","fr","hu","nl","ru","slv","sv"]
                            readonly property var labels: ["DE","EN","ES","FR","HU","NL","RU","SLV","SV"]

                            function active(lang) {
                                return appConfig.glossLanguages
                                    .split(",")
                                    .map(function(s) { return s.trim() })
                                    .indexOf(lang) >= 0
                            }

                            function toggle(lang) {
                                var set = {}
                                appConfig.glossLanguages.split(",").forEach(function(t) {
                                    var r = t.trim()
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

                    // Fallback language (single-select pill group)
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.minimumHeight: 36
                            spacing: 16

                            Text {
                                text: "Fallback Language"
                                font.pixelSize: Theme.fontSizeBody; font.weight: Font.Medium
                                color: page.textColor
                                Layout.fillWidth: true; Layout.alignment: Qt.AlignVCenter
                            }
                            InfoBadge {
                                tip: "Used when a definition isn't available in your selected gloss languages."
                                Layout.alignment: Qt.AlignVCenter
                            }
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: 6

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

                    // Interface language (single-select pill group)
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.minimumHeight: 36
                            spacing: 16

                            Column {
                                Layout.fillWidth: true; Layout.alignment: Qt.AlignVCenter
                                spacing: 2

                                Text {
                                    text: "Interface Language"
                                    font.pixelSize: Theme.fontSizeBody; font.weight: Font.Medium
                                    color: page.textColor
                                }
                                Text {
                                    text: "Requires restart"
                                    font.pixelSize: Theme.fontSizeXSmall
                                    color: Qt.rgba(page.hintColor.r, page.hintColor.g, page.hintColor.b, 0.5)
                                }
                            }
                            InfoBadge {
                                tip: "Language used for the app UI. Not all languages may be fully translated."
                                Layout.alignment: Qt.AlignVCenter
                            }
                        }

                        Flow {
                            id: ifaceFlow
                            Layout.fillWidth: true
                            spacing: 6

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
                            anchors.right:          parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            checked: appConfig.searchOnTyping
                            onToggled: (v) => { appConfig.searchOnTyping = v; page.markDirty() }
                        }
                    }
                    RowDivider {}

                    SettingRow {
                        label: "Search Delay"
                        tip:   "Milliseconds after typing before triggering search.\n\nDefault: 150 ms"

                        StepperField {
                            anchors.left:           parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            value: appConfig.searchDelayMs
                            min:   0; max: 2000; unit: " ms"
                            onValueChanged: { appConfig.searchDelayMs = value; page.markDirty() }
                        }
                    }
                    RowDivider {}

                    SettingRow {
                        label: "Show Romaji"

                        ToggleSwitch {
                            anchors.right:          parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            checked: appConfig.showRomaji
                            onToggled: (v) => { appConfig.showRomaji = v; page.markDirty() }
                        }
                    }


                    // ── SPACED REPETITION ─────────────────────────────────────
                    SectionHeader { title: "Spaced Repetition" }

                    SettingRow {
                        label: "New Cards / Day"
                        tip:   "Maximum new cards per day. 0 = unlimited.\n\nDefault: 20"

                        StepperField {
                            anchors.left:           parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            value:     appConfig.newCardsPerDay
                            min: 0; max: 9999; zeroLabel: "∞"
                            onValueChanged: { appConfig.newCardsPerDay = value; page.markDirty() }
                        }
                    }
                    RowDivider {}

                    SettingRow {
                        label: "Reviews / Day"
                        tip:   "Maximum review cards per day. 0 = unlimited.\n\nDefault: 200"

                        StepperField {
                            anchors.left:           parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            value:     appConfig.reviewsPerDay
                            min: 0; max: 9999; zeroLabel: "∞"
                            onValueChanged: { appConfig.reviewsPerDay = value; page.markDirty() }
                        }
                    }
                    RowDivider {}

                    // Desired Retention — slider row
                    SettingRow {
                        label:    "Desired Retention"
                        subtitle: Math.round(appConfig.desiredRetention * 100) + "% recall target"
                        tip:      "Target recall probability at review time. Higher = more reviews.\n\nRecommended: 85–92%. Default: 90%."

                        Slider {
                            anchors.left:           parent.left
                            anchors.right:          parent.right
                            anchors.verticalCenter: parent.verticalCenter
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
                            anchors.left:           parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            value:     appConfig.maximumInterval
                            min: 1; max: 99999; unit: " days"
                            onValueChanged: { appConfig.maximumInterval = value; page.markDirty() }
                        }
                    }
                    RowDivider {}

                    SettingRow {
                        label: "Leech Threshold"
                        tip:   "Lapses before a card is flagged as a leech.\n\nDefault: 8"

                        StepperField {
                            anchors.left:           parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            value:     appConfig.leechThreshold
                            min: 2; max: 99; unit: " lapses"
                            onValueChanged: { appConfig.leechThreshold = value; page.markDirty() }
                        }
                    }
                    RowDivider {}

                    SettingRow {
                        label: "Day Offset"
                        tip:   "Seconds after midnight when the SRS day starts. 14400 = 4:00 AM.\n\nDefault: 14400"

                        StepperField {
                            anchors.left:           parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            value:     appConfig.dayOffset
                            min: 0; max: 86399; unit: " sec"
                            onValueChanged: { appConfig.dayOffset = value; page.markDirty() }
                        }
                    }
                    RowDivider {}

                    SettingRow {
                        label:   "Interval Fuzz"
                        tip:     "Randomizes intervals slightly to avoid review clustering."
                        compact: true

                        ToggleSwitch {
                            anchors.right:          parent.right
                            anchors.verticalCenter: parent.verticalCenter
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
                            width: parent.width
                            height: 30

                            model: ["Mixed", "Review First", "New First"]
                            currentIndex: appConfig.orderMode

                            onCurrentIndexChanged: {
                                appConfig.orderMode = currentIndex
                                page.markDirty()
                            }

                            // ── Background ─────────────────────────────────────────
                            background: Rectangle {
                                radius: 6
                                color: combo.pressed
                                    ? Qt.rgba(1, 1, 1, 0.14)
                                    : combo.hovered
                                        ? Qt.rgba(1, 1, 1, 0.10)
                                        : Qt.rgba(1, 1, 1, 0.05)
                                border.width: 1
                                border.color: combo.hovered
                                    ? Qt.rgba(page.accentColor.r, page.accentColor.g, page.accentColor.b, 0.5)
                                    : Qt.rgba(1, 1, 1, 0.10)
                                Behavior on color       { ColorAnimation { duration: 100 } }
                                Behavior on border.color { ColorAnimation { duration: 120 } }
                            }

                            // ── Texto seleccionado ──────────────────────────────────
                            contentItem: Text {
                                text:              combo.displayText
                                font.pixelSize:    Theme.fontSizeSmall
                                color:             page.textColor
                                verticalAlignment: Text.AlignVCenter
                                elide:             Text.ElideRight
                                leftPadding:       10
                                rightPadding:      24
                            }

                            // ── Flecha ─────────────────────────────────────────────
                            indicator: Text {
                                text:   "▾"
                                font.pixelSize: 12
                                color:  page.hintColor
                                anchors.right:          parent.right
                                anchors.rightMargin:    8
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            // ── Popup ───────────────────────────────────────────────
                            popup: Popup {
                                y:      combo.height + 4
                                width:  combo.width
                                height: combo.count * 36
                                padding: 4

                                // Cierra si se clickea fuera
                                closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

                                background: Rectangle {
                                    radius:       6
                                    color:        Theme.cardBackground
                                    border.width: 1
                                    border.color: page.dividerColor

                                    layer.enabled: true
                                    layer.effect: DropShadow {
                                        transparentBorder: true
                                        radius:   12
                                        samples:  17
                                        color:    "#60000000"
                                        verticalOffset: 4
                                    }
                                }

                                contentItem: ListView {
                                    clip:        true
                                    interactive: false
                                    model:       combo.delegateModel   // ← delegateModel, no combo.model

                                    // Highlight animado entre items
                                    highlight: Rectangle {
                                        radius: 4
                                        color: Qt.rgba(
                                            page.accentColor.r,
                                            page.accentColor.g,
                                            page.accentColor.b,
                                            0.20
                                        )
                                        Behavior on y { SmoothedAnimation { velocity: 200 } }
                                    }
                                    highlightFollowsCurrentItem: true
                                    currentIndex: combo.highlightedIndex
                                }
                            }

                            // ── Delegate ────────────────────────────────────────────
                            delegate: ItemDelegate {
                                width:  combo.width - 8   // margen para el padding del popup
                                height: 36

                                // Selecciona y cierra al hacer click
                                onClicked: {
                                    combo.currentIndex = index
                                    combo.popup.close()
                                }

                                // Hover highlight
                                background: Rectangle {
                                    radius: 4
                                    color: hovered
                                        ? Qt.rgba(1, 1, 1, 0.08)
                                        : "transparent"
                                    Behavior on color { ColorAnimation { duration: 80 } }
                                }

                                contentItem: Row {
                                    spacing:         8
                                    leftPadding:     6
                                    anchors.verticalCenter: parent.verticalCenter

                                    // Checkmark en el item activo
                                    Text {
                                        text:           index === combo.currentIndex ? "✓" : ""
                                        font.pixelSize: Theme.fontSizeSmall
                                        color:          page.accentColor
                                        width:          14
                                        verticalAlignment: Text.AlignVCenter
                                    }

                                    Text {
                                        text:              modelData
                                        font.pixelSize:    Theme.fontSizeSmall
                                        color:             index === combo.currentIndex
                                                            ? page.accentColor
                                                            : page.textColor
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
                            anchors.right:          parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            text:  appConfig.appVersion
                            font.pixelSize: Theme.fontSizeBody
                            color: Qt.rgba(page.hintColor.r, page.hintColor.g, page.hintColor.b, 0.6)
                        }
                    }
                    RowDivider {}

                    SettingRow {
                        label:    "Device ID"
                        subtitle: "Used for sync identification"

                        Text {
                            anchors.right:          parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            text: appConfig.deviceId.toString(16).toUpperCase().substring(0, 8) + "…"
                            font.pixelSize: Theme.fontSizeSmall
                            font.family:    "monospace"
                            color: Qt.rgba(page.hintColor.r, page.hintColor.g, page.hintColor.b, 0.5)
                        }
                    }
                    RowDivider {}

                    // Reset to defaults
                    SettingRow {
                        label:    qsTr("Reset to Defaults")
                        subtitle: "Restore all settings to their original values"

                        // Override label color for destructive action
                        Component.onCompleted: {
                            // Find the first Text child of the Column and re-colour it
                            for (var i = 0; i < children.length; i++) {
                                var c = children[i]
                                if (c && c.children) {
                                    for (var j = 0; j < c.children.length; j++) {
                                        if (c.children[j] && c.children[j].text === qsTr("Reset to Defaults")) {
                                            c.children[j].color = "#F87171"
                                            break
                                        }
                                    }
                                }
                            }
                        }

                        Rectangle {
                            anchors.right:          parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            width:  rstLbl.implicitWidth + 20; height: 30; radius: 6
                            color:  rstMa.containsMouse
                                    ? Qt.rgba(0.97, 0.44, 0.44, 0.22)
                                    : Qt.rgba(0.97, 0.44, 0.44, 0.10)
                            border.width: 1; border.color: Qt.rgba(0.97, 0.44, 0.44, 0.40)
                            Behavior on color { ColorAnimation { duration: 100 } }

                            Text {
                                id:   rstLbl
                                anchors.centerIn: parent
                                text: "Reset"
                                font.pixelSize: Theme.fontSizeSmall; font.weight: Font.Medium
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

        // ── Apply / Discard bar — slides in from bottom when dirty ────────────
        Rectangle {
            id: applyBar
            anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
            height: page.dirty ? 56 : 0
            clip: true
            color: Theme.cardBackground
            z: 10
            Behavior on height { NumberAnimation { duration: 200; easing.type: Easing.OutQuad } }

            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: page.dividerColor }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 24; anchors.rightMargin: 24
                spacing: 12
                visible: page.dirty

                Text {
                    text: "Unsaved changes"
                    font.pixelSize: Theme.fontSizeSmall
                    color: Qt.rgba(page.hintColor.r, page.hintColor.g, page.hintColor.b, 0.65)
                    Layout.fillWidth: true; Layout.alignment: Qt.AlignVCenter
                }

                // Discard button
                Rectangle {
                    width: dscLbl.implicitWidth + 20; height: 30; radius: 6
                    color: dscMa.containsMouse ? Qt.rgba(1, 1, 1, 0.08) : "transparent"
                    border.width: 1; border.color: Qt.rgba(1, 1, 1, 0.10)
                    Layout.alignment: Qt.AlignVCenter
                    Behavior on color { ColorAnimation { duration: 100 } }

                    Text {
                        id: dscLbl; anchors.centerIn: parent
                        text: "Discard"
                        font.pixelSize: Theme.fontSizeSmall; font.weight: Font.Medium
                        color: Qt.rgba(page.hintColor.r, page.hintColor.g, page.hintColor.b, 0.7)
                    }
                    MouseArea {
                        id: dscMa; anchors.fill: parent
                        hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: { appConfig.reloadFromDisk(); page.dirty = false }
                    }
                }

                // Apply button
                Rectangle {
                    width: aplLbl.implicitWidth + 24; height: 30; radius: 6
                    color: aplMa.containsMouse ? Qt.lighter(page.accentColor, 1.15) : page.accentColor
                    Layout.alignment: Qt.AlignVCenter
                    Behavior on color { ColorAnimation { duration: 100 } }

                    Text {
                        id: aplLbl; anchors.centerIn: parent
                        text: "Apply"
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

    // ── Confirmation dialog ───────────────────────────────────────────────────
    Dialog {
        id: resetConfirmDialog
        title: "Reset to defaults?"
        width: 320
        anchors.centerIn: Overlay.overlay
        modal: true

        Text {
            text: "All settings will be restored to their default values."
            wrapMode: Text.Wrap; width: 260
            color: page.textColor
            font.pixelSize: Theme.fontSizeBody
        }

        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: page.resetToDefaults()
    }
}
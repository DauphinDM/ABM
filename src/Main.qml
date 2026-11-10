import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "Garage.js" as Garage

ApplicationWindow {
    id: win
    width: 440; height: 700; minimumWidth: 320; minimumHeight: 440
    visible: true; title: "ABM"
    color: "#d90d0e12" // Translucent Omarchy window: desktop shows through.

    property color accent: theme.accent
    property color surface: "#d016171c"
    property color textMain: "#e8e8ed"
    property color textDim: "#9b9ca9"
    property int pad: 10
    property int gap: 8

    property bool metric: backend.metric
    onMetricChanged: backend.metric = metric
    property int page: 0
    property bool helpVisible: false
    readonly property var game: backend.state
    readonly property var telemetry: game.telemetry || ({})
    readonly property bool live: backend.connected
    readonly property var launcher: backend.launcher
    readonly property bool controls: live && ["0.3.0", "0.4.0", "0.5.0", "0.6.0", "0.6.1", "0.6.2", "0.7.0", "0.7.1","0.9.0"].indexOf(game.bridgeVersion) >= 0 && game.controlsAvailable === true
    readonly property bool sandboxAllowed: game.joining !== true && game.policyCleanup !== true && (game.multiplayer !== true || game.sandboxAllowed === true)
    readonly property bool tuningReady: controls && sandboxAllowed && ["0.4.0", "0.5.0", "0.6.0", "0.6.1", "0.6.2", "0.7.0", "0.7.1","0.9.0"].indexOf(game.bridgeVersion) >= 0
    readonly property bool sandboxReady: controls && sandboxAllowed && ["0.5.0", "0.6.0", "0.6.1", "0.6.2", "0.7.0", "0.7.1","0.9.0"].indexOf(game.bridgeVersion) >= 0 && !backend.busy
    readonly property bool engineReady: controls && sandboxAllowed && ["0.6.0", "0.6.1", "0.6.2", "0.7.0", "0.7.1","0.9.0"].indexOf(game.bridgeVersion) >= 0 && !backend.busy
    readonly property bool dimensionsReady: controls && sandboxAllowed && ["0.6.1", "0.6.2", "0.7.0", "0.7.1","0.9.0"].indexOf(game.bridgeVersion) >= 0 && !backend.busy
    readonly property bool gearsReady: controls && sandboxAllowed && ["0.6.2", "0.7.0", "0.7.1","0.9.0"].indexOf(game.bridgeVersion) >= 0 && !backend.busy
    readonly property var workshop: game.workshop || ({})
    readonly property string workshopIdentity: (game.sessionId || "")+":"+(game.vehicleId || "")+":"+(workshop.generation || "")
    onWorkshopIdentityChanged: if (powerInput) powerInput.text=String((workshop.enginePower===undefined ? 1 : workshop.enginePower)*100)
    readonly property bool livePowerReady: controls && sandboxAllowed && ownVehicle && supports("power") && game.bridgeVersion==="0.9.0" && !!workshop.generation
    readonly property var tuning: backend.tuning
    readonly property bool ownVehicle: !!game.vehicle && game.vehicleOwned !== false
    readonly property bool stopped: typeof telemetry.speed === "number" && isFinite(telemetry.speed) && telemetry.speed <= 1
    readonly property var capabilities: game.capabilities || ({})
    function supports(name) { return ownVehicle && capabilities[name] === true }
    readonly property bool canDriveAction: controls && sandboxAllowed && !backend.busy && !demoMode && (!telemetry.speed || telemetry.speed <= 1) && !!game.map
    property var selectedModel: ({})
    property var selectedConfig: ({})
    property var actionTarget: ({})
    property string pendingAction: ""
    property string garageQuery: ""
    property string configSearch: ""
    property string modelFilter: "all"
    property string configFilter: "all"
    property var modelFacets: ({})
    property var configFacets: ({})
    property string garageOrder: "name"
    property bool favoriteOnly: false
    property bool extraFilters: false
    readonly property var garageEntries: selectedModel.id ? (backend.configsModel === selectedModel.id ? backend.configs : []) : backend.catalog
    readonly property var activeFacets: selectedModel.id ? configFacets : modelFacets
    function setFacet(field, value) {
        var next = Object.assign({}, activeFacets); next[field] = value
        if (selectedModel.id) configFacets = next; else modelFacets = next
    }
    function clearGarageFilters() {
        garageQuery = ""; favoriteOnly = false
        if (selectedModel.id) { configFilter = "all"; configFacets = ({}) }
        else { modelFilter = "all"; modelFacets = ({}) }
    }
    property real modelScroll: 0
    property string autoCatalogSession: ""
    property string notice: ""

    property real smoothSpeed: 0
    property real smoothRpm: 0
    property real frameBudget: 0
    property int uiFrames: 0
    property real uiElapsed: 0
    property int uiFps: 0
    // Development aid: --dev-open TARGET pre-opens pages, dialogs and menus.
    property string devOpen: ""
    onDevOpenChanged: {
        const targets = {drive:0, car:1, garage:2, session:3}
        if (targets[devOpen] !== undefined) win.page = targets[devOpen]
        else if (["handling", "gears", "fun", "engine", "dimensions", "blocked"].indexOf(devOpen) >= 0) { win.page = 1; carSection.currentIndex = {handling:1,gears:1,fun:2,engine:0,dimensions:3,blocked:0}[devOpen] }
        else if (devOpen === "owned") ownedDialog.open()
        else if (devOpen === "trip") tripDialog.open()
        else if (devOpen === "diagnostics") diagnostics.open()
        else if (devOpen === "paths") settingsDialog.open()
        else if (devOpen === "help") win.helpVisible = true
        else if (devOpen === "filters") { win.page = 2; filterBox.popup.open() }
        else if (devOpen === "facets") { win.page = 2; win.extraFilters = true }
        else if (["spawn", "replace", "switch"].indexOf(devOpen) >= 0) {
            win.pendingAction = devOpen
            win.actionTarget = {id:"etk800/base", name:"ETK 800 · Factory configuration", vehicleId:42}
            confirmDialog.open()
        }
        else if (devOpen === "garage-menu") { win.page = 2; garageMenu.popup(garageOptions, 0, garageOptions.height) }
        else if (devOpen === "settings-menu") { win.page = 0; settingsMenu.popup(optionsButton, 0, -settingsMenu.height - win.gap) }
    }

    readonly property var garageItems: {
        var entries = Garage.select(garageEntries,activeFacets,selectedModel.id ? configFilter : modelFilter,backend.favorites,garageQuery,garageOrder)
        return favoriteOnly ? entries.filter(function(v) { return backend.favorites.indexOf(v.id) >= 0 }) : entries
    }
    function number(value, decimals) {
        var language = translator.language
        return typeof value === "number" && isFinite(value) ? value.toLocaleString(Qt.locale(), 'f', decimals || 0) : "—"
    }
    function speed(value) { return typeof value === "number" ? number(value * (metric ? 3.6 : 2.236936)) : "—" }
    function time(value) { return typeof value !== "number" ? "—" : number(Math.floor(value/60)) + ":" + ("0" + Math.floor(value%60)).slice(-2) }
    function tell(text) { notice = text; noticeTimer.restart() }
    function inputNumber(text) { return text.trim() === "" ? NaN : Number(text) }
    function applyPowerDraft() {
        backend.applyTuning({enginePower:supports("power") ? powerInput.multiplier : 1,
            gravity:tuning.gravity !== undefined ? tuning.gravity : -9.81,
            damageImmunity:supports("deformation") && damageToggle.checked})
    }
    function submitLivePower() { if (livePowerReady && powerInput.validPower) backend.setLivePower(powerInput.multiplier) }
    function applyGravityDraft(gravity) {
        backend.applyTuning({enginePower:supports("power") && tuning.enginePower !== undefined ? tuning.enginePower : 1,
            gravity:gravity, damageImmunity:supports("deformation") && tuning.damageImmunity === true})
    }
    function inRange(text, low, high) { var n = inputNumber(text); return isFinite(n) && n >= low && n <= high }
    readonly property bool validHandling: (!supports("grip") || inRange(gripInput.text,0,10) && inRange(slidingInput.text,0,10)) && (!supports("brakes") || inRange(brakeInput.text,0,10)) && (!supports("steering") || inRange(steeringInput.text,.1,3))
    readonly property bool validRatios: {
        var text = ratiosInput.text.trim()
        if (!text || text.length > 24000) return false
        var ratios = text.split(/[,;\s]+/).map(Number)
        return ratios.every(function(n) { return isFinite(n) && n >= .01 && n <= 100 }) && JSON.stringify(ratios).length <= 24000
    }
    function maybeCatalog() {
        var session = game.sessionId || ""
        if (page === 2 && controls && !backend.busy && !backend.catalog.length && autoCatalogSession !== session) {
            autoCatalogSession = session; backend.requestCatalog()
        }
    }
    function chooseModel(item) {
        if (backend.busy) return
        modelScroll = garageGrid.contentY
        selectedModel = item; selectedConfig = ({}); garageQuery = configSearch
        backend.requestConfigs(item.id); garageGrid.positionViewAtBeginning()
    }
    function backToModels() {
        configSearch = garageQuery; selectedModel = ({}); selectedConfig = ({})
        Qt.callLater(function() { garageGrid.contentY = Math.min(modelScroll, Math.max(0,garageGrid.contentHeight-garageGrid.height)) })
    }
    function confirmVehicle(action, target) {
        pendingAction = action; actionTarget = Object.assign({}, target); confirmDialog.open()
    }
    onPageChanged: maybeCatalog()
    Connections {
        target: backend
        function onChanged() { win.maybeCatalog(); if (!win.live && !demoMode) { win.selectedConfig = ({}); win.selectedModel = ({}) } }
        function onActionChanged() { if (backend.actionMessage) win.tell(backend.actionMessage) }
    }
    Connections {
        target: win.launcher
        function onChanged() {
            if (win.launcher.message && win.launcher.message !== win.lastLaunchMessage) {
                win.lastLaunchMessage = win.launcher.message; win.tell(win.launcher.message)
            }
        }
    }
    Timer { id: noticeTimer; interval: 8000; onTriggered: win.notice = "" }
    Shortcut { sequence: "?"; context: Qt.ApplicationShortcut; onActivated: win.helpVisible = !win.helpVisible }
    Shortcut { sequence: "Q"; context: Qt.ApplicationShortcut; onActivated: Qt.quit() }
    Shortcut { sequence: "U"; context: Qt.ApplicationShortcut; onActivated: win.metric = !win.metric }
    Shortcut { sequence: "Space"; context: Qt.ApplicationShortcut; onActivated: backend.toggleMonitoring() }
    Shortcut { sequence: "1"; context: Qt.ApplicationShortcut; onActivated: win.page = 0 }
    Shortcut { sequence: "2"; context: Qt.ApplicationShortcut; onActivated: win.page = 1 }
    Shortcut { sequence: "3"; context: Qt.ApplicationShortcut; onActivated: win.page = 2 }
    Shortcut { sequence: "4"; context: Qt.ApplicationShortcut; onActivated: win.page = 3 }
    Shortcut {
        sequence: "Escape"; context: Qt.ApplicationShortcut
        onActivated: {
            if (confirmDialog.visible) confirmDialog.close()
            else if (settingsDialog.visible) settingsDialog.close()
            else if (diagnostics.visible) diagnostics.close()
            else if (tripDialog.visible) tripDialog.close()
            else if (ownedDialog.visible) ownedDialog.close()
            else if (win.helpVisible) win.helpVisible = false
            else if (garageInput.activeFocus && win.garageQuery) win.garageQuery = ""
            else if (win.page === 2 && win.selectedModel.id) win.backToModels()
        }
    }
    FrameAnimation {
        running: win.visible && win.visibility !== Window.Minimized && win.page === 0 && (backend.monitoring || demoMode) && (win.live || demoMode)
        onTriggered: {
            win.frameBudget += frameTime; win.uiElapsed += frameTime
            if (win.frameBudget < 1/60) return
            var dt = Math.min(.1,win.frameBudget); win.frameBudget %= 1/60
            win.smoothSpeed += ((win.telemetry.speed || 0)-win.smoothSpeed)*(1-Math.exp(-dt*22))
            win.smoothRpm += ((win.telemetry.rpm || 0)-win.smoothRpm)*(1-Math.exp(-dt*22))
            win.uiFrames++
            if (win.uiElapsed >= 1) { win.uiFps = Math.round(win.uiFrames/win.uiElapsed); win.uiFrames=0; win.uiElapsed=0 }
        }
    }
    Component.onCompleted: { smoothSpeed=telemetry.speed || 0; smoothRpm=telemetry.rpm || 0; maybeCatalog() }
    Item {
        anchors.fill: parent
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true
        ColumnLayout {
            anchors.fill: parent; anchors.margins: pad; spacing: gap
            // Header: app name + launch button
            RowLayout {
                Layout.fillWidth: true; spacing: gap
                PlainLabel { text: "ABM"; font.pixelSize: 13; font.weight: Font.DemiBold; color: accent; Layout.fillWidth: true }
                Button {
                    text: win.launcher.launching ? qsTr("Starting…") : win.launcher.gameRunning ? (win.launcher.beammpRunning ? qsTr("BeamMP running") : qsTr("Start BeamMP")) : qsTr("Start BeamNG")
                    font.pixelSize: 11; padding: 6; implicitHeight: 28
                    enabled: !demoMode && !backend.busy && !win.launcher.launching && (!win.launcher.gameRunning || !win.launcher.beammpRunning)
                    onClicked: backend.launchAuto()
                    ToolTip.visible: hovered
                    ToolTip.text: win.launcher.gameRunning ? qsTr("Enable BeamMP and start its launcher with --no-launch") : qsTr("Install the bridge and start BeamNG through Steam")
                }
            }
            // Status line
            RowLayout {
                Layout.fillWidth: true; spacing: gap
                Rectangle { width: 6; height: 6; radius: 3; color: win.live ? accent : "#555966" }
                PlainLabel {
                    Layout.fillWidth: true; maximumLineCount: 1; elide: Text.ElideRight; color: textDim; font.pixelSize: 10
                    text: demoMode ? qsTr("DEMO · Sample data") : !backend.monitoring ? qsTr("Display paused") : win.live
                        ? (win.game.multiplayer ? win.game.server || "BeamMP" : qsTr("Single player"))
                        : win.launcher.gameRunning ? qsTr("Game running · waiting for bridge") : qsTr("Game closed")
                }
            }
            Rectangle {
                visible: win.live && !win.sandboxAllowed
                Layout.fillWidth: true; implicitHeight: policyText.implicitHeight + 14
                color: "#d02a2117"; border.color: "#c59b55"; border.width: 1
                PlainLabel { id: policyText; objectName: "serverPolicyBanner"; anchors.fill: parent; anchors.margins: 7
                    text: qsTr("Gameplay locked · %1").arg(win.game.policyReason || qsTr("Install bridge 0.7 and restart BeamNG for server verification."))
                    color: "#f0d3a0"; wrapMode: Text.Wrap; font.pixelSize: 10 }
            }
            // Navigation
            RowLayout {
                Layout.fillWidth: true; spacing: gap
                Button { text: qsTr("Drive"); font.pixelSize: 11; padding: 6; implicitHeight: 28; checkable: true; checked: win.page === 0; onClicked: win.page = 0
                    background: Rectangle { color: win.page === 0 ? accent : surface; radius: 0 } }
                Button { text: qsTr("Car"); font.pixelSize: 11; padding: 6; implicitHeight: 28; checkable: true; checked: win.page === 1; onClicked: win.page = 1
                    background: Rectangle { color: win.page === 1 ? accent : surface; radius: 0 } }
                Button { text: qsTr("Garage"); font.pixelSize: 11; padding: 6; implicitHeight: 28; checkable: true; checked: win.page === 2; onClicked: win.page = 2
                    background: Rectangle { color: win.page === 2 ? accent : surface; radius: 0 } }
                Button { text: qsTr("Session"); font.pixelSize: 11; padding: 6; implicitHeight: 28; checkable: true; checked: win.page === 3; onClicked: win.page = 3
                    background: Rectangle { color: win.page === 3 ? accent : surface; radius: 0 } }
            }
            StackLayout {
                currentIndex: win.page; Layout.fillWidth: true; Layout.fillHeight: true
                // DRIVE
                ScrollView {
                    clip: true; contentWidth: availableWidth
                    ColumnLayout {
                        width: parent.width; spacing: gap
                        PlainLabel { Layout.fillWidth: true; text: win.game.vehicleName || win.game.vehicle || qsTr("No active vehicle"); font.pixelSize: 13; horizontalAlignment: Text.AlignHCenter; color: textMain }
                        DriveGauge { visible: win.live; Layout.fillWidth: true; rpm: win.smoothRpm; speed: win.smoothSpeed; redline: win.telemetry.redline || 8000; speedText: win.telemetry.speed === undefined ? "—" : win.speed(win.smoothSpeed); unit: win.metric ? "km/h" : "mph"; gearText: win.telemetry.gear === undefined ? "—" : win.telemetry.gear === 0 ? "N" : win.telemetry.gear < 0 ? "R" : win.number(win.telemetry.gear) }
                        PlainLabel { visible: !win.live; Layout.fillWidth: true; text: win.launcher.gameRunning ? qsTr("The game is open, but its bridge is not connected.") : qsTr("Start BeamNG above."); color: textDim; font.pixelSize: 10 }
                        RowLayout { visible: win.live; Layout.fillWidth: true; spacing: gap
                            MetricCard { label: qsTr("FUEL"); value: win.number(win.telemetry.fuel === undefined ? undefined : win.telemetry.fuel*100); unit: "%" }
                            MetricCard { label: qsTr("ENGINE"); value: win.number(win.smoothRpm); unit: "RPM" }
                        }
                        PlainLabel { visible: (win.telemetry.fuel !== undefined && win.telemetry.fuel < .1) || (win.telemetry.coolant || 0) > 110; text: (win.telemetry.coolant || 0) > 110 ? qsTr("High coolant temperature") : qsTr("Low fuel"); color: "#cf8a4a"; font.pixelSize: 10 }
                        PlainLabel { visible: win.live; text: qsTr("SPEED HISTORY · LAST 60 SECONDS"); color: textDim; font.pixelSize: 9 }
                        TrendChart { visible: win.live; Layout.fillWidth: true; Layout.preferredHeight: 76; samples: backend.history; multiplier: win.metric ? 3.6 : 2.23694; ceiling: 60 }
                        RowLayout { visible: win.live; Layout.fillWidth: true; spacing: gap
                            MetricCard { label: qsTr("COOLANT"); value: win.number(win.telemetry.coolant); unit: "°C" }
                            MetricCard { label: qsTr("BOOST"); value: win.number(win.telemetry.boost,1); unit: "bar" }
                        }
                        Button { visible: win.live; text: qsTr("Vehicle details"); Layout.fillWidth: true; font.pixelSize: 11; padding: 6; implicitHeight: 30; flat: true; onClicked: diagnostics.open() }
                        RowLayout { Layout.fillWidth: true; spacing: gap
                            Button { id: optionsButton; text: qsTr("Options"); Layout.fillWidth: true; font.pixelSize: 11; padding: 6; implicitHeight: 30; flat: true; onClicked: settingsMenu.popup(optionsButton, 0, -settingsMenu.height - win.gap) }
                        }
                    }
                }
                // CAR — tune the vehicle you are currently driving: engine power,
                // gravity, deformation immunity, plus a few safe fun commands.
                ScrollView {
                    clip: true; contentWidth: availableWidth
                    ColumnLayout {
                        width: parent.width; spacing: gap
                        PlainLabel { text: qsTr("Sandbox workshop"); font.pixelSize: 13; color: textMain }
                        PlainLabel { visible: !win.controls; Layout.fillWidth: true; text: win.launcher.gameRunning ? qsTr("The game is open, but its bridge is not connected.") : qsTr("Connect the bridge (game or settings) to tune your vehicle."); color: textDim; font.pixelSize: 10; wrapMode: Text.Wrap }
                        PlainLabel { visible: win.controls && win.sandboxAllowed && !win.tuningReady; Layout.fillWidth: true; text: qsTr("Restart BeamNG once so the bridge with tuning support loads."); color: textDim; font.pixelSize: 10; wrapMode: Text.Wrap }
                        RowLayout { visible: win.controls; Layout.fillWidth: true; spacing: gap
                            MetricCard { Layout.fillWidth: true; label: qsTr("VEHICLE"); value: (win.game.vehicleName || win.game.vehicle || "—"); unit: (win.telemetry.speed !== undefined ? win.speed(win.telemetry.speed) + " " + (win.metric ? "km/h" : "mph") : "parked or unknown") }
                            MetricCard { Layout.fillWidth: true; label: qsTr("GEAR"); value: win.telemetry.gear === undefined ? "—" : win.telemetry.gear === 0 ? "N" : win.telemetry.gear < 0 ? "R" : win.number(win.telemetry.gear); unit: qsTr("RPM %1").arg(win.telemetry.rpm === undefined ? "—" : win.number(win.smoothRpm)) }
                        }
                        ComboBox { id: carSection; objectName: "carSection"; Layout.fillWidth: true; visible: win.controls; font.pixelSize: 11
                            model: [qsTr("Engine & protection"), qsTr("Handling & gearbox"), qsTr("World & recovery"), qsTr("Body · experimental")] }
                        PlainLabel { visible: win.controls && !win.ownVehicle; Layout.fillWidth: true; text: qsTr("Select your own vehicle to use the workshop."); color: textDim; font.pixelSize: 10; wrapMode: Text.Wrap }
                        PlainLabel { visible: win.controls && win.ownVehicle && Object.keys(win.capabilities).length < 3; Layout.fillWidth: true; text: qsTr("Waiting for vehicle capabilities. Install the updated bridge and restart BeamNG if controls stay disabled."); color: textDim; font.pixelSize: 10; wrapMode: Text.Wrap }
                        ColumnLayout { visible: win.controls && carSection.currentIndex === 0; Layout.fillWidth: true; spacing: 2
                            RowLayout { Layout.fillWidth: true
                                PlainLabel { Layout.fillWidth: true; text: qsTr("Engine power"); font.pixelSize: 11; color: textMain }
                                TextField { id: powerInput; objectName: "powerInput"; Layout.preferredWidth: 112; implicitHeight: 30; font.pixelSize: 11; color: textMain
                                    text: String((win.workshop.enginePower !== undefined ? win.workshop.enginePower : 1) * 100)
                                    selectByMouse: true; enabled: win.livePowerReady
                                    property string editingTarget: ""
                                    onActiveFocusChanged: if (activeFocus) editingTarget=win.workshopIdentity
                                    onEditingFinished: if (editingTarget===win.workshopIdentity) win.submitLivePower()
                                    onTextEdited: if (!validPower) backend.cancelPowerEdits()
                                    property real multiplier: text.trim() === "" ? NaN : Number(text) / 100
                                    property bool validPower: isFinite(multiplier) && multiplier > 0
                                    background: Rectangle { color: surface; border.width: 1; border.color: powerInput.validPower ? "#474952" : "#df6b61" }
                                }
                                PlainLabel { text: "%"; font.pixelSize: 11; color: accent }
                            }
                            Slider {
                                id: powerSlider; Layout.fillWidth: true; implicitHeight: 24
                                from: 0.5; to: 3; stepSize: 0.05
                                value: Math.max(from, Math.min(to, powerInput.multiplier || 1))
                                onMoved: { powerInput.text = String(Math.round(value * 100)); win.submitLivePower() }
                                enabled: powerInput.enabled
                                background: Rectangle { y: (powerSlider.height - 5) / 2; height: 5; width: powerSlider.availableWidth; color: "#26282f"
                                    Rectangle { width: parent.width * powerSlider.position; height: parent.height; color: accent } }
                                handle: Rectangle { width: 14; height: 14; color: powerSlider.pressed || powerSlider.hovered ? accent : textMain; border.width: 1; border.color: "#0e0f13"
                                    x: powerSlider.visualPosition * (powerSlider.availableWidth - width); y: (powerSlider.height - height) / 2 }
                            }
                            PlainLabel { Layout.fillWidth: true; text: win.livePowerReady ? qsTr("Live power · drag or finish typing to apply. Latest value wins, at most 4 requests/s. No beam/gravity changes. Overflow rejected.") : !win.sandboxAllowed ? qsTr("Gameplay locked by server policy. Live power edits are canceled.") : qsTr("Live power needs a supported engine and bridge 0.9.0. Restart BeamNG after installing."); font.pixelSize: 10; color: textDim; wrapMode: Text.Wrap }
                            PlainLabel { Layout.fillWidth: true; text: win.workshop.enginePower===undefined ? qsTr("Accepted power: unknown until ABM applies a value.") : qsTr("Vehicle readback: %1% accepted torque scale").arg(win.number(win.workshop.enginePower*100)); font.pixelSize: 10; color: accent; wrapMode: Text.Wrap }
                            Repeater { model: win.workshop.engines || []
                                PlainLabel { required property var modelData; Layout.fillWidth: true; text: qsTr("%1 · max %2 RPM · limiter %3 RPM").arg(modelData.name).arg(win.number(modelData.maxRPM)).arg(win.number(modelData.revLimiterRPM)); font.pixelSize: 10; color: textDim; wrapMode: Text.Wrap }
                            }
                        }
                        ColumnLayout { visible: win.controls && carSection.currentIndex === 2; Layout.fillWidth: true; spacing: 2
                            RowLayout { Layout.fillWidth: true
                                PlainLabel { Layout.fillWidth: true; text: qsTr("Gravity"); font.pixelSize: 11; color: textMain }
                                PlainLabel { text: gravitySlider.value.toFixed(2) + " m/s²"; font.pixelSize: 11; color: accent }
                            }
                            Slider {
                                id: gravitySlider; Layout.fillWidth: true; implicitHeight: 24
                                from: -30; to: 0; stepSize: 0.01
                                value: win.tuning.gravity !== undefined ? win.tuning.gravity : -9.81
                                enabled: win.tuningReady && !backend.busy && win.ownVehicle
                                background: Rectangle { y: (gravitySlider.height - 5) / 2; height: 5; width: gravitySlider.availableWidth; color: "#26282f"
                                    Rectangle { width: parent.width * gravitySlider.position; height: parent.height; color: accent } }
                                handle: Rectangle { width: 14; height: 14; color: gravitySlider.pressed || gravitySlider.hovered ? accent : textMain; border.width: 1; border.color: "#0e0f13"
                                    x: gravitySlider.visualPosition * (gravitySlider.availableWidth - width); y: (gravitySlider.height - height) / 2 }
                            }
                            Flow { Layout.fillWidth: true; spacing: 4
                                Repeater {
                                    model: [{label: qsTr("Earth"), g: -9.81}, {label: qsTr("Moon"), g: -1.62}, {label: qsTr("Mars"), g: -3.71}, {label: qsTr("Venus"), g: -8.87}, {label: qsTr("Jupiter"), g: -24.92}, {label: qsTr("Pluto"), g: -0.58}, {label: qsTr("Zero-g"), g: 0}]
                                    Button { required property string label; required property real g; text: label; font.pixelSize: 10; padding: 5; implicitHeight: 24; flat: true
                                        enabled: gravitySlider.enabled && Math.abs(gravitySlider.value-g) > .005
                                        onClicked: { gravitySlider.value = g; win.applyGravityDraft(g) } }
                                }
                            }
                            PlainLabel { Layout.fillWidth: true; text: qsTr("Applies to the entire world. Earth is −9.81 m/s²."); font.pixelSize: 10; color: textDim; wrapMode: Text.Wrap }
                             Button { text: qsTr("Apply gravity"); Layout.fillWidth: true; implicitHeight: 30; font.pixelSize: 11; enabled: gravitySlider.enabled
                                 onClicked: win.applyGravityDraft(gravitySlider.value) }
                        }
                        Button {
                            id: damageToggle; visible: win.controls && carSection.currentIndex === 0; Layout.fillWidth: true; checkable: true; checked: win.tuning.damageImmunity === true
                            text: checked ? qsTr("Deformation immunity: ON") : qsTr("Deformation immunity")
                            font.pixelSize: 11; padding: 6; implicitHeight: 30; enabled: win.tuningReady && !backend.busy && win.supports("deformation")
                            background: Rectangle { color: damageToggle.enabled && damageToggle.checked ? accent : surface; radius: 0 }
                        }
                        PlainLabel { visible: win.controls && carSection.currentIndex === 0; Layout.fillWidth: true; text: qsTr("Beam protection changes deformation/strength limits, not engine or tire damage."); font.pixelSize: 10; color: textDim; wrapMode: Text.Wrap }
                        Button { text: qsTr("Apply beam limits"); visible: win.controls && carSection.currentIndex === 0; Layout.fillWidth: true; implicitHeight: 30; font.pixelSize: 11
                              enabled: win.tuningReady && !backend.busy && win.ownVehicle && win.supports("deformation") && (win.game.bridgeVersion!=="0.9.0" || win.workshop.enginePower!==undefined)
                              onClicked: backend.applyTuning({enginePower:win.workshop.enginePower===undefined ? 1 : win.workshop.enginePower,gravity:win.tuning.gravity===undefined ? -9.81 : win.tuning.gravity,damageImmunity:damageToggle.checked}) }
                         ColumnLayout { visible: win.controls && carSection.currentIndex === 1; Layout.fillWidth: true; spacing: 4
                            PlainLabel { text: qsTr("Grip & handling"); font.pixelSize: 12; color: accent }
                             GridLayout { Layout.fillWidth: true; columns: 2; columnSpacing: gap; rowSpacing: 4
                                PlainLabel { text: qsTr("Tire grip ×"); color: textMain; font.pixelSize: 11 }
                                 TextField { id: gripInput; Layout.fillWidth: true; text: "1"; selectByMouse: true; font.pixelSize: 11; implicitHeight: 28; padding: 4; enabled: win.sandboxReady && win.supports("grip") }
                                PlainLabel { text: qsTr("Sliding grip ×"); color: textMain; font.pixelSize: 11 }
                                 TextField { id: slidingInput; Layout.fillWidth: true; text: "1"; selectByMouse: true; font.pixelSize: 11; implicitHeight: 28; padding: 4; enabled: gripInput.enabled }
                                PlainLabel { text: qsTr("Brake strength ×"); color: textMain; font.pixelSize: 11 }
                                 TextField { id: brakeInput; Layout.fillWidth: true; text: "1"; selectByMouse: true; font.pixelSize: 11; implicitHeight: 28; padding: 4; enabled: win.sandboxReady && win.supports("brakes") }
                                PlainLabel { text: qsTr("Steering response ×"); color: textMain; font.pixelSize: 11 }
                                 TextField { id: steeringInput; Layout.fillWidth: true; text: "1"; selectByMouse: true; font.pixelSize: 11; implicitHeight: 28; padding: 4; enabled: win.sandboxReady && win.supports("steering") }
                            }
                             PlainLabel { Layout.fillWidth: true; text: qsTr("1 = stock. Grip/brakes 0–10×; steering 0.1–3× within physical limits. Skips damaged tires; other controllers may override."); font.pixelSize: 10; color: textDim; wrapMode: Text.Wrap }
                            Button { text: qsTr("Apply handling"); objectName: "handlingApplyButton"; Layout.fillWidth: true; implicitHeight: 30; font.pixelSize: 11; enabled: win.sandboxReady && win.validHandling && (win.supports("grip") || win.supports("brakes") || win.supports("steering"))
                                onClicked: backend.applyHandling({tireGrip:win.supports("grip") ? win.inputNumber(gripInput.text) : 1, slidingGrip:win.supports("grip") ? win.inputNumber(slidingInput.text) : 1, brakePower:win.supports("brakes") ? win.inputNumber(brakeInput.text) : 1, steeringResponse:win.supports("steering") ? win.inputNumber(steeringInput.text) : 1}) }
                            Button { text: qsTr("Cycle ESC / traction mode"); objectName: "stabilityButton"; Layout.fillWidth: true; implicitHeight: 30; font.pixelSize: 11; enabled: win.sandboxReady && win.supports("stability"); onClicked: backend.carAction("stability") }
                             PlainLabel { Layout.fillWidth: true; text: qsTr("ESC uses the car's own mode cycle. Reset restores stock handling."); font.pixelSize: 10; color: textDim; wrapMode: Text.Wrap }
                        }
                         ColumnLayout { visible: win.controls && carSection.currentIndex === 1; Layout.fillWidth: true; spacing: 4
                            PlainLabel { text: qsTr("Actual gearbox & edited ratios"); font.pixelSize: 12; color: accent }
                            GearEditor { id: ratiosInput; objectName: "ratiosInput"; Layout.fillWidth: true; current: win.workshop.gears || []; generation: win.workshopIdentity; enabled: win.gearsReady && win.supports("gears") && !!win.workshop.generation && current.length>0; foreground: textMain; muted: textDim; surface: win.surface }
                            PlainLabel { visible: !ratiosInput.current.length; Layout.fillWidth: true; text: qsTr("Actual manual ratios unavailable. Install bridge 0.9.0; unsupported or oversized readback is not replaced with sample ratios."); font.pixelSize: 10; color: textDim; wrapMode: Text.Wrap }
                            RowLayout { Layout.fillWidth: true
                                Button { text: qsTr("+ Gear"); objectName: "addGearButton"; font.pixelSize: 11; implicitHeight: 30; enabled: ratiosInput.enabled && ratiosInput.canSuggest && win.validRatios && ratiosInput.text.length < 23980; onClicked: ratiosInput.addGear() }
                                Button { text: qsTr("Apply ratios"); objectName: "gearsApplyButton"; Layout.fillWidth: true; font.pixelSize: 11; implicitHeight: 30; enabled: ratiosInput.enabled && win.validRatios && win.stopped; onClicked: backend.applyGears(ratiosInput.text) }
                            }
                            RowLayout { Layout.fillWidth: true
                                Button { text: qsTr("Read actual"); font.pixelSize: 10; enabled: ratiosInput.enabled; onClicked: ratiosInput.loadCurrent() }
                                Button { text: qsTr("Remove last"); font.pixelSize: 10; enabled: ratiosInput.enabled && ratiosInput.forward.length>1; onClicked: ratiosInput.text=ratiosInput.forward.slice(0,-1).join(", ") }
                            }
                            PlainLabel { Layout.fillWidth: true; text: qsTr("+ Gear suggests the median of the last three valid ratio gaps—not a guaranteed optimum. Edit before applying. 24 KB command budget; ratios 0.01–100."); font.pixelSize: 10; color: textDim; wrapMode: Text.Wrap }
                             PlainLabel { Layout.fillWidth: true; text: qsTr("Stop first. Supported manual gearboxes only, not automatic/DCT/CVT. Rebuilds synchronizers and shift points in neutral. Extra gears inherit top-gear thresholds. Reset restores stock."); font.pixelSize: 10; color: textDim; wrapMode: Text.Wrap }
                        }
                        GridLayout { visible: win.controls && carSection.currentIndex === 2; Layout.fillWidth: true; columns: 2; columnSpacing: gap; rowSpacing: gap
                            Button { text: qsTr("Why This Dealer?"); Layout.fillWidth: true; font.pixelSize: 11; implicitHeight: 32; enabled: win.sandboxReady && win.supports("dealer"); onClicked: backend.carAction("dealer") }
                            Button { text: qsTr("Stop bouncing"); objectName: "dealerStopButton"; Layout.fillWidth: true; font.pixelSize: 11; implicitHeight: 32; enabled: win.controls && win.supports("dealer") && ["0.6.1", "0.6.2", "0.7.0", "0.7.1","0.9.0"].indexOf(win.game.bridgeVersion) >= 0 && !backend.busy; onClicked: backend.carAction("dealerStop") }
                            PlainLabel { Layout.columnSpan: 2; Layout.fillWidth: true; text: qsTr("The real easter egg: why(\"this dealer\") starts; why(\"stop\") ends it without a reset. Stop needs bridge 0.6.1. Only game builds that expose this API."); font.pixelSize: 10; color: textDim; wrapMode: Text.Wrap }
                            Button { text: qsTr("Repair"); objectName: "repairButton"; Layout.fillWidth: true; font.pixelSize: 11; implicitHeight: 30; enabled: win.tuningReady && !backend.busy && win.supports("repair") && win.stopped; onClicked: backend.carAction("repair") }
                            Button { text: qsTr("Fire"); Layout.fillWidth: true; font.pixelSize: 11; implicitHeight: 30; enabled: win.tuningReady && !backend.busy && win.supports("ignite"); onClicked: backend.carAction("ignite") }
                            Button { text: qsTr("Extinguish"); Layout.fillWidth: true; font.pixelSize: 11; implicitHeight: 30; enabled: win.tuningReady && !backend.busy && win.supports("extinguish"); onClicked: backend.carAction("extinguish") }
                            Button { text: qsTr("Deflate tires"); Layout.fillWidth: true; font.pixelSize: 11; implicitHeight: 30; enabled: win.tuningReady && !backend.busy && win.supports("deflate"); onClicked: backend.carAction("deflate") }
                        }
                        ColumnLayout { visible: win.controls && carSection.currentIndex === 0; Layout.fillWidth: true; spacing: 4
                            RowLayout { Layout.fillWidth: true
                                PlainLabel { Layout.fillWidth: true; text: qsTr("Engine strength"); font.pixelSize: 12; color: accent }
                                PlainLabel { text: strengthSlider.value.toFixed(1) + "×"; font.pixelSize: 13; color: textMain }
                            }
                            Slider { id: strengthSlider; objectName: "strengthSlider"; Layout.fillWidth: true; implicitHeight: 24
                                from: 0.1; to: 100; value: 1; stepSize: 0.1; enabled: win.engineReady && win.supports("engine") && !engineToggle.checked
                                background: Rectangle { y: (strengthSlider.height - 5) / 2; height: 5; width: strengthSlider.availableWidth; color: "#26282f"
                                    Rectangle { width: parent.width * strengthSlider.position; height: parent.height; color: accent } }
                                handle: Rectangle { width: 14; height: 14; color: strengthSlider.pressed || strengthSlider.hovered ? accent : textMain; border.width: 1; border.color: "#0e0f13"
                                    x: strengthSlider.visualPosition * (strengthSlider.availableWidth - width); y: (strengthSlider.height - height) / 2 }
                            }
                            PlainLabel { Layout.fillWidth: true; text: qsTr("0.1–100× damage tolerance · 1× = stock. Does not change power or the rev limiter."); font.pixelSize: 10; color: textDim; wrapMode: Text.Wrap }
                            Button { id: engineToggle; objectName: "engineToggle"; Layout.fillWidth: true; checkable: true; enabled: win.engineReady && win.supports("engine")
                                text: checked ? qsTr("Indestructible engine: ON") : qsTr("Indestructible engine: OFF")
                                font.pixelSize: 11; implicitHeight: 32
                                onClicked: backend.applyEngine({engineStrength:strengthSlider.value, engineIndestructible:checked})
                            }
                            PlainLabel { Layout.fillWidth: true; text: qsTr("Protection blocks supported engine damage and hydrolocking; freezes heat simulation. No repair or tire/mount/turbo/drivetrain protection."); font.pixelSize: 10; color: textDim; wrapMode: Text.Wrap }
                             RowLayout { Layout.fillWidth: true; spacing: gap
                                 Button { text: qsTr("Apply strength"); Layout.fillWidth: true; implicitHeight: 30; font.pixelSize: 11; enabled: strengthSlider.enabled
                                     onClicked: backend.applyEngine({engineStrength:strengthSlider.value, engineIndestructible:engineToggle.checked}) }
                                 Button { text: qsTr("Stock limits"); Layout.fillWidth: true; implicitHeight: 30; font.pixelSize: 11; enabled: win.engineReady && win.supports("engine") && (engineToggle.checked || Math.abs(strengthSlider.value-1) > .01)
                                     onClicked: { strengthSlider.value = 1; engineToggle.checked = false; backend.applyEngine({engineStrength:1, engineIndestructible:false}) } }
                             }
                            PlainLabel { Layout.fillWidth: true; text: qsTr("OFF restores damage at the selected strength. Reset restores stock engine limits."); font.pixelSize: 10; color: textDim; wrapMode: Text.Wrap }
                        }
                        ColumnLayout { visible: win.controls && carSection.currentIndex === 3; Layout.fillWidth: true; spacing: gap
                            PlainLabel { text: qsTr("Physical dimensions · experimental"); font.pixelSize: 12; color: accent }
                            PlainLabel { Layout.fillWidth: true; text: qsTr("Width, length and height relative to the first captured shape. 1× = captured size. Nodes and beam rest lengths change together; this is not a visual-only stretch."); font.pixelSize: 10; color: textDim; wrapMode: Text.Wrap }
                            Repeater { id: dimensionSliders; model: [qsTr("Width"), qsTr("Length"), qsTr("Height")]
                                ColumnLayout { required property string modelData; Layout.fillWidth: true; spacing: 2
                                    property alias factor: dimensionSlider.value
                                    RowLayout { Layout.fillWidth: true
                                        PlainLabel { text: modelData; Layout.fillWidth: true; color: textMain; font.pixelSize: 11 }
                                        PlainLabel { text: dimensionSlider.value.toFixed(2) + "×"; color: accent; font.pixelSize: 11 }
                                    }
                                    Slider { id: dimensionSlider; Layout.fillWidth: true; implicitHeight: 26; from: .5; to: 2; value: 1; stepSize: .01; enabled: win.dimensionsReady && win.supports("dimensions") && win.stopped
                                        background: Rectangle { y: (dimensionSlider.height-5)/2; height: 5; width: dimensionSlider.availableWidth; color: "#26282f"
                                            Rectangle { width: parent.width*dimensionSlider.position; height: parent.height; color: accent } }
                                        handle: Rectangle { width: 14; height: 14; color: dimensionSlider.pressed || dimensionSlider.hovered ? accent : textMain; border.width: 1; border.color: "#0e0f13"
                                            x: dimensionSlider.visualPosition*(dimensionSlider.availableWidth-width); y: (dimensionSlider.height-height)/2 }
                                    }
                                }
                            }
                            PlainLabel { Layout.fillWidth: true; text: qsTr("0.5–2× each axis. Stop and repair broken beams first. Your own car only in BeamMP (bridge 0.6.2): resized geometry is NOT synchronized to other players. Follow server rules. Suspension, wheels and props may fight resizing; mass/tire parameters stay unchanged. Reset restores stock."); font.pixelSize: 10; color: textDim; wrapMode: Text.Wrap }
                            CheckBox { id: dimensionConsent; text: qsTr("I accept unstable physics and vehicle damage"); font.pixelSize: 10; Layout.fillWidth: true; enabled: win.dimensionsReady && win.supports("dimensions") && win.stopped }
                            Button { text: qsTr("Apply physical dimensions"); objectName: "dimensionsApplyButton"; Layout.fillWidth: true; implicitHeight: 32; font.pixelSize: 11
                                enabled: dimensionConsent.enabled && (win.game.multiplayer !== true || ["0.6.2", "0.7.0", "0.7.1","0.9.0"].indexOf(win.game.bridgeVersion) >= 0) && dimensionConsent.checked
                                onClicked: backend.applyDimensions({width:dimensionSliders.itemAt(0).factor,length:dimensionSliders.itemAt(1).factor,height:dimensionSliders.itemAt(2).factor}) }
                            PlainLabel { visible: !win.dimensionsReady; Layout.fillWidth: true; text: qsTr("Multiplayer changes require bridge 0.7 and server permission. Restart BeamNG after installing the bridge."); font.pixelSize: 10; color: textDim; wrapMode: Text.Wrap }
                        }
                        PlainLabel { visible: win.controls; Layout.fillWidth: true; text: qsTr("Engine/gear readback is labeled separately; other controls show drafts, not measured settings. Saved configs are unchanged. Servers or other mods may override changes."); font.pixelSize: 9; color: textDim; wrapMode: Text.Wrap }
                        RowLayout { visible: win.controls && carSection.currentIndex === 0 && win.tuning.enginePower !== undefined; Layout.fillWidth: true; spacing: gap
                            Button { text: qsTr("Engine 100%"); Layout.fillWidth: true; font.pixelSize: 11; padding: 6; implicitHeight: 30; flat: true
                                enabled: win.tuningReady && !backend.busy && win.supports("power") && powerInput.multiplier !== 1
                                  onClicked: { powerInput.text = "100"; win.submitLivePower() } }
                        }
                        Button { text: qsTr("Full reset · clear ABM vehicle tuning"); objectName: "fullResetButton"; Layout.fillWidth: true; implicitHeight: 34; font.pixelSize: 11
                            enabled: win.controls && win.sandboxAllowed && win.ownVehicle && win.stopped && win.game.bridgeVersion==="0.9.0" && !!win.workshop.generation && (!backend.busy || backend.pendingAction==="power")
                            onClicked: resetVehicleDialog.open() }
                    }
                }
                // GARAGE
                ColumnLayout { spacing: gap
                    RowLayout {
                        Layout.fillWidth: true
                        Button { visible: !!win.selectedModel.id; text: qsTr("‹ Models"); font.pixelSize: 11; padding: 6; implicitHeight: 30; flat: true; onClicked: win.backToModels() }
                        PlainLabel { Layout.fillWidth: true; text: win.selectedModel.id ? ((win.selectedModel.brand || "") + " " + win.selectedModel.name).trim() : qsTr("Choose a model"); font.pixelSize: 13; maximumLineCount: 1; elide: Text.ElideRight; color: textMain }
                        Button { id: garageOptions; text: "⋯"; font.pixelSize: 11; padding: 6; implicitHeight: 30; flat: true; onClicked: garageMenu.popup(garageOptions, 0, height) }
                    }
                    TextField {
                        id: garageInput; objectName: "garageSearch"; Layout.fillWidth: true
                        placeholderText: win.selectedModel.id ? qsTr("Search configurations") : qsTr("Search models")
                        text: win.garageQuery; onTextEdited: win.garageQuery = text
                        font.pixelSize: 11; padding: 6; implicitHeight: 32; rightPadding: 30
                        Button { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: "×"; font.pixelSize: 11; width: 28; height: 28; visible: !!garageInput.text; flat: true; onClicked: win.garageQuery = "" }
                    }
                    RowLayout {
                        Layout.fillWidth: true; spacing: gap
                        ComboBox {
                            id: filterBox; objectName: "garageView"; Layout.fillWidth: true; Layout.minimumWidth: 0; textRole: "label"; valueRole: "value"
                            model: [{label:qsTr("All content"),value:"all"},{label:qsTr("Custom configs"),value:"custom"},{label:qsTr("Mod content"),value:"mods"},{label:qsTr("Stock content"),value:"stock"}]
                            currentIndex: { var filter=win.selectedModel.id ? win.configFilter : win.modelFilter; for(var i=0;i<model.length;i++) if(model[i].value===filter) return i; return 0 }
                            onActivated: { if(win.selectedModel.id) win.configFilter=currentValue; else win.modelFilter=currentValue }
                            font.pixelSize: 10; padding: 4; implicitHeight: 28
                        }
                        GarageFilter { objectName: "garageType"; Layout.fillWidth: true; Layout.minimumWidth: 0; entries: win.garageEntries; field: "type"; allLabel: qsTr("All vehicle types"); selectedValue: win.activeFacets.type || ""; onChosen: function(value) { win.setFacet("type",value) } }
                        Button { text: "★"; checkable: true; checked: win.favoriteOnly; onClicked: win.favoriteOnly = checked; implicitHeight: 28; implicitWidth: 30; padding: 4; font.pixelSize: 12; ToolTip.visible: hovered; ToolTip.text: qsTr("Favorites only") }
                    }
                    RowLayout { Layout.fillWidth: true; spacing: gap
                        Button { text: win.extraFilters ? qsTr("Less filters") : qsTr("More filters"); checked: win.extraFilters; checkable: true; font.pixelSize: 10; implicitHeight: 26; padding: 4; onClicked: win.extraFilters = checked }
                        Button { text: qsTr("Clear"); font.pixelSize: 10; implicitHeight: 26; padding: 4; enabled: !!win.garageQuery || win.favoriteOnly || (win.selectedModel.id ? win.configFilter : win.modelFilter) !== "all" || Object.keys(win.activeFacets).some(function(k) { return !!win.activeFacets[k] })
                             onClicked: win.clearGarageFilters() }
                        Item { Layout.fillWidth: true }
                        PlainLabel { text: qsTr("%1 / %2").arg(win.number(win.garageItems.length)).arg(win.number(win.garageEntries.length)); color: textDim; font.pixelSize: 10 }
                        ComboBox { objectName: "garageSort"; textRole: "label"; valueRole: "value"; font.pixelSize: 10; implicitHeight: 26; padding: 4; Layout.preferredWidth: 108
                            model: [{label:qsTr("Name A–Z"),value:"name"},{label:qsTr("Name Z–A"),value:"reverse"},{label:qsTr("Brand"),value:"brand"},{label:qsTr("Type"),value:"type"},{label:qsTr("Source"),value:"source"},{label:qsTr("Favorites first"),value:"favorites"}]
                            onActivated: win.garageOrder = currentValue }
                    }
                    GridLayout { visible: win.extraFilters; Layout.fillWidth: true; columns: 2; columnSpacing: gap; rowSpacing: 4
                        Repeater { model: [{field:"brand",label:qsTr("All brands")},{field:"bodyStyle",label:qsTr("All body styles")},{field:"configType",label:qsTr("All config types")},{field:"drivetrain",label:qsTr("All drivetrains")},{field:"transmission",label:qsTr("All transmissions")},{field:"source",label:qsTr("All sources / mods")},{field:"country",label:qsTr("All countries")},{field:"fuelType",label:qsTr("All fuel types")},{field:"propulsion",label:qsTr("All propulsion")}]
                            GarageFilter { required property var modelData; Layout.fillWidth: true; Layout.minimumWidth: 0; entries: win.garageEntries; field: modelData.field; allLabel: modelData.label; selectedValue: win.activeFacets[field] || ""; onChosen: function(value) { win.setFacet(field,value) } }
                        }
                    }
                    PlainLabel { visible: win.extraFilters; Layout.fillWidth: true; text: win.selectedModel.id ? qsTr("Only metadata supplied by the game is listed. Missing metadata is not guessed.") : qsTr("Model filters match any of its configs. Open a model to filter individual variants."); color: textDim; font.pixelSize: 9; wrapMode: Text.Wrap
                    }
                    GridView {
                        id: garageGrid; objectName: "garageGrid"
                        Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                        model: win.garageItems; focus: win.page === 2
                        readonly property int columns: Math.max(1, Math.min(3, Math.floor((width + win.gap) / 156)))
                        readonly property int tileW: Math.max(96, Math.floor(width / columns) - win.gap)
                        readonly property int tileH: Math.round(tileW * 0.7)
                        cellWidth: Math.max(96 + win.gap, Math.floor(width / columns))
                        cellHeight: tileH + win.gap

                        delegate: VariantCard {
                            width: garageGrid.tileW; height: garageGrid.tileH
                            modelCard: !win.selectedModel.id
                            selected: win.selectedConfig.id === modelData.id
                            favorite: backend.favorites.indexOf(modelData.id) >= 0
                            enabled: !backend.busy || demoMode
                            onChosen: { if(win.selectedModel.id) win.selectedConfig=modelData; else win.chooseModel(modelData) }
                            onFavoriteRequested: backend.toggleFavorite(modelData.id)
                        }
                        PlainLabel {
                            anchors.centerIn: parent; width: parent.width-20; horizontalAlignment: Text.AlignHCenter
                            visible: garageGrid.count === 0 && !backend.busy
                            text: demoMode || win.controls ? qsTr("No matches. Clear the search or change the view.") : qsTr("Connect the updated bridge to browse vehicles.")
                            color: textDim; font.pixelSize: 10
                        }
                        BusyIndicator { anchors.centerIn: parent; running: backend.busy && (backend.pendingAction === "catalog" || backend.pendingAction === "configs") }
                    }
                    ColumnLayout { visible: !!win.selectedModel.id; Layout.fillWidth: true; spacing: gap
                        PlainLabel { text: win.selectedConfig.name || qsTr("Select a configuration"); Layout.fillWidth: true; maximumLineCount: 1; elide: Text.ElideRight; color: textMain; font.pixelSize: 12 }
                        RowLayout {
                            Layout.fillWidth: true; spacing: gap
                            Button { text: qsTr("Spawn"); Layout.fillWidth: true; font.pixelSize: 11; padding: 6; implicitHeight: 30; enabled: !!win.selectedConfig.id && win.canDriveAction; onClicked: win.confirmVehicle("spawn", win.selectedConfig) }
                            Button { text: qsTr("Replace"); Layout.fillWidth: true; font.pixelSize: 11; padding: 6; implicitHeight: 30; enabled: !!win.selectedConfig.id && win.canDriveAction; onClicked: win.confirmVehicle("replace", win.selectedConfig) }
                        }
                        PlainLabel { visible: !win.canDriveAction; text: demoMode ? qsTr("Demo mode: game actions are disabled.") : !win.controls ? qsTr("Connect the bridge first.") : !win.game.map ? qsTr("Load a map before spawning.") : qsTr("Stop before changing vehicles."); font.pixelSize: 10; color: textDim }
                    }
                }
                // SESSION
                ScrollView {
                    clip: true; contentWidth: availableWidth
                    ColumnLayout {
                        width: parent.width; spacing: gap
                        RowLayout { Layout.fillWidth: true; spacing: gap
                            MetricCard { label: qsTr("SESSION"); value: win.time(win.game.sessionSeconds) }
                            MetricCard { label: qsTr("DISTANCE"); value: win.number(win.game.distance === undefined ? undefined : win.game.distance/(win.metric ? 1000:1609.344),2); unit: win.metric ? "km":"mi" }
                        }
                        RowLayout { Layout.fillWidth: true; spacing: gap
                            MetricCard { label: qsTr("TOP SPEED"); value: win.speed(win.game.topSpeed); unit: win.metric ? "km/h":"mph" }
                            MetricCard { label: qsTr("AVERAGE SPEED"); value: win.speed(win.game.averageSpeed); unit: win.metric ? "km/h":"mph" }
                        }
                        PlainLabel { text: qsTr("FRAME RATE · LAST 60 SECONDS"); color: textDim; font.pixelSize: 9 }
                        TrendChart { Layout.fillWidth: true; Layout.preferredHeight: 70; samples: backend.history; field: "fps"; ceiling: 60 }
                        PlainLabel { visible: !!win.game.multiplayer; text: win.game.server || qsTr("Unnamed server"); font.pixelSize: 13; Layout.fillWidth: true; color: textMain }
                        PlainLabel { visible: !!win.game.multiplayer; text: qsTr("Players %1 · Vehicles %2 · Ping %3 ms").arg(win.number(win.game.playerCount)).arg(win.number(win.game.mpVehicleCount)).arg(win.number(win.game.ping)); Layout.fillWidth: true; color: textDim; font.pixelSize: 10 }
                        TextField { id: searchInput; visible: !!win.game.multiplayer; Layout.fillWidth: true; placeholderText: qsTr("Find a player"); font.pixelSize: 11; padding: 6; implicitHeight: 32 }
                        Repeater {
                            model: backend.players.filter(function(p) { return (p.name || "").toLowerCase().indexOf(searchInput.text.toLowerCase()) >= 0 })
                            delegate: RowLayout {
                                required property var modelData
                                Layout.fillWidth: true; spacing: gap
                                PlainLabel { text: modelData.name; Layout.fillWidth: true; color: textMain; font.pixelSize: 11 }
                                PlainLabel { text: win.number(modelData.ping) + " ms"; color: textDim; font.pixelSize: 10 }
                            }
                        }
                        Button { text: qsTr("Your vehicles"); visible: backend.activeVehicles.length > 0; Layout.fillWidth: true; font.pixelSize: 11; padding: 6; implicitHeight: 30; flat: true; onClicked: ownedDialog.open() }
                        Button { text: qsTr("Reset trip"); enabled: !demoMode && !backend.busy; Layout.fillWidth: true; font.pixelSize: 11; padding: 6; implicitHeight: 30; flat: true; onClicked: tripDialog.open() }
                        Button { text: qsTr("Diagnostics"); Layout.fillWidth: true; font.pixelSize: 11; padding: 6; implicitHeight: 30; flat: true; onClicked: diagnostics.open() }
                    }
                }
            }
            PlainLabel { Layout.fillWidth: true; visible: !!win.notice; text: win.notice; maximumLineCount: 3; elide: Text.ElideRight; color: accent; font.pixelSize: 11 }
        }
        Menu { id: settingsMenu; margins: win.pad; font.pixelSize: 11
            MenuItem { text: win.metric ? qsTr("Units: km/h") : qsTr("Units: mph"); onTriggered: win.metric = !win.metric }
            MenuItem { text: qsTr("Paths & bridge setup"); onTriggered: settingsDialog.open() }
            MenuItem { text: backend.monitoring ? qsTr("Pause display") : qsTr("Resume display"); enabled: !demoMode; onTriggered: backend.toggleMonitoring() }
            MenuItem { text: qsTr("Help & diagnostics"); onTriggered: diagnostics.open() }
        }
        Menu { id: garageMenu; margins: win.pad; font.pixelSize: 11
            MenuItem { text: qsTr("Refresh"); enabled: (win.controls || demoMode) && !backend.busy; onTriggered: { if(win.selectedModel.id) backend.requestConfigs(win.selectedModel.id); else backend.requestCatalog() } }
             MenuItem { text: qsTr("Clear search and filters"); onTriggered: win.clearGarageFilters() }
        }
        Dialog { id: settingsDialog; anchors.centerIn: parent; width: Math.min(parent.width-20,460)
            onOpened: { userPath.text = demoMode ? "/home/player/BeamNG" : win.launcher.userFolder; gamePath.text = demoMode ? "/games/BeamNG.drive" : win.launcher.gameFolder; mpPath.text = demoMode ? "/games/BeamMP/BeamMP-Launcher" : win.launcher.launcherPath }
            title: qsTr("Paths & bridge setup"); modal: true; standardButtons: Dialog.Close
            contentItem: ColumnLayout { spacing: gap
                TextField { id:userPath; Layout.fillWidth:true; placeholderText:qsTr("BeamNG user folder (contains beamng.log)"); font.pixelSize: 11; padding: 6; implicitHeight: 32 }
                TextField { id:gamePath; Layout.fillWidth:true; placeholderText:qsTr("BeamNG installation folder"); font.pixelSize: 11; padding: 6; implicitHeight: 32 }
                TextField { id:mpPath; Layout.fillWidth:true; placeholderText:qsTr("BeamMP launcher executable"); font.pixelSize: 11; padding: 6; implicitHeight: 32 }
                PlainLabel { text:qsTr("The bridge loads automatically on the next game start. Updating it never restarts your game."); Layout.fillWidth:true; color:textDim; font.pixelSize: 10 }
                Button { text:qsTr("Save & install bridge"); Layout.fillWidth:true; font.pixelSize: 11; padding: 6; implicitHeight: 32; enabled:!demoMode; onClicked: { win.launcher.userFolder=userPath.text; win.launcher.gameFolder=gamePath.text; win.launcher.launcherPath=mpPath.text; win.launcher.installBridge() } }
                PlainLabel { text:win.launcher.message; Layout.fillWidth:true; color:accent; font.pixelSize: 10 }
            }
        }
        Dialog { id: resetVehicleDialog; anchors.centerIn: parent; width: Math.min(parent.width-20,420); modal: true
            contentHeight: 180
            title: qsTr("Clear ABM vehicle tuning?"); standardButtons: Dialog.Ok | Dialog.Cancel
            contentItem: PlainLabel { text: qsTr("Cancels pending live power, restores ABM vehicle adapters, then requests a physics reset. Existing damage may be repaired. Loaded custom configs and saved preset files are not deleted. World gravity is not changed. Stop first; failed adapters are reported, not hidden."); wrapMode: Text.Wrap; font.pixelSize: 11; color: textMain }
            onAccepted: if (win.stopped && win.sandboxAllowed) backend.fullReset()
        }
        Dialog { id: confirmDialog; anchors.centerIn:parent; width:Math.min(parent.width-20,420); modal:true
            title: win.pendingAction === "replace" ? qsTr("Replace current vehicle?") : win.pendingAction === "switch" ? qsTr("Switch vehicle?") : qsTr("Spawn configuration?")
            standardButtons:Dialog.Ok | Dialog.Cancel
            contentHeight: 120
            contentItem: PlainLabel { text: qsTr("%1\n\n%2").arg(win.actionTarget.name || win.actionTarget.id || qsTr("Selected vehicle")).arg(win.pendingAction === "replace" ? qsTr("The current vehicle will be discarded. Server restrictions still apply.") : qsTr("This changes the game session. Server restrictions still apply.")); font.pixelSize: 11 }
            onAccepted: { if(!win.canDriveAction) { win.tell(qsTr("The game state changed. Select the vehicle again.")); return }; if(win.pendingAction==="switch") backend.switchVehicle(win.actionTarget.vehicleId); else backend.requestVehicle(win.pendingAction,win.actionTarget.id) }
        }
        Dialog { id:tripDialog; anchors.centerIn:parent; width:Math.min(parent.width-20,400); modal:true
            title:qsTr("Reset trip statistics?"); standardButtons:Dialog.Ok | Dialog.Cancel
            contentHeight: 100
            contentItem:PlainLabel { text:qsTr("Only ABM's estimated trip statistics are reset. Your game is not changed."); font.pixelSize: 11 }
            onAccepted:backend.resetTrip()
        }
        Dialog { id:ownedDialog; anchors.centerIn:parent; width:Math.min(parent.width-20,420); modal:true
            title:qsTr("Your vehicles"); standardButtons:Dialog.Close
            contentItem:ColumnLayout {
                PlainLabel { visible: backend.activeVehicles.filter(function(v){return v.owned===true}).length === 0; text: qsTr("No owned vehicles in this session."); Layout.fillWidth: true; color: textDim }
                Repeater {
                    model:backend.activeVehicles.filter(function(v){return v.owned===true})
                    delegate:Button { required property var modelData; text:modelData.name || modelData.model; Layout.fillWidth:true; font.pixelSize: 11; padding: 6; implicitHeight: 32; enabled:win.canDriveAction; onClicked:{ownedDialog.close();win.confirmVehicle("switch",{vehicleId:modelData.id,name:modelData.name})} }
                }
            }
        }
        Dialog { id:diagnostics; anchors.centerIn:parent; width:Math.min(parent.width-20,480); height:Math.min(parent.height-40,410); modal:true
            title:qsTr("Vehicle details & diagnostics"); standardButtons:Dialog.Close
            contentItem:ScrollView { contentWidth:availableWidth; clip:true
                ColumnLayout { width:parent.width; spacing: gap
                    PlainLabel { text:qsTr("App %1 · Bridge %2 · Feed %3 Hz").arg(Qt.application.version).arg(win.game.bridgeVersion || "—").arg(win.number(win.game.sampleHz)); Layout.fillWidth:true; font.pixelSize: 10; color: textDim }
                    PlainLabel { text:qsTr("FPS %1 · CPU %2 ms · GPU %3 ms").arg(win.number(win.game.fps)).arg(win.number(win.game.cpuMs,1)).arg(win.number(win.game.gpuMs,1)); Layout.fillWidth:true; font.pixelSize: 10; color: textDim }
                    PlainLabel { text:qsTr("Coolant %1 °C · Oil %2 °C · Boost %3 bar").arg(win.number(win.telemetry.coolant)).arg(win.number(win.telemetry.oil)).arg(win.number(win.telemetry.boost,1)); Layout.fillWidth:true; font.pixelSize: 10; color: textDim }
                    PlainLabel { text:qsTr("Throttle %1% · Brake %2% · Steering %3%").arg(win.number(win.telemetry.throttle === undefined ? undefined : win.telemetry.throttle*100)).arg(win.number(win.telemetry.brake === undefined ? undefined : win.telemetry.brake*100)).arg(win.number(win.telemetry.steering === undefined ? undefined : win.telemetry.steering*100)); Layout.fillWidth:true; font.pixelSize: 10; color: textDim }
                    PlainLabel { text:qsTr("Best 0–100 km/h: %1 s · Fuel left: %2 L").arg(win.number(win.game.bestZeroHundred,2)).arg(win.number(win.telemetry.fuelVolume,1)); Layout.fillWidth:true; font.pixelSize: 10; color: textDim }
                    PlainLabel { text:qsTr("Average FPS %1 · Minimum FPS %2 · Acceleration %3 m/s²").arg(win.number(win.game.fpsAverage)).arg(win.number(win.game.fpsMinimum)).arg(win.number(win.game.acceleration,1)); Layout.fillWidth:true; font.pixelSize: 10; color: textDim }
                    PlainLabel { text:qsTr("Lateral %1 g · Longitudinal %2 g").arg(win.number(win.telemetry.lateralG,2)).arg(win.number(win.telemetry.longitudinalG,2)); Layout.fillWidth:true; font.pixelSize: 10; color: textDim }
                    PlainLabel { text:qsTr("Lights %1 · ABS %2 · ESC %3").arg(win.telemetry.lights === undefined ? "—" : win.telemetry.lights ? qsTr("On") : qsTr("Off")).arg(win.telemetry.abs === undefined ? "—" : win.telemetry.abs ? qsTr("On") : qsTr("Off")).arg(win.telemetry.esc === undefined ? "—" : win.telemetry.esc ? qsTr("On") : qsTr("Off")); Layout.fillWidth:true; font.pixelSize: 10; color: textDim }
                    PlainLabel { text:qsTr("Trip, acceleration and timing values are estimates. Missing readings are shown as —."); color:textDim; Layout.fillWidth:true; font.pixelSize: 9 }
                    PlainLabel { text:backend.error; visible:!!backend.error; Layout.fillWidth:true; font.pixelSize: 10; color: accent }
                    Button { text:qsTr("Keyboard shortcuts"); font.pixelSize: 11; padding: 6; implicitHeight: 32; onClicked:{diagnostics.close();win.helpVisible=true} }
                }
            }
        }
        Rectangle { anchors.fill:parent; visible:win.helpVisible; color:"#f2101116"; z: 20
            MouseArea { anchors.fill:parent; onClicked:win.helpVisible=false }
            ColumnLayout { anchors.centerIn:parent; width:parent.width-40; spacing: 16
                PlainLabel { text:qsTr("Keyboard shortcuts"); font.pixelSize: 16; color: textMain }
                PlainLabel { text:qsTr("1–4 — Drive, Car, Garage, Session\nSpace — Pause/resume display\nU — Change units\nEscape — Clear search, go back or close dialog\n? — Help\nQ — Close window"); Layout.fillWidth:true; lineHeight:1.6; color: textDim; font.pixelSize: 11 }
                PlainLabel { text:qsTr("On Omarchy the receiver keeps running after this window closes. In standalone mode it runs while the app is open."); Layout.fillWidth:true; color:textDim; font.pixelSize: 10 }
            }
        }
    }
}

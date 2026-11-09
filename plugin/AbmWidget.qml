import QtQuick
import QtQuick.Layouts
import Quickshell
import Quickshell.Io
import qs.Commons as Commons
import qs.Ui
import "AbmModel.js" as Model

Panel {
    id: root
    moduleName: "snobbledev.abm"
    ipcTarget: "snobbledev.abm"
    manageIpc: false
    property var snapshot: ({})
    property double now: Date.now()
    property string raw: ""
    property var processState: ({})
    property string launchMessage: ""
    readonly property bool gameRunning: snapshot.connected === true || processState.gameRunning === true
    property bool interactive: true
    property bool pressable: true
    readonly property bool monitoring: setting("monitoring", true) === true
    readonly property string barTitle: {
        if (root.bar && root.bar.vertical) return "ABM"
        if (!root.monitoring) return "ABM · " + strings.t("paused")
        if (root.snapshot.connected !== true)
            return root.processState.beammpRunning === true ? "BeamMP"
                : root.processState.gameRunning === true ? "BeamNG" : "ABM"
        var game = root.snapshot.multiplayer ? "BeamMP" : "BeamNG"
        return root.snapshot.speed === null ? game
            : game + " · " + root.format(root.snapshot.speed * 3.6) + " km/h"
    }
    implicitWidth: button.implicitWidth
    implicitHeight: button.implicitHeight
    Commons.Translations { id: strings; directory: Qt.resolvedUrl("i18n") }
    function refreshSnapshot() { snapshot = monitoring ? Model.parse(raw, now) : ({}) }
    function triggerPress(mouseButton) { button.triggerPress(mouseButton) }
    function launchDashboard() { root.close(); Quickshell.execDetached(["abm", "open"]) }
    function launchGame() { if (!launchProcess.running) { launchMessage = ""; launchProcess.running = true } }
    function format(value, decimals) { return typeof value !== "number" || !isFinite(value) ? "—" : Commons.I18n.formatNumber(value, decimals || 0) }
    Process {
        id: statusProcess; command: ["abm", "launch-status", "--json"]
        stdout: SplitParser { onRead: function(data) { try { root.processState = JSON.parse(data) } catch(e) { root.processState = ({}) } } }
    }
    Process {
        id: launchProcess; command: ["abm", "launch", "--auto", "--json"]
        stdout: SplitParser { onRead: function(data) { try { root.launchMessage = JSON.parse(data).message || "" } catch(e) {} } }
        onExited: { if (!statusProcess.running) statusProcess.running = true }
    }
    Timer { interval: 4000; running: !root.snapshot.connected || root.opened; repeat: true; triggeredOnStart: true; onTriggered: if (!statusProcess.running) statusProcess.running = true }
    onMonitoringChanged: refreshSnapshot()
    onNowChanged: refreshSnapshot()
    FileView {
        path: (Quickshell.env("XDG_RUNTIME_DIR") || "/run/user/" + Quickshell.env("UID")) + "/abm/state.json"
        watchChanges: true
        printErrors: false
        onFileChanged: reload()
        onLoaded: { root.raw = text(); root.refreshSnapshot() }
        onLoadFailed: { root.raw = ""; root.refreshSnapshot() }
    }
    Timer { interval: 1000; running: true; repeat: true; onTriggered: root.now = Date.now() }
    Commons.ShellIpc {
        target: "snobbledev.abm"
        function open(): void { root.open() }
        function close(): void { root.close() }
        function toggle(): void { root.toggle() }
        function launch(): void { root.launchDashboard() }
        function state(): string { return JSON.stringify({opened:root.opened, connected:root.snapshot.connected === true}) }
    }
    WidgetButton {
        id: button
        anchors.fill: parent
        bar: root.bar
        dimmed: !root.snapshot.connected
        text: root.barTitle
        tooltipText: strings.t("open")
        onPressed: function(mouseButton) {
            var action = Model.clickAction(mouseButton, root.gameRunning)
            if (action === "toggle") root.toggle()
            else if (action === "launch") root.launchDashboard()
            else if (action === "start") { root.open(); root.launchGame() }
        }
    }
    KeyboardPanel {
        id: popup
        anchorItem: button
        bar: root.bar
        owner: root
        open: root.opened
        focusTarget: summary
        contentWidth: Commons.Style.space(260)
        contentHeight: summary.implicitHeight + padding * 2
        ColumnLayout {
            id: summary
            width: parent.width
            spacing: Commons.Style.space(8)
            Keys.onEscapePressed: root.close()
            Keys.onReturnPressed: root.launchDashboard()
            Keys.onEnterPressed: root.launchDashboard()
            Text { text: "ABM"; textFormat: Text.PlainText; color: Commons.Color.accent; font.pixelSize: Commons.Style.font.body; font.bold: true }
            Text { text: root.snapshot.connected ? root.snapshot.multiplayer ? root.snapshot.server : strings.t("single") : strings.t("waiting"); textFormat: Text.PlainText; color: Commons.Color.foreground; wrapMode: Text.Wrap; Layout.fillWidth: true; font.pixelSize: Commons.Style.font.bodySmall }
            Text { visible: root.snapshot.connected === true; text: root.snapshot.vehicle || strings.t("noVehicle"); textFormat: Text.PlainText; color: Commons.Color.accent; wrapMode: Text.Wrap; Layout.fillWidth: true; font.pixelSize: Commons.Style.font.bodySmall }
            GridLayout {
                visible: root.snapshot.connected === true
                columns: 2; Layout.fillWidth: true; columnSpacing: Commons.Style.space(12); rowSpacing: Commons.Style.space(6)
                Repeater {
                    model: [
                        {label:strings.t("speed"), value:root.format(root.snapshot.speed === null ? null : root.snapshot.speed * 3.6) + " km/h"},
                        {label:"RPM", value:root.format(root.snapshot.rpm)},
                        {label:"FPS", value:root.format(root.snapshot.fps)},
                        {label:strings.t("fuel"), value:root.format(root.snapshot.fuel === null ? null : root.snapshot.fuel * 100) + "%"},
                        {label:strings.t("playerLabel"), value:root.format(root.snapshot.playerCount)},
                        {label:strings.t("ping"), value:root.format(root.snapshot.ping) + " ms"}
                    ].filter(function(item, index) { return (index < 4 || root.snapshot.multiplayer) && (root.snapshot.vehicle || index === 2) })
                    delegate: ColumnLayout {
                        required property var modelData
                        Layout.fillWidth: true; spacing: Commons.Style.space(1)
                        Text { text: modelData.label; textFormat: Text.PlainText; color: Commons.Color.foreground; font.pixelSize: Commons.Style.font.caption }
                        Text { text: modelData.value; textFormat: Text.PlainText; color: Commons.Color.foreground; font.pixelSize: Commons.Style.font.bodySmall }
                    }
                }
            }
            Button { text: launchProcess.running ? strings.t("starting") : root.gameRunning ? (root.processState.beammpRunning === true ? strings.t("beammpRunning") : strings.t("startBeamMP")) : strings.t("startBeamNG"); Layout.fillWidth: true; enabled: !launchProcess.running && (!root.gameRunning || root.processState.beammpRunning !== true); onClicked: root.launchGame() }
            Text { visible: root.launchMessage !== ""; text: root.launchMessage; textFormat: Text.PlainText; color: Commons.Color.accent; wrapMode: Text.Wrap; Layout.fillWidth: true; font.pixelSize: Commons.Style.font.caption }
            Button { text: strings.t("dashboard"); Layout.fillWidth: true; onClicked: root.launchDashboard() }
            Text { text: strings.t("hint"); textFormat: Text.PlainText; color: Commons.Color.foreground; wrapMode: Text.Wrap; Layout.fillWidth: true; font.pixelSize: Commons.Style.font.caption }
        }
    }
}

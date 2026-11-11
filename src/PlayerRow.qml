import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// One stable multiplayer row. Identity is the session row key, never the
// display name. Ping provenance and freshness stay visible.
ColumnLayout {
    id: row
    required property var modelData
    objectName: "playerRow_" + (player.rowKey || "missing")
    readonly property var player: modelData || ({})
    property var profiles
    property color foreground: "#e8e8ed"
    property color muted: "#9b9ca9"
    property color accentColor: "#7aa2f7"
    property int fontSize: 11
    readonly property bool actionPressed: nameButton.pressed || detailsButton.pressed || profileButton.pressed || forumButton.pressed || sessionButton.pressed
    signal profileRequested()

    function millis(value) {
        return typeof value === "number" && isFinite(value) ? Math.round(value) + " ms" : "—"
    }
    function pingLabel() {
        var p = row.player
        if (p.ping === undefined) return qsTr("ping unavailable")
        var text = millis(p.ping)
        if (p.pingSource === "local") text += qsTr(" · local")
        else if (p.pingSource === "remote") text += qsTr(" · remote")
        else if (p.pingSource === "server") text += qsTr(" · server")
        if (p.pingAgeMs !== undefined && isFinite(p.pingAgeMs)) text += qsTr(" · %1s old").arg(Math.round(p.pingAgeMs / 1000))
        return text
    }

    Layout.fillWidth: true
    spacing: 4

    RowLayout {
    Layout.fillWidth: true
    spacing: 6
    Rectangle {
        width: 6; height: 6; radius: 3
        color: row.player.local ? row.accentColor : "#555966"
    }
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 0
        RowLayout {
            Layout.fillWidth: true
            spacing: 4
            PlainLabel {
                text: row.player.name || row.player.rawName || qsTr("Unknown player")
                color: row.foreground
                font.pixelSize: row.fontSize
                maximumLineCount: 1
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            PlainLabel {
                visible: row.player.local === true
                text: qsTr("You")
                color: row.accentColor
                font.pixelSize: 9
            }
            PlainLabel {
                visible: row.player.guest === true
                text: qsTr("guest")
                color: "#c59b55"
                font.pixelSize: 9
            }
        }
        PlainLabel {
            Layout.fillWidth: true
            maximumLineCount: 1
            elide: Text.ElideRight
            color: row.muted
            font.pixelSize: 9
            text: {
                var parts = []
                if (row.player.sessionPlayerId !== undefined) parts.push(qsTr("Session #%1").arg(row.player.sessionPlayerId))
                if (row.player.role) parts.push(row.player.role)
                if (row.player.vehicles !== undefined) parts.push(qsTr("%1 vehicles").arg(row.player.vehicles))
                parts.push(row.pingLabel())
                return parts.join(" · ")
            }
        }
    }
    }
    Flow {
    Layout.fillWidth: true
    spacing: 4
    Button {
        id: nameButton
        objectName: "copyPlayerName"
        text: qsTr("Copy name")
        font.pixelSize: 9; padding: 4; implicitHeight: 24
        enabled: !!row.player.rawName && !!row.profiles
        onClicked: row.profiles.copyName(row.player.rowKey)
    }
    Button {
        id: detailsButton
        objectName: "copyPlayerDetails"
        text: qsTr("Copy details")
        font.pixelSize: 9; padding: 4; implicitHeight: 24
        enabled: !!row.player.rowKey && !!row.profiles
        onClicked: row.profiles.copyDetails(row.player.rowKey)
    }
    Button {
        id: profileButton
        text: qsTr("Profile")
        font.pixelSize: 9; padding: 4; implicitHeight: 24; flat: true
        enabled: !!row.player.rawName && !!row.profiles
        onClicked: if (row.profiles) { row.profiles.openProfile(row.player.rawName); row.profileRequested() }
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Open the public forum username page. This does not verify account ownership.")
    }
    Button {
        id: forumButton
        text: qsTr("Forum ID")
        font.pixelSize: 9; padding: 4; implicitHeight: 24; flat: true
        enabled: !!row.player.rowKey && row.player.guest !== true && !!row.profiles
        onClicked: if (row.profiles) row.profiles.copyAccount(row.player.rowKey)
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Copy the public forum ID by username lookup, not a verified BeamMP account ID. Clipboard changes cancel late copies.")
    }
    Button {
        id: sessionButton
        text: qsTr("Session ID")
        font.pixelSize: 9; padding: 4; implicitHeight: 24; flat: true
        objectName: "copyPlayerSessionId"
        enabled: row.player.sessionPlayerId !== undefined && !!row.profiles
        onClicked: if (row.profiles) row.profiles.copySessionId(row.player.rowKey)
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Copy this session only. It is not the permanent account ID.")
    }
    }
    Rectangle { Layout.fillWidth: true; height: 1; color: "#22ffffff" }
}

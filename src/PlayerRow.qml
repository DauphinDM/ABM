import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// One stable multiplayer row. Identity is the session row key, never the
// display name. Ping provenance and freshness stay visible.
SectionPanel {
    id: row
    required property var modelData
    objectName: "playerRow_" + (player.rowKey || "missing")
    readonly property var player: modelData || ({})
    property var profiles
    property color foreground: "#e8e8ed"
    property color muted: "#9b9ca9"
    property color accentColor: "#7aa2f7"
    property int fontSize: 13
    property bool publicProfilesEnabled: true
    property string actionMessage: ""
    onModelDataChanged: if (player.rowKey !== lastRowKey) { actionMessage = ""; lastRowKey = player.rowKey || "" }
    property string lastRowKey: ""
    readonly property bool actionPressed: nameButton.pressed || detailsButton.pressed || profileButton.pressed || forumButton.pressed || sessionButton.pressed
    signal profileRequested()
    Connections {
        target: row.profiles || null
        function onChanged() {
            if (row.profiles.messageRowKey === row.player.rowKey) row.actionMessage = row.profiles.message
        }
    }

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
    spacing: 12

    RowLayout {
    Layout.fillWidth: true
    spacing: 6
    Rectangle {
        width: 6; height: 6; radius: 3
        color: row.player.local ? row.accentColor : "#555966"
    }
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 5
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
                font.pixelSize: 10
            }
            PlainLabel {
                visible: row.player.guest === true
                text: qsTr("guest")
                color: "#c59b55"
                font.pixelSize: 10
            }
        }
        PlainLabel {
            Layout.fillWidth: true
            maximumLineCount: 1
            elide: Text.ElideRight
            color: row.muted
            font.pixelSize: 11
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
        font.pixelSize: 11; padding: 6; implicitHeight: 30
        enabled: !!row.player.rawName && !!row.profiles
        onClicked: row.profiles.copyName(row.player.rowKey)
    }
    Button {
        id: detailsButton
        objectName: "copyPlayerDetails"
        text: qsTr("Copy details")
        font.pixelSize: 11; padding: 6; implicitHeight: 30
        enabled: !!row.player.rowKey && !!row.profiles
        onClicked: row.profiles.copyDetails(row.player.rowKey)
    }
    Button {
        id: profileButton
        objectName: "openPlayerProfile"
        text: qsTr("Profile")
        font.pixelSize: 11; padding: 6; implicitHeight: 30; flat: true
        enabled: row.publicProfilesEnabled && !!row.player.rawName && row.player.guest !== true && !!row.profiles
        onClicked: if (row.profiles) { row.profiles.openProfile(row.player.rawName); row.profileRequested() }
        ToolTip.visible: hovered
        ToolTip.text: !row.publicProfilesEnabled ? qsTr("Sample players have no real public profile.") : qsTr("Open the public forum username page. This does not verify account ownership.")
    }
    Button {
        id: forumButton
        objectName: "copyPlayerForumId"
        text: qsTr("Forum ID")
        font.pixelSize: 11; padding: 6; implicitHeight: 30; flat: true
        enabled: row.publicProfilesEnabled && !!row.player.rawName && !!row.player.rowKey && row.player.guest !== true && !!row.profiles && !row.profiles.resolving
        onClicked: if (row.profiles) row.profiles.copyAccount(row.player.rowKey)
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Copy the public forum ID by username lookup, not a verified BeamMP account ID. Clipboard changes cancel late copies.")
    }
    Button {
        id: sessionButton
        text: qsTr("Session ID")
        font.pixelSize: 11; padding: 6; implicitHeight: 30; flat: true
        objectName: "copyPlayerSessionId"
        enabled: row.player.sessionPlayerId !== undefined && !!row.profiles
        onClicked: if (row.profiles) row.profiles.copySessionId(row.player.rowKey)
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Copy this session only. It is not the permanent account ID.")
    }
    }
    PlainLabel {
        Layout.fillWidth: true; visible: !!row.actionMessage
        text: row.actionMessage; wrapMode: Text.Wrap
        color: row.accentColor; font.pixelSize: 11
        Accessible.role: Accessible.StaticText
    }
}

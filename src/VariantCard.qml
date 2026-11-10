import QtQuick
import QtQuick.Layouts

Item {
    id: card
    required property var modelData
    property bool selected: false
    property bool favorite: false
    property bool modelCard: false
    property bool fallbackPreview: !modelData.thumbnail
    property bool hovered: area.containsMouse
    readonly property string title: card.modelCard
        ? ((card.modelData.brand || "") + " " + (card.modelData.name || card.modelData.id)).trim()
        : card.modelData.name || card.modelData.id
    signal chosen()
    signal favoriteRequested()

    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        onClicked: card.chosen()
    }

    Image {
        id: photo
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: parent.height - labelStrip.height
        source: card.modelData.thumbnail || card.modelData.modelThumbnail || ""
        sourceSize: Qt.size(280, 176)
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        cache: false
        onStatusChanged: {
            if (status === Image.Error && card.modelData.modelThumbnail && source.toString() !== card.modelData.modelThumbnail) {
                card.fallbackPreview = true; source = card.modelData.modelThumbnail
            }
        }
    }

    Rectangle {  // hover veil over the photo
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: photo.height
        color: card.hovered ? "#2e272a33" : "transparent"
    }

    Rectangle {  // caption strip
        id: labelStrip
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 40
        color: card.hovered || card.selected ? "#b81c1f26" : "#8c121319"
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 5
            spacing: 0
            PlainLabel {
                Layout.fillWidth: true
                text: card.title || (card.modelData.model || card.modelData.id)
                font.weight: Font.DemiBold
                font.pixelSize: 11
                elide: Text.ElideRight
                maximumLineCount: 1
            }
            PlainLabel {
                Layout.fillWidth: true
                text: (card.modelData.custom ? qsTr("Custom") : card.modelData.type || card.modelData.drivetrain || qsTr("Configuration"))
                    + (card.modelData.source && card.modelData.source !== "BeamNG - Official" && card.modelData.source !== "Custom" ? " · " + card.modelData.source : "")
                    + (!card.modelCard && card.fallbackPreview ? " · " + qsTr("Model preview") : "")
                font.pixelSize: 9
                color: "#9b9ca9"
                elide: Text.ElideRight
                maximumLineCount: 1
            }
        }
    }

    MouseArea {  // favorite star, over the photo
        id: star
        anchors.top: parent.top
        anchors.right: parent.right
        width: 26
        height: 20
        hoverEnabled: true
        onClicked: card.favoriteRequested()
        Rectangle { anchors.fill: parent; color: "#8c0b0c10" }
        PlainLabel {
            anchors.centerIn: parent
            text: card.favorite ? "★" : "☆"
            color: card.favorite ? theme.accent : "#d5d6dd"
            font.pixelSize: 13
            font.weight: Font.Bold
        }
    }

    // faint image edge so tiles without photos still read as tiles
    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: photo.height
        visible: photo.status !== Image.Ready
        color: "#40121319"
        PlainLabel { anchors.centerIn: parent; text: photo.status === Image.Loading ? qsTr("Loading…") : qsTr("No preview"); color: "#9b9ca9"; font.pixelSize: 10 }
    }

    Rectangle {  // selection frame drawn on top
        anchors.fill: parent
        color: "transparent"
        border.width: card.selected ? 1 : 0
        border.color: theme.accent
    }
}

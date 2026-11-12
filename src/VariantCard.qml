import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Control {
    id: card
    required property var modelData
    property bool selected: false
    property bool favorite: false
    property bool modelCard: false
    readonly property string exactPreview: modelData.thumbnail || ""
    readonly property string modelPreview: modelData.modelThumbnail || ""
    property bool exactPreviewFailed: false
    readonly property bool fallbackPreview: (!exactPreview || exactPreviewFailed) && !!modelPreview
    readonly property string previewKind: photo.status === Image.Ready
        ? (!modelCard && fallbackPreview ? "model" : "exact") : "unavailable"
    onModelDataChanged: exactPreviewFailed = false
    onExactPreviewChanged: exactPreviewFailed = false
    onModelPreviewChanged: exactPreviewFailed = false
    hoverEnabled: true
    readonly property string title: card.modelCard
        ? ((card.modelData.brand || "") + " " + (card.modelData.name || card.modelData.id)).trim()
        : card.modelData.name || card.modelData.id
    signal chosen()
    signal favoriteRequested()
    focusPolicy: Qt.StrongFocus
    Accessible.role: Accessible.Button
    Accessible.name: title
    Keys.onReturnPressed: chosen()
    Keys.onEnterPressed: chosen()
    Keys.onSpacePressed: chosen()

    Rectangle {
        anchors.fill: parent
        color: card.hovered ? "#222a35" : "#191f28"
        radius: Math.min(6, theme.radius)
        border.width: 1
        border.color: card.selected || card.visualFocus ? theme.accent : card.hovered ? "#596475" : "#303846"
    }

    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: { card.forceActiveFocus(Qt.MouseFocusReason); card.chosen() }
    }

    Image {
        id: photo
        objectName: "variantPhoto"
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 1
        height: parent.height - labelStrip.height - 1
        source: card.exactPreview && !card.exactPreviewFailed ? card.exactPreview : card.modelPreview
        sourceSize: Qt.size(480, 300)
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        cache: false
        onStatusChanged: {
            if (status === Image.Error && card.exactPreview && !card.exactPreviewFailed)
                card.exactPreviewFailed = true
        }
    }

    Rectangle {  // hover veil over the photo
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: photo.height
        color: card.hovered ? "#10ffffff" : "transparent"
    }

    Rectangle {  // caption strip
        id: labelStrip
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 64
        color: "transparent"
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 5
            PlainLabel {
                Layout.fillWidth: true
                text: card.title || (card.modelData.model || card.modelData.id)
                font.weight: Font.DemiBold
                font.pixelSize: 13
                elide: Text.ElideRight
                maximumLineCount: 1
            }
            PlainLabel {
                Layout.fillWidth: true
                text: (card.modelData.custom ? qsTr("Custom") : card.modelData.type || card.modelData.drivetrain || qsTr("Configuration"))
                    + (card.modelData.source && card.modelData.source !== "BeamNG - Official" && card.modelData.source !== "Custom" ? " · " + card.modelData.source : "")
                    + (card.previewKind === "model" ? " · " + qsTr("Model preview") : "")
                font.pixelSize: 10
                color: "#a0aabb"
                elide: Text.ElideRight
                maximumLineCount: 1
            }
        }
    }

    Button { // Separate focusable action; never selects or opens the card.
        id: star
        z: 2
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 8
        width: 28
        height: 28
        padding: 0
        text: card.favorite ? "★" : "☆"
        Accessible.name: card.favorite ? qsTr("Remove %1 from favorites").arg(card.title) : qsTr("Favorite %1").arg(card.title)
        onClicked: card.favoriteRequested()
        background: Rectangle { color: "#e6191f28"; radius: Math.min(4, theme.radius); border.width: star.visualFocus ? 2 : 1; border.color: star.visualFocus ? theme.accent : "#465162" }
        contentItem: PlainLabel {
            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
            text: star.text
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
        color: "#1c232d"
        Column {
            anchors.centerIn: parent; spacing: 6
            PlainLabel { anchors.horizontalCenter: parent.horizontalCenter; text: "—"; color: "#556174"; font.pixelSize: 28 }
            PlainLabel { text: photo.status === Image.Loading ? qsTr("Loading preview…") : qsTr("Preview unavailable"); color: "#a0aabb"; font.pixelSize: 11 }
        }
    }

    Rectangle {  // selection frame drawn on top
        anchors.fill: parent
        color: "transparent"
        border.width: card.selected ? 1 : 0
        border.color: theme.accent
        radius: Math.min(6, theme.radius)
    }
}

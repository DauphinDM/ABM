import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// ABM-owned presets store accepted ABM settings, not native .pc files or Lua.
ColumnLayout {
    id: panel
    property var backend
    readonly property var store: backend && backend.presets ? backend.presets : null
    readonly property var entries: store ? (store.entries || []) : []
    readonly property string storeMessage: store ? (store.message || "") : ""
    readonly property bool running: backend ? !!backend.presetRunning : false
    readonly property bool engineBusy: backend ? !!backend.busy : false
    readonly property string progress: backend ? (backend.presetProgress || "") : ""
    readonly property string applyMessage: backend ? (backend.presetMessage || "") : ""
    property color foreground: "#e8e8ed"
    property color muted: "#9b9ca9"
    property color accentColor: "#7aa2f7"
    property color surfaceColor: "#d016171c"
    property string selectedId: ""
    property string selectedName: ""
    property string selectedModel: ""
    property bool exportVisible: false
    property bool importVisible: false
    spacing: 6

    function selectedEntry() {
        var list = panel.entries
        for (var i = 0; i < list.length; ++i)
            if (list[i].id === panel.selectedId) return list[i]
        return ({})
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 6
        TextField {
            id: presetName
            Layout.fillWidth: true
            placeholderText: qsTr("New preset name")
            font.pixelSize: 11; padding: 6; implicitHeight: 30
            maximumLength: 80
        }
        Button {
            text: qsTr("Save current")
            font.pixelSize: 10; implicitHeight: 30
            enabled: presetName.text.trim().length > 0 && !panel.running && !panel.engineBusy && panel.backend
            onClicked: {
                if (panel.backend && panel.backend.saveCurrentPreset(presetName.text)) presetName.text = ""
            }
        }
    }
    PlainLabel {
        Layout.fillWidth: true
        text: qsTr("Saves accepted ABM overrides only: power, rev limit, clutch protection or ratios that are readable now. It is not a native vehicle clone and does not save mods, Lua or server data.")
        color: panel.muted; font.pixelSize: 9; wrapMode: Text.Wrap
    }
    PlainLabel {
        Layout.fillWidth: true
        visible: !!panel.storeMessage
        text: panel.storeMessage
        color: panel.accentColor; font.pixelSize: 10; wrapMode: Text.Wrap
    }
    PlainLabel {
        visible: panel.entries.length === 0
        Layout.fillWidth: true
        text: qsTr("No saved presets.")
        color: panel.muted; font.pixelSize: 10
    }
    Repeater {
        model: panel.entries
        delegate: Rectangle {
            required property var modelData
            Layout.fillWidth: true
            implicitHeight: body.implicitHeight + 10
            color: panel.selectedId === modelData.id ? "#33343c" : panel.surfaceColor
            border.width: panel.selectedId === modelData.id ? 1 : 0
            border.color: panel.accentColor
            ColumnLayout {
                id: body
                anchors.fill: parent; anchors.margins: 5; spacing: 2
                PlainLabel {
                    Layout.fillWidth: true
                    text: modelData.name || modelData.id || qsTr("Invalid preset")
                    color: modelData.error ? "#df6b61" : panel.foreground
                    font.pixelSize: 11
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }
                PlainLabel {
                    Layout.fillWidth: true
                    visible: !!modelData.error
                    text: modelData.error || ""
                    color: "#df6b61"; font.pixelSize: 9; wrapMode: Text.Wrap
                }
                PlainLabel {
                    Layout.fillWidth: true
                    visible: !modelData.error
                    text: {
                        var c = modelData.configuration || ({})
                        return (c.label || c.model || qsTr("unknown vehicle")) + qsTr(" · ABM settings: ") + Object.keys(modelData.settings || ({})).join(", ")
                    }
                    color: panel.muted; font.pixelSize: 9; elide: Text.ElideRight; maximumLineCount: 1
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    Button {
                        text: qsTr("Select")
                        font.pixelSize: 9; implicitHeight: 24; padding: 4
                        enabled: !modelData.error
                        onClicked: {
                            panel.selectedId = modelData.id
                            panel.selectedName = modelData.name || ""
                            panel.selectedModel = (modelData.configuration || ({})).label || ""
                        }
                    }
                    Button {
                        text: qsTr("Apply")
                        font.pixelSize: 9; implicitHeight: 24; padding: 4
                        enabled: !modelData.error && panel.selectedId === modelData.id && !panel.running
                        onClicked: applyDialog.open()
                    }
                    Button {
                        text: qsTr("Rename")
                        font.pixelSize: 9; implicitHeight: 24; padding: 4
                        enabled: !modelData.error && panel.selectedId === modelData.id
                        onClicked: renameField.text = modelData.name || ""
                    }
                    Button {
                        text: qsTr("Delete")
                        font.pixelSize: 9; implicitHeight: 24; padding: 4
                        enabled: !modelData.error && panel.selectedId === modelData.id
                        onClicked: deleteDialog.open()
                    }
                }
            }
        }
    }
    RowLayout {
        Layout.fillWidth: true
        spacing: 4
        TextField {
            id: renameField
            Layout.fillWidth: true
            visible: !!panel.selectedId
            placeholderText: qsTr("Rename selected preset")
            font.pixelSize: 10; padding: 5; implicitHeight: 28; maximumLength: 80
        }
        Button {
            visible: !!panel.selectedId
            text: qsTr("Rename")
            font.pixelSize: 9; implicitHeight: 28
            enabled: renameField.text.trim().length > 0
            onClicked: if (panel.store && panel.store.renamePreset(panel.selectedId, renameField.text)) renameField.text = ""
        }
    }
    RowLayout {
        Layout.fillWidth: true
        spacing: 4
        TextField {
            id: tagsField
            Layout.fillWidth: true
            visible: !!panel.selectedId
            placeholderText: qsTr("Tags")
            font.pixelSize: 10; padding: 5; implicitHeight: 28; maximumLength: 256
            text: panel.selectedEntry().tags || ""
        }
        TextField {
            id: notesField
            Layout.fillWidth: true
            visible: !!panel.selectedId
            placeholderText: qsTr("Notes")
            font.pixelSize: 10; padding: 5; implicitHeight: 28; maximumLength: 4096
            text: panel.selectedEntry().notes || ""
        }
        Button {
            visible: !!panel.selectedId
            text: qsTr("Save notes")
            font.pixelSize: 9; implicitHeight: 28
            onClicked: if (panel.store) panel.store.annotate(panel.selectedId, tagsField.text, notesField.text)
        }
    }
    RowLayout {
        Layout.fillWidth: true
        spacing: 4
        Button {
            text: qsTr("Export selected"); font.pixelSize: 9; implicitHeight: 26; enabled: !!panel.selectedId
            onClicked: {
                panel.importVisible = false; panel.exportVisible = true
                documentArea.text = panel.store ? panel.store.exportJson(panel.selectedId) : ""
            }
        }
        Button {
            text: qsTr("Import JSON"); font.pixelSize: 9; implicitHeight: 26
            onClicked: { panel.exportVisible = false; panel.importVisible = !panel.importVisible; documentArea.text = "" }
        }
    }
    TextArea {
        id: documentArea
        Layout.fillWidth: true
        visible: panel.exportVisible || panel.importVisible
        Layout.preferredHeight: 120
        font.pixelSize: 9
        wrapMode: TextArea.Wrap
        placeholderText: panel.importVisible ? qsTr("Paste one preset JSON document (128 KiB max)") : ""
        readOnly: panel.exportVisible
    }
    Button {
        visible: panel.importVisible
        Layout.fillWidth: true
        text: qsTr("Import pasted JSON")
        font.pixelSize: 9; implicitHeight: 26
        onClicked: { if (panel.store && panel.store.importJson(documentArea.text)) { panel.importVisible = false; documentArea.text = "" } }
    }
    PlainLabel {
        Layout.fillWidth: true
        visible: panel.running || !!panel.applyMessage
        text: panel.running ? (panel.progress || qsTr("Applying preset…")) : panel.applyMessage
        color: panel.accentColor; font.pixelSize: 9; wrapMode: Text.Wrap
    }
    Button {
        visible: panel.running
        Layout.fillWidth: true
        text: qsTr("Cancel remaining preset steps")
        font.pixelSize: 9; implicitHeight: 26
        onClicked: if (panel.backend) panel.backend.cancelPresetApply()
    }

    Dialog {
        id: applyDialog
        anchors.centerIn: parent
        modal: true
        width: Math.min(400, panel.width - 20)
        title: qsTr("Apply preset?")
        standardButtons: Dialog.Ok | Dialog.Cancel
        contentItem: Item {
            implicitHeight: 150
            PlainLabel {
                width: parent.width
                text: qsTr("Apply the supported steps of %1 to your own stopped vehicle?\n\nSteps run in a documented order with a fresh target check. There is no atomic rollback: a later failure leaves earlier accepted steps in place. Unsupported settings are skipped and listed.").arg(panel.selectedName || qsTr("this preset"))
                wrapMode: Text.Wrap; font.pixelSize: 10
            }
        }
        onAccepted: if (panel.backend) panel.backend.applyPreset(panel.selectedId)
    }
    Dialog {
        id: deleteDialog
        anchors.centerIn: parent
        modal: true
        width: Math.min(360, panel.width - 20)
        title: qsTr("Delete preset?")
        standardButtons: Dialog.Ok | Dialog.Cancel
        contentItem: Item {
            implicitHeight: 60
            PlainLabel { width: parent.width; text: qsTr("Delete %1? This removes only the ABM preset file.").arg(panel.selectedName || qsTr("the selected preset")); wrapMode: Text.Wrap; font.pixelSize: 10 }
        }
        onAccepted: if (panel.store && panel.store.remove(panel.selectedId, true)) panel.selectedId = ""
    }
}

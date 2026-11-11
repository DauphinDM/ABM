import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Keyboard-first command surface. It only emits an id; the window performs the
// action through the same guarded handlers as the visible controls.
Popup {
    id: palette
    property var commands: []
    property color accentColor: theme.accent
    property int selectedIndex: 0
    signal chosen(string commandId)

    readonly property var filtered: {
        var q = query.text.trim().toLowerCase()
        if (!q) return commands
        return commands.filter(function(c) {
            return (c.label || "").toLowerCase().indexOf(q) >= 0
            || (c.hint || "").toLowerCase().indexOf(q) >= 0
        })
    }

    width: Math.min(420, parent ? parent.width - 24 : 420)
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round(parent.height * .16) : 40
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    padding: 0

    background: GlassPanel {
        corner: 14
        fillColor: "#f20c0d12"
        strokeColor: Qt.alpha(palette.accentColor, .32)
    }

    contentItem: ColumnLayout {
        spacing: 0
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 10
            spacing: 8
            PlainLabel {
                text: "⌘"
                color: palette.accentColor
                font.pixelSize: 15
            }
            TextField {
                id: query
                Layout.fillWidth: true
                placeholderText: qsTr("Jump to a page, car, preset or action")
                font.pixelSize: 12
                padding: 6
                onTextChanged: palette.selectedIndex = 0
                Keys.onDownPressed: palette.move(1)
                Keys.onUpPressed: palette.move(-1)
                Keys.onReturnPressed: palette.choose()
                Keys.onEnterPressed: palette.choose()
            }
            PlainLabel { text: "ESC"; color: "#777b88"; font.pixelSize: 8 }
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: "#22ffffff" }
        ListView {
            id: list
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(300, Math.max(42, palette.filtered.length * 38))
            clip: true
            model: palette.filtered
            currentIndex: palette.selectedIndex
            delegate: Item {
                id: row
                required property int index
                required property var modelData
                width: list.width
                height: 38
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 3
                    radius: 7
                    color: row.ListView.isCurrentItem ? Qt.alpha(palette.accentColor, .18) : (mouse.containsMouse ? "#12ffffff" : "transparent")
                }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 10
                    PlainLabel {
                        Layout.fillWidth: true
                        text: row.modelData.label || ""
                        color: "#e8e8ed"
                        font.pixelSize: 11
                        elide: Text.ElideRight
                        maximumLineCount: 1
                    }
                    PlainLabel {
                        visible: !!row.modelData.hint
                        text: row.modelData.hint || ""
                        color: "#858997"
                        font.pixelSize: 8
                    }
                }
                MouseArea {
                    id: mouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: { palette.selectedIndex = row.index; palette.choose() }
                }
            }
        }
        PlainLabel {
            visible: palette.filtered.length === 0
            Layout.fillWidth: true
            Layout.margins: 14
            text: qsTr("No matching command.")
            color: "#858997"
            font.pixelSize: 10
        }
    }

    function openPalette() {
        query.text = ""
        selectedIndex = 0
        open()
        query.forceActiveFocus()
    }
    function move(delta) {
        if (!filtered.length) return
        selectedIndex = (selectedIndex + delta + filtered.length) % filtered.length
    }
    function choose() {
        if (!filtered.length) return
        var commandId = filtered[Math.max(0, Math.min(selectedIndex, filtered.length - 1))].id
        close()
        if (commandId) chosen(commandId)
    }
}

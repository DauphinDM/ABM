import QtQuick
import QtQuick.Controls

// ContextMenu/TextEditingContextMenu require newer Qt than the portable 6.8 runtime.
Item {
    id: root
    required property var editor
    parent: editor
    anchors.fill: parent
    Menu {
        id: menu
        objectName: "fieldEditMenu"
        MenuItem { text: qsTr("Undo"); enabled: !root.editor.readOnly && root.editor.canUndo; onTriggered: root.editor.undo() }
        MenuItem { text: qsTr("Redo"); enabled: !root.editor.readOnly && root.editor.canRedo; onTriggered: root.editor.redo() }
        MenuSeparator { }
        MenuItem { text: qsTr("Cut"); enabled: !root.editor.readOnly && !!root.editor.selectedText; onTriggered: root.editor.cut() }
        MenuItem { text: qsTr("Copy"); objectName: "fieldCopy"; enabled: !!root.editor.selectedText; onTriggered: root.editor.copy() }
        MenuItem { text: qsTr("Paste"); enabled: !root.editor.readOnly && root.editor.canPaste; onTriggered: root.editor.paste() }
        MenuItem { text: qsTr("Select all"); enabled: root.editor.length > 0; onTriggered: root.editor.selectAll() }
    }
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: function(mouse) { menu.popup(root.editor, mouse.x, mouse.y) }
    }
    Shortcut {
        sequence: "Shift+F10"
        enabled: root.editor.activeFocus
        onActivated: menu.popup(root.editor, 0, root.editor.height)
    }
}

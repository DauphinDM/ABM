import QtQuick

Rectangle {
    required property var field
    property bool invalid: false
    objectName: "glassFieldSurface"
    implicitWidth: 120
    implicitHeight: 32
    radius: Math.min(6, theme.radius)
    color: field.activeFocus ? "#0c111a" : "#0c0f15"
    border.width: field.activeFocus ? 2 : 1
    border.color: !field.enabled ? "#29303b" : invalid ? "#ef8179" : field.activeFocus ? theme.accent : field.hovered ? "#657086" : "#3a4353"
    Rectangle {
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        anchors.margins: 3
        height: 1
        color: invalid ? "#ef8179" : theme.accent
        opacity: !field.enabled ? 0 : field.activeFocus ? .55 : .12
    }
}

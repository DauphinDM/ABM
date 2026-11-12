import QtQuick

Rectangle {
    required property var field
    property bool invalid: false
    objectName: "glassFieldSurface"
    implicitWidth: 120
    implicitHeight: 36
    radius: Math.min(6, theme.radius)
    color: field.activeFocus ? "#1b222c" : "#151a22"
    border.width: field.activeFocus ? 2 : 1
    border.color: !field.enabled ? "#29303b" : invalid ? "#ef8179" : field.activeFocus ? theme.accent : field.hovered ? "#657086" : "#353e4b"
}

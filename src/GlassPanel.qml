import QtQuick

// Shared quiet surface. Accent is reserved for actions and live readings.
Rectangle {
    id: panel
    property color fillColor: "#171b22"
    property color strokeColor: "#2b313c"
    property color highlightColor: theme.accent
    property bool highlightTop: false
    property real corner: Math.min(8, theme.radius)

    color: fillColor
    radius: corner
    border.width: 1
    border.color: strokeColor

    Rectangle {
        visible: panel.highlightTop
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: 0
        anchors.leftMargin: 12
        width: Math.max(18, parent.width * 0.28)
        height: 1
        radius: 1
        color: panel.highlightColor
        opacity: 0.85
    }
}

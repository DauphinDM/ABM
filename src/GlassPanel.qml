import QtQuick

// Restrained glass for chrome and floating surfaces. Dense data stays on solid
// tiles; this avoids stacking blur across the whole window.
Rectangle {
    id: panel
    property color fillColor: "#cc12141a"
    property color strokeColor: "#2effffff"
    property color highlightColor: theme.accent
    property bool highlightTop: true
    property real corner: 10

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

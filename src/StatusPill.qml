import QtQuick
import QtQuick.Layouts

Rectangle {
    id: pill
    property string text: ""
    property bool active: false
    property bool warning: false
    property color accentColor: theme.accent
    implicitHeight: 24
    implicitWidth: row.implicitWidth + 16
    radius: 8
    color: active ? "#3310141c" : "#9910141a"
    border.width: 1
    border.color: warning ? "#66cf8a4a" : active ? "#55ffffff" : "#26ffffff"

    RowLayout {
        id: row
        anchors.centerIn: parent
        spacing: 6
        Rectangle {
            width: 6; height: 6; radius: 3
            color: pill.warning ? "#cf8a4a" : pill.active ? pill.accentColor : "#555966"
        }
        PlainLabel {
            text: pill.text
            color: pill.warning ? "#f0d3a0" : pill.active ? "#e8e8ed" : "#9b9ca9"
            font.pixelSize: 9
            maximumLineCount: 1
            elide: Text.ElideRight
        }
    }
}

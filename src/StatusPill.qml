import QtQuick
import QtQuick.Layouts

Rectangle {
    id: pill
    property string text: ""
    property bool active: false
    property bool warning: false
    property color accentColor: theme.accent
    implicitHeight: 26
    implicitWidth: row.implicitWidth + 16
    radius: Math.min(4, theme.radius)
    color: warning ? "#30271c" : active ? "#182a25" : "#202630"
    border.width: 1
    border.color: warning ? "#735b36" : active ? "#2c4c40" : "#343c48"

    RowLayout {
        id: row
        anchors.centerIn: parent
        spacing: 6
        Rectangle {
            width: 6; height: 6; radius: 3
            color: pill.warning ? "#e0b36e" : pill.active ? "#83d3ad" : "#8893a4"
        }
        PlainLabel {
            text: pill.text
            color: pill.warning ? "#f0d3a0" : pill.active ? "#e8e8ed" : "#9b9ca9"
            font.pixelSize: 10
            maximumLineCount: 1
            elide: Text.ElideRight
        }
    }
}

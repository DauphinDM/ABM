import QtQuick
import QtQuick.Controls.Basic as Basic

Basic.MenuItem {
    id: control
    font.pixelSize: 12
    implicitHeight: 36
    horizontalPadding: 12
    palette.windowText: "#edf1f7"
    contentItem: Text {
        text: control.text
        textFormat: Text.PlainText
        font: control.font
        color: control.enabled ? "#edf1f7" : "#858c9b"
        leftPadding: !control.mirrored && control.checkable ? 24 : 0
        rightPadding: control.mirrored && control.checkable ? 24 : control.subMenu ? 20 : 0
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: Rectangle {
        color: control.down ? "#343e4c" : control.highlighted ? "#2a3340" : "transparent"
        radius: Math.min(4, theme.radius)
        border.width: control.visualFocus ? 1 : 0
        border.color: theme.accent
    }
}

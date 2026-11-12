import QtQuick
import QtQuick.Controls.Basic as Basic

Basic.Menu {
    padding: 4
    font.pixelSize: 12
    delegate: MenuItem { }
    background: Rectangle {
        implicitWidth: 220
        implicitHeight: 40
        color: "#1c222c"
        radius: Math.min(8, theme.radius)
        border.color: "#465162"
    }
}

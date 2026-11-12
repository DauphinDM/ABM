import QtQuick
import QtQuick.Templates as T

T.Button {
    id: control
    property color accentColor: theme.accent
    property bool destructive: false
    readonly property color actionColor: destructive ? "#ef8179" : accentColor
    implicitWidth: Math.max(implicitBackgroundWidth, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight, implicitContentHeight + topPadding + bottomPadding)
    padding: 9
    horizontalPadding: 12
    spacing: 6
    font.pixelSize: 12
    font.weight: Font.DemiBold
    hoverEnabled: true

    contentItem: Text {
        text: control.text
        textFormat: Text.PlainText
        font: control.font
        color: !control.enabled ? "#858c9b" : control.highlighted ? (control.actionColor.r * .299 + control.actionColor.g * .587 + control.actionColor.b * .114 > .55 ? "#11151b" : "#ffffff") : control.checked || control.destructive ? control.actionColor : "#e8ecf2"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: Rectangle {
        objectName: "glassButtonSurface"
        implicitHeight: 36
        radius: Math.min(6, theme.radius)
        color: !control.enabled ? "#191d25" : control.down ? "#303743" : control.highlighted ? control.actionColor : control.checked ? Qt.alpha(control.actionColor, .12) : control.hovered ? "#29313d" : control.flat ? "transparent" : "#202630"
        border.width: control.visualFocus ? 2 : 1
        border.color: !control.enabled ? "#2b313b" : control.visualFocus || control.checked || control.destructive ? control.actionColor : control.highlighted ? "transparent" : control.hovered ? "#596475" : control.flat ? "transparent" : "#39424f"
        Behavior on color { ColorAnimation { duration: 100 } }
        Behavior on border.color { ColorAnimation { duration: 100 } }
    }
}

import QtQuick
import QtQuick.Templates as T

T.Button {
    id: control
    property color accentColor: theme.accent
    property bool destructive: false
    readonly property color actionColor: destructive ? "#ef8179" : accentColor
    implicitWidth: Math.max(implicitBackgroundWidth, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight, implicitContentHeight + topPadding + bottomPadding)
    padding: 7
    horizontalPadding: 12
    spacing: 6
    font.pixelSize: 11
    font.weight: Font.DemiBold
    hoverEnabled: true

    contentItem: Text {
        text: control.text
        textFormat: Text.PlainText
        font: control.font
        color: !control.enabled ? "#737885" : control.checked || control.highlighted || control.destructive ? control.actionColor : "#eef0f6"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: Rectangle {
        objectName: "glassButtonSurface"
        implicitHeight: 32
        radius: Math.min(8, theme.radius)
        gradient: Gradient {
            GradientStop { position: 0; color: !control.enabled ? "#151820" : control.down ? "#10131a" : control.checked || control.highlighted ? Qt.tint("#202633", Qt.alpha(control.actionColor, .22)) : control.hovered ? "#303744" : control.flat ? "#171c25" : "#252c38" }
            GradientStop { position: 1; color: !control.enabled ? "#101319" : control.down ? "#1b202a" : "#12161e" }
        }
        border.width: control.visualFocus ? 2 : 1
        border.color: !control.enabled ? "#29303b" : control.visualFocus || control.checked || control.highlighted || control.hovered || control.destructive ? Qt.alpha(control.actionColor, control.visualFocus ? 1 : .65) : "#465062"
        Behavior on border.color { ColorAnimation { duration: 100 } }
        Rectangle {
            anchors.top: parent.top; anchors.horizontalCenter: parent.horizontalCenter
            anchors.topMargin: 1
            width: Math.max(0, parent.width - 16); height: 1
            color: control.actionColor
            opacity: !control.enabled ? 0 : control.checked || control.highlighted ? .65 : control.hovered ? .4 : .12
        }
    }
}

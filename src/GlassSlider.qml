import QtQuick
import QtQuick.Templates as T

T.Slider {
    id: control
    implicitWidth: horizontal ? 160 : 28
    implicitHeight: horizontal ? 28 : 160
    padding: 8
    hoverEnabled: true
    background: Item {
        objectName: "glassSliderTrack"
        x: control.leftPadding + (control.horizontal ? control.handle.width / 2 : 0)
        y: control.topPadding + (control.horizontal ? 0 : control.handle.height / 2)
        width: control.availableWidth - (control.horizontal ? control.handle.width : 0)
        height: control.availableHeight - (control.horizontal ? 0 : control.handle.height)
        Rectangle {
            anchors.centerIn: parent
            width: control.horizontal ? parent.width : 6
            height: control.horizontal ? 6 : parent.height
            radius: 3
            color: "#080b10"
            border.width: 1; border.color: control.enabled ? "#485267" : "#29303b"
            Rectangle {
                x: control.horizontal && control.mirrored ? parent.width - width : 0
                y: control.horizontal ? 0 : parent.height - height
                width: control.horizontal ? control.position * parent.width : parent.width
                height: control.horizontal ? parent.height : control.position * parent.height
                radius: 3
                color: control.enabled ? theme.accent : "#4a5261"
                opacity: control.pressed ? 1 : .72
            }
        }
    }
    handle: Rectangle {
        objectName: "glassSliderHandle"
        x: control.leftPadding + (control.horizontal ? control.visualPosition * (control.availableWidth - width) : (control.availableWidth - width) / 2)
        y: control.topPadding + (control.horizontal ? (control.availableHeight - height) / 2 : control.visualPosition * (control.availableHeight - height))
        implicitWidth: 18; implicitHeight: 18
        radius: Math.min(5, theme.radius)
        color: !control.enabled ? "#252c38" : control.pressed ? theme.accent : "#dce3ef"
        border.width: control.visualFocus ? 2 : 1
        border.color: control.enabled && (control.visualFocus || control.hovered || control.pressed) ? theme.accent : "#657086"
        Rectangle {
            anchors.centerIn: parent
            width: control.horizontal ? 2 : 8
            height: control.horizontal ? 8 : 2
            radius: 1
            color: control.enabled ? "#242d3c" : "#657086"
        }
    }
}

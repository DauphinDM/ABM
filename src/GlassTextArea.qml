import QtQuick
import QtQuick.Templates as T

T.TextArea {
    id: control
    implicitWidth: Math.max(implicitBackgroundWidth, contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight, contentHeight + topPadding + bottomPadding)
    padding: 9
    font.pixelSize: 11
    color: enabled ? "#eef0f6" : "#737885"
    placeholderTextColor: "#929bad"
    selectionColor: Qt.alpha(theme.accent, .35)
    selectedTextColor: "#ffffff"
    selectByMouse: true
    hoverEnabled: true
    opacity: enabled ? 1 : .65
    FieldEditMenu { editor: control }
    background: FieldSurface { field: control; implicitHeight: 80 }
    Text {
        x: control.leftPadding; y: control.topPadding
        width: control.availableWidth; height: control.availableHeight
        text: control.placeholderText
        textFormat: Text.PlainText
        font: control.font
        color: control.placeholderTextColor
        wrapMode: Text.Wrap
        visible: !control.length && !control.preeditText
    }
}

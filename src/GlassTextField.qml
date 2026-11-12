import QtQuick
import QtQuick.Templates as T

T.TextField {
    id: control
    property bool invalidInput: text.length > 0 && !acceptableInput
    implicitWidth: Math.max(implicitBackgroundWidth, contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight, contentHeight + topPadding + bottomPadding)
    padding: 7
    leftPadding: 9; rightPadding: 9
    font.pixelSize: 12
    color: enabled ? "#eef0f6" : "#737885"
    placeholderTextColor: enabled ? "#929bad" : "#656d7d"
    selectionColor: Qt.alpha(theme.accent, .35)
    selectedTextColor: "#ffffff"
    verticalAlignment: TextInput.AlignVCenter
    selectByMouse: true
    hoverEnabled: true
    opacity: enabled ? 1 : .65
    FieldEditMenu { editor: control }
    background: FieldSurface { field: control; invalid: control.invalidInput }
    Text {
        x: control.leftPadding; y: control.topPadding
        width: control.availableWidth; height: control.availableHeight
        text: control.placeholderText
        textFormat: Text.PlainText
        font: control.font
        color: control.placeholderTextColor
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: control.horizontalAlignment
        elide: Text.ElideRight
        visible: !control.length && !control.preeditText
    }
}

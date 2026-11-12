import QtQuick
import QtQuick.Controls.Basic as Basic

Basic.CheckBox {
    id: control
    padding: 4
    spacing: 10
    font.pixelSize: 12
    indicator: Rectangle {
        implicitWidth: 20; implicitHeight: 20
        x: control.mirrored ? control.width - width - control.rightPadding : control.leftPadding
        y: (control.height - height) / 2
        radius: Math.min(4, theme.radius)
        color: control.checked ? Qt.alpha(theme.accent, .16) : "#151a22"
        border.width: control.visualFocus ? 2 : 1
        border.color: !control.enabled ? "#353e4b" : control.visualFocus || control.checked ? theme.accent : "#647184"
        Text {
            anchors.centerIn: parent
            text: control.checkState === Qt.PartiallyChecked ? "−" : "✓"
            textFormat: Text.PlainText
            visible: control.checkState !== Qt.Unchecked
            font.pixelSize: 15
            color: control.enabled ? theme.accent : "#858c9b"
        }
    }
    contentItem: Text {
        text: control.text
        textFormat: Text.PlainText
        font: control.font
        color: control.enabled ? "#edf1f7" : "#858c9b"
        leftPadding: control.mirrored ? 0 : control.indicator.width + control.spacing
        rightPadding: control.mirrored ? control.indicator.width + control.spacing : 0
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.Wrap
    }
}

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.Basic as Basic
import QtQuick.Controls.Basic.impl

T.ComboBox {
    id: control
    implicitWidth: Math.max(140, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(36, implicitContentHeight + topPadding + bottomPadding)
    padding: 7
    spacing: 8
    leftPadding: mirrored ? 29 : 9
    rightPadding: mirrored ? 9 : 29
    font.pixelSize: 12
    hoverEnabled: true
    opacity: enabled ? 1 : .65
    background: FieldSurface { field: control }
    indicator: Text {
        x: control.mirrored ? 9 : control.width - width - 9
        y: (control.height - height) / 2
        text: control.popup.visible ? "⌃" : "⌄"
        textFormat: Text.PlainText
        color: "#a6afbd"
        font.pixelSize: 15
    }
    contentItem: T.TextField {
        id: comboEditor
        implicitWidth: contentWidth
        implicitHeight: contentHeight
        text: control.editable ? control.editText : control.displayText
        enabled: control.editable
        autoScroll: control.editable
        readOnly: control.down
        font: control.font
        color: "#eef0f6"
        selectionColor: Qt.alpha(theme.accent, .35)
        selectedTextColor: "#ffffff"
        verticalAlignment: TextInput.AlignVCenter
        inputMethodHints: control.inputMethodHints
        validator: control.validator
        selectByMouse: control.selectTextByMouse
        clip: true
        FieldEditMenu { editor: comboEditor }
    }
    delegate: Basic.ItemDelegate {
        id: option
        required property var model
        required property int index
        width: ListView.view.width
        implicitHeight: 36
        text: model[control.textRole]
        highlighted: control.highlightedIndex === index
        hoverEnabled: true
        contentItem: Text {
            text: option.text
            textFormat: Text.PlainText
            font: control.font
            color: option.highlighted ? theme.accent : "#eef0f6"
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        background: Rectangle {
            color: option.highlighted || option.hovered ? Qt.alpha(theme.accent, .16) : "transparent"
        }
    }
    popup: Basic.Popup {
        y: control.height + 4
        width: control.width
        height: Math.min(contentItem.implicitHeight + topPadding + bottomPadding, control.Window.height - topMargin - bottomMargin)
        topMargin: 8; bottomMargin: 8
        padding: 4
        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            highlightMoveDuration: 0
            Basic.ScrollIndicator.vertical: Basic.ScrollIndicator { }
        }
        background: Rectangle {
            color: "#1c222c"
            radius: Math.min(8, theme.radius)
            border.width: 1; border.color: "#465162"
        }
    }
}

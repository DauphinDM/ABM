import QtQuick
import QtQuick.Templates as T

T.Dialog {
    id: control
    implicitWidth: Math.max(implicitBackgroundWidth, contentWidth + leftPadding + rightPadding, implicitHeaderWidth, implicitFooterWidth)
    implicitHeight: Math.max(implicitBackgroundHeight, contentHeight + topPadding + bottomPadding + (header && header.visible ? implicitHeaderHeight + spacing : 0) + (footer && footer.visible ? implicitFooterHeight + spacing : 0))
    padding: 12
    spacing: 8
    background: Rectangle {
        color: "#f20c1017"
        radius: Math.min(10, theme.radius)
        border.width: 1; border.color: Qt.alpha(theme.accent, .4)
    }
    header: Text {
        text: control.title
        textFormat: Text.PlainText
        visible: control.title.length > 0
        padding: 12
        font.pixelSize: 13; font.weight: Font.DemiBold
        color: "#eef0f6"
        elide: Text.ElideRight
    }
    footer: DialogButtonBox {
        visible: count > 0
    }
    T.Overlay.modal: Rectangle { color: "#9904080e" }
    T.Overlay.modeless: Rectangle { color: "#3304080e" }
}

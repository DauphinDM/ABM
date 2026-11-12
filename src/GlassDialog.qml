import QtQuick
import QtQuick.Templates as T

T.Dialog {
    id: control
    implicitWidth: Math.max(implicitBackgroundWidth, contentWidth + leftPadding + rightPadding, implicitHeaderWidth, implicitFooterWidth)
    implicitHeight: Math.max(implicitBackgroundHeight, contentHeight + topPadding + bottomPadding + (header && header.visible ? implicitHeaderHeight + spacing : 0) + (footer && footer.visible ? implicitFooterHeight + spacing : 0))
    padding: 20
    spacing: 12
    background: Rectangle {
        color: "#191e27"
        radius: Math.min(10, theme.radius)
        border.width: 1; border.color: "#465162"
    }
    header: Text {
        text: control.title
        textFormat: Text.PlainText
        visible: control.title.length > 0
        padding: 20
        bottomPadding: 8
        font.pixelSize: 18; font.weight: Font.DemiBold
        color: "#eef0f6"
        elide: Text.ElideRight
    }
    footer: DialogButtonBox {
        visible: count > 0
    }
    T.Overlay.modal: Rectangle { color: "#9904080e" }
    T.Overlay.modeless: Rectangle { color: "#3304080e" }
}

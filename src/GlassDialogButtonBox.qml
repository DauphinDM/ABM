import QtQuick
import QtQuick.Templates as T

T.DialogButtonBox {
    id: control
    implicitWidth: implicitContentWidth + leftPadding + rightPadding
    implicitHeight: implicitContentHeight + topPadding + bottomPadding
    contentWidth: contentItem.contentWidth
    padding: 10
    spacing: 8
    alignment: Qt.AlignRight
    delegate: Button { }
    contentItem: ListView {
        implicitWidth: contentWidth
        implicitHeight: 32
        model: control.contentModel
        spacing: control.spacing
        orientation: ListView.Horizontal
        boundsBehavior: Flickable.StopAtBounds
    }
    background: Rectangle { color: "#10141b" }
}

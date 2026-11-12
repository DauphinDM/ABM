import QtQuick
import QtQuick.Layouts

Rectangle {
    id: card
    required property string label
    required property string value
    property string unit: ""
    Layout.fillWidth: true
    implicitHeight: 72
    color: "#171c24"
    border.width: 1
    border.color: "#2c3440"
    radius: Math.min(6, theme.radius)
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 12; spacing: 6
        PlainLabel { text: card.label; color: "#a0aabb"; font.pixelSize: 10; font.letterSpacing: .8; Layout.fillWidth: true; elide: Text.ElideRight }
        RowLayout { Layout.fillWidth: true
            PlainLabel { text: card.value; font.pixelSize: 20; font.weight: Font.Medium; Layout.fillWidth: true; elide: Text.ElideRight; maximumLineCount: 1; color: "#edf1f7" }
            PlainLabel { text: card.unit; color: "#a0aabb"; font.pixelSize: 11; Layout.maximumWidth: card.width * .45; elide: Text.ElideRight; Layout.alignment: Qt.AlignBottom | Qt.AlignRight }
        }
    }
}

import QtQuick
import QtQuick.Layouts

Rectangle {
    id: card
    required property string label
    required property string value
    property string unit: ""
    Layout.fillWidth: true
    implicitHeight: 40
    color: "#d016171c"
    border.width: 1
    border.color: "#401c1f26"
    radius: 0
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 8; spacing: 1
        PlainLabel { text: card.label; color: "#9b9ca9"; font.pixelSize: 9; Layout.fillWidth: true }
        RowLayout { Layout.fillWidth: true
            PlainLabel { text: card.value; font.pixelSize: 13; font.weight: Font.DemiBold; Layout.fillWidth: true; elide: Text.ElideRight; maximumLineCount: 1; color: "#e8e8ed" }
            PlainLabel { text: card.unit; color: "#9b9ca9"; font.pixelSize: 9; Layout.alignment: Qt.AlignBottom | Qt.AlignRight }
        }
    }
}

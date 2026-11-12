import QtQuick
import QtQuick.Layouts

// A readable instrument panel rather than a decorative gauge. The tachometer
// is normalized to the vehicle's reported redline, not an invented RPM scale.
GlassPanel {
    id: cluster
    property real speedValue: 0
    property real rpm: 0
    property real redline: 8000
    property string speedText: "0"
    property string unit: "km/h"
    property string gearText: "N"
    property bool live: false
    property color accentColor: theme.accent
    implicitHeight: width < 370 ? 204 : 224
    readonly property real ratio: redline > 0 ? Math.max(0, Math.min(1, rpm / redline)) : 0
    readonly property bool hot: ratio > .9

    ColumnLayout {
        anchors.fill: parent; anchors.margins: cluster.width < 370 ? 16 : 24; spacing: 12
        RowLayout {
            Layout.fillWidth: true
            PlainLabel { Layout.fillWidth: true; text: qsTr("GROUND SPEED"); font.pixelSize: 10; font.letterSpacing: 1.5; color: "#a0aabb" }
            PlainLabel { text: qsTr("GEAR"); font.pixelSize: 10; font.letterSpacing: 1.5; color: "#a0aabb" }
        }
        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 8
            PlainLabel {
                text: cluster.speedText; color: "#f0f4fa"; font.pixelSize: cluster.width < 370 ? 64 : 80
                font.weight: Font.Light; font.letterSpacing: -3
            }
            PlainLabel { text: cluster.unit; color: "#a0aabb"; font.pixelSize: 14; Layout.alignment: Qt.AlignBottom; Layout.bottomMargin: 14 }
            Item { Layout.fillWidth: true }
            PlainLabel { text: cluster.gearText; color: cluster.hot ? "#ef8179" : cluster.accentColor; font.pixelSize: 46; font.weight: Font.Light }
        }
        RowLayout {
            Layout.fillWidth: true
            PlainLabel { Layout.fillWidth: true; text: qsTr("%1 RPM").arg(cluster.live ? Math.round(cluster.rpm).toLocaleString(Qt.locale(), 'f', 0) : "—"); color: "#d4dbe5"; font.pixelSize: 12 }
            PlainLabel { text: qsTr("Limit %1").arg(Math.round(cluster.redline).toLocaleString(Qt.locale(), 'f', 0)); color: "#a0aabb"; font.pixelSize: 10 }
        }
        RowLayout {
            Layout.fillWidth: true; spacing: 3
            Repeater {
                model: 32
                Rectangle {
                    Layout.fillWidth: true; implicitHeight: 6
                    color: cluster.live && cluster.ratio >= (index + 1) / 32 ? (index >= 28 ? "#ef8179" : cluster.accentColor) : "#303846"
                }
            }
        }
    }
}

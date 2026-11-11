import QtQuick
import QtQuick.Layouts

// Hero drive cluster: one glance for speed, RPM and selected gear.
Item {
    id: cluster
    property real speedValue: 0
    property real rpm: 0
    property real redline: 8000
    property string speedText: "0"
    property string unit: "km/h"
    property string gearText: "N"
    property bool live: false
    property color accentColor: theme.accent
    implicitHeight: 208

    readonly property real ratio: cluster.redline > 0 ? Math.max(0, Math.min(1, cluster.rpm / cluster.redline)) : 0
    readonly property bool hot: cluster.ratio > 0.9

    Canvas {
        id: arc
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -10
        width: 210; height: 172
        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            var cx = width / 2, cy = height * 0.82, radius = 86
            ctx.lineCap = "round"
            ctx.lineWidth = 5
            ctx.strokeStyle = "#331c1f26"
            ctx.beginPath(); ctx.arc(cx, cy, radius, Math.PI, Math.PI * 2); ctx.stroke()
            if (cluster.live && cluster.ratio > 0) {
                ctx.strokeStyle = cluster.hot ? "#df6b61" : cluster.accentColor
                ctx.lineWidth = 5
                ctx.beginPath(); ctx.arc(cx, cy, radius, Math.PI, Math.PI * (1 + cluster.ratio)); ctx.stroke()
            }
            for (var i = 0; i <= 12; ++i) {
                var a = Math.PI + Math.PI * (i / 12)
                var r1 = radius - 14, r2 = radius - 8
                ctx.strokeStyle = (i / 12) <= cluster.ratio ? (cluster.hot ? "#df6b61" : cluster.accentColor) : "#332f3440"
                ctx.lineWidth = 1
                ctx.beginPath()
                ctx.moveTo(cx + Math.cos(a) * r1, cy + Math.sin(a) * r1)
                ctx.lineTo(cx + Math.cos(a) * r2, cy + Math.sin(a) * r2)
                ctx.stroke()
            }
        }
        Connections { target: cluster; function onRatioChanged() { arc.requestPaint() } function onLiveChanged() { arc.requestPaint() } }
        Component.onCompleted: requestPaint()
    }

    ColumnLayout {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: arc.verticalCenter
        anchors.verticalCenterOffset: 28
        spacing: -2
        PlainLabel {
            Layout.alignment: Qt.AlignHCenter
            text: cluster.speedText
            color: "#f5f6fa"
            font.pixelSize: 48
            font.weight: Font.Light
            horizontalAlignment: Text.AlignHCenter
        }
        PlainLabel {
            Layout.alignment: Qt.AlignHCenter
            text: cluster.unit.toUpperCase()
            color: "#9b9ca9"
            font.pixelSize: 9
            font.letterSpacing: 2.4
        }
    }

    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 12
        width: 38; height: 38
        radius: 8
        color: "#d0101218"
        border.width: 1
        border.color: cluster.hot ? "#77df6b61" : "#33ffffff"
        PlainLabel {
            anchors.centerIn: parent
            text: cluster.gearText
            color: cluster.hot ? "#df6b61" : cluster.accentColor
            font.pixelSize: 18
            font.weight: Font.Bold
        }
    }

    Row {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 2
        spacing: 4
        Repeater {
            model: 10
            Rectangle {
                width: 13; height: 4; radius: 2
                property real threshold: (index + 1) / 10
                color: cluster.ratio >= threshold ? (index >= 8 ? "#df6b61" : index >= 6 ? "#cf8a4a" : cluster.accentColor) : "#20232b"
            }
        }
    }

    PlainLabel {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        text: qsTr("%1 RPM").arg(cluster.rpm > 0 ? Math.round(cluster.rpm) : "—")
        color: "#777b88"
        font.pixelSize: 8
    }
    PlainLabel {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        text: qsTr("REDLINE %1").arg(Math.round(cluster.redline))
        color: "#777b88"
        font.pixelSize: 8
    }
}

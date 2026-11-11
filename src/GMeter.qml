import QtQuick
import QtQuick.Layouts

// Compact live G dot. It shows the instantaneous vector, not a recorded
// friction circle or a measured tyre envelope.
Rectangle {
    id: meter
    property real lateral: 0
    property real longitudinal: 0
    property color accentColor: theme.accent
    implicitHeight: 82
    radius: 8
    color: "#d0101218"
    border.width: 1
    border.color: "#22ffffff"
    clip: true

    readonly property real magnitude: Math.min(2.5, Math.sqrt(lateral * lateral + longitudinal * longitudinal))
    readonly property real dotX: width / 2 + Math.max(-1, Math.min(1, lateral / 2)) * (width * 0.38)
    readonly property real dotY: height / 2 - Math.max(-1, Math.min(1, longitudinal / 2)) * (height * 0.34)

    Canvas {
        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            var cx = width / 2, cy = height / 2
            ctx.strokeStyle = "#2d3a4152"
            ctx.lineWidth = 1
            ctx.beginPath(); ctx.moveTo(0, cy); ctx.lineTo(width, cy); ctx.stroke()
            ctx.beginPath(); ctx.moveTo(cx, 0); ctx.lineTo(cx, height); ctx.stroke()
            ctx.strokeStyle = "#243a4152"
            ctx.beginPath(); ctx.arc(cx, cy, Math.min(width, height) * 0.38, 0, Math.PI * 2); ctx.stroke()
        }
        Component.onCompleted: requestPaint()
    }

    Rectangle {
        width: 7; height: 7; radius: 4
        x: meter.dotX - width / 2
        y: meter.dotY - height / 2
        color: meter.accentColor
        border.width: 1
        border.color: "#e8f5f6fa"
        Behavior on x { NumberAnimation { duration: 90; easing.type: Easing.OutCubic } }
        Behavior on y { NumberAnimation { duration: 90; easing.type: Easing.OutCubic } }
    }

    Column {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 7
        spacing: 0
        PlainLabel { text: qsTr("G METER"); color: "#9b9ca9"; font.pixelSize: 8; font.letterSpacing: 1.3 }
        PlainLabel { text: meter.magnitude.toFixed(2) + " g"; color: "#e8e8ed"; font.pixelSize: 11; font.weight: Font.DemiBold }
    }
    PlainLabel {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 7
        text: qsTr("LAT %1 · LONG %2").arg(meter.lateral.toFixed(2)).arg(meter.longitudinal.toFixed(2))
        color: "#777b88"
        font.pixelSize: 7
    }
}

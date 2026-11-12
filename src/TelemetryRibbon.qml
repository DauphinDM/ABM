import QtQuick
import QtQuick.Layouts

// Live sparkline with a soft gradient wash and end marker. It is a reading,
// not a precision chart: stale or missing telemetry is stated explicitly.
Item {
    id: ribbon
    property var samples: []
    property string field: "speed"
    property real multiplier: 1
    property real ceiling: 1
    property color lineColor: theme.accent
    property bool live: false
    property string caption: ""
    implicitHeight: 84

    function values() {
        var out = []
        for (var i = 0; i < (samples || []).length; ++i) {
            var v = Number(samples[i][field])
            if (isFinite(v) && v >= 0) out.push(v * multiplier)
        }
        return out
    }

    Canvas {
        id: chart
        anchors.fill: parent
        anchors.topMargin: 24
        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            var vals = ribbon.values()
            var maxValue = Math.max(ribbon.ceiling, 0.0001)
            for (var i = 0; i < vals.length; ++i) maxValue = Math.max(maxValue, vals[i])
            ctx.strokeStyle = "#303846"
            ctx.lineWidth = 1
            for (var g = 1; g <= 2; ++g) {
                var gy = height - 3 - (g / 3) * (height - 8)
                ctx.beginPath(); ctx.moveTo(0, gy); ctx.lineTo(width, gy); ctx.stroke()
            }
            if (vals.length < 2) return
            function px(index) { return (index / (vals.length - 1)) * width }
            function py(value) { return height - 3 - (value / maxValue) * (height - 8) }
            var grad = ctx.createLinearGradient(0, 0, 0, height)
            grad.addColorStop(0, Qt.rgba(ribbon.lineColor.r, ribbon.lineColor.g, ribbon.lineColor.b, 0.28))
            grad.addColorStop(1, Qt.rgba(ribbon.lineColor.r, ribbon.lineColor.g, ribbon.lineColor.b, 0))
            ctx.beginPath()
            ctx.moveTo(0, height)
            for (var j = 0; j < vals.length; ++j) ctx.lineTo(px(j), py(vals[j]))
            ctx.lineTo(width, height)
            ctx.closePath()
            ctx.fillStyle = grad
            ctx.fill()
            ctx.beginPath()
            for (var k = 0; k < vals.length; ++k) {
                if (k === 0) ctx.moveTo(px(k), py(vals[k]))
                else ctx.lineTo(px(k), py(vals[k]))
            }
            ctx.strokeStyle = ribbon.live ? ribbon.lineColor : "#666b78"
            ctx.lineWidth = 1.6
            ctx.stroke()
            var lastX = px(vals.length - 1), lastY = py(vals[vals.length - 1])
            ctx.beginPath(); ctx.arc(lastX, lastY, 2.6, 0, Math.PI * 2)
            ctx.fillStyle = ribbon.live ? ribbon.lineColor : "#666b78"; ctx.fill()
        }
        Connections {
            target: ribbon
            function onSamplesChanged() { chart.requestPaint() }
            function onLiveChanged() { chart.requestPaint() }
            function onMultiplierChanged() { chart.requestPaint() }
            function onCeilingChanged() { chart.requestPaint() }
        }
        Component.onCompleted: requestPaint()
    }

    PlainLabel {
        anchors.left: parent.left
        anchors.top: parent.top
        text: ribbon.caption
        color: "#9b9ca9"
        font.pixelSize: 10
        font.letterSpacing: .5
    }
    PlainLabel {
        anchors.right: parent.right
        anchors.top: parent.top
        text: ribbon.live ? qsTr("LIVE") : qsTr("NO DATA")
        color: ribbon.live ? "#7fb98e" : "#9b9ca9"
        font.pixelSize: 10
    }
    PlainLabel {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        visible: ribbon.values().length < 2
        text: qsTr("Waiting for telemetry…")
        color: "#777b88"
        font.pixelSize: 9
    }
}

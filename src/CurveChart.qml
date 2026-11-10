import QtQuick

// Bounded supported-torque curve readback. Not a measured dyno result.
Canvas {
    id: chart
    property var points: []
    property color lineColor: "#7aa2f7"
    property color gridColor: "#34363f"
    implicitHeight: 90
    onPointsChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()

    onPaint: {
        var ctx = getContext("2d")
        ctx.reset()
        ctx.fillStyle = "#121319"
        ctx.fillRect(0, 0, width, height)
        var list = (points || []).filter(function(p) { return p && isFinite(p.rpm) && isFinite(p.torque) })
        if (list.length < 2) return
        var maxRpm = 1, minT = 0, maxT = 1
        for (var i = 0; i < list.length; ++i) {
            maxRpm = Math.max(maxRpm, list[i].rpm)
            minT = Math.min(minT, list[i].torque)
            maxT = Math.max(maxT, list[i].torque)
        }
        maxT = Math.max(maxT, minT + 1)
        ctx.strokeStyle = gridColor
        ctx.lineWidth = 1
        ctx.beginPath()
        ctx.moveTo(0, height - 1); ctx.lineTo(width, height - 1)
        ctx.moveTo(0, 1); ctx.lineTo(0, height - 1)
        ctx.stroke()
        ctx.strokeStyle = lineColor
        ctx.lineWidth = 2
        ctx.beginPath()
        for (var j = 0; j < list.length; ++j) {
            var x = (list[j].rpm / maxRpm) * (width - 4) + 2
            var y = height - 4 - ((list[j].torque - minT) / (maxT - minT)) * (height - 8)
            if (j === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y)
        }
        ctx.stroke()
    }
}

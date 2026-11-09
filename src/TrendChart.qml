import QtQuick

Canvas {
    id: chart
    required property var samples
    property string field: "speed"
    property real multiplier: 1
    property color lineColor: theme.accent
    property real ceiling: 1
    implicitHeight: 50
    onSamplesChanged: if (visible) requestPaint()
    onWidthChanged: requestPaint()
    onVisibleChanged: if (visible) requestPaint()
    onMultiplierChanged: requestPaint()
    onCeilingChanged: requestPaint()
    onPaint: {
        var ctx = getContext("2d")
        ctx.reset()
        if (!visible || width < 1 || height < 1) return
        ctx.strokeStyle = "#4d1a1c22"; ctx.lineWidth = 1
        for (var row = 1; row < 3; row++) {
            ctx.beginPath(); ctx.moveTo(0, height * row / 3); ctx.lineTo(width, height * row / 3); ctx.stroke()
        }
        var values = samples || [], max = ceiling, drawing = false
        for (var i = 0; i < values.length; i++) {
            var v = values[i][field]
            if (typeof v === "number" && isFinite(v)) max = Math.max(max, v * multiplier)
        }
        if (values.length < 2) return
        ctx.beginPath(); ctx.strokeStyle = lineColor; ctx.lineWidth = 1.5
        for (var i = 0; i < values.length; i++) {
            var v = values[i][field]
            if (typeof v !== "number" || !isFinite(v)) { drawing = false; continue }
            var x = width * i / (values.length - 1)
            var y = height - 3 - Math.max(0, v * multiplier) / max * (height - 6)
            if (!drawing) ctx.moveTo(x, y); else ctx.lineTo(x, y)
            drawing = true
        }
        ctx.stroke()
    }
}

import QtQuick
import QtQuick.Layouts

Item {
    id: gauge
    required property real speed
    required property real rpm
    property real redline: 8000
    property string speedText: "—"
    property string unit: "km/h"
    property string gearText: "—"
    implicitHeight: 140
    Canvas {
        id: dial
        anchors.centerIn: parent
        width: 120; height: 90
        onPaint: {
            var c = getContext("2d"); c.reset()
            var cx = width/2, cy = 60, r = 52
            var start = Math.PI * .82, sweep = Math.PI * 1.36
            c.lineWidth = 4; c.lineCap = "round"
            c.strokeStyle = "#4d1a1c22"; c.beginPath(); c.arc(cx, cy, r, start, start+sweep); c.stroke()
            c.strokeStyle = gauge.rpm > gauge.redline * .9 ? "#cf8a4a" : theme.accent
            c.beginPath(); c.arc(cx, cy, r, start, start+sweep*Math.min(1,gauge.rpm/gauge.redline)); c.stroke()
            c.strokeStyle = "#4d1a1c22"; c.lineWidth = 1
            for (var i = 0; i <= 10; i++) {
                var a = start + sweep * (i / 10)
                var x1 = cx + (r-6) * Math.cos(a), y1 = cy + (r-6) * Math.sin(a)
                var x2 = cx + r * Math.cos(a), y2 = cy + r * Math.sin(a)
                c.beginPath(); c.moveTo(x1, y1); c.lineTo(x2, y2); c.stroke()
            }
        }
        Connections { target: gauge; function onRpmChanged() { dial.requestPaint() } function onRedlineChanged() { dial.requestPaint() } }
        Connections { target: theme; function onChanged() { dial.requestPaint() } }
    }
    ColumnLayout {
        anchors.centerIn: parent; anchors.verticalCenterOffset: 4; spacing: 0
        PlainLabel { text: gauge.speedText; font.pixelSize: 32; color: theme.accent; font.weight: Font.Light; Layout.alignment: Qt.AlignHCenter }
        PlainLabel { text: gauge.unit; color: "#9b9ca9"; font.pixelSize: 9; Layout.alignment: Qt.AlignHCenter }
        PlainLabel { text: gauge.gearText; font.pixelSize: 16; font.weight: Font.Bold; Layout.topMargin: 2; Layout.alignment: Qt.AlignHCenter; color: "#e8e8ed" }
    }
}

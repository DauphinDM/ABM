import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: editor
    property var current: []
    property string generation: ""
    property string text: ""
    property bool dirty: false
    property bool loading: false
    property real referenceRPM: 0
    property color foreground: "#eeeeee"
    property color muted: "#aaaaaa"
    property color surface: "#26282f"
    spacing: 4
    readonly property var forward: text.trim() ? text.trim().split(/[,;\s]+/) : []
    readonly property var rows: {
        var entries=(current || []).filter(function(g) { return g.index <= 0 })
        return entries.concat(forward.map(function(r,i) { return {index:i+1,ratio:Number(r)} }))
    }
    function actual(index) {
        var found=(current || []).filter(function(g) { return g.index === index })
        return found.length ? found[0].ratio : undefined
    }
    function retainedRPM(index) {
        if (index<=1 || !isFinite(referenceRPM) || referenceRPM<=0) return "—"
        var previous=Number(forward[index-2]),ratio=Number(forward[index-1])
        return isFinite(previous) && previous>0 && isFinite(ratio) && ratio>0 ? String(Math.round(referenceRPM*ratio/previous)) : "—"
    }
    function loadCurrent() {
        loading=true
        text=(current || []).filter(function(g) { return g.index > 0 }).sort(function(a,b) { return a.index-b.index }).map(function(g) { return String(g.ratio) }).join(", ")
        loading=false; dirty=false
    }
    function edit(index,value) {
        var next=forward.slice(); next[index-1]=value; text=next.join(", ")
    }
    function addGear() {
        var values=forward.map(Number), factors=[]
        for (var i=Math.max(1,values.length-3);i<values.length;++i) {
            var factor=values[i]/values[i-1]
            if (isFinite(factor) && factor>0 && factor<1) factors.push(factor)
        }
        if (!factors.length) return
        factors.sort(function(a,b) { return a-b })
        var middle=Math.floor(factors.length/2)
        var retention=factors.length%2 ? factors[middle] : (factors[middle-1]+factors[middle])/2
        var next=Number((values[values.length-1]*retention).toFixed(4))
        if (next>=.01 && next<=100) text += ", "+next
    }
    readonly property bool canSuggest: forward.length>=2 && forward.slice(1).some(function(r,i) { var q=Number(r)/Number(forward[i]); return isFinite(q) && q>0 && q<1 })
    onCurrentChanged: if (!dirty) loadCurrent()
    onGenerationChanged: loadCurrent()
    onTextChanged: if (!loading) dirty=true
    RowLayout {
        Layout.fillWidth: true
        PlainLabel { text: qsTr("Gear"); color: editor.muted; font.pixelSize: 10; Layout.preferredWidth: 30 }
        PlainLabel { text: qsTr("Actual"); color: editor.muted; font.pixelSize: 10; Layout.fillWidth: true }
        PlainLabel { text: qsTr("Edited ratio"); color: editor.muted; font.pixelSize: 10; Layout.preferredWidth: 90 }
        PlainLabel { text: qsTr("Shift RPM*"); color: editor.muted; font.pixelSize: 10; Layout.preferredWidth: 75 }
    }
    ListView {
        Layout.fillWidth: true; Layout.preferredHeight: 148; clip: true; spacing: 3
        model: editor.rows
        ScrollBar.vertical: ScrollBar {}
        delegate: RowLayout {
            required property var modelData
            width: ListView.view.width; height: 28
            PlainLabel { Layout.preferredWidth: 30; text: modelData.index<0 ? "R"+Math.abs(modelData.index) : modelData.index===0 ? "N" : String(modelData.index); color: editor.foreground; font.pixelSize: 11 }
            PlainLabel { Layout.fillWidth: true; text: editor.actual(modelData.index)===undefined ? "—" : editor.actual(modelData.index).toFixed(4); color: editor.muted; font.pixelSize: 11 }
            TextField { Layout.preferredWidth: 90; implicitHeight: 28; padding: 4; font.pixelSize: 11; color: editor.foreground
                text: String(modelData.ratio); enabled: modelData.index>0 && editor.enabled; selectByMouse: true
                onEditingFinished: if (!editor.loading && modelData.index>0 && text!==String(modelData.ratio)) editor.edit(modelData.index,text)
                background: Rectangle { color: editor.surface }
            }
            PlainLabel { Layout.preferredWidth: 75; text: editor.retainedRPM(modelData.index); color: editor.muted; font.pixelSize: 10 }
        }
    }
    PlainLabel { Layout.fillWidth: true; text: editor.dirty ? qsTr("Edited draft · actual data keeps updating without replacing edits.") : qsTr("Actual ratios from selected vehicle · reverse/neutral are read-only."); color: editor.muted; font.pixelSize: 10; wrapMode: Text.Wrap }
    PlainLabel { Layout.fillWidth: true; text: qsTr("*RPM immediately after upshift at the reported limiter; theoretical, no clutch slip or controller delay."); color: editor.muted; font.pixelSize: 9; wrapMode: Text.Wrap }
}

import QtQuick
import QtQuick.Controls
import "Garage.js" as Garage

ComboBox {
    id: choice
    property var entries: []
    property string field: ""
    property string allLabel: ""
    property string selectedValue: ""
    signal chosen(string value)
    textRole: "label"; valueRole: "value"
    model: Garage.options(entries,field,allLabel)
    currentIndex: {
        for (var i=0; i<model.length; ++i) if (model[i].value === selectedValue) return i
        return 0
    }
    enabled: model.length > 1
    font.pixelSize: 10; implicitHeight: 28; padding: 4
    onActivated: chosen(currentValue)
    ToolTip.visible: hovered
    ToolTip.text: allLabel
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Pane {
    id: panel
    default property alias contents: body.data
    padding: 16
    spacing: 12
    background: GlassPanel { }
    contentItem: ColumnLayout { id: body; spacing: panel.spacing }
}

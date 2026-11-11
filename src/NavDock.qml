import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Compact floating navigation. Becomes a fixed left rail on wide windows.
Item {
    id: dock
    property int currentPage: 0
    property bool vertical: false
    property color accentColor: theme.accent
    property var labels: [qsTr("DRIVE"), qsTr("CAR"), qsTr("GARAGE"), qsTr("SESSION")]
    signal selected(int page)

    implicitWidth: vertical ? 66 : 260
    implicitHeight: vertical ? 4 * 52 + 12 : 46

    GlassPanel {
        anchors.fill: parent
        corner: 12
        fillColor: "#e6101116"
        highlightTop: false
    }

    GridLayout {
        anchors.fill: parent
        anchors.margins: 6
        columns: dock.vertical ? 1 : 4
        rowSpacing: 2
        columnSpacing: 2

        Repeater {
            model: dock.labels.length
            delegate: Item {
                id: cell
                required property int index
                Layout.fillWidth: true
                Layout.fillHeight: true
                readonly property bool active: dock.currentPage === index
                Rectangle {
                    anchors.fill: parent
                    radius: 8
                    color: cell.active ? Qt.alpha(dock.accentColor, .18) : (hover.containsMouse ? "#18ffffff" : "transparent")
                    border.width: cell.active ? 1 : 0
                    border.color: Qt.alpha(dock.accentColor, .45)
                    Behavior on color { ColorAnimation { duration: 120 } }
                }
                Rectangle {
                    width: dock.vertical ? 3 : 16
                    height: dock.vertical ? 16 : 3
                    radius: 2
                    color: dock.accentColor
                    opacity: cell.active ? 1 : 0
                    anchors.horizontalCenter: dock.vertical ? undefined : parent.horizontalCenter
                    anchors.verticalCenter: dock.vertical ? parent.verticalCenter : undefined
                    anchors.left: dock.vertical ? parent.left : undefined
                    anchors.bottom: dock.vertical ? undefined : parent.bottom
                    anchors.leftMargin: dock.vertical ? 2 : 0
                    anchors.bottomMargin: dock.vertical ? 0 : 2
                    Behavior on opacity { NumberAnimation { duration: 140 } }
                }
                PlainLabel {
                    anchors.centerIn: parent
                    text: dock.labels[index]
                    color: cell.active ? dock.accentColor : "#9b9ca9"
                    font.pixelSize: dock.vertical ? 8 : 9
                    font.letterSpacing: 1.1
                    font.bold: cell.active
                }
                MouseArea {
                    id: hover
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: dock.selected(cell.index)
                }
            }
        }
    }
}

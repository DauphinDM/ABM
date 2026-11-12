import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// The same keyboard-focusable navigation becomes a workspace rail when wide.
Item {
    id: dock
    property int currentPage: 0
    property bool vertical: false
    property color accentColor: theme.accent
    property var labels: [qsTr("Drive"), qsTr("Car"), qsTr("Garage"), qsTr("Session")]
    signal selected(int page)
    implicitWidth: vertical ? 144 : 280
    implicitHeight: vertical ? 208 : 44

    Rectangle {
        visible: !dock.vertical
        anchors.bottom: parent.bottom
        width: parent.width; height: 1; color: "#303846"
    }
    GridLayout {
        anchors.fill: parent
        columns: dock.vertical ? 1 : 4
        rowSpacing: 8; columnSpacing: 4
        Repeater {
            model: dock.labels.length
            Button {
                id: tab
                required property int index
                readonly property bool active: dock.currentPage === index
                Layout.fillWidth: true; Layout.fillHeight: true
                padding: dock.vertical ? 12 : 4
                text: dock.labels[index]
                Accessible.role: Accessible.PageTab
                Accessible.name: text
                Accessible.description: qsTr("Keyboard shortcut %1").arg(index + 1)
                onClicked: dock.selected(index)
                contentItem: RowLayout {
                    spacing: 10
                    PlainLabel {
                        visible: dock.vertical
                        text: "0" + (tab.index + 1)
                        color: tab.active ? dock.accentColor : "#747f91"
                        font.pixelSize: 10
                    }
                    PlainLabel {
                        Layout.fillWidth: true
                        text: tab.text
                        horizontalAlignment: dock.vertical ? Text.AlignLeft : Text.AlignHCenter
                        color: tab.active ? "#f1f4f9" : "#a4adbd"
                        font.pixelSize: 13; font.weight: tab.active ? Font.DemiBold : Font.Normal
                    }
                }
                background: Rectangle {
                    radius: Math.min(6, theme.radius)
                    color: tab.active && dock.vertical ? "#232a35" : tab.hovered || tab.visualFocus ? "#1e2530" : "transparent"
                    border.width: tab.visualFocus ? 1 : 0; border.color: dock.accentColor
                    Rectangle {
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: dock.vertical ? undefined : parent.right
                        anchors.top: dock.vertical ? parent.top : undefined
                        width: dock.vertical ? 2 : parent.width
                        height: dock.vertical ? parent.height : 2
                        color: dock.accentColor; visible: tab.active
                    }
                }
            }
        }
    }
}

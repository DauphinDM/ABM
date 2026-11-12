import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

SectionPanel {
    id: pane
    property var updater: null
    property bool disabled: false
    PlainLabel { text: qsTr("App updates"); font.pixelSize: 17; font.weight: Font.DemiBold; color: "#edf1f7" }
    PlainLabel { Layout.fillWidth: true; color: "#a0aabb"; font.pixelSize: 12
        text: qsTr("Checks contact the official GitHub repository, which receives your IP address. Downloads are SHA-256 checked. Installation is never silent and never interrupts your game automatically.") }
    CheckBox { Layout.fillWidth: true; text: qsTr("Check automatically at startup and daily"); enabled: !!pane.updater && !pane.disabled; checked: !!(pane.updater && pane.updater.automatic); onToggled: pane.updater.automatic=checked }
    CheckBox { Layout.fillWidth: true; text: qsTr("Include preview releases"); enabled: !!pane.updater && !pane.disabled && !pane.updater.busy; checked: !!(pane.updater && pane.updater.previews); onToggled: pane.updater.previews=checked }
    CheckBox { Layout.fillWidth: true; text: qsTr("Download verified updates automatically"); enabled: !!pane.updater && !pane.disabled; checked: !!(pane.updater && pane.updater.autoDownload); onToggled: pane.updater.autoDownload=checked }
    PlainLabel { Layout.fillWidth: true; color: "#edf1f7"; font.pixelSize: 12; text: pane.updater ? qsTr("Installed: %1").arg(pane.updater.currentVersion)+"\n"+pane.updater.message : qsTr("Update checking is unavailable.") }
    ProgressBar { Layout.fillWidth: true; visible: !!(pane.updater && pane.updater.busy); value: pane.updater ? pane.updater.progress : 0 }
    Flow {
        Layout.fillWidth: true; spacing: 8
        Button { objectName: "checkAppUpdates"; text: qsTr("Check now"); enabled: !!pane.updater && !pane.disabled && !pane.updater.busy; onClicked: pane.updater.check() }
        Button { text: qsTr("Release notes"); visible: !!(pane.updater && pane.updater.available); enabled: !pane.disabled; onClicked: pane.updater.openRelease() }
        Button { text: qsTr("Download update"); visible: !!(pane.updater && pane.updater.canDownload && !pane.updater.downloaded); enabled: !!pane.updater && !pane.disabled && !pane.updater.busy; onClicked: pane.updater.download() }
        Button { text: qsTr("Open verified download / installer"); visible: !!(pane.updater && pane.updater.downloaded); enabled: !!pane.updater && !pane.disabled && !pane.updater.busy; onClicked: pane.updater.openDownload() }
        Button { text: qsTr("Cancel transfer"); visible: !!(pane.updater && pane.updater.busy); enabled: !pane.disabled; onClicked: pane.updater.cancel() }
    }
    PlainLabel { Layout.fillWidth: true; color: "#a0aabb"; font.pixelSize: 12; text: pane.updater ? pane.updater.installHelp : "" }
}

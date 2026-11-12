import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    id: pane
    property var host: null
    property bool demo: false
    clip: true; contentWidth: availableWidth
    ColumnLayout {
        width: parent.width; spacing: 16
        SectionPanel {
            Layout.fillWidth: true
            PlainLabel { text: qsTr("ABM on your phone"); color: "#edf1f7"; font.pixelSize: 20; font.weight: Font.DemiBold }
            PlainLabel { Layout.fillWidth: true; color: "#a0aabb"; font.pixelSize: 13
                text: qsTr("Connect Android ABM Mobile over private Wi-Fi. The PC remains in charge of every request. No internet relay, port forwarding, or game UDP exposure.") }
            StatusPill { text: pane.host && pane.host.connectedDevice ? qsTr("PHONE APPROVED") : pane.host && pane.host.running ? qsTr("PAIRING / LISTENING") : qsTr("ACCESS OFF"); active: !!(pane.host && pane.host.connectedDevice) }
            ComboBox { id: address; objectName: "mobileAddress"; Layout.fillWidth: true; model: pane.demo ? ["192.168.1.20"] : pane.host ? pane.host.addresses : []; enabled: !pane.demo && !!pane.host && !pane.host.running }
            Button { objectName: "mobileEnable"; Layout.fillWidth: true; highlighted: true
                text: pane.host && pane.host.running ? qsTr("Stop access & revoke phone") : qsTr("Enable phone connection")
                enabled: !pane.demo && !!pane.host && (pane.host.running || address.count > 0)
                onClicked: if (pane.host.running) pane.host.stop(); else pane.host.start(address.currentText)
            }
            PlainLabel { Layout.fillWidth: true; color: "#a0aabb"; font.pixelSize: 12
                text: pane.demo ? qsTr("Demo mode: networking is disabled.") : pane.host ? pane.host.message : qsTr("Phone connection is unavailable.") }
            PlainLabel { visible: !!(pane.host && pane.host.connectedDevice); Layout.fillWidth: true; color: "#edf1f7"; font.pixelSize: 13
                text: qsTr("Connected phone: %1").arg(pane.host ? pane.host.connectedDevice : "") }
        }
        SectionPanel {
            visible: !!(pane.host && pane.host.pairingLink); Layout.fillWidth: true
            PlainLabel { text: qsTr("1. Copy the private pairing link"); color: "#edf1f7"; font.pixelSize: 15 }
            PlainLabel { Layout.fillWidth: true; color: "#a0aabb"; font.pixelSize: 12
                text: qsTr("Paste it into Connect in ABM Mobile. It contains the certificate fingerprint and a temporary secret; share it only with your own phone. It expires after two minutes. QR scanning is not included in this first version.") }
            Button { text: qsTr("Copy pairing link"); Layout.fillWidth: true; onClicked: pane.host.copyPairingLink() }
        }
        SectionPanel {
            visible: !!(pane.host && pane.host.pendingDevice); Layout.fillWidth: true
            PlainLabel { text: qsTr("2. Approve your phone"); color: "#edf1f7"; font.pixelSize: 15 }
            PlainLabel { Layout.fillWidth: true; text: pane.host ? pane.host.pendingDevice : ""; color: "#edf1f7"; font.pixelSize: 14 }
            PlainLabel { Layout.fillWidth: true; text: qsTr("Only approve if you just initiated pairing on your phone. The device name is self-reported, not proof of identity."); color: "#a0aabb"; font.pixelSize: 12 }
            RowLayout { Layout.fillWidth: true
                Button { text: qsTr("Approve control"); highlighted: true; Layout.fillWidth: true; onClicked: pane.host.approve() }
                Button { text: qsTr("Decline"); Layout.fillWidth: true; onClicked: pane.host.reject() }
            }
        }
        SectionPanel {
            Layout.fillWidth: true
            PlainLabel { text: qsTr("Safe by default"); color: "#edf1f7"; font.pixelSize: 15 }
            PlainLabel { Layout.fillWidth: true; color: "#a0aabb"; font.pixelSize: 12
                text: qsTr("TLS encryption and a pinned certificate protect the connection. Approval lasts only for this connection; reconnecting requires pairing again. Remote changes require a stopped, owned vehicle and server permission. Never forward port 44445 to the internet. Allow only private-network TCP access in your firewall. First-time certificate creation requires OpenSSL on the PC's PATH (Windows may need it installed separately).") }
        }
    }
}

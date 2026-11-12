import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    id: pane
    property var host: null
    property var firewall: null
    property bool demo: false
    readonly property bool firewallConfirmationVisible: firewallConfirm.visible
    function dismissFirewallConfirmation() { firewallConfirm.reject() }
    clip: true; contentWidth: availableWidth
    ColumnLayout {
        width: parent.width; spacing: 16
        SectionPanel {
            Layout.fillWidth: true
            PlainLabel { text: qsTr("ABM on your phone"); color: "#edf1f7"; font.pixelSize: 20; font.weight: Font.DemiBold }
            PlainLabel { Layout.fillWidth: true; color: "#a0aabb"; font.pixelSize: 13
                text: qsTr("Connect Android or iPhone ABM Mobile over private Wi-Fi. The PC remains in charge of every request. No internet relay, port forwarding, or game UDP exposure.") }
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
            PlainLabel { text: qsTr("Firewall & connection help"); color: "#edf1f7"; font.pixelSize: 15 }
            PlainLabel { Layout.fillWidth: true; color: "#a0aabb"; font.pixelSize: 12
                text: qsTr("If the phone stays on Connecting securely to PC, the firewall may be dropping its requests. Use the same trusted LAN, allow Local Network access on iPhone, and approve the phone on PC. After setup, stop and enable access again for a fresh two-minute link.") }
            PlainLabel { Layout.fillWidth: true; color: "#edf1f7"; font.pixelSize: 12
                text: pane.demo ? qsTr("Demo: no firewall changes are available.") : pane.firewall ? pane.firewall.message : qsTr("Check inbound TCP port 44445 in your system firewall. Never disable the firewall or forward this port.") }
            PlainLabel { Layout.fillWidth: true; visible: !pane.demo && !!pane.firewall && !!pane.firewall.scope; color: "#a0aabb"; font.pixelSize: 12
                text: pane.firewall ? pane.firewall.scope : "" }
            Button { objectName: "mobileFirewallSetup"; Layout.fillWidth: true
                text: pane.firewall && pane.firewall.busy ? qsTr("Waiting for administrator…") : qsTr("Allow LAN pairing through firewall…")
                enabled: !pane.demo && !!pane.firewall && pane.firewall.supported && !pane.firewall.busy
                onClicked: { firewallConfirm.confirmedScope=pane.firewall.scope; firewallConfirm.open() } }
        }
        SectionPanel {
            Layout.fillWidth: true
            PlainLabel { text: qsTr("Safe by default"); color: "#edf1f7"; font.pixelSize: 15 }
            PlainLabel { Layout.fillWidth: true; color: "#a0aabb"; font.pixelSize: 12
                text: qsTr("TLS encryption and a pinned certificate protect the connection. Approval lasts only for this connection; reconnecting requires pairing again. Remote changes require a stopped, owned vehicle and server permission. Never forward port 44445 to the internet. Allow only private-network TCP access in your firewall. First-time certificate creation requires OpenSSL on the PC's PATH (Windows may need it installed separately).") }
        }
    }
    Dialog {
        id: firewallConfirm
        objectName: "mobileFirewallConfirm"
        property string confirmedScope: ""
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(parent.width-24,480)
        height: Math.min(parent.height-24,460)
        title: qsTr("Allow local-network pairing?")
        modal: true; standardButtons: Dialog.Ok | Dialog.Cancel
        onOpened: standardButton(Dialog.Ok).text=qsTr("Request access")
        contentItem: ScrollView {
            clip: true; contentWidth: availableWidth
            PlainLabel { width: parent.width; wrapMode: Text.WordWrap; color: "#edf1f7"; font.pixelSize: 13
                text: qsTr("Ask your system administrator prompt to allow inbound TCP for:\n%1\n\nThis permits devices on this subnet, not just one phone. Windows also restricts the rule to this ABM executable and Private network profiles. TLS and PC approval remain required. The rule persists after ABM closes; remove it in your firewall settings when no longer needed. No firewall is disabled and no router ports are forwarded.").arg(firewallConfirm.confirmedScope) }
        }
        onAccepted: if (!pane.demo && pane.firewall) pane.firewall.requestAccess(confirmedScope)
    }
}

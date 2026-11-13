import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    id: pane
    property var host: null
    property var firewall: null
    property var usb: null
    property var updater: null
    readonly property string activeLink: transport.currentIndex>0 && usb ? usb.pairingLink : host ? host.pairingLink : ""
    property bool demo: false
    readonly property bool firewallConfirmationVisible: firewallConfirm.visible
    function dismissFirewallConfirmation() { firewallConfirm.reject() }
    clip: true; contentWidth: availableWidth
    ColumnLayout {
        width: parent.width; spacing: 16
        SectionPanel {
            Layout.fillWidth: true
            PlainLabel { text: qsTr("ABM on your phone or browser"); color: "#edf1f7"; font.pixelSize: 20; font.weight: Font.DemiBold }
            PlainLabel { Layout.fillWidth: true; color: "#a0aabb"; font.pixelSize: 13
                text: qsTr("Connect Android or iPhone ABM Mobile, or open the web control page from an older device on the same private network. The PC remains in charge of every request. No internet relay, port forwarding, or game UDP exposure.") }
            StatusPill { text: pane.host && pane.host.connectedDevice ? qsTr("DEVICE APPROVED") : pane.host && pane.host.running ? qsTr("PAIRING / LISTENING") : qsTr("ACCESS OFF"); active: !!(pane.host && pane.host.connectedDevice) }
            ComboBox { id: transport; Layout.fillWidth: true; model: [qsTr("Private Wi-Fi / LAN"),qsTr("iPhone USB cable"),qsTr("Android USB cable")]; enabled: !pane.demo && !!pane.host && !pane.host.running }
            ComboBox { id: address; objectName: "mobileAddress"; Layout.fillWidth: true; visible: transport.currentIndex===0; model: pane.demo ? ["192.168.1.20"] : pane.host ? pane.host.addresses : []; enabled: !pane.demo && !!pane.host && !pane.host.running }
            Button { objectName: "mobileEnable"; Layout.fillWidth: true; highlighted: true
                text: pane.host && pane.host.running ? qsTr("Stop remote access") : qsTr("Enable remote access")
                enabled: !pane.demo && !!pane.host && (pane.host.running || (transport.currentIndex===0 ? address.count>0 : !!pane.usb))
                onClicked: {
                    if (pane.host.running) { if (pane.usb) pane.usb.stop(); pane.host.stop() }
                    else if (transport.currentIndex>0 && pane.usb) pane.usb.start(transport.currentIndex===1 ? "iphone" : "android")
                    else pane.host.start(address.currentText)
                }
            }
            PlainLabel { Layout.fillWidth: true; color: "#a0aabb"; font.pixelSize: 12
                text: pane.demo ? qsTr("Demo mode: networking is disabled.") : pane.host ? pane.host.message : qsTr("Phone connection is unavailable.") }
            PlainLabel { visible: !!(pane.host && pane.host.connectedDevice); Layout.fillWidth: true; color: "#edf1f7"; font.pixelSize: 13
                text: qsTr("Connected device: %1").arg(pane.host ? pane.host.connectedDevice : "") }
            PlainLabel { visible: transport.currentIndex>0; Layout.fillWidth: true; color: "#a0aabb"; font.pixelSize: 12
                text: (pane.usb ? pane.usb.message : "")+"\n"+qsTr("iPhone: install iproxy/libusbmuxd and Apple USB drivers where needed, unlock/trust the PC, then enable USB mode in the mobile app. Android: adb and explicit USB-debugging authorization are required. Cable mode binds only to loopback and does not open the LAN firewall. Keep only the intended phone connected.") }
        }
        SectionPanel {
            visible: !!(pane.host && pane.host.running); Layout.fillWidth: true
            PlainLabel { text: qsTr("Browser control for older devices"); color: "#edf1f7"; font.pixelSize: 15 }
            PlainLabel { Layout.fillWidth: true; color: "#a0aabb"; font.pixelSize: 12
                text: qsTr("Open this HTTPS address on an older phone, tablet, or computer on the same private network. Your browser will warn about the PC's self-signed certificate; accept it only for this PC and never over the internet.") }
            PlainLabel { objectName: "mobileBrowserUrl"; Layout.fillWidth: true; text: pane.host ? pane.host.browserUrl : ""; color: "#edf1f7"; font.pixelSize: 13; wrapMode: Text.WrapAnywhere }
            Button { objectName: "mobileCopyBrowserUrl"; text: qsTr("Copy browser address"); Layout.fillWidth: true; enabled: !pane.demo && !!pane.host && !!pane.host.browserUrl; onClicked: pane.host.copyBrowserUrl() }
            PlainLabel { Layout.fillWidth: true; color: "#a0aabb"; font.pixelSize: 12
                text: qsTr("Paste the same private pairing link into the page, then approve the browser here. Browser sessions are HTTPS-only, revocable, and limited to the same fixed actions and game checks as the mobile apps.") }
        }
        SectionPanel {
            visible: !!pane.activeLink; Layout.fillWidth: true
            PlainLabel { text: qsTr("1. Copy the private pairing link"); color: "#edf1f7"; font.pixelSize: 15 }
            PlainLabel { Layout.fillWidth: true; color: "#a0aabb"; font.pixelSize: 12
                text: qsTr("Paste it into Connect in ABM Mobile or the browser page. It contains the certificate fingerprint and a temporary secret; share it only with your own device. It expires after two minutes. QR scanning is not included in this first version.") }
            Button { text: qsTr("Copy pairing link"); Layout.fillWidth: true; onClicked: if (transport.currentIndex>0 && pane.usb) pane.usb.copyPairingLink(); else pane.host.copyPairingLink() }
        }
        SectionPanel {
            visible: !!(pane.host && pane.host.pendingDevice); Layout.fillWidth: true
            PlainLabel { text: qsTr("2. Approve your device"); color: "#edf1f7"; font.pixelSize: 15 }
            PlainLabel { Layout.fillWidth: true; text: pane.host ? pane.host.pendingDevice : ""; color: "#edf1f7"; font.pixelSize: 14 }
            PlainLabel { Layout.fillWidth: true; text: qsTr("Only approve if you just initiated pairing on your phone or browser. The device name is self-reported, not proof of identity."); color: "#a0aabb"; font.pixelSize: 12 }
            CheckBox { id: rememberPhone; text: qsTr("Remember this device until I revoke it"); checked: true }
            RowLayout { Layout.fillWidth: true
                Button { text: qsTr("Approve control"); highlighted: true; Layout.fillWidth: true; onClicked: pane.host.approveRemembered(rememberPhone.checked) }
                Button { text: qsTr("Decline"); Layout.fillWidth: true; onClicked: pane.host.reject() }
            }
        }
        SectionPanel {
            visible: !!(pane.host && pane.host.rememberedCount); Layout.fillWidth: true
            PlainLabel { Layout.fillWidth: true; text: qsTr("Remembered devices: %1").arg(pane.host ? pane.host.rememberedCount : 0); color: "#edf1f7"; font.pixelSize: 15 }
            PlainLabel { Layout.fillWidth: true; text: qsTr("Remembered devices can reconnect without a new link or approval, but only while you enable access. Forgetting disconnects and revokes every saved device."); color: "#a0aabb"; font.pixelSize: 12 }
            Button { text: qsTr("Forget all devices & stop access"); enabled: !pane.demo; Layout.fillWidth: true; onClicked: pane.host.forgetDevices() }
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
        UpdatesPane { Layout.fillWidth: true; updater: pane.updater; disabled: pane.demo }
        SectionPanel {
            Layout.fillWidth: true
            PlainLabel { text: qsTr("Safe by default"); color: "#edf1f7"; font.pixelSize: 15 }
            PlainLabel { Layout.fillWidth: true; color: "#a0aabb"; font.pixelSize: 12
                text: qsTr("Native apps pin the PC's TLS certificate. Browsers use HTTPS with the PC's local certificate; verify that you're connecting to your PC before accepting its certificate warning. The browser page uses an HttpOnly, Secure, SameSite session cookie and the same PC approval and fixed game actions. Remembering a device stores a revocable credential; stop closes all connections, Forget devices revokes saved credentials. Vehicle edits require a stopped, owned vehicle and server permission; spawning into an empty map does not require an existing vehicle. Never forward the port to the internet. ABM creates its local certificate automatically; no separate OpenSSL installation is needed.") }
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

#include "companionfirewall.h"
#include "mobilehost.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QNetworkInterface>
#include <QRegularExpression>
#include <QStringConverter>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {
QString psQuote(QString value) { return "'" + value.replace("'", "''") + "'"; }
}
QString CompanionFirewall::Rule::scope() const {
    return valid() ? QString("%1 → %2:%3/TCP (%4)").arg(subnet,address).arg(port).arg(interfaceName) : QString();
}
CompanionFirewall::Rule CompanionFirewall::ruleFor(const QString &address, int prefix,
        const QString &interfaceName, const QString &application, int port) {
    const QHostAddress host(address);
    // Never guess /24, permit all private networks, or widen a public/loopback bind.
    // Unusually broad or host-only masks require manual administrator review.
    if (host.protocol()!=QAbstractSocket::IPv4Protocol || !MobileHost::localAddress(host) || host.isLoopback() || host.toString() != address
            || prefix < 16 || prefix > 30 || port < 1 || port > 65535
            || interfaceName.isEmpty() || interfaceName.size() > 128
            || application.isEmpty() || application.size() > 4096) return {};
    const quint32 mask = 0xffffffffu << (32-prefix);
    const QHostAddress first(host.toIPv4Address() & mask), last(first.toIPv4Address() | ~mask);
    if (!MobileHost::localAddress(first) || !MobileHost::localAddress(last)) return {};
    return {address,first.toString()+"/"+QString::number(prefix),interfaceName,application,port};
}
QStringList CompanionFirewall::ufwArguments(const Rule &rule) {
    // Interface names are arguments, not shell text; reject option-shaped input too.
    static const QRegularExpression name("^[A-Za-z0-9][A-Za-z0-9_.:-]{0,63}$");
    if (!rule.valid() || !name.match(rule.interfaceName).hasMatch()) return {};
    return {"allow","in","on",rule.interfaceName,"proto","tcp","from",rule.subnet,
            "to",rule.address,"port",QString::number(rule.port),"comment","ABM mobile companion LAN"};
}
QString CompanionFirewall::windowsScript(const Rule &rule) {
    if (!rule.valid()) return {};
    const auto id = QString::fromLatin1(QCryptographicHash::hash(
        (rule.scope()+rule.application).toUtf8(),QCryptographicHash::Sha256).toHex().left(20));
    // Existing named rules are never silently broadened or overwritten. A mismatched
    // administrator-edited rule needs explicit review in Windows Firewall.
    return "$ErrorActionPreference='Stop'; $name='ABM-Mobile-"+id+"'; "
        "$r=Get-NetFirewallRule -Name $name -ErrorAction SilentlyContinue; "
        "if ($r) { "
        "$p=$r | Get-NetFirewallPortFilter; $a=$r | Get-NetFirewallAddressFilter; "
        "$app=$r | Get-NetFirewallApplicationFilter; "
        "if ($r.Enabled -ne 'True' -or $r.Direction -ne 'Inbound' -or $r.Action -ne 'Allow' "
        "-or $r.Profile -ne 'Private' -or $p.Protocol -ne 'TCP' -or $p.LocalPort -ne '"+QString::number(rule.port)+"' "
        "-or $a.LocalAddress -ne "+psQuote(rule.address)+" -or $a.RemoteAddress -ne "+psQuote(rule.subnet)+" "
        "-or $app.Program -ne "+psQuote(QDir::toNativeSeparators(rule.application))+") "
        "{ throw 'Existing ABM rule differs; review it in Windows Firewall.' } "
        "} else { New-NetFirewallRule -Name $name -DisplayName 'ABM Mobile companion (private LAN)' "
        "-Direction Inbound -Action Allow -Enabled True -Profile Private -Protocol TCP -LocalPort "+QString::number(rule.port)+
        " -LocalAddress "+psQuote(rule.address)+" -RemoteAddress "+psQuote(rule.subnet)+
        " -Program "+psQuote(QDir::toNativeSeparators(rule.application))+" | Out-Null };";
}
QString CompanionFirewall::encodedScript(const QString &script) {
    QStringEncoder encoder(QStringConverter::Utf16LE);
    const QByteArray bytes = encoder(script);
    return QString::fromLatin1(bytes.toBase64());
}
CompanionFirewall::CompanionFirewall(MobileHost *host, QObject *parent) : QObject(parent), m_host(host) {
#ifdef Q_OS_LINUX
    for (const auto &path : {QString("/usr/bin/ufw"),QString("/usr/sbin/ufw")})
        if (QFileInfo(path).isExecutable()) { m_tool=path; break; }
    if (QFileInfo("/usr/bin/pkexec").isExecutable()) m_elevation="/usr/bin/pkexec";
#elif defined(Q_OS_WIN)
    wchar_t directory[MAX_PATH];
    const auto count=GetSystemDirectoryW(directory,MAX_PATH);
    if (count && count < MAX_PATH) {
        const auto path=QString::fromWCharArray(directory)+"/WindowsPowerShell/v1.0/powershell.exe";
        if (QFileInfo(path).isExecutable()) m_tool=path;
    }
#endif
    connect(host,&MobileHost::changed,this,&CompanionFirewall::refresh);
    connect(&m_process,&QProcess::errorOccurred,this,[this](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart) return;
        m_busy=false; m_message=tr("Could not open the administrator prompt. No firewall access was confirmed."); emit changed();
    });
    connect(&m_process,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus status) {
        m_busy=false;
        refresh();
        m_message = status==QProcess::NormalExit && code==0
            ? tr("Firewall request completed. Retry with a fresh pairing link. Windows also requires a Private network profile. The rule remains until removed in your firewall settings.")
            : tr("Firewall request was canceled or failed (exit %1). No connection is confirmed. Check your firewall settings; never disable the firewall or forward the port.").arg(code);
        emit changed();
    });
    // Do not surface administrative output, account names or unbounded tool output.
    connect(&m_process,&QProcess::readyReadStandardOutput,this,[this] { m_process.readAllStandardOutput(); });
    connect(&m_process,&QProcess::readyReadStandardError,this,[this] { m_process.readAllStandardError(); });
    refresh();
}
CompanionFirewall::~CompanionFirewall() {
    if (m_process.state()!=QProcess::NotRunning) { m_process.kill(); m_process.waitForFinished(1000); }
}
bool CompanionFirewall::supported() const {
#ifdef Q_OS_LINUX
    return m_host->running() && m_rule.valid() && !m_tool.isEmpty() && !m_elevation.isEmpty() && !ufwArguments(m_rule).isEmpty();
#elif defined(Q_OS_WIN)
    return m_host->running() && m_rule.valid() && !m_tool.isEmpty();
#else
    return false;
#endif
}
void CompanionFirewall::refresh() {
    if (m_busy) return;
    Rule next;
    if (m_host->running()) {
        for (const auto &adapter : QNetworkInterface::allInterfaces())
            for (const auto &entry : adapter.addressEntries())
                if (entry.ip()==m_host->listenAddress())
                    next=ruleFor(entry.ip().toString(),entry.prefixLength(),adapter.name(),
                                 QCoreApplication::applicationFilePath(),m_host->listenPort());
    }
    const auto binding=m_host->running() ? m_host->listenAddress().toString()+":"+QString::number(m_host->listenPort()) : QString();
    if (m_rule.scope()!=next.scope() || m_binding!=binding || m_message.isEmpty()) {
        m_rule=next;
        m_binding=binding;
        m_message = !m_host->running() ? tr("Enable phone access first. If the phone stays on Connecting securely, check inbound TCP access here.")
            : supported() ? tr("A firewall can silently block pairing. After confirmation, ABM can request local-subnet TCP access using your system administrator prompt. Windows rules apply only to Private networks.")
            : tr("Automatic setup is unavailable for this interface or firewall. Allow only inbound TCP to the displayed PC address and port from your trusted LAN using your firewall manager. Linux automatic setup requires UFW and pkexec; other firewalls need manual setup.");
    }
    emit changed();
}
void CompanionFirewall::requestAccess(const QString &confirmedScope) {
    refresh();
    if (m_busy || !supported() || !m_host->running() || confirmedScope!=scope()) {
        if (!m_busy) { m_message=tr("Connection settings changed or automatic setup is unavailable. Review the current scope before retrying."); emit changed(); }
        return;
    }
    m_busy=true; m_message=tr("Waiting for administrator approval. Cancel the system prompt to leave the firewall unchanged."); emit changed();
#ifdef Q_OS_LINUX
    m_process.start(m_elevation,QStringList{m_tool}+ufwArguments(m_rule));
#elif defined(Q_OS_WIN)
    const auto command=encodedScript(windowsScript(m_rule));
    const auto elevation="$ErrorActionPreference='Stop'; $p=Start-Process -FilePath "+psQuote(QDir::toNativeSeparators(m_tool))+
        " -Verb RunAs -Wait -PassThru -ArgumentList @('-NoProfile','-NonInteractive','-EncodedCommand',"+psQuote(command)+"); exit $p.ExitCode";
    m_process.start(m_tool,{"-NoProfile","-NonInteractive","-EncodedCommand",encodedScript(elevation)});
#endif
}

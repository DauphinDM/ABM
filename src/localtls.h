#pragma once
#include <QByteArray>

namespace LocalTls {
// Generated in-process. Private keys never leave the PC or use a shared template.
bool generate(QByteArray &certificate, QByteArray &privateKey);
}

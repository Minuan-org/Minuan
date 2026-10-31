#pragma once

#include <QSet>
#include <QString>
#include <QUrl>

namespace NavigationUtils {

QString cleanHost(QString host);
QUrl resolveAddressInput(const QString& input);
bool isAllowedHost(const QString& candidate, const QSet<QString>& allowedHosts);
bool isAllowedHttpsUrl(const QUrl& url, const QSet<QString>& allowedHosts);

} // namespace NavigationUtils

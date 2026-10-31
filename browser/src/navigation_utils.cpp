#include "navigation_utils.hpp"

#include <QUrlQuery>

namespace NavigationUtils {

QString cleanHost(QString host) {
    host = host.trimmed().toLower();
    if (host.startsWith(QStringLiteral("www."))) {
        host.remove(0, 4);
    }
    while (host.startsWith(QStringLiteral("*."))) {
        host.remove(0, 2);
    }
    while (host.endsWith(QLatin1Char('.'))) {
        host.chop(1);
    }
    return host;
}

QUrl resolveAddressInput(const QString& input) {
    const QString text = input.trimmed();
    if (text.isEmpty()) {
        return {};
    }

    const bool looksLikeAddress =
        text.contains(QStringLiteral("://")) ||
        text.contains(QLatin1Char('.')) ||
        text.startsWith(QStringLiteral("localhost"), Qt::CaseInsensitive);

    if (!looksLikeAddress || text.contains(QLatin1Char(' '))) {
        QUrl searchUrl(QStringLiteral("https://www.google.co.uk/search"));
        QUrlQuery query;
        query.addQueryItem(QStringLiteral("q"), text);
        searchUrl.setQuery(query);
        return searchUrl;
    }

    QUrl url = QUrl::fromUserInput(text);
    if (url.scheme() == QStringLiteral("http") &&
        !text.contains(QStringLiteral("://"))) {
        url.setScheme(QStringLiteral("https"));
    }
    return url;
}

bool isAllowedHost(const QString& candidate,
                   const QSet<QString>& allowedHosts) {
    const QString host = candidate.trimmed().toLower();
    if (host.isEmpty()) {
        return false;
    }

    for (const QString& configuredHost : allowedHosts) {
        const QString allowed = cleanHost(configuredHost);
        if (allowed.isEmpty()) {
            continue;
        }
        if (host == allowed ||
            host.endsWith(QStringLiteral(".") + allowed)) {
            return true;
        }
    }
    return false;
}

bool isAllowedHttpsUrl(const QUrl& url,
                       const QSet<QString>& allowedHosts) {
    if (!url.isValid() ||
        url.scheme().compare(QStringLiteral("https"),
                             Qt::CaseInsensitive) != 0 ||
        url.host().isEmpty()) {
        return false;
    }

    // Do not let an approved hostname hide credentials or another service port.
    if (url.authority().contains(QLatin1Char('@'))) {
        return false;
    }

    const int port = url.port(-1);
    if (port != -1 && port != 443) {
        return false;
    }

    return isAllowedHost(url.host(), allowedHosts);
}

} // namespace NavigationUtils

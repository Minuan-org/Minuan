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

bool isAllowedNavigationUrl(const QUrl& url,
                            const QSet<QString>& allowedHosts,
                            bool isMainFrame) {
    if (!url.isValid() || url.isEmpty()) {
        return false;
    }

    const QString scheme = url.scheme().toLower();

    // Empty documents and srcdoc frames are browser-internal HTML documents.
    if (scheme == QStringLiteral("about")) {
        const QString address = url.toString().toLower();
        return address == QStringLiteral("about:blank") ||
               (!isMainFrame && address == QStringLiteral("about:srcdoc"));
    }

    // Data/blob documents are commonly used by modern sites inside frames.
    // Keep them confined to subframes; they cannot replace the top-level page.
    if (!isMainFrame &&
        (scheme == QStringLiteral("data") ||
         scheme == QStringLiteral("blob"))) {
        return true;
    }

    // The address bar remains restricted to approved HTTPS websites. HTTPS
    // subframes may use other origins for sign-in, embedded content and app UI.
    if (!isMainFrame && scheme == QStringLiteral("https")) {
        if (url.host().isEmpty() ||
            url.authority().contains(QLatin1Char('@'))) {
            return false;
        }
        const int port = url.port(-1);
        return port == -1 || port == 443;
    }

    return isAllowedHttpsUrl(url, allowedHosts);
}

} // namespace NavigationUtils

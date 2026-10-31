#include "navigation_utils.hpp"

#include <QUrlQuery>

#include <iostream>

namespace {

bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        return false;
    }
    return true;
}

} // namespace

int main() {
    bool passed = true;

    const QUrl search = NavigationUtils::resolveAddressInput(
        QStringLiteral("SimVerse ideas"));
    const QUrlQuery searchQuery(search);
    passed &= expect(search.scheme() == QStringLiteral("https"),
                     "search uses HTTPS");
    passed &= expect(search.host() == QStringLiteral("www.google.co.uk"),
                     "search uses the UK Google domain");
    passed &= expect(search.path() == QStringLiteral("/search"),
                     "search uses the search endpoint");
    passed &= expect(searchQuery.queryItemValue(QStringLiteral("q")) ==
                         QStringLiteral("SimVerse ideas"),
                     "search text is URL encoded and preserved");

    const QUrl address = NavigationUtils::resolveAddressInput(
        QStringLiteral("google.com"));
    passed &= expect(address.scheme() == QStringLiteral("https"),
                     "plain website names use HTTPS");
    passed &= expect(address.host() == QStringLiteral("google.com"),
                     "plain website names keep their host");

    const QSet<QString> allowed{
        QStringLiteral("google.co.uk"),
        QStringLiteral("google.com"),
        QStringLiteral("github.com")
    };

    passed &= expect(NavigationUtils::isAllowedHttpsUrl(
                         QUrl(QStringLiteral("https://www.google.co.uk")),
                         allowed),
                     "configured host is allowed");
    passed &= expect(NavigationUtils::isAllowedHttpsUrl(
                         QUrl(QStringLiteral("https://maps.google.co.uk")),
                         allowed),
                     "subdomains of configured hosts are allowed");
    passed &= expect(!NavigationUtils::isAllowedHttpsUrl(
                         QUrl(QStringLiteral("https://evilgoogle.co.uk")),
                         allowed),
                     "similar but unrelated hosts are blocked");
    passed &= expect(!NavigationUtils::isAllowedHttpsUrl(
                         QUrl(QStringLiteral("http://google.co.uk")),
                         allowed),
                     "plain HTTP is blocked");
    passed &= expect(!NavigationUtils::isAllowedHttpsUrl(
                         QUrl(QStringLiteral("https://visitor:secret@github.com")),
                         allowed),
                     "approved hosts cannot be reached with hidden URL credentials");
    passed &= expect(!NavigationUtils::isAllowedHttpsUrl(
                         QUrl(QStringLiteral("https://github.com:8443")),
                         allowed),
                     "non-standard HTTPS ports are blocked");
    passed &= expect(NavigationUtils::isAllowedHttpsUrl(
                         QUrl(QStringLiteral("https://github.com:443")),
                         allowed),
                     "the standard HTTPS port is allowed");
    passed &= expect(!NavigationUtils::isAllowedHttpsUrl(
                         QUrl(QStringLiteral("https://unlisted.example")),
                         allowed),
                     "unlisted hosts are blocked");
    passed &= expect(NavigationUtils::cleanHost(
                         QStringLiteral(" WWW.Google.co.uk. ")) ==
                         QStringLiteral("google.co.uk"),
                     "host configuration is normalized");

    return passed ? 0 : 1;
}

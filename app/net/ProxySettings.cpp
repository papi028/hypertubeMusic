#include "ProxySettings.h"

#include "Connectivity.h"
#include "core/Logging.h"

#include <QCoreApplication>
#include <QNetworkProxyFactory>
#include <QNetworkProxyQuery>
#include <QSettings>
#include <QUrl>

namespace {

const QString kModeKey = QStringLiteral("proxy/mode");
const QString kTypeKey = QStringLiteral("proxy/type");
const QString kHostKey = QStringLiteral("proxy/host");
const QString kPortKey = QStringLiteral("proxy/port");
const QString kUserKey = QStringLiteral("proxy/user");
const QString kPasswordKey = QStringLiteral("proxy/password");

}

namespace net {

ProxySettings::ProxySettings(QObject *parent)
    : QObject(parent)
{
}

ProxySettings &ProxySettings::instance()
{
    static auto *settings = new ProxySettings(QCoreApplication::instance());
    return *settings;
}

ProxySettings *ProxySettings::create(QQmlEngine *, QJSEngine *)
{
    QQmlEngine::setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
    return &instance();
}

void ProxySettings::restore()
{
    const QSettings settings;
    m_mode = Mode(settings.value(kModeKey, int(System)).toInt());
    m_type = Type(settings.value(kTypeKey, int(Http)).toInt());
    m_host = settings.value(kHostKey).toString();
    m_port = settings.value(kPortKey, 8080).toInt();
    m_user = settings.value(kUserKey).toString();
    m_password = settings.value(kPasswordKey).toString();
    apply();
}

void ProxySettings::save()
{
    QSettings settings;
    settings.setValue(kModeKey, int(m_mode));
    settings.setValue(kTypeKey, int(m_type));
    settings.setValue(kHostKey, m_host);
    settings.setValue(kPortKey, m_port);
    settings.setValue(kUserKey, m_user);
    settings.setValue(kPasswordKey, m_password);
}

void ProxySettings::setMode(Mode mode)
{
    if (m_mode == mode)
        return;
    m_mode = mode;
    save();
    apply();
}

void ProxySettings::setType(Type type)
{
    if (m_type == type)
        return;
    m_type = type;
    save();
    apply();
}

void ProxySettings::setHost(const QString &host)
{
    if (m_host == host)
        return;
    m_host = host;
    save();
    apply();
}

void ProxySettings::setPort(int port)
{
    if (m_port == port)
        return;
    m_port = port;
    save();
    apply();
}

void ProxySettings::setUser(const QString &user)
{
    if (m_user == user)
        return;
    m_user = user;
    save();
    apply();
}

void ProxySettings::setPassword(const QString &password)
{
    if (m_password == password)
        return;
    m_password = password;
    save();
    apply();
}

void ProxySettings::refreshSystemProxy()
{
    apply();
}

QNetworkProxy ProxySettings::resolveSystemProxy() const
{
    const QNetworkProxyQuery query(QUrl(QStringLiteral("https://music.youtube.com/")));
    const QList<QNetworkProxy> proxies = QNetworkProxyFactory::systemProxyForQuery(query);
    for (const QNetworkProxy &proxy : proxies) {
        if (proxy.type() != QNetworkProxy::NoProxy && !proxy.hostName().isEmpty())
            return proxy;
    }
    return QNetworkProxy(QNetworkProxy::NoProxy);
}

QString ProxySettings::effectiveProxy() const
{
    const QString mpv = effectiveMpvProxy();
    return mpv.isEmpty() ? tr("Direct connection") : mpv;
}

QString ProxySettings::effectiveMpvProxy() const
{
    if (m_mode == Direct)
        return {};

    if (m_mode == Custom) {
        const QString trimmedHost = m_host.trimmed();
        if (trimmedHost.isEmpty() || m_port <= 0)
            return {};
        const QString scheme =
            m_type == Socks5 ? QStringLiteral("socks5://") : QStringLiteral("http://");
        QString auth;
        if (!m_user.isEmpty()) {
            auth = QString::fromUtf8(QUrl::toPercentEncoding(m_user)) + QLatin1Char(':')
                + QString::fromUtf8(QUrl::toPercentEncoding(m_password)) + QLatin1Char('@');
        }
        return scheme + auth + trimmedHost + QLatin1Char(':') + QString::number(m_port);
    }

    const QNetworkProxy sys = resolveSystemProxy();
    if (sys.type() == QNetworkProxy::NoProxy || sys.hostName().isEmpty())
        return {};

    QString scheme = QStringLiteral("http://");
    if (sys.type() == QNetworkProxy::Socks5Proxy)
        scheme = QStringLiteral("socks5://");

    QString auth;
    if (!sys.user().isEmpty()) {
        auth = QString::fromUtf8(QUrl::toPercentEncoding(sys.user())) + QLatin1Char(':')
            + QString::fromUtf8(QUrl::toPercentEncoding(sys.password())) + QLatin1Char('@');
    }
    return scheme + auth + sys.hostName() + QLatin1Char(':') + QString::number(sys.port());
}

QString ProxySettings::systemProxySummary() const
{
    const QNetworkProxy sys = resolveSystemProxy();
    if (sys.type() == QNetworkProxy::NoProxy || sys.hostName().isEmpty())
        return tr("No proxy detected in system settings");

    const QString typeName = sys.type() == QNetworkProxy::Socks5Proxy ? QStringLiteral("SOCKS5")
                                                                      : QStringLiteral("HTTP");
    return QStringLiteral("%1 %2:%3").arg(typeName, sys.hostName()).arg(sys.port());
}

void ProxySettings::apply()
{
    if (m_mode == Direct) {
        QNetworkProxyFactory::setUseSystemConfiguration(false);
        QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::NoProxy));
    } else if (m_mode == Custom) {
        QNetworkProxyFactory::setUseSystemConfiguration(false);
        const QString trimmedHost = m_host.trimmed();
        if (trimmedHost.isEmpty() || m_port <= 0) {
            QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::NoProxy));
        } else {
            const QNetworkProxy::ProxyType pType =
                m_type == Socks5 ? QNetworkProxy::Socks5Proxy : QNetworkProxy::HttpProxy;
            const QNetworkProxy proxy(pType, trimmedHost, quint16(m_port), m_user, m_password);
            QNetworkProxy::setApplicationProxy(proxy);
        }
    } else {
        QNetworkProxyFactory::setUseSystemConfiguration(true);
    }

    const QString mpv = effectiveMpvProxy();
    if (!mpv.isEmpty()) {
        const QByteArray utf8 = mpv.toUtf8();
        qputenv("http_proxy", utf8);
        qputenv("https_proxy", utf8);
        qputenv("all_proxy", utf8);
        qputenv("HTTP_PROXY", utf8);
        qputenv("HTTPS_PROXY", utf8);
        qputenv("ALL_PROXY", utf8);
    } else {
        qunsetenv("http_proxy");
        qunsetenv("https_proxy");
        qunsetenv("all_proxy");
        qunsetenv("HTTP_PROXY");
        qunsetenv("HTTPS_PROXY");
        qunsetenv("ALL_PROXY");
    }

    if (mpv != m_lastEffectiveMpvProxy) {
        m_lastEffectiveMpvProxy = mpv;
        Q_EMIT effectiveProxyChanged(mpv);
    }

    Q_EMIT changed();
    Q_EMIT systemProxyChanged();
    Connectivity::instance().check();
}

}

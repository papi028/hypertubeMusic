#pragma once

#include <QNetworkProxy>
#include <QObject>
#include <QQmlEngine>
#include <QString>

namespace net {

class ProxySettings : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY changed)
    Q_PROPERTY(Type type READ type WRITE setType NOTIFY changed)
    Q_PROPERTY(QString host READ host WRITE setHost NOTIFY changed)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY changed)
    Q_PROPERTY(QString user READ user WRITE setUser NOTIFY changed)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY changed)
    Q_PROPERTY(QString effectiveProxy READ effectiveProxy NOTIFY changed)
    Q_PROPERTY(QString systemProxySummary READ systemProxySummary NOTIFY systemProxyChanged)

public:
    enum Mode {
        System,
        Custom,
        Direct,
    };
    Q_ENUM(Mode)

    enum Type {
        Http,
        Socks5,
    };
    Q_ENUM(Type)

    explicit ProxySettings(QObject *parent = nullptr);

    static ProxySettings &instance();
    static ProxySettings *create(QQmlEngine *, QJSEngine *);

    void restore();
    void apply();

    Mode mode() const { return m_mode; }
    Type type() const { return m_type; }
    const QString &host() const { return m_host; }
    int port() const { return m_port; }
    const QString &user() const { return m_user; }
    const QString &password() const { return m_password; }

    QString effectiveProxy() const;
    QString effectiveMpvProxy() const;
    QString systemProxySummary() const;

    void setMode(Mode mode);
    void setType(Type type);
    void setHost(const QString &host);
    void setPort(int port);
    void setUser(const QString &user);
    void setPassword(const QString &password);

    Q_INVOKABLE void refreshSystemProxy();

Q_SIGNALS:
    void changed();
    void effectiveProxyChanged(const QString &mpvProxyUrl);
    void systemProxyChanged();

private:
    void save();
    QNetworkProxy resolveSystemProxy() const;

    Mode m_mode = System;
    Type m_type = Http;
    QString m_host;
    int m_port = 8080;
    QString m_user;
    QString m_password;
    QString m_lastEffectiveMpvProxy;
};

}

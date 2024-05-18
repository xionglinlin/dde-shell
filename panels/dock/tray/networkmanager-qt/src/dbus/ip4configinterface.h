/*
 * This file was generated by qdbusxml2cpp version 0.8
 * Command line was:
 *
 * qdbusxml2cpp is Copyright (C) 2016 The Qt Company Ltd.
 *
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

#ifndef IP4CONFIGINTERFACE_H
#define IP4CONFIGINTERFACE_H

#include "generictypes.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QtDBus>

/*
 * Proxy class for interface org.freedesktop.NetworkManager.IP4Config
 */
class OrgFreedesktopNetworkManagerIP4ConfigInterface : public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
#ifdef NMQT_STATIC
    {
        return "org.kde.fakenetwork.IP4Config";
    }
#else
    {
        return "org.freedesktop.NetworkManager.IP4Config";
    }
#endif

public:
    OrgFreedesktopNetworkManagerIP4ConfigInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = nullptr);

    ~OrgFreedesktopNetworkManagerIP4ConfigInterface() override;

    Q_PROPERTY(NMVariantMapList AddressData READ addressData)
    inline NMVariantMapList addressData() const
    {
        return qvariant_cast<NMVariantMapList>(property("AddressData"));
    }

    Q_PROPERTY(UIntListList Addresses READ addresses)
    inline UIntListList addresses() const
    {
        return qvariant_cast<UIntListList>(property("Addresses"));
    }

    Q_PROPERTY(QStringList DnsOptions READ dnsOptions)
    inline QStringList dnsOptions() const
    {
        return qvariant_cast<QStringList>(property("DnsOptions"));
    }

    Q_PROPERTY(int DnsPriority READ dnsPriority)
    inline int dnsPriority() const
    {
        return qvariant_cast<int>(property("DnsPriority"));
    }

    Q_PROPERTY(QStringList Domains READ domains)
    inline QStringList domains() const
    {
        return qvariant_cast<QStringList>(property("Domains"));
    }

    Q_PROPERTY(QString Gateway READ gateway)
    inline QString gateway() const
    {
        return qvariant_cast<QString>(property("Gateway"));
    }

    Q_PROPERTY(UIntList Nameservers READ nameservers)
    inline UIntList nameservers() const
    {
        return qvariant_cast<UIntList>(property("Nameservers"));
    }

    Q_PROPERTY(NMVariantMapList RouteData READ routeData)
    inline NMVariantMapList routeData() const
    {
        return qvariant_cast<NMVariantMapList>(property("RouteData"));
    }

    Q_PROPERTY(UIntListList Routes READ routes)
    inline UIntListList routes() const
    {
        return qvariant_cast<UIntListList>(property("Routes"));
    }

    Q_PROPERTY(QStringList Searches READ searches)
    inline QStringList searches() const
    {
        return qvariant_cast<QStringList>(property("Searches"));
    }

    Q_PROPERTY(UIntList WinsServers READ winsServers)
    inline UIntList winsServers() const
    {
        return qvariant_cast<UIntList>(property("WinsServers"));
    }

public Q_SLOTS: // METHODS
Q_SIGNALS: // SIGNALS
    void PropertiesChanged(const QVariantMap &properties);
};

#endif
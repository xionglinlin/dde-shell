// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "appitem.h"
#include "appgroupmanager.h"
#include "appitemmodel.h"
#include "appsdockedhelper.h"
#include "appslaunchtimes.h"

#include <tuple>

namespace apps
{
AppItem::AppItem(const QString &appid)
{
    setAppId(appid);

    int groupPos, pagePos, itemPos;
    std::tie(groupPos, pagePos, itemPos) = AppGroupManager::instance()->getAppGroupInfo(appId());
    QVariantList data = {groupPos, pagePos, itemPos};
    setData(data, AppItemModel::GroupRole);

    auto launchedTimes = AppsLaunchTimesHelper::instance()->getLaunchedTimesFor(appId());
    setData(launchedTimes, AppItemModel::LaunchedTimesRole);
}

void AppItem::launch(const QString &action, const QStringList &fields, const QVariantMap &options)
{
    Q_UNUSED(action)
    Q_UNUSED(fields)
    Q_UNUSED(options)

    setLaunchedTimes(launchedTimes() + 1);
}

QString AppItem::appId() const
{
    return data(AppItemModel::DesktopIdRole).toString();
}

void AppItem::setAppId(const QString &appid)
{
    return setData(appid, AppItemModel::DesktopIdRole);
}

QString AppItem::appName() const
{
    return data(AppItemModel::NameRole).toString();
}

void AppItem::setAppName(const QString &appName)
{
    return setData(appName, AppItemModel::NameRole);
}

QString AppItem::appIconName() const
{
    return data(AppItemModel::IconNameRole).toString();
}

void AppItem::setAppIconName(const QString &appIconName)
{
    return setData(appIconName, AppItemModel::IconNameRole);
}

QString AppItem::startupWMClass() const
{
    return data(AppItemModel::StartUpWMClassRole).toString();
}

void AppItem::setStartupWMclass(const QString &wmClass)
{
    return setData(wmClass, AppItemModel::StartUpWMClassRole);
}

bool AppItem::noDisplay() const
{
    return data(AppItemModel::NoDisplayRole).toBool();
}

void AppItem::setNoDisPlay(bool noDisplay)
{
    return setData(noDisplay, AppItemModel::NoDisplayRole);
}

AppItemModel::DDECategories AppItem::ddeCategories() const
{
    return data(AppItemModel::DDECategoryRole).value<AppItemModel::DDECategories>();
}

void AppItem::setDDECategories(const AppItemModel::DDECategories &categories)
{
    return setData(categories, AppItemModel::DDECategoryRole);
}

QString AppItem::actions() const
{
    return data(AppItemModel::ActionsRole).toString();
}

void AppItem::setActions(const QString &actions)
{
    return setData(actions, AppItemModel::ActionsRole);
}

quint64 AppItem::lastLaunchedTime() const
{
    return data(AppItemModel::LastLaunchedTimeRole).toULongLong();
}

void AppItem::setLastLaunchedTime(const quint64 &time)
{
    return setData(time, AppItemModel::LastLaunchedTimeRole);
}

quint64 AppItem::installedTime() const
{
    return data(AppItemModel::InstalledTimeRole).toULongLong();
}

void AppItem::setInstalledTime(const quint64 &time)
{
    return setData(time, AppItemModel::InstalledTimeRole);
}

quint64 AppItem::launchedTimes() const
{
    return data(AppItemModel::LaunchedTimesRole).toULongLong();
}

void AppItem::setLaunchedTimes(const quint64 &times)
{
    AppsLaunchTimesHelper::instance()->setLaunchTimesFor(appId(), times);
    return setData(times, AppItemModel::LaunchedTimesRole);
}

QList<int> AppItem::group() const
{
    return data(AppItemModel::GroupRole).value<QList<int>>();
}

void AppItem::setGroup(const QList<int> &group)
{
    if (group.size() != 3)
        return;
    auto groupPos = group[0];
    auto pagePos = group[1];
    auto itemPos = group[2];
    AppGroupManager::instance()->setAppGroupInfo(appId(), std::make_tuple(groupPos, pagePos, itemPos));
    QVariantList data = {groupPos, pagePos, itemPos};
    return setData(data, AppItemModel::GroupRole);
}

bool AppItem::docked() const
{
    return data(AppItemModel::DockedRole).toBool();
}

void AppItem::setDocked(bool docked)
{
    AppsDockedHelper::instance()->setDocked(appId(), docked);
    return setData(docked, AppItemModel::DockedRole);
}

bool AppItem::onDesktop() const
{
    return data(AppItemModel::OnDesktopRole).toBool();
}

void AppItem::setOnDesktop(bool on)
{
    return setData(on, AppItemModel::OnDesktopRole);
}

bool AppItem::autoStart() const
{
    return data(AppItemModel::AutoStartRole).toBool();
}

void AppItem::setAutoStart(bool start)
{
    return setData(start, AppItemModel::AutoStartRole);
}

}
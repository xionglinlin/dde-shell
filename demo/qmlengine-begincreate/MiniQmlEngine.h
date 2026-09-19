// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QPointer>
#include <QUrl>
#include <QVariantMap>

class QQmlEngine;
class QQmlComponent;
class QQmlContext;

/**
 * @brief 一个刻意精简的 “DQmlEngine 仿制品”
 *
 * 对照 dde-shell 的 frame/qmlengine.h + frame/qmlengine.cpp，
 * 这里只保留 DQmlEngine 的骨架，方便一眼看出 beginCreate()/completeCreate()
 * 在整条链路里处于什么位置：
 *
 *   共享 QQmlEngine  ->  QQmlComponent(异步 loadUrl)  ->  QQmlContext(注入 _ds_applet)
 *        ->  beginCreate()  ->  [发 createFinished，交给外层]  ->  completeCreate()
 *
 * 唯一被“简化”的地方是没有 DObject 私有类（d-pointer）和 DTK 依赖。
 */
class MiniQmlEngine : public QObject
{
    Q_OBJECT
public:
    /**
     * @param url            要加载的 qml（对应 pluginMetaData().url()）
     * @param contextObject  注入到 QML 上下文里的对象，名字为 _ds_applet（对应 DApplet）
     */
    explicit MiniQmlEngine(const QUrl &url, QObject *contextObject = nullptr, QObject *parent = nullptr);
    ~MiniQmlEngine() override;

    /// 共享的、进程内唯一的 QML 运行时（对应 DQmlEnginePrivate::engine()）
    static QQmlEngine *sharedEngine();

    /// 异步加载 qml；组件就绪后【只】调用 beginCreate()，然后发出 createFinished()
    bool create();

    /// 由外部在合适的时机调用，完成第二阶段（对应 DQmlEngine::completeCreate()）
    void completeCreate();

    QObject *rootObject() const { return m_rootObject; }
    bool isCompleted() const { return m_completed; }

    /// 一次性创建、一次性完成（对应 DQmlEngine::createObject()）
    static QObject *createObject(const QUrl &url, const QVariantMap &initialProperties = QVariantMap());

Q_SIGNALS:
    /// 与 DQmlEngine 同名：beginCreate() 完成、对象处于“半成品”状态时发出
    void createFinished();

private:
    void continueLoading();

    QUrl m_url;
    QPointer<QObject> m_contextObject;
    QQmlComponent *m_component = nullptr;
    QQmlContext *m_context = nullptr;
    QObject *m_rootObject = nullptr;
    bool m_begun = false;
    bool m_completed = false;
};

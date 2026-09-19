// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MiniQmlEngine.h"

#include <QDebug>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>

#define MINI_LOG "[MiniQmlEngine]"

QQmlEngine *MiniQmlEngine::sharedEngine()
{
    // 函数内 static：进程级单例。与 DQmlEnginePrivate::engine() 的思路一致——
    // 多个插件界面共用一个 QQmlEngine，只保留一份类型注册信息与 import 缓存。
    static QQmlEngine *s_engine = nullptr;
    if (!s_engine) {
        s_engine = new QQmlEngine();
        qInfo().noquote() << MINI_LOG << "创建唯一的 QQmlEngine（所有 MiniQmlEngine 实例共享）";
    }
    return s_engine;
}

MiniQmlEngine::MiniQmlEngine(const QUrl &url, QObject *contextObject, QObject *parent)
    : QObject(parent)
    , m_url(url)
    , m_contextObject(contextObject)
{
}

MiniQmlEngine::~MiniQmlEngine() = default;

bool MiniQmlEngine::create()
{
    QQmlEngine *engine = sharedEngine();

    // 1) 蓝图：加载 qml 文件。用 Asynchronous，避免大批量插件时阻塞主线程。
    m_component = new QQmlComponent(engine, this);
    m_component->loadUrl(m_url, QQmlComponent::Asynchronous);

    // 2) 作用域：每个实例一个独立的 QQmlContext，并注入 C++ 对象。
    m_context = new QQmlContext(engine, this);
    if (m_contextObject) {
        m_context->setContextProperty(QStringLiteral("_ds_applet"), m_contextObject);
    }

    qInfo().noquote() << MINI_LOG << "开始异步加载" << m_url;

    if (m_component->isLoading()) {
        connect(m_component, &QQmlComponent::statusChanged, this, [this]() {
            continueLoading();
        });
    } else {
        // 极少数情况（比如本地已缓存）会同步就绪
        continueLoading();
    }
    return true;
}

void MiniQmlEngine::continueLoading()
{
    if (!m_component || m_begun)
        return;

    if (m_component->isReady()) {
        m_begun = true;
        // 关键：这里只 beginCreate()，【绝不】在这里 completeCreate()。
        // 对应的就是 DQmlEnginePrivate::continueLoading()。
        qInfo().noquote() << MINI_LOG << "组件就绪 -> beginCreate()（对象此时是“半成品”）";
        m_rootObject = m_component->beginCreate(m_context);
        Q_EMIT createFinished();
    } else if (m_component->isError()) {
        m_begun = true;
        qWarning().noquote() << MINI_LOG << "加载失败:" << m_component->errorString();
        Q_EMIT createFinished();  // 失败也发信号，此时 rootObject 为 nullptr
    }
}

void MiniQmlEngine::completeCreate()
{
    if (!m_component || m_completed)
        return;

    if (!m_component->isReady()) {
        qWarning().noquote() << MINI_LOG << "completeCreate() 被忽略：组件尚未就绪";
        return;
    }

    // 关键：第二阶段。到这里才会：求值绑定、连接 QML 里的信号处理器、执行 Component.onCompleted。
    // 对应的就是 DQmlEngine::completeCreate()。
    qInfo().noquote() << MINI_LOG << "completeCreate() -> 绑定求值 / 信号连接 / onCompleted 执行";
    m_component->completeCreate();
    m_completed = true;
}

QObject *MiniQmlEngine::createObject(const QUrl &url, const QVariantMap &initialProperties)
{
    QQmlEngine *engine = sharedEngine();

    QQmlComponent component(engine);
    component.loadUrl(url);
    if (component.isError()) {
        qWarning().noquote() << MINI_LOG << "加载失败:" << component.errorString();
        return nullptr;
    }

    // 注意：createWithInitialProperties() 内部等价于
    //   beginCreate() -> 设置初始属性 -> completeCreate()
    // context 故意不释放：它必须比它作用域里创建出来的对象活得久。
    auto *context = new QQmlContext(engine, engine->rootContext());
    QObject *object = component.createWithInitialProperties(initialProperties, context);
    if (!object) {
        delete context;
        return nullptr;
    }
    return object;
}

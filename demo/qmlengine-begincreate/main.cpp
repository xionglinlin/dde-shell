// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// =============================================================================
// 这个 demo 用 4 个场景说明 QQmlEngine / QQmlComponent / QQmlContext 的关系，
// 重点是：
//   * QQmlComponent::beginCreate() 与 completeCreate() 到底解决什么问题
//   * 什么情况下直接 component.create() 就够了
//   * 什么情况下必须拆成两阶段
//
// 对应 dde-shell 的实现：
//   frame/qmlengine.cpp 的 DQmlEngine、DQmlEnginePrivate::continueLoading()
//   shell/appletloader.cpp 的 DAppletLoaderPrivate::doCreateRootObject()
//
// 建议配合 README.md 一起阅读。
// =============================================================================

#include "MiniQmlEngine.h"

#include <QCoreApplication>
#include <QDebug>
#include <QEventLoop>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QTimer>
#include <QVariantMap>

#include <utility>

static const QUrl kProbeUrl(QStringLiteral("qrc:/qml/Probe.qml"));
static const QUrl kPanelUrl(QStringLiteral("qrc:/qml/PanelWindow.qml"));

static void banner(const QString &title)
{
    qInfo().noquote() << "\n\n==========" << title << "==========";
}

// 模拟 dde-shell 里被注入到 QML 上下文的 C++ 对象。
// 对应 DApplet，在 DQmlEngine::create() 中通过 setContextProperty("_ds_applet", applet) 注入。
class FakeApplet : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString displayName READ displayName CONSTANT)
public:
    explicit FakeApplet(QString name, QObject *parent = nullptr)
        : QObject(parent)
        , m_name(std::move(name))
    {
    }
    QString displayName() const { return m_name; }

private:
    QString m_name;
};

// QML 里用 `signal xxx(...)` 声明的信号，C++ 只能用字符串形式的 connect 去接，
// 所以准备一个带槽的接收者。
class SignalCatcher : public QObject
{
    Q_OBJECT
public:
    explicit SignalCatcher(QString tag, QObject *parent = nullptr)
        : QObject(parent)
        , m_tag(std::move(tag))
    {
    }

public Q_SLOTS:
    void onProbeReady(const QString &from)
    {
        qInfo().noquote() << m_tag << "收到 QML 信号 probeReady，来自" << from;
    }

private:
    QString m_tag;
};

// -----------------------------------------------------------------------------
// 场景 1：直接 create()
//
// 适用条件：QML 自给自足，C++ 不需要在它“完成初始化之前”做任何事。
// create() 语义上 == beginCreate() + completeCreate()，一步到位，也就没有干预窗口。
// -----------------------------------------------------------------------------
static void scenario1_directCreate()
{
    banner("场景 1：component.create() —— 一步到位，不给干预机会");

    QQmlEngine engine;
    QQmlComponent component(&engine, kProbeUrl);
    if (component.isError()) {
        qWarning().noquote() << "[main] 组件加载失败:" << component.errorString();
        return;
    }

    qInfo().noquote() << "[main] 调用 create() 之前……";
    QObject *object = component.create();  // 等价于 beginCreate() + completeCreate()
    qInfo().noquote() << "[main] create() 返回，object =" << object;

    if (object) {
        // create() 出来且没有 parent 的对象归 JS 引擎所有（JavaScriptOwnership）。
        // 交给 C++ 管理生命周期时，要显式改成 CppOwnership（或给它设 parent）。
        QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);
        delete object;
    }

    qInfo().noquote() << "[main] 注意日志顺序：Component.onCompleted 是在 create() 内部执行的，"
                         "C++ 完全插不进去。";
}

// -----------------------------------------------------------------------------
// 场景 2：beginCreate() + 干预 + completeCreate()
//
// 这是 begin/complete 存在的全部意义：在“对象已存在但还没完成”的窗口期里
//   ① 写入初始属性（在绑定首次求值 / onCompleted 之前生效）
//   ② 把信号先接好（避免 QML 在完成瞬间发的信号被漏掉）
// -----------------------------------------------------------------------------
static void scenario2_beginComplete()
{
    banner("场景 2：beginCreate() + 干预 + completeCreate() —— 两阶段创建");

    QQmlEngine engine;
    QQmlComponent component(&engine, kProbeUrl);
    if (component.isError()) {
        qWarning().noquote() << "[main] 组件加载失败:" << component.errorString();
        return;
    }

    // context 故意不在这里析构：它必须比“它作用域里创建出来的对象”活得久。
    auto *context = new QQmlContext(&engine);
    context->setContextProperty(QStringLiteral("_ds_applet"),
                                new FakeApplet(QStringLiteral("C++ 注入的对象"), context));

    SignalCatcher catcher(QStringLiteral("[main]"));

    qInfo().noquote() << "[main] ① 调用 beginCreate() 之前";
    QObject *object = component.beginCreate(context);
    qInfo().noquote() << "[main] ② beginCreate() 返回，object =" << object
                      << " —— 对象已存在，但还没“完成”";

    // ==== begin 窗口期开始 ====
    // ①  写入“初始属性”
    object->setProperty("value", 10);
    qInfo().noquote() << "[main] ③ begin 窗口里设置 value = 10（初始属性）";

    // ②  在 QML 开始发信号之前先把信号接好
    QObject::connect(object, SIGNAL(probeReady(QString)), &catcher, SLOT(onProbeReady(QString)));
    qInfo().noquote() << "[main] ④ begin 窗口里 connect(object, probeReady, ...)";
    // ==== begin 窗口期结束 ====

    qInfo().noquote() << "[main] ⑤ 即将 completeCreate()……";
    component.completeCreate();
    qInfo().noquote() << "[main] ⑥ completeCreate() 返回：绑定已求值、onCompleted 已执行、"
                         "probeReady 信号也已收到";

    QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);
    delete object;
}

// -----------------------------------------------------------------------------
// 场景 3：createWithInitialProperties()
//
// 只是“需要初始属性”这一个诉求时，Qt 提供了现成的便捷函数；
// 它内部实现就是 beginCreate() -> 设置初始属性 -> completeCreate()。
// 也就是说：能覆盖场景 2 的 ①，但覆盖不了 ②（提前连信号），更做不到“暂停”。
// -----------------------------------------------------------------------------
static void scenario3_createWithInitialProperties()
{
    banner("场景 3：createWithInitialProperties() —— 场景 2 的便捷写法");

    QQmlEngine engine;
    QQmlComponent component(&engine, kProbeUrl);
    if (component.isError()) {
        qWarning().noquote() << "[main] 组件加载失败:" << component.errorString();
        return;
    }

    auto *context = new QQmlContext(&engine);

    QVariantMap initialProperties;
    initialProperties.insert(QStringLiteral("value"), 99);

    qInfo().noquote() << "[main] 调用 createWithInitialProperties({value: 99}, context)……";
    QObject *object = component.createWithInitialProperties(initialProperties, context);
    qInfo().noquote() << "[main] 返回，object =" << object;

    if (object) {
        qInfo().noquote() << "[main] 它内部 = beginCreate() -> 设置初始属性 -> completeCreate()；"
                             "所以 onCompleted 里看到的 value 已经是 99。";
        QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);
        delete object;
    }
}

// -----------------------------------------------------------------------------
// 场景 4：仿 DQmlEngine —— 异步加载 + begin/complete 跨事件循环分离
//
// 对应 dde-shell：
//   DQmlEnginePrivate::continueLoading() 里 beginCreate() 后发 createFinished
//   DAppletLoaderPrivate::doCreateRootObject() 回调里 setRootObject() + completeCreate()
//
// 这个场景里我们故意把 completeCreate() 推迟到下一个事件循环，
// 模拟“等外部条件就绪后再收尾”，这是 create() 一步到位做不到的。
// -----------------------------------------------------------------------------
static void scenario4_miniEngine()
{
    banner("场景 4：仿 DQmlEngine（异步 + begin/complete 跨事件循环 + 注入 C++ 对象）");

    FakeApplet applet(QStringLiteral("FakeApplet-from-C++"));
    SignalCatcher catcher(QStringLiteral("[main]"));

    MiniQmlEngine miniEngine(kPanelUrl, &applet);

    QObject::connect(&miniEngine, &MiniQmlEngine::createFinished, &miniEngine, [&]() {
        QObject *root = miniEngine.rootObject();
        qInfo().noquote() << "[main] createFinished -> beginCreate() 出来的 rootObject =" << root;
        if (!root) {
            qWarning().noquote() << "[main] 根对象创建失败";
            QTimer::singleShot(0, qApp, []() { QCoreApplication::quit(); });
            return;
        }

        // 在 begin 窗口里把信号接好（QML 要等 completeCreate() 才会发这些信号）
        QObject::connect(root, SIGNAL(panelReady(QString)), &catcher, SLOT(onProbeReady(QString)));
        qInfo().noquote() << "[main] begin 窗口里 connect(root, panelReady, ...)";

        // 在 begin 窗口里也可以处理所有权：把 rootObject 的生命周期挂到持有者身上
        // （对应 dde-shell 里 DApplet 持有 rootObject、由容器负责销毁）。
        root->setParent(&miniEngine);

        // 刻意【不】在当前调用栈里 completeCreate()，
        // 而是等下一个事件循环，模拟“外部异步资源就绪后再完成创建”。
        // 注意：root 必须按值捕获，否则这里拿到的是已经失效的栈变量。
        QTimer::singleShot(0, &miniEngine, [&, root]() {
            qInfo().noquote() << "[main] 外部条件就绪 -> 现在才调用 completeCreate()";

            // 关键：show() 必须放在 completeCreate() 之后，
            // 否则窗口会带着“未完成”的绑定/信号处理器被显示出来。
            miniEngine.completeCreate();

            if (auto *window = qobject_cast<QQuickWindow *>(root)) {
                window->show();
                qInfo().noquote() << "[main] 窗口已显示（visible = true）";
            }

            QTimer::singleShot(600, qApp, []() { QCoreApplication::quit(); });
        });
    });

    miniEngine.create();

    // 异步加载依赖事件循环，所以这里要跑一个 loop 等它跑完。
    QEventLoop loop;
    QObject::connect(qApp, &QCoreApplication::aboutToQuit, &loop, &QEventLoop::quit);
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);  // 兜底，避免卡死
    loop.exec();
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    qInfo().noquote() << "\n* QQmlEngine   = 运行时（类型注册 / import 路径 / 对象工厂）"
                      << "\n* QQmlComponent= 某个 qml 文件加载后的“蓝图”，负责造对象"
                      << "\n* QQmlContext  = 作用域，决定 QML 里的名字去哪里找"
                      << "\n* create()     = beginCreate() + completeCreate()";

    scenario1_directCreate();
    scenario2_beginComplete();
    scenario3_createWithInitialProperties();
    scenario4_miniEngine();

    qInfo().noquote() << "\n全部场景结束。";
    return 0;
}

#include "main.moc"

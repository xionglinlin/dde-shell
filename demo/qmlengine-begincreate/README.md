# QML 创建流程 demo：`beginCreate()` / `completeCreate()`

这个 demo 用来回答两个问题：

1. `QQmlEngine`、`QQmlComponent`、`QQmlContext` 之间是什么关系？
2. `beginCreate()` / `completeCreate()` 到底干什么用，什么时候直接 `create()` 就够了？

它刻意模仿 dde-shell 里的 [`DQmlEngine`](../../frame/qmlengine.cpp)，把插件加载时必须拆成两阶段的原因拆开演示。

---

## 一、三个类的关系

| 类 | 角色 | 类比 |
|---|---|---|
| `QQmlEngine` | 运行时：类型注册表、import 路径、JS 虚拟机、对象创建机制 | JVM / 工厂 |
| `QQmlComponent` | 一个 qml 文件被加载后的“蓝图”，可以反复造实例 | class / 图纸 |
| `QQmlContext` | 作用域：QML 里的名字去哪里找（`setContextProperty` 注入 C++ 对象） | 符号表 |

```
QQmlEngine ──(创建)──> QQmlComponent ──create(context)──> QObject* 实例
     │                                                       ▲
     └──(派生)──> QQmlContext ──(提供符号查找)────────────────┘
```

- `QQmlComponent` 必须依附一个 `QQmlEngine`。
- `create()` 时如果没有显式传 `QQmlContext`，就用 `engine->rootContext()`。
- 造出来的对象依赖它的 context 解析绑定，所以 **context 必须比对象活得久**。

### 那 `QQmlApplicationEngine` 是什么？

它是这种情况的便利封装：自己就是一个 `QQmlEngine`，`load(url)` 内部干的事约等于

```cpp
QQmlComponent component(this, url);          // 蓝图
QObject *object = component.create(rootContext());  // 造实例
if (root 是 QQuickWindow) object->setVisible(true); // 自动显示
```

语义锁定为“一个应用一个主窗口”。而 dde-shell 一个进程要装几十个插件界面，所以必须回到上面那套原生写法自己控制——这就是 `DQmlEngine` 存在的原因。

---

## 二、`create()` 与 `beginCreate()` / `completeCreate()` 的关系

```
create()  ≈  beginCreate()  +  completeCreate()
```

| 阶段 | 已经完成的事 | 还没发生的事 |
|---|---|---|
| `beginCreate()` 之后 | 对象已存在，属性可读可写（`setProperty`） | 绑定未求值、QML 信号处理器未连接、`Component.onCompleted` 未执行 |
| `completeCreate()` 之后 | 绑定已求值、信号已连、`onCompleted` 已跑 | —— |

所以 **`beginCreate()` 与 `completeCreate()` 之间的窗口期**可以干三件事：

1. **写入“初始属性”**：这个值会在绑定首次求值、`onCompleted` 之前生效，QML 一上来就看见它。`createWithInitialProperties()` 内部就是这么实现的。
2. **提前连接信号**：QML 在完成瞬间（`onCompleted` 里）发出的信号不会漏掉。
3. **暂停**：可以把 `completeCreate()` 推迟到任何时刻——下一个事件循环、异步资源就绪之后、甚至被用户操作触发。这一点 `create()` 做不到。

对应到 dde-shell：

| dde-shell 代码 | 作用 |
|---|---|
| [`DQmlEnginePrivate::continueLoading()`](../../frame/qmlengine.cpp:55) | 组件就绪后 `beginCreate()`，随即发 `createFinished` |
| [`DQmlEngine::completeCreate()`](../../frame/qmlengine.cpp:87) | 暴露给外部的第二阶段 |
| [`DAppletLoaderPrivate::doCreateRootObject()`](../../shell/appletloader.cpp:175) | 在 `createFinished` 回调里 `setRootObject()` 后再 `completeCreate()` |

---

## 三、什么时候可以直接 `create()`

**满足以下全部条件就直接用 `create()`：**

- [ ] QML 自给自足，不需要在它初始化完成前注入值；
- [ ] 不需要在 `onCompleted` 之前连接信号；
- [ ] 不需要延迟/暂停创建过程；
- [ ] 创建是同步完成的，没有“等另一个异步资源”的需求；
- [ ] 不需要在对象可用之前做所有权/父子关系的干预。

**只要命中下面任意一条，就该用 `beginCreate()` / `completeCreate()`：**

- [ ] 需要 initial properties（且必须早于绑定首次求值）
- [ ] 需要在 QML 发信号前 hook 上去
- [ ] 需要在对象显示/使用前设置 `parent`、完成所有权转移
- [ ] 创建过程需要暂停（异步依赖、等待外部条件）
- [ ] 需要保证对象不会被别人拿到“半成品”状态

**如果只命中“需要 initial properties”这一条**，优先用 `createWithInitialProperties()`——它内部就是 `beginCreate() → set → completeCreate()`，更省事。只有还需要“信号 hook”或“暂停”时才手动拆成两阶段。

### 常见坑

- `beginCreate()` 之后**必须**配对调用 `completeCreate()`，包括出错路径；否则对象停在半成品状态（信号不连、`onCompleted` 不跑）。
- `beginCreate()` 与 `completeCreate()` 之间不要 `show()` 窗口/暴露给他人使用。
- `create()` 出来且**没有 parent** 的对象归 JS 引擎所有（`JavaScriptOwnership`）；想让 C++ 管就 `setObjectOwnership(obj, QQmlEngine::CppOwnership)` 或给它设 parent。
- 每个 `QQmlContext` 都要比它作用域里创建出来的对象活得久（demo 里 `context` 故意不析构）。

---

## 四、demo 的 4 个场景

| 场景 | 内容 | 看点 |
|---|---|---|
| 1 | `component.create()` | 一条龙，`onCompleted` 在 `create()` 内部就跑完了，C++ 插不进去 |
| 2 | `beginCreate()` + `setProperty` + `connect` + `completeCreate()` | 观察 `onCompleted` 在 `completeCreate()` 时才执行；初始属性 `value=10` 被 QML 看到 |
| 3 | `createWithInitialProperties()` | 场景 2 的便捷写法，内部实现相同 |
| 4 | 仿 `DQmlEngine`：共享 engine + 独立 context + 注入 `_ds_applet` + 异步加载 + 延迟 `completeCreate()` | `begin`/`complete` 跨事件循环分离；`show()` 放在 `completeCreate()` 之后 |

### 目录结构

```
demo/qmlengine-begincreate/
├── CMakeLists.txt          # 独立工程，不参与 dde-shell 主构建
├── main.cpp                # 4 个场景
├── MiniQmlEngine.h/.cpp    # 精简版 DQmlEngine
├── demo.qrc
├── qml/
│   ├── Probe.qml           # QtObject，用来观察创建过程的“探针”
│   └── PanelWindow.qml     # Window，模拟插件 main.qml
└── README.md
```

---

## 五、构建与运行

```bash
cd demo/qmlengine-begincreate
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./build/qmlengine_begincreate_demo
```

无显示环境（CI、SSH）时：

```bash
QT_QPA_PLATFORM=offscreen ./build/qmlengine_begincreate_demo
```

需要 Qt 5.15+ 或 Qt 6（含 `Qt::Qml`、`Qt::Quick` 模块）。

### 期望输出解读（关键片段）

场景 1 —— 没有干预窗口：

```
[main] 调用 create() 之前……
[QML Probe] 绑定 doubled 被求值 -> 2
[QML Probe] Component.onCompleted: value = 1 , doubled = 2
[main] create() 返回，object = QObject(0x...)
```

`onCompleted` 出现在 `create()` **内部**，`value` 是 QML 里的默认值 `1`。

场景 2 —— 两阶段创建，初始属性与信号 hook 生效：

```
[main] ② beginCreate() 返回，object = QObject(0x...) —— 对象已存在，但还没“完成”
[main] ③ begin 窗口里设置 value = 10（初始属性）
[main] ④ begin 窗口里 connect(object, probeReady, ...)
[main] ⑤ 即将 completeCreate()……
[QML Probe] 绑定 doubled 被求值 -> 20
[QML Probe] Component.onCompleted: value = 10 , doubled = 20
[main] 收到 QML 信号 probeReady，来自 Probe.qml
[main] ⑥ completeCreate() 返回……
```

要点：`onCompleted` 与绑定求值都发生在 `completeCreate()` 时，而且它们看到的 `value` 已经是 `10`；信号也是先在 begin 窗口里接好才收到的。

场景 4 —— 仿 `DQmlEngine` 的异步链路：

```
[MiniQmlEngine] 组件就绪 -> beginCreate()（对象此时是“半成品”）
[main] createFinished -> beginCreate() 出来的 rootObject = QQuickWindow(0x...)
[main] begin 窗口里 connect(root, panelReady, ...)
[main] 外部条件就绪 -> 现在才调用 completeCreate()
[MiniQmlEngine] completeCreate() -> 绑定求值 / 信号连接 / onCompleted 执行
[QML PanelWindow] Component.onCompleted（此时窗口才算“完成”）
[main] 收到 QML 信号 probeReady，来自 PanelWindow.qml
[main] 窗口已显示（visible = true）
```

这就是 [`shell/appletloader.cpp`](../../shell/appletloader.cpp:181) 里 `createFinished → setRootObject → completeCreate` 那条链路的完整等价形式。

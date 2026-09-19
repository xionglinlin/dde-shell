// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 一个“没有界面”的 QML 对象，专门用来观察 QQmlComponent 的创建过程。
// 留意三个观察点：
//   1. value 是普通属性，C++ 可以在 beginCreate()/completeCreate() 之间写入（初始属性）
//   2. doubled 是绑定属性，用来观察绑定什么时候被求值
//   3. Component.onCompleted 与 probeReady 信号，用来观察“对象何时算完成”
import QtQml

QtObject {
    id: probe

    property int value: 1
    property int doubled: value * 2

    // 声明一个信号，方便 C++ 在 begin 窗口里提前 connect
    signal probeReady(string from)

    onDoubledChanged: console.log("[QML Probe] 绑定 doubled 被求值 ->", doubled)

    Component.onCompleted: {
        console.log("[QML Probe] Component.onCompleted: value =", value, ", doubled =", doubled)
        probeReady("Probe.qml")
    }
}

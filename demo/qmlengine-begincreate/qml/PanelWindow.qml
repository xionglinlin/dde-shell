// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 对应 dde-shell 里插件的 main.qml（根对象是个 Window）。
// 用来演示：窗口只有在 completeCreate() 之后才适合 show()。
import QtQuick
import QtQuick.Window

Window {
    id: root
    width: 480
    height: 220
    visible: false                 // 关键：等 C++ 在 completeCreate() 之后再 show()

    signal panelReady(string from)

    color: "#1e1e2e"

    Column {
        anchors.centerIn: parent
        spacing: 14

        Text {
            color: "#cdd6f4"
            font.pixelSize: 16
            text: "我是由 beginCreate()/completeCreate() 创建出来的 Window"
        }

        // 直接使用 C++ 注入的 context property "_ds_applet"（对应 dde-shell 的 DApplet）
        Text {
            color: "#a6e3a1"
            font.pixelSize: 14
            text: "_ds_applet.displayName = " + _ds_applet.displayName
        }

        Text {
            color: "#f9e2af"
            font.pixelSize: 13
            text: "C++ 在 begin 窗口里接好了信号，completeCreate() 之后才调用 show()"
        }
    }

    Component.onCompleted: {
        console.log("[QML PanelWindow] Component.onCompleted（此时窗口才算“完成”）")
        panelReady("PanelWindow.qml")
    }
}

# Touch UI V5.1 架构

## 目标

把星语伴侣做成一个可长期维护的 UI Overlay，而不是把 LCD/Touch 底层改乱。

## 分层

```text
xiaozhi upstream
├─ main/boards/waveshare/esp32-s3-touch-lcd-3.5/
│  └─ 保持官方 LCD / Touch / Audio / PMIC 初始化
│
├─ main/display/lcd_display.*
│  └─ 保持小智通用显示框架
│
└─ main/display/stellar/
   ├─ stellar_display.h
   ├─ stellar_display.cc
   ├─ stellar_avatar.h
   └─ stellar_avatar.c
```

GitHub Actions 构建时由 `scripts/apply_mods.py` 自动把 `overlay/` 复制到 upstream，然后仅做三类补丁：

1. 将 STELLAR 源码加入 `main/CMakeLists.txt`；
2. 让本板从 `SpiLcdDisplay` 实例化为 `StellarDisplay`；
3. 将本板 `CONFIG_USE_WECHAT_MESSAGE_STYLE` 设为 `n`。

## V4 UI 资源

`stellar_avatar.c` 是 200×320 RGB565 图片，来源于 V4 已确认的浅色整体设计。首页把它直接放在屏幕中央 x=140，因此图片背景和左右浅色卡片属于同一个视觉体系。

## 为什么仍先调用 LcdDisplay::SetupUI()

小智框架的主题、字体替换、通知、状态和电源相关逻辑仍可能依赖原生对象存在。V4 先创建原生 UI，再把不透明的 STELLAR 首页/对话层放到最上方，减少对 upstream 的侵入。

## 不修改范围

- ST7796 初始化
- LCD SPI 配置
- FT6336/FT5x06-compatible Touch 初始化
- I2C
- AXP2101
- ES8311
- Wi-Fi / 网络协议栈

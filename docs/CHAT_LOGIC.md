# Touch UI V5.1 对话逻辑

## 页面切换

```text
首页
 -> 点击中央人物 / AI 卡片 / 底部提示
 -> ShowChat()
 -> Application::ToggleChatState()
```

BOOT 键继续沿用小智原有行为。

## 消息显示

V4 禁用本板的微信 Bubble 编译配置，不把用户看到的对话交给 `LcdDisplay::SetChatMessage()`。

- `user`：显示在对话卡片顶部的小预览行，并重置为普通对话布局。
- `assistant`：显示在 STELLAR 自定义正文区域。
- `system`：只更新顶部状态，不生成大气泡。

## 长回答

1. 正文从 y=0 顶部开始。
2. 如果正文高度不超过可视区，不滚动。
3. 如果超出可视区，先扩大正文卡片，并把人物缩到右侧。
4. 等待 `2000ms`。
5. 以约 `120ms / 像素` 从顶部向底部滚动。
6. 动画不 repeat，滚到底停止。
7. 新一轮用户消息到来时才重置布局。

## 回首页

AI 从 `kDeviceStateSpeaking` 离开后，保留最后回答约 4.5 秒，再自动回首页。

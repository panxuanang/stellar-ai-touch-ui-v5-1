# V4 验证清单

## 构建前自动检查

`scripts/apply_mods.py` 会验证：

- STELLAR 四个源码文件已复制；
- 当前板实例化 `StellarDisplay`；
- `CONFIG_USE_WECHAT_MESSAGE_STYLE=n`；
- 不同时残留 `=y`；
- V4 首页关键中文文案存在；
- 200×320 人物/场景 RGB565 资源存在；
- `2000ms` + `120ms/像素` 长回答逻辑存在；
- 自定义 UI 没有固定运行时 `lv_font_t*`；
- 自定义对话没有再调用原生 `LcdDisplay::SetChatMessage()`。

## GitHub Actions 成功标准

构建产物中必须存在：

```text
merged-binary.bin
STELLAR_AI_Touch_UI_V4_merged-binary.bin
SHA256SUMS.txt
BUILD_INFO.txt
```

## 实机验证

- 联网后不自动重启；
- 首页整体是统一浅色，中央没有深色矩形；
- 点击人物能进入 AI；
- 不出现微信 Bubble 页；
- 短回答从顶部显示；
- 长回答扩大卡片、人物缩小；
- 2 秒后慢速单向滚动；
- 滚到底不循环；
- 回答结束约 4.5 秒回首页。

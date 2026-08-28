# 星语伴侣 / STELLAR AI — Touch UI V5.1

目标开发板：**Waveshare ESP32-S3-Touch-LCD-3.5**  
显示方向：**480×320 横屏**  
上游基础：**78/xiaozhi-esp32**

## 这一版是什么

Touch UI V5.1 是在已可运行的 V4 基础上，继续围绕你确认的最终审美方向做的产品化界面版本：

- 首页采用浅色统一背景
- 中央动漫人物与背景融为一体
- 左右四个功能区继续保留，但边框弱化，视觉上更柔和
- 不再追求“每块都很重的独立卡片”，而是更像同一张完整桌面界面
- 对话页保持你认可的第二张预览图风格
- 长回答逻辑保持 RLCD 项目喜欢的那套：顶部开始、慢速单向滚动、不循环

## V5 页面内容

### 首页
- 日期
- 大时钟
- AI 助手状态
- 点击开始对话
- 天气
- 今日待办
- 中央动漫角色主视觉

### AI 对话页
- 顶部状态栏：正在与星语对话 / 计时 / 返回首页
- 左侧大对话区
- 右侧人物图
- 短回答正常显示
- 长回答自动扩展对话区
- 内容超出时 2000ms 后按约 120ms/像素缓慢向下滚动，滚到底停止

## 工程结构

```text
.github/workflows/build-stellar-v5-1.yml
overlay/main/display/stellar/
scripts/
preview/
docs/
README.md
CHANGELOG.md
VERSION
```

## 编译方式

继续使用 GitHub Actions 在线编译：

1. 上传整个工程到 GitHub 仓库
2. 运行 `Build STELLAR AI Touch UI V5.1`
3. workflow 会自动：
   - 拉取 `78/xiaozhi-esp32`
   - 自动执行 `cp secret_config.h.example secret_config.h`
   - 应用 overlay 与补丁
   - 编译 `Waveshare ESP32-S3-Touch-LCD-3.5`
   - 生成可直接烧录的 `merged-binary.bin`

## 说明

本版仍然遵循你的原则：

- **优先改 UI 层，不乱动底层驱动**
- UI 逻辑与板级底座分离
- 便于以后继续做 V6、V7 时维护

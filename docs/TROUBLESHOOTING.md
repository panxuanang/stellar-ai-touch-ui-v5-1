# Touch UI V5.1 排错

## Actions 失败

请展开失败步骤，优先找第一处真正的：

```text
error:
fatal error:
assert
Traceback
```

不要只发最后一行 `Process completed with exit code 1`。

## 编译成功但还是微信聊天页

检查 Actions 的 `Verify V4 patch` 是否通过，并在源码补丁后的 `config.json` 中确认：

```text
CONFIG_USE_WECHAT_MESSAGE_STYLE=n
```

同时 board 源码必须有：

```text
new StellarDisplay(
```

## 首页仍有深色人物背景

V4 正常资源应为：

```text
preview/stellar_avatar_200x320.png
```

并且 `stellar_avatar.c` 里描述符应为 200×320。若烧录后仍是旧星空图，通常是烧了 V3/V2 的旧 BIN。

## 联网后重启

V4 没有重新修改 V2 已稳定的底层。若再次出现重启，请提供串口重启前最后几十行，尤其是：

```text
Guru Meditation Error
LoadProhibited
StoreProhibited
assert failed
Backtrace:
rst:
```

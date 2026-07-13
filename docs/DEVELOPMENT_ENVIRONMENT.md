# 开发环境

## 1. 已确认环境

开发机为 Apple Silicon Mac，已检测到：

| 组件 | 当前状态 |
| --- | --- |
| 宿主架构 | macOS ARM64 |
| Android Studio | 2026.1，build `AI-261.23567.138.2611.15503007` |
| Android Studio JBR | OpenJDK 21.0.10 |
| 命令行 Java | OpenJDK 21.0.11 |
| Android SDK | `~/Library/Android/sdk` |
| Platform Tools | 37.0.0 |
| Emulator | 36.6.11 |
| SDK Platform | Android 36、36.1 |
| Build Tools | 35.0.0、36.0.0、36.1.0、37.0.0 |
| 本机 Gradle | `/opt/homebrew/bin/gradle` |

项目构建应使用仓库内 Gradle Wrapper，而不是依赖本机 Gradle 版本。

## 2. Pixel 7 AVD

现有 AVD：

```text
name: Pixel_7
device: Pixel 7
system image: Android 33 Google APIs Play Store
ABI: arm64-v8a
resolution: 1080 x 2400
density: 420 dpi
RAM: 2048 MB
data partition: 10 GB
```

该 AVD 与首期 ARM64 APK 目标一致，适合：

- 快速安装和启动回归。
- JNI/FluffOS ARM64 加载测试。
- WebView 和本地回环 TCP 联调。
- API 33 前台服务、通知和进程生命周期测试。
- 故障注入、清除数据、升级安装和自动化冒烟测试。

它不能替代真机的熄屏保活、OEM 省电策略、长期耗电和真实低内存测试。

## 3. 当前缺失组件

SDK 中尚未检测到：

- Android NDK。
- SDK CMake。

在开始 native 原型前，应通过 Android Studio SDK Manager 安装并固定一个 NDK 与 CMake
版本。不要使用“latest”作为项目配置；安装完成后把实际版本写入：

```text
android.ndkVersion
externalNativeBuild.cmake.version
```

首选策略是先用 Android Studio 2026.1 推荐的稳定 NDK 构建。如果旧 FluffOS 因编译器
变化无法通过，再以最小补丁适配当前 NDK，而不是立即降级整个 Android 工具链。

## 4. 建议的首轮调试顺序

1. 安装并固定 NDK、CMake。
2. 创建最小 Android 工程和 Gradle Wrapper。
3. 编译一个 ARM64 JNI smoke library，在 `Pixel_7` 上验证加载。
4. 将精确 FluffOS 提交接入 native 构建。
5. 先用最小 mudlib/config 验证驱动入口，再接入完整 zjmud payload。
6. 用 `adb logcat`、应用日志和 FluffOS `debug.log` 联合定位启动问题。
7. 服务端握手通过后再加载 WebView。

## 5. 调试约定

- AVD 启动和 APK 安装由 Gradle/ADB 命令记录，保证可重复。
- Native Debug 使用 Android Studio LLDB；Release 崩溃使用保留的 symbols 符号化。
- `adb forward` 仅可用于开发诊断，不能进入正式应用运行链路。
- 调试构建可以开启 WebView debugging；Release 必须关闭。
- 自动化测试不得依赖 Play Store 登录状态或外部网络。
- 在 AVD 中至少执行一次完全断网的首次安装和游戏流程。

## 6. 环境注意事项

`sdkmanager --list_installed` 当前报告 SDK XML 版本兼容警告，说明命令行工具与 SDK
元数据版本存在轻微差异。它不影响已安装 AVD 运行，但安装 NDK/CMake 前应优先通过
Android Studio SDK Manager 操作，或更新 command-line tools 后再使用命令行安装。

当前没有正在运行或连接的 Android 设备。正式开发开始时应先启动 `Pixel_7`，再通过：

```text
adb devices -l
adb shell getprop ro.product.cpu.abi
adb shell getprop ro.build.version.sdk
```

确认实际 ABI 和 API 与 AVD 配置一致。

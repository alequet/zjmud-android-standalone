# 构建与打包

## 1. 预期仓库结构

```text
app/
  src/main/
    java/...                 Android UI、服务和传输层
    aidl/...                 跨进程控制接口
    assets/web/              清理后的原 Web 页面
    assets/runtime/          版本化 mudlib payload
    cpp/                     JNI 适配器
native/
  fluffos/                   固定提交或可验证补丁集
  third_party/               固定版本的 native 依赖构建描述
tools/
  import_zjmud/              从唯一源 ZIP 生成 payload
  verify_payload/            清单和编码验证
docs/
gradle/
```

不要直接把 `~/zjmud-main.zip` 解压后全部提交。导入工具必须产生可审计、可重复的结果。

本机 Android Studio、SDK 和 Pixel 7 AVD 的已确认版本及缺失前置项见
`DEVELOPMENT_ENVIRONMENT.md`。

当前工程固定 AGP `8.13.2`、Gradle `8.13`、Kotlin `2.2.21`、NDK
`29.0.14206865` 和 CMake `3.31.6`，仅生成 `arm64-v8a`。

## 2. zjmud 资源导入

输入仅允许：

```text
~/zjmud-main.zip
```

导入流程：

1. 校验输入 ZIP 的 SHA-256。
2. 读取中央目录并审计文件名编码、路径穿越、绝对路径和重复条目。
3. 按 allowlist 提取 mudlib 运行目录和 `config.ini`。
4. 导入 `web/www`，保留本地 JS/CSS/图片资源。
5. 排除 Windows EXE/DLL、`start-64.bat`、`node_modules`、`.DS_Store` 和已确认无用的历史包。
6. 对 Web 页面应用最小、可复查的离线传输补丁。
7. 生成 `payload-manifest.json`，记录每个文件的路径、长度和 SHA-256。
8. 打成单一 payload 归档，作为 APK asset。

当前入口为 `tools/import_zjmud.sh`。归档使用排序条目、固定时间戳和无扩展属性的标准
ZIP；必须能被 Java `ZipInputStream` 完整读出 11,268 个文件。当前确定性 SHA-256 为
`572ca846597495eb9437816aaf3a6bd87c02681c5ee818f2473149b5b518d745`。

对于 ZIP 中编码异常的文件名，不得由 Java `ZipInputStream` 猜测编码后直接重写。应先
确定 FluffOS 实际访问的名字字节；若文件未被引用，可在 allowlist 中显式排除；若被引用，
则使用能保留原始名字字节的 payload 格式和 native 解包器。

## 3. FluffOS 基线

固定源码：

```text
repository: https://github.com/fluffos/fluffos.git
commit: b1453d8b4cb909a381bfb00752a89ffe41d5559f
reported version: 3.0.20170907
```

原 `tangmen.exe` 的构建信息为 GCC 7.3、`-O3 -funroll-loops` 和 x86 专用
`-march=westmere`。Android 构建不得继承 `westmere` 参数。

### 3.1 Native 构建要求

- 使用固定 Android NDK 版本。
- 首期 ABI 仅为 `arm64-v8a`。
- 使用 NDK CMake toolchain 和 libc++。
- 所有第三方库固定源码版本与补丁哈希。
- 优先静态链接非系统依赖，减少 APK 动态装载顺序问题。
- 保留 `-funsigned-char` 和 `-fwrapv` 等原驱动兼容参数。
- Release 保留单独的 native symbols 归档，用于崩溃符号化。

### 3.2 JNI 适配

JNI 层应尽量薄，只负责：

- 接收配置绝对路径和工作目录。
- 设置日志文件描述符。
- 在当前 `:mud` 进程调用 FluffOS 入口。
- 返回正常退出码。
- 提供受控停止入口；若旧驱动无法线程安全接收 JNI 停止调用，则通过本地管理命令或
  信号触发原生关闭流程。

不要尝试在同一个服务进程中无条件第二次调用旧驱动入口。正常重启策略是结束并重建
`:mud` 进程。

### 3.3 当前兼容补丁

- Android `configure.h`、固定 `cc.h`、Bionic 头文件和 iconv 类型兼容。
- C++17 的 `ptr_fun/not1` 替换，以及 Android custom crypt 回退。
- 无 `execinfo` 时禁用 backtrace；FluffOS 入口重命名为 `fluffos_main()`。
- 关闭 `SENSIBLE_MODIFIERS` 并重新生成 grammar/options，恢复旧 MudOS `static` 语义。
- Android 禁用反向 DNS，避免外部查询及空 resolver 崩溃。
- stdout/stderr 写入 `android-driver.log`，FluffOS 日志写入 runtime `log/debug.log`。

## 4. Android 专用配置

从原 `config.ini` 派生，不原地修改源包。至少调整：

```text
mud ip : 127.0.0.1
port number : 3000
mudlib directory : <应用私有目录中的绝对路径>
binary directory : <应用私有目录中的绝对路径>
```

继续保留原数组、mapping、字符串和 evaluation cost 上限，除非真机测试证明必须调整。
配置生成后记录哈希，启动日志中输出配置版本但不输出玩家数据。

## 5. Web 构建

- `index.html` 删除 `/socket.io/socket.io.js`。
- 删除外部 MD5 CDN；当前远程登录代码已被注释，若仍需 MD5 则使用本地文件。
- 在 `main.js` 前加载 `local-transport.js`。
- 保留原 zjmud ESC 控制码解析。
- 删除或替换 `paym()`、`main_login()` 和远程 AJAX 路径。
- Release 页面不允许任意 `window.open()` 或非本地导航。
- Web 资源保持 UTF-8 页面编码；游戏 TCP 字节编码由 Kotlin 传输层负责。

## 6. Android Manifest 要点

- 声明前台服务权限及适合目标 Android 版本的服务类型。
- `MudForegroundService` 放入 `:mud` 进程且不导出。
- 所有 Activity、Receiver 和 Service 显式设置 `exported`。
- 只在回环 TCP 确有需要时声明 `INTERNET` 普通权限。
- 禁止明文外部 HTTP 导航，关闭不必要的文件访问和内容访问。
- 默认关闭 Android Auto Backup，避免未经版本控制地恢复部分 mudlib；使用应用自己的
  存档归档格式。

具体前台服务类型必须按最终 target SDK 再核对。当前用途更接近持续本地运行的
`specialUse`，不能虚报为媒体播放等类型。

## 7. Release 产物

每个版本应产生：

- 签名 APK。
- 未签名或可复现构建 APK。
- `mapping.txt`。
- Native symbols ZIP。
- payload manifest 和源 ZIP 哈希。
- FluffOS 提交及补丁清单。
- APK SHA-256。
- 版本升级和存档兼容说明。

覆盖升级验证必须同时检查玩家存档和 `adm/etc/wizlist`，确保运行期间新增或调整的管理员
权限不会被新版 payload 中的静态列表覆盖。

Release APK 在签名前必须验证不包含 Windows 二进制、Node.js 依赖目录、私有构建缓存
或临时导出存档。

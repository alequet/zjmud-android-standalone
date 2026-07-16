# ZJMud Android Standalone

中文名：指间 MUD 安卓单机版。

本项目把 zjmud 服务端、原 Web 客户端和兼容旧 MudOS 语义的 FluffOS 驱动打包进一个
Android ARM64 应用。应用启动后，MUD 服务端运行在独立的 `:mud` 前台服务进程中，界面由
WebView 加载本地 Web 资源；前后端仅通过 `127.0.0.1:3000` 通信，不依赖设备外部网络。

`ZJMud Android Standalone` 是项目和 GitHub 仓库的正式名称；Android 包名继续使用
`com.zjmud.android`，以保证已安装版本能够覆盖升级并保留私有目录中的玩家存档。

## 当前状态

- 已生成可侧载的 ARM64 Debug APK，当前约 21.8 MiB，SHA-256 为
  `d7bd419e69346a801fbdd51b3d348edee21036771ed7d9a3fdef52da7a4e08fe`。
- 已在 Pixel 7 ARM64/API 33 模拟器验证首次安装、覆盖升级、注册、登录、角色创建、进入
  世界、移动、师门奖励、八卦盘任务飞行、完整角色快照、AI 玩家、存档迁移和前台服务重启。
- 服务仅监听 IPv4 回环地址，WebView 拒绝非应用资产 URL。
- runtime 归档包含 12,086 个条目，当前 SHA-256 为
  `5aebf990587d4be3b313a992b38a1bed7f1e0f2440e5ec933bde480380141589`。
- 5 名 AI 玩家已完成 30 分钟稳定性回归：398 次累计动作、无新增错误或动作失败，
  对象数经过回收后净减少 245，RSS 净增约 1.28 MiB，5 份角色档均周期更新。
- 当前仍是个人单机版工程，不是可替代原联网服的通用发行版。

## 源码基线

| 组成 | 固定基线 |
| --- | --- |
| 原生 zjmud 公开仓库 | [MudRen/zjmud](https://github.com/MudRen/zjmud) |
| zjmud 上游基线提交 | [`c56a166380d74858d7b4f0ba2817478ccea6b83d`](https://github.com/MudRen/zjmud/commit/c56a166380d74858d7b4f0ba2817478ccea6b83d) |
| zjmud 前置补丁 | [`tools/zjmud_patch/web_frontend.patch`](tools/zjmud_patch/web_frontend.patch) |
| zjmud 前置补丁 SHA-256 | `e1f5157a544ae8523b78d2b8bd62a4d4f4bc1b0800e5e8f8c35a9604812c3cbe` |
| 补丁后 `web/www/main.js` SHA-256 | `d5ce16fb65a1beb9d39989244b832e4d776ff0db193f75074e8b69dad77b121c` |
| 本地构建输入 | `~/zjmud-main.zip`（也可通过导入脚本第一个参数指定路径） |
| 本地源 ZIP SHA-256 | `2eee2ab12f81a3a7a7f5824a552f85f9d287c6d3dbfb03c4b1e2c0bfc2578ba0` |
| 原 Windows 驱动 | `fluffos64/tangmen.exe` |
| 驱动版本 | FluffOS `3.0.20170907` |
| FluffOS 上游提交 | `b1453d8b4cb909a381bfb00752a89ffe41d5559f` |
| Android ABI | `arm64-v8a` |
| Android NDK | `29.0.14206865` |

原 `config.ini` 顶部的 `MudOS 0.9.20` 是 mudlib 的历史兼容说明，不是实际驱动版本。
本项目的实际构建输入始终是已经调通的本地 `~/zjmud-main.zip`，不下载或直接使用公开仓库
归档。公开仓库用于追溯原生 zjmud；其 Web 前端存在 ANSI 默认状态、聊天区高度、建角性别
控件以及本地登录/注册流程问题。本地修复版本与公开上游的完整差异保存在独立的
[`tools/zjmud_patch/web_frontend.patch`](tools/zjmud_patch/web_frontend.patch) 中，并随本仓库
版本控制。本地 ZIP 已经包含该补丁结果，因此 `tools/import_zjmud.sh` 只校验本地 ZIP、补丁
文件以及补丁后 `main.js` 的哈希，不会重复应用。公开仓库若要作为新快照来源，必须先应用
该 `zjmud_patch`，但当前构建仍只接受上表固定哈希的本地快照。

导入脚本会拒绝哈希不一致的本地 ZIP、`zjmud_patch` 或补丁后 Web 文件，避免字节级补丁静默
应用到未知版本。仓库跟踪确定性生成的 runtime 资产，但不跟踪本地 ZIP、Windows 二进制和
构建缓存。

## 运行架构

```text
Android MainActivity / WebView
          |
          | WebMessage + GB18030 bridge
          v
127.0.0.1:3000
          |
          v
MudForegroundService (:mud process)
          |
          | JNI
          v
FluffOS ARM64 shared library + zjmud mudlib
```

- 原 Web 页面的 ESC/ZJ 控制码和主要 UI 结构保持不变。
- Kotlin `LocalMudConnection` 代替 Node.js/Socket.IO 代理，固定发送原 Web 握手
  `zjmDMaIpOvxdb\n\n`，并负责 GB18030 编解码和断线重连。
- JNI 只负责切换工作目录、重定向驱动日志、启动 FluffOS 和请求受控停止。
- runtime 以内容哈希作为版本 ID。升级时先解包到 staging，再迁移可变目录和管理员名单，
  最后原子切换；失败则恢复上一份 runtime。

## 相对原生 zjmud 的补丁

所有 mudlib 和 Web 改动都由导入脚本从固定源 ZIP 重建，避免直接维护一份无法追溯的完整
分叉。Android 驱动改动保存在 `native/`，导入规则保存在 `tools/`。

### FluffOS 与 Android 驱动

| 范围 | 当前补丁 |
| --- | --- |
| NDK 构建 | 使用 CMake、NDK Clang、C++17 和静态 libevent；固定 Android `configure.h`、`cc.h` 与编译生成表。 |
| 旧语义 | 关闭 `SENSIBLE_MODIFIERS` 并固定 grammar/options 生成结果，恢复该 mudlib 依赖的旧 MudOS `static` 行为。 |
| Bionic 兼容 | 适配 Bionic 头文件、iconv 类型和缺失接口；使用随包携带的 musl DES/MD5 crypt 保持旧账号密码兼容。 |
| JNI 入口 | 将驱动 `main()` 编译为 `fluffos_main()`，由 `native_bridge.cpp` 在 `:mud` 进程内启动。 |
| 网络边界 | Android 上禁用反向 DNS；不编译 socket efun 包，游戏端口固定绑定 `127.0.0.1`。 |
| 停止与保存 | 使用 `eventfd` 把停止请求送入 FluffOS 事件线程；停止前保存在线角色并 `sync()`，运行期间每 30 秒自动保存。 |
| 诊断输出 | stdout/stderr 写入 `android-driver.log`；旧源码 warning 不再进入玩家 UI，真正的 LPC 编译错误仍按原路径报告。 |
| 第三方依赖 | 固定携带 libevent 和最小 musl crypt 源码，不依赖设备上的非 NDK 动态库。 |

`native/fluffos/src/.gitignore` 对 Android 构建直接引用的配置和 `*.autogen.*` 文件设有明确
例外。这些文件是固定构建输入，不是本机临时产物，必须随仓库提交。

### Mudlib 规则与兼容补丁

| 补丁 | 目标与行为 |
| --- | --- |
| `remove_anticheat.patch` | 删除联网服基于属性和、第二储物袋、未分配体会及付费额的小黑屋判定；旧存档若停在小黑屋则自动释放。 |
| `death-block.c` / `death-shenxun.c` | 保留旧路径作为兼容释放房间，避免已有存档引用缺失或继续被困。 |
| `singleplayer_boosts.patch` | 新账号默认 `(admin)`；`learn/practice/research/study` 收益 10 倍；师门非物品数值奖励 100 倍。 |
| `singleplayer_boosts.patch` | `jiqu` 把实战体会转化为武学修养或剑道修养的单周期吸收量提升至原版 20 倍；指定吸收数量时不会超额扣除。 |
| `singleplayer_boosts.patch` | 师门随机授予武学的概率由 0.01% 提升到 1%，即原版 100 倍；授予等级保持原版 50 级。 |
| `singleplayer_boosts.patch` | 击杀目标的延迟奖励携带师门标记，保证经验、潜能、实战体会、神、阅历、威望和门派贡献都走同一倍率。 |
| `shaolin_unarmed_rewards.patch` | 少林石桌与洗髓经只提升 `unarmed/cuff/strike/finger/hand/claw` 六项基本空手；保留原技能等级范围和触发门槛，单次技能点为原版 500 倍。 |
| `static_admins.patch` | 静态管理员包含 `ranger`、`sxjian`、`pyh0721` 和 `pyh0791`；升级时与运行中的名单合并。 |
| `hide_room_paths.patch` | 管理员移动房间时只输出房间名称，不向中央控制台打印完整 mudlib 路径。 |
| `quest_fly.patch` / `quest_commands.patch` | 扩展受限的 `quest` 目标语义：`fly quest`、`goto quest` 根据当前 `quest`/`bquest` 的击杀目标移动，`kill quest` 在同房间直接攻击该目标；不接受客户端传入任意对象或路径。 |
| `full_character_save.patch` / `fullsave.c` | 增加版本化角色快照：保存安全的当前房间、完整物品树、可序列化动态属性和穿戴状态；旧 `autoload` 存档首次登录后自动迁移。快照只允许在磁盘 `restore()` 后回放一次，在线再次 `setup()` 不会复制背包。 |
| `ai_players.patch` / `ai-playerd.c` | 常驻加载 5 个持久化 AI 玩家；它们使用真实玩家对象按角色分时日程巡游、驻足、补给和交谈。结构化感知玩家进出、说话、问询、附近/自身战斗与时段变化；关系、地点和时段共同改变招呼与对白。v1.6 增加多点巡游、安全休息、真实购物及受控同区域结伴会面；v1.7 增加活动 schema v2、白名单迁移和幂等检查点；v1.8 增加只回应攻击者的有限自卫决策、基于气血/敌我强度/胆量的受控撤退，以及有界重试和战后恢复。系统保留账号隔离、有界寻路、动作白名单和隔离故障场景。训练、主动 PK/追杀、通用组队与任意命令仍显式不支持。 |
| `challenger_runtime.patch` | 修复整点异域挑战死亡和胜利结算的错误延迟销毁调用，并保护空伤害记录、零总伤害及非法掉落概率。 |
| `challenger_level.patch` | 将整点异域挑战 boss 的整体等级由 120 调整为 220；技能、经验、加力、气血、精神及内力统一按 220 级计算。 |
| `runtime_error_fixes.patch` | 修复倚天剑剧情、组合战斗和注册流程中的对象销毁后继续访问问题，包括新账号发送建角控制码前对空角色对象的错误销毁；确保乾坤大挪移招架及九阳奖励补发消息在换行前复位颜色，避免后续状态控制包显示成普通文本。 |
| `offline-gchannel.c` / 离线频道导入替换 | `wiz/ic/ultra` 保留本地显示但不再跨服转发，`gchannel` 本身也替换为离线空实现，避免加载单机版不可用的 `dns_master`；显式跨服私聊、回复、查询和 telnet 入口返回离线提示。 |
| 离线守护替代 | `dns_master`、`messaged`、`versiond`、`cmwhod`、`payd` 和 telnet shadow 替换为 API 兼容的离线实现；保留本地玩家查找、私聊、输出和表情编辑判断，跨服、支付回调、版本同步及外部 telnet 不执行。 |
| network service 替代 | `network/services` 下 22 个 inter-MUD service 及 `network/fs.c` 全部替换为宽 API 离线实现，避免残余路径继续加载 `INETD`、`NETMAIL_D`、`PING_Q` 等缺失组件。 |
| Web 联网清理 | 两份客户端脚本统一使用本地登录/注册流程，移除旧 API AJAX、充值页和主页打开代码；导入时校验外部入口为零，WebView 非本地流量强制代理到不可用的回环端口。 |
| 武庙奖励对象修复 | VIP3 随机及 VIP4 自选九阳神功统一发放实际存在的 `jiuyang-book`；历史失败角色进入武庙时一次性补发。同步修复周师门档位中福寿膏和灵芝的失效对象路径，并验证巫师全部静态奖励对象存在。 |
| `cultivation_success_boost.patch` | 打通任督二脉与元婴出世的每 tick 总成功率提升至原版 10 倍；潜能消耗、tick 间隔、先天根骨判定和其他门槛保持不变。 |
| 八卦盘导入替换 | 四种八卦盘显示的目标房间名包裹 `cmds:fly quest` 链接，点击后飞往目标当前所在房间。 |
| preload 精简 | `securityd` 固定最先预载；不预载依赖联网运营环境的 `messaged` 和 `payd`。 |
| include guard | 首次部署时为 `ansi.h`、`zjmud.h` 和 `globals.h` 补充保护，避免旧源码重复包含导致编译错误。 |

导入脚本扫描完整 mudlib，发布 runtime 中不允许出现任何 socket efun、`resolve()`、
`external_start()`、`db_connect()` 或对已移除联网守护的调用；任何联网能力回归都会使
资源导入失败。

物品奖励、师门次数、连续完成统计和随机武学等级没有跟随数值倍率放大。倍率应用在原版
上限与兜底计算之后，因此潜能或实战体会接近上限时也能得到预期的单机版最终数值。
`jiqu` 只提高每个吸收周期实际消耗的体会及由该体会计算的修养增量；每周期附带的少量
实战经验、潜能、体力精神门槛和技能等级上限判断保持原版。

### Web 客户端

| 范围 | 当前补丁 |
| --- | --- |
| 传输 | 删除 Socket.IO 客户端引用，以 `local-transport.js` 提供兼容的 `on/emit` 接口。 |
| 登录 | 绕过远程账号 API，直接向本机 MUD 发送账号、密码和本地占位邮箱；修复自动登录和错误密码时卡在“登录中”。 |
| 外部资源 | 删除外部 MD5 CDN，移除充值和游戏主页入口。 |
| 导航限制 | WebView 只加载 `https://appassets.androidplatform.net/assets/`，其他 URL 返回 403。 |
| 断线状态 | 本地服务未就绪时持续重连；登录断开和游戏中断开分别显示明确状态。 |
| 文本操作 | 移除原页面对 `contextmenu` 的全局拦截并保持 WebView 可长按；输出文本可复制/全选，输入框可粘贴。 |

### Android 外壳与数据

- `MainActivity` 部署 runtime、启动前台服务并用 `WebViewAssetLoader` 加载本地页面。
- `MudForegroundService` 位于不可导出的独立 `:mud` 进程，使用 `specialUse` 前台服务类型；
  持有服务级 partial wake lock，保证标准后台/熄屏 Doze 期间 FluffOS 心跳和周期保存继续；
  最近任务移除或服务销毁时请求驱动保存、释放 wake lock 并退出。该策略会增加耗电，用户
  强制停止、卸载、重启后未重新打开应用和极端 OEM 回收仍不承诺常驻。
- Debug APK 额外包含显式故障注入 receiver，只能请求非导出的服务自杀，用于验证 `:mud`
  异常退出恢复，或只重建 UI/WebView 进程而保留 `:mud`；release source set 不包含该组件，
  服务端入口也会检查应用可调试标志。
- 应用禁用 Android Auto Backup。存档位于应用私有目录，普通应用和共享存储不可直接访问。
- 角色原有永久属性继续由 MudOS `save_object()` 保存；Android 单机扩展同时保存当前安全房间、
  递归背包/容器、物品动态 dbase、主副手和护具状态。驱动写存档时先写同目录临时文件，
  成功关闭后再原子替换正式 `.o` 文件。
- 克隆房间、封闭流程房间、带 `out_room` 的临时场景及明确禁止登录/保存的位置不会成为恢复点；
  登录时回退到原 `startroom`。NPC、对象引用、函数、`temp` 数据、战斗和 `call_out` 不进入快照。
- 覆盖升级时迁移 `data`、`backup`、`log`、`dump`、`temp`、`adm/tmp`，并合并动态管理员名单。
- APK 内不包含 Node.js、Windows EXE/DLL、原 Socket.IO 服务端或外部下载步骤。

## 与原生联网版的差距

下表同时列出刻意的单机化取舍和仍未完成的工程能力。它们不是原生联网服的等价实现。

| 领域 | 原生联网版 | 当前 Android 单机版 |
| --- | --- | --- |
| 服务可达性 | 可向公网或局域网玩家开放 | 只绑定设备回环地址；不能从其他设备登录 |
| 玩家规模 | 多人长期在线、共享世界 | 一名本机玩家与 5 个持久化 AI 玩家；驱动仍保留 MUD 多对象模型 |
| 账号体系 | Web 远程 API、运营侧账号数据 | mudlib 本地账号和密码存档，无找回、封禁后台或跨设备同步 |
| 充值与运营 | 充值、主页、付费校验、消息和支付守护进程 | 入口和预载均移除，不提供充值、支付回调或运营后台 |
| 跨服能力 | 可依赖 DNS、MUD 间服务和外部 socket | 反向 DNS、socket efun 包及相关联网预载关闭 |
| 权限模型 | 管理员由运营方控制 | 新账号默认管理员，另有静态管理员名单，安全模型只适合个人设备 |
| 游戏平衡 | 联网服倍率、反作弊与付费门槛 | 移除小黑屋反作弊，并启用学习、师门奖励和随机武学概率倍率 |
| 服务生命周期 | 独立常驻服务端 | 受 Android 前台服务和应用进程生命周期约束，不能保证永不被系统终止 |
| 数据位置 | 服务器集中存储与备份 | 应用私有目录以版本化角色快照保存永久属性、位置、物品树和装备；卸载应用会删除存档 |
| 客户端传输 | 浏览器、Node.js、Socket.IO 到 Telnet | WebView、WebMessage、Kotlin 回环 TCP 到 Telnet |
| 平台支持 | Windows x86 驱动为已知原始运行环境 | 当前只构建 Android ARM64，不支持 x86/x86_64 或 32 位 ARM |
| 发布方式 | 服务器部署和 Web 站点 | 当前只有 Debug APK，没有正式签名、商店发布或自动更新通道 |

虽然端口绑定在回环地址，Android 设备上的其他本地进程理论上仍可能尝试连接该端口；当前
设计保证“不出设备”，但没有把 TCP 端口变成应用独占的安全边界。

## 尚未完成

- 使用 Android Storage Access Framework 导入和导出存档。
- Release keystore、正式签名、版本号策略、native symbols 和发布流水线。
- AI 玩家跨区域旅行、结构化社会记忆、玩家邀请的有限同行、注册任务图和可选生成式表达
  尚未实现；主动 PK 和任意组队明确不在规划范围。
- 面向任意 zjmud 版本的通用导入器；当前只接受上面固定哈希的源码包。
- 恢复任何联网服功能。若未来需要多人或跨设备访问，应重新设计认证、端口暴露、TLS、
  备份和管理员权限，而不是直接放开当前本机端口。

Android 前台服务只能提高存活优先级。用户强制停止、设备重启、极端内存压力和 OEM 策略
仍可终止进程，因此“不被杀掉”不是 Android 平台能够绝对保证的条件。

## 构建

环境版本见 [开发环境](docs/DEVELOPMENT_ENVIRONMENT.md)。重新导入 zjmud 资源时使用已经包含
`zjmud_patch` 修复结果的本地 ZIP：

```bash
./tools/import_zjmud.sh ~/zjmud-main.zip
./gradlew :app:assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

只修改 Android/Kotlin/native 代码且不重建 mudlib 时，可直接执行 Gradle 构建。Debug APK
输出在 `app/build/outputs/apk/debug/app-debug.apk`，该目录和 APK 文件不会提交到 Git。

## 文档

- [产品与验收要求](docs/REQUIREMENTS.md)
- [技术架构](docs/ARCHITECTURE.md)
- [实施计划](docs/IMPLEMENTATION_PLAN.md)
- [AI 玩家能力开发路线图](docs/AI_DEVELOPMENT_ROADMAP.md)
- [构建与打包](docs/BUILD_AND_RELEASE.md)
- [开发环境](docs/DEVELOPMENT_ENVIRONMENT.md)
- [测试计划](docs/TEST_PLAN.md)
- [风险清单](docs/RISK_REGISTER.md)

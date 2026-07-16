# 测试计划

## 1. 测试层次

### 1.1 单元测试

- GB18030 编码和有状态解码，特别是每个可能字节边界拆包。
- `local-transport.js` 的 `on/emit` 兼容行为。
- TCP 状态机、写入串行化、重连和握手只发送一次。
- payload manifest 校验、路径安全和原子部署。
- 存档归档校验、导入失败回滚和版本拒绝。
- Web URL allowlist。

### 1.2 Native 测试

- ARM64 依赖链接和符号完整性。
- FluffOS 配置解析。
- JNI 启动、正常退出、异常退出和重复进程启动。
- mudlib preload 编译错误基线。
- 禁止非回环监听和连接。

### 1.3 集成测试

- Activity -> Service -> JNI -> FluffOS -> TCP -> WebView 完整链路。
- 固定协议样本与原 Node 代理输出对比。
- 注册、登录、创建角色、重连和退出。
- AI 玩家创建、在线展示、区域移动、问询回应、周期保存和服务重启恢复。
- 页面控制码 `ESC 000/001/.../022` 的主要 UI 路径。
- 数据写入后重启服务和重启应用。

## 2. 本机与真机验证政策

本机模拟器用于确定性构建、核心冒烟、短时生命周期、迁移、进程终止和恢复回归。测试范围
按改动风险选择，不要求每个版本重复运行完整 Android 生命周期矩阵。

用户持有的 Android 真机作为长期现场环境。耗电、后台存活、OEM 回收和特定系统版本问题
在实际出现后记录场景并单点修复。多设备、多 Android 版本、长时间运行和每版一小时 Doze
不作为开发完成或发布门槛。

涉及数据完整性、安全边界、schema 迁移和幂等的测试必须在本机确定性执行，不能只依赖
真机人工观察。

## 3. 核心冒烟流程

Pixel 7 ARM64/API 33 AVD 已完成登录、进入世界和存档生成。测试账号生成了
`data/login/c/codex713.o` 与 `data/user/c/codex713.o`。2026-07-13 回归还验证了：

- 进入世界后 UI 不显示 unused variable、unknown pragma 或“编译时段错误”，诊断只在驱动日志。
- 在线角色每 30 秒自动保存，角色文件修改时间随周期更新。
- 服务优雅停止时 native 关闭端口并退出，应用与 `:mud` 进程均不残留。
- `adb install -r` 前后自定义数据标记 SHA-256 不变。
- 模拟旧 payload 的完整 runtime 迁移前后，角色文件和自定义标记 SHA-256 不变。
- instrumentation 真实调用 `finishAndRemoveTask()`：在线角色退出存档时间推进，端口关闭，
  主进程、`:mud` 进程和最近任务均无残留；重启后 `online_time`、`mud_age` 继续递增。
- 同一测试确认驱动日志实际产生 unused variable、unknown pragma、参数不匹配、无副作用
  表达式四类 warning，同时 WebView `document.body.innerText` 不含这些诊断或通用 `Warning:`。
- 模拟目录交换中断（只有 `previous`、没有 `current`）后，启动可恢复 runtime 和角色数据。
- 小黑屋移除回归：存档起点注入 `/d/death/block` 后登录，12 秒内进入宝昌客栈，存档立即
  改为 `/d/city/kedian` 且旧路径消失；测试后恢复原角色文件 SHA-256。
- 完整角色快照回归：在中央广场保存普通长剑、已穿棉衣、礼品盒内匕首和八卦盘；退出后
  重登仍在中央广场，长剑与棉衣保持装备标记，匕首可从礼品盒取出。
- 给普通长剑写入动态 `codex_marker=713` 后保存并再次重登，源码路径、动态 dbase 和数值
  均恢复；原本不会进入 autoload 的普通武器和容器没有丢失。
- 使用只有旧 autoload 字段的既有账号覆盖升级登录，首次保存生成 `full_save_version=1`，
  未发生重复物品或登录编译错误。
- `jiqu 2000` 在零级武学修养时单周期把 `learned_experience` 从 0 增至 2,000；原版同等级
  单周期为 100。装备长剑后执行 `jiqu 2000 sword-cognize` 同样单周期吸收 2,000，且分别
  触发武学修养和剑道修养增长，没有超出显式指定数量。
- 师门交付和击杀目标的延迟结算均将经验、潜能、实战体会、正负神、江湖阅历、威望及
  门派贡献按原版最终值乘以 100；物品奖励、任务计数和连续次数不随倍率变化。
- 师门随机授予武学的两条结算路径均使用 `random(10000) < 100`，触发率为 1%（原版
  100 倍），授予等级仍固定为 50 级。
- 接取师门击杀任务后，`fly quest` 与 `goto quest` 均到达目标当前房间，随后 `kill quest`
  无需目标 ID 即可开战；没有击杀任务或目标不在当前房间时返回受限错误，不解析任意路径。
- 强制启动整点异域挑战并分别覆盖挑战者死亡与获胜结算；`ic` 战败消息只在本地显示，
  不加载 `gchannel` 或已移除的 `dns_master`。奖励和掉落完成后挑战者在下一时段安全销毁，
  玩家界面与驱动日志均无执行时段错误。另以空伤害映射、零总伤害及非法掉落概率回归，
  结算安全跳过异常条目。
- 分别执行本地 `wiz/ic/ultra` 频道及本地 `tell/reply/finger`，确认功能保持；对带 `@mud`
  的私聊、回复、查询和管理员 `telnet` 确认返回单机离线提示，且 `dns_master` 不被加载。
- 强制加载离线 `dns_master/messaged/versiond/cmwhod/payd/gchannel` 与 telnet shadow，逐个调用
  公开查询或 no-op API；确认全部 LPC 编译成功、空拓扑/离线状态符合预期且错误日志为空。
- 解包最终 runtime 并扫描所有 `.c`，确认 socket 全接口、resolve、external_start 与 db_connect
  调用数量严格为零，且没有调用已移除的联网守护；六个显式跨服入口均有守卫，七个可
  懒加载组件均带离线替代标记。
- 强制加载 `network/services` 下全部 22 个对象和离线 `network/fs.c`，确认无 `INETD`、
  `NETMAIL_D`、`PING_Q` 或 `DNS_MASTER` 链式加载错误。
- 检查两份最终 `main.js` 哈希一致、本地登录/注册/角色创建发送路径各自存在，且可执行代码
  不含 `92mud.com`、`$.ajax`、`window.open` 或 `io.connect`；清空站点数据和保留自动登录
  cookie 两种启动路径均不出现未初始化 `dialog("close")` 异常。
- 清空 WebView 数据后首次启动，检查应用 UID 的 TCP 表；除 `127.0.0.1:3000` 的监听与
  WebView/MUD 回环会话外不得出现连接，WebView Safe Browsing 与指标服务不得触发外连。
- 从武庙巫师领取 VIP4 并自选九阳神功，确认技能提升至 50 级且背包收到
  `/clone/book/jiuyang-book`，驱动日志无对象加载错误；VIP3 随机九阳分支执行相同检查。
  对已写入 VIP3/VIP4 领取标记、九阳为 50 级但缺少经书的旧角色，确认进入武庙只补发一次。
- 背包复制回归：角色携带普通长剑和匕首保存后，在线连续两次执行 `setup()`，背包始终为
  2 件；退出重登恢复后再次执行 `setup()` 仍为 2 件，证明完整快照只在磁盘恢复后回放一次。
- Pixel 7 WebView 长按输出文字显示 Copy、Share、Select all；复制文字后在空命令输入框长按
  显示 Paste。原页面的全局 `contextmenu` 拦截已不存在。
- 2026-07-14 首次启动创建 5 组 `data/login/a/ai_*.o` 与 `data/user/a/ai_*.o`；`who` 同时
  显示一名真实玩家和 5 名 AI 玩家，`aiplayer status` 显示五者均为空闲且分布在各自区域。
- 暂停 AI 后传送到沈清风所在房间并执行 `ask ai_qingfeng about jianghu`，恢复调度后收到
  延迟的通用回答；运行期间五份存档的安全房间持续变化，证明白名单移动与周期保存生效。
- 覆盖安装并完整重启两个应用进程后，5 个 AI 玩家从既有角色和背包快照恢复，继续移动并
  重写存档；日志无 AI 编译错误、执行时段错误或 `Can't load objects when no effective user`。
- 2026-07-14 AI v1.1 回归：`status` 显示巡游与驻足目标，`inspect` 显示意图、需求、最后
  动作和最近房间；`trace` 记录实际路径动作，`home songlan` 和 `reset qingfeng` 分别恢复
  常驻点与完整行为状态。暂停 20 秒前后食物值均为 270，证明生存消耗冻结且保存仍可执行。
- 将沈清风食物和饮水强制降至 1 后，AI 使用白名单补给动作把饮水恢复到 314、食物恢复到
  163，超过安全阈值后继续巡游；临时补给未进入完整背包快照。问询后存档记录
  `codex713` 熟悉度与最近话题，完整进程重启后关系、路线索引和位置均保留。
- 2026-07-14 AI v1.2 回归：`aiplayer validate` 确认 5 份分时日程房间与常驻点双向可达；
  首次校验发现并移除了不可达的少林 `fzlou`。`metrics qingfeng` 记录 44 次动作、0 次失败、
  0 个错误、6 次路径搜索与 11 个 BFS 节点，导航缓存 14 次命中/6 次未命中；12 个事件全部
  处理且无丢弃。trace 明确记录 `player_enter/player_leave`，进入事件按熟悉度称呼“测试侠客”。
  `ask ... about jianghu` 的结构化主题写入存档，关系每日增量停在 3；覆盖安装和完整进程重启
  后熟悉度、话题、日程位置和招呼阈值继续生效。附近/自身战斗转换已通过真实 FluffOS 编译，
  仍需在不会击杀测试角色的隔离战斗场景中做行为回归。
- 2026-07-15 AI v1.3 回归：`aiplayer selftest` 对 5 个角色全部通过，覆盖活动配置、日关系上限、
  队列优先级保护和导航缓存。五个角色均产生并完成角色活动；`metrics zhiyuan` 观察到路径
  动态阻挡后的 `route_failures=3 relocations=1`，失败动作停止继续增长，活动仍可完成，且
  FluffOS 预加载无 AI warning/error。活动路径失败会取消当前活动并回常驻点，后续重新安排，
  证明不会因 `valid_leave` 阻挡形成无限重试。
- 2026-07-15 AI v1.4 回归：重建后 `aiplayer validate` 和 5 名角色 `selftest` 全部通过；
  `aiplayer scenario combat ai_wantang` 在无出口测试室启动真实非致死 `fight_ob`，状态报告确认
  `self_combat_started`、`self_combat_ended`、原房间和气血/精神/内力快照均恢复，场景状态中的
  `pending_events=0`。场景旁观事件未改变后续日程，也未残留“隔离测试”目标。`activity supplies ai_wantang`
  在无银两时记录适配器前置条件失败并结束活动；
  `activity supplies ai_wantang seed` 注入 30 银后在醉仙楼真实执行 `buy 1 baozi` 和 `eat baozi`，
  货币从 3000 文降至 2950/2900 文，食物从 135 回升至 195，指标记录适配器成功且无命令/后置失败。
- AI 首次初始化必须在清空应用数据后验证：`data/login/a/ai_*.o` 与 `data/user/a/ai_*.o`
  各生成 5 份，`android-driver.log` 不得出现 AI 登录对象的 `Denied write permission in save_object()`；
  完整重启后五名角色仍可加载且不重复创建账号。

常用命令广度和导入导出仍待执行；真机长期运行问题按实际反馈跟进。

1. 清除应用数据并开启飞行模式。
2. 启动应用，观察资源部署和前台通知。
3. 确认服务只监听回环端口。
4. Web UI 显示本地登录页。
5. 注册新账号并创建角色。
6. 进入世界，执行移动、查看、聊天、技能和战斗命令。
7. 退出登录后重新登录。
8. 将应用切后台、熄屏再恢复。
9. 停止并重启服务，确认角色数据存在。
10. 检查当前位置、容器子物品、动态属性及穿戴状态均恢复。
11. 导出存档并验证归档清单。

AI 玩家回归可由管理员执行 `aiplayer status|pause|resume|reload|save`，并使用
`metrics [id]`、`events <id>`、`validate`、`selftest [id]`、`scenario combat/status <id>`、
`scenario defense <id> <defend|retreat|noexit|blocked|unconscious|death>`、
`activity supplies <id>`、`recovery <id>` 和 `inspect/trace/home/reset <id>` 诊断。测试问询时
先暂停并传送到目标身边，恢复调度后再发出 `ask`，确保显式 `ask` 事件带入真实主题；测试
结束必须恢复调度，且不得用人类客户端登录任何 `ai_` 前缀账号。

自动化入口：

- `tools/test_ai_player_startup.sh --yes` 会构建并安装 Debug APK、清除
  `com.zjmud.android` 数据、验证首次启动 5+5 存档和错误日志，再完整停止重启并复查。
  这是破坏性测试，未提供 `--yes` 时脚本拒绝运行。
- `adb forward tcp:3000 tcp:3000` 后运行 `tools/ai_admin_smoke.py`，会通过本地回环协议创建或
  登录一次性管理员，执行 `status`、`validate` 和五角色 `selftest`。增加 `--scenarios` 后会
  顺序运行 v1.8 六模式矩阵：弱敌自卫、强敌安全撤退、无出口、出口连续阻断、昏迷恢复和
  死亡复活；每项读取 `AI_SCENARIO` 机器输出，并触发苏晚棠带测试银两的真实补给活动。
- `tools/ai_admin_smoke.py --behaviors` 强制五名角色分别完成角色专属多点巡游和安全休息，
  再让沈清风邀请苏晚棠到小花园会面；通过 `inspect` 的最近活动与结果字段等待真实完成，
  失败时自动复位五名角色，不依赖固定延时假定成功。
- `tools/test_ai_player_stability.py --duration 1800 --interval 60` 每分钟采集机器可读的
  `aiplayer stability`、Android PSS/RSS、5+5 存档集合与修改时间，并按测试起点计算新增
  `errors/action_failures`。脚本还检查角色是否全部加载、登录档与角色档是否一一对应、
  300 秒周期保存是否推进，以及对象数在后半程是否持续显著增长；JSON/CSV 输出到
  `build/reports/ai-stability/`。
- `tools/test_ai_player_recovery.py` 依次在巡游、休息、结伴、购物已扣款、隔离战斗、周期保存
  边界和非法 legacy 状态处精确终止 `:mud`，每次重启后验证 schema v2、5+5 存档、无残余
  战斗/测试房间、无永久等待、事件队列和错误指标。Debug APK 的 receiver 只负责让非导出的
  `:mud` 服务进程自杀，release APK 不包含该 receiver。可用多个 `--case <name>` 只跑指定项。
- 第4项生命周期测试使用同一脚本的 `--lifecycle-case`：`force-stop`、`low-memory`、
  `background`、`webview`、`doze`、`reboot`、`upgrade`。`--lifecycle` 运行全部；`reboot`
  和 `upgrade` 必须显式传 `--yes`。升级用例接收从 `34c83e6` 构建的 v1.6 APK，通过
  `--legacy-apk` 传入，并要求当前 APK 的 runtime SHA 不同。

2026-07-15 AI v1.6 稳定性回归在 Pixel 7 ARM64/API 33 模拟器连续运行 1800 秒，共 30 个
采样点。5 名角色动作计数最终分别为 70、83、75、118、52，合计 398；五者新增
`errors=0`、`action_failures=0`、`route_failures=0`、`relocations=0`、`respawns=0`，最终事件
积压均为 0。驱动对象数范围为 1233–1897，起止为 1584→1339，运行中出现明确对象回收；
RSS 范围为 196460–198260 KiB，起止为 196944→198252 KiB。5 份登录档和 5 份角色档始终
精确对应，所有角色档修改时间均跨越多个 300 秒保存周期，新增运行时错误标记为空。

2026-07-15 AI v1.6 行为增强回归：`validate` 和五角色 `selftest` 全部通过，新增校验覆盖完整
分时招呼、地点语境、多点巡游、休息配置及合法结伴对象。`--behaviors` 确认沈清风、苏晚棠、
赵砚秋、林知远、陈松岚分别完成角色专属巡游和休息，沈清风与苏晚棠在扬州小花园完成
受控会面；`--scenarios` 再次确认 5 个隔离战斗及苏晚棠真实购买/进食无回归。随后 300 秒
稳定性复核中动作计数增加 80，新增 `errors/action_failures` 均为 0，5+5 存档保持完整。

2026-07-16 AI v1.7 恢复回归：活动 schema v2 在 profile 重建时只保留白名单进度；legacy
巡游状态成功迁移，非法测试房间目标恢复为 `cancelled_invalid_target`。巡游、休息、双方
结伴、苏晚棠购买酒袋后、隔离战斗运行中和 300 秒保存边界的 `:mud` 精确终止均恢复成功；
购物检查点保持一次 1500 文酒袋扣款，恢复后仅继续按 50 文单价购买所需包子，没有重复
购买酒袋。所有用例结束时 5 名角色 schema 均为 2，无残余战斗、测试房间或事件积压。
随后 `--behaviors`、`--scenarios` 全部通过；300 秒稳定性复核共 11 个采样点，动作计数
149→236，新增 `errors/action_failures` 均为 0，对象数净增 76，RSS 净增 7400 KiB，未出现
持续显著增长。

2026-07-16 AI v1.7 Android 生命周期回归：`am force-stop`、critical trim-memory、3 次前后台
切换、2 次 UI/WebView 进程重建、设备重启和 v1.6 APK 覆盖升级全部通过；升级后实际记录
schema v2 migration，设备重启后默认不自启动服务。Doze/熄屏 3600 秒完整测试通过，`:mud`
PID 全程稳定，AI 动作持续推进，5 份角色档修改时间均跨越多个 300 秒周期。Debug receiver
只存在于 debug manifest，release manifest 复核不包含它。最终 Debug APK SHA-256 为
`59e2d19896137703da1d08d27dea2c6cd3d37dd8be0ab4cb91af73bf9d810c08`。

2026-07-16 AI v1.8 有限自卫回归：六模式隔离矩阵全部通过。弱敌由木桩先发起 `fight_ob`，
AI 决策为 `defend` 且未尝试撤退；3 倍经验强敌触发 `retreat` 并通过唯一安全出口离开；
无出口场景进入 `trapped` 且不发出任意移动；阻断出口执行真实 `go`，3 次失败后停止重试。
真实 `unconcious()` 被观察并恢复；死亡场景通过仅允许 `ai_playerd` 调用的角色入口注入幽灵态，
再走生产 `process_player → respawn_player` 路径，避免测试触发经验/技能死亡惩罚。场景结束均
还原气血、精神、内力、原房间、活动和事件队列。完整七项 `:mud` 恢复矩阵再次通过，报告
`recovery-20260716-153925.json`；force-stop、低内存、3 次前后台、2 次 WebView 重建和 60 秒
Doze 快速回归通过，报告 `recovery-20260716-155541.json`。v1.7 的 3600 秒 Doze、设备重启与
v1.6 覆盖升级长验收仍由当日既有报告覆盖。最终加固版又通过六模式矩阵及战斗/migration
恢复复核，报告 `recovery-20260716-162548.json`。最终 v1.8 Debug APK SHA-256 为
`d7bd419e69346a801fbdd51b3d348edee21036771ed7d9a3fdef52da7a4e08fe`。
随后 300 秒稳定性复核共 11 个采样点，动作计数 90→159，新增 `errors/action_failures` 均为
0，对象数净增 124，RSS 净增 7068 KiB，五份角色档均跨过周期保存且后半程未持续显著增长；
报告为 `stability-20260716-155817.json`。

## 4. 故障注入

- 部署过程中杀死应用。
- payload 文件缺失、哈希错误、磁盘空间不足。
- TCP 端口被占用。
- FluffOS 启动超时或 preload 失败。
- 强制终止 `:mud` 进程。
- 在 AI 活动 schema 迁移、购物扣款后置条件和结伴双方对账之间强制终止 `:mud`。
- 强制终止 WebView 主进程。
- 在中文字符的每个字节位置拆分 TCP 包。
- 连续快速发送命令和大段地图数据。
- 导入损坏、截断或版本不兼容的存档。
- APK 更新时保留旧玩家数据。
- runtime payload 变化时保留 `data/` 并清理或恢复 `previous`。
- 在定时保存间隔内优雅停止服务，确认退出保存和端口释放。
- 系统低内存和 Doze 场景。
- 在有限自卫的 defend/retreat/trapped、昏迷和死亡节点终止 `:mud`，确认没有策略或敌对残留。

预期结果必须是可解释的错误状态、可恢复操作和数据不被静默覆盖。

## 5. 离线验证

- 飞行模式完成首次启动和完整冒烟流程。
- 使用抓包或系统网络统计确认没有非回环连接。
- 检查 DNS 查询为零。
- 尝试触发原页面充值、主页和远程 AJAX 路径，确认入口不存在或请求被拒绝。
- 审计 FluffOS/LPC socket 日志，确认没有 MUD 间 UDP 服务。
- 从另一台局域网设备扫描手机端口，确认 TCP 3000 不可达。

## 6. WebView 验证

- 不同屏幕尺寸、状态栏、导航栏和显示缩放下无关键控件遮挡。
- 软键盘打开时登录和命令输入可见。
- 横竖屏策略符合最终产品决定，旋转不导致未保存命令或服务重启。
- 外部 URL、`file://`、任意 intent 和新窗口被拒绝。
- Release 构建关闭 WebView 调试。
- 页面重载后能重新连接但不会重复创建服务端。

## 7. 性能与稳定性

原型阶段记录以下可重复测量的基线，再为 Release 固定预算：

- 首次资源部署时间。
- 常规冷启动到登录页时间。
- UI 进程、服务进程和合计内存。
- 服务空闲和战斗高频输出时 CPU。
- 短时后台或熄屏期间的存活、动作推进和周期保存。
- 相关故障注入后的 native 状态、断线和存档恢复结果。
- APK 体积和安装后数据体积。

不应在没有真机测量前承诺固定耗电或 OEM 存活数值。Release 候选必须完成与本次改动相关的
短时稳定性和恢复测试；长时间运行数据作为现场信息，不作为统一门槛。

## 8. 发布门槛

- 所有必测冒烟场景通过。
- 无 P0/P1 数据损坏、启动失败或外网访问问题。
- Native 崩溃可符号化并有导出日志。
- 本机确定性构建、导入、核心流程和相关恢复测试通过。
- 签名 APK 在断网的新设备上可独立安装运行。
- 升级和存档导入导出经过实测，不只依赖单元测试。

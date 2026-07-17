# AI v2.x 区域旅行

状态：v2.4 已冻结。v2.0-v2.3 的基础结构、闭环、真实接入和恢复迁移均已完成。
不允许玩家注入目的地。

## 能力与 schema

旅行诊断必须分别报告：

- `capability_version`: `v2.4`
- `schema_version`: `2`

区域、节点和边都使用稳定的 ASCII ID。ID 一旦进入存档或诊断脚本，不得因为显示名称变化
而复用给另一个对象。

## 区域图

每个区域记录 `id`、显示名称和 `enabled`。每个交通节点记录：

| 字段 | 约束 |
| --- | --- |
| `id` | 全局唯一稳定 ID |
| `region` | 必须引用已启用区域 |
| `room` | 观察或到达确认用的房间路径；v2.0 不直接移动到此路径 |
| `kind` | `walk`、`carriage`、`boat` 或 `teleport` |
| `enabled` | 禁用节点不可作为起点、终点或中转 |
| `safe` | v2.0 只批准安全节点 |

交通边记录 `id`、`from`、`to`、`transport`、`cost`、`risk`、`time`、`enabled`、
`prohibited` 和 `allowed_roles`。边是有向的；需要往返必须显式注册反向边。未注册的房间
出口、命令或对象路径都不属于区域图。

当前确定性夹具包含扬州、武当、杭州和少林四个区域、8 个节点、14 条有向边和注册路线
`test_city_to_wudang`。路线搜索
只在这张白名单图上执行，最多访问 24 个节点、8 层深度。

## 旅行状态与预算

规划结果使用以下机器可读字段：

```text
state: planned | executing | arrived | cancelled
cancel_reason: none | unknown_node | unknown_route | role_denied | no_route | path_limit |
               budget_exceeded | node_disabled | edge_disabled | adapter_failed | route_removed |
               route_disabled | invalid_checkpoint | legacy_cancelled | unsupported_schema |
               migration_failed
schema_version: 2
from, to, role, nodes, edges
total_cost, total_risk, total_time, transfers, retries
checkpoint: actor_id, route_id, state, edge_index, charged_edges, updated_at, trigger,
            event_seq, last_event, last_event_at, cancel_reason
```

检查点还必须包含 `current_node`、`charged_total`、`refunded`、`retries` 和 `started_at`。
`safe_return` 随注册路线保存，用于路线配置已经删除时仍能安全返回。`migrated_from_schema`、
`recovery`、`seeded_total`、`last_departed_edge` 和 `test_route_disabled` 是迁移、执行或管理员
回归专用的可选字段，不构成玩家可写入口。

v2.0 固定上限为：费用 500 文、累计风险 3、换乘 4 次、执行时间 1800 秒、重试 2 次。
这些字段先作为规划和检查点契约存在，不会触发真实扣费或移动。

## 适配器接口

交通适配器必须接收已校验的边 ID 和旅行请求，返回 `state`、`adapter`、`edge_id`、
`side_effects` 和 `schema_version`。v2.0 只提供 `test` 适配器：它验证边存在并返回
`executed`，`side_effects=0`，不会发送命令、扣除货币或改变 AI 位置。

未来的步行、车船、传送适配器只能在批准的角色、节点和边上实现，并保持同一取消原因
集合；不得从适配器内部发现新路径或扩大预算。

## 管理员诊断

```text
aiplayer travel schema
aiplayer travel validate
aiplayer travel selftest
aiplayer travel inspect city_gate
aiplayer travel route test_city_to_wudang
aiplayer travel run qingfeng_city_to_wudang seed
aiplayer travel status ai_qingfeng
aiplayer travel schedule ai_qingfeng day seed
aiplayer travel recovery ai_qingfeng
aiplayer travel recover ai_qingfeng
```

`validate` 检查 ID、端点、区域、安全属性、交通类型、角色白名单和数值上限；`route` 只
返回规划结果；`selftest` 固定验证图完整性、单路线、预算上限和无副作用测试适配器。

## v2.1 单角色闭环

`qingfeng_city_to_wudang` 只批准 `ai_qingfeng` 从扬州驿站前往武当山门。管理员显式触发
后，执行器按边扣费、保存检查点、通过批准节点移动并确认到达。任一步失败都会退还本次已
扣费用并返回扬州驿站；每条边最多重试 2 次，总费用、风险、换乘和时间仍受 v2.0 预算限制。
`seed` 独立注入并消耗本次测试所需资金，不使用或改变角色原有余额，不属于 AI 经济行为。

管理员回归还提供 `insufficient` 和 `edge_disabled` 两个故障注入模式。前者临时清空资金并
在断言后恢复原值；后者在首边扣费后模拟交通边关闭。两者都必须取消到 `city_station`，
后者必须精确退回 120 文。这些模式不会进入日程或普通玩家命令。

该闭环不会由日程自动触发，不对其他四名 AI 开放，也不接受任意起点、终点或房间路径。

## v2.2 真实世界接入

v2.2 仍只批准 `ai_qingfeng`。白天目的节点为 `wudang_gate`，早晨、晚间和夜间目的节点为
`city_station`；出发路线分别固定为 `qingfeng_city_to_wudang` 和
`qingfeng_wudang_to_city`。守护心跳只在角色恰好位于注册路线起点时执行当前时段路线，
不能从任意房间发现或拼接路径。

角色在批准的远端节点停留时，v1.x 本地巡游暂停，不会把角色直接送回家；时段变化后必须
通过注册返程路线回到扬州。每条交通边移动前后向房间广播离开/到达文本，同时把 `trigger`、
`event_seq` 和 `last_event` 写入旅行检查点，供管理员和回归测试检查。普通玩家只有观察权，
没有触发、改目的地或注入命令的入口。

## v2.3 恢复与迁移

旅行检查点在计划、扣费、换乘和到达确认前后保存。恢复只从检查点的 `edge_index`、
`charged_edges` 和 `current_node` 继续：已扣边不会再次扣费，已完成移动不会再次移动，
`arrived` 是幂等无操作。进程重新启动后守护心跳自动恢复；管理员也可以显式执行：

```text
aiplayer travel recovery prepare ai_qingfeng planned
aiplayer travel recovery prepare ai_qingfeng charged
aiplayer travel recovery prepare ai_qingfeng executing
aiplayer travel recovery prepare ai_qingfeng arrived
aiplayer travel recovery prepare ai_qingfeng legacy
aiplayer travel recovery prepare ai_qingfeng removed
aiplayer travel recovery prepare ai_qingfeng disabled
aiplayer travel recovery prepare ai_qingfeng invalid
aiplayer travel recovery prepare ai_qingfeng future
aiplayer travel recovery prepare ai_qingfeng legacy_invalid
aiplayer travel recovery prepare ai_qingfeng legacy_removed
```

schema 1 检查点会迁移到 schema 2；缺少路线、禁用路线、非法节点、旧格式无法补全或未知
schema 都会写入明确的 `cancel_reason`，退还已扣费用并移动到登记的安全返回节点，不能
永久停留在等待状态。恢复取消原因包括 `route_removed`、`route_disabled`、
`invalid_checkpoint`、`legacy_cancelled`、`unsupported_schema` 和 `migration_failed`。

## v2.4 冻结契约

v2.4 不新增行为能力。区域图维持 4 个区域、8 个节点、14 条有向边和 3 条注册路线；
真实日程仍只批准 `ai_qingfeng`。travel schema 固定为 2，`AI_TRAVEL_SCHEMA`、
`AI_TRAVEL_STATUS`、`AI_TRAVEL_RECOVERY`、`AI_TRAVEL_ROUTE` 和 `AI_TRAVEL_SELFTEST`
的字段顺序冻结。修改这些字段、取消原因、预算或稳定 ID 必须作为后续独立兼容性变更，
不能静默改写。

冻结回归由三层组成：`tools/test_ai_travel.py` 固定图、预算、schema、状态和诊断字段；
`tools/ai_admin_smoke.py --travel` 验证单路线闭环、失败退款、往返日程和角色白名单；
`tools/test_ai_player_recovery.py --case travel` 杀死 MUD 进程并验证计划中、已扣费、换乘中、
已到达、旧 schema、路线删除、路线禁用、非法节点和未来 schema 的恢复矩阵。

## 明确不在 v2.4

- 不接入其余四名 AI 的跨区域日程。
- 不开放任意目的地、任意房间或自动交通命令发现。
- 不开放自由经济、玩家委托、组队、社会记忆、任务图或语言模型。

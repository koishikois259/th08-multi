# 换关加载同步问题：修复交接说明

日期：2026-10-17

审计基线：`94a8482`，v0.42（包含主机慢速模式同步的刷新版）

目的：记录三个缺陷及修复依据。第 1–8 节保留修复前的审计结论，**本地修复和验证结果见第 9 节**。

## 1. 结论与证据边界

已找到三个具体代码缺陷，均有隔离测试支持。但**尚未用用户故障现场或真实双机完整流程证明它们就是本次反馈的全部根因**。

| 编号 | 问题 | 已确认的结果 | 与 error=7 的关系 |
| --- | --- | --- | --- |
| A | 开局边界上，游戏先获准更新，随机数下一次取输入才重置 | 能出现“simulation ready=true，RNG synchronized=false” | 若首帧前双方随机状态不同，可能造成不可撤销的模拟差异；实际分岔来源尚未完全确认 |
| B | 换关就绪信号复用真实 R 键，未隔离硬件输入 | 加载未完成、甚至设置未确认时，真实 R 仍能冒充 ready | 可以提前放行较快的一端，较慢的一端仍被加载检查阻挡，具有直接造成模拟不一致的条件 |
| C | 加载线程重置当前帧的采样标记 | 指定交错下，同一帧重复采样冲突，远端输入到了也卡住 | 已复现的是卡帧，不是直接 error=7；不要把它报告成已经复现的不同步退出 |

`MULTI_NET_ERROR_DESYNC = 7` 定义于 `src/MultiNetSession.hpp`。`StoreLocalHash` / `CompareRemoteHash` 发现相同网络帧的哈希不同后设置该错误，`src/Supervisor.cpp:134–149` 记录日志并主动结束游戏。因此，看到 error=7 不等于发生了访问违规或堆损坏。

文中行号是上述基线的定位提示，实施时以函数名和实际代码为准。

## 2. 阅读顺序与基本约束

先阅读仓库 `AGENTS.md` 以及相关架构/开发说明，再检查当前工作树和最新代码，防止覆盖其他修改。主要文件：

- `src/MultiPlayerCoordinator.cpp/.hpp`：联机生命周期、取输入、ready 屏障、RNG 同步。
- `src/GameManager.cpp`：`AddedCallback`、`GameplaySetupThread`、`OnUpdate`、`DeletedCallback`。
- `src/Supervisor.cpp`：主线程网络 Pump、输入获取和场景切换。
- `src/MultiNetSession.cpp`：采样、输入消费、哈希比较。
- `src/MultiNetProtocol.cpp/.hpp`：输入历史和线上协议。
- `src/Global.cpp`：回调链与锁；不能假定回调执行期间持有链表锁。

保持以下行为：网络帧号在连接期间单调递增；已提交的同一帧输入不可改变；加载和等待期间继续网络收发/重传；设置确认屏障仍然有效。保留 error=7 检测，不以关闭哈希校验掩盖问题。

普通单人、本地双人、远程双人要区分。联机修复应按项目现有条件编译方式隔离，遵守原版重建的 ABI 和比较要求。不要顺手改动原版数据、存档或无关重建实现。

## 3. 问题 A：第一帧放行与 RNG 重置边界不一致

### 代码位置

- `MultiPlayerCoordinator::IsGameplaySimulationReady`，约第 303 行：远程路径使用 `networkFrame >= gameplayStartFrame`。
- `AcquireGameplayInputs`，第 514–520 行：在函数前部根据上述条件重置 RNG。
- 同函数，第 598–604 行：先 `networkFrame++`，再判断 ready 并增加游戏帧数。
- `GameManager::OnUpdate`，第 405–408 行：根据同一个 ready 判断放行下游游戏更新。

### 触发顺序

设双方约定的开始边界为 100：

1. 进入取输入时 `networkFrame=99`，RNG 同步检查不通过。
2. 成功取得输入后，`networkFrame` 增至 100。
3. ready 变成 true，同一轮主线程回调中的 GameManager 和下游模拟开始执行。
4. 下一次进入取输入，才把 RNG 重置为会话种子。

隔离探针输出：

```text
boundary: ready=1 rngSynchronized=0 seed=beef networkFrame=100
next input: rngSynchronized=1 seed=1234
```

这里的 `beef` 是探针主动设置的旧种子，用来证明执行顺序；不是声称真实游戏必然产生这个值，也不是双机实际分岔的证据。

### 建议修改方向

统一“本轮是否执行模拟”的判定、随机数重置、输入消费与帧计数边界。建议明确区分“下一帧网络编号”与“本轮获准执行的模拟帧”，避免前后读取变化中的 `networkFrame` 导致含义不同。

可选择在取输入成功后的边界转换处完成一次性的模拟启动准备，并在返回给 GameManager 之前同步 RNG；或者重构为显式阶段状态，在同一网络边界完成启动。具体放置点由实施者根据哈希的采样时刻确定。

必须满足：

- 第一轮下游模拟执行前，RNG 已同步；每次换关只同步一次。
- 等待远端输入和重复调用不能再次重置正在运行的 RNG。
- 首帧哈希、`frameNumber`、玩家 input.current/previous 与实际执行的模拟帧有一致语义。
- 不要只是把 `>=` 改成 `>`、将 startFrame 加一/减一或提前重置种子，随后不检查其他使用者。
- 不要仅把 `gameplayRngSynchronized` 加入原 ready 判断：若重置 RNG 的条件仍依赖同一个 ready，会产生循环依赖而永远不能启动。

### 验收

覆盖 start−1、start、start+1；输入延迟 1、3、12；双方加载耗时不对称；开局边界等待远端输入多次；连续换关和返回标题后重新开始。断言每一次实际模拟更新都在本关 RNG 同步之后，且两端使用相同输入和游戏帧编号。

## 4. 问题 B：真实 R 键能够伪造换关 ready

### 代码位置

- `src/Global.hpp`：`TH_BUTTON_RESET = 1 << 14`；输入实现把 R 键映射到该位。
- `AcquireGameplayInputs`，第 524–536 行：满足加载完成和设置确认条件时，通过 `localButtons |= TH_BUTTON_RESET` 发送 ready，但此前没有移除真实 R 输入。
- 第 586–596 行：把双方输入中的这个位直接解读为加载完成。
- 远程 `IsGameplaySimulationReady` 不额外检查本地 `gameplaySetupState`。

### 触发顺序

较慢的一端仍在加载，用户按住 R；较快的一端正常发出 ready。较慢端的真实 R 与系统 ready 无法区分，双方可能提前得到 `gameplayReadyMask=3`。网络和联机帧计数开始推进，但慢端 GameManager 仍因加载未完成阻止游戏模拟，快端则可能已经运行。

隔离探针输出：

```text
physical R during load: setupState=1 readyMask=3 start=8 ready=1 gameplayFrames=3
unconfirmed settings: capturedReadyBit=1
```

第二行单独证明：即使 `AreGameplaySettingsConfirmed()` 为 false，玩家真实的 R 仍能被发成 ready 位。

### 建议修改方向

优先把阶段就绪信息与用户按钮分离，使 ready 只能由程序根据加载完成、设置确认产生。若增设控制字段，建议携带本次关卡/加载代次，避免旧阶段信号用于新阶段；接收端也要校验。

若采用较小范围的兼容实现，至少需要：

- 在远程加载握手阶段先过滤物理 R，再由程序条件性地生成 ready 信号。
- 控制信号不得进入正常游戏输入或暂停菜单处理。
- 注意输入延迟和冗余重传：readyMask 变成 3 后，缓冲区里仍可能有旧 ready 位；不能因为“不再等待 ready”便把这些位当成真正的 R。
- 进入正常游戏后仍保留原有合法 R 操作，不要永久禁用 R 或影响本地模式。
- 检查是否应对本地加载完成加防御条件，但不能只给一端加阻挡而放任另一端继续模拟；修复核心是双方一致的屏障。

不要未经检查就换用其他按钮位；需检查输入编码校验、标题 ready 位及所有消费方。

### 验收

双方加载均未完成、一快一慢、设置尚未确认时，分别测试一端/两端按 R、按住 R 穿过加载、松开再按、完全不按 R。任何硬件输入都不能提前推进屏障。还要测试延迟队列中的 ready 不触发暂停菜单重开，以及正常游戏的 R 功能不退化。

## 5. 问题 C：加载线程使同一帧被重新采样

### 代码位置

- `GameManager.cpp:683` 启动 `GameplaySetupThread`；第 724 行由加载线程调用 `BeginGameplay`。
- `BeginGameplay`，`MultiPlayerCoordinator.cpp:439`：无条件清除 `capturedSimulationFrame`。
- `AcquireGameplayInputs`，第 564–568 行：先采样并标记，再尝试取得双方输入。
- `MultiNetSession::CaptureLocalInput`，第 592–593 行：输入历史拒绝插入时直接返回 false。
- `MultiNetInputHistory::Insert`，`MultiNetProtocol.cpp:238–247`：同帧只接受相同按钮。
- `Global.cpp:171`：执行回调前释放链表临界区。因此，链表锁并未保护这些协调器字段。

### 可复现交错

```text
主线程：networkFrame=20，采样按钮=1，存入未来帧23；远端20帧尚未到，返回false。
加载线程：BeginGameplay(false, ...) 清除 capturedSimulationFrame。
外部事件：远端20帧到达；玩家已松开按钮。
主线程：再次采样20帧，试图给已存在的未来帧23写入按钮=0。
输入历史：拒绝同帧不同按钮。
主线程：提前返回，根本没有消费已经到达的远端20帧。
```

测试输出：

```text
before worker: result=0 frame=20 capturedFrame=20 storedButtons=1
after worker BeginGameplay: capturedFrame=4294967295
remote arrived, key released: result=0 repeat=0 frame=20 conflicts=2
restore original key: result=1 frame=21
control without worker reset: result=1 frame=21 conflicts=0
```

这是条件性卡帧：恢复原采样的按键组合可以继续，不是已证明的永久死锁。该失败路径未设置 error=7。

### 建议修改方向

建立明确所有权：网络帧、已捕获标记、输入提交和 ready 状态转换尽量由主线程管理；加载线程发布完成结果，由主线程在定义好的边界处理。

也可以先做局部修复：保持当前网络帧的“已提交输入”事实，不让关卡生命周期清除它。下一次采样只能发生在网络帧真正推进之后。检查 `BeginGameplay`、`PrepareGameplay`、`EndGameplay` 等所有写入处，不只删除一个赋值便结束。

关键注意事项：

- 仅加互斥锁不一定解决：即使两个调用完全串行，BeginGameplay 在两次取输入之间清标记，仍会出现同样的逻辑错误。
- 保留原已提交样本后，若本地刚加载完成，ready 应随之后尚未提交的输入帧发送，不要回写已提交帧。
- 不要修改 Insert 使其允许覆盖同帧不同按钮；旧值可能已经发给另一端，覆盖会把卡住问题变成真正的不同步。
- 不要重置网络帧或粗暴清空输入历史来处理换关，这会破坏在途样本及哈希编号。
- 如涉及跨线程发布，用项目工具链支持的同步方式；仅增加 volatile 不能替代线程同步和状态机设计。

### 验收

固定交错测试必须覆盖：采样后远端缺包 → BeginGameplay → 松键/换键 → 远端到达。要求不再重新提交该帧，原帧能顺利消费，新按键留给后续帧。添加远端未缺包、没有 BeginGameplay、按钮不变的对照组，并验证在途输入、重传和多次换关。

## 6. 尚未证实的线索，不计作第四个已确认问题

`EnemyManager.cpp:1634–1635` 用 RNG 初始化掉落计数与序列索引，后置种子重置不会重建它们，当前哈希也不包含这两个字段。但 `GameManager.cpp:719–725` 在初始化前已经统一种子。

本次读取本地数据发现以下文件无 ANM 随机指令：`nowloading.anm`、`ascii.anm`、`capture.anm`、`text.anm`、`player00–03.anm`、各关 `stg*bg.anm`。`etama.anm` 有随机脚本，但与弹幕模板初始化使用的脚本索引无交集。这些是本次本地文件的检查结果，不是对所有修改版资源的保证。

尚未复现“相同资源和游戏条件下，仅加载快慢就导致不同掉落状态”。不要据此盲目增加每关随机数重置、重建掉落序列，或把它写进修复已完成清单。

## 7. 现有探针与验证限制

原工作区的辅助文件位于 `.analysis/desync-audit-20261017/`（通常被 Git 忽略，单独传本文时不会自动包含）：

- `reproduce.py`：抽取当前源码中的取输入和 ready 判断，生成 `coordinator-boundary-probe.cpp` 与 `build.cmd`，验证 A/B。
- `reproduce_capture_race.py`：抽取 BeginGameplay 等函数，并链接真实 `MultiNetProtocol.cpp`，生成 `capture-race-probe.cpp` 与 `build-race.cmd`，验证 C。
- `inspect_anm_rng.py`：只读本地 DAT，检查动画随机指令；不保存或分发原版资源。
- `loading-followup.md`：两条加载线索的详细复核记录。

这些探针使用引擎/网络替身和本机 MSVC x64，不是 VC7 完整游戏或真实双机运行。C 用固定交错模拟调度，没有统计实际触发率。探针当前以“成功重现旧缺陷”为成功条件，不能直接当作修复后的绿色回归测试；修复时需改为验证上述正确行为，并加入真实调用链集成检查。

旧的几个测试 EXE 在本次环境启动时遇到系统 DLL 重定位错误；VC7 编译器也未成功启动。本次没有重新跑通完整旧回归套件。不要沿用历史文档里的 PASS 作为本次修复验证结果。

本地 DAT SHA256 为 `9d7edf43b8ddd347cbb641836f6b5050745dd936f688daebbf9382ca557043bb`，仅标识此次分析输入，不代表已核验为官方 DAT 哈希。原版目标 EXE 的大小与仓库规定 SHA256 已核验。

## 8. 建议实施顺序与交付要求

1. 重新确认三个缺陷仍存在，先建立失败测试，清晰定义本轮输入/模拟帧的语义。
2. 修复 C 的已提交输入生命周期，再处理 B 的控制信号隔离和 A 的启动边界；三者相互影响，应联合回归。
3. 若更改协议格式，更新协议版本及启动器/游戏兼容门槛；即使格式不变，只要影响确定性模拟时序，也应避免新旧实现混联。显示版本名如何处理另行遵循项目要求。
4. 跑适用的状态、协议、会话及协调器测试，正常/联机编译和仓库要求的检查。完整引擎验证应含一快一慢加载、丢包/延迟、按键跨加载、连续多关、重开，以及原来的本地/单人模式。
5. 保留诊断能力：有条件记录关卡/加载代次、网络帧、游戏帧、ready 状态、RNG 同步发生帧及输入提交状态。不得因日志出现 error=7 就自行推断具体差异字段。
6. 最终报告分别说明：改了什么、哪些隔离测试通过、哪些真实双机测试完成、仍未确认的故障。不能宣称“所有少女祈祷中闪退已修好”。

可给后续模型的任务：**按本文修复 A/B/C，保持已提交输入不可变及双方模拟边界一致，遵守仓库规则；不要把第 6 节的未证实线索当成必修 bug。完成测试并如实报告验证范围。**

## 9. 已实施的本地修复（2026-10-17）

本节优先于前面的修复建议和旧探针运行限制。修复仅提交本地 Git，不发布、不推送，也未安装到原游戏目录。

### A：统一首帧边界

`AcquireGameplayInputs` 根据本次消费的网络帧决定是否模拟，并将结果保存为 `gameplaySimulationReady`。`IsGameplaySimulationReady` 只返回这个已确定的结果；`networkFrame++` 不会自行放行下一帧。

首次模拟的种子重置发生在本帧哈希采样之前，哈希与游戏帧计数使用同一个模拟决定。缺包重试继续使用该帧已确定的决定。关卡生命周期变化后，旧代次的待消费帧不能打开新关卡模拟门。

### B：控制信息与按键完全分开

每个输入样本新增 `u16 control` 和 `u32 gameplayEpoch`。控制类型是无、标题就绪或关卡就绪，独立于 `buttons`；不再借用 R 键或 `0x8000` 按键位。只有工作线程已发布完成且设置确认后，才发送关卡就绪。

进入每次关卡加载时增加代次；双方只接受当前代次的关卡就绪信号。所有控制字段随对应输入一起进入不可变历史、冗余重传和消费流程。控制字段不交给游戏按键处理，因此延迟到达的 ready 不会触发重开；正常 R 键仍保留。编码和解码拒绝未知控制类型及错误代次组合。

线上协议升为 `0x00010004`，游戏指纹与启动器兼容门槛升为 `0x00040004`。修复初始提交沿用 v0.42 显示名，随后已整理为 v0.43 本地补丁，见 [v0.43 打包记录](MULTIPLAYER_V0_43.md)。**双方必须使用本次构建，不能与之前的 v0.42 混联**。输入样本从 6 字节变为 12 字节，最多 15 个样本的包为 216 字节，仍小于 512 字节收包缓冲区。

### C：主线程拥有生命周期与输入提交状态

`BeginGameplay` 改在 `GameManager::AddedCallback`、启动工作线程前调用。`PrepareGameplay`、`BeginGameplay`、`EndGameplay` 均不再清除 `capturedSimulationFrame`；只在输入成功消费或会话关闭时清除。

工作线程在最后一次游戏状态写入后调用 `NotifyGameplaySetupComplete`，用 Windows Interlocked 操作发布完成。主线程通过 Interlocked 读取；不以裸 `volatile` 代替同步。保留“同帧内容不同则拒绝”的规则，并扩展到控制字段和代次，未清空网络帧或输入历史。

### 回归测试与重跑方式

新增正式测试 `tests/MultiPlayerCoordinatorTests.cpp` 和引擎适配头 `tests/support/MultiPlayerCoordinatorTestRuntime.hpp`，编译的是实际协调器、会话、协议和玩家状态源码；只替代 D3D、设备及游戏全局对象。固定调度覆盖延迟 1/3/12、P1/P2 首帧边界、未完成加载、未确认设置、R 键、旧关卡及旧标题信号、ready 在途尾部、主线程生命周期变化和实际工作线程完成通知、本地双人完成门。协议测试覆盖控制字段往返、畸形包、不可变样本及丢包冗余；会话测试用真实本机 UDP 传递控制字段。

另有双协调器联动测试：两个实际 UDP 会话各自保存引擎适配状态，延迟分别取 1/3/12，连续两次加载并交换快慢端，P1 跨加载持续按 R。每次加载后双方执行 40 个模拟 tick，检查首个模拟网络帧、每个 tick 的 RNG 和游戏帧计数一致，且会话未出现不同步。该测试通过；它使用引擎适配状态，不等同于真实双机完整 D3D 游戏实测。

```powershell
python scripts/build.py --build-type multi -j 1 build/th08-multi.exe build/th08-multi-launcher.exe multiplayer-tests
$tests = Get-ChildItem build -Filter 'multiplayer-*-tests.exe' | Sort-Object Name
foreach ($test in $tests) {
    & $test.FullName
    if ($LASTEXITCODE -ne 0) { throw $test.Name }
}
```

当前 Windows 环境的可用 Python 是 `C:/Users/a1851/anaconda3/python.exe`；旧版 32 位编译器和真实 UDP 测试须在正常 Windows 权限环境运行。先前的 DLL 重定位和回环丢包来自沙箱限制，移到沙箱外后可运行。VC7 联机游戏与启动器构建、全部 8 组联机回归测试通过；新增测试也已接入 Ninja 的 `multiplayer-tests` 和 Windows CMake/CTest。

### 验证限制和已有基线问题

- 尚未完成真实双机从游戏内连续跨关、快慢机器、实际网络丢包及长时间运行；回归测试不能证明用户反馈的所有 error=7 已消失。哈希及 error=7 诊断保留。
- 普通单人整包构建被已有问题阻断：`EnemyManagerUpdate.cpp` 无条件调用 `ValidateTrailState`，声明却只在 `TH08_MULTI` 中存在。该两文件本次未改动。
- `GameManager.obj` 的 42 个原版比较单元中 41 个精确通过；`InitializeScoreData @ 0x0043BBE1` 因现有生成字符串的 COFF 符号与清单不同而未通过。另将原 HEAD 的 `GameManager.cpp` 单独编译，与修改后的普通单人对象比较：45 个有函数定义的函数，机器码与重定位符号全部相同。没有修改比较清单，也不宣称整库精确比较通过。
- 原 `scripts/ci.py` 在 shell 检查处因本机 WSL 缺少 `/bin/bash` 中断。仅在忽略的分析脚本中跳过这一项，并将临时文件目录设在 `build/` 后，其余原 CI 检查全部通过；正式 CI 脚本未改动。`git diff --check` 通过。
- 第 6 节掉落随机状态线索仍未证实，本轮没有扩展修复。

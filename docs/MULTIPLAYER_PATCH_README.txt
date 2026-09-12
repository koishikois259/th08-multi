th08-multi v0.31 联机补丁
========================

适用版本：东方永夜抄 日文原版 1.00d。

GitHub 源码：https://github.com/koishikois259/th08-multi
v0.31 发布页：https://github.com/koishikois259/th08-multi/releases/tag/v0.31

安装：
1. 将补丁压缩包内的所有文件直接解压到原版游戏目录。
2. 保留原版 th08.exe、th08.dat 和 thbgm.dat；补丁不会覆盖原版 th08.exe。
3. 双方都双击 th08-multi-launcher.exe。
4. 启动器会显示 Local IPv4 信息，不设 Bind local IPv4 输入栏。主机选择
   Host；客机选择 Guest，并填写主机的局域网/VPN/内网穿透地址。
5. 两边使用相同的 Input delay，然后分别点击 Connect。
6. 两边均显示 Connected 后，由 Host 点击 Start both games。Guest 不能
   单独启动，两边游戏会由 Host 的指令同步启动。

本补丁仅适合互相信任的玩家，默认使用 UDP 17708。协议没有身份认证和
加密。启动器会自动监听全部本地 IPv4 接口；如需 Windows 防火墙规则，
请只允许预期使用的网络，并尽量限制为对方地址。

双方必须使用完全相同的 v0.31 补丁和日文原版 1.00d 数据。联机模式
不支持 Replay。连接与异常状态会显示在游戏窗口标题栏，诊断信息写入
游戏目录的 log.txt。

v0.31 修复内容：
- 修复 4、5、6、EX 面对话及符卡间对话的立绘轨迹残影。
- 加固对话立绘、ANM、P1/P2、子 ECL 和换面资源的释放所有权，降低
  后三面换面或对话期间因悬空指针、重复释放导致崩溃的风险。

卸载：关闭游戏后删除以下补丁文件即可，原版游戏不受影响：
- th08-multi.exe
- th08-multi-launcher.exe
- th08_multi.ini
- th08_multi.ini.example
- th08-multi-README.txt
- th08-multi-LICENSE.txt
- SHA256SUMS.txt

th08-multi v0.22 联机补丁
=======================

适用版本：东方永夜抄 日文原版 1.00d。

GitHub 源码：https://github.com/koishikois259/th08-multi
v0.22 发布页：https://github.com/koishikois259/th08-multi/releases/tag/v0.22

安装：
1. 将补丁压缩包内的所有文件直接解压到原版游戏目录。
2. 保留原版 th08.exe、th08.dat 和 thbgm.dat；补丁不会覆盖原版 th08.exe。
3. 双方都双击 th08-multi-launcher.exe。
4. 双方在 Bind local IPv4 填写各自准确的局域网/VPN 地址；主机选择
   Host，客机选择 Guest 并填写主机在同一可信网络中的 IPv4 地址。
5. 两边使用相同的 Input delay，然后分别点击 Connect。
6. 两边均显示 Connected 后，由 Host 点击 Start both games。Guest 不能
   单独启动，两边游戏会由 Host 的指令同步启动。

本补丁仅支持双方互相信任的私有局域网或可信 VPN，默认使用 UDP
17708。协议没有身份认证和加密，不支持直接公网联机。不要设置路由器
端口转发，不要使用 0.0.0.0 绑定；如需 Windows 防火墙规则，只允许
专用网络配置文件，并尽量限制为对方的局域网/VPN 地址。

双方必须使用完全相同的 v0.22 补丁和日文原版 1.00d 数据。联机模式
不支持 Replay。连接与异常状态会显示在游戏窗口标题栏，诊断信息写入
游戏目录的 log.txt。

卸载：关闭游戏后删除以下补丁文件即可，原版游戏不受影响：
- th08-multi.exe
- th08-multi-launcher.exe
- th08_multi.ini
- th08_multi.ini.example
- th08-multi-README.txt
- th08-multi-LICENSE.txt
- SHA256SUMS.txt

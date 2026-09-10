th08-multi v0.1 联机补丁
=======================

适用版本：东方永夜抄 日文原版 1.00d。

安装：
1. 将补丁压缩包内的所有文件直接解压到原版游戏目录。
2. 保留原版 th08.exe、th08.dat 和 thbgm.dat；补丁不会覆盖原版 th08.exe。
3. 双击 th08-multi-launcher.exe。
4. 主机选择 Host；客机选择 Guest 并填写主机 IPv4 地址。
5. 两边使用相同的 Input delay，然后点击 Save and launch。

局域网默认使用 UDP 17708。公网联机时，主机需要在防火墙放行该 UDP
端口，并在路由器上把该端口转发到主机的局域网 IPv4 地址。v0.1 不含
中继、匹配服务器、自动端口映射或 NAT 穿透。

双方必须使用完全相同的补丁版本和日文原版 1.00d 数据。联机模式初版
不支持 Replay。连接与异常状态会显示在游戏窗口标题栏，诊断信息写入
游戏目录的 log.txt 和 th08_multi_*.log。

卸载：关闭游戏后删除以下补丁文件即可，原版游戏不受影响：
- th08-multi.exe
- th08-multi-launcher.exe
- th08_multi.ini
- th08_multi.ini.example
- th08-multi-README.txt
- th08-multi-LICENSE.txt
- SHA256SUMS.txt

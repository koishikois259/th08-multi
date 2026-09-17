th08-multi v0.33 联机补丁
========================

一、项目性质与权利声明
----------------------

th08-multi 是基于《东方永夜抄 ～ Imperishable Night》的非官方二次创作
联机项目，与上海爱丽丝幻乐团、ZUN 及任何官方发行方均无隶属、授权或
合作关系，请勿将本项目误认为官方作品或官方更新。

“东方Project”、《东方永夜抄》及其角色、美术、音乐、文字和其他原作内容
的权利归原权利人所有。本项目不授予用户复制或传播原版游戏内容的权利。
使用者必须自行合法取得《东方永夜抄》日文原版 1.00d。

东方Project 二次创作指南：
https://touhou-project.news/guidelines_en/

二、支持版本
------------

仅支持《东方永夜抄》日文原版 1.00d：
- th08.exe 文件大小：840704 字节
- th08.exe SHA-256：
  330fbdbf58a710829d65277b4f312cfbb38d5448b3df523e79350b879213d924

汉化版、体验版、其他版本或经过修改的游戏数据不在支持范围内。
联机双方必须使用完全相同的 v0.33 补丁和相同版本的原版游戏数据。

三、补丁内容与分发边界
----------------------

本补丁压缩包只包含：
- th08-multi.exe
- th08-multi-launcher.exe
- th08_multi.ini.example
- th08-multi-README.txt
- th08-multi-LICENSE.txt
- 联机教程.txt
- SHA256SUMS.txt

补丁不包含原版 th08.exe、th08.dat、thbgm.dat、音乐、美术素材、存档或
Replay。请勿将上述原版文件、从原作提取的素材或他人的存档重新打包进
本补丁，也不要发布包含这些内容的整合游戏包。

SHA256SUMS.txt 用于核对补丁内部各文件。整个 ZIP 的 SHA-256 以对应
GitHub Release 页面公布的值为准。

四、安装与启动
--------------

1. 备份自己的存档和配置。
2. 将补丁压缩包内全部文件解压到合法取得的日文原版 1.00d 游戏目录。
3. 保留原版 th08.exe、th08.dat 和 thbgm.dat；补丁不会覆盖 th08.exe。
4. 双方分别运行 th08-multi-launcher.exe。
5. 启动器会显示 Local IPv4，不提供 Bind local IPv4 输入栏。主机选择
   Host；客机选择 Guest，并填写主机的局域网、可信 VPN 或受控隧道地址。
6. 双方设置相同的 Input delay，然后分别点击 Connect。
7. 双方均显示 Connected 后，由 Host 点击 Start both games；两侧游戏
   会在同一启动指令后进入游戏。

GitHub 源码：
https://github.com/koishikois259/th08-multi

v0.33 发布页：
https://github.com/koishikois259/th08-multi/releases/tag/v0.33

五、网络与安全限制
------------------

本项目仅面向互相信任的两名玩家在可信局域网、可信 VPN 或具有访问控制
的临时内网穿透环境中使用，默认端口为 UDP 17708。协议不提供身份认证、
数据加密、抗重放或恶意对端防护，不应直接暴露给公共互联网，也不要与
不可信玩家连接。

启动器会监听全部本地 IPv4 接口。Windows 防火墙规则应限制在预期网络
和可信对端；使用内网穿透时应采用临时映射，只向对方提供连接信息，并在
测试结束后关闭映射。连接地址、外网 IP 和隧道端口也可能属于隐私信息。

六、功能范围与已知限制
----------------------

- P1、P2 独立选择队伍，允许选择相同队伍。
- 联机模式暂不支持 Replay。
- 连接及异常状态显示在游戏窗口标题栏，诊断信息写入游戏目录 log.txt。
- 本项目仍处于测试阶段；请保留存档备份，并优先在可信环境测试。

v0.33 主要修复：
- 联机时由 P1 的通关进度决定路线、Extra 解锁和其他存档进度条件，
  P2 的本地通关进度不参与本次联机。
- 只同步经过边界校验的进度快照，不传输原始 score.dat；P2 的磁盘存档不会被
  P1 存档覆盖。
- 双方均进入主菜单后才解锁 P1 的菜单操作，加载期间提前按下的按键会被清除。
- 联机模式禁用主菜单自动 Demo，因为本版仍不支持 Replay。
- 在双方完成关卡加载后的共同模拟起点重新统一 RNG，避免加载速度或存档不同
  导致随机弹幕分叉和 network error 7。
- 状态校验改用联机会话内单调递增的网络帧标识，避免不断线再开一局时命中上一局的
  旧校验记录。

v0.33 包含 v0.32 的全部内存安全防护。仍建议在可信环境中对 4、5、6、EX 面进行
长时间双机测试。

七、源码、许可证与致谢
----------------------

项目源码依据仓库根目录 LICENSE 中的 MIT License 发布。MIT License 仅
适用于项目贡献者有权授权的源码与改动，不适用于东方Project 原作、原版
游戏文件或其他第三方素材。补丁内附 th08-multi-LICENSE.txt；复制、修改
或再分发源码时必须保留许可证和版权声明。

本项目基于以下公开项目和设计参考：
- TH08 源码重构：https://github.com/N0zoM1z0/th08
- th06_multi_net：https://github.com/RUEEE/th06_multi_net
- Microsoft Detours：https://github.com/microsoft/Detours

感谢原项目作者、历史贡献者及测试人员。

特别感谢以下合作者参与开发与测试：
- HDZsuper：https://github.com/HDZsuper
- nmmuwu：https://github.com/nmmuwu

他们在测试和开发方面提供的帮助改善了本版本的稳定性与完整性。各第三方
项目仍适用其各自许可证。

本项目的部分开发、调试和代码审查工作由 OpenAI Codex 协助完成。

八、问题与安全报告
------------------

一般 Bug 可通过 GitHub Issues 报告。报告崩溃时请提供版本、关卡、复现
步骤和异常代码；上传日志、截图或转储前请删除外网 IP、用户名、个人路径
等隐私信息。严禁上传 th08.exe、DAT、音乐、美术素材、存档或其他原版
游戏内容。

安全问题不要公开披露。如仓库已启用 GitHub Private vulnerability
reporting，请使用该入口；否则请先私下联系维护者，不要在公开 Issue 中
发布利用细节。项目按 MIT License “AS IS” 条款提供，不
承诺无错误、无崩溃或适合任何特定用途。

九、卸载
--------

关闭游戏后，删除以下补丁文件即可；原版游戏文件不受影响：
- th08-multi.exe
- th08-multi-launcher.exe
- th08_multi.ini
- th08_multi.ini.example
- th08-multi-README.txt
- th08-multi-LICENSE.txt
- 联机教程.txt
- SHA256SUMS.txt

如需保留联机设置，请在卸载前备份 th08_multi.ini。

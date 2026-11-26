# UniXNU elish UEFI 移植交接

## 当前交接 · run81 触控 · 2026-11-26

- run84 真机空闲测试到达 Hello，随后自动回到 bootloader fastboot。放大版只读查看器回读 `stage=7`、HID `stage=5 match=1`，边沿、读取和报告均为 0。下一轮复用 run84，录下实机上滑，并用同一查看器回读计数。[镜像和实机证据](reports/elish-run81-touch-20261126.md#event-acquisition)
- run83 真机已完成 SE4 初始化并匹配 IOHIDEventDriver。40 次固定间隔读取均被解码拒绝，随后扫描停止。后续候选按原厂 GPIO39 上升沿读取 79 字节事件，保留前 65 字节手指数据的校验。[读帧调整与实机记录](reports/elish-run81-touch-20261126.md#event-acquisition)
- 后续 ramoops 回读拍到 `stage=2`，失败发生在 QUP 时钟等待。已按 MiCode Kona 分支定义修正从 AHB 的硬件门控判断，下一版诊断同时记录投票寄存器和三条分支状态。[时钟修正与验证](reports/elish-run81-touch-20261126.md#qup0-clock-admission)
- 首版只读 UEFI 查看器在内屏显示了文本，录像分辨率不足以读取寄存器。其 PSCI 复位后没有枚举 fastboot，内屏随后出现已安装系统的苹果标志。新版查看器改用 PMIC 热复位。触控候选增加 420 秒终点快照，可从内屏读取 SE4 寄存器、触点和 HID 计数。设备需回到 bootloader fastboot 才能上机验证。[构建和回读步骤](reports/elish-run81-touch-20261126.md#deadline-snapshot-and-ramoops-reader)
- run80 保留在 `boot_b` 作为恢复基线。新镜像只通过核验过的 bootloader `fastboot -s 4bed07e4 boot` 送入 RAM；不使用 `continue`，不写 boot、Data 或其他持久分区。
- 已确认工厂 fw01 的精确字节并实现 opt-in transport/解析/HID 路径。在 XNU 读到 QUP S4 已载入 SPI 且启用 FIFO 前不发 Novatek 命令；触控不切换面板电源或 GPIO75。
- run81e 已通过封包读回及 21H461 HID 链接检查。19:20:39 CST 核验设备身份后发出 RAM 启动，fastboot 两阶段均返回 `OKAY`。连续录像写入 `run81e-touch-hid/device-attempt.LumUbu/camera.mkv`。录像 `05:46`–`05:56` 拍到 Hello 内屏上滑尝试，问候动画继续，未打开语言列表。19:31:00 CST 已自动回到核验过的 bootloader fastboot。暂停启动其他镜像，先用独立固件镜像回读本轮日志；旧 Lineage reader 会尝试读写挂载，不能作本轮回读。后续候选默认 420 秒；镜像哈希、路径及命令见 [run81 报告](reports/elish-run81-touch-20261126.md)。

## 最新交接 · run80 · 2026-11-26

- 活动槽 `boot_b` 已安装 run80，平板经 Recovery 的 `adb -s 4bed07e4 reboot` 正常启动到原版 Hello。约 457 秒的录像仍显示问候语。[内屏照片](reports/elish-run80-installed-hello-20261126.jpg)
- 写入并读回核验了 UFS LUN 4 LBA `393654–442805`。分区和整盘读回 SHA-256 均为 `d4dceb7aa1ec16a51e828ca6c983efe513fba1e1e152808545f55f09da505b2c`。原槽备份、相邻块和 GPT 校验见 [run80 报告](reports/elish-run80-installed-hello-20261126.md)。
- run80 复用 run79 的内核、ADT、RootPolicies、信任缓存和 RAM Data seed，移除了终止快照和定时重启参数。触控、语言选择、Setup 完成、SpringBoard 主屏幕和 Data 持久化仍待验证。

## 最新交接 · run79 · 2026-11-26

- **原版 Hello 已验收。** `run79-key-store/boot.img` SHA-256 `15d21e8dc7c1e902e2aba21792c1bd938cab457db640c34fcc26e0e09fa23b7d`。`device-attempt.89vwTy/camera-035.mkv` 约 4 秒处可见英文 Hello 和 `Swipe up to open`，下一段拍到问候语切换。约 382.55 秒后回到已核验身份的 bootloader fastboot。镜像、录像、构建指令与源码版本见 [run79 报告](reports/elish-run79-key-store-20261126.md)；[Hello 照片](reports/elish-run79-hello-20261126.jpg)。
- run79 复用 run56 的只读 UFS System、SystemOS、RootPolicies 与 Apple trust cache，以及 run76 的未配置 RAM Data seed（UUID `7D443C28-95AC-49F8-AF25-4FC12FBC93E5`）。`elish-aks=1` 在可写 Data 挂载后发布 `AppleKeyStoreState`。这轮录像未读出终止快照的键存储计数和 PID。
- 下一轮可用独立目录复制 run79 并只改启动参数，移除 `elish-userspace-snapshot=360`，把 `elish-ram-only` 延长到足够取证的有限时间。保留 warm reset 和其余输入。先复现 Hello，再拍摄真机上滑到语言选择的可见响应。触控、语言选择、Setup 完成、SpringBoard 主屏幕和 Data 持久化仍未验证。
- 每次上机核对 `18d1:d00d`、`4bed07e4`、`product=elish`、`is-userspace=no`。只用 `fastboot boot`，禁止 `fastboot continue`。run79 首次传输卡在 `Sending`，USB 枚举不代表可响应命令，须重新通过全部身份检查。Data 窗口未经写入授权。[备份与恢复预检](reports/elish-run54-ufs-data-readonly-20261126.md#backup-and-rollback-preflight)

## 最新交接 · run78 · 2026-11-26

- `run78-session-breakpoint/boot.img` SHA-256 `6d5edad9d82b89c633b46cd47d28b8a78149e7dfd1552ac602dbb8b3b4b19ccf`。160 秒采样捕获 `SessionSyncEngine` 内的 `BRK #1`，紧随 `MKBEventsRegister` 返回空句柄。`Setup=0`，约 183 秒回到已核对身份的 bootloader fastboot。run79 复用 MatePad 11 采用的 AppleKeyStoreState。[实机记录](reports/elish-run64-run65-gop-20261126.md#run78)

## 最新交接 · run77 · 2026-11-26

- `run77-session-daemon/boot.img` SHA-256 `960a69a946b9a2566ecb2c3f2ea2f50daa8127de11e003e873d7a8288e3d995e`。`chronod` 等待 ActivityKit 同步调用，`liveactivitiesd` 已退出 15 次，末次状态 `0x5`，`Setup=0`。约 383 秒回到已核对身份的 bootloader fastboot。[实机记录](reports/elish-run64-run65-gop-20261126.md#run77)

## 最新交接 · run75 / run76 · 2026-11-26

- `run75-chronod/boot.img` SHA-256 `d4c25473abdbeedd06aa090971fb888d0e4acb8dc0ff0d3e996a36a1548e87b9`。`chronod` PID 98 有同步 XPC 等待的线程，workqueue 待处理请求为 0。`pkd` PID 384，`Setup=0`。约 383 秒回到已核对身份的 bootloader fastboot。[实机记录](reports/elish-run64-run65-gop-20261126.md#run75)
- `run76-mobile-tmp/boot.img` SHA-256 `caa454b9869b6f04fd450d466b68fc6f61b207fb244dfb221af02119cc5f4c48`。只读重挂载核对 RAM Data 的 `mobile/tmp` 属主已改为 `501:501`。360 秒时内屏显示苹果标志，`Setup=0`，随后回到已核对身份的 bootloader fastboot。[实机记录](reports/elish-run64-run65-gop-20261126.md#run76)

## 最新交接 · run74 · 2026-11-26

- `run74-springboard-deep-stack/boot.img` SHA-256 `99f00245102b7b84c9455913cbd20b58a7a93efcc0fe7efc871aff63241056b2`。360 秒的 SpringBoard 深栈定位到 `SBHIconManager` 初始化时同步订阅 ChronoServices 扩展。内屏保持苹果标志，`Setup=0`，约 383 秒后返回已核对身份的 bootloader fastboot。下一轮采样提供该服务的 `chronod`，并记录 `pkd` PID。Hello、主屏幕、触控、Setup 完成和 Data 持久化待验收。[实机记录](reports/elish-run64-run65-gop-20261126.md#run74)

## 最新交接 · run73 · 2026-11-26

- `run73-springboard-stack/boot.img` SHA-256 `b7ef0044def312d25656d9b638cf2829364d45e3fab51ff68ca70b86c93c4b63`。采样确认 `launchd`、`backboardd`、SpringBoard、可写 RAM Data，及 6 次 GOP 交换完成。SpringBoard 在 BoardServices 同步 XPC 调用中等待。相机拍到苹果标志和诊断，约 383 秒后返回 bootloader fastboot。Hello、主屏幕、触控、Setup 完成和 Data 持久化待验收。[实机记录](reports/elish-run64-run65-gop-20261126.md#run73)

## 最新交接 · run72 · 2026-11-26

- `run72-workqueue-admission/boot.img` SHA-256 `21f4e946a6b6b3d41afe589b2313f20a533154a0853a48392287312ff8f8ffee`。FAT payload 读回确认八个 CPU 节点的 E/P 类型。mobile `lsd` 的待处理请求为 0，快照确认 `launchd`、`backboardd`、SpringBoard 和可写 RAM Data。录像记录苹果标志和诊断，约 383 秒返回已核对身份的 bootloader fastboot。Hello、主屏幕、触控、Setup 完成和 Data 持久化仍待验收。[实机记录](reports/elish-run64-run65-gop-20261126.md#run72)

## 最新交接 · run71 · 2026-11-26

- `run71-lsd-workqueue/boot.img` SHA-256 `37892cb8971345c4618f2bdaac5da3c386fabc4f8677dad24a01dd2bbe34bc18`。mobile `lsd` 的 workqueue 快照显示 `total 1 idle 1 req 5 done 15 F10 C0`。内屏没有出现要求验收的界面，设备约 292 秒后回到 bootloader fastboot。run72 的 SoC 本地覆盖补齐八个 CPU 的 E/P 分类。[实机记录](reports/elish-run64-run65-gop-20261126.md#run71)

## 最新交接 · run70 · 2026-11-26

- `run70-preferences-wait/boot.img` SHA-256 `1931f57170b688a8f82f085480278a60d4c29040291885183e1ad6e08a7d6135`。mobile `lsd` PID 123 的唯一非空闲 workqueue 样本停在 `_LSPreferences` 同步等待。内屏显示苹果标志和诊断，设备约 292 秒后自动返回 bootloader fastboot。Setup 界面、触控和 Data 持久化待验收。[实机记录](reports/elish-run64-run65-gop-20261126.md#run70)

## 最新交接 · run69 · 2026-11-26

- `run69-launchservices/boot.img` SHA-256 `43f174a6cfd93d18e2ee94b1bc852e69d9542a7f49ffc517b08f3129774168ba`。270 秒诊断拍到 `lsd` 在 `_LSPreferences::with` 的同步队列等待。内屏在诊断前显示苹果标志，随后约 293 秒返回 bootloader fastboot。Setup、触控和 Data 持久化待验收。[实机记录](reports/elish-run64-run65-gop-20261126.md#run69)

## 最新交接 · run68 · 2026-11-26

- `run68-userspace-stacks/boot.img` SHA-256 `e3f3fc67400e9933b26a8675b23d9c4e9945447d0186bff5ae7e716f8d4048c2`。录像记录苹果标志和 270 秒诊断。SpringBoard 等待 LaunchServices 存储，backboardd 初始线程已进入 `CFRunLoop`，采样时没有 Setup 进程。设备约 293 秒后回到 bootloader fastboot。[快照和栈解析](reports/elish-run64-run65-gop-20261126.md#run68)

## 最新交接 · run67 · 2026-11-26

- `run67-longer-ui-window/boot.img` SHA-256 `962ac89be5f5d53822515f756e924399a990d9d11278dbbdb03510ce527fe944`。诊断记录 GOP 4 次拷贝、4 次交换完成、0 次拒绝。苹果标志保留到诊断，采样时没有 Setup 进程。原生 UI、触控、Setup 完成和 Data 持久化待验收。[快照和录像位置](reports/elish-run64-run65-gop-20261126.md#run67)

## 最新交接 · run66 · 2026-11-26

- `run66-gop-copies/boot.img` SHA-256 `052c02930c6fb782d1471b1b9ef879bf7aea5a5fe1a1e674fbe6f8e87b484215`。启动约 191 秒的录像拍到苹果标志。后续诊断记录 `launchd=1 main=1`、`backboardd=57`、`SpringBoard=26`、`Setup=0`。GOP 计数在画幅外，约 253 秒返回已核对身份的 bootloader fastboot。run67 将诊断下移并延长观察窗口。[照片和录像位置](reports/elish-run64-run65-gop-20261126.md#run66)

## 最新交接 · run64 / run65 · 2026-11-26

- `run64-gop-cpu/boot.img` SHA-256 `b3956b0d323285d846230fc710545b905a691fc5f6df249c97dcb7ecd576956c`。实机 185 秒快照显示 `launchd=1 main=1`、`backboardd=57`、`SpringBoard=27`、`Setup=0` 和 GOP `stage=10 opens=5`。设备在启动后 207.85 秒回到已核对身份的 bootloader fastboot。录像未见原生 UI，帧数在画幅外。
- `run65-gop-counters/boot.img` SHA-256 `5f78b342a914a5193741dd189069926b1fec36dfd70bad3a495dee888d37709e`。手动重进 bootloader fastboot 后，四项身份核对通过，镜像成功在 RAM 启动，约 253 秒后回到 fastboot。`device-attempt.wJk4m6/camera-024.mkv` 拍到快照，但右侧出画且文字失焦，无法核对计数。下一轮分别记录 IOSurface 拷贝和黑场填充。[构建和实机记录](reports/elish-run64-run65-gop-20261126.md)

## 最新交接 · run63 · 2026-11-26

- `run63-graphics-console/boot.img` SHA-256 `810e53eceb4de2220ac93650edd289efc386784d84ae2d09bff346e0f599479d`。内核、ADT、信任缓存、RootPolicies 和 RAM seed 与 run62 一致。185 秒快照显示 APFS Data 可写、`launchd=1 main=1`、`backboardd=57`、`SpringBoard=0 Setup=0`。设备约 207.84 秒后返回已核对身份的 bootloader fastboot。[录像和构建记录](reports/elish-run63-backboardd-20261126.md)

## 最新交接 · run61 / run62 · 2026-11-26

- `run61-userspace-snapshot/boot.img` SHA-256 `7fbdb2cb35ea96c69456ad5734efa13d64053165b529ab074afa41ae1c7d62ef`。实机在 157.65 秒时重新枚举为 bootloader fastboot，录像止于 124.58 秒，最后显示 pre-MMU。该轮后续 RAM-only reader 枚举为 `18d1:d001`，ADB `unauthorized`，未取回 ramoops。
- `run62-earlier-snapshot/boot.img` SHA-256 `80e7067f4069e92f103cb678cf333bd496e404292aa335e73bb7af78454686a7`。快照确认 `/private/var` 可写、`launchd=1 main=1`、`SpringBoard=26`，采样时 `backboardd=0 Setup=0`。设备约 117.49 秒后回到 bootloader。相机分段覆盖了快照及重启。内屏原生 UI、触控和 Data 持久化还未验收。[本轮报告](reports/elish-run62-data-springboard-20261126.md)

## 最新交接 · run58 · 2026-11-26

- `run58-writable-ram-data/boot.img` SHA-256 `4230b261d836393079f9585341435a9b421a785c6be3e4f9a0e3df1642938177`。保留 run56 的 Apple trust cache 和 RootPolicies，启用 owner boot manifest 和 1 GiB RAM Data。实机画面保留 pre-MMU 标记，约 257 秒后回到 bootloader fastboot。
- run59 保留 seed 并换回 run56 内核，复现 `mount[3] exited`。run60 用 run58 内核去掉 seed。原始 Data seed、卷 UUID、运行命令和录像路径见[本轮报告](reports/elish-run58-writable-ram-data-20261126.md)。

## 最新交接 · run57 · 2026-11-26

- `run57-pid1-continues/boot.img` SHA-256 `3ea8adeec3d7a4b870851175cf925adc8bce5310bef241211cd70dca549d74a5`。沿用 run56 的启动文件，关闭 PID 1 终止检查点，保留 240 秒 RAM-only 定时返回。实机显示 `mount[3] exited` 后回到 bootloader fastboot。本轮没有 Data seed。[实机录像和构建输入](reports/elish-run57-pid1-continues-20261126.md)

## 最新交接 · run56 · 2026-11-26

- `run56-verified-systemos-union/boot.img` SHA-256 `11919eb7908ddec029480e248851041cfe9dc148d8438a470e98eca0cd61466e`。实机确认完整 System 根卷 `/dev/disk3s1`，UUID `04769D26-2CA0-4825-B443-E2E5F88300F2`，SystemOS 只读 union `/dev/disk1s1`，以及 Apple `launchd` PID 1 `main()`。检查点热重启回 bootloader fastboot。[实机记录](reports/elish-run56-verified-systemos-union-20261126.md)
- Setup 未测试。当前主机没有目标 Data seed，标准生成工具需要 macOS `hdiutil`。Data 持久化未测试。空闲窗口备份和回滚尚未验证，尚无该范围的写入授权。[Data 窗口与恢复预检](reports/elish-run54-ufs-data-readonly-20261126.md#backup-and-rollback-preflight)

## 最新交接 · run55 · 2026-11-26

- `run55-verified-system-handoff/boot.img` SHA-256 `40a5b16c6b318abcaf2784aa8ba5f2d94e3fba26ed7872ac7d64fe4c60c0a755`。摘要包走 DarwinBoot `RootPolicies` 交接，XNU 验证后发布只读 OS/SystemOS。[run55](reports/elish-run55-verified-system-handoff-20261126.md)
- RAM-only 录像在 84 秒显示 PID 1 的 dyld 加载失败，随后返回 bootloader fastboot。System UUID、两层 APFS mount、`launchd` `main()`、Setup 和 Data 持久化待验收。

## Data 窗口准备 · run54 · 2026-11-26

- `run54-ufs-data-readonly/boot.img` SHA-256 `b5e5c8df09ef6e5aa5093249bf5cbe9805d96f95d24131a6f920f9033d3d5be2`。实机从 XNU 读到 3 GiB Data 候选窗口首末块，CRC32 均为 `c71c0011`，报告 `stopped=1 faults-same=1`，随后回到 bootloader fastboot。[run54](reports/elish-run54-ufs-data-readonly-20261126.md)
- Data 候选窗口备份长 3,221,327,872 字节。整段独立备份、主备 GPT 和相邻块保护值、恢复验证和写入授权均待补齐。[备份与恢复步骤](reports/elish-run54-ufs-data-readonly-20261126.md#backup-and-rollback-preflight)

## 最新交接 · run53 · 2026-11-26

- `run53-verified-system-layout/boot.img` SHA-256 `2771fd3ab994caa782a7e60f4fadd8502a13e3fde177181b83c3a515a4993cf5`。XNU 编译、预链接、双存储 ABI 和 FAT 检查通过。RAM-only 启动录像在 80 秒显示 pre-MMU 标记，之后返回 bootloader fastboot。[run53](reports/elish-run53-verified-system-layout-20261126.md)
- PID 1 检查点会主动停止。RAM-only Setup 需允许 `launchd` 继续运行并提供逻辑容量不超过 2 GiB 的未配置 Data seed。Data 持久化还需要 elish 的 `native_ufs_store_*` 和经核验的可写窗口，目前两项均未测试。[验收条件](reports/elish-run53-verified-system-layout-20261126.md#hardware-acceptance)

## 最新交接 · run52 · 2026-11-26

- run52 的 UFS 校验镜像实机确认到 pre-MMU。`__TEXT` 中的摘要表改变了早期可执行段布局。run53 将摘要表移至 `__LASTDATA_CONST`。[run52](reports/elish-run52-verified-system-20261126.md)
- 已从 `boot_b` 只读提取同哈希的 RAM-only Lineage reader，位于 `run52-verified-system/boot-b-reader-candidate.img`。启动 reader 后 ADB 显示 `4bed07e4 unauthorized`，未回收 ramoops。

## 最新交接 · run51 · 2026-11-26

- 已离线安装 21H461 前缀，LUN 0 LBA `6619136–8909823`。清缓存后从整盘读回匹配 SHA-256 `fd43ee7035ce185ce4215ac93fdc006fd784a54b3b7442e21457c226cbe03093`。主备 GPT 与 LBA `8909824` 对照通过。原始前缀的已验证恢复备份仍位于 `/root/elish-userdata-prefix-backup.MV0PYNbU/userdata-prefix.img`。
- run51 重用 run50 启动镜像。实机到挂载后的 `FSIOC_KERNEL_ROOTAUTH`，屏幕记录 `rootvp not authenticated after mounting @bsd_init.c:1100`。平板约 20 秒后返回 fastboot，进入 Recovery 后 pstore 为空。实挂系统 UUID 和 Apple PID 1 待验收。[run51](reports/elish-run51-system-root-auth-20261126.md)
- 在内核构建中设置 `UNIXNU_ELISH_ROOT_IMAGE` 为主机 `userdata-root.img` 的路径，生成绑定固定 IPSW 窗口内容的只读摘要表。诊断从 RAM-only 启动，在现有安装上继续读取。

## 最新交接 · run50 · 2026-11-26

- `/tmp/elish-run50-xnu-system-checkpoint-20261126/boot/boot.img` 的 SHA-256 为 `a97b762d32dc5da6fc55c325536432503ffa7967f647db7aff284dc448cbbedd`。实机只读检查在缺少 `SHNGROOT` 时停下，随后自动回到 fastboot。RAM-only reader 的 root ADB 取回 XNU ramoops。[run50](reports/elish-run50-xnu-system-checkpoint-20261126.md)
- run48 的反汇编定位了复位指令截断。run49 修正后 PSCI 自动重启通过，pstore 为空。run50 使用固件已准备的热重启路径，验证日志保留。[run48](reports/elish-run48-xnu-psci-root-20261126.md) · [run49](reports/elish-run49-xnu-psci-asm-20261126.md)
- 完整 System 根卷、Apple `launchd` PID 1、Setup 和 Data 持久化各自待验收。平板留在 `_b` 槽 Recovery。当前离线备份 `/root/elish-userdata-prefix-backup.MV0PYNbU/userdata-prefix.img` 的完整 SHA-256 与设备独立读回一致，主备 GPT 和范围后首块通过校验。首次安装和同范围回滚等待对 LUN 0 LBA `6619136–8909823` 的明确授权。[恢复方案](reports/elish-run50-xnu-system-checkpoint-20261126.md#offline-installation-and-rollback-preflight)

## 最新交接 · run46 / run47 · 2026-11-26

- run46 修正 UPIU LUN 和 Task Tag 偏移后，XNU 自有 DMA 通过容量与七块 CRC 检查。实机屏显示 `reads=7 stopped=1 faults-same=1`。[run46](reports/elish-run46-xnu-ufs-upiu-20261126.md)
- run47 用 `elish-ufs-root=1 elish-ram-only=45` 启动，无 Restore ramdisk。ADT 选择 UUID `04769D26-2CA0-4825-B443-E2E5F88300F2`。屏幕确认两份 GPT 捕获块通过后，XNU 在缺少 `SHNGROOT` manifest 时停止，OS/SystemOS 发布与 APFS mount 待测。run47 未自动回到 fastboot。[run47](reports/elish-run47-xnu-ufs-root-20261126.md)
- 目标镜像和已验证离线备份各为 9,382,658,048 字节。安装和回滚范围均为 LUN 0 LBA `6619136–8909823`。首次写入需用户明确授权，Recovery 下复核备份及两个 GPT 与范围后的首块。[恢复方案](reports/elish-run42-ufs-root-windows-20261126.md#offline-prefix-backup)

## 最新交接 · run45 · 2026-11-26

- RAM-only run45 核验 `4bed07e4`、`product=elish`、`is-userspace=no` 后进入 XNU，通过 SMMU 检查并接管空闲队列。终止屏显示 `READ CAPACITY(10)`、`reads=0 stopped=1 faults-same=1`。run45 将 Task Tag 写到了 LUN 字段，NOP 响应检查也用了错误字节。run46 修正字节 2/3，保留队列与故障守卫，并显示命令完成状态。[实机记录](reports/elish-run45-xnu-ufs-20261126.md)

## 最新交接 · run44 · 2026-11-26

- RAM-only run44 进入 XNU，屏幕定位首个不匹配寄存器 `0x15000000`。32 位 `sCR0` 实测 `0x00230406`、原期望 `0x00270406`。DMA 地址为 0。run43 的 Recovery ramoops 回收超时。run45 对 `MTCFG=0` 的两种实测值均执行后续路由和队列守卫，并对照七个已知 LBA 的 CRC。[run44 证据](reports/elish-run44-xnu-ufs-20261126.md)

## 最新交接 · run43 · 2026-11-26

- `/tmp/elish-run43-xnu-ufs-20261126/boot/boot.img` 的 SHA-256 为 `718bb8e187d3c3fd85823996165ce6dcd04c6b48af6a798756e90e29f9bff65a`。核验 `4bed07e4`、`product=elish`、bootloader fastboot 后 RAM-only 启动。照片确认 XNU 已进入 UFS 探针，在 SMMU 检查处停下，尚无 XNU UFS 命令。run44 细化寄存器诊断。[run43 记录](reports/elish-run43-xnu-ufs-20261126.md)
- `userdata` 前缀备份位于 `/root/elish-userdata-prefix-backup.KGeZtgQ9/userdata-prefix.img`，完整哈希已从独立整盘读取复核。任何写入必须先获得对应 LBA `6619136–8909823` 的授权。完整 System 根卷、Setup 与 Data 持久化尚未验证。[恢复方案](reports/elish-run42-ufs-root-windows-20261126.md#offline-prefix-backup)

## 最新交接 · run42 · 2026-11-26

- `--ufs-root-windows` 从已验证的 LUN 0 GPT 出发读六个目标块。CRC32 与 Android `/dev/block/sda`、`/dev/block/sda34` 的双视图基线一致。设备上的 manifest、OS GPT、OS NXSB 和 SystemOS NXSB 签名均缺失。SystemOS 尾块吻合的内容为全零。
- `/tmp/elish-run42-ufs-root-windows-rebuilt-20261126/boot.img` SHA-256 `5d0be74cc6744aa36543d782404a26f1deef6acf6f770404372b2f5b0aca8192`。两次 RAM boot 前均核验 USB bootloader fastboot、serial `4bed07e4`、`product=elish`。探针完成后队列停止并在 DarwinBoot 前热重启，已由 RAM reader 取回日志。[记录与恢复方案](reports/elish-run42-ufs-root-windows-20261126.md)
- XNU DMA、完整 System 根卷、Setup 和 Data 持久化待实机确认。首轮镜像写入范围拟限制为 LUN 0 LBA `6619136–8909823`。需要先在 `/data` 未挂载的恢复环境完成该前缀的原始备份校验，再请求写入授权。

## 最新交接 · run40 · 2026-11-26

- run40 在 RAM-only elish 启动中复核两条 UFS SMMU 路由，通过数据 PRD `0x9A71B000–0x9A720000` 读取原生 LUN 0。限时 NOP 成功，READ CAPACITY 返回 61,880,320 个 4 KiB 块；已知 LBA 1、主备 GPT 及 header/entries CRC 均通过，`userdata` LBA 边界与 Android 基线一致。
- 命令完成后 UFS 队列停止、doorbell 为 0，GFSR/FSR2/FSR3 未变化。探针在 DarwinBoot 前 warm-restart，仅发 NOP、READ CAPACITY 和 READ(10)，未写分区。[run40 记录与镜像哈希](reports/elish-run40-ufs-gpt-20261126.md) · [console ramoops](reports/elish-run40-ufs-gpt-console-ramoops-20261126.txt)
- run40 证明的是 UEFI 的只读 UFS/SMMU/DMA 路径；XNU 尚未接管 UFS，也未读取 OS/SystemOS 或挂载完整 System 根卷。`--ufs-run-kernel` 诊断入口只保留路由并进入 DarwinBoot，不提交 UFS 命令；设备验证待完成。run28 仍是 Restore APFS 根卷 Apple `launchd` PID 1 的回归基线。

## 最新交接 · run34 / run35

- run34 镜像 `run34-android-smmu-diagnostic/boot.img` SHA-256 为 `e6624ee4d4e47f9c6d6c29ad563ae4da1ce207a4790509c6be188a12c8e8488d`。核验 elish 后 RAM-only 启动获得 `Sending OKAY` / `Booting OKAY`。USB 随后断开，reader 等待序列号超时，未回收日志。该内核的内建驱动可在 PID 1 前改写 SMMU 状态或启动 UFS DMA，实机是否执行到该阶段未知。停止使用此镜像。[run34 记录](reports/elish-run34-android-kernel-20261126.md)
- run35 镜像 `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run35-ufs-queues-readonly/boot.img` SHA-256 为 `bfca4d3e00af3ace7808d6f9f2be5213c90aac745983a92ba322086ca24880a7`。固件编译、板级 preflight 和封装通过。run35 验证 core reset 已释放且电源、时钟有效后，仅读 HCI 状态及请求列表寄存器，然后在 DarwinBoot 前重启。镜像待上机。[run35 记录](reports/elish-run35-ufs-queues-20261126.md)
- 用户恢复 fastboot 后，serial `4bed07e4`、product `elish`、`is-userspace=no` 均已核验。已校验的 reader 上传在 `Sending` 超时。设备仍枚举，USB reset 和两个 fastboot 客户端均未恢复应答。直接探测确认 `getvar:product` 经 bulk OUT 完整发送，bulk IN 超时。`reboot bootloader` 也超时。重新进入可响应命令的 fastboot 后，先走已验证的 Recovery ADB 路径回收日志，再 RAM-only 启动 run35。最后确认的 iPadOS 检查点仍为 run28 Restore 根卷 Apple PID 1。[传输记录](reports/elish-run34-android-kernel-20261126.md#reader-attempt-after-manual-fastboot-entry)
- Recovery 与原厂 DT 的 TBU 范围一致。候选 SID `0xE0` 落在 ANOC1，`0x4E0` 落在 ANOC2。两个 TBU 寄存器窗口均在现有 UEFI SMMU 描述符之外。[拓扑核对](reports/elish-ufs-active-firmware-20261125.md#kona-tbu-topology-cross-check--2026-11-26)

## 最新交接 · run33

- run33 RAM-only boot image `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run33-smmu-context-state-readonly/boot.img`，235,528,192 bytes，SHA-256 `c650c13b6bba566b959d32ba2ee0d867300e902069ce2aa725faf0a457673f7c`；UEFI FD SHA-256 `3a9ca3cd64619d9775a5e91905635a74c5e4909b57bc2367c134940b941a2cd5`。确认 serial `4bed07e4`、model `M2105K81AC`、device/product `elish` 后 `fastboot boot` 返回 `Sending OKAY` / `Booting OKAY`；探针随后 warm-restart 到 fastboot。
- [run33 日志](reports/elish-run33-smmu-context-console-ramoops-20261125.txt)，53,966 bytes，SHA-256 `4a3d2bf2ce1fd4ee98e12ab2ce4d5c1b425a72e70f74dd5ba07a5228344e048b`。活动 ACPI 无 IORT；只发现 group4 SID `0x820`/mask `0x402`/CB4，仍归属 MDSS SDE。CB0–5 有读数；CB6–73 的 SCTLR 原始值带高位非零、解释未明，不能据此分配任何 context bank。
- stock XBL 内嵌 IORT 表头校验和无效，且不是当前活动 ACPI 表；其中 UFS0 input IDs `0`/`0x10` 映射到 SMMU node `0x30` 的候选 SID `0x4E0`/`0xE0`。Android live FDT 的 apps-SMMU 有 `qcom,skip-init`，匹配 Linux 驱动保留已有 SMR/S2CR/context-bank 状态；这提示 Android 可能使用预置路由，但不证明 UFS SID/context，也不确认 UEFI 状态。stock Android 内核未编入 debugfs，`/dev/mem` 与 SMMU sysfs resource 节点不存在，无法从当前内核只读查看寄存器。候选 SID 不匹配 run33 当前唯一有效的 SMMU group；UFS ARID→live SID、context 所有权和 DMA 范围仍未确认。详见 [run33 报告](reports/elish-run33-smmu-context-20261125.md)。
- 本轮未发 UFS command/DMA/SMMU write，也未写任何分区。run33 在 DarwinBoot 前停止；最后确认的 XNU/userspace 阶段仍是 run28 Restore APFS root 上 Apple `launchd` PID 1。只读 Android live-FDT 检查发现 UFS 节点没有 `iommus`、`iommu-map`、`dma-ranges` 或 `dma-coherent`；这不证明 UEFI 路由或 bypass。完整 System root、Setup 和持久 Data 未验证。保持 UFS DMA 关闭；先验证 IORT 来源、live UFS route/context 和全 DMA 地址范围。
- After ADB authorization, bootloader `fetch` proved unsupported; `fastboot reboot recovery` enabled root ADB without flashing. Read-only slot-`_b` exports of `xbl_b`, `xbl_config_b`, `abl_b`, and `imagefv_b` have exact supplied-ROM payload prefixes. This ties the stock-XBL IORT source to the active slot but does not validate its bad-checksum table or a runtime UFS route. Upstream generic SM8250 DTS lists candidate SIDs `0xE0`/`0x4E0`; live UEFI ownership is still unknown. Recovery had no debugfs, `/dev/mem`, SMMU resource files or UFS `iommu_group`. `adb reboot` returned to Android (`sys.boot_completed=1`). Hashes and commands: [firmware readback](reports/elish-ufs-active-firmware-20261125.md).
- 2026-11-26 read-only Android kernel-log capture: model `M2105K81AC`, `elish`, `sys.boot_completed=1`, kernel `4.19.325-cip136-st20-perf-g6b34d2476532`. `apps-smmu` reports 87 stream-match groups; UFS binds and reaches Gear 4 / two-lane FAST, but unlike other clients has no visible SMMU consumer/group log entry. This does not prove bypass or resolve the UEFI route/context. Full capture hash, command and limitations: [live firmware/UFS report](reports/elish-ufs-active-firmware-20261125.md). Keep DMA disabled.
- run32 的控制器时钟/HCI 只读结果与命令仍见 [run32 记录](reports/elish-run32-smmu-control-20261125.md)。run28 镜像继续作为 Restore-root 回归基线；[run28 实机画面](reports/elish-run28-launchd-20261125.jpg)。

## 上一交接 · run31

- run31 RAM-only 镜像 `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run31-ufs-host-status-gated/boot.img`，235,528,192 bytes，SHA-256 `fb7d0ea8f68b8716eacc9e2e3eeff3d98ebd584be9ba74ea7dda412f75ce8f5c`；FD SHA-256 `838964f207748cc42db6e1129f81e9e2cb9454a68c8ba128840e7830379aba4c`。第一次上传 240 秒超时；重授权主机 USB `/sys/bus/usb/devices/1-3/authorized` 后，确认 fastboot `product=elish` 并重试，`Sending OKAY` / `Booting OKAY`。设备运行 probe 后 warm-restart 到 fastboot，再 RAM-only boot 已校验的 reader 回收 ramoops。
- [run31 日志](reports/elish-run31-ufs-hci-console-ramoops-20261125.txt)，45,518 bytes，SHA-256 `b1d4e692456c4f83d3dd59a328d0e5602cf4ce74a06e072f1c6b55c00ee0d20d`。GDSC `0xF822F000`（powered）、core/interface gates enabled；HCI CAP `0x1587031F`、VER `0x300`、IS `0x400`、HCS `0x10F`、HCE `1`。GCC core-BCR raw `0`（未解释极性）；PHY/ICE registers 未读。无 UFS command/DMA/分区写入。
- ACPI 无 IORT；SMMU group4 SID `0x820`/mask `0x402`/CB4 对应 live FDT MDSS SDE，不是 UFS。UFS SID/context 和 DMA addressability 仍未知。报告含完整 build/fastboot 命令及输入哈希：[run31](reports/elish-run31-ufs-hci-20261125.md)。
## 最新交接 · run28

- run24 用户看到绿、蓝、粉三条彩条，随后回到 fastboot。58 秒录像的最后一帧仍是彩条，复位原因未确定。
- run25/26 的 20.19/24.19 秒 USB 断开时长与 7/11 秒复位探针吻合，支持有恢复卷时 `_arm_vm_init()` 已返回。run27 将终止探针移到 `IOSecureBSDRoot()` 返回点，实机 23.23 秒后回到 fastboot，支持 `vfs_mountroot()` 已成功。命令、镜像、哈希和证据见 [恢复启动记录](reports/elish-restore-root-20261125.md)。
- run28 的实机录像出现绿、蓝、粉、青四条彩条。`scripts/patch-xnu-launchd-checkpoint.py` 在 PID 1 首次成功写入 `kern.initproc_spawned=1` 后绘制第四条并继续执行，确认 21H461 恢复环境中的 Apple `launchd` 到达 `main()`。镜像为 `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run28-restore-launchd-cyan/boot.img`，SHA-256 `7e4b05e374c381aefaef41eaa355490d74b3980159adda8b7f01b52a75ab1c13`。本轮只用 `fastboot boot`。[实机画面](reports/elish-run28-launchd-20261125.jpg)
- 完整 System 根卷、Setup 和持久 Data 尚待实测。全量根卷路径仍缺 elish UFS 控制器、PHY 和 DMA 交接验证。

## 最新交接 · run21

- 4 GiB RAM-only run21 到达 `IOFindBSDRoot`，在 `IOKitBSDInit.cpp:984` 报 `Failed to mount root device`。设备照片的四个栈帧与打包内核一致。MMU、`_arm_vm_init()` 和 BSD 初始化已实机确认，APFS 根挂载和 `launchd` 待验证。
- 主机随后重新看到 `4bed07e4`，`getvar product` 返回 `elish`，`max-download-size` 返回 `805306368`。启动命令、镜像和照片哈希见 [run21 记录](reports/elish-run21-root-panic-20261125.md)。
- 21H461 恢复卷 `9071148B-B025-402D-AD7A-48464D8B5FFF` 已从原 IPSW 提取和稀疏打包。下一镜像通过 RamBlock 发布该卷，使用配套恢复信任缓存，并以 `rd=md0-ramblock` 保留 UUID 匹配。后续验证完整 System 根卷需要 UFS 或其他可容纳 8.74 GiB 镜像的存储路径。

## 最新交接 · 2026-11-25 17:36 CST

- 用户手动执行了 `fastboot -s 4bed07e4 boot /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run12-postmmu-console/boot.img`，报告屏幕出现与此前照片近似的彩条。此命令是 RAM-only `fastboot boot`，不是 `fastboot flash`，不会持久写分区。用户没有提供这次照片，故作为口头观察记录；按描述至多对应 run09 已见的 XNU pre-MMU 彩条，不能确认 run12 的 post-MMU 文字或 `_arm_vm_init()` 返回。
- 同一轮主机发出的 run12 命令曾返回 `Sending ... FAILED (remote: 'unknown command')`，没有 `Booting OKAY`；之后 USB 仍列出 `4bed07e4 fastboot` / `18d1:d00d`，ADB 为空，但只读 `getvar product` / `getvar is-userspace` 又返回 `GetVar Variable Not found`。用户手动启动后设备现状未再探测。新会话接手时不要假设它仍在正常 fastboot，也不要用 `continue` 或菜单返回系统项；先遵守用户“从 UniXNU 菜单退出就重启”的要求，并且只在设备重新有稳定控制响应时继续。
- run12 镜像路径与 SHA-256：`/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run12-postmmu-console/boot.img`，`49460e8b7aabf22dc6e921e3435ab47811c9dbd4cfe27b1880a5858d786f324a`。此前一轮主机 RAM-only 启动返回 `Sending OKAY` / `Booting OKAY`，约 6 秒后 fastboot 重枚举，早于探针配置的 15 秒 post-MMU hold；顶置摄像头画面过暗。用户这次手动运行虽看到彩条，但没有新增可核验的 MMU 后证据。
- 最新直接确认仍是 run09 照片所示 pre-MMU；六秒 stage-3 定时复位/USB 时间线符合 UniXNU 文档对 MMU enable 的诊断判据，但 `_arm_vm_init()` 返回未直接确认。run06/run11/run18/run19 ramoops 仍直接确认 FAT payload、DarwinBoot、256 MiB window `0xc007c000+0x10000000` 与 EBS Success。UFS stream/DMA、APFS root、`launchd` 和 iPadOS 用户态未验证。
- 21H461 普通主机候选 `/root/elish-build-20261125/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/boot.img`，SHA-256 `baa41e0a3b5b0e3c77f97b59af4904a6ec8fdad0afccdf7b401348da0c8e0140`；`kernel.json` SHA-256 `4adfd506a92454cf451d71bbf1aa3a0afe93d57e1954f2ea99f423132d22c83f`，`loadable=false`，未上机。板级 `owned_memory_size` 仍为 256 MiB；本轮未构建或实测 4 GiB 配置。
- 4 GiB 离线评估输入：run19 accepted-map 日志的 ConventionalMemory 描述符为 `pa=0xc60bc000 pages=0x1b9f44`，即连续长度 `0x1b9f44000`、终点 `0x280000000`，从该区间开头算 4 GiB 终点 `0x1c60bc000`，几何上放得下。UniXNU DarwinBoot 文档说明 4 KiB bootstrap table 上限约 6 GiB，窗口语法允许到 8 GiB；这仍不是 4 GiB 实机分配证据。产物构建目录是 `DEVELOPMENT_ARM64_BCM2837`，源码中 `ARM_PGSHIFT=12`（未定义 `__ARM_16K_PG__` 时），最终编译宏/预处理分支还应由新会话直接确认后再把 board 请求改为 4 GiB。
- 当前用户要求把工作交给新会话。本轮只更新记录，没有改 `board.toml`、没有持久刷写、没有稳定的新增启动日志。建议下一会话先读取本节及 `PLAN.md` 顶部，确认设备状态与 RAM-only run12 的屏幕证据，再用隔离输出准备有 4 GiB 内存请求的诊断镜像；不要把 `loadable=false` 普通候选当作可启动 iPadOS 镜像。

## 最新交接 · 2026-11-25 17:29 CST

- 按用户要求再次用 RAM-only 重试 run12 post-MMU 检查点。镜像 `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run12-postmmu-console/boot.img` SHA-256 `49460e8b7aabf22dc6e921e3435ab47811c9dbd4cfe27b1880a5858d786f324a`。启动前只读 `getvar product` 为 `elish`；本次 fastboot 对 `boot` 命令返回 `Sending ... FAILED (remote: 'unknown command')`，没有 `Booting OKAY`，所以这次镜像没有执行，不产生 XNU 阶段证据，也未写设备分区。
- 失败后 USB 仍枚举为 fastboot (`4bed07e4`, USB `1-2`, `18d1:d00d`)，ADB 为空；随后的只读 `getvar product` 与 `getvar is-userspace` 返回 `GetVar Variable Not found`，控制协议状态异常。本轮到此暂停设备探测，不发更多 fastboot 命令、不重启设备。并行顶置摄像头视频 `/tmp/elish-run12-retry-20261125.mkv` 为 24.933 秒、SHA-256 `59bcd6dfa58efbb19eebfbef354fd975aa27c17458c5bbd88ac97e710d19b9ec`；没有可辨认的屏幕日志证据。
- 本轮较早的一次 run12 RAM-only 启动成功返回 `Sending OKAY` / `Booting OKAY`，约 6 秒后设备重新枚举 fastboot，但早于该探针 15 秒 checkpoint；顶置单帧 `/tmp/elish-run12-now.jpg` 因欠曝不可辨识。因此两次 run12 重试都没有增加确认阶段。当前最后直接 XNU 证据仍是 run09 屏照确认的 pre-MMU；stage-3 时序满足 UniXNU MMU-enable 诊断判据，但 `_arm_vm_init()` 返回未直接确认。FAT payload、DarwinBoot 256 MiB window 与 EBS 成功仍由 run06/run11/run18/run19 ramoops 直接确认。
- 21H461 常规主机候选仍为 `/root/elish-build-20261125/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/boot.img`，SHA-256 `baa41e0a3b5b0e3c77f97b59af4904a6ec8fdad0afccdf7b401348da0c8e0140`；未上机，`loadable=false`。设备探测暂停期间继续离线核查 kernel 实际页粒度和内存窗口；UFS DMA 仍禁用，APFS 根卷与 iPadOS 用户态未验证。

## 最新交接 · 2026-11-25 17:20 CST

- 用户提醒设备已在 fastboot；本轮只读确认 `fastboot devices -l` 枚举到 `4bed07e4`（USB `1-2`），`fastboot -s 4bed07e4 getvar product` 返回 `product: elish`，`fastboot -s 4bed07e4 getvar is-userspace` 返回 `no`。设备确在 bootloader fastboot 且型号正确。没有重启、启动镜像、写分区或发送关机命令；本轮未生成新启动日志。
- 当前 21H461 主机候选 `/root/elish-build-20261125/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/boot.img`，SHA-256 `baa41e0a3b5b0e3c77f97b59af4904a6ec8fdad0afccdf7b401348da0c8e0140`；本轮未启动。配套 `kernel.json` SHA-256 `4adfd506a92454cf451d71bbf1aa3a0afe93d57e1954f2ea99f423132d22c83f`，`instruction_relocation_complete`、`register_dataflow_qualified`、`abi_qualified`、`runtime_qualified` 与 `loadable` 仍全部为 false。
- 最后确认阶段没有因本轮 fastboot 枚举而改变：run06/run11/run18/run19 ramoops 直接确认 FAT payload、DarwinBoot、256 MiB 窗口 `0xc007c000+0x10000000` 和 EBS Success；run09 屏照直接确认 XNU 到 pre-MMU。run09 的 stage-3 六秒复位/USB 时间线符合 UniXNU 的 MMU-enable 诊断判据，但 `_arm_vm_init()` 返回尚无直接证据。UFS DMA、APFS 根卷挂载和 iPadOS 用户态仍未确认。
- 本轮核对了官方 4 KiB 页表说明和候选实际打补丁的 `arm_init.c`。源代码在 native non-Apple 4 KiB 分支下使页 shift 固定跟随 `ARM_PGSHIFT`；Apple 分支则会在 `args->memSize > 1 GiB` 时选择 `PAGE_MAX_SHIFT`。候选的最终预处理宏/构建配置尚未从该产物单独确认，故未擅自把 `owned_memory_size` 从 256 MiB 改为 4 GiB。run19 EBS 图里第二 DRAM bank 的 ConventionalMemory 区间足以让 4 GiB 请求值得做独立 RAM-only 验证，但并不等于该窗口已分配或 XNU 能管理它。
- 下一步先以已确认的 fastboot 控制通道做有界 RAM-only 检查点，并在运行期间取得屏幕近照/回读日志；同时确定候选 kernel 实际编译的页粒度，再决定是否构建 4 GiB 诊断候选。不要把常规 `loadable=false` 镜像当作用户态候选；UFS DMA 在 stream/context 所有权明确前保持关闭。

## 最新交接 · 2026-11-25 17:02 CST

- 用户询问到 iPadOS 用户态的距离。当前仍在早期 XNU bring-up：FAT payload、DarwinBoot 的 256 MiB 窗口和成功 `ExitBootServices()` 已有 ramoops 直接记录；run09 近照直接确认 XNU 入口到 pre-MMU，配套 stage-3 六秒复位/USB 时间线符合官方 MMU-enable 诊断判据。`_arm_vm_init()` 返回没有直接标记，生产 kernel metadata 仍为 `loadable=false`。
- 本轮只读快照：ADB `4bed07e4` 为 root，设备 `elish` / `M2105K81AC`，活动槽 `_b`；`ro.build.version.release=17`、SDK `37`，但 build fingerprint 字段仍为 `13/RKQ1.211001.001/V816.0.2.0.TKYCNXM`，版本属性不一致，故不单凭 release 属性判定系统版本。`/sys/fs/pstore` 为空，fastboot 未枚举。没有重启、启动镜像、MMIO 访问或分区写入。空 pstore 只是当前系统的状态，不代表历史 XNU 运行结果。
- 离线核对 run18 marker 调用点与 pinned XNU 声明相符：`ml_phys_write(paddr, data)` 的 AArch64 参数为 `x0` 地址、`x1` 32-bit 数据，脚本常量生成也与读回期望一致。因此 `magic=0x00004000 / complement=0` 仍不能证明 `_arm_vm_init()` 返回；待解问题在实际物理写入/映射或执行路径，而不是已发现的参数次序错误。
- 当前 21H461 主机候选 `/root/elish-build-20261125/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/boot.img`，SHA-256 `baa41e0a3b5b0e3c77f97b59af4904a6ec8fdad0afccdf7b401348da0c8e0140`，本轮未上机。对应 `kernel.json` SHA-256 `4adfd506a92454cf451d71bbf1aa3a0afe93d57e1954f2ea99f423132d22c83f`；relocation、register dataflow、ABI、runtime 资格及 `loadable` 均为 false。`python3 scripts/check-elish.py --static-only` 与 `git diff --check` 通过。
- 因此距离不是“再跨一步”：还需直接确认 XNU VM 初始化及 kernel loadability，证明 elish UFS/SMMU stream 与安全 DMA，再做到 UFS 命令、GPT/root、APFS 挂载，最后验证启动服务和 `launchd`。当前没有可信的日历 ETA。下一步优先把 post-MMU checkpoint 做成可直接观察的标记，并继续只读确定 UFS 电源/时钟/SMMU 路径；UFS DMA 维持关闭。

## 最新交接 · 2026-11-25 16:50 CST

- 本轮只读 ADB 导出 `reports/elish-live-ufs-smmu-20261125.txt`，SHA-256 `95402cb216f86c2c576ddbfed47d97def68b7b64ffd53efe90fc406362c09e56`。设备是 Android 13 `V816.0.2.0.TKYCNXM`，Lineage kernel `4.19.325-cip136-st20-perf-g6b34d2476532`（源码 `6b34d24765327d746074c8e53b5ad0d24a51acc7`）；UFS 由 `ufshcd-qcom` 绑定，无 `iommu_group`，SMMU `disable_bypass=Y`。日志明确 UFS host/PHY 因 proper PHY settings 被重新初始化，再从 Gear 1/单 lane/SLOWAUTO 升到 Gear 4/双 lane/FAST。
- 更正之前的临时推断：此前误把 `ptrace_scope=0` 读成 `disable_bypass=0`，并据此推断未绑定的 UFS 使用物理直通 DMA。现在对正确的 sysfs 路径实测为 `Y`；Lineage kernel 的 SMMU 源码说明 unmatched streams fault。故“无 iommu_group => 物理直通”不成立，Android 的 UFS stream/DMA 路径仍未知。该更正已写入 storage record；绝不根据此参数猜测 UEFI/XNU 路径。
- 查阅当前 UniXNU UFS 指南和公开 SM8250-AC `NativeUfs-alioth`/`UniXNU-alioth` 移植记录：参考代码验证 stream 精确匹配、context 独占、bypass 状态及 DMA 地址宽度；同 SoC alioth 有 UFS root 与 launchd 结果。可复用的是流程与检查模式，`0xe0`/`0x4e0` stream 和 alioth GCC/SM8550 派生时钟值不可用于 elish。详情及精确 revision 见 [UFS handoff/reference audit](reports/elish-ufs-reference-audit-20261125.md)。
- 该对照不改变 elish 当前里程碑：run09 照片确认 XNU 到 pre-MMU，stage-3 定时复位/USB 时间线符合官方 MMU-enable 诊断判据；`_arm_vm_init()` 返回未被有效 marker 直接证实。成功 EBS 有 ramoops 证据，但 kernel 仍 `loadable=false`，尚未做 XNU 设备 UFS 命令、设备 APFS mount 或 launchd。距离用户态仍有早期 XNU/内存 handoff、生产 kernel 资格、UFS DMA/transport、root/APFS 和启动服务等关卡，暂无可靠 ETA。
- 下一步先加一轮只读 UEFI handoff probe：读取板级 GCC power/clock/reset 与 SMMU global control/stream map；只有确认 UFS power/access window 正常后再读控制器寄存器。之后先证实 stream 路由和 DMA buffer 可达，再做有界 NOP/read-capacity、已知块、GPT/root window、APFS 和 launchd。UFS DMA 继续关闭。主机 `git diff --check` 与 porting record checker (`elish: 14 structured areas checked`) 通过；本轮未重启设备、未触碰 MMIO、未写分区，设备最后观察仍为 root ADB Android。

## 最新交接 · 2026-11-25 16:05 CST

- run09 用户近照 `/root/repo/A3AABAD37D0E0E787C85F39100B9ACCB.png`（SHA-256 `b03cc52878f046d7574ab1a776a3f83662f10251875438dcfbd2181ac99ee7b8`）显示绿色 EBS、蓝色 XNU 入口和品红色 MMU 前阶段条，直接确认 XNU 到达 MMU 前检查点。该镜像在 MMU 开启后六秒执行 stage-3 PSCI 复位；USB 于 15:47:32 断开、15:47:50 返回 fastboot，照片时间为 15:47:51。按 UniXNU green-bar diagnosis 的 stage-3 判据，当前记录为 MMU 已开启；这是定时复位证据，不是 pstore/串口的内核日志标记。run09 曾写入备份过的活动槽 `boot_b`，随后已恢复原分区。
- run13 `_arm_vm_init` 返回探针（SHA-256 `05b8620d4ccc74e95bccb3fef0edc71946d205036e0fd71da005361a2ff459b9`）和 run17 `PE_init_printf` 探针（SHA-256 `3bcc3f62f519b96a5b56fd47da752a68c168b3f63e5c17c36a3bf8e92c9d6f7e`）均有接近配置复位时序的 USB 返回，属于间接证据。run18 pmsg readback 的 marker 为 `magic=0x00004000, complement=0, valid=0`，因此 `_arm_vm_init` 返回未直接证实。没有后续 XNU 平台启动或 Darwin kernel log 证据。
- run19 只读 SMMU 捕获未发现活动 ACPI IORT；唯一有效 apps-SMMU 项为 SID `0x820`、mask `0x402`、context bank 4。Android live FDT 中 MDSS 的 `qcom,smmu_sde_unsec_cb` 声明 `iommus = <0x46 0x820 0x402>`，phandle `0x46` 为 `#iommu-cells=2` 的 apps-SMMU，因此该映射归属于显示 SDE。UFS 节点没有 `iommus` 或 `iommu_group`，UFS SID/DMA 模式仍未知，继续禁用 UFS DMA。
- FAT payload、DarwinBoot 256 MiB window `0xc007c000+0x10000000`、EBS Success（key `0x3ee`）已有 run06/run11/run18/run19 console ramoops 直接记录。生产 XNU prelink metadata 仍为 `loadable=false`；UFS 设备访问、APFS SystemOS/root 挂载和 iPadOS 用户态均未验证。最新阶段/下一步汇总见 [进度报告](reports/elish-progress-20261125-1605.md)。
- 后续先定位 run18 pmsg marker 未读回原因，并在 `_arm_vm_init` 返回后加入可见检查点；并行核实 UFS 的 XBL ARID/DMA 路径和运行时初始化。之后确认 XNU runtime memory ownership 与 production kernel 资格，再推进只读 UFS、设备 APFS mount 和 launchd。设备最后一次记录状态为 reader 恢复后的 root `lineage_elish` ADB；未留持久分区修改。

## 最新交接 · 2026-11-25 15:38 CST

- 用户给出的 `web1n/android_device_xiaomi_elish` 已核对，HEAD 为 `23e652dde3a1b1a2f6f6ff3f7e088d3f6c814772`。它是 LineageOS 21 设备配置仓，不含 DTS/DTSI；`BoardConfig.mk` 继承 `sm8250-common`，`lineage.dependencies` 也只引入该公共设备仓。公共仓 HEAD `ae56501ed9cedbd1a66047e0d3aea9f41a830aec` 的 fstab 用 `sysfs_path=/sys/devices/platform/soc/1d84000.ufshc` 标识 Android UFS host，但没有 SID。实机 root ADB 当前读取的 UFS FDT 节点没有 `iommus` 属性，`iommu_group` 链接也不存在。该项目补充了板级 Android 路径信息，仍不能把 run19 的 apps-SMMU SID `0x820` 归属给 UFS。详情见 [Kona UFS/SMMU 审计](reports/elish-ufs-kona-clock-map-20261125.md)。
- run17 `PE_init_printf` RAM-only 镜像 `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run17-printf-init/boot.img`，SHA-256 `3bcc3f62f519b96a5b56fd47da752a68c168b3f63e5c17c36a3bf8e92c9d6f7e`；fastboot 37.0.3 的发送和启动均成功，设备随后在约 15 秒探针复位时序附近重新枚举为 fastboot。没有可读近照，也没有该次 run17 的 ramoops 读回；USB 回来时间不能证明 `PE_init_printf` 返回或 MMU 后路径。随后已 RAM-only 启动 reader，当前设备回到 root ADB `lineage_elish` / `M2105K81AC`。本轮未写分区、未发关机命令，也未使用 `fastboot continue`。完整测试说明见 [run14–17 报告](reports/elish-run14-run17-xnu-timing-20261125.md)。
- 本轮只读 ADB 确认设备在线且 `uid=0`；fastboot 当前未枚举。未启动常规 21H461 镜像；当前主机候选 `/root/elish-build-20261123/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/boot.img` SHA-256 为 `a8ce2352da7416064a891b6d7ab29102190aaaf508c236cc4d180c5685b146fa`。
- 最新直接确认的 XNU 阶段仍是 run09 近照中的 pre-MMU 彩条。run06/run11/run18/run19 的 ramoops 直接确认 FAT payload、DarwinBoot 256 MiB 窗口 `0xc007c000+0x10000000` 和成功 `ExitBootServices()`；run18/readback 未读到有效 post-VM marker，run19 读到 ACPI configuration 没有 IORT，故 MMU 后、UFS DMA、设备 APFS 根卷和 iPadOS 用户态均未确认。下一步先做能直接区分 MMU 后检查点的诊断；UFS DMA 继续禁用。

## 最新交接 · 2026-11-25 14:58 CST

- 设备当时以 root ADB 枚举为 `lineage_elish` / `M2105K81AC`，活动槽 `_b`；测试前 `/sys/fs/pstore` 为空。之后 run18 marker、run18 readback、增强版 run19 和 hash-verified Lineage reader 均通过 RAM-only `fastboot boot`。没有写任何设备分区，没有发关机命令，也没有选择 UniXNU 菜单返回系统项。测试结束后 reader 已恢复 root `lineage_elish` ADB。
- run18 post-VM 镜像 `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run18-post-vm-pmsg-image/boot.img`，SHA-256 `842ac4db5666b20cae96725ec621fa0c42ce74fe612f19eebb8ac8d36a6e30`；回读镜像 `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run18-pmsg-readback-image/boot.img`，SHA-256 `d96aeadaf64999975f6af3427b8a811aaed639679d111ff70992cb24f559c939`。顶置相机观察到绿/蓝/品红条，约与探针 15 秒复位时序相符，但没有可读近照。pmsg 回读得 `magic=0x00004000 complement=0x00000000 valid=0`，故 `_arm_vm_init` 返回仍未确认。readback console 日志 `reports/elish-run18-pmsg-readback-console-ramoops-20261125.txt` 为 46,607 字节，SHA-256 `a2d7e0c084aa13572dc9dc9ff51faf1593f829f1ddab8716426154137b3c3627`。
- 增强版 run19 IORT/SMMUv2 镜像 `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run19-iort-smmu-nodeids/boot.img`，SHA-256 `c8d724467afb0822ad6caba9074a2b40a5dc8075a4947589ee947cfcef6dd371`；FD SHA-256 `c9b1be78eb7866155798a9a082d76b0403ed2d5331a94e7a9427c3cb67de34dc`。实机日志明确报告当前 ACPI configuration 中没有 IORT；SMMU group 4 为 `SID=0x820 mask=0x402 S2CR=0x4 cb=4`，context `CBAR=0x1f000 SCTLR=0xe0 TTBR0=0 TCR=0 FSR=0x400`，S1 未启用。无法把该 SID 归属到 UFS，UFS DMA 继续禁用。原始 console log `reports/elish-run19-iort-smmu-console-ramoops-20261125.txt` 为 49,786 字节，SHA-256 `322c28323084e3239e35300f1be43a77b062b379c08af99fbcf2842f575140f0`。
- run18 readback 与 run19 ramoops 均再次确认 FAT initrd 子卷、DarwinBoot 256 MiB 窗口 `0xc007c000+0x10000000`，以及 `ExitBootServices()` 成功（key `0x3ee` 与 snapshot 相同）；两探针均按设计在 XNU 前停止。最新日志确认阶段是 EBS，最新直接 XNU 证据仍是 run09 用户近照中的 pre-MMU 条带。MMU 后、UFS 读取、APFS root 和 iPadOS 用户态仍未确认。
- 两轮命令、所有镜像路径/哈希和日志见 [run18/run19 实机报告](reports/elish-run18-run19-device-20261125.md)。下一步离线检查 pmsg marker 地址与 XNU MMU 后运行路径，并寻找 elish 专属 UFS 初始化/映射证据；不得根据 Kona/alioth 的 `0x4e0`/`0xe0` 猜测，也不启用 UFS DMA。

## 最新交接 · 2026-11-25 12:08 CST

- 用户允许休息期间写设备分区，但要求设备不能关机；持久写入前仍须备份。本轮只读 `adb devices -l` 与 `fastboot devices -l` 均为空，与用户报告可进入 fastboot 的状态不一致。按设备不可见时暂停探测的约定，没有继续发 USB/fastboot 命令，没有启动、重启、写分区或改变电源状态。
- 顶置摄像头快照 `/tmp/elish-current-camera-20261125-late.jpg` 为 1920x1080，SHA-256 `25dde9a4cf57cd90a6bff7e4fb249fa6e13695e11784acd2e73758546c80f7ac`；画面欠曝且无法辨认平板屏幕，不能据此判定屏幕或设备电源状态。
- 离线审阅 run19 `ElishLoaderProbeDxe.c`：IORT 表长、节点长度及映射数组做了边界校验；SMMU 扫描只读受限 MMIO 寄存器，不写寄存器、不安装表、不进入 XNU。该审阅没有生成新的实机证据。run19 镜像 `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run19-iort-smmu-readonly/boot.img` SHA-256 `ab653c2ebee272a7a56fc0c60eba6490b14e77132dd3b3eccc3a91d7aaf9fe0b`，仍未上机；run18 marker/readback 镜像 SHA-256 分别为 `842ac4db5666b20cae96725ec621fa0c42ce74fe612f19eebb8ac8d36a5e6a30` 与 `d96aeadaf64999975f6af3427b8a811aaed639679d111ff70992cb24f559c939`，也仍未上机。
- 本轮没有启动镜像、没有新日志。最近直接确认阶段仍为 run09 屏照中的 XNU pre-MMU；run06/run11 ramoops 仍确认 FAT payload、DarwinBoot、256 MiB 窗口和成功 `ExitBootServices()`。MMU 返回、UFS/SMMU 归属、APFS 根卷及 iPadOS 用户态未确认。设备恢复枚举后先核验 `product=elish`，RAM-only 运行 run18 marker/readback，再单独运行 run19；UFS DMA 继续禁用。不得发送关机命令或选择 UniXNU 菜单返回系统项。

## 最新交接 · 2026-11-25 11:59 CST

- 本轮设备没有重新枚举；最近一次只读快照（11:40 CST）中 USB、fastboot、ADB 均未发现平板。没有发送设备请求、启动/重启、写分区或改变电源状态，相机帧也无法判断屏幕状态。
- 新增单独的 run19 只读 IORT/SMMUv2 诊断镜像，尚未上机：`/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run19-iort-smmu-readonly/boot.img`，67,751,936 字节，SHA-256 `ab653c2ebee272a7a56fc0c60eba6490b14e77132dd3b3eccc3a91d7aaf9fe0b`；配套 FD `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run19-iort-smmu-readonly/SILICIUM_UEFI.fd`，3,145,728 字节，SHA-256 `2f40713e964cd2ba54d66759244e4dba7bfe1340746b21676864a9f795b797dc`。构建参数、payload/DTB 哈希、修订和限制见 [run19 构建报告](reports/elish-iort-smmu-probe-20261125.md)。固件已编译并确认包含 IORT/SMMU 诊断字符串；这仅是主机证据。
- run19 在现有 SMMU 只读模式下枚举当前 UEFI ACPI IORT，校验表/节点/映射边界后记录 `UFS0` 映射，再读取 SMMUv2 寄存器；不会写寄存器、安装表或进入 XNU。运行时表和 ARID 到 SID 的关系仍待实机读取，UFS DMA 继续禁用。
- 本轮未运行镜像，无新增设备日志。最近直接启动阶段仍为 run09 用户照片中的 XNU pre-MMU；run06/run11 日志确认 FAT payload、DarwinBoot、256 MiB 窗口和 EBS 成功。MMU 后、UFS/SMMU 接管、APFS 根卷挂载和 iPadOS 用户态都未确认。常规 21H461 镜像 SHA-256 仍为 `baa41e0a3b5b0e3c77f97b59af4904a6ec8fdad0afccdf7b401348da0c8e0140`；run18 marker/readback 镜像未上机。设备恢复后先 RAM-only 测 run18，再单独运行 run19；任何持久分区写入前先备份，不发送关机命令，也不选择 UniXNU 菜单返回系统项。

## 最新交接 · 2026-11-25 11:40 CST

- 本轮只读 USB 快照未发现平板：`lsusb` 只有主机 root hub、顶置摄像头、键盘和鼠标，`fastboot devices -l` 与 `adb devices -l` 均为空。只做了这一轮枚举，没有继续发送设备协议命令、启动/重启、写分区或改变电源状态。V4L2 单帧 `/tmp/elish-current-camera-20261125.jpg`（640x480，SHA-256 `f42a6437d40f232b6b767f97ec949c25202c6971015d14518dddbba15aab365b`）全暗，不能据此判断屏幕或设备状态。
- 主机验证重跑通过：`git diff --check`、`python3 scripts/check-elish.py --static-only`、UniXNU `porting-record.py check --device /root/repo/UniXNU-elish`（14 区域）及 `scripts/package.sh --check --os ipados-17.7.11-21H461`（解析 63 个补丁）。静态检查仍提示运行时内存 handoff 未验证；package check 没有构建或启动镜像。
- 离线核对到共享 Kona IORT 模板含 `\\_SB.UFS0` 节点，映射输出基址为 `0x4e0`/`0xe0`；但 elish 的 DSC/FDF 没有集成 `AcpiTables.inf` 或 `AcpiTableDxe`，所以不能把模板当作 elish 运行时 IORT。它与 run11 唯一观测到的 SID `0x820` 也不相同。结论、哈希和参考板集成方式见 [UFS/SMMU 审计](reports/elish-ufs-uefi-arid-audit-20261125.md)，UFS DMA 继续禁用。
- 本轮未运行镜像，无新增设备日志。未上机候选：常规 21H461 镜像 `/root/elish-build-20261125/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/boot.img`，SHA-256 `baa41e0a3b5b0e3c77f97b59af4904a6ec8fdad0afccdf7b401348da0c8e0140`；run18 post-VM marker `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run18-post-vm-pmsg-image/boot.img`，SHA-256 `842ac4db5666b20cae96725ec621fa0c42ce74fe612f19eebb8ac8d36a5e6a30`；pmsg readback `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run18-pmsg-readback-image/boot.img`，SHA-256 `d96aeadaf64999975f6af3427b8a811aaed639679d111ff70992cb24f559c939`。
- 最近直接确认阶段仍是 run09 用户近照中的 XNU pre-MMU。run06/run11 ramoops 日志 `reports/elish-run06-smmu-console-ramoops-20261125.txt`、`reports/elish-run11-smmu-context-console-ramoops-20261125.txt` 确认读取 FAT payload、进入 DarwinBoot、256 MiB 窗口及成功 `ExitBootServices()`；run11 在 XNU 前停止。MMU 后、UFS/SMMU 接管、设备 APFS root 和 iPadOS 用户态都未确认。设备恢复枚举后，先确认 fastboot 控制通道及 `product=elish`，再按 run18/readback 流程 RAM-only 测试；任何持久写入先备份分区，不发送关机命令，也不选择菜单中的返回系统项。

## 最新交接 · 2026-11-25 11:24 CST

- 本轮 `lsusb -d 18d1:d00d`、`fastboot devices -l` 和 `adb devices -l` 均未枚举平板。遵照设备不可见时暂停设备操作，没有发 fastboot 控制请求、启动/重启、改写分区或改变电源状态。V4L2 快照 `/tmp/elish-current-camera-check-mjpg.jpg`（640x480，SHA-256 `79d4c0f1c65f0e15306c6c90baede715d8c9a491b9b87caa475045d197484e0d`）近乎全暗，不能判断屏幕或设备电源状态。
- 更正 UFS firmware 审计：`UFSDxe`/`SdccDxe` 实际查询 GUID 为 `54b6d3b4-5d33-4f91-8600-6c41d5deb19a`；此前的 `416c0086-ded5-9ab1-b673-0e22db6b1344` 来自查询 GUID 起始地址偏移 8 字节的误读。stock XBL `HALIOMMUDxe` PE32 与 alioth/pipa catalog 模块完全相同，并通过同一 GUID 安装 provider。HAL 按设备路径和 ARID 读取 ACPI IORT mapping 后配置 SMMU；elish 当前 IORT 实际 stream mapping 未取回，所以 run11 SID `0x0820` 仍无法归属 UFS。审计和复现脚本见 [UFS/SMMU firmware audit](reports/elish-ufs-uefi-arid-audit-20261125.md)。UFS DMA 仍禁用。
- 本轮未启动镜像或写分区，也没有新的设备日志。21H461 常规候选 `/root/elish-build-20261125/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/boot.img`（SHA-256 `baa41e0a3b5b0e3c77f97b59af4904a6ec8fdad0afccdf7b401348da0c8e0140`）、run18 post-VM marker（`842ac4db5666b20cae96725ec621fa0c42ce74fe612f19eebb8ac8d36a5e6a30`）和 pmsg readback（`d96aeadaf64999975f6af3427b8a811aaed639679d111ff70992cb24f559c939`）均未上机。
- 最近直接确认阶段仍为 run09 屏照中的 XNU pre-MMU。run06/run11 ramoops 日志 `reports/elish-run06-smmu-console-ramoops-20261125.txt`、`reports/elish-run11-smmu-context-console-ramoops-20261125.txt` 确认读取 FAT payload、进入 DarwinBoot、使用 256 MiB 窗口并成功 `ExitBootServices()`；run11 在 XNU 前停止。MMU 后、UFS/SMMU 接管、设备 APFS 根卷和 iPadOS 用户态都未确认。设备恢复枚举后先核验 fastboot 控制通道与 `product=elish`，再执行 run18/readback 流程；退出 UniXNU 菜单时直接重启，不选择返回系统项。

## 最新交接 · 2026-11-25 10:54 CST

- 设备在 USB、fastboot 和 ADB 快照中都未枚举。本轮遵照设备不可见时停止探测，没有发送设备命令、重启、启动镜像或写分区；也没有改变设备电源状态。V4L2 单帧 `/tmp/elish-live-state-current.jpg`（640x480，SHA-256 `42a001a78be79cc67a2b11bea0cf065dbcf49ff235558838b2d524df5b0d59e7`）处理后仍近乎全暗，不能辨认屏幕或判断设备当前状态。
- 继续离线反编译 elish stock XBL 中与 catalog 同哈希的 `UFSDxe`。它与 SDCC DXE 共用私有 IOMMU protocol GUID `416c0086-ded5-9ab1-b673-0e22db6b1344`；UFS attach 使用 ACPI 路径 `\_SB_.UFS0` 和 ARID `0x0`/`0x10`，SDCC 使用 `0x0`/`0x1`。这确认 ARID 是协议的设备侧参数，不能映射成 run11 apps-SMMU SID；SID `0x0820` 所有者仍未知。catalog 中未定位该私有协议的提供者。审计细节与可复跑 Ghidra 脚本见 [UFS ARID 审计](reports/elish-ufs-uefi-arid-audit-20261125.md)。没有启用 UFS DMA。
- 本轮没有启动镜像，分区未写入。21H461 常规候选 `/root/elish-build-20261125/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/boot.img`（SHA-256 `baa41e0a3b5b0e3c77f97b59af4904a6ec8fdad0afccdf7b401348da0c8e0140`）、run18 post-VM marker `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run18-post-vm-pmsg-image/boot.img`（`842ac4db5666b20cae96725ec621fa0c42ce74fe612f19eebb8ac8d36a5e6a30`）和 pmsg readback `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run18-pmsg-readback-image/boot.img`（`d96aeadaf64999975f6af3427b8a811aaed639679d111ff70992cb24f559c939`）本轮均未上机。
- 没有新增启动阶段或设备日志。最近直接确认仍是 run09 照片所示 XNU pre-MMU；run06/run11 ramoops 日志确认 FAT payload、DarwinBoot、256 MiB 窗口和 EBS Success，run11 在 XNU 前停止。MMU 后、UFS/SMMU 接管、设备 APFS 根卷和 iPadOS 用户态均未确认。用户休息期间不做电源状态切换；设备重新枚举后先核实 fastboot 控制通道，再按 run18 与 readback 流程操作。

## 最新交接 · 2026-11-25 10:44 CST

- 本轮只读 USB 快照仍未枚举平板：`lsusb -d 18d1:d00d`、fastboot 和 ADB 均为空。遵照不可见时暂停设备探测，没有发送设备协议请求、启动/重启或写分区；设备屏幕和电源状态未知。
- 本轮没有启动或生成新镜像。常规候选 SHA-256 仍为 `baa41e0a3b5b0e3c77f97b59af4904a6ec8fdad0afccdf7b401348da0c8e0140`；run18 post-VM marker 镜像为 `842ac4db5666b20cae96725ec621fa0c42ce74fe612f19eebb8ac8d36a5e6a30`，pmsg readback 镜像为 `d96aeadaf64999975f6af3427b8a811aaed639679d111ff70992cb24f559c939`，均未上机。
- 核对移植记录后，Android R UFS/SMMU 交叉审计已由 storage 区域引用；它没有识别 UFS stream，run11 的 SID `0x0820` 所有者仍未知，UFS DMA 仍禁用。详细证据见 [Kona UFS 映射审计](reports/elish-ufs-kona-clock-map-20261125.md)。
- 主机复核通过：`git diff --check`、`python3 scripts/check-elish.py --static-only` 和 14 区域移植记录检查。静态检查仍提示运行时 handoff 未验证。
- 无新增启动阶段或设备日志。最近直接阶段仍是 run09 的 XNU pre-MMU 屏照；run06/run11 直接日志确认 FAT payload、DarwinBoot、256 MiB 窗口和成功 EBS。MMU 后阶段、UFS/SMMU 接管、APFS 根卷和 iPadOS 用户态仍未确认。设备恢复枚举且 fastboot 控制请求正常后，按 run18 与 pmsg readback 流程 RAM-only 测试；不发关机命令，也不选择 UniXNU 菜单返回系统项。

## 最新交接 · 2026-11-25 10:41 CST

- 本轮 USB 快照仍未枚举平板（`lsusb -d 18d1:d00d`、fastboot 和 ADB 为空）。遵照设备不可见时暂停探测，没有发送设备协议请求、启动、重启或写入分区。屏幕/电源状态未知；本轮未能从相机辨认屏幕。
- 更正旧 run18 记录中的风险描述：pmsg readback 镜像只读取 pmsg 区和 marker，`RamLogWrite()` 写到不重叠的 console 区，不会覆盖 pmsg 最后一页。当前读取流程已核对至 root ADB reader；真实 warm-reset 保留行为仍待设备验证。详见 [run18 pmsg 路径审计](reports/elish-run18-pmsg-path-audit-20261125.md) 和 [run18 构建报告](reports/elish-run18-post-vm-pmsg-20261125.md)。
- Android R 内核 revision `52a2f180865f8368828985d6384130acd093fa93` 的本地 Kona MTP UFS 配置和通用 `qcom,ufshc` 驱动没有给出 UFS stream ID；它不是 elish 专属证据。结合 stock elish DT 无 `iommus` 属性，run11 的 SID `0x0820` 所有者仍未知，不启用 UFS DMA。详见 [Kona UFS 映射审计](reports/elish-ufs-kona-clock-map-20261125.md)。
- 离线审计 pinned XNU `d8b80295118ef25ac3a784134bcf95cd8e88109f` 后确认 run18 地址路径在源码上成立：`0xb03ff000` 位于 run06/11 XNU 窗口 `0xc007c000+0x10000000` 之外，`_ml_phys_write` 对非 managed 页走 copy window；`pmap_cache_attributes()` 为无特定 I/O range 的页返回 `VM_WIMG_IO`。这与 UEFI map 将 `0xb0000000+0x400000` ramoops 标作 reserved/uncached 相符，但没有运行时写入或跨复位保留证据。细节见 [run18 pmsg 路径审计](reports/elish-run18-pmsg-path-audit-20261125.md)。
- 独立复现 post-VM 补丁后，产物与既有 run18 内核 SHA-256 完全相同（`8f0fcd60069e14672bb71ec633c74b9b02f53870daafc5c8188c507bae146ee5`）。`llvm-objdump` 确认 `_arm_init` 的调用分支进入代码洞，代码洞调用 `_arm_vm_init`，返回后两次调用 `_ml_phys_write` 写 marker/反码，再等待 15 秒并 PSCI warm reset；这是主机二进制验证，未增加实机阶段。
- 审计 UEFI 日志边界后确认 RamLog console 环只写 `0xb0000000..0xb0200000`；pmsg 读取区为 `0xb0200000..0xb0400000`，readback 日志回写到 console 环，因此回读本身不会覆盖 marker。仍需设备实测验证 warm reset 保留行为，见同一 [run18 pmsg 路径审计](reports/elish-run18-pmsg-path-audit-20261125.md)。
- 本轮没有启动任何镜像或生成新镜像。常规候选 `/root/elish-build-20261125/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/boot.img`（SHA-256 `baa41e0a3b5b0e3c77f97b59af4904a6ec8fdad0afccdf7b401348da0c8e0140`）、run18 post-VM marker 镜像 `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run18-post-vm-pmsg-image/boot.img`（SHA-256 `842ac4db5666b20cae96725ec621fa0c42ce74fe612f19eebb8ac8d36a5e6a30`）和 pmsg readback 镜像 `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run18-pmsg-readback-image/boot.img`（SHA-256 `d96aeadaf64999975f6af3427b8a811aaed639679d111ff70992cb24f559c939`）本轮均未上机。
- 本轮主机检查：run18 补丁可确定性复现，反汇编分支/调用目标符合预期；`python3 scripts/check-elish.py --static-only` 通过并提示运行时 handoff 仍待验证；移植记录检查通过（14 区域）；设备仓 `git diff --check` 通过。这些不构成设备启动证据。
- 无新增启动阶段或设备日志。最近直接 XNU 阶段仍是 run09 屏照所示 entry/pre-MMU；run06/run11 控制台日志直接确认 FAT payload、DarwinBoot、256 MiB 窗口和 EBS Success，run11 明确在 XNU 前停止。MMU 后启动、UFS/SMMU 接管、设备 APFS 根卷及 iPadOS 用户态均未确认。设备恢复枚举且 fastboot 控制请求正常后，按现有方案 RAM-only 启动 run18，再启动只读 pmsg 回读镜像并用 root ADB reader 导出 UEFI console。不得发送关机命令，也不要选择 UniXNU 菜单的返回系统项。

## 最新交接 · 2026-11-25 10:08 CST

- 本轮初始只读快照中 `lsusb -d 18d1:d00d`、fastboot `devices -l` 和 ADB 均未枚举平板。遵照设备不可见时暂停探测，没有发送 fastboot/ADB 请求、启动或重启设备，也没有写分区。设备当前屏幕/电源状态未知。
- 离线构建 run18 `_arm_vm_init` 返回检查点：内核在函数返回后用 `_ml_phys_write` 将 magic/反码写到 pmsg 保留区 `0xb03ff000`，等待 15 秒后 PSCI reset。内核 SHA-256 `8f0fcd60069e14672bb71ec633c74b9b02f53870daafc5c8188c507bae146ee5`；payload SHA-256 `d6ff20e31c4b375ea15192c7c718c595d3de2bb8310596b6647ccc5acf27c0ce`；boot image `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run18-post-vm-pmsg-image/boot.img`，67,751,936 字节，SHA-256 `842ac4db5666b20cae96725ec621fa0c42ce74fe612f19eebb8ac8d36a5e6a30`。另构建只读 pmsg 回读镜像 `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run18-pmsg-readback-image/boot.img`，67,751,936 字节，SHA-256 `d96aeadaf64999975f6af3427b8a811aaed639679d111ff70992cb24f559c939`；它在 EBS 后复位，不进入 XNU。完整路径、FD 哈希和构建命令见 [run18 报告](reports/elish-run18-post-vm-pmsg-20261125.md)。
- 主机检查通过：脚本语法、两次 DEBUG 固件构建、boot image 构建/回读、`test_bootimg.py`（1 项）、`test_boot_template.py`（13 项）、`check-elish.py --static-only`、14 区域移植记录及两个仓库 `git diff --check`。这些均非实机证据；常规 iPadOS 候选仍为 SHA-256 `baa41e0a3b5b0e3c77f97b59af4904a6ec8fdad0afccdf7b401348da0c8e0140`，本轮未修改或启动。
- 本轮没有新增启动日志或阶段。最近直接 XNU 阶段仍为 run09 近照显示的 entry/pre-MMU 条带；run06/run11 仍直接确认 FAT payload、DarwinBoot、256 MiB 窗口与 EBS Success。MMU 完成、UFS/SMMU 所有权、设备 APFS root 和用户态未确认。设备重新枚举且 fastboot 控制请求正常后，核验 `product=elish`，RAM-only 启动 run18，再启动回读镜像并用 root ADB reader 导出 `console-ramoops-0`。不要选择 UniXNU 菜单的返回系统项；不得发送关机命令。

## 最新交接 · 2026-11-25 09:31 CST

- 按用户最新说明，可在其休息期间继续授权范围内的设备操作，但不得将设备关机；本轮主机 `lsusb`、fastboot 和 ADB 均未枚举平板，因此没有发送设备协议请求、启动/复位设备或写分区。顶置摄像头只读帧 `/tmp/elish-live-state-20261125.jpg`（640x480，SHA-256 `cdd594ab518dbc640ce6c0daac1a38f2aafb053e3b47146765fb919967cd5f3e`）严重欠曝，不能辨认屏幕阶段，也不据此判断设备电源状态。
- 离线复核通过：`python3 scripts/check-elish.py --static-only`（提示运行时 UEFI/XNU handoff 仍未验证）、`python3 /root/repo/UniXNU/scripts/porting-record.py check --device /root/repo/UniXNU-elish`（14 区域）、UniXNU `test_bootimg.py`（1 项）、`test_boot_template.py`（13 项）、XnuPrelink（65 项）和 `git diff --check`。iPadOS 候选 `/root/elish-build-20261125/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/boot.img` 为 67,747,840 字节，SHA-256 `baa41e0a3b5b0e3c77f97b59af4904a6ec8fdad0afccdf7b401348da0c8e0140`；run17 检查点 `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run17-printf-init/boot.img` 为 67,751,936 字节，SHA-256 `3bcc3f62f519b96a5b56fd47da752a68c168b3f63e5c17c36a3bf8e92c9d6f7e`。两镜像本轮均未上机。
- 本轮无新增启动阶段或日志。最近直接 XNU 阶段证据仍为 run09 近照 `/root/repo/UniXNU-elish/reports/elish-run09-screen-20261125.jpg`：蓝色 XNU entry 与品红色 pre-MMU 标记可见；run06/run11 原始控制台记录确认 FAT payload、DarwinBoot、256 MiB 窗口及 `ExitBootServices()` 成功，详见 `reports/elish-run06-smmu-console-ramoops-20261125.txt`、`reports/elish-run11-smmu-context-console-ramoops-20261125.txt`。`_arm_vm_init` 返回、UFS/SMMU 交接、设备 APFS root 和 iPadOS 用户态均未确认。后续设备恢复枚举时，先验证 fastboot 控制请求和 `product=elish`，再按授权 RAM-only 启动 run17 并用 reader root ADB 分别检查 pmsg 与 console；若涉及持久分区写入，先备份。严禁选择 UniXNU 菜单“返回系统”或发送关机命令。

## 最新交接 · 2026-11-25 09:25 CST

- 离线解析 run09 后使用的原始 Lineage reader 镜像。reader SHA-256 `57dbb98dec813b2a8c7bbd67a59f715aa1abf366905a24270aba594ec4ca44c4`，由备份 `boot_b.img` 去掉分区尾部填充而来，kernel 与 ramdisk 未改。提取的 4.19.325 kernel 含内嵌配置：`CONFIG_PSTORE_PMSG=y`、`CONFIG_PSTORE_CONSOLE=y`、`CONFIG_PSTORE_RAM=y`。所以 run09 后 `/sys/fs/pstore` 为空不能归因于 reader 没有 pmsg 前端；写入是否到达/保留于 `0xb0200000` 仍未解决。kernel/config 哈希和复现命令见 [pmsg reader 配置审计](reports/elish-pmsg-reader-config-20261125.md)。
- 本轮没有设备 USB 探测或设备命令；最近一次 09:07 CST `lsusb`、fastboot 与 ADB 快照仍未枚举平板。当前无新启动阶段证据。最近直接阶段仍为 run09 照片确认 XNU 到 pre-MMU；run06/run11 直接日志确认 FAT payload、DarwinBoot、256 MiB 窗口和成功 EBS。MMU 完成、UFS/SMMU、APFS root 和 iPadOS 用户态尚未验证。
- fastboot 恢复后仍先 RAM-only 启动 run17（`/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run17-printf-init/boot.img`，67,751,936 字节，SHA-256 `3bcc3f62f519b96a5b56fd47da752a68c168b3f63e5c17c36a3bf8e92c9d6f7e`），然后 boot reader 并分别检查 `pmsg-ramoops-0` 和 `console-ramoops-0`。若仍无记录，再确认 reader 是否能只读导出 pmsg 保留区；不要将空 pstore 当作阶段证据。

## 最新交接 · 2026-11-25 09:07 CST

- 完成 iPadOS 17.7.11 (`21H461`) 主机打包，命令 `./scripts/package.sh --no-deps --os ipados-17.7.11-21H461 --uefi /root/repo/uefi-elish --out /root/elish-build-20261125/UniXNU/out -j 8`。实际产物在 `/root/elish-build-20261125/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/boot.img`（67,747,840 字节，SHA-256 `baa41e0a3b5b0e3c77f97b59af4904a6ec8fdad0afccdf7b401348da0c8e0140`）；FAT payload `payload/payload.img` SHA-256 `9aaca2b53648d2dd8bb9ae7f362b8069c951e1f6760c048f8b8b18e45b5f3821`；`kernel.macho` SHA-256 `acb92e0807afc554567d99f0e0105cf8d15abb45ce108b064afdc7e64b79638b`。包内 FD SHA-256 `286fd558b9988a7e9c6e9dbde30cf10b9a62a2ba5c0e379db1efbc102ec230`。详细产物、revision、日志与验证见 [主机打包和设备状态](reports/elish-host-package-20261125.md)。
- 当前主机 `lsusb -d 18d1:d00d`、fastboot 37.0.3 `devices -l` 和 `adb devices -l` 均未枚举平板；因此没有发 fastboot 命令、传镜像、重启/复位、`fastboot continue` 或写分区。顶置摄像头 09:07 帧近乎全黑，不据此判断屏幕或设备状态。遵照“设备探测不到时暂停探测”，转做离线核验。
- 主机检查通过：Android v3 image / manifest / FD / payload 哈希一致，`fsck.fat -vn` 未报告错误，`check-elish.py --static-only`、14 区域移植记录检查、`git diff --check`、UniXNU `test_bootimg.py`（1 项）和 `test_boot_template.py`（13 项）通过。XNU `loadable=false`，relocation、register-dataflow、ABI、runtime 资格仍为 false；payload 与 boot manifest 的 `hardware_tested=false`。该镜像本轮没有上机。
- 新核对 UniXNU 的 UFS 资料：catalog `ShengUfs` 限定 SM8550；cupid 的 SM8450 端口也记录了不能直接套用该驱动，并需按 SoC 核实时钟、SMMU 与状态接管。elish 的 Kona GCC/RPMh clock 与 reset ID 已静态映射，但运行时投票、PHY 序列和 UFS stream 仍未知，继续禁用 UFS DMA。不得将 alioth `0xe0/0x4e0` 或 cupid `0xe0` 认作 elish stream。
- 本轮无新启动阶段证据。最近直接确认仍是 run09 近照中的 XNU pre-MMU 阶段；run06/run11 直接日志确认 FAT payload、DarwinBoot、256 MiB 窗口和成功 EBS。MMU 完成、设备端 APFS root 与 iPadOS 用户态未确认。设备重新枚举且 fastboot 控制请求响应后，先 RAM-only 启动 run17（`/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run17-printf-init/boot.img`，SHA-256 `3bcc3f62f519b96a5b56fd47da752a68c168b3f63e5c17c36a3bf8e92c9d6f7e`），再按既有 reader 流程取 root ADB/pstore。run17 尚未启动。严禁从 UniXNU 菜单选择“返回系统”；退出时只直接重启。

## 最新交接 · 2026-11-25 08:50 CST

- 用户确认设备可进入 fastboot，但本轮主机 `lsusb` 和 fastboot 37.0.3 `devices -l` 均未枚举平板；本机记录中的 `/tmp/elish-platform-tools/platform-tools/adb` 路径也不存在。未发送 fastboot 控制请求、未传输镜像、未重启或复位，等待 USB 重新枚举。顶置相机 `/tmp/elish-current-device-state-20261125-0844.jpg`（1920x1080，SHA-256 `8a57ecd29cd38af7ce6a3410dae43b2410a242cb8b61e512fcec2139791673cf`）近乎全黑；临时将相机亮度调至 220 后仍无法辨认，随后恢复默认 128。此画面不证明设备关机或处于任何特定启动阶段。
- 离线检查 XNU revision `d8b80295118ef25ac3a784134bcf95cd8e88109f`：`PE_init_printf(FALSE)` 仅设置 `PE_putc = console_write_char` 后返回。run16 的探针在该调用前等待并复位；run17 设计为调用后等待并复位，能区分是否返回，但不能证明输出后端工作或后续平台初始化成功。run17 镜像未变化，SHA-256 `3bcc3f62f519b96a5b56fd47da752a68c168b3f63e5c17c36a3bf8e92c9d6f7e`，仍待 fastboot 可用后 RAM-only 测试。
- 新增 stock Kona GCC 时钟及 reset 对照：九个 UFS host clocks 已逐项对应到 `qcom,gcc-kona` binding ID 和寄存器偏移；`ref_clk` 实际来自独立 RPMh provider，`core_reset` 是 GCC reset ID 33 / `0x77000`。来源为 stock ROM DTBO entry 12 与 Xiaomi Android R `elish-r-oss` commit `52a2f180865f8368828985d6384130acd093fa93`。这是静态映射，不给出运行时 clock/rpmh 状态、PHY 初始化顺序或 UFS SMMU 所有权；继续保持 UFS/DMA 未启用。详见 [Kona UFS 映射](reports/elish-ufs-kona-clock-map-20261125.md)。
- 最近的直接启动阶段仍是 run09 用户近照确认 XNU 到 MMU 前。run14–run16 时序仅为间接证据；run17 未运行。FAT payload、DarwinBoot、256 MiB 窗口和成功 EBS 仍由 run06/run11 日志直接确认；设备端 APFS 根卷和 iPadOS 用户态未验证。详见 [时序与离线核对](reports/elish-run14-run17-xnu-timing-20261125.md)、[UFS 主机审计](reports/elish-ufs-host-audit-20261125.md)。

## 最新交接 · 2026-11-25 07:53 CST

- run14–run16 的 RAM-only XNU 时序测试补充了间接阶段证据。run14 镜像 `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run14-vm-return-30-image/boot.img`，SHA-256 `499bf02f87858aef0062fbf6dbedecdaec57d9ad9db906ab368079692afbbb6f`；一次运行约 43 秒后重新枚举 fastboot，但后续重传卡在 USB bulk transfer、没有 `Sending OKAY`/`Booting OKAY`，所以不把时序当成直接的 `_arm_vm_init` 返回证明。run15 `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run15-console-create/boot.img` SHA-256 `cfcdf0e2e7d0ff1f384537c9834321f7d15bd008e95a7db0fad940a9097b618b`，在 `_PE_create_console()` 后等待 15 秒再 PSCI reset，约 27 秒后回到 fastboot。run16 `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run16-before-printf/boot.img` SHA-256 `6922981c36442748a8cdd97e657875e9e5409f5ce28f72d9b9c75fae0ecf81d0`，在 `_PE_init_printf` 调用点前等待 15 秒，约 28 秒后回到 fastboot。后两轮时序支持控制流越过 `_arm_vm_init()` 及到达后续调用点，但没有屏幕/串口的直接确认。
- run17 `PE_init_printf()` 后等待 15 秒的镜像已构建：`/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run17-printf-init/boot.img`，67,751,936 字节，SHA-256 `3bcc3f62f519b96a5b56fd47da752a68c168b3f63e5c17c36a3bf8e92c9d6f7e`。两次启动命令返回 `unknown command`，未见镜像被 bootloader 接受；后续 reader 传输卡住。run17 尚未运行，不能证明 `PE_init_printf()` 或更后阶段。
- run14–run16 后通过 root ADB reader 读取的 `/sys/fs/pstore` 为空；ramoops 为 `0xb0000000+4 MiB`、`record_size=0`，内核报告 `Invalid compression size for deflate: 0`。该空目录不能作为标记未执行的证据。XNU 源码快照 revision `d8b80295118ef25ac3a784134bcf95cd8e88109f` 显示 `arm_vm_init()` 在新 TTBR/TLB 切换后还运行 AuxKC、`pmap_bootstrap()` 和 L3 映射；所以当前仍没有 `_arm_vm_init()` 完成的直接证据。run09 照片仍是最新直接阶段证据：XNU 到 MMU 前。
- 用户本轮无法提供屏幕照片。07:53 CST 顶置相机帧 `/tmp/elish-current-state-followup.png`（SHA-256 `a782ae8c5e04db2822d03c06b2da9e934c280386d3d64952aeedc2ce07b18468`）欠曝，不能辨认启动阶段。主机短暂列出 fastboot USB `18d1:d00d`、serial `4bed07e4`，随后只读 `getvar product` 返回 `usb_read failed: No such device`；ADB 为空。此后停止设备协议操作，没有重试传输或复位。未写分区、未运行 `fastboot continue`、未发送关机命令。完整路径、哈希和证据边界见 [run14–run17 时序报告](reports/elish-run14-run17-xnu-timing-20261125.md)。
- 当前最后直接确认的阶段仍是 run09 用户近照中的 XNU MMU 前标记。MMU 完成、UFS/SMMU 所有权、设备侧 APFS 根卷挂载及 iPadOS 用户态未验证。fastboot 控制通道恢复后优先 RAM-only 启动已构建 run17，再用 reader 检查日志；如仍无直接阶段证据，在 `_arm_vm_init()` 内设置窄范围检查点。

## 最新交接 · 2026-11-25 07:12 CST

- 用户确认设备可进入 fastboot，但本轮没有屏幕近照。07:06:55 主机只读快照枚举到 USB `18d1:d00d`、serial `4bed07e4`；ADB 无设备。fastboot 37.0.3 的 `devices -l` 能列出 serial，但随后 `getvar product` 在 8 秒超时（退出码 124），没有返回产品值。当前可见 USB 枚举不代表 fastboot 控制通道可用。
- 未重传 run14、未重启/复位设备、未写入分区，也未发送关机或 `fastboot continue`。run14 候选 `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run14-vm-return-30-image/boot.img`（67,751,936 字节，SHA-256 `499bf02f87858aef0062fbf6dbedecdaec57d9ad9db906ab368079692afbbb6f`）本轮未启动。顶置摄像头单帧 `/tmp/elish-current-state.png`（640x480，SHA-256 `95bb6002b989b62437beb87bce02d492d8a6b6ea9114b850de875e7970416dba`）近乎全黑，不能判断屏幕阶段。
- 本轮没有新增启动阶段证据。最近直接证据仍是 run09 照片确认 XNU 已执行到 MMU 前标记；`_arm_vm_init` 返回、UFS/SMMU 交接、设备端 APFS 根卷挂载及 iPadOS 用户态未确认。主机打包预检解析 63 个补丁、静态板级预检和 14 区域移植记录校验通过，`git diff --check` 通过；这些结果不代表实机启动验证。现有 fastboot 控制请求无响应期间停止进一步设备协议命令，继续离线工作。详见 [fastboot 只读状态复核](reports/elish-fastboot-readonly-status-20261125.md)。

## 最新交接 · 2026-11-25 06:55 CST

- run14 30 秒 `_arm_vm_init` 返回探针重试，镜像 `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run14-vm-return-30-image/boot.img`，67,751,936 字节，SHA-256 `499bf02f87858aef0062fbf6dbedecdaec57d9ad9db906ab368079692afbbb6f`。本次使用系统 `/usr/bin/fastboot` 34.0.5；06:43:25.700 开始传输后停在 `Sending boot.img (66164 KB)`，到 06:43:59 主机进程处于 `usbfs_start_wait_urb`，没有 `Sending OKAY` 或 `Booting OKAY`。Ctrl-C 后进程退出码 130。此前 `getvar product` 正常返回 `elish`，其他查询变量由 bootloader 报 `Variable not found`。
- 失败后 06:44:51 USB 仍枚举为 fastboot `18d1:d00d`，fastboot 进程和 ADB 均未运行。48 秒顶置摄像头录像 `/tmp/elish-run14-overhead.mkv`（640×480，SHA-256 `e7c59fbda0919ed82d642b62725d8f6097d12e19db5ebd526918beb45e4ff6d5`）末帧过暗，无法识别屏幕阶段标记；本轮没有新的启动阶段证据。
- 本次没有传完镜像，未写设备分区、未发 PSCI/关机命令、未运行 `fastboot continue`。最新直接阶段仍是 run09 照片确认的 XNU MMU 前标记；`_arm_vm_init` 返回、UFS/SMMU、APFS 挂载和 iPadOS 用户态未确认。暂停 fastboot 请求并继续离线分析。之前通过的 fastboot 37.0.3 位于 `/tmp/elish-platform-tools/platform-tools/fastboot`；后续设备测试需使用该版本并先确认大镜像传输恢复。详见 [run14 传输复测](reports/elish-run14-fastboot-retry-20261125.md)。
- XNU 源码顺序确认 `_arm_init` 在 `_arm_vm_init` 返回后还经过低区映射、debug inheritance、`serial_init()` 和 kprintf 初始化；run12 标记位于更后的 `_PE_init_printf` 调用，因此缺标记不能单独指向 VM-init。`_arm_vm_init` 本身在新 TTBR/TLB 切换后仍执行 AuxKC、`pmap_bootstrap()` 和 L3 映射，函数返回才代表这段完整完成。另查 UniXNU UKE 设备记录 revision `5ed46420025be331b51d98207ea6aa679faac9c7`：同类 `_arm_vm_init` 复位候选仅构建、未实机验证；其 framebuffer 文本环未产生新标记，记录指出 `PE_state.video` 物理地址与 `video_console` 映射地址不同，不直接复用该屏幕环方案。
- 06:54 使用已验证的 fastboot 37.0.3 对同一 serial 执行只读 `getvar product`，8 秒超时且无输出。06:55:27 USB 仍枚举 `18d1:d00d`，没有 fastboot 进程或 ADB 设备；未重试镜像传输、未做 USB reset 或设备重启。当前是 USB 设备可见但 fastboot 控制请求无响应，停止设备协议探测并继续离线工作。

## 最新交接 · 2026-11-25 06:33 CST

- run12 的屏幕照确认绿色 handoff、蓝色 XNU entry、品红色 MMU 前带；post-MMU 标记不可见。reader pstore 为空，仍有 record_size=0 的 deflate 警告。详情见 reports/elish-run12-postmmu-console-20261125.md。
- 新增 _arm_vm_init 返回探针。run13 的 15 秒版镜像 /root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run13-vm-return-image/boot.img（SHA-256 05b8620d4ccc74e95bccb3fef0edc71946d205036e0fd71da005361a2ff459b9）RAM-only 启动成功；fastboot 在命令结束后约 13.9 秒重新枚举。这个时序接近配置等待值，不能独立证明函数返回。
- run14 30 秒差分镜像已构建，SHA-256 499bf02f87858aef0062fbf6dbedecdaec57d9ad9db906ab368079692afbbb6f；fastboot 传输卡在 usbfs_start_wait_urb，没有 Booting OKAY。中断主机进程后设备仍枚举为 fastboot，随后的 getvar 也卡住。
- 当前设备保持开机并处于 fastboot USB 18d1:d00d；最后一次成功的 product=elish 查询在 run14 传输前。ADB 未枚举。本轮未写分区、未运行 fastboot continue，也没有发关机命令。待 USB 协议恢复后再测 run14。
- 最新直接阶段仍是 XNU 执行到 MMU 前。MMU 初始化返回、UFS/SMMU、APFS 挂载和 iPadOS 用户态都未确认。详情见 reports/elish-xnu-vm-return-isolation-20261125.md。

## 最新交接 · 2026-11-25 06:10 CST

- run12 post-MMU console 镜像已 RAM-only 启动并复测。镜像 `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run12-postmmu-console/boot.img`，67,756,032 字节，SHA-256 `49460e8b7aabf22dc6e921e3435ab47811c9dbd4cfe27b1880a5858d786f324a`；payload SHA-256 `b35392da63b9448093c497cfa7395704dfbcdf8fe554a5dc79c1a9c1a6d180f6`；补丁内核 SHA-256 `5678c2cce3e27df5308ea34936583c5d76ea78c4c84fdd3a6d7c30b6e6a20f42`。它在 `_arm_vm_init` 返回及控制台初始化后打印 `ELISH_POST_MMU_VM_INIT_CONSOLE_READY`，等待 15 秒后 PSCI reset。
- 用户近照 `/root/repo/86D644D8053EE3CF1BB0D8414E2EBB4D.png`（SHA-256 `51420af2d0b488f94776c6e19711e238af87f798a7ddeac91d927f53739aa2d4`）能确认绿、蓝、品红三条带；下方仍是 loader/capsule 固件日志，没有可见 post-MMU 标记。复测中 fastboot 37.0.3 接受镜像（Sending/Booting OKAY，1.760 秒），约 6.9 秒后 USB 回到 fastboot；reader Android 的 arch_timer 为 19.20 MHz，与 15 秒计数基准一致。时序不支持已进入 15 秒等待段，但不能单独定位停止点。25 秒顶置摄像头录像 SHA-256 `c01c3c7582fbe7a7e48b4d152ddd3bc07864590e97b5d6f4c8467bd9baffd52c`，画面太暗，无法辨读屏幕。
- 已用校验通过的 LineageOS reader RAM-only 启动，root ADB 返回 `lineage_elish` / `elish`。`/sys/fs/pstore` 为空；dmesg 显示 ramoops `0xb0000000+0x400000`、`record_size=0` 和 `Invalid compression size for deflate: 0`，没有恢复 run12 检查点。当前设备保持开机、处于 root ADB；活动槽仍为 `_b`。本轮没有写设备分区，也没有运行 `fastboot continue`。
- 最新直接阶段证据仍是 XNU 已执行到 MMU 前。run12 没有证明 `_arm_vm_init` 返回或 post-MMU 控制台可用；停点可能在该函数内，也可能在随后的控制台/检查点路径。UFS/SMMU、设备端 APFS 挂载和 iPadOS 用户态仍未验证。详情见 [run12 报告](reports/elish-run12-postmmu-console-20261125.md)。

## 最新交接 · 2026-11-25 05:08 CST

- run11 只读 context-bank 探针通过 RAM-only fastboot 启动，随后按设计重启回 fastboot；已用哈希核验的 reader 恢复 root ADB 并取回本轮日志。镜像 `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run11-smmu-context-readonly/boot.img`，67,751,936 字节，SHA-256 `9e6dbce96f9551c56265c375a05877d3b0fcbc283b9c03c322987f8ca18f8767`；raw pstore 49,439 字节，SHA-256 `f16b04efae4fb5859b84b7f6351fe970de52d2b89cfaf7f3525a5239a48b8600`。当前 live root ADB 读回与归档同 hash，详见 [SMMU 报告](reports/elish-smmu-readonly-probe-20261125.md) 和 [run11 原始日志](reports/elish-run11-smmu-context-console-ramoops-20261125.txt)。
- run11 扫描 87 个 apps-SMMU group，仅 group 4 有效：SID `0x0820`、mask `0x0402`、S2CR type 0、context bank 4。bank 4 的 `CBAR=0x1f000`（type 1，S1 translation/S2 bypass）、`SCTLR=0xe0`（M bit 0 为 0）、`TTBR0=0`、`TCR=0`、`FSR=0x400`。这没有启用的 S1 页表；FSR 保留原始值。仍不能把 SID 归属到 UFS，也不能据此判断 UFS DMA 已可用。
- run11 再次直接确认 FAT payload、DarwinBoot、256 MiB 窗口 `0xc007c000+0x10000000` 与 EBS Success；key `0x3ee`、accepted snapshot 一致，map 72 条（`0xD80/0x30`）。探针在 EBS 后明确停止，XNU 未进入。综合阶段仍以 run09 用户近照为准：XNU 已执行到 MMU 前，MMU 后未独立确认。
- 本轮没有写入设备分区、未运行 `fastboot continue`。当前设备 Android 13 `lineage_elish`、root ADB、活动槽 `_b`；fastboot 未枚举，`console-ramoops-0` 保存 run11 日志。下一步独立确认 XNU MMU 后阶段；并通过 elish 固件/SoC 来源将 SID/context 和 UFS 对应起来，再核实 UFS 时钟、reset、PHY 与 DMA 路径。

## 最新交接 · 2026-11-25 04:42 CST

- RAM-only 启动 run06 只读 SMMU 探针后，经已校验的 LineageOS reader 取回 `console-ramoops-0`。测试镜像 `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run06-smmu-readonly/boot.img`，67,751,936 字节，SHA-256 `dc552b5eb9a2795f1de63abd3f48ab02003add18d2bd26b153f4af1789d0e24f`；pstore 为 49,328 字节，SHA-256 `fdaa24dcf70d4dbb41d63a49b2c705b8f7c33517128135c8439d56d6a9b1d7b2`。主机归档与当前 root ADB 文件逐字节一致，详见 [SMMU 探针报告](reports/elish-smmu-readonly-probe-20261125.md) 和 [原始日志](reports/elish-run06-smmu-console-ramoops-20261125.txt)。
- run06 直接确认 FAT initrd block device 和未对齐 FAT 子卷已发布；DarwinBoot 打开 `unixnu.toml`、`kernel.macho` 与 ADT，选择 `xnu`，分配 `0xc007c000+0x10000000` 的 256 MiB 窗口。`ExitBootServices()` 返回 Success，接受的 map snapshot 与 key `0x3ee` 一致；`0xD80` 字节、stride `0x30`，共 72 条描述符，最后一条为 `0xB0000000+0x400000` 保留区。run06 探针随后按设计停止，明确记录 XNU 未进入。
- run06 只读扫描 apps-SMMU 的 87 个 stream group，仅 group 4 有效：raw `SMR=0x84020820`、`S2CR=0x4`、type 0、context bank 4。原诊断字符串误把 bit 31 的 VALID 计入 MASK；按 SMMUv2 bits 30:16 正确解码 mask 为 `0x0402`、SID `0x0820`，匹配 `0x0820/0x0822/0x0c20/0x0c22`。此项仍不能归属 UFS。UEFI 探针源已修正解析，修正不在 run06 已测镜像中。
- 本轮无设备分区写入、未运行 `fastboot continue`。当前设备经只读核对为 Android 13 `lineage_elish`、root ADB、活动槽 `_b`；fastboot 未枚举，`/sys/fs/pstore/console-ramoops-0` 仍与 run06 归档同 hash。run09 屏幕仍是最新 XNU 阶段证据：已到 MMU 前，MMU 后未独立确认。下一步确认 MMU 后 checkpoint，再根据 elish 证据确定 SMMU stream/context、UFS 时钟/reset/PHY；不可凭这一个 mapping 启用 UFS DMA。

## 最新交接 · 2026-11-25 04:16 CST

- 本次将 run09 写入活动槽 `boot_b` 进行一次启动测试，之后已用完整备份恢复该分区。两次 flash 均返回 `Sending OKAY` / `Writing OKAY`；原备份 SHA-256 为 `fce50614e0f90a4fd42b1eef26b913712883e4133723b802b4ac65a8dd1dd890`。未运行 `fastboot continue`。
- 用户近照 [run09 屏幕](reports/elish-run09-screen-20261125.jpg)（SHA-256 `ebec55fc542f9cfbc754ae8c36895c69a45e453679206949dec9695d0464f1bd`）同时显示绿色 EBS、蓝色 XNU 入口和品红色 MMU 前阶段条，直接确认 XNU 已执行到 MMU 前检查点。该镜像含有 MMU 后 6 秒 PSCI 复位，但 pmsg 标记未恢复；回到 fastboot 与复位相符，不能单独证明 MMU 已开启。
- 已用哈希核验的 LineageOS reader RAM-only 启动，root ADB 恢复为 `lineage_elish` / `M2105K81AC` / `elish`。`/sys/fs/pstore` 为空；pmsg/console 数据未取回。当前设备处于 reader 的 root ADB 环境，原始 `boot_b` 已恢复。详情见 [入口可视复测](reports/elish-xnu-entry-visual-retest-20261125.md)。
- 后续先查明 pmsg/console 为何未回读及 warm firmware re-entry 是否清除 ramoops，再验证 MMU 后阶段；UFS/SMMU、设备端 APFS 与 iPadOS 用户态仍未验证。

## 最新交接 · 2026-11-25 03:46 CST

- 最新主机快照显示 serial `4bed07e4` 已从 RAM-only 诊断启动返回 `lineage_elish` / `M2105K81AC` / `elish` root ADB，fastboot 未枚举。本轮两次仅 RAM 启动同一诊断镜像，没有写设备分区。测试记录见 [入口可视复测](reports/elish-xnu-entry-visual-retest-20261125.md)。
- 入口可视诊断镜像 `elish-xnu-entry-reset.img`（67,727,360 字节，SHA-256 `441e30dcc04b350c8016668c85cc2f64f32b179fa70bb790b12ff15f69bfd121`）两次均获 `Sending OKAY` / `Booting OKAY`，耗时 1.835 秒与 1.765 秒；约 12 秒后 ADB 回到 LineageOS。两次 root pstore 均为空，deflate 警告依旧。16 秒低分辨率 V4L2 片段只显示暗屏和 `FASTBOOT` 字样，没有可辨认的蓝/品红标记，故 XNU 入口/MMU 仍未确认。未调用 `fastboot continue`。
- UFS 在 Android 下处于 Gear 4、双 lane、FAST/Rate-A，Samsung `KLUEG8UHDC-B0E1`，`pa_err_cnt_total`、`dl_err_cnt_total`、`dme_err_cnt` 均为 0。apps-SMMU 有 87 个 stream-match groups，`ufshc_mem` 日志中的 handle 36 是 interconnect client handle，不是 SMMU stream ID；设备树没有 UFS `iommus`，不能据此选择 alioth stream。
- 已按 LineageOS kernel revision `6b34d24765327d746074c8e53b5ad0d24a51acc7` 核对 pstore 源码：`record_size=0` 令 panic-dump deflate buffer size 为 0，从而出现 `Invalid compression size for deflate: 0`；console ring 仍配置为 2 MiB。该警告不解释空的 `/sys/fs/pstore`，所以 run08 XNU marker 仍未确认，空目录也不能证明未执行。
- 目标距离判断：run08 画面确认到 DarwinBoot 的绿色 handoff；run04 日志直接确认成功 `ExitBootServices()`、256 MiB 窗口 `0xc007c000+0x10000000`、key `0x3ee`、73 个描述符。XNU 第一条指令、MMU/内存交接、UFS/SMMU、设备端 APFS 挂载与 iPadOS 用户态都没有实机证据。还需跨过多个核心启动门槛，不能按单个故障修复估算。详见 [复核报告](reports/elish-pstore-ufs-live-20261125.md)。

- 用户报告设备已启动到 fastboot。初次主机枚举序列 `4bed07e4`，USB `18d1:d00d`，`fastboot getvar product` 返回 `elish`。
- 此前旧版 fastboot 的 reader 传输曾卡在 `usbfs_start_wait_urb`；改用已校验的 fastboot 37.0.3 后 reader RAM-only 启动成功。Reader 为 55,083,008 字节，SHA-256 `57dbb98dec813b2a8c7bbd67a59f715aa1abf366905a24270aba594ec4ca44c4`。ADB 恢复为 `lineage_elish`，`adb root` 成功。
- 用户报告设备再次进入 fastboot 后，序列 `4bed07e4` 枚举为 `18d1:d00d`，37.0.3 的 `getvar product` 返回 `elish`。修正版 run08 以 RAM-only 方式启动，`Sending OKAY` / `Booting OKAY`，耗时 1.760 秒；boot.img SHA-256 `9cd0577439a5f143ff11d81cdaa6aece79da01a25cfd5cfdf67ddea0a7a5075a`。约 10 秒后主机未见 ADB、fastboot 或 USB 设备，停止探测。
- 用户照片 `/root/repo/D5E326E8269942AF9C00495CFD941302.png`（SHA-256 `2d94beff27bbe43fd9f7d5832303debd0bb3b9946c2935d8e791921a6dde4a2c`）可读到 `Exiting boot services. A green bar across the top marks the loader handoff before the kernel branch.` 这确认屏幕停在成功 EBS 后、XNU 分支前，不确认 XNU 第一条指令。
- 用户随后提供近照 `/root/repo/21A164FDA8C43322023FEFA8F6331EAF.png`（SHA-256 `1fbd8b10ec5bd3c17d7ebc960d909ee9232a8929d549c740e7179de224a6f9a4`），归档为 `reports/elish-run08-screen-20261125.jpg`。照片显示绿色 EBS handoff 画面；屏幕阶段不等于 XNU 入口。V4L2 仅用于确认大致画面状态，不用于辨读日志。
- 之后用户再次报告 fastboot，主机枚举 `4bed07e4` 并核验 `product=elish`；fastboot 37.0.3 成功 RAM-only 启动 reader。root ADB 的 `/sys/fs/pstore` 已挂载但为空；dmesg 有 `ramoops: attached 0x400000@0xb0000000`、pstore backend 注册和 `Invalid compression size for deflate: 0`。未取回 run08 marker；压缩元数据异常使空目录不构成未执行证明。当前设备经 reader 回到 root `lineage_elish`。
- Run08 镜像、构建输出、pstore 结果与照片观察见 [run08 报告](reports/elish-xnu-entry-checkpoint-run08-20261125.md)。
- 本轮离线 NativeUfs 主机测试：GPT/root layout、窗口读取、root 发布和合成 SMMU 测试通过；临时修正测试 shim 后，QEMU ARM64 命令测试 5 个 DMA 情况和 14 个历史描述符比较均通过。超时诊断为合成故障注入。测试只验证主机逻辑，不提供 elish UFS/SMMU 硬件证据。结果见 [UFS host audit](reports/elish-ufs-host-audit-20261125.md)。
- 阶段边界：run04 仍是最后一份持久日志直接记录 `ExitBootServices()` 成功的运行，256 MiB 窗口 `0xc007c000+0x10000000`、key `0x3ee`、73 描述符。run08 RAM-only 启动后屏幕显示绿色 handoff；没有 XNU 第一条指令、MMU 开启、UFS/SMMU、APFS 挂载或用户态的直接证据。本轮没有设备分区写入。

目标仍是在 elish（M2105K81AC，SM8250-AC）上运行 iPadOS 17.7.11（21H461），当前未完成。以下是最新实测结果；后面的日期章节保留历史观察，不能替代本节。公开命令使用 `ELISH_SERIAL` 占位符。

- **FAT 配置发现已修复。** ABL 将 18,811,579 字节 gzip vendor ramdisk 拼在 64 MiB FAT 前，偏移 `0x11f0abb` 不按扇区对齐。运行时探针直接记录了 gzip 首字节和 FAT 偏移。共享 UniXNURamDiskDxe 现在保留完整原始 initrd，并发布只读 FAT 子卷；DarwinBoot 支持从相对偏移子卷验证完整父卷。详见 [本轮报告](reports/elish-loader-fat-initrd-20261124.md)。
- **DarwinBoot 和成功的 EBS 已直接确认。** `run04-fat-child/boot.img`（SHA-256 `8ef2e0a78c37171b74d50601703aa94b2f8ec7ed7a1aed00fbb0aebfb2bb33e7`）读取 FAT 的 `unixnu.toml`，三秒后自动选择 `xnu`。DarwinBoot 取得 `0xc007c000+0x10000000` 窗口，EBS 返回 Success，key `0x3ee` 与固定缓冲快照一致，73 条描述符已保存。该探针在 EBS 后主动重启，**没有执行 XNU**。[完整日志](reports/elish-darwinboot-ebs-20261124.txt)
- **run05 检查点仍待 pstore。** `run05-xnu-before-mmu-inputs/firmware/boot.img`（SHA-256 `14c3bf7d50c7e5fb35151d312efa144bdaf648a557fcaf098c2044193b191b32`）曾在 EBS 后放行 XNU，之后未返回 USB；该轮 ramoops 标记尚未知，不能声称检查点执行或内核启动成功。
- **2026-11-25 屏幕与入口复测。** 用户照片 `/root/repo/FCAB2A776FDB44AE9E8A5920E572CC64.png`（SHA-256 `c1eb7499267cf31e0d69ce29396d2d5dac20759be94b6f0ad0118f1a35a82fc9`）显示 DarwinBoot 的 256 MiB 窗口、`entry words 140006F5 ...`、CPU 状态与绿色交接带。绿带确认 `ExitBootServices()` 后到达 DarwinEnter；照片没有蓝/紫入口条，不能确认 XNU 第一条指令执行。照片对应的启动镜像未知，不归属于 run05。
- fastboot 实际枚举 `18d1:d00d`，序列 `4bed07e4`，`getvar product=elish`。用已核验的 reader 镜像 `backups/lineage-reader-boot.img`（SHA-256 `57dbb98dec813b2a8c7bbd67a59f715aa1abf366905a24270aba594ec4ca44c4`）RAM 启动后，ADB 识别 `lineage_elish` / `M2105K81AC` / `elish`；`adb root` 成功，`/sys/fs/pstore` 为 `total 0`。run05 检查点状态仍未知，空 pstore 不能证明未执行。
- 之后 RAM 启动六秒可视化 XNU 入口镜像 `out/elish/ipados-17.7.11-21H461/elish/boot/diagnostics/elish-xnu-entry-reset.img`，SHA-256 `441e30dcc04b350c8016668c85cc2f64f32b179fa70bb790b12ff15f69bfd121`。Fastboot `Sending`/`Booting OKAY`；ADB 随后回到 `lineage_elish`，重新 root 后 pstore 仍空。主机没有屏幕捕获；用户对蓝/紫条的观察待回复，因此 XNU entry/MMU 仍未确认。没有调用 `fastboot continue`，没有写设备分区。详见 [入口复测报告](reports/elish-xnu-entry-retest-20261125.md)。
- 用户随后报告设备已进入 fastboot；主机紧接着复核时 `fastboot devices -l` 为空，`adb devices -l` 仍显示 `4bed07e4` 为 `lineage_elish` / `M2105K81AC` / `elish`。root ADB 的 `sys.boot.reason=bootloader`，`/sys/fs/pstore` 无文件。这只确认经 bootloader 启动的 Android，不确认采样时处于 fastboot；本轮未发送重启或 fastboot 命令。入口照片只显示 EBS 后绿带，没有蓝/紫条。详见 [入口复测报告](reports/elish-xnu-entry-retest-20261125.md#host-check-after-the-users-fastboot-report)。
- **run07 入口 ramoops 检查点已 RAM 启动。** 镜像 `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run07-xnu-entry-checkpoint/boot.img`（67,751,936 字节，SHA-256 `38b548e03d2a0156f443e010cfdecca2becd7bc745f911a56dbb99e1b99c1e37`）获 fastboot `Sending OKAY` / `Booting OKAY`。入口检查点仅在 `SCTLR.M=0` 且 ramoops 环形缓冲区头有效时写标记，否则直接 warm reset。之后 USB 出现 `18d1:4e11`，又出现 fastboot `18d1:d00d` 并再次断开；没有取得本轮 pstore，时序不能证明标记执行。报告记录检查点代码哈希、FD 和 payload 哈希及 USB 时序：[run07 报告](reports/elish-xnu-entry-checkpoint-20261125.md)。
- **入口检查点 writer 已完成离线核对。** 对照 Linux v4.19 `persistent_ram_buffer` 与 elish `RamLogWrite` 后，确认 `start == size` 表示 console ring 未环绕。修正了生成脚本在 marker 恰好写到容量末尾时未把 `start` 回绕到零的边界。原 run07 镜像未改（260 字节检查点）；修正后在临时内核副本上生成 276 字节检查点，kernel SHA-256 `0ad2ca17724f66adf25afd473008e9c47842c2b00d55d8b1ec79ba6c40b9086f`。修正版随后构建为 run08 并已 RAM-only 实测，但未从 pstore 找回标记。详见 [run07 报告](reports/elish-xnu-entry-checkpoint-20261125.md#ramoops-format-audit-and-tool-correction) 与 [run08 报告](reports/elish-xnu-entry-checkpoint-run08-20261125.md)。
- 先前一次 USB 快照没有枚举到设备；用户再次报告进入 fastboot 后，本轮枚举并核验了 `product=elish`。reader 传输后 USB fastboot 查询也出现不一致响应，尚未取回 pstore；用户重连 USB 前不再提交镜像。
- **UFS/SMMU 离线准备已构建。** 新的只读 SMMUv2 DXE 探针默认关闭，不写 SMMU/UFS 寄存器，只读取 apps-SMMU ID0/ID1 和有界的有效 SMR/S2CR。`run06-smmu-readonly/boot.img` SHA-256 `dc552b5eb9a2795f1de63abd3f48ab02003add18d2bd26b153f4af1789d0e24f`，FD SHA-256 `6067d8dcc754b6a8f6fa71ec8bc5e6fc8bdc9c319b06a4addf1c02d72ab754b5`。构建成功但尚未在设备运行。先完成 XNU 入口观察，再运行此探针并在下一次固件启动前读取 pstore。详见 [SMMU 探针报告](reports/elish-smmu-readonly-probe-20261125.md)。
- **日志读取使用 reader 镜像。不要运行 `fastboot continue`。** 它曾导致 USB mass storage 或关机。活动槽 boot_b 和 vendor_boot_b 已备份并双端核对哈希，reader 只去除了 boot_b 尾部填充。run03/run04 均在枚举到 fastboot 并核验产品后，用 `fastboot boot backups/lineage-reader-boot.img` 恢复 LineageOS，再 `adb root` 读取 pstore。不要选择 UniXNU 菜单的“返回系统”；需要退出时直接重启。进入 bootloader 通常需约 10 秒，必须依据实际枚举，而非五秒延迟。
- **本轮设备分区写入为零。** 原始候选仍是 `out/elish-resume/.../boot/boot.img`（SHA-256 `c6581b4dcf531a55d6ad843c8ae8f3db41d6a24fe831b836fdaf3325b12c11fd`）。其 FAT 内容已经用修正固件实机读取，但其内核没有获得新运行资格；kernel.json 的 `loadable=false` 等字段仍保留。UFS/SMMU、APFS 根卷、用户态、触控、GPU 均未验证。

### 工作区、补丁与产物

| 用途 | 位置 |
| --- | --- |
| 设备仓 | `/root/repo/UniXNU-elish`，基线 `4b3da68ea0aca815a80e8d1bad802dfec8d5c905` 加本轮未提交改动 |
| UniXNU 核心 | `/root/repo/UniXNU` 指向 `/root/elish-build-20261123/UniXNU`，基线 `71c5c6569bae6369c0e0f634df8b9bd5afc19139`，保留原有未提交构建改动 |
| DarwinBoot 子模块 | 核心 `drivers/DarwinBoot`，基线 `5243c68ec332a227fe3680cd5da6d5a0e127035c` 加父子卷校验和测试改动 |
| 固件 | `/root/repo/uefi-elish`，基线 `f03839d31a3f07e9e4b032fcf850ac0dfb9961a0`；Mu_Basecore `cfb93724a06ff86ff5ba971413368a7809ee42a5` |
| 本轮输出根 | `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/` |
| 已确认 EBS 镜像 | 输出根 `run04-fat-child/{boot.img,SILICIUM_UEFI.fd,probe.json,device-commands.json,console-ramoops.raw}` |
| 最新 XNU 阶段证据 | run09 `run09-xnu-pmsg-entry/boot.img`；用户近照确认到 MMU 前阶段，pmsg 未读回 |
| 最近 RAM-only 测试镜像 | `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run11-smmu-context-readonly/boot.img`；见本节报告与 SHA-256 |
| 最近回读 ramoops | `reports/elish-run11-smmu-context-console-ramoops-20261125.txt`；49,439 字节，SHA-256 `f16b04efae4fb5859b84b7f6351fe970de52d2b89cfaf7f3525a5239a48b8600` |
| 最近一次恢复 pstore 的方法 | 用 fastboot 37.0.3 RAM-only 启动 `backups/lineage-reader-boot.img`，再 `adb root` |
| SMMU 只读探针产物 | run06 `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run06-smmu-readonly/`；run11 `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run11-smmu-context-readonly/`；均已实机读取并归档 pstore |
| 备份与 reader | 输出根 `backups/`。reader SHA-256 `57dbb98dec813b2a8c7bbd67a59f715aa1abf366905a24270aba594ec4ca44c4` |
| 新补丁 | `patches/unixnu-fat-initrd.patch` 在核心根应用；`patches/darwinboot-fat-initrd.patch` 在 DarwinBoot 子模块根应用。当前工作区已应用，勿重复应用 |
| 本地输入 | 官方 ROM 在 `/root/repo/elish_os1.0.2.0`；核心被忽略的 `unixnu.local.toml` 保存 IPSW 和平台种子，勿输出或提交其内容 |

`board.toml` 的常规 `ELISH_LOADER_PROBE=FALSE`。`scripts/build-loader-probe.py` 创建独立影子板配置，不覆盖旧测试产物；默认在 EBS 后重启。只有 `--run-kernel` 才会放行终止型 XNU 检查点。探针超时为 20 秒，120 秒是另一份旧内存探针的参数。`scripts/patch-xnu-ramoops-checkpoint.py` 只对复制内核应用终止型检查点。

### 接手顺序

1. 当前设备为 root `lineage_elish`。`Invalid compression size for deflate: 0` 由 ramoops `record_size=0` 导致 panic-dump 压缩缓冲区为零；它不解释 run09 pmsg 未恢复。run06 的 `console-ramoops-0` 已由 reader 成功取回并与归档完全一致。离线核对发现 debug SEC 入口会调用 `RamLogReset()` 并重置 ring header，仍需区分不同启动路径下 console 与 pmsg 的生命周期。不要把空目录、USB 返回时间或绿色 handoff 屏幕当作 XNU 入口证据。
2. 新 board 已启用 `payload.adt.loader_inputs=true`。run04 没有该属性；run05 Apple ADT 已主机解析并核对 `/chosen/darwinboot-loader-inputs` 的 64 个零字节，内核哈希 `91fa2b034920a7beb8b8892d3067236875140634699b3bdd00ba6184e07249ed` 与 FAT 回读一致。设备端的输入记录仍待 pstore。
3. run09 用户近照确认 XNU 入口及 MMU 前阶段；run06 SMMU 探针已实机运行并通过 reader 取回 pstore。顶置 V4L2 只判断整体状态，不能辨读屏幕日志。
4. 先独立确认 MMU 后 checkpoint，再用 elish-specific 证据把 SMMU stream/context 映射到 UFS，并核实 SM8250 时钟/reset/PHY。之后推进 UFS、GPT/APFS 和 launchd；userdata 几何为 LBA `6619136–61870314`、4096 字节块。不可套用 alioth stream 或 SM8550 时钟值。

## 2026-11-24 崩溃恢复后的主机检查

- 用户表示可接入 root/recovery ADB；本机复查时 `adb devices -l`、`fastboot devices -l` 和 `lsusb` 均未枚举到 elish，尚未进行新的设备读取或启动操作。已通知用户恢复 USB ADB 枚举后继续。
- 临时建立 `/root/repo/UniXNU -> /root/elish-build-20261123/UniXNU`，用于满足 `board.toml` 的相对核心仓路径。`./scripts/package.sh --check --os ipados-17.7.11-21H461` 成功，依赖、IPSW、defconfig 和 62 个补丁解析通过。此命令没有构建 payload 或最终镜像。
- `python3 scripts/check-elish.py --static-only`、`git diff --check`、`python3 -m py_compile scripts/check-elish.py` 和 `python3 scripts/porting-record.py check --device /root/repo/UniXNU-elish` 通过。静态预检仍报告 DRAM/reserved ranges 与 UEFI/XNU 所有权未对齐。
- 在核心仓创建了被 `.gitignore` 忽略的 `unixnu.local.toml`，固定当前设备、UEFI、EDK II、IPSW、kernelcache、live FDT、实验内核和 FD 路径。本地配置现包含非零 8 位十六进制平台软件身份种子，文件权限为 0600，值未进入 Git 或日志。
- 2026-11-24 离线 ROM 存储审计：从 stock vendor_boot v3 拆出 Kona v2.1 基础 DTB，从 DTBO entry 12（elish board ID 47）应用 overlay，确认官方树中的 UFS/PHY、ICE 窗口、中断、时钟和 reset 输入。ShengUfs 官方组件声明 SM8550 范围；NativeUfs-alioth 参考包含 SM8250 身份和 0xe0/0x4e0 stream 配置，但没有 elish 运行结果，stock elish DT 也没有 SMMU stream 属性。报告位于 `reports/elish-stock-storage-audit-20261124.md`。本轮按用户要求停止设备探测，没有运行 ADB、fastboot 或 USB 枚举命令。
- 当前产物仍只是实验 XNU 候选、UEFI 固件和空 ramdisk 探测镜像。内核 `loadable=false`，UEFI/XNU 内存交接未验证，elish UFS/SMMU 与设备端 APFS 根卷路径未完成，也没有 iPadOS payload 或最终启动镜像。

## 2026-11-24 DXE FV 读取状态修复

- 设备照片 `/root/repo/946BF84D23D5AC424DD564A47EFE82EA.png` 显示两个 FV HOB 的 FVB 安装都成功，但 `FwVol` 读取两个 FV 头均返回 `EFI_ACCESS_DENIED`。Mu_Basecore 的 `FwVolBlockReadBlock()` 会在 FV 头 `EFI_FVB2_READ_STATUS` 位未设置时返回该状态，导致没有 FV2、DXE 驱动没有调度，并触发缺少架构协议断言。
- `uefi-elish/Platforms/Xiaomi/elishPkg/elish.fdf` 的 `[FV.FvMain]` 与 `[FV.FVMAIN_COMPACT]` 已设置 `READ_ENABLED_CAP=TRUE`、`READ_STATUS=TRUE`。输出 FV 属性为 `0x000C0C06` 和 `0x00030C06`。临时 Mu_Basecore 跟踪日志已撤销，永久 FDF 修复保留。
- 修复版 RAM-only Android v3 镜像路径为 `/root/elish-build-20261123/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/elish-dxe-fv-readable.img`，SHA-256 `3b1044c711dc5c6b0ee2ddf6222faf45dc2a00fa124407e9c4a136abcbf52ab0`。`fastboot boot` 返回 `Sending`/`Booting OKAY`，用户报告进入原系统；主机之后通过 ADB 看到 `product:lineage_elish`、`model:M2105K81AC`、`device:elish`。该镜像没有 iPadOS payload，也没有进行分区写入。
- 先前的内存图探针使用 `DEBUG_INFO`，elish DEBUG 的 `PcdDebugPrintErrorLevel=0x8007EE0F` 不包含该位。现已将探针入口、ReadyToBoot 注册状态、RAM Partition 表及 UEFI map 行改为 `DEBUG_ERROR`。修正版 622,592 字节镜像 SHA-256 为 `75bbfd228f57b22fdc1bd03349be01720ab9fac3b31cd392900ae874638ddbae`，显示停留 120 秒。用户离开期间不再进行设备操作。
- 当前 RAM Partition 表和 UEFI 描述符仍未捕获，XNU 内核仍 `loadable=false`，尚无 iPadOS payload 或最终镜像。命令、源码 revision、构建记录及结果见 `reports/elish-dxe-fv-read-status-20261124.md`。

## 2026-11-24 XNU 内存范围回移

- `patches/xnu/elish-memory-excluded-ranges.patch` 回移 DarwinBoot 的 `/chosen/darwinboot-memory-excluded` 契约到目标 XNU 10063.141.1。实现校验并裁剪区间，按 XNU 页大小对齐、排序合并，保护内核早期分配，并让 pmap 页分配、线性页数组、physmap 和内存统计忽略 excluded pages。
- 设备仓正常入口 `./scripts/package.sh --check --os ipados-17.7.11-21H461` 通过，解析 63 个补丁。XNU/kernel 与 XnuPrelink 构建完成；产物和哈希见 `PLAN.md` 与 `reports/elish-memory-handoff-xnu-backport-20261124.md`。Mach-O parser test 通过，XnuPrelink 65 项主机测试通过。
- 记录该次回移时，`board.toml` 尚未请求 excluded-range 表，DTS 仍无 memory node。当前配置和后续主机构建已变化，运行时 UEFI/XNU 内存所有权仍未验证。内核候选仍有 `loadable=false` 及 relocation、ABI、runtime 等未资格化字段。

## 2026-11-24 XNU 入口探针

- 查阅 `/root/UniXNU` 的 boot-chain、payload、bring-up、DarwinBoot、debugging 和 XNU boot argument 文档。完整 UEFI 内存图仍未取得，但这不是唯一阻塞：当前内核 `loadable=false`，内存 ownership 未验证，elish XNU UFS/SMMU 和 APFS 根卷路径也未完成。
- 在 `/tmp` 的内核副本上运行 `patch-xnu-entry-probe.py --reset-delay 6`。此诊断内核带 stage 1/2 蓝、品红入口条和 MMU 后 stage 3 复位，不带 panic screen。panic screen 与 reset probe 同时启用会超过内核 padding，补丁工具拒绝该组合。
- RAM-only fastboot 启动镜像 `/root/elish-build-20261123/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/diagnostics/elish-xnu-entry-reset.img`，SHA-256 `441e30dcc04b350c8016668c85cc2f64f32b179fa70bb790b12ff15f69bfd121`。Fastboot 总耗时 1.900 秒并接受镜像。18:25:34 fastboot USB 枚举，18:25:38 USB 断开，18:25:51 Android USB 重新枚举。约 17 秒的整体时间与 3 秒菜单倒计时和 6 秒内核探针相符，间接支持已到 MMU 后检查点，但没有屏幕或串口直接确认。Android ADB 状态为 `unauthorized`。
- 已构建但尚未运行 `_arm_vm_init` 调用返回探针镜像 `diagnostics/elish-xnu-arm-vm-return.img`，SHA-256 `d206d18d7a8bd4f8cdadb46a0e42dd3bf578baa2e6961384417fb0e42ad57e3f`。设备重新提供 fastboot 后可用该镜像检查更靠后的 XNU 阶段。
- 完整命令、revision、镜像哈希及证据限制见 `reports/elish-xnu-entry-probe-20261124.md`。所有设备启动均为 RAM-only，没有刷写分区。

## 构建主机

- 用户授权使用 Debian 构建主机 `ELISH_BUILD_HOST`，工作区 `/root/elish-build-20261123/`，核心在 `UniXNU/`，设备仓在 `device/`。凭据不要写进仓库或日志。当前 BatchMode SSH 探测返回 `Permission denied (publickey,password)`；这没有测试用户提供的密码，也没有运行远端命令或修改远端状态。
- 当前核心 revision：`71c5c6569bae6369c0e0f634df8b9bd5afc19139`；设备仓 HEAD：`208b47d20e87d3876fd862bb1f0ee0c17467b0c2`。设备仓的 `board.toml`、`package/LinkStubs.c`、`porting.toml`、`scripts/package.sh` 和 `unixnu.lock` 有未提交改动，`HANDOFF.md` 和 `PLAN.md` 未跟踪。内核构建使用了设备配置和 LinkStubs 的工作区内容，尚未形成可引用的完整设备源码提交。
- 已在远端核心目录成功运行 `./unixnu fetch -D ../device`（生成远端 `unixnu.lock`）及 `./unixnu release -D ../device --check --no-deps --os ipados-17.7.11-21H461`（解析 62 个补丁，写出 `out/elish/ipados-17.7.11-21H461/elish/.config`）。不是编译验证；设备工作树不干净，未标为 `host-verified`。本地更新后的 `porting.toml` 用远端核心的 `scripts/porting-record.py check` 检查通过，覆盖 14 个区域。
- 2026-11-23 在核心 checkout 成功重跑 XNU 构建：`./unixnu build kernel -D /root/repo/UniXNU-elish --os ipados-17.7.11-21H461`。XNU 编译和 `XnuPrelink` 均完成，产物 `/root/elish-build-20261123/UniXNU/out/elish/ipados-17.7.11-21H461/elish/kernel/kernel.macho` 大小为 `51122176` 字节，SHA-256 为 `759574c4d0a6c8cbd72bf8a8e1026c59390712d1a819a826379a0512d21ed8ec`。配套 `kernel.json` 标记 `instruction_relocation_complete=false`、`register_dataflow_qualified=false`、`runtime_qualified=false`、`abi_qualified=false`、`loadable=false`，因此它是实验候选，不代表设备可启动。核心 revision 为 `71c5c6569bae6369c0e0f634df8b9bd5afc19139`，设备 HEAD 为上述基线且工作树有未提交修改。日志位于输出目录的 `logs/kernel.log`、`logs/prelink.log` 和 `logs/kernel-bootstrap.log`。
- Apple IPSW 缓存位于核心 checkout 的 `.cache/firmware/iPad_Pro_HFR_17.7.11_21H461_Restore.ipsw`，SHA-256 为 `c84ad24b6981a859eec499b1802be9ac593acfd3dc0a185a76d06e4d767a1451`。Profile 与 IPSW 信息对应 `21H461`、`iPad7,1`、`j120ap`。
- `./scripts/package.sh --check --os ipados-17.7.11-21H461` 完成依赖、IPSW 输入、defconfig 和 62 个补丁检查。当前设备仓与核心 checkout 不在相邻目录，执行时临时在 `/root/repo/UniXNU` 建立指向核心 checkout 的符号链接以满足 `board.toml` 相对路径约定，命令结束后已移除。userdata GPT 检查通过。此后 UEFI 包在主机成功构建，`uefi_platform` 已标为 ready；严格 `scripts/check-elish.py` 仍因 UEFI/XNU 内存所有权未经实机验证而拒绝 payload/镜像构建。
- 2026-11-24 完成 Mu_Basecore pinned submodules 初始化和 BaseTools 编译：`make -C BaseTools/Source/C -j8`。随后 `./unixnu build firmware -D /root/repo/UniXNU-elish --uefi /root/repo/uefi-elish --edk2 /root/repo/uefi-elish/Mu_Basecore -j 8` 成功，生成 `/root/repo/uefi-elish/Build/elishPkg/DEBUG_CLANGPDB/FV/SILICIUM_UEFI.fd`，3,145,728 字节，SHA-256 `f8f662313c0ffee611a2e55f687486b1625c9e80a983d9280f26f46f5dbdbf74`。`firmware.log` 有一条 `ArmPsciResetSystemLib.inf` 不支持 `HwResetSystemLib` 的非致命警告。完整依赖、命令和输出记录见 `reports/elish-uefi-build-20261124.md`。
- 使用 `/tmp/elish-uefi-probe.dtb` 中刚从 live root ADB 导出的 FDT，运行 `./unixnu bootimg -D /root/repo/UniXNU-elish --uefi /root/repo/uefi-elish --fd /root/repo/uefi-elish/Build/elishPkg/DEBUG_CLANGPDB/FV/SILICIUM_UEFI.fd --dtb /tmp/elish-uefi-probe.dtb --output /root/elish-build-20261123/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/elish-uefi-probe.img` 成功，输出 618,496 字节，SHA-256 `8b08c449b4af3a4ab13c07389a789c170a83d6c373b68dfe76dc959224fef349`。`boot.json` 记录 `ramdisk_decoded_bytes=0`、`hardware_tested=false`，因此这只是固件探测镜像，不包含 iPadOS。
- 2026-11-24 新增内存诊断固件：`ElishMemoryMapProbeDxe` 在 ReadyToBoot 读取 Qualcomm RAM Partition protocol 和当前 UEFI DDR 描述符，通过 framebuffer debug logger 输出并停留 30 秒。新 FD 为 3,145,728 字节，SHA-256 `8f797816d1b3ac59338c248a7ed17442d4f3878e5abc2fae80314811d6967548`。RAM-only Android v3 探测镜像为 622,592 字节，SHA-256 `b2ccb5968d7cb30a462bf0f1c6deca84167ccb10c7f7a36782e07559c0fbf6d6`。它在 UniXNU 启动前采样，不代表 ExitBootServices 最终 map。第二段 DRAM 结束地址已按 live FDT 修正为 `0x280000000`，早期构建哈希作废。通过 `fastboot -s ELISH_SERIAL boot` 发送后，Sending 和 Booting 均返回 `OKAY`；随后设备从 USB 枚举消失，30 秒后仍未重新枚举。主机未捕获 framebuffer，无法确认 DXE callback 是否运行或取得内存数据。没有执行分区写入。另有 120 秒版 FD SHA-256 `063614fdc3de0d2e7ab7a26bb7856c684fa4d4c45e95dfe83e8065fe31d1b2b1` 和 622,592 字节镜像 SHA-256 `f312ddfa2adacd2a48ef3d09e313765b668b1affe7e710426b3e3411fcd1e650`，尚未在设备运行。详细命令和结果见 `reports/elish-uefi-build-20261124.md`。
- 前一轮 root ADB 连接 Android (`M2105K81AC`) 时，`/proc/filesystems` 未列出 debugfs，`/sys/kernel/debug` 和 `/dev/mem` 不存在，因而无法从 Android 导出 SMEM RAM Partition table 或 UEFI `GetMemoryMap`。严格 `check-elish.py` 现只剩 memory handoff blocker；`uefi_platform` 已按成功的主机编译标为 ready，`memory_map` 仍为 unverified。
- `scripts/check-elish.py --static-only` 与 `scripts/porting-record.py check --device /root/repo/UniXNU-elish` 通过。`dtc` 曾将 DTS 编译到 `/tmp/elish-check.dtb`，SHA-256 为 `34e4b5d32942f829d1e44211cc7939b0265d25ff41dd4019f820939ee81d3293`。
- 当时远端没有 elish UEFI checkout 或固件缓存。更新 GPT 输入前，`python3 scripts/check-elish.py` 严格模式因内存图、userdata GPT 与 UEFI 未就绪退出 1。更新后 userdata 几何已通过预检，内存 handoff 和 UEFI 平台仍未就绪。`release --check` 不等于通过此板级检查。
- 前次崩溃恢复后 root ADB 重新导出的 live FDT SHA-256 与已有记录一致，仍为 `90cf1d5333e758f001a8e21ed4e78fc000ebf3b5502338f010435a2da292a663`。核心 checkout 新增 `scripts/unixnu/test_bootimg.py`，并修正 `bootimg.py` 读取 `[boot_image_kernel]`、控制 DTB 追加及验证报告。`python3 scripts/unixnu/test_bootimg.py` 和 `python3 scripts/unixnu/test_boot_template.py` 均通过。

## 固件输入与分析进度

- 官方 stock ROM 镜像目录现位于 `/root/repo/elish_os1.0.2.0/`，含 28 个 `.img` 文件（约 8.2 GB），包括 `xbl.img`、`xbl_config.img`、`imagefv.img`、`abl.img`、`vendor_boot.img`、`boot.img`、`dtbo.img` 和 `vendor.img`。镜像与 Apple IPSW 均不放入 Git 或发布包。
- `xbl.img` SHA-256：`9a3a3bb92e6cf478b227f70dd1bd140fde32982994a6c34a07bd314ea0acb72f`。只读解析确认 FV 位于 `0x888b0`，长度 `0x270000`、头长 `0x48`、结束偏移（不含）`0x2f88b0`；`_FVH`、边界与 FV 头校验和（0）均正确。外层 FFS 有一个 UI 名为 `uefiplat.cfg` 的自由格式文件，原始配置含 `PlatConfigFileName = uefiplatLA.cfg`。
- XBL 中的 `uefiplat.cfg` 给出 UEFI FD `0x9fc00000+0x300000`、stack `0x9ff90000+0x40000`、DXE heap `0x98900000+0x03300000`、Display Reserved `0x9c000000+0x02400000`。live FDT 的同名 display 区域为 `0x9c000000+0x02300000`，相差 1 MiB。XBL 引用 `uefiplatLA.cfg`，但其内容、选择路径和完整 UEFI 内存图仍未解析。细节已写入 `/root/repo/uefi-elish/Platforms/Xiaomi/elishPkg/PORTING.md`。
- `uefiplat.cfg` 的版本 3 `[MemoryMap]` 有 `MaxMemoryRegions=74`，嵌入文本列出 22 个 DDR 条目，但最高只到 Kernel reservation `0xa0000000+0x10000000`，没有第二 DRAM bank 的描述。其 PIL 项为 `0x86000000+0x0d200000`，live FDT 的 PIL/secure entries 覆盖 `0x86200000–0x92700000`；XBL `DBI Dump` `0x9e400000+0x00f00000` 标记 `NoHob/MMAP_IO`，Linux `/proc/iomem` 却报告该段为 System RAM。该表是嵌入配置，不代表实际加载的 UEFI `GetMemoryMap`。完整静态条目和差异记在 UEFI `Platforms/Xiaomi/elishPkg/PORTING.md`。
- ROM 输入哈希：`dtbo.img` `4d797a4d3ea9ea16168128a9c189d7f32bf21bd1b049205fe39959a90046b9d7`；`vendor_boot.img` `63dbb687d39b4d3c12aae6828fdd414d15e41dbec74f48ec56e2b23195a3a467`；`boot.img` `336f36fea4b386af81be542f6573098d493036f1ed9bd3bd7d59f41bef2487df`；`xbl_config.img` `f290260d3c38c9c93db5790c6550846871f5c3dbc1607433af52c1cd19e184bd`；`imagefv.img` `4b354ee876cb2518da70f94cafad6deff5ceeddf1891199860c3669fd6e7cf0e`；`abl.img` `53e95850b306178b46f96abe68ca0f64952a3f43abffffc3218425a1527fbe8c`；`vendor.img` `43ca4c47c7eec355cbbf9b3ba0d5ebf7a609fdaeda7b6ad7b0bffba5511c129c`。
- `vendor.img` 是 ext4；其中 `build.prop` 确认 `ro.product.vendor.device=elish`、`ro.product.vendor.model=M2105K81AC`，版本指纹为 `Xiaomi/elish/elish:13/RKQ1.211001.001/V816.0.2.0.TKYCNXM:user/release-keys`。除 XBL 内 `uefiplat.cfg` 对 `uefiplatLA.cfg` 的引用外，原始镜像字节搜索没有找到该配置文件内容；vendor `/firmware` 目录中也没有同名文件。此 ROM 目录没有 GPT/rawprogram XML 或 userdata 几何文件。
- `vendor.img:/etc/init.panel_info.sh` 从运行时 `/sys/class/touch/touch_dev/` 读取 `panel_vendor`、`panel_display` 和 `touch_vendor`。root ADB 现在确认值为 `S`、`B`、`4`；DRM/启动参数标识 K81 `42_02_0a` dual-CPHY 面板。Android dmesg 记录 NT36523 控制器加载 `novatek_nt36523_k81a_fw01.bin` 成功，PID `5961`、固件版本 `0x1a`。静态 ROM 中仍有 595C/5961 两份测试标准文件，运行日志确认的是固件 PID 5961。
- 该 FFS 配置引用了 `uefiplatLA.cfg`，但没有证明该文件已找到或在设备上生效。另一 GUID-defined section（GUID `1d301fe9-be79-4353-91c2-d23bc959ae0c`，属性 `0x1`）内含 gzip 数据；解压得到内层 FV（偏移 `0x8`、长度 `0x659000`、头长 `0x48`、校验和 0），共解析 147 个 FFS（含 88 个 DXE driver）。内层 FV 中未发现 `uefiplatLA.cfg` 字符串。
- 对内层 FV 的 PE32 section 搜索 RAM Partition protocol GUID，只命中 `EnvDxe`（FFS GUID `90a49afd-422f-08ae-9611-e788d3804845`，SHA-256 `62e81b81dc507a5ecd2e9e2d732c3e642c7c107bcdd51a236bbc029800b645ed`），且与 `Binaries/alioth`、`Binaries/pipa` 的 EnvDxe 二进制完全相同。其字符串包含 `RamPartitionTableLib.c` 和从 SMEM 读取 memory partition table 的错误消息。当前 Android 未挂载 debugfs，没有 `/dev/mem`，因此不能从该运行系统只读导出 SMEM 表或确认协议返回范围。`SmemDxe` PE32 section 也与这两个 catalog binary 相同，哈希 `4ad21b61ab64d1d91403403c8ce23b60648a2fb29c781fe4a7cb5a3996d727aa`。这些匹配收敛了固件模块候选，仍没有设备实际 UEFI 内存图。
- 只读记录了候选 `SimpleTextInOutSerial`、`DisplayDxe`、`UFSDxe` 的 FFS 项和 DEPEX；相应 PE32 临时提取到 `/tmp` 用于 hash 和协议检查，没有复制到仓库或固件缓存。`DisplayDxe` PE32 SHA-256 为 `6c6351c3c770d4f698e5e22097021fe8d20d35a8d4181df05676a48c4347f425`，`UFSDxe` 为 `ffeaed467f8251b7e30a45075da84a1c563e2bac7dc4be0f79f0ed49a88b8dc9`。Display/UFS DEPEX 的协议 GUID 到提供方模块映射仍未解决；候选不等于已批准复用。
- 对全部 `.img` 做原始字节搜索，仅 `xbl.img` 出现 ASCII `uefiplatLA.cfg`；已解压的 XBL 内层 FV 中没有该字符串。`vendor.img` 的 ext4 根目录含 `/firmware`，但该目录中没有以 `uefiplatLA.cfg` 命名的文件；未递归导出 vendor 文件系统。当前环境的 `dsm.open(xbl.img)` 因 PyGhidra 找不到 `uv` 或 Python 3.10–<3.14 而失败，没有反汇编/xref 结果；只读结构解析未写出二进制文件。
- `vendor_boot.img` 为 v3；Kona v2.1 DTB 对应 SoC `0x164/0x20001`，elish DTBO entry 12 是 board ID `47`。stock DTB 的 `/memory/reg` 为零，Android `MemTotal` 不是物理 bank map。fastboot `getvar` 查询没有响应，随后通过 root ADB 直接读取 live FDT 与 GPT，数据及哈希记在 `reports/elish-live-hardware-20261123.md`。
- live FDT 为 845892 字节，SHA-256 `90cf1d5333e758f001a8e21ed4e78fc000ebf3b5502338f010435a2da292a663`。两段 DRAM 为 `0x80000000+0x3bb00000` 与 `0xc0000000+0x1c0000000`，中间空洞 `0x4500000`，总 bank size `0x1fbb00000`。`/reserved-memory` 含 22 个固定和 12 个动态子项。Linux `/proc/iomem` 不是 UEFI `GetMemoryMap`，不能据此确认 DarwinBoot/XNU 可用范围；当前核心 ADT 生成器只接收一个连续范围。iPadOS 17.7.11 patch series 未带 DarwinBoot/XNU excluded-range 补丁，因此不能把 gap 简单夹进单个 XNU 内存窗口。
- root ADB 读取 `/dev/block/sda` 得到容量 253461790720 字节、逻辑块 4096 字节和 61880320 LBA。userdata 分区 34 inclusive LBA 为 `6619136–61870314`，容量 226308829184 字节。主备 GPT header/array CRC 均通过，分区数组逐字节相同。`board.toml` 已写入这些边界并设 `userdata_geometry = "verified"`。
- 当前设备仓终端没有可交互的 SSH stdin 接口；远端目录内容及未提交状态未检查，本轮没有向远端传输文件。
- 本轮及崩溃恢复后的 ADB 设备枚举为 `lineage_elish` / `M2105K81AC`，shell UID 为 0。执行了 block capacity/sector size、live FDT、`/proc/iomem` 和 GPT sector 只读读取。复查发现 Android 未提供 `/sys/firmware/memmap`、`/sys/kernel/debug` 或 `/dev/mem`，pstore 为空，dmesg 未见 RAM Partition/SMEM memory-partition 信息。`qcom-smem` 驱动映射 no-map 区域 `0x80900000+0x200000`，但未向用户空间导出 SMEM 表。没有执行 `fastboot boot`、刷写或分区写入。

## 接手顺序

1. **先做不依赖实机的输入准备（部分完成）。** 已解析上述 XBL FV 并记录 DXE 的 FFS 类型/GUID、PE32 哈希与 DEPEX；仍需定位 `uefiplatLA.cfg` 的实际来源和选择关系。UFSDxe 的 catalog DEPEX 两个 protocol GUID 已静态追到 SmemDxe/SPMIDxe provider，elish stock 两个模块均与 catalog hash 匹配。stock NpaDxe 也与 catalog 匹配，并安装 UFSDxe 查找的 NPA protocol GUID `79d6c870-725e-489e-a0a1-27e7a5d0cb35`。`wh1stl3/uefi-enuma` 的 `uefi-enuma@bec84f0ad9ab167ed08d0ce2ff46ab8cd2d15610` 只作 Kona DSC/FDF 结构参考；不复用其地址、memory map、SKU 或厂商二进制。
2. **核对内存 handoff。** root ADB 已提供 live FDT、DRAM banks、reserved-memory 子项、Linux `/proc/iomem` 和主备 GPT。GPT 已与磁盘容量、逻辑块大小和主备表 CRC 交叉核对并写入 `board.toml`。后续需从 elish UEFI 输入中确认 FD/stack 与 `GetMemoryMap`，再确定 DarwinBoot/XNU 使用的连续范围和保留区所有权。不要用 8 GiB SKU 或 enuma GPT 补值，不要绕过 preflight。
3. **实现并构建。** 钉住 Mu-Silicium 与子模块版本，建立 `uefi-elish/Platforms/Xiaomi/elishPkg/{elish.dsc,elish.fdf}` 和 `Resources/Configs/elish.toml`。按 [UEFI 指南](https://unixnu.com/porting/uefi-firmware) 接入 UniXNU、UniXNUDxe、DarwinBoot、UniXNURamDiskDxe、DiskIoDxe、FAT 与对应 LibraryClass；核对 FDT/initrd 的映射与保留。严格预检通过后才构建 FD 和 firmware-only boot image，记录输入 revision、哈希、产物大小、命令和日志。stock header v3 只证明 stock 格式，不证明自制镜像会被 ABL 接受。
4. **单独审批实机验证。** 先审阅镜像和 SHA-256，获得逐镜像 `fastboot boot` 授权后，按 [bring-up 顺序](https://unixnu.com/porting/bring-up) 看固件/菜单、payload 发现、DarwinBoot handoff、XNU 入口。已有备份和恢复路径为用户确认，不是在本轮实测。不要刷写 boot/vendor/bootloader，不要改 GPT 或写 userdata。未取得实机证据前不勾选计划项，不标 `device-verified`。

## 2026-11-24 离线续作

用户要求在设备无法探测时停止重试并先推进可离线完成的工作。本轮没有运行 ADB、fastboot 或 USB 枚举命令，也没有启动或写入设备。

- DarwinBoot 与目标 XNU 源码审查确认，当前 board 请求完整 256 MiB 连续窗口。未设置 `darwinboot-memory-excluded` 时，loader 从当时的 UEFI map 分配完整连续空闲区。目标 XNU 使用 `physBase`/`memSize` 建立连续区间，源码没有 excluded-range 属性解析器。运行时 UEFI map 和最终物理基址仍未验证。
- UFS 审查确认 NativeUfs-alioth 的 `0xe0`、`0x4e0` 是 alioth 的板级 SMMU 数据；elish DT 未给出 stream ID。该分支的 clock register 来源仍标为 SM8550，reset、RPMh、TCSR 和 QMP-PHY 初始化输入未建立，因此没有加入 elish 组件选择。
- pinned DarwinBoot 的生产 Mach-O parser test 已直接接受现有 XNU 候选。它包含有效 `LC_UNIXTHREAD` 入口和位于 256 MiB window 限制内的对齐 segments。该结构检查不会改变 kernel metadata 中 relocation、ABI、runtime 和 `loadable` 均为 false 的资格状态。
- XnuPrelink 输出中的 relocation、register-dataflow、ABI、runtime、loadable 字段由 pinned emitter 保持为 false。现有实验内核不能通过编辑 JSON 报告变成可加载内核。
- 常规 UEFI 固件的 `ELISH_MEMORY_MAP_PROBE` 默认值已改为 `FALSE`，并在主机重建成功。3 MiB FD SHA-256 为 `5c1cc81f58df5e255ea2e87e29169ae17e3f8aecbce464feb9ff16df13ba0b3e`，FV 清单不含探测 DXE。
- 固定的 SM8250 Linux GCC 基线与 NativeUfs-alioth 的 SM8550-derived 寄存器常量有多项偏移差异；stock elish clock specifier IDs 也不能直接映射到上游绑定。具体来源哈希和寄存器项已补入离线审计报告，UFS 仍不启用。
- DarwinBoot memory-window、ADT-window、loader-input tests 通过；XnuPrelink 65 项测试通过；`scripts/check-elish.py --static-only` 通过。严格预检仍因运行时内存 handoff 未核实而阻止镜像构建。
- 审计、测试环境说明和离线下一步记录在 `reports/elish-offline-audit-20261124.md`。本地移植记录已同步更新。设备侧探测保持暂停。

## 2026-11-24 构建环境续核与 XBL 静态分析

- 本轮只执行了一次 `fastboot devices`，没有列出设备。按用户要求停止后续设备探测，没有运行 ADB、`getvar`、启动或写入命令。
- 为满足 `board.toml` 中 `../UniXNU` 的相对路径，重新建立并保留 `/root/repo/UniXNU -> /root/elish-build-20261123/UniXNU`。经设备仓正常入口运行 `./scripts/package.sh --check --os ipados-17.7.11-21H461` 成功，解析完成本地 IPSW、配置、依赖和 62 个补丁。此检查没有构建 payload 或启动镜像。
- 构建环境已有 Python 3.13、Clang 21、EDK II/BaseTools、mtools、dtc 和 LLVM 工具。本地忽略文件 `unixnu.local.toml` 中的路径及 8 位非零十六进制平台种子通过配置检查，权限为 0600，值未输出。标准 payload 路径依赖 mtools；只有显式使用 `--skip-setup` 才调用 macOS 的 `hdiutil`。
- 安装在仓库之外的 Ghidra `12.2_PUBLIC_20261016` 和 Temurin JDK `25.0.5.1+2` 均按官方发布校验值验证。XBL 的 headless AArch64 ELF 分析得到 4,230 个函数。`FUN_9fc107e0` 通过 `FUN_9fc12890` 请求 `uefiplat.cfg` 并解析 `Config`、`MemoryMap`、`RegisterMap`、`ConfigParameters` 和 `ChipIDConfig`。`FUN_9fc10e14` 将 `ConfigParameters` 保存为键值表。配置内含 `PlatConfigFileName="uefiplatLA.cfg"` 和 `OsTypeString="LA"`，但实际打开该文件的消费者和文件来源仍未定位。分析命令、地址、校验值及未决问题见 `reports/elish-xbl-config-lookup-20261124.md`。
- `kernel.macho` 与 IPSW 的 `kernelcache.decompressed` 均为普通 Mach-O `MH_EXECUTE`。当前 `MH_FILESET` emitter 不适用于这两个输入。已有内核候选仍标记 `loadable=false`，没有生成 payload 或 iPadOS 启动镜像。
- `python3 scripts/unixnu/test_bootimg.py`、`python3 scripts/unixnu/test_boot_template.py` 和 `python3 -m unittest discover -s kits/XnuPrelink -p 'test_*.py'` 均通过，XnuPrelink 共 65 项。`python3 scripts/porting-record.py check --device /root/repo/UniXNU-elish` 检查 14 个区域通过。`scripts/check-elish.py --static-only` 和 `git diff --check` 通过，严格预检仍因 UEFI/XNU 内存所有权未核实而阻止镜像构建。

## 2026-11-24 UFS DXE 静态依赖续核

本轮只分析本地 ROM 提取件和 UEFI catalog 二进制，没有运行 ADB、fastboot 或 USB 枚举，也没有启动或写入设备。

- elish stock XBL 提取的 `UFSDxe.efi` PE32 与 UEFI catalog 中 alioth、pipa 的 `UFSDxe.efi` 完全相同，SHA-256 为 `ffeaed467f8251b7e30a45075da84a1c563e2bac7dc4be0f79f0ed49a88b8dc9`。catalog DEPEX 要求两个 protocol GUID。Ghidra 反编译确认 elish stock `SmemDxe` 和 `SPMI` 分别通过 `EFI_BOOT_SERVICES.InstallMultipleProtocolInterfaces` 安装对应 GUID，且二者 PE32 hash 均与 catalog 文件一致。elish stock `NpaDxe` 也与 catalog hash 一致；UFSDxe 查询的 NPA GUID `79d6c870-725e-489e-a0a1-27e7a5d0cb35` 与该模块的安装调用相同。
- stock XBL `uefiplat.cfg` 的 UFS 配置块给出 `EnableUfsIOC=0`、`UfsSmmuConfigForOtherBootDev=1`，与 elish UEFI `ConfigurationMapLib` 一致。ClockDxe、CmdDbDxe、PwrUtilsDxe、RpmhDxe、HWIODxeDriver 和 VcsDxe 的 PE32 也与 pipa/alioth catalog 完全匹配；stock elish DALSys 是不同变体。UFS 的活动时钟、SMMU context/stream 所有权、reset/RPMh/TCSR/QMP-PHY 状态仍未核实，驱动尚未接入。
- 详细哈希、DEPEX 表达式、Ghidra 命令和结果记录在 `reports/elish-offline-audit-20261124.md`。核心阻塞仍是 XNU 候选 `loadable=false`、运行时 UEFI/XNU 内存交接未核实，以及 elish XNU UFS/APFS 根卷路径未实现。

## 2026-11-24 DXE 屏幕与主机镜像续核

- 用户提供的屏幕帧显示 `MMU: 45 regions`、`MMU: map valid`、SEC/HOB 成功、FV 解压和 DXE Core 入口。DXE Core 打印的 `Memory Allocation` HOB 行包含多个可与 elish XBL 配置对应的 first-bank ranges。逐行辨认、源码和公开 pipa/Kona 配置核对、图片哈希见 `reports/elish-live-uefi-hob-observation-20261124.md`。
- 这些 HOB allocation 记录证明固件已越过 PEI 并进入 DXE，仍不包含完整 UEFI `GetMemoryMap`、Qualcomm RAM Partition 表或 `ExitBootServices` 最终 map。照片中未见 `ElishMemoryMapProbeDxe` 的输出标记。
- 主机构建的 iPadOS payload 为 67,108,864 字节，Android v3 `boot.img` 为 67,727,360 字节，SHA-256 分别为 `eb3f2dd1ab8b662ead42c5c05b86d530162f35804f6ba4593a57f18ffc9357b9` 和 `0f6fdd711ff2a22284e32f969716041d5e79ed7c46503fc4335666d26eec3aef`。第二次 round-trip 输出与 `boot.img` 逐字节相同。详细输入、FD hash 和 `hardware_tested=false` 状态见 `reports/elish-ipados-host-image-20261124.md`。
- 内核 candidate 仍标记 `loadable=false`，且 relocation、register dataflow、ABI、runtime 资格字段均为 false。payload 和 boot image 尚未在设备上启动。未执行分区写入。

## 2026-11-24 当前工作树重建

- 按目标仓 `AGENTS.md` 重读 README、设备指南、构建/验证记录，并复核本计划和交接记录。设备目标保持 Xiaomi Pad 5 Pro Wi-Fi/elish、SM8250-AC、iPadOS 17.7.11（21H461）。
- 正常设备入口 `./scripts/package.sh --no-deps --os ipados-17.7.11-21H461 --uefi /root/repo/uefi-elish` 成功。构建输入 revision 为 UniXNU `71c5c6569bae6369c0e0f634df8b9bd5afc19139`、设备仓 HEAD `208b47d20e87d3876fd862bb1f0ee0c17467b0c2`、Mu-Silicium `66421cb7bb5694843aec8de1ca3d1de4c20cca38`。补丁序列解析 63 项；strict `check-elish.py`、bootimg/boot-template 测试、XnuPrelink 65 项测试和 14 区域 porting-record 检查均通过。
- 当前输出 `kernel.macho` SHA-256 `7ab7c4c90647e17ae1a301117fc3e067616431a17b164a5ef01b0867603605b2`，payload SHA-256 `73a1bfa3f410a68246456bc4f85042b4753b714e0a7718294c39f9e98a66aec6`，67,727,360 字节 `boot.img` SHA-256 `2920a4deb130a789f2e2992e5fc6eb944e77a58e7464a8446215d7ef1b5678a9`，UEFI FD SHA-256 `33c28a3dea6fc5e54f967cb0fc765318bec1f566ca429359d272b9ce5e0c6bed`。完整记录见 `reports/elish-ipados-host-image-20261124.md`。
- 当前 boot image 和 payload manifest 均为 `hardware_tested=false`。XnuPrelink 仍将 relocation、register dataflow、ABI、runtime 与 `loadable` 标为 false。板级内存状态允许主机构建，但运行时 UEFI/XNU 所有权尚无实机验证；UFS/APFS 设备根路径也未完成。
- 用户表示设备已进入 fastboot，但本机当次及构建后的枚举均显示 ADB serial `ELISH_SERIAL` 为 `unauthorized`，`fastboot devices -l` 为空，USB ID 为 `18d1:4e11`。因此没有启动镜像或写入设备分区。主机产物是待 RAM-only 设备验证的候选，不作为已启动 iPadOS 的证据。

## 2026-11-24 Fastboot handoff retry

- Rechecked the current 67,727,360-byte iPadOS boot image. `fastboot -s ELISH_SERIAL boot .../boot.img` waited for that serial and was stopped by a 20-second timeout before transfer.
- `adb -s ELISH_SERIAL reboot bootloader` returned `device unauthorized`. The default host ADB key exists, so the connected Android device has not authorized this host key. `fastboot devices -l` remains empty and `adb devices -l` remains `unauthorized`.
- The production DarwinBoot Mach-O parser test passes for the generated kernel. That checks image structure only; no runtime execution was observed. No device partition was written.
- Resume hardware validation once the host enumerates a bootloader fastboot device or the Android authorization dialog is accepted. Then use RAM-only `fastboot boot` on the current image and capture the first visible UEFI/DarwinBoot/XNU stage.

## 2026-11-24 Authorized RAM-only boot tests

- ADB authorization became active and identified `lineage_elish` / `M2105K81AC`. `adb -s ELISH_SERIAL reboot bootloader` succeeded; the device appeared as fastboot USB `18d1:d00d`.
- Ran `fastboot -s ELISH_SERIAL boot /root/elish-build-20261123/UniXNU/out/elish/ipados-17.7.11-21H461/elish/boot/boot.img`. The 66,140 KiB image sent and booted with `OKAY` in 1.889 seconds. About five seconds later, USB enumerated Android ADB again. No UEFI, DarwinBoot or XNU output was captured.
- Then ran the 622,592-byte `diagnostics/elish-memory-map-fdt-probe.img` with RAM-only fastboot boot. Fastboot accepted it. Android returned, but ADB was `unauthorized`, so the probe's `/chosen/unixnu,uefi-memory-map` property could not be collected. The image's recorded SHA-256 is `3ccdc1738073add586a8eb87c7c6ad1deff3120fff899aa6d525f87335379a93`.
- The bootloader's acceptance confirms the image transport format and size only. The XNU candidate still has `loadable=false`, runtime memory ownership is unverified, and no elish UFS/APFS root driver is enabled. The host build is complete, but an ETA for a verified iPadOS boot depends on diagnosing these runtime blockers.
- No partition was written. Next useful device input is accepting the ADB host key with the persistent authorization option, then reading the diagnostic FDT property and capturing the first visible boot stage.

## 2026-11-24 GetMemoryMap requirement and re-test

- DarwinBoot calls UEFI `GetMemoryMap()` at runtime to select and allocate the XNU memory window. It obtains the map again before `ExitBootServices()` and uses that map key for the handoff. This runtime behavior is part of the boot chain.
- Exporting the complete UEFI map before building is not a build prerequisite. The board uses `memory_map = "runtime-managed"`; strict `scripts/check-elish.py` passes and reports that DarwinBoot resolves usable memory from the runtime map. Device ownership still needs runtime validation. The dedicated map probe is a diagnostic path.
- Rechecked `fastboot -s ELISH_SERIAL getvar product`, which returned `product: elish`, then RAM-booted the current `boot.img`. Fastboot accepted the 67,727,360-byte image. USB then enumerated stock Android 13 `V816.0.2.0.TKYCNXM`. The Android shell ran as uid 2000, could not read `/sys/fs/pstore`, and had no `su`; this run captured no UEFI, DarwinBoot or XNU logs.
- The main boot blockers remain the XNU candidate's `loadable=false` and unqualified relocation, register-dataflow, ABI/runtime fields, plus the unfinished elish XNU UFS/SMMU and APFS root path. Commands and hashes are in `reports/elish-ipados-host-image-20261124.md`.
- Ran the existing `_arm_vm_init` return probe. Fastboot accepted it and Android USB returned. The USB timeline is consistent with the configured probe reset, but no framebuffer or serial marker confirms that `_arm_vm_init` returned. See `reports/elish-xnu-entry-probe-20261124.md`.

## 2026-11-24 memory-map probe retest

- `GetMemoryMap()` remains a DarwinBoot runtime operation used to select XNU memory and complete the `ExitBootServices()` handoff. A separately exported map is not a host-build prerequisite.
- Rebooted to fastboot and confirmed `product: elish`. Re-ran the FDT-export image `diagnostics/elish-memory-map-fdt-probe.img`, SHA-256 `3ccdc1738073add586a8eb87c7c6ad1deff3120fff899aa6d525f87335379a93`. Android returned with ADB authorized, but shell UID 2000 could not find `/proc/device-tree/chosen/unixnu,uefi-memory-map`.
- Added a 120-second unconditional hold after the memory-map display routine returns, rebuilt the 3 MiB FD and a 634,880-byte firmware-only Android v3 image. FD SHA-256 `98dc9b6ab41af1f80db8159c087d8aead4b3a315807f4b01f4bca6d714128699`; image SHA-256 `03b08a9f39c09038d2c4f57c5b0df0fbb754f6e23964d7a9c40507e2b5027cd0`.
- Fastboot accepted the new image, but Android ADB re-enumerated within 10 seconds. The probe marker and map were not captured, so DXE driver dispatch and platform Stall behavior remain unresolved. The user-supplied frames show the DXE Core HOB allocation walk, not the full UEFI memory map.
- A third image added a visible DXE entry marker and a 15-second entry stall before the memory read, with the 120-second post-return hold retained. Fastboot accepted it, and Android ADB re-enumerated within 10 seconds. This still does not distinguish driver dispatch from timer behavior. Image SHA-256 `70af40d78b3e46e1b3faf0fe3d8079aef642e22d41022c4582114325c1b2b7c5`; full details are in the probe report.
- The device is currently back in `lineage_elish` Android. No device partition was written. Full evidence and the earlier successful-output-path hold image are recorded in `reports/elish-memory-map-probe-20261124.md`.

## 2026-11-24 UFS SMMU diagnostic

- Built a diagnostic XNU with NativeUfs-alioth revision `336680c353e6c3f25d6b29284532c4fd16104f68`. The full component failed to link against the stage-1 kernel because it expected IOStorageFamily and VerifiedRoot symbols. The local diagnostic checkout at `/tmp/elish-nativeufs-probe-only` adds `#define NATIVE_UFS_PROBE_ONLY 1` at the top of `NativeUfs.cpp` and uses the read-only `native-ufs-probe=7` path.
- The first full package selected the old baseline kernel via the ignored local `[build] kernel` override. Removed that override and explicitly rebuilt payload with the diagnostic kernel. Read the FAT contents back with mtools to verify the Mach-O hash, SMMU probe string and argument. The final Android v3 diagnostic image uses firmware with `ELISH_MEMORY_MAP_PROBE=FALSE` and is at `out/elish/ipados-17.7.11-21H461/elish_ufs_probe/boot/elish-ufs-smmu-probe.img` in UniXNU. It is 67,739,648 bytes with SHA-256 `fa575d4a230a90b19ad349257741e004cbd2ec36f1958721bcbdb3f29649c314`. The payload SHA-256 is `4c5c14a1bed7eeae2a36d3ab4e5df50240b14821fd49b433d1f89499b5a9bf18`.
- The first RAM-only transfer failed during download with `unknown command`. Ran `fastboot -s ELISH_SERIAL reboot bootloader`, verified `product: elish` and repeated `fastboot -s ELISH_SERIAL boot` with the diagnostic image. Both sending and booting returned `OKAY`. USB disconnected for 12.9 seconds, then Android ADB returned. Root ADB showed `bootloader` as the reported boot reason and an empty `/sys/fs/pstore`. The results leave probe execution and UFS DMA ownership unconfirmed. Commands and USB events are in `/tmp/elish-ufs-smmu-boot-retry-20261124.log` and `/tmp/elish-ufs-smmu-usb-retry-20261124.log`. No partition-write command was run.
- Restored the default `elish_defconfig` and removed temporary ShengUfs selection and probe arguments from the default board. The diagnostic kernel's relocation, register-dataflow, ABI, runtime and loadable qualification fields remain false.

## 2026-11-24 Android pstore 固件日志

- 复用 enuma 的 ramoops 热重启采集流程，核对 sheng 的 Linux 日志读取和 pipa 的 DXE 调度顺序。elish 的 Android 实测 ramoops 地址为 `0xb0000000`，console 为 2 MiB，ECC 为 0。平板热重启回 fastboot 后用 `fastboot continue` 启动 Android，启用 root ADB 并读取 `/sys/fs/pstore/console-ramoops-0`。
- 首次导出的 5,986 字节混有相邻内存的数据。修正共享固件 `FrameBufferSerialPortLib` 后导出同样长的纯文本，确认进入 DXE 并加载 `EnvDxe.efi`。第二轮在 DXE 调度器临时加入调用前后的日志，导出 6,184 字节。日志显示 `PcdDxe` 返回 `Success`，最后一条停在调用 `EnvDxe` 前的标记。`EnvDxe` 返回状态和复位原因待查。
- 两次验证均用 `fastboot boot` 运行诊断镜像，平板已回到 LineageOS。临时调度器改动已从 Mu_Basecore 撤回。已确认的 Linux DRAM 和 GPT 仍见 `reports/elish-live-hardware-20261123.md`，Qualcomm RAM Partition 表和 UEFI `GetMemoryMap()` 仍待采集。参考实现、命令、镜像 SHA-256 和日志见 `reports/elish-uefi-ramoops-20261124.md`。

## 2026-11-24 EnvDxe 通过与 HII 调度修正

- `EnvDxe` 需要 `Info Blk` HOB。恢复 elish DSC 中的 `PcdBuildXblHobs` 后，实机成功注册 Qualcomm RAM Partition 协议并完成到 `SmemDxe` 的调度。采集了 14,317 字节的 pstore，最后一次断言发生在 `ConPlatformDxe` 查找 HII String 协议时。已将 `HiiDatabaseDxe` 排在 `ConPlatformDxe` 之前。
- HII 修正版通过 RAM-only fastboot 启动，约 35 秒后重新枚举 fastboot。执行 `continue` 后出现 Xiaomi `05c6:f000` Android USB 存储接口，报告无介质。主机无法访问 ADB 或读取此轮 pstore，实际到达的固件阶段待确认。两轮均未写分区。
- 另构建了在控制台启动前运行 `RamManagerDxe` 和内存探针的诊断镜像 `boot/memory-map-ramoops/boot.img`。它直接保存 RAM Partition 表及 UEFI 描述符到 ramoops，然后热重启入 fastboot。常规板级配置保留 `ELISH_MEMORY_MAP_PROBE=FALSE`。待 USB 恢复至可控模式后运行。镜像 SHA-256、构建命令及日志见 `reports/elish-uefi-info-hob-20261124.md`。

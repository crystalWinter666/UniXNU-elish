# 小米平板 5 Pro（elish）iPadOS 17 移植计划

## 当前检查点 · 2026-11-25 17:36 CST

用户手动执行 run12 `fastboot boot` RAM-only 镜像后报告看到与此前相近的彩条；本次没有照片，不能确认彩条之外的 XNU 检查点，也不能证实 `_arm_vm_init()` 返回。主机先前同一命令的一次启动获 `Sending OKAY` / `Booting OKAY`，但约 6 秒后 fastboot 重枚举，未达到探针设置的 15 秒 post-MMU hold；另一次主机命令因 `remote: 'unknown command'` 失败。设备最后状态未再探测，避免把用户手动启动后的实时状态当成旧快照。

run12 镜像为 `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run12-postmmu-console/boot.img`，SHA-256 `49460e8b7aabf22dc6e921e3435ab47811c9dbd4cfe27b1880a5858d786f324a`。最新可核验的 XNU 证据仍是 run09 照片中的 pre-MMU 彩条；其六秒 stage-3 复位/USB 时间线满足 UniXNU 的 MMU-enable 诊断判据，但 `_arm_vm_init()`、UFS、APFS root、`launchd` 与 iPadOS 用户态均未确认。

21H461 主机普通候选 SHA-256 `baa41e0a3b5b0e3c77f97b59af4904a6ec8fdad0afccdf7b401348da0c8e0140`，对应 metadata 仍 `loadable=false`，未上机。board 仍请求 256 MiB。run19 accepted memory map 有 `0xc60bc000+0x1b9f44000` 的单段 ConventionalMemory，4 GiB 在几何上可容纳；尚未分配 4 GiB 窗口。下一会话先确认 run12 屏照与设备控制状态，再核定 BCM2837 4 KiB 编译宏，并用隔离输出准备 4 GiB RAM-only 诊断候选；UFS DMA 在 stream/context 所有权有设备证据前维持关闭。

## 当前检查点 · 2026-11-25 16:05 CST

run09 近照显示绿色 EBS、蓝色 XNU 入口和品红色 MMU 前检查点。镜像包含 MMU 开启后六秒复位；USB 于 15:47:32 断开、15:47:50 重新枚举 fastboot，照片时间为 15:47:51。按 UniXNU green-bar diagnosis 的 stage-3 判据，记录为 **MMU 已开启**，证据来自定时复位，不是内核文本日志。照片 SHA-256 为 `b03cc52878f046d7574ab1a776a3f83662f10251875438dcfbd2181ac99ee7b8`。

run13 `_arm_vm_init` 返回探针和 run17 `PE_init_printf` 探针均被 fastboot 接受，设备回到 fastboot 的时间接近配置的复位时序，属于间接佐证。run18 pmsg readback 得到 `magic=0x00004000/complement=0/valid=0`，所以 `_arm_vm_init` 返回尚未直接确认。run19 未发现活动 ACPI IORT；live FDT 的 `qcom,smmu_sde_unsec_cb` 以 `<0x46 0x820 0x402>` 指向 apps-SMMU，故唯一有效的 SID `0x820/mask 0x402` 属于显示 SDE。UFS stream/DMA 模式仍未知，DMA 保持禁用。

对照本地 `docs/zh/porting/steps.md` 官方八步流程，当前在第 6 步的平台/内存和存储依赖，并按第 7 步 bring-up 验证内核入口；根卷、用户态和第 8 步发布尚未到达。下一步先用可见检查点确认 `_arm_vm_init` 返回，并追查 run18 标记未读回原因；同时离线追踪 elish UFS ARID、DMA、时钟/reset/RPMh/PHY 初始化。随后核实 XNU runtime memory ownership 和 `loadable=false` 资格；按 UFS 指南依次验证 DMA/命令、容量和已知块，再做设备侧 APFS SystemOS/root 挂载与 `launchd`。完整阶段与证据等级见 `reports/elish-progress-20261125-1605.md`。

## 当前检查点 · 2026-11-25 15:38 CST

核对用户给出的 `web1n/android_device_xiaomi_elish`（HEAD `23e652dde3a1b1a2f6f6ff3f7e088d3f6c814772`）：它提供 LineageOS elish 产品配置并继承 `sm8250-common`，但仓库本身没有 DTS/DTSI 或 UFS/SMMU SID 映射。公共设备仓 fstab 仅给出 UFS sysfs host 路径 `1d84000.ufshc`；当前 root ADB 的运行时 UFS FDT 节点无 `iommus`，也无 `iommu_group` 链接，run19 SID `0x820` 仍不能归属 UFS。分析写入 `reports/elish-ufs-kona-clock-map-20261125.md`。

run17 `PE_init_printf` 探针 RAM-only 启动成功，镜像 SHA-256 `3bcc3f62f519b96a5b56fd47da752a68c168b3f63e5c17c36a3bf8e92c9d6f7e`；设备约在探针复位时序附近回到 fastboot，但没有屏照或该轮 pstore 读回，仍不能确认 MMU 后阶段。之后已用 reader 恢复 root ADB。常规 21H461 候选当前 SHA-256 `a8ce2352da7416064a891b6d7ab29102190aaaf508c236cc4d180c5685b146fa`，本轮没有启动它。最近直接 XNU 证据仍是 run09 pre-MMU 彩条；EBS 及 256 MiB 窗口已由 run06/run11/run18/run19 日志确认。无分区写入或关机操作；UFS DMA、设备 APFS root 和 iPadOS 用户态仍未验证。

## 当前检查点 · 2026-11-25 14:58 CST

run18 post-VM marker 与 pmsg readback 已 RAM-only 实测，marker 为 `0x00004000/0x00000000`、无效，`_arm_vm_init` 返回未证实。增强版 run19 实机确认当前 UEFI ACPI config 无 IORT；SMMU 唯一有效 stream 为 SID `0x820`/mask `0x402`/context 4，S1 translation 未启用，因此 UFS stream 所有权仍未知、DMA 继续禁用。两次 readback 日志同时确认 FAT payload、DarwinBoot 的 256 MiB 窗口和成功 EBS；探针均在 XNU 前停止。详细镜像、哈希、阶段和日志见 `reports/elish-run18-run19-device-20261125.md`。

测试期间所有镜像均用 RAM-only `fastboot boot`。设备最后已由哈希验证的 Lineage reader 恢复 root ADB；无持久分区写入、无关机命令。下一步分析 MMU 后标记失败原因及可用的 elish 专属 UFS 证据，之后再决定 UFS 驱动初始化；不得套用参考板 stream ID。

## 当前检查点 · 2026-11-25 12:08 CST

本轮 `adb devices -l` 和 `fastboot devices -l` 均为空，虽然用户报告设备可进 fastboot；只读查询后停止设备探测。顶置相机快照 `/tmp/elish-current-camera-20261125-late.jpg`（1920x1080，SHA-256 `25dde9a4cf57cd90a6bff7e4fb249fa6e13695e11784acd2e73758546c80f7ac`）过暗，不能判定屏幕或设备电源状态。用户授权休息期间进行分区写入但要求不关机；由于设备未枚举，本轮没有启动或写入分区。

离线审阅 run19 只读 IORT/SMMUv2 探针的表/节点/映射边界校验和寄存器访问路径，没有发现 SMMU/UFS 写入或进入 XNU 的代码路径。run19 镜像 SHA-256 `ab653c2ebee272a7a56fc0c60eba6490b14e77132dd3b3eccc3a91d7aaf9fe0b`，run18 marker/readback 哈希分别为 `842ac4db5666b20cae96725ec621fa0c42ce74fe612f19eebb8ac8d36a5e6a30`、`d96aeadaf64999975f6af3427b8a811aaed639679d111ff70992cb24f559c939`，均尚未上机。最近直接阶段仍为 run09 XNU pre-MMU；run06/run11 已确认 FAT payload、DarwinBoot、256 MiB 窗口及成功 EBS。MMU 后、UFS/SMMU、APFS 根卷和 iPadOS 用户态仍未确认。设备重新枚举后先核验 fastboot 产品为 `elish`，运行 run18 marker/readback，再单独运行 run19；UFS DMA 维持禁用。

## 当前检查点 · 2026-11-25 11:59 CST

设备最近一次 USB/fastboot/ADB 快照没有枚举到平板，本轮只进行离线构建。新增的 run19 RAM-only 候选已在主机成功构建，加入只读 ACPI IORT `UFS0` 映射采集及 SMMUv2 状态扫描；67,751,936 字节，SHA-256 `ab653c2ebee272a7a56fc0c60eba6490b14e77132dd3b3eccc3a91d7aaf9fe0b`，尚未上机。它不写 SMMU，也不进入 XNU。构建及输入详情见 `reports/elish-iort-smmu-probe-20261125.md`。

设备最近直接确认阶段仍为 run09 XNU pre-MMU；run06/run11 确认 FAT payload、DarwinBoot、256 MiB 窗口和成功 EBS。MMU 完成、UFS/DMA 所有权、设备 APFS root 和 iPadOS 用户态仍未确认。下一次设备可访问时优先 RAM-only 测 run18 post-VM marker/pmsg readback，之后才单独测 run19 的 IORT/SMMU；UFS DMA 维持禁用。

## 当前检查点 · 2026-11-25 11:40 CST

本轮 `lsusb`、fastboot 和 ADB 均未发现平板；只做了一轮枚举，没有发设备协议命令、启动/重启、写分区或改变电源状态。顶置 V4L2 帧全暗，无法判断屏幕状态。主机 `git diff --check`、静态板级检查、14 区域 porting-record 检查和 63 补丁 package preflight 均通过；运行时内存 handoff 仍未验证。

共享 Kona IORT 模板的 `\\_SB.UFS0` 节点映射到 `0x4e0`/`0xe0`，但 elish 构建未集成 `AcpiTables.inf`/`AcpiTableDxe`，因此模板不能代表运行时表，也不能解释 run11 的 SID `0x820`。UFS DMA 继续禁用，细节见 `reports/elish-ufs-uefi-arid-audit-20261125.md`。本轮没有运行镜像或新增启动日志；21H461 与 run18 marker/readback 候选哈希不变且均未上机。最近直接 XNU 阶段仍为 run09 pre-MMU；run06/run11 已确认 FAT payload、DarwinBoot、256 MiB 窗口和成功 EBS，MMU 后、APFS 根卷及 iPadOS 用户态仍未确认。

## 当前检查点 · 2026-11-25 11:24 CST

设备 USB、fastboot 和 ADB 均未枚举；本轮没有发设备控制请求、启动/重启、写分区或改变电源状态。纠正 Ghidra 地址误读后，stock `UFSDxe`/`SdccDxe` 查询的 GUID 为 `54b6d3b4-5d33-4f91-8600-6c41d5deb19a`，stock `HALIOMMUDxe` 是已确认的 provider。HAL 依据 ACPI IORT 中的设备路径/ARID mapping 配置 SMMU；elish 的实际 IORT mapping 尚未提取，run11 SID `0x0820` 所有者仍未知，因此 UFS DMA 继续禁用。细节见 `reports/elish-ufs-uefi-arid-audit-20261125.md`。

本轮没有运行镜像或新增设备日志。21H461 常规镜像 SHA-256 为 `baa41e0a3b5b0e3c77f97b59af4904a6ec8fdad0afccdf7b401348da0c8e0140`；run18 marker 为 `842ac4db5666b20cae96725ec621fa0c42ce74fe612f19eebb8ac8d36a5e6a30`；pmsg readback 为 `d96aeadaf64999975f6af3427b8a811aaed639679d111ff70992cb24f559c939`，三者本轮均未上机。最近直接 XNU 阶段仍是 run09 的 pre-MMU 屏照；run06/run11 确认 FAT payload、DarwinBoot、256 MiB 窗口和 EBS 成功。MMU 后、UFS、设备 APFS root 与 iPadOS 用户态未验证。

## 当前检查点 · 2026-11-25 10:54 CST

设备 USB/fastboot/ADB 当前均未枚举，本轮未发送设备命令、启动镜像、改写分区或改变电源状态。离线 Ghidra 反编译 stock UFS/SDCC DXE 后确认 Qualcomm 私有 IOMMU API 按 ACPI device path 和 ARID 附加 domain；ARID 不是可直接复用的 apps-SMMU SID，run11 SID `0x0820` 所有者仍未知。审计没有找到该私有协议的 catalog provider，因此不据此启用 UFS DMA。详情见 `reports/elish-ufs-uefi-arid-audit-20261125.md`。

本轮没有运行镜像。21H461、run18 marker 和 pmsg readback 候选的 SHA-256 与前一检查点相同且均未上机。最近直接阶段仍为 run09 的 XNU pre-MMU 照片；run06/run11 确认 UEFI/DarwinBoot 至 EBS，MMU 后、设备 APFS root 和 iPadOS 用户态未验证。

## 当前检查点 · 2026-11-25 10:44 CST

21H461 常规主机候选仍有效，产物及验证见 `reports/elish-host-package-20261125.md`。本轮设备未枚举，没有发送设备协议命令或启动镜像。run18 pmsg 流程已有源码、反汇编和地址隔离核验，但实机写入/跨复位保留未验证。Android R UFS/SMMU 源码也没有提供 elish stream ID，SID `0x0820` 所有者未知；storage 记录继续保持 UFS DMA 未启用。详见 `reports/elish-run18-pmsg-path-audit-20261125.md` 和 `reports/elish-ufs-kona-clock-map-20261125.md`。

最近直接 XNU 阶段仍为 run09 entry/pre-MMU；run06/run11 日志确认 FAT payload、DarwinBoot、256 MiB 窗口和 EBS Success。候选仍 `loadable=false`；MMU 后、UFS/SMMU 接管、设备 APFS root 和 iPadOS 用户态未验证。设备重新枚举且 fastboot 控制请求可用后，先 RAM-only 启动 run18，再启动 pmsg-readback 镜像并用已校验 reader root ADB 导出 console/pmsg 证据；不要将空 pstore 当作阶段证据。不得由 SM8550/alioth/cupid stream 猜测 elish stream，也不启用未经确认的 UFS DMA。

## Context

目标设备为小米平板 5 Pro Wi-Fi（elish，SM8250-AC / 骁龙 870），不是 5G/enuma；目标 OS 为 iPadOS 17.7.11（21H461），验收愿景是尽可能完整可用。用户有已解锁实机可用于验证。设备身份已通过型号、DT/DTBO 和固件身份核对，独立 Format 2 设备仓已建立。官方 ROM 镜像现位于 `/root/repo/elish_os1.0.2.0`。root ADB 已测得两段 DRAM、reserved-memory 子项和 userdata GPT 几何；内存所有权、UEFI handoff 与存储控制器/DMA 仍待核对，不能把 SM8550 或 enuma 的板级数值移植过来。

## Approach

从格式 2 设备模板新建独立 elish 设备仓库；现有社区分支仅作为只读资料来源，逐项核验来源和适用型号后，仅迁入可验证资料，不直接迁移旧格式代码。先统一 Wi-Fi elish 的设备身份文档，再为该设备准备 Mu-Silicium UEFI 平台包。复用经审查的 SM8250-AC 公共支持及同 SoC 设备驱动；不引入 enuma 的基带路径。设备专属参数必须来自 elish 固件/DTS、设备读取或实测。按最小启动链路逐阶段验收，先 kernel/root，再输入显示/GPU，最后接入连接、音频及电源功能。

## Files to modify

- 先审计的社区基线：`hpadmx/UniXNU-elish`（设备目录记录的 revision 为 `3481d725...`；本地设备页链接的源码 revision 为 `a357eae...`，需确定实际采用版本）。预期审计 `board.toml`、`configs/`、`scripts/check-elish.py`、`scripts/make-elish-adt.py`、`tools/` 及 `README.md`。
- 已建立的独立格式 2 设备仓库 `/root/repo/UniXNU-elish/`：`board.toml`、`feeds.toml`、`unixnu.lock`、`configs/elish_defconfig`、`dts/elish.dts`、`patches/xnu/series`、`porting.toml`、`proprietary-files.txt`、`package/`、`scripts/`、`README.md`、`AGENTS.md` 和 `.github/workflows/`。README 明确标注 Wi-Fi elish，并将 enuma/5G 列为不同设备。
- UEFI 平台仓库 `uefi-elish/Platforms/Xiaomi/elishPkg/`、`Resources/Configs/elish.toml`、DSC/FDF、固件输入清单与 preflight；是否需要导入厂商 DXE 二进制待核实。
- 若要更新 UniXNU 中央目录，再同步 `devices.toml` 和 `docs/zh/devices/elish.md`，确保名称、SoC、仓库链接、版本和完成状态一致。
- 仅当审计确认共享缺陷时，才考虑修改当前 UniXNU 核心仓库或共享组件；本计划不把板级驱动塞入共享核心。

## Reuse

- `docs/zh/porting/steps.md`、`soc-support.md`、`bring-up.md`、`device-repository.md`、`drivers.md`：规定新格式设备移植、SoC 归属、阶段验收和记录要求。
- `devices.toml`、`docs/zh/devices/elish.md`：登记现有 elish 社区移植与其已知缺口；需对照远端固定 revision 更新判断。
- `devices/alioth.md`：SM8250/iPadOS 17.7.11 参考，包含实测 UFS、触控及 Adreno 650 方向的板级经验；不能复制 alioth 板级数值。
- `docs/zh/devices/enuma.md`：同系列平板 SM8250-AC 的 UEFI/启动链参考；其板级接线、分区与未实测项目不能直接用于 elish。
- `examples/device/`：格式 2 骨架，但默认选择 SM8550；仅复用文件结构与流程，不复用 SoC 选择。
- `profiles/os/ipados-17.7.11-21H461/profile.toml`：目标 OS/XNU/IPSW profile。
- SM8250 参考实现可能来自设备 feeds/社区仓库；复用前必须检查 pinned commit、OS ABI、板级依赖和真实验证记录。

## Steps

- [x] 核验目标机为 Wi-Fi elish（而非 5G/enuma），审计现有分支 README/元数据中的型号矛盾和来源 revision；建立以实机型号、DT/DTBO、固件身份为准的事实记录，不纳入 5G 基带配置。
- [ ] 收集并固定完整板级证据。已通过 root ADB 确认 live FDT 两段 DRAM、34 个 reserved-memory 子项、Linux `/proc/iomem`、主备 GPT 中的 userdata LBA，以及运行中的 K81 42_02_0a 面板和 NT36523 PID 5961/fw01 触控固件；GPT 几何已录入 `board.toml`。UEFI/XNU 内存所有权和 UFS/PHY/SMMU 所有权仍待核对。区分公开资料、静态推断与实机测量，并确认数据备份和恢复路径。
- [x] 从格式 2 模板建立独立设备仓库；仅迁入现有分支中来源明确且确认适用于 Wi-Fi elish 的资料，按 SM8250-AC 和 iPadOS 17.7.11 固定 SoC/profile/组件版本，整理一致的 README、board DTS、分阶段 defconfig、移植记录、日志及静态 preflight。
- [ ] 继续验证 elish Mu-Silicium/XNU 启动链。主机 payload 与 Android v3 boot image 已回读校验；run04/run06 直接确认 FAT 配置、DarwinBoot、256 MiB 窗口与成功 EBS，run09 近照确认 XNU 已到 MMU 前阶段。XNU 可加载资格和 MMU 后阶段仍未验证；UFS/SMMU 设备端所有权与 APFS 根卷挂载也未完成。
  - 2026-11-24 最新验证：修复 Android v3 initrd 中未对齐拼接的 vendor gzip/FAT 后，`run04-fat-child` 已实机读取配置、选择 `xnu`，进入 DarwinBoot 并成功执行 `ExitBootServices()`；探针记录匹配 key 的 73 描述符内存图和 256 MiB 窗口，随后主动重启且未执行 XNU。`run05` 使用终止型 ramoops 内核检查点并在 EBS 后放行 XNU，fastboot 接受镜像，但 120 秒内无 USB 重枚举。已停止探测，等待 root ADB 恢复后先读 pstore。哈希、阶段和恢复方式见 `reports/elish-loader-fat-initrd-20261124.md`。
  - 2026-11-25 更新：用户屏幕照片显示 DarwinBoot 绿色交接带、256 MiB 窗口及入口寄存器；确认 EBS/交接点，不确认 XNU 第一条指令。经 fastboot `product=elish` 和备份 reader 恢复 root ADB 后，pstore 为空。六秒入口条诊断镜像 SHA-256 `441e30dcc04b350c8016668c85cc2f64f32b179fa70bb790b12ff15f69bfd121` 被 fastboot 接受并回到 LineageOS，但蓝/紫条是否出现待用户观察；XNU entry/MMU 仍未确认。另已离线构建默认关闭的只读 SMMU 探针，镜像 SHA-256 `dc552b5eb9a2795f1de63abd3f48ab02003add18d2bd26b153f4af1789d0e24f`，尚未上机。详见 `reports/elish-xnu-entry-retest-20261125.md` 与 `reports/elish-smmu-readonly-probe-20261125.md`。
  - 2026-11-25 06:46 CST run14 传输复测：30 秒 `_arm_vm_init` 返回探针镜像 SHA-256 `499bf02f87858aef0062fbf6dbedecdaec57d9ad9db906ab368079692afbbb6f`。本次系统 fastboot 34.0.5 只打印 `Sending`，后续卡在 `usbfs_start_wait_urb`；没有 `Sending OKAY` / `Booting OKAY`，故镜像未启动、没有新增阶段证据。设备随后仍枚举 fastboot，暂停设备协议探测。顶置摄像头视频过暗，无法判读条带。后续用已验证的 fastboot 37.0.3 并确认传输恢复后再继续。详见 `reports/elish-run14-fastboot-retry-20261125.md`。
  - 2026-11-25 06:55 CST 控制通道复核：使用 fastboot 37.0.3 对 serial `4bed07e4` 执行只读 `getvar product`，8 秒无输出后超时；USB 仍枚举 fastboot，说明仅 USB presence 不代表协议可用。本次未发送镜像、未做 USB reset 或重启。设备协议探测暂停，离线工作继续。
  - 2026-11-25 07:08 CST 复核：fastboot 37.0.3 的 `devices -l` 枚举到 serial `4bed07e4`，但只读 `getvar product` 仍在 8 秒后超时；ADB 为空。V4L2 单帧过暗，无法观察屏幕。run14（SHA-256 `499bf02f87858aef0062fbf6dbedecdaec57d9ad9db906ab368079692afbbb6f`）未传输。本轮无设备重启、分区写入或新增启动证据；保持设备协议操作暂停，继续离线 UFS/启动链审查。详见 `reports/elish-fastboot-readonly-status-20261125.md`。
  - 2026-11-25 07:53 CST 更新：run14 一次启动约 43 秒后回到 fastboot，但同镜像后续传输在 USB bulk 阶段失败；run15 `_PE_create_console` 后 15 秒复位约 27 秒回到 fastboot，run16 在 `_PE_init_printf` 前等待 15 秒约 28 秒回到 fastboot。这些 USB 时序是间接证据，支持运行越过 `_arm_vm_init()` 并到达后续调用点，不能代替屏幕/串口标记。run17（SHA-256 `3bcc3f62f519b96a5b56fd47da752a68c168b3f63e5c17c36a3bf8e92c9d6f7e`）已构建，但两次命令返回 `unknown command`，没有启动。reader pstore 为空且 `record_size=0`，不构成未执行的证据。当前 USB 只读 `getvar product` 返回 `usb_read failed: No such device`，停止设备协议探测；相机画面欠曝。仍以 run09 照片确认 XNU 到 MMU 前为最新直接证据。详见 `reports/elish-run14-run17-xnu-timing-20261125.md`。
  - 2026-11-25 08:14 CST：主机 `lsusb`、fastboot 37.0.3 和 ADB 均未发现设备，本轮未发送设备协议命令。XNU 源码确认 `PE_init_printf(FALSE)` 只是设置 `PE_putc` 并返回；run17 能测试该调用点，但仍须等设备可探测后再 RAM-only 启动。离线 UFS 审计确认 ShengUfs 面向 SM8550，alioth stream/clock/reset/PHY 不能直接用于 elish；该板 UFS SMMU 所有权仍未知，未启用 DMA。详见 `reports/elish-run14-run17-xnu-timing-20261125.md` 和 `reports/elish-ufs-host-audit-20261125.md`。
  - 2026-11-25 08:43 CST 离线 UFS 映射复核：stock ROM `vendor_boot.img` / `dtbo.img` 哈希与既有记录一致，entry 12 合并 DTB 也复现既有哈希。Xiaomi `elish-r-oss` commit `52a2f180865f8368828985d6384130acd093fa93` 将九个 UFS host clock specifier 对应到 Kona GCC IDs/registers；PHY aux、RPMh ref clocks 与 GCC UFS PHY reset 也已映射。该 Android R 源码只提供编号和寄存器布局，不证明 UEFI handoff 后状态；stock UFS DT 无 `iommus`，run11 SID 0x820 仍无设备归属。本轮设备未在 USB/fastboot 枚举；未运行镜像或写分区，没有新增启动阶段。run17 候选 `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run17-printf-init/boot.img` SHA-256 `3bcc3f62f519b96a5b56fd47da752a68c168b3f63e5c17c36a3bf8e92c9d6f7e` 未启动；最后直接阶段仍是 run09 照片确认的 XNU pre-MMU。详见 `reports/elish-ufs-kona-clock-map-20261125.md`。
  - 2026-11-25 08:50 CST 设备状态观察：USB/fastboot 仍未枚举平板；顶置相机默认亮度 128 的 1920x1080 帧近乎全黑，临时亮度 220 后仍无法辨认，已恢复 128。帧 `/tmp/elish-current-device-state-20261125-0844.jpg` SHA-256 `8a57ecd29cd38af7ce6a3410dae43b2410a242cb8b61e512fcec2139791673cf`，不能判断屏幕状态或设备是否关机。未发设备协议命令、未运行镜像、未重启/复位/写分区；最后直接启动阶段仍为 run09 照片确认的 XNU pre-MMU。run17 候选路径与 SHA-256 见上一条，未启动。
  - 2026-11-25 run07：构建并 RAM-only 启动 XNU 入口 ramoops 检查点镜像，SHA-256 `38b548e03d2a0156f443e010cfdecca2becd7bc745f911a56dbb99e1b99c1e37`；fastboot 返回 `Sending OKAY` / `Booting OKAY`。检查点仅在 MMU 关闭且 ramoops ring 未环绕时写标记。本轮 pstore 尚未恢复，USB 断连/重连不能证明执行到 XNU。离线核对发现并修正检查点工具在恰好写满 console 时未回绕 start 的边界；随后已用修正后的 276 字节检查点构建独立 run08，镜像 SHA-256 9cd0577439a5f143ff11d81cdaa6aece79da01a25cfd5cfdf67ddea0a7a5075a。run08 尚未上机。设备初次重枚举 fastboot 且 `product=elish`；reader 发送卡在 USB bulk 等待，没有 `Booting OKAY`。之后 `getvar product` 返回 variable not found，`max-download-size` 无值，pstore 未读取。用户最新照片文字确认 EBS 后、内核分支前的 handoff。详见 `reports/elish-xnu-entry-checkpoint-20261125.md`、`reports/elish-xnu-entry-checkpoint-run08-20261125.md`、`reports/elish-xnu-entry-retest-20261125.md` 与 `reports/elish-ufs-host-audit-20261125.md`。
  - 2026-11-25 离线 storage 验证：NativeUfs host suites 的 GPT/root layout、window read、publication 和合成 SMMU 通过；修正仅位于 `/tmp` 测试副本的 shims 后，QEMU ARM64 command cases 与 14 项历史描述符比较通过。测试不代表设备 UFS/SMMU 硬件执行。详细范围和限制见 `reports/elish-ufs-host-audit-20261125.md`。
  - 2026-11-25 run08 实机复测：fastboot 37.0.3 核验 `product=elish`，RAM-only 启动修正版入口检查点镜像，`Sending`/`Booting OKAY`，总计 1.760 秒。用户照片显示绿色 ExitBootServices handoff 屏幕；随后用 reader 恢复 root ADB，`/sys/fs/pstore` 挂载但为空，dmesg 报 `Invalid compression size for deflate: 0`。因此没有恢复到入口标记，XNU 入口仍未知；空目录在压缩元数据异常下不构成未执行的证明。无分区写入。详见 `reports/elish-xnu-entry-checkpoint-run08-20261125.md`。
  - 2026-11-25 03:26 CST 只读复核：当前设备经 ADB 为 root `lineage_elish`，fastboot 未枚举；本轮未启动镜像。对应 Linux 源 revision `6b34d24765327d746074c8e53b5ad0d24a51acc7` 证明 deflate 警告由 ramoops `record_size=0` 导致的 panic-dump 压缩缓冲区为零，console ring 仍配置 2 MiB，空 pstore 原因仍未知。Android 下 UFS Samsung `KLUEG8UHDC-B0E1` 为 Gear 4 / 双 lane / FAST Rate-A，错误计数为零；apps-SMMU 87 个 stream-match groups 和 `ufshc_mem` client handle 均不能给出 UFS stream ID。XNU 入口、UFS/SMMU 接管、设备端 APFS 和用户态仍未确认。详见 `reports/elish-pstore-ufs-live-20261125.md`。
  - 2026-11-25 03:46 CST 入口可视复测：已知六秒阶段镜像 `elish-xnu-entry-reset.img`（SHA-256 `441e30dcc04b350c8016668c85cc2f64f32b179fa70bb790b12ff15f69bfd121`）两次获 fastboot `Sending OKAY` / `Booting OKAY`，耗时 1.835 / 1.765 秒；约 12 秒后 LineageOS ADB 回来。两次 root pstore 为空，deflate 警告由 `record_size=0` 引起。16 秒、640x480 V4L2 片段 SHA-256 `0566bbe3a2c5e62c859050504984358946bb2c75b41924e9fc618a8a7a1cfd2b` 只显示暗屏和 `FASTBOOT`，未确认蓝/品红阶段条；USB 时序也不构成 XNU/MMU 证据。无分区写入、未运行 `fastboot continue`。详见 `reports/elish-xnu-entry-visual-retest-20261125.md`。
  - 2026-11-25 04:16 CST run09 实机复测：将 `run09-xnu-pmsg-entry/boot.img`（SHA-256 `87ad74c775aa67d968827ce17bf56659289ebc8e2519f2e9ea2e541c314a704c`）写入备份过的活动槽 `boot_b`，启动后再成功恢复原 192 MiB 分区备份。用户近照同时显示绿色 EBS、蓝色 XNU 入口和品红色 MMU 前阶段条，直接确认 XNU 已运行到 MMU 前；MMU 后阶段仍无独立证据。随后 RAM-only reader 恢复 root ADB，但 `/sys/fs/pstore` 为空，预埋 pmsg 标记没有读回。当前原 `boot_b` 已恢复，设备在 reader 的 `lineage_elish` root ADB。未运行 `fastboot continue`。照片、哈希和命令见 `reports/elish-xnu-entry-visual-retest-20261125.md`。
  - 2026-11-24: Built the target iPadOS userdata-root container on the Linux host from the pinned 21H461 IPSW. The 9,382,658,048-byte image fits the measured userdata partition; its SHNGROOT manifest and embedded OS/SystemOS window hashes passed independent read-back verification. It has not been flashed or read by an elish storage driver. See `reports/elish-userdata-build-20261124.md`.
  - 2026-11-24 offline continuation: Parsed the stock vendor_boot v3 DTB set, selected the elish DTBO entry by model and board ID, and recorded UFS/PHY/ICE/IRQ/clock inputs in `reports/elish-stock-storage-audit-20261124.md`. Audited the pinned SM8550 ShengUfs and SM8250 NativeUfs-alioth source scopes. The latter is an adaptation candidate while its stream IDs remain unverified for elish. Generated a stable random software identity seed in the ignored core `unixnu.local.toml`; the value is kept out of this repository. Device detection was suspended for this work.
  - 2026-11-23: `./unixnu build kernel -D /root/repo/UniXNU-elish --os ipados-17.7.11-21H461` completed on the Linux build host, including `XnuPrelink`. The resulting 51,122,176-byte experimental kernel candidate has SHA-256 `759574c4d0a6c8cbd72bf8a8e1026c59390712d1a819a826379a0512d21ed8ec`; its metadata says `instruction_relocation_complete=false`, `register_dataflow_qualified=false`, `runtime_qualified=false`, `abi_qualified=false`, and `loadable=false`. At core revision `71c5c6569bae6369c0e0f634df8b9bd5afc19139`, `native_prelink.py` and `emit_prelink.py` set these qualification fields false while preserving executable instruction bytes, so the report cannot be made qualified by changing metadata alone. `./scripts/package.sh --check --os ipados-17.7.11-21H461` resolved 62 patches. Strict `check-elish.py` now has measured userdata geometry, but blocks payload and final boot-image work until UEFI/XNU memory ownership is reconciled. A 3 MiB UEFI FD and RAM-only probe images exist. Fastboot accepted the first diagnostic image, but no framebuffer output was captured. No iPadOS payload, final boot image, or verified iPadOS boot exists.
- 2026-11-23 本地固件解析：在 `xbl.img` FV 中定位到 `uefiplat.cfg`，其中引用 `uefiplatLA.cfg`；内层 gzip FV 含 147 个 FFS，并确认了 `SimpleTextInOutSerial`、`DisplayDxe`、`UFSDxe` 的模块 GUID、PE32 SHA-256 与 DEPEX。`uefiplatLA.cfg` 的实际来源和 DEPEX 提供方仍未定位；没有导出或复制厂商二进制。PyGhidra 因当前 PATH 缺少 `uv`/受支持 Python 而不可用。BatchMode SSH 认证失败但未测试用户密码，远端未访问。UEFI/XNU 内存所有权仍阻止构建；没有 UEFI/image 构建或设备启动。
  - 2026-11-24: elish DSC/FDF compiled successfully after initializing the pinned BaseTools submodules. The 3,145,728-byte FD SHA-256 is `f8f662313c0ffee611a2e55f687486b1625c9e80a983d9280f26f46f5dbdbf74`. A firmware-only Android v3 probe image was built and round-trip verified at 618,496 bytes, SHA-256 `8b08c449b4af3a4ab13c07389a789c170a83d6c373b68dfe76dc959224fef349`; it has an empty ramdisk and is not an iPadOS image. The device has not booted it. Root Android ADB cannot export the RAM Partition table or UEFI `GetMemoryMap`, so the memory handoff gate remains. The kernel candidate remains marked `loadable=false`. Full commands and pins are recorded in `reports/elish-uefi-build-20261124.md`.
  - 2026-11-24: Added `ElishMemoryMapProbeDxe` behind `ELISH_MEMORY_MAP_PROBE=TRUE`. The diagnostic FD builds at 3,145,728 bytes with SHA-256 `8f797816d1b3ac59338c248a7ed17442d4f3878e5abc2fae80314811d6967548`. Its 622,592-byte Android v3 probe image has SHA-256 `b2ccb5968d7cb30a462bf0f1c6deca84167ccb10c7f7a36782e07559c0fbf6d6`. At ReadyToBoot it prints the RAM Partition entries and pre-application UEFI DDR descriptors to framebuffer. The first image held for 30 seconds. A rebuilt diagnostic image holds for 120 seconds. `fastboot boot` returned `Sending` and `Booting` `OKAY`, then the target disappeared from USB enumeration and remained absent after the 30-second hold. No framebuffer output was captured, so probe execution and reported ranges remain unverified. No partition-write command was run. The endpoint was corrected to `0x280000000` from the live FDT, and earlier diagnostic-build hashes are superseded. The final ExitBootServices map and XNU memory ownership remain unverified. The rebuilt 120-second FD SHA-256 is `063614fdc3de0d2e7ab7a26bb7856c684fa4d4c45e95dfe83e8065fe31d1b2b1`; its 622,592-byte image SHA-256 is `f312ddfa2adacd2a48ef3d09e313765b668b1affe7e710426b3e3411fcd1e650` and has not yet been booted.
- 2026-11-23 ROM 到位后复核：官方镜像目录 `/root/repo/elish_os1.0.2.0` 含 28 个 `.img` 文件，总计约 8.2 GB。`xbl.img` SHA-256 `9a3a3bb92e6cf478b227f70dd1bd140fde32982994a6c34a07bd314ea0acb72f`、`dtbo.img` SHA-256 `4d797a4d3ea9ea16168128a9c189d7f32bf21bd1b049205fe39959a90046b9d7` 与既有记录相符。`vendor.img` 是 ext4，`build.prop` 确认 `elish` / `M2105K81AC` / Android 13 / `V816.0.2.0.TKYCNXM`。镜像目录没有 GPT/rawprogram 分区布局文件；原始字节搜索在 XBL 外未找到 `uefiplatLA.cfg`，vendor 的 `/firmware` 目录也没有同名文件。`/etc/init.panel_info.sh` 从运行时 sysfs 读取面板/触控厂商编号；vendor 固件目录含 `NT36xxx_MP_Setting_Criteria_595C.csv` 和 `...5961.csv`，但静态 ROM 没有表明本机使用哪一组。这些输入完成了固件身份与只读文件系统核对，但不提供运行时 DRAM map 或 userdata 起始 LBA。
- [ ] 验证 CPU/中断、内存映射与 DMA；再实现 SM8250 UFS/根卷/Data，使用已测 GPT 几何并读取目标卷 UUID，完成 iPadOS trust cache/owner chain 及用户态启动，并验证 Data 持久化和恢复。
  - 2026-11-24: The IPSW OS/SystemOS userdata-root container is now built and its manifest and both window hashes pass read-back validation. The APFS System-role root volume UUID in the OS image matches `board.toml`; only the host-side identity is verified, with device UFS reads and mounting still pending.
- [ ] 实现并验收显示/亮度和 Novatek NT36523 触控；再评估 Adreno 650/GPU 与 Metal，避免误用 SM8550 Adreno 740 路径。
- [ ] 逐项适配 Wi-Fi/蓝牙、USB、音频、摄像头、按键/传感器、电池、温控、频率调度和休眠/唤醒；无可用实现时记录限制，不将其标成已支持。
- [ ] 做重复冷启动、长时间运行、温控/功耗、升级与救援验证；完善移植记录、CI、构建输入 pin 和发布包。


## Verification

### 2026-11-25 run06 SMMU/FAT/EBS 实机验证

- RAM-only 启动 run06 只读 SMMU 探针镜像 `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run06-smmu-readonly/boot.img`，67,751,936 字节，SHA-256 `dc552b5eb9a2795f1de63abd3f48ab02003add18d2bd26b153f4af1789d0e24f`。用已校验 reader 返回 Android root ADB，并回读 49,328 字节 `console-ramoops-0`，SHA-256 `fdaa24dcf70d4dbb41d63a49b2c705b8f7c33517128135c8439d56d6a9b1d7b2`；当前 live pstore 与归档逐字节相同。
- 日志显示完整 initrd block device 和非对齐 FAT 子卷成功发布，DarwinBoot 打开 `unixnu.toml`、`kernel.macho` 与 ADT，选择 `xnu`；256 MiB 窗口为 `0xc007c000+0x10000000`。`ExitBootServices()` 返回 Success，key `0x3ee` 与 accepted snapshot 一致。map 大小 `0xD80`、stride `0x30`，为 72 条描述符，编号 0–71；最后一条是 `0xB0000000+0x400000` 保留区。探针在 EBS 后按设计重启并明确记录 XNU 未进入。
- apps-SMMU ID0/ID1 为 `0x4c017e57`/`0x6000004a`，共 87 组；仅 group 4 有效，raw SMR/S2CR 为 `0x84020820`/`0x4`，translation type 0、context bank 4。SMMUv2 mask 位于 bits 30:16，正确 mask 为 `0x0402`（SID `0x0820`）；原诊断字符串错误地把 VALID bit 31 纳入 mask，UEFI 探针源已修正。该 mapping 未能归属 UFS，不能据此启用 UFS DMA。
- run11 将探针扩展为有界读取 group 4 context bank 4 的 CBAR/SCTLR/TTBR0/TCR/FSR。镜像 `/root/elish-build-20261123/UniXNU/out/elish-darwinboot-20261125/run11-smmu-context-readonly/boot.img`，67,751,936 字节，SHA-256 `9e6dbce96f9551c56265c375a05877d3b0fcbc283b9c03c322987f8ca18f8767`；49,439 字节 pstore SHA-256 `f16b04efae4fb5859b84b7f6351fe970de52d2b89cfaf7f3525a5239a48b8600`，与设备实时读取相同。实测 `CBAR=0x1f000`（S1 translation/S2 bypass）、`SCTLR=0xe0`（M=0）、`TTBR0=0`、`TCR=0`、`FSR=0x400`。该上下文没有启用的 S1 页表；group 4 的 SID 所属设备仍未知，不能启用 UFS DMA。
- run11 再次确认 FAT/DarwinBoot、256 MiB 窗口、EBS Success 与 72 个最终描述符；探针明确停止于 EBS 后，XNU 未进入。当前活动槽 `_b`、Android 13 `lineage_elish` root ADB，fastboot 未枚举；本轮无分区写入、未调用 `fastboot continue`。run09 近照仍是最新 XNU 阶段证据（到 MMU 前），MMU 后、设备端 APFS mount 和 iPadOS 用户态未验证。镜像、源码 revisions、命令与证据边界见 `reports/elish-smmu-readonly-probe-20261125.md`。

### 2026-11-24 FAT 子卷与 EBS 实测

- Run03 ramoops 证明 FDT initrd 从 gzip vendor ramdisk 开始；有效 FAT32 引导扇区位于偏移 `0x11f0abb`，正好紧随压缩 vendor ramdisk，未按扇区对齐。此前共享 ramdisk 驱动将 gzip 头当作 FAT 首扇区，造成 `unixnu.toml` 配置未发现并退到内置 `exit` 菜单。
- 已为共享 UniXNURamDiskDxe 实现完整 FAT32 后缀识别和只读 relative-offset 子卷，同时保留完整原始 initrd。DarwinBoot 从子卷设备路径解析父块设备，并对完整 FDT initrd 的父子首尾数据执行校验。
- Run04 实测显示 FAT 子卷成功发布、配置文件加载、`xnu` 项目默认选择、DarwinBoot 请求、256 MiB 窗口和 `ExitBootServices()` Success。最终 map 为 73 个描述符，`key=0x3ee` 与预先取得的 map key 一致；窗口 `0xc007c000+0x10000000` 位于第二 DRAM bank，排除区间数为 0。诊断镜像随后主动重启，因此这次没有执行 XNU。
- 已增加专用 FAT32 后缀和 DarwinBoot 父子盘身份/边界主机测试；随机内存图测试 1,200 组、移植记录 14 个区域均通过。报告、完整 pstore、镜像路径与 SHA-256 见 `reports/elish-loader-fat-initrd-20261124.md`。
- Run05 新 ADT 开启 `loader_inputs=true`，已回读确认终止型内核副本哈希，但暂未取得设备日志。Fastboot 接受 SHA-256 `14c3bf7d50c7e5fb35151d312efa144bdaf648a557fcaf098c2044193b191b32` 后 120 秒无 USB 重枚举；停止了设备探测，等待用户恢复 LineageOS/root ADB 后优先导出 pstore。XNU 标记、storage、APFS 和 userspace 仍未确认。

### 2026-11-24 DXE 内存 HOB 与主机镜像续核

- 用户提供的四帧画面清楚显示 `Project Silicium for Xiaomi Xiaomi Pad 5 Pro Wi-Fi`、`MMU: 45 regions`、`MMU: map valid`、PEI/HOB 成功、FV 解压成功及 DXE Core 入口。DXE Core 的 `Memory Allocation` 输出可见 XBL 固件区、DXE heap、UEFI stack 和 `0xa0000000–0xafffffff` kernel reservation。已按 Mu-Silicium DXE Core HOB 遍历实现转录，并以公开 Xiaomi pipa Kona 配置逐项交叉核对，见 `reports/elish-live-uefi-hob-observation-20261124.md`。
- 这些画面是实际运行时 HOB 分配证据，尚不是完整 `GetMemoryMap()`、Qualcomm RAM Partition 表或 `ExitBootServices` 最终 map。画面没有自定义 `ElishMemoryMapProbeDxe` 标记。
- `board.toml` 当前以 `runtime-managed` 状态配置双 DRAM DTS、256 MiB DarwinBoot 窗口及 16 个 XNU excluded-range slot。主机已生成并回读验证 64 MiB iPadOS payload 和 Android v3 `boot.img`。镜像、payload、FD 哈希及验证结果见 `reports/elish-ipados-host-image-20261124.md`。
- `kernel.json` 仍将 instruction relocation、register-dataflow、ABI、runtime 和 `loadable` 标为 false。host package 的 `hardware_tested` 也为 false，当前没有 iPadOS 实机启动结果。

- 社区基线的静态资料与 revision 审计完成；新仓库 README、`board.toml` 和 `porting.toml` 对设备型号一致标识为 Xiaomi Pad 5 Pro Wi-Fi/elish/SM8250-AC，并明确其不同于 enuma/5G。
- 格式 2 解析、`fetch`、`status --json`、`tree`、设备记录检查及目标 defconfig/DTS 编译通过；源码 pin、archive 构建器和补丁顺序可复现。
- QEMU 只用于验证通用 UEFI/构建链，不替代真实 elish 验收。实机按 bring-up 阶段记录对应镜像哈希、串口/ramoops/屏幕观察；先证实 `fastboot boot` 可回到 bootloader，再逐层验证 DarwinBoot、XNU、根卷和 launchd。
- 2026-11-24 设备诊断：用户授权 RAM-only fastboot 启动。DXE FV 读取状态修复后的镜像将设备带回 `lineage_elish`（ADB 枚举为 M2105K81AC）。用户当前离开期间停止设备操作，先完成主机端探针和记录工作。
- 所有 DRAM、保留区、userdata 几何和外设参数必须有目标设备证据；验证 UFS/APFS、Data 写入与重启后读取，并测试失败恢复。
- 显示、触控、GPU、无线、音频、摄像头及电源功能分别按端到端操作验收；完成度按真实证据标注，不以源码存在或构建成功代替设备验证。
- 先备份 Android 数据并确认恢复路径；未经单独确认不规划写 boot/vendor/bootloader 分区，不分发 Apple IPSW/专有固件镜像。

## 2026-11-24 离线续作

- DarwinBoot 当前 256 MiB 请求不启用 excluded-range ADT 属性，loader 会从运行时 UEFI map 中分配一整段连续的 `EfiConventionalMemory`。目标 iPadOS 17.7.11 XNU 源码没有解析 excluded-range 属性的实现，因此不能把跨 bank gap 或固件保留区拼入 XNU 窗口。运行时分配基址仍未知，`memory_map` 保持 `unverified`。
- SM8250 `NativeUfs-alioth` 仅作为传输实现参考。其 `0xe0/0x4e0` stream 来自 alioth 的 SMMU 映射，elish stock DT 未提供这些 stream；时钟宏仍标注为 SM8550 来源，reset、RPMh、TCSR 和 QMP PHY 初始化也未完成。
- 常规 UEFI FD 已将 `ELISH_MEMORY_MAP_PROBE` 设为 `FALSE` 并主机构建通过，SHA-256 为 `5c1cc81f58df5e255ea2e87e29169ae17e3f8aecbce464feb9ff16df13ba0b3e`。固定 SM8250 Linux GCC 寄存器基线与 alioth 常量有多项差异，stock overlay 时钟 ID 也未能直接对应上游绑定。
- XNU 候选仍为 `loadable=false`。当前 XnuPrelink emitter 将 relocation、register-dataflow、ABI 和 runtime qualification 保持为 false，不能靠编辑报告字段完成资格证明。
- DarwinBoot 的 memory-window、ADT-window、loader-input tests 和 XnuPrelink 65 项主机测试已通过。详细命令与边界见 `reports/elish-offline-audit-20261124.md`。
- 2026-11-24 主机续核：一次 `fastboot devices` 未发现设备，按用户要求停止后续探测。重新保留 `/root/repo/UniXNU` 核心仓符号链接后，设备仓入口 `scripts/package.sh --check --os ipados-17.7.11-21H461` 通过并解析 62 个补丁。Ghidra 对 stock XBL 的静态分析确认其请求并解析 `uefiplat.cfg`，配置含 `PlatConfigFileName="uefiplatLA.cfg"` 和 `OsTypeString="LA"`；实际 LA 文件来源与消费者仍未知，运行时内存图也未取得。详见 `reports/elish-xbl-config-lookup-20261124.md`。iPadOS kernel candidate 仍为 `loadable=false`，严格预检仍阻止 payload 和最终镜像构建。

## 2026-11-24 UFS DXE 离线审计

- elish stock `UFSDxe` PE32 与 Mu-Silicium alioth、pipa catalog 模块逐字节相同，大小 118,784 字节，SHA-256 为 `ffeaed467f8251b7e30a45075da84a1c563e2bac7dc4be0f79f0ed49a88b8dc9`。catalog DEPEX 的两个 protocol GUID 已由 Ghidra 核验到 `SmemDxe` 和 `SPMIDxe` 的 `InstallMultipleProtocolInterfaces` 调用。elish stock `SmemDxe`、`SPMI`、`NpaDxe` PE32 均与 catalog 对应模块 hash 一致。UFSDxe 查询的 NPA protocol GUID `79d6c870-725e-489e-a0a1-27e7a5d0cb35` 也已在 stock `NpaDxe` 的安装调用中确认。
- UFS 模块静态字符串包含 NPA protocol lookup、IOMMU ARID `0x0`/`0x10`、UFS SMMU 配置选项及九个 GCC UFS/PHY clock 名称。XBL `uefiplat.cfg` 明确给出 `EnableUfsIOC=0` 和 `UfsSmmuConfigForOtherBootDev=1`，与 elish UEFI 配置一致。ClockDxe、CmdDbDxe、PwrUtilsDxe、RpmhDxe、HWIODxeDriver 和 VcsDxe 也与 pipa/alioth catalog PE32 hash 一致；elish DALSys 是不同变体。时钟状态、SMMU context/stream 所有权、reset/RPMh/TCSR/QMP-PHY 初始化仍未确定，因此尚未启用存储驱动。
- 具体 DEPEX GUID、模块哈希、Ghidra 命令和反编译观察已记录在 `reports/elish-offline-audit-20261124.md` 与 UEFI `Platforms/Xiaomi/elishPkg/PORTING.md`。设备探测保持暂停。
- pinned DarwinBoot 的生产 Mach-O parser test 接受当前 51,122,176 字节 `kernel.macho`：arm64 `MH_EXECUTE`、`LC_UNIXTHREAD` 入口、18 个对齐 segment，映射范围满足 256 MiB loader window。该结果只验证 loader 结构约束，XnuPrelink 的 relocation、ABI、runtime 和 `loadable` 资格仍为 false。

## 2026-11-24 XNU 内存范围回移

- 将 DarwinBoot 已有的 excluded-memory ADT 契约移植到目标 XNU 10063.141.1，并加入设备补丁序列。XNU 现在解析 `/chosen/darwinboot-memory-excluded`、将窗口内的区间排序合并、避开早期内核分配，并在 pmap、页数组、physmap 和内存统计中排除这些页面。
- `./scripts/package.sh --check --os ipados-17.7.11-21H461` 通过，补丁数为 63。完整 XNU 与 XnuPrelink 构建成功，`kernel.macho` 为 51,122,176 字节，SHA-256 `7ab7c4c90647e17ae1a301117fc3e067616431a17b164a5ef01b0867603605b2`。生产 DarwinBoot Mach-O parser test 接受该候选，XnuPrelink 65 项测试通过。
- excluded-range 表尚未在 `board.toml` 启用，DTS 仍没有 memory 节点，当前镜像也没有构建 ADT/payload。该回移只解决 XNU 消费者缺失，不提供运行时 UEFI map 或保留区所有权证据。
- `kernel.json` 仍将 instruction relocation、register dataflow、ABI、runtime 和 `loadable` 全部标为 false；没有最终启动镜像或设备端 iPadOS 启动验证。详见 `reports/elish-memory-handoff-xnu-backport-20261124.md`。

## 2026-11-24 DXE FV 读取修复

- 设备画面 `/root/repo/946BF84D23D5AC424DD564A47EFE82EA.png` 显示两个 FV HOB 的 FVB 安装成功，但 `FwVol` 对两个句柄读取卷头均返回 `EFI_ACCESS_DENIED`。Mu_Basecore 的 `FwVolBlockReadBlock()` 在 FV 头 `EFI_FVB2_READ_STATUS` 位清零时返回该状态。原始内外 FV 属性为 `0x000C0C00` 和 `0x00030C00`，造成未发现 FV2、没有 DXE 驱动调度及架构协议断言。
- `uefi-elish/Platforms/Xiaomi/elishPkg/elish.fdf` 已在 `[FV.FvMain]` 和 `[FV.FVMAIN_COMPACT]` 设置 `READ_ENABLED_CAP=TRUE`、`READ_STATUS=TRUE`。新构建卷头属性为 `0x000C0C06` 和 `0x00030C06`。常规 3 MiB FD SHA-256 为 `809131c7ffc4273e8e5e25deaa423499172113ba135366c06c144c50c7b9247f`。临时 Mu_Basecore 跟踪日志已移除。
- 修正属性后的 RAM-only Android v3 镜像通过 fastboot 启动，用户报告进入设备原系统，ADB 枚举标识为 `lineage_elish`。该镜像没有 iPadOS payload，也没有写入设备分区。
- 首次修复后的内存探针没有可见输出，因为它使用 `DEBUG_INFO`，而 elish DEBUG 掩码 `0x8007EE0F` 不包含该位。探针已改用屏幕上确认可见的 `DEBUG_ERROR`，并加入驱动入口和 ReadyToBoot 注册状态。新的 120 秒探针镜像为 622,592 字节，SHA-256 `75bbfd228f57b22fdc1bd03349be01720ab9fac3b31cd392900ae874638ddbae`，目前尚未在设备运行。用户用餐期间暂停设备操作。
- 具体命令、源码 revision、镜像哈希、运行记录和剩余输入见 `reports/elish-dxe-fv-read-status-20261124.md`。

### 2026-11-24 当前工作树完整打包复核

- 从设备仓运行 `./scripts/package.sh --no-deps --os ipados-17.7.11-21H461 --uefi /root/repo/uefi-elish` 成功，解析 63 个补丁并重建 XNU、payload、UEFI FD 和 Android v3 boot image。核心 revision `71c5c6569bae6369c0e0f634df8b9bd5afc19139`，设备仓 HEAD `208b47d20e87d3876fd862bb1f0ee0c17467b0c2`，Mu-Silicium HEAD `66421cb7bb5694843aec8de1ca3d1de4c20cca38`。
- 产物位于 `/root/elish-build-20261123/UniXNU/out/elish/ipados-17.7.11-21H461/elish/`。`kernel.macho` 为 51,122,176 字节，SHA-256 `7ab7c4c90647e17ae1a301117fc3e067616431a17b164a5ef01b0867603605b2`；payload 为 67,108,864 字节，SHA-256 `73a1bfa3f410a68246456bc4f85042b4753b714e0a7718294c39f9e98a66aec6`；`boot.img` 为 67,727,360 字节，SHA-256 `2920a4deb130a789f2e2992e5fc6eb944e77a58e7464a8446215d7ef1b5678a9`；UEFI FD 为 3,145,728 字节，SHA-256 `33c28a3dea6fc5e54f967cb0fc765318bec1f566ca429359d272b9ce5e0c6bed`。
- 严格板级预检通过，DarwinBoot/XNU 内存交接仍标为 runtime-managed 且未实机验证。boot image 已由打包流程验证，但内核报告仍将 instruction relocation、register dataflow、ABI、runtime 和 `loadable` 标为 false，`hardware_tested=false`。这是待 RAM-only 启动验证的候选镜像，尚不能记为已验证可启动。
- `test_bootimg.py`、`test_boot_template.py`、XnuPrelink 65 项测试及 14 区域移植记录检查通过。构建后主机仍枚举到 ADB `unauthorized`，`fastboot devices` 为空；没有进行设备启动或分区写入。

### 2026-11-24 首次 RAM-only 设备试启动

- 用户授权后，ADB 识别设备为 `lineage_elish` / `M2105K81AC`。运行 `adb -s ELISH_SERIAL reboot bootloader` 后，主机枚举到 fastboot `18d1:d00d`。
- 对当前完整 `boot.img` 执行 `fastboot -s ELISH_SERIAL boot .../boot.img`，镜像 66,140 KiB，发送与启动均返回 `OKAY`，总耗时 1.889 秒。约 5 秒后 USB 再次枚举为 Android ADB。未捕获 UEFI、DarwinBoot 或 XNU 输出，因此不能判为 iPadOS 启动。
- 随后 RAM-only 启动 622,592 字节的 FDT 内存图探针，fastboot 同样返回 `OKAY`。Android ADB 恢复后状态为 `unauthorized`，尚未读取 `/chosen/unixnu,uefi-memory-map` 属性，RAM Partition 和 UEFI 描述符仍未知。
- 这确认 Android v3 镜像格式/尺寸被 bootloader 接受。XNU `loadable=false`、运行时内存交接未验证、UFS/APFS 根卷驱动未接入，故不能给出“可启动 iPadOS”的固定 ETA。打包已完成；接下来要先定位 UEFI/DarwinBoot/XNU 实际停点，再推进内核资格与存储启动链。

## 2026-11-24 XNU 入口探针

- 完整 UEFI 内存图不是唯一阻塞。按 UniXNU bring-up 文档，本轮加入了蓝色内核入口条、品红色 MMU 前条和 MMU 开启后 6 秒 PSCI 复位。原 payload 的 panic 屏幕与定时复位探针无法同时放入目标内核 padding，因此诊断 payload 使用独立内核副本，保留入口条和复位，不带 panic 屏幕。
- RAM-only Android v3 镜像 SHA-256 为 `441e30dcc04b350c8016668c85cc2f64f32b179fa70bb790b12ff15f69bfd121`。fastboot 发送和启动均返回 `OKAY`，总耗时 1.900 秒。主机 USB 在 18:25:34 枚举 fastboot，18:25:38 断开，18:25:51 重新枚举 Android。约 17 秒总时间与菜单 3 秒倒计时、探针 6 秒延时和 Android 返回路径相符，属于内核已到 MMU 后检查点的间接证据，没有屏幕或串口记录作直接确认。ADB 随后为 `unauthorized`。
- 已构建下一轮 `_arm_vm_init` 返回探针镜像 `diagnostics/elish-xnu-arm-vm-return.img`，SHA-256 `d206d18d7a8bd4f8cdadb46a0e42dd3bf578baa2e6961384417fb0e42ad57e3f`，但本轮尚未在设备运行。探针命令、时序、镜像内容和证据边界见 `reports/elish-xnu-entry-probe-20261124.md`。
- 当前阶段还包括 XnuPrelink 的 relocation、register-dataflow、ABI 与 runtime 资格，UEFI/XNU 内存 ownership，以及 elish UFS/SMMU 和 APFS 根卷实现。现有内核仍是 `loadable=false`，因此无法根据已完成的主机构建或这次早期探针给出最终可启动时间。
- 所有操作均为 RAM-only `fastboot boot`，没有写入设备分区。完整结果、命令和哈希见 `reports/elish-ipados-host-image-20261124.md`。

## 2026-11-24 GetMemoryMap 与启动验证更新

- UEFI `GetMemoryMap()` 的运行时调用是 DarwinBoot 选择 XNU 内存窗口并执行 `ExitBootServices()` 交接的一部分；在构建前另行导出完整描述符表不是构建条件。`memory_map = "runtime-managed"` 的严格设备预检通过，同时提示运行时交接仍需设备验证。
- 再次确认 fastboot `product: elish`，并 RAM-only 启动当前 SHA-256 `2920a4deb130a789f2e2992e5fc6eb944e77a58e7464a8446215d7ef1b5678a9` 的镜像。bootloader 返回 `OKAY`，设备随后运行原厂 Android 13。没有捕获自制 UEFI、DarwinBoot 或 XNU 日志；Android shell 无权读取 pstore。
- 镜像可构建且被 fastboot 接受，但尚未证明可启动 iPadOS。优先推进 XNU relocation/ABI/runtime 资格和 elish UFS/APFS root path，并用 UEFI/DarwinBoot/XNU 检查点定位启动回退。完整 UEFI map 导出可作为诊断手段，不再作为 host build 的前置项。
- 已运行现成的 `_arm_vm_init` 返回探针镜像，fastboot 接受，Android 随后恢复。USB 时间线与探针配置相符，但缺少直接屏幕/串口输出，不能将该 checkpoint 标为已验证。详见 `reports/elish-xnu-entry-probe-20261124.md`。

## 2026-11-24 内存图探针重试

- `GetMemoryMap()` 是 DarwinBoot 运行时内存选择和 `ExitBootServices()` 交接的一部分，主机构建不需要提前导出完整 map。
- FDT 导出探针 RAM-only 启动后回到 Android，ADB 可读状态下 `/proc/device-tree/chosen/unixnu,uefi-memory-map` 不存在。
- 重新构建的入口级 120 秒停留镜像被 fastboot 接受，但 10 秒内 Android ADB 已恢复。当前未确认 DXE 探针入口执行或平台 Stall 有效。详见 `reports/elish-memory-map-probe-20261124.md`。
- 后续增加了入口标记和 15 秒停留，再运行前述内存读取及 120 秒停留。Fastboot 接受镜像，但 10 秒内 ADB 已恢复，仍未观察到探针入口或内存图输出。

# UniXNU elish UEFI 移植交接

## 最新交接 · 2026-11-24 22:10 CST 后

目标是在小米平板 5 Pro Wi-Fi（`elish`、`M2105K81AC`、`SM8250-AC`）构建并验证可启动的 iPadOS 17.7.11（`21H461`）。目标已暂停，等待新会话接手。下方日期章节保留当时的观察，当前状态以本节为准。命令中的 `ELISH_SERIAL` 和 `ELISH_BUILD_HOST` 是公开记录使用的占位符。 [移植计划](PLAN.md)

- **实机已进入 UniXNU 菜单。** `EnvDxe` 在补齐 Info Blk HOB 后安装了 Qualcomm RAM Partition 协议。随后将 `HiiDatabaseDxe` 排到 `ConPlatformDxe` 前。用户在运行 `hii-before-console/boot.img` 时看到了 UniXNU 菜单。主机已回读该镜像的 64 MiB FAT payload，确认包含 `xnu` 条目、三秒倒计时、Mach-O 和 ADT。实机是否读到了该 FAT 配置、实际选择了哪个条目以及是否执行 DarwinBoot 仍待取证。 [实机记录](reports/elish-uefi-pre-bds-memory-map-20261124.md#hii-and-unixnu-menu)
- **已取得 BDS 前的内存图。** `memory-map-ramoops/boot.img` 临时启动后主动热重启。主机经 `fastboot continue` 和 `adb root` 取回 21,189 字节 pstore，其中有两个 Qualcomm RAM Partition 区间和 47 条 UEFI DRAM 描述符。两段分别为 `0x80000000+0x3bb00000` 和 `0xc0000000+0x1c0000000`，与 live Linux FDT 一致。第二段在该快照中为连续 `EfiConventionalMemory`，容得下 256 MiB loader 窗口。实际窗口位置、低地址保留区归属、initrd 位置和 `ExitBootServices()` 接受的最终图仍待验证。 [采集报告](reports/elish-uefi-pre-bds-memory-map-20261124.md) · [文本日志](reports/elish-uefi-pre-bds-memory-map-20261124.txt#L258)
- **最新设备状态是 LineageOS ADB。** 序列号 `ELISH_SERIAL`，`adb devices -l` 显示 `device`，`adb root` 在 LineageOS 可用并已确认 `uid=0`。上一轮菜单测试后约 37 秒返回 fastboot，`continue` 后一度只枚举 `05c6:f000` USB 存储接口。设备之后恢复到 LineageOS，pstore 已空。最新一次枚举时 fastboot 为空。本轮两次测试均用 `fastboot boot`，设备分区写入次数为零。
- **iPadOS 候选已从标准入口重建。** 独立输出目录 `out/elish-resume` 的构建和打包校验通过。该候选仍使用阶段 1 defconfig，`kernel.json` 的 relocation、register-dataflow、ABI、runtime 与 `loadable` 字段仍为 `false`，`boot.json` 的 `hardware_tested` 为 `false`。新候选尚未上机。`userdata-root.img` 已验证文件结构和 APFS System 卷 UUID，仍需在 elish 上实现并验证 UFS/SMMU 读取、APFS 根卷挂载和用户态启动。 [根卷构建](reports/elish-userdata-build-20261124.md)

### 工作区与镜像

| 用途 | 路径或版本 |
| --- | --- |
| 设备仓 | `/root/repo/UniXNU-elish`，基础提交 `208b47d20e87d3876fd862bb1f0ee0c17467b0c2`，含未提交的板级配置、补丁和日志 |
| UniXNU 核心 | `/root/elish-build-20261123/UniXNU`，`/root/repo/UniXNU` 指向该目录，基础提交 `71c5c6569bae6369c0e0f634df8b9bd5afc19139`，含未提交的构建支持改动 |
| 固件 | `/root/repo/uefi-elish`，Mu-Silicium 基础提交 `66421cb7bb5694843aec8de1ca3d1de4c20cca38`，Mu_Basecore `cfb93724a06ff86ff5ba971413368a7809ee42a5`，含未提交的 elish 平台和 ramoops 实现 |
| 本地输入 | 官方 ROM 在 `/root/repo/elish_os1.0.2.0`。核心的 `unixnu.local.toml` 保存本地构建输入及平台种子，文件由 Git 忽略 |
| 新候选 boot image | 核心下 `out/elish-resume/elish/ipados-17.7.11-21H461/elish/boot/boot.img`，67,747,840 字节，SHA-256 `c6581b4dcf531a55d6ad843c8ae8f3db41d6a24fe831b836fdaf3325b12c11fd` |
| 新候选构建清单 | 同一输出的 `package.manifest.json`、`kernel/kernel.json`、`payload/payload.img` 和 `boot/boot.json` |
| 已上机镜像 | 核心下 `out/elish/ipados-17.7.11-21H461/elish/boot/{hii-before-console,memory-map-ramoops}/boot.img`。该目录根部的 `boot.img` 来自较早实验，使用新候选或上述明确子目录复现实验 |

构建新候选的设备仓命令如下。

```sh
./scripts/package.sh --no-deps --os ipados-17.7.11-21H461 \
  --uefi /root/repo/uefi-elish \
  --out /root/elish-build-20261123/UniXNU/out/elish-resume --jobs 8
```

### 接手顺序

1. 从 UniXNU 菜单确认 FAT payload 的 `xnu` 条目，然后定位 DarwinBoot。建议用独立诊断镜像尝试在 ramoops 写入 loader 状态、实际 256 MiB 窗口和 `ExitBootServices()` 返回结果，成功取得最终图后走已验证的主动热重启路径。**该诊断镜像还没有实现或构建。** 相关入口位于核心仓 `drivers/DarwinBoot/DarwinBootPkg/Driver/DarwinBoot.c`，可复用固件的 `RamLogWrite` 和 `RamLogRestartIntoFastboot`。板级常规配置为 `ELISH_MEMORY_MAP_PROBE=FALSE`，DEBUG 配置已启用 `ELISH_RAMOOPS_LOG`。
2. 实机测试前使用 `/root/platform-tools/adb devices -l` 和 `/root/platform-tools/fastboot devices -l` 确认状态。已连到 LineageOS 时，先用 `adb -s ELISH_SERIAL root` 读取尚存的 pstore，再用 `adb -s ELISH_SERIAL reboot bootloader`。核验 `fastboot -s ELISH_SERIAL getvar product` 为 `elish`、核对镜像哈希后临时启动。热重启回 fastboot 时，用 `fastboot continue` 恢复 Android，随后以 root ADB 导出 `/sys/fs/pstore/console-ramoops-0`。`05c6:f000` 存储状态可能需要用户恢复 LineageOS 或进入带 ADB 的 Recovery，重启后日志有可能丢失。探测不到时按用户要求先做离线工作。 [已验证命令](reports/elish-uefi-pre-bds-memory-map-20261124.md#ram-partition-and-uefi-descriptors)
3. 取得直接证据后推进 XNU 入口、异常日志和内核资格。再完成 SM8250 UFS 控制器及 SMMU/DMA 所有权、读取 GPT 与已核对的 APFS 卷、挂载根卷并验证 `launchd`。实测 `userdata` 为 LBA `6619136–61870314`，逻辑块 4096 字节。复用 enuma 的热重启日志链、pipa 的 Kona DXE 顺序，以及 alioth 的 SM8250 UFS 实现作为参考。elish 的 SMMU stream ID 仍需本机证据。 [硬件数据](reports/elish-live-hardware-20261123.md#ufs-and-userdata-gpt) · [参考移植](reports/elish-uefi-ramoops-20261124.md#reference-implementations)

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

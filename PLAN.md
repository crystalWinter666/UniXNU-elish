# 小米平板 5 Pro（elish）iPadOS 17 移植计划

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
- [ ] 在实机验证已构建的 elish Mu-Silicium UEFI 和启动链。主机端已构建 64 MiB iPadOS payload 和 Android v3 boot image，并完成 boot image 回读校验。设备端启动、完整 UEFI 内存交接和 kernel 可加载资格仍未验证。
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

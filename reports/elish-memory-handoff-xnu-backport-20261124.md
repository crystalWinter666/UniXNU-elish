# elish XNU excluded-memory handoff backport, 2026-11-24

## Change

The pinned iPadOS 17.7.11 XNU source (10063.141.1, core input revision `d8b80295118ef25ac3a784134bcf95cd8e88109f`) has no consumer for DarwinBoot's `/chosen/darwinboot-memory-excluded` property. The device patch series now carries `patches/xnu/elish-memory-excluded-ranges.patch` after `sheng-platform-power.patch` and before the OS profile patches.

The backport reads 64-bit `(base, size)` pairs, rejects malformed property sizes and arithmetic overflow, clips ranges to the XNU managed span, aligns to the target page size, sorts and merges overlaps, and checks the kernel's early allocation reserve. It then excludes those pages from boot allocation, free-page accounting, the physical aperture mapping, and the VM page count. The adjacent booter-memory accounting change avoids underflow when `memSizeActual` reports usable bytes smaller than `memSize`.

The code is adapted to XNU 10063.141.1 context and types. The known newer-profile implementation was used as a source reference. The target XNU patch applied through the regular device package patch sequence and compiled in the target kernel build.

## Host verification

The standard package check passed and resolved all 63 patches:

```sh
./scripts/package.sh --check --os ipados-17.7.11-21H461
```

The complete target kernel and XnuPrelink build completed:

```sh
./unixnu build kernel -D /root/repo/UniXNU-elish --os ipados-17.7.11-21H461 -j 8
```

Output: `/root/elish-build-20261123/UniXNU/out/elish/ipados-17.7.11-21H461/elish/kernel/kernel.macho`

- Size: 51,122,176 bytes
- SHA-256: `7ab7c4c90647e17ae1a301117fc3e067616431a17b164a5ef01b0867603605b2`
- Target DarwinBoot production Mach-O parser: passed, including arm64 `MH_EXECUTE`, `LC_UNIXTHREAD`, aligned segments, and the configured 256 MiB loader-window bounds
- XnuPrelink host tests: 65 passed
- Device static preflight: passed
- Strict preflight: remains blocked because `memory_map` is unverified

The kernel metadata still reports `instruction_relocation_complete=false`, `register_dataflow_qualified=false`, `abi_qualified=false`, `runtime_qualified=false`, and `loadable=false`.

## Remaining integration

`board.toml` does not yet set `owned_memory_excluded_ranges`, and `dts/elish.dts` still has no memory node. The host build therefore verifies patch application and XNU compilation, not the DarwinBoot-to-XNU runtime handoff. No ADT, iPadOS payload, or final boot image was generated. The UEFI `GetMemoryMap`, the selected physical allocation, and ownership of fixed and dynamic reservations remain unobserved. UFS/APFS root startup also remains unimplemented, and the candidate has not booted on elish.

No ADB, fastboot, USB-enumeration, image-write, or device-boot command was run during this offline continuation.

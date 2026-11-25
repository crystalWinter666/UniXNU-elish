#!/usr/bin/env python3
"""Patch a kernel copy to print a visible post-MMU startup checkpoint."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import sys
import tempfile


MARKER = "ELISH_POST_MMU_VM_INIT_CONSOLE_READY"
PMSG_RETURN_MARKER_ADDRESS = 0xB03FF000
PMSG_RETURN_MARKER = 0x4D564C45


def load_probe(path):
    sys.path.insert(0, str(path.parent))
    spec = importlib.util.spec_from_file_location("entry_probe", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main():
    default_unixnu = Path(__file__).resolve().parents[2] / "UniXNU"
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("kernel", type=Path, help="input kernel copy")
    parser.add_argument("output", type=Path, help="separate diagnostic kernel output")
    parser.add_argument("--unixnu", type=Path, default=default_unixnu)
    parser.add_argument("--hold-seconds", type=int, default=15)
    checkpoint = parser.add_mutually_exclusive_group()
    checkpoint.add_argument(
        "--after-vm-init",
        action="store_true",
        help="call arm_vm_init from the hook, then reset after it returns",
    )
    checkpoint.add_argument(
        "--after-console-create",
        action="store_true",
        help="call PE_create_console from the hook, then reset after it returns",
    )
    checkpoint.add_argument(
        "--before-printf",
        action="store_true",
        help="reset from the _PE_init_printf call site without calling it",
    )
    checkpoint.add_argument(
        "--after-printf-init",
        action="store_true",
        help="call PE_init_printf from the hook, then reset before kprintf",
    )
    parser.add_argument(
        "--pmsg-return-marker",
        action="store_true",
        help="write an _arm_vm_init return marker to reserved pmsg memory",
    )
    args = parser.parse_args()
    if not 1 <= args.hold_seconds <= 120:
        parser.error("--hold-seconds must be in 1..120")
    if args.kernel.resolve() == args.output.resolve():
        parser.error("output must be separate from the input kernel")
    if args.pmsg_return_marker and not args.after_vm_init:
        parser.error("--pmsg-return-marker requires --after-vm-init")

    helper = args.unixnu.resolve() / "scripts/patch-xnu-entry-probe.py"
    probe = load_probe(helper)
    data = bytearray(args.kernel.read_bytes())
    before = hashlib.sha256(data).hexdigest()
    probe.load_segments(data)
    entry = probe.unixthread_pc(data)
    slide = entry - probe.ENTRY_VA
    symbols = probe.kernel_symbols(args.kernel)
    symbols_by_name = {name: address for address, name in symbols.items()}

    arm_init_start, arm_init_end = probe.function_range(symbols, "_arm_init")
    vm_site, vm_target = probe.find_call(data, symbols, "_arm_init", "_arm_vm_init", 1)
    console_site, console_target = probe.find_call(data, symbols, "_arm_init", "_PE_create_console", 1)
    printf_site, printf_target = probe.find_call(data, symbols, "_arm_init", "_PE_init_printf", 1)
    kprintf = symbols_by_name.get("_kprintf")
    if kprintf is None:
        raise SystemExit("kernel has no _kprintf symbol")
    if not arm_init_start <= vm_site < console_site < printf_site < arm_init_end:
        raise SystemExit("expected arm_init order: arm_vm_init, PE_create_console, PE_init_printf")

    code_va = probe.CAVES[4] + slide + (0x500 if args.pmsg_return_marker else 0x100)
    cave_limit = 0x300
    cave_offset = probe.file_offset(code_va)
    if any(data[cave_offset:cave_offset + cave_limit]):
        raise SystemExit(f"post-MMU checkpoint cave at {code_va:#x} is not empty")

    if args.after_vm_init:
        pmsg_write = ""
        if args.pmsg_return_marker:
            if "_ml_phys_write" not in symbols_by_name:
                raise SystemExit("kernel has no _ml_phys_write symbol")
            pmsg_write = f'''  ldr x0, =pmsg_marker_address
  movz w1, #0x4c45
  movk w1, #0x4d56, lsl #16
  bl ml_phys_write
  ldr x0, =pmsg_marker_complement_address
  movz w1, #0xb3ba
  movk w1, #0xb2a9, lsl #16
  bl ml_phys_write
  dsb sy
'''
        source = f'''.text
.global _start
_start:
  bl arm_vm_init
{pmsg_write}  mrs x1, cntpct_el0
  ldr x2, =hold_ticks
  add x2, x1, x2
1:
  isb
  mrs x1, cntpct_el0
  cmp x1, x2
  b.lo 1b
  movz w0, #0x0009
  movk w0, #0x8400, lsl #16
  mov x1, xzr
  mov x2, xzr
  mov x3, xzr
  smc #0
2:
  wfe
  b 2b
.ltorg
'''
        symbols_for_asm = {
            "arm_vm_init": vm_target,
            "hold_ticks": probe.TIMEBASE * args.hold_seconds,
        }
        if args.pmsg_return_marker:
            symbols_for_asm.update({
                "ml_phys_write": symbols_by_name["_ml_phys_write"],
                "pmsg_marker_address": PMSG_RETURN_MARKER_ADDRESS,
                "pmsg_marker_complement_address": PMSG_RETURN_MARKER_ADDRESS + 4,
            })
        hook_site = vm_site
        original_target = vm_target
    elif args.after_console_create:
        source = f'''.text
.global _start
_start:
  bl pe_create_console
  mrs x1, cntpct_el0
  ldr x2, =hold_ticks
  add x2, x1, x2
1:
  isb
  mrs x1, cntpct_el0
  cmp x1, x2
  b.lo 1b
  movz w0, #0x0009
  movk w0, #0x8400, lsl #16
  mov x1, xzr
  mov x2, xzr
  mov x3, xzr
  smc #0
2:
  wfe
  b 2b
.ltorg
'''
        symbols_for_asm = {
            "pe_create_console": console_target,
            "hold_ticks": probe.TIMEBASE * args.hold_seconds,
        }
        hook_site = console_site
        original_target = console_target
    elif args.before_printf:
        source = f'''.text
.global _start
_start:
  mrs x1, cntpct_el0
  ldr x2, =hold_ticks
  add x2, x1, x2
1:
  isb
  mrs x1, cntpct_el0
  cmp x1, x2
  b.lo 1b
  movz w0, #0x0009
  movk w0, #0x8400, lsl #16
  mov x1, xzr
  mov x2, xzr
  mov x3, xzr
  smc #0
2:
  wfe
  b 2b
.ltorg
'''
        symbols_for_asm = {"hold_ticks": probe.TIMEBASE * args.hold_seconds}
        hook_site = printf_site
        original_target = printf_target
    elif args.after_printf_init:
        source = f'''.text
.global _start
_start:
  bl pe_init_printf
  mrs x1, cntpct_el0
  ldr x2, =hold_ticks
  add x2, x1, x2
1:
  isb
  mrs x1, cntpct_el0
  cmp x1, x2
  b.lo 1b
  movz w0, #0x0009
  movk w0, #0x8400, lsl #16
  mov x1, xzr
  mov x2, xzr
  mov x3, xzr
  smc #0
2:
  wfe
  b 2b
.ltorg
'''
        symbols_for_asm = {
            "pe_init_printf": printf_target,
            "hold_ticks": probe.TIMEBASE * args.hold_seconds,
        }
        hook_site = printf_site
        original_target = printf_target
    else:
        source = f'''.text
.global _start
_start:
  bl pe_init_printf
  adr x0, message
  bl kprintf
  mrs x1, cntpct_el0
  ldr x2, =hold_ticks
  add x2, x1, x2
1:
  isb
  mrs x1, cntpct_el0
  cmp x1, x2
  b.lo 1b
  movz w0, #0x0009
  movk w0, #0x8400, lsl #16
  mov x1, xzr
  mov x2, xzr
  mov x3, xzr
  smc #0
2:
  wfe
  b 2b
.balign 4
message:
  .asciz "{MARKER}"
.ltorg
'''
        symbols_for_asm = {
            "pe_init_printf": printf_target,
            "kprintf": kprintf,
            "hold_ticks": probe.TIMEBASE * args.hold_seconds,
        }
        hook_site = printf_site
        original_target = printf_target
    with tempfile.TemporaryDirectory(prefix="elish-post-mmu-") as temporary:
        code = probe.assemble(source, code_va, symbols_for_asm, Path(temporary))
    if len(code) > cave_limit:
        raise SystemExit(f"post-MMU checkpoint is too large for its cave: {len(code):#x}")

    branch_site = struct.unpack_from("<I", data, probe.file_offset(hook_site))[0]
    call_opcode = probe.branch(hook_site, original_target) | 0x80000000
    if branch_site != call_opcode:
        raise SystemExit("checkpoint call changed after symbol resolution")
    data[cave_offset:cave_offset + len(code)] = code
    struct.pack_into(
        "<I", data, probe.file_offset(hook_site),
        probe.branch(hook_site, code_va) | 0x80000000,
    )

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    manifest = {
        "input": str(args.kernel.resolve()),
        "output": str(args.output.resolve()),
        "before_sha256": before,
        "after_sha256": hashlib.sha256(data).hexdigest(),
        "arm_vm_init_call": hex(vm_site),
        "console_init_call": hex(console_site),
        "hook_call": hex(hook_site),
        "checkpoint_point": (
            "after_arm_vm_init" if args.after_vm_init else
            "after_pe_create_console" if args.after_console_create else
            "before_pe_init_printf" if args.before_printf else
            "after_pe_init_printf" if args.after_printf_init else
            "post_console_init"
        ),
        "original_target": hex(original_target),
        "code_va": hex(code_va),
        "code_bytes": len(code),
        "marker": None if (args.after_vm_init or args.after_console_create or args.before_printf or args.after_printf_init) else MARKER,
        "hold_seconds": args.hold_seconds,
        "reset": "PSCI SYSTEM_RESET",
        "pmsg_return_marker": None if not args.pmsg_return_marker else {
            "address": hex(PMSG_RETURN_MARKER_ADDRESS),
            "magic": hex(PMSG_RETURN_MARKER),
            "complement": hex((~PMSG_RETURN_MARKER) & 0xFFFFFFFF),
        },
        "terminal_probe": True,
        "hardware_tested": False,
    }
    args.output.with_suffix(args.output.suffix + ".post-mmu.json").write_text(
        json.dumps(manifest, indent=2) + "\n"
    )
    print(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    main()

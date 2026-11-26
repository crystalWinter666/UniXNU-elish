// SPDX-License-Identifier: BSD-2-Clause-Patent
// RAM-only GOP-backed IOMobileFramebuffer registration and diagnostics for elish.
#include <IOKit/IOPlatformExpert.h>
#include <IOKit/IODeviceMemory.h>
#include <IOKit/IODeviceTreeSupport.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOUserClient.h>
#include <libkern/c++/OSMetaClass.h>
#include <libkern/c++/OSData.h>
#include <kern/task.h>
#include <sys/proc.h>
#include <pexpert/pexpert.h>
#include <console/video_console.h>
#include <mach/kmod.h>
#include <machine/machine_routines.h>

extern "C" kmod_info_t g_kernel_kmod_info;
extern "C" void native_legacy_display_static_init(void) __asm__("__GLOBAL__sub_I_NativeLegacyDisplay.cpp");
extern "C" IOService *native_legacy_display_allocate(void);
extern "C" kern_return_t OSKextLoadKextWithIdentifier(const char *);

static IOService *pendingDisplay;
static IOService *displayObject;

extern "C" {
volatile uint32_t native_legacy_stage;
volatile int native_legacy_meta_status = -1;
volatile uint32_t native_legacy_init_result, native_legacy_start_result, native_legacy_probe_match;
volatile uint32_t native_legacy_dt_attached, native_legacy_released;
volatile uint64_t native_legacy_object, native_legacy_video_base;
volatile int native_legacy_surface_status = -1;
volatile uint32_t native_legacy_surface_found;
volatile uint32_t native_legacy_pm_stage, native_legacy_pm_calls;
volatile int native_legacy_pm_status = -1;
volatile uint32_t native_legacy_swap_accepted_count, native_legacy_swap_rejected_count, native_legacy_swap_validation_reason;
volatile uint64_t native_legacy_rej_surface[7];
volatile uint32_t native_legacy_frame_count, native_legacy_transfer_calls, native_legacy_registration_calls;
static volatile uint32_t successfulClientOpens;
volatile int native_legacy_transfer_status = -1;
volatile uint64_t native_legacy_transfer_bytes, native_legacy_copy_n, native_legacy_copy_last, native_legacy_copy_max;
volatile uint32_t native_legacy_client_calls, native_legacy_retirement_calls, native_legacy_gpu_fallbacks;
volatile int native_legacy_client_pid, native_legacy_client_status = -1;
volatile uint64_t native_legacy_gop_physical, native_legacy_gop_bytes, native_legacy_gop_virtual;
volatile uint32_t native_legacy_gop_width, native_legacy_gop_height;
volatile uint64_t native_legacy_vsync_events[6];

// The CPU-only elish kernel has neither an Adreno fence provider nor a DPU aperture.
__attribute__((weak)) bool native_adreno_fences(uint32_t *, uint32_t *) { return false; }
__attribute__((weak)) bool native_adreno_fences_detail(uint32_t *, uint32_t *, uint32_t *) { return false; }
__attribute__((weak)) int native_gpu_trace_enabled(void) { return 0; }
__attribute__((weak)) void native_gpu_trace_record(uint32_t, uint32_t, const uint64_t *, uint32_t) {}
__attribute__((weak)) volatile uint32_t native_ufs_dpu_state;
__attribute__((weak)) uint32_t native_ufs_dpu_map(unsigned, const uint64_t *, unsigned) { return 0; }

// The display archive binds these IOSurface accessors to the qualified 21H461 kernel.
extern IOMemoryDescriptor *native_legacy_surface_descriptor(OSObject *);
extern uint64_t native_legacy_surface_width(OSObject *);
extern uint64_t native_legacy_surface_height(OSObject *);
extern uint64_t native_legacy_surface_stride_raw(OSObject *);
extern uint64_t native_legacy_surface_allocation(OSObject *);
extern uint32_t native_legacy_surface_format(OSObject *);
extern uint32_t native_legacy_surface_planes(OSObject *);

uint64_t native_legacy_surface_stride(OSObject *surface) {
	const uint64_t raw = native_legacy_surface_stride_raw(surface);
	const uint64_t allocation = native_legacy_surface_allocation(surface);
	const uint64_t height = native_legacy_surface_height(surface);
	return raw && allocation && height && raw <= allocation / height / 4 ? raw * 4 : raw;
}

bool native_legacy_prepare_scanout(void) {
	return !ml_at_interrupt_context() && ml_get_interrupts_enabled();
}

bool native_legacy_gpu_offer(void *, IOMemoryDescriptor *, uint64_t, uint32_t, uint32_t, uint32_t) {
	// No GPU copy engine is enabled for this CPU-only bring-up; core's existing CPU path remains authoritative.
	return false;
}

static bool usableConsole(PE_Video *video) {
	// Measured DarwinBoot input in reports/elish-darwinboot-ebs-20261124.txt.
	return !PE_current_console(video) && video->v_baseAddr == 0x9c000000 &&
	       video->v_width == 1600 && video->v_height == 2560 && video->v_rowBytes == 6400 &&
	       video->v_depth == 32 && video->v_display == kPEGraphicsMode && !video->v_rotate;
}

static void verdict(const char *where) {
	unsigned enabled = 0;
	if (PE_parse_boot_argn("offline-legacy-verdict", &enabled, sizeof(enabled)) && enabled) {
		IOLog("LegacyDisplay: %s stage=%u meta=%d init=%u match=%u start=%u surface=%d/%u base=%llx mode=%ux%u bytes=%llu\n",
		    where, __atomic_load_n(&native_legacy_stage, __ATOMIC_RELAXED), native_legacy_meta_status,
		    native_legacy_init_result, native_legacy_probe_match,
		    native_legacy_start_result, native_legacy_surface_status, native_legacy_surface_found, native_legacy_video_base,
		    native_legacy_gop_width, native_legacy_gop_height, native_legacy_gop_bytes);
	}
}

void native_legacy_display_probe(void) {
	if (__atomic_load_n(&native_legacy_stage, __ATOMIC_RELAXED)) return;
	__atomic_store_n(&native_legacy_stage, 1, __ATOMIC_RELAXED);
	void *handle = OSMetaClass::preModLoad(g_kernel_kmod_info.name);
	if (!handle) return verdict("preModLoad");
	native_legacy_display_static_init();
	native_legacy_meta_status = OSMetaClass::postModLoad(handle);
	if (native_legacy_meta_status) return verdict("postModLoad");
	PE_Video video = {};
	__atomic_store_n(&native_legacy_stage, 2, __ATOMIC_RELAXED);
	if (!usableConsole(&video)) return verdict("console");
	IOService *display = native_legacy_display_allocate();
	native_legacy_object = (uintptr_t)display;
	if (!display) return verdict("allocate");
	displayObject = display;
	__atomic_store_n(&native_legacy_stage, 3, __ATOMIC_RELAXED);
	native_legacy_init_result = display->init();
	unsigned phase = 1;
	PE_parse_boot_argn("offline-legacy-phase", &phase, sizeof(phase));
	if (native_legacy_init_result && phase == 4) {
		pendingDisplay = display;
		return;
	}
	display->release();
	displayObject = nullptr;
	native_legacy_released = 1;
	verdict("init-or-phase");
}

void native_legacy_display_late_probe(void) {
	unsigned enabled = 0;
	if (!PE_parse_boot_argn("offline-legacy-register", &enabled, sizeof(enabled)) || enabled != 1) return;
	IOService *display = pendingDisplay;
	if (!display) return verdict("no-pending-instance");
	pendingDisplay = nullptr;
	native_legacy_surface_status = OSKextLoadKextWithIdentifier("com.apple.iokit.IOSurface");
	OSDictionary *matching = IOService::nameMatching("IOCoreSurfaceRoot");
	IOService *surface = matching ? IOService::waitForMatchingService(matching, 5000000000ULL) : nullptr;
	native_legacy_surface_found = surface != nullptr;
	OSSafeReleaseNULL(surface);
	OSSafeReleaseNULL(matching);
	PE_Video video = {};
	if (native_legacy_surface_status || !native_legacy_surface_found || !usableConsole(&video)) {
		display->release(); displayObject = nullptr; native_legacy_released = 1;
		return verdict("surface-or-console");
	}
	const uint64_t bytes = uint64_t(video.v_rowBytes) * video.v_height;
	// Firmware documents this reserve as [0x9c000000, 0x9e400000); reject any map outside it.
	if (bytes > 0x02400000ULL) {
		display->release(); displayObject = nullptr; native_legacy_released = 1;
		return verdict("outside-firmware-reserve");
	}
	native_legacy_video_base = video.v_baseAddr;
	__atomic_store_n(&native_legacy_stage, 4, __ATOMIC_RELAXED);
	IOPlatformDevice *nub = new IOPlatformDevice;
	IOService *parent = IOService::getResourceService();
	IODeviceMemory *memory = IODeviceMemory::withRange(video.v_baseAddr, bytes);
	OSArray *ranges = OSArray::withCapacity(1);
	IORegistryEntry *armIO = IORegistryEntry::fromPath("/arm-io", gIODTPlane);
	bool nubAttached = false, nubStarted = false, attached = false, started = false;
	if (nub && memory && ranges && parent && armIO && nub->init() && ranges->setObject(memory)) {
		nub->setDeviceMemory(ranges);
		nub->setName("NativeGOPFramebuffer");
		nubAttached = nub->attach(parent);
		nubStarted = nubAttached && nub->start(parent);
		const char type[] = "display-subsystem";
		OSData *data = OSData::withBytes(type, sizeof(type));
		if (nubStarted && data && nub->setProperty("device_type", data)) {
			nub->setName("gop-display", gIODTPlane);
			native_legacy_dt_attached = nub->attachToParent(armIO, gIODTPlane);
		}
		OSSafeReleaseNULL(data);
		SInt32 score = 0;
		native_legacy_probe_match = native_legacy_dt_attached && display->probe(nub, &score) == display;
		attached = native_legacy_probe_match && display->attach(nub);
		if (attached) {
			__atomic_store_n(&native_legacy_stage, 5, __ATOMIC_RELAXED);
			started = display->start(nub);
			native_legacy_start_result = started;
		}
	}
	if (started) {
		__atomic_store_n(&native_legacy_stage, 10, __ATOMIC_RELAXED);
		ranges->release();
		memory->release();
		OSSafeReleaseNULL(armIO);
		return;
	}
	if (attached) display->detach(nub);
	if (native_legacy_dt_attached) nub->detachFromParent(armIO, gIODTPlane);
	if (nubStarted) nub->stop(parent);
	if (nubAttached) nub->detach(parent);
	OSSafeReleaseNULL(ranges); OSSafeReleaseNULL(memory); OSSafeReleaseNULL(nub); OSSafeReleaseNULL(armIO);
	display->release(); displayObject = nullptr; native_legacy_released = 1;
	verdict("probe-attach-start");
}

bool native_legacy_gop_mapping_observed(IOService *object, IOMemoryMap *mapping, uint32_t width, uint32_t height) {
	if (!object || object != displayObject || !mapping) return false;
	PE_Video video = {};
	if (!usableConsole(&video) || mapping->getPhysicalAddress() != video.v_baseAddr ||
	    mapping->getLength() != uint64_t(video.v_rowBytes) * video.v_height ||
	    video.v_width != width || video.v_height != height) return false;
	native_legacy_gop_physical = mapping->getPhysicalAddress();
	native_legacy_gop_virtual = mapping->getVirtualAddress();
	native_legacy_gop_bytes = mapping->getLength();
	__atomic_store_n(&native_legacy_gop_width, width, __ATOMIC_RELAXED);
	__atomic_store_n(&native_legacy_gop_height, height, __ATOMIC_RELAXED);
	IOLog("NativeLegacyDisplay: GOP mapped pa=%llx va=%llx bytes=%llu mode=%ux%u row=%lu\n",
	    native_legacy_gop_physical, native_legacy_gop_virtual, native_legacy_gop_bytes, width, height, video.v_rowBytes);
	return true;
}

void native_legacy_registration_observed(IOService *) {
	if (__atomic_add_fetch(&native_legacy_registration_calls, 1, __ATOMIC_RELAXED) == 1)
		IOLog("NativeLegacyDisplay: service registered stage=%u\n", __atomic_load_n(&native_legacy_stage, __ATOMIC_RELAXED));
}
void native_legacy_swap_stage(unsigned) {}
void native_legacy_client_enter(task_t task, uint32_t) {
	__atomic_add_fetch(&native_legacy_client_calls, 1, __ATOMIC_RELAXED);
	proc_t p = task ? (proc_t)get_bsdtask_info(task) : nullptr;
	__atomic_store_n(&native_legacy_client_pid, p ? proc_pid(p) : 0, __ATOMIC_RELAXED);
}
void native_legacy_client_observed(IOReturn status, IOUserClient *client) {
	__atomic_store_n(&native_legacy_client_status, status, __ATOMIC_RELAXED);
	if (status == kIOReturnSuccess && client) __atomic_add_fetch(&successfulClientOpens, 1, __ATOMIC_RELAXED);
}
void native_legacy_pm_observed(unsigned stage, IOReturn status, unsigned long) {
	__atomic_store_n(&native_legacy_pm_stage, stage, __ATOMIC_RELAXED);
	__atomic_store_n(&native_legacy_pm_status, status, __ATOMIC_RELAXED);
	__atomic_add_fetch(&native_legacy_pm_calls, 1, __ATOMIC_RELAXED);
}
void native_legacy_frame_observed(unsigned, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t) { __atomic_add_fetch(&native_legacy_frame_count, 1, __ATOMIC_RELAXED); }
void native_legacy_frame_colors_observed(const uint32_t *, const uint64_t *, uint64_t) {}
void native_legacy_swap_validation_observed(bool accepted, unsigned reason, const void *, const void *) {
	if (accepted) __atomic_add_fetch(&native_legacy_swap_accepted_count, 1, __ATOMIC_RELAXED);
	else { __atomic_add_fetch(&native_legacy_swap_rejected_count, 1, __ATOMIC_RELAXED); __atomic_store_n(&native_legacy_swap_validation_reason, reason, __ATOMIC_RELAXED); }
}
void native_legacy_surface_rejected(uint64_t w, uint64_t h, uint32_t f, uint32_t p, uint64_t s, uint64_t a, uint64_t n) {
	const uint64_t values[7] = {w,h,f,p,s,a,n};
	for (unsigned i=0;i<7;i++) __atomic_store_n(&native_legacy_rej_surface[i], values[i], __ATOMIC_RELAXED);
}
void native_legacy_transfer_observed(IOReturn status, const void *, uint64_t bytes) {
	__atomic_store_n(&native_legacy_transfer_status, status, __ATOMIC_RELAXED);
	__atomic_store_n(&native_legacy_transfer_bytes, bytes, __ATOMIC_RELAXED);
	__atomic_add_fetch(&native_legacy_transfer_calls, 1, __ATOMIC_RELAXED);
}
void native_legacy_copy_timing_observed(uint64_t ticks, uint64_t bytes) {
	__atomic_store_n(&native_legacy_copy_last, ticks, __ATOMIC_RELAXED);
	uint64_t old = __atomic_load_n(&native_legacy_copy_max, __ATOMIC_RELAXED);
	while (old < ticks && !__atomic_compare_exchange_n(&native_legacy_copy_max, &old, ticks, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED)) {}
	if (__atomic_add_fetch(&native_legacy_copy_n, 1, __ATOMIC_RELAXED) == 1)
		IOLog("NativeLegacyDisplay: first software frame bytes=%llu mode=%ux%u\n", bytes,
		    __atomic_load_n(&native_legacy_gop_width, __ATOMIC_RELAXED),
		    __atomic_load_n(&native_legacy_gop_height, __ATOMIC_RELAXED));
}
void native_legacy_retirement_observed(IOService *) { __atomic_add_fetch(&native_legacy_retirement_calls, 1, __ATOMIC_RELAXED); }
void native_legacy_gpu_fallback_observed(int) { __atomic_add_fetch(&native_legacy_gpu_fallbacks, 1, __ATOMIC_RELAXED); }
void native_legacy_vsync_observed(unsigned event, unsigned) { if (event < 6) __atomic_add_fetch(&native_legacy_vsync_events[event], 1, __ATOMIC_RELAXED); }

__attribute__((noreturn, noinline)) void native_legacy_backend_guard(unsigned slot, void *, void *caller) {
	panic("NativeLegacyDisplay: unsupported original leaf=%u caller=%llx stage=%u swaps=%u/%u copy=%d bytes=%llu",
	    slot, (uint64_t)(uintptr_t)caller, __atomic_load_n(&native_legacy_stage, __ATOMIC_RELAXED),
	    __atomic_load_n(&native_legacy_swap_accepted_count, __ATOMIC_RELAXED),
	    __atomic_load_n(&native_legacy_swap_rejected_count, __ATOMIC_RELAXED),
	    __atomic_load_n(&native_legacy_transfer_status, __ATOMIC_RELAXED),
	    __atomic_load_n(&native_legacy_transfer_bytes, __ATOMIC_RELAXED));
}

// Keep the panic report compact enough for the camera to capture each counter.
void elish_snapshot_display(unsigned values[8]) {
	values[0] = __atomic_load_n(&native_legacy_stage, __ATOMIC_RELAXED);
	values[1] = __atomic_load_n(&successfulClientOpens, __ATOMIC_RELAXED);
	values[2] = __atomic_load_n(&native_legacy_copy_n, __ATOMIC_RELAXED);
	values[3] = __atomic_load_n(&native_legacy_frame_count, __ATOMIC_RELAXED);
	values[4] = __atomic_load_n(&native_legacy_retirement_calls, __ATOMIC_RELAXED);
	values[5] = __atomic_load_n(&native_legacy_swap_accepted_count, __ATOMIC_RELAXED);
	values[6] = __atomic_load_n(&native_legacy_swap_rejected_count, __ATOMIC_RELAXED);
	values[7] = __atomic_load_n(&native_legacy_swap_validation_reason, __ATOMIC_RELAXED);
}
}

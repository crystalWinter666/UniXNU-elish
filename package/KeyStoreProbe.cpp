// SPDX-License-Identifier: BSD-2-Clause-Patent
#include <IOKit/IODeviceTreeSupport.h>
#include <IOKit/IOService.h>
#include <libkern/c++/OSData.h>
#include <libkern/c++/OSSymbol.h>
#include <mach/kmod.h>
#include <pexpert/pexpert.h>

extern kmod_info_t g_kernel_kmod_info;
extern "C" IOService *native_keyevents_allocate();
extern "C" void keyEventsInit() __asm__("__GLOBAL__sub_I_NativeKeyStoreEvents.cpp");

namespace {
// The owner seed supplies these ADT values. This identity has no SEP protection.
struct Identity {
    uint8_t uniqueChip[8];
    uint8_t chip[4];
};
Identity identity;
bool stateEnabled;
unsigned opens, refused, calls;
int status = kIOReturnNotReady;

bool readProperty(IORegistryEntry *entry, const char *name, void *value, size_t size)
{
    const auto *data = OSDynamicCast(OSData, entry->getProperty(name));
    if (!data || data->getLength() != size) return false;
    memcpy(value, data->getBytesNoCopy(), size);
    return true;
}

IOReturn publish()
{
    const auto name = OSSymbol::withCString("AppleKeyStore");
    if (!name) return kIOReturnNoMemory;
    const bool alreadyRegistered = OSMetaClass::getMetaClassWithName(name) != nullptr;
    name->release();
    if (alreadyRegistered) return kIOReturnExclusiveAccess;
    auto *provider = IOService::getResourceService();
    if (!provider) return kIOReturnNoDevice;
    void *registration = OSMetaClass::preModLoad(g_kernel_kmod_info.name);
    if (!registration) return kIOReturnNoMemory;
    keyEventsInit();
    const auto error = OSMetaClass::postModLoad(registration);
    if (error) return error;
    auto *service = native_keyevents_allocate();
    if (!service) return kIOReturnNoMemory;
    const bool attached = service->init() && service->attach(provider);
    if (!attached || !service->start(provider)) {
        if (attached) service->detach(provider);
        service->release();
        return kIOReturnError;
    }
    service->release();
    return kIOReturnSuccess;
}
} // namespace

extern "C" void elish_key_store_start()
{
    unsigned enabled = 0;
    if (!PE_parse_boot_argn("elish-aks", &enabled, sizeof(enabled)) || enabled != 1) return;
    auto *chosen = IORegistryEntry::fromPath("/chosen", gIODTPlane);
    if (!chosen) return;
    uint32_t ephemeral = 0;
    const bool ready = readProperty(chosen, "ephemeral-storage", &ephemeral, sizeof(ephemeral)) && ephemeral == 1 &&
        readProperty(chosen, "unique-chip-id", identity.uniqueChip, sizeof(identity.uniqueChip)) &&
        readProperty(chosen, "chip-id", identity.chip, sizeof(identity.chip));
    chosen->release();
    IOReturn result = kIOReturnNotPermitted;
    if (ready) {
        __atomic_store_n(&stateEnabled, true, __ATOMIC_RELEASE);
        result = publish();
        if (result != kIOReturnSuccess) __atomic_store_n(&stateEnabled, false, __ATOMIC_RELEASE);
    }
    __atomic_store_n(&status, result, __ATOMIC_RELAXED);
    IOLog("elish: RAM Data AppleKeyStore publish 0x%x\n", static_cast<unsigned>(result));
}

extern "C" bool native_aks_state_enabled()
{
    return __atomic_load_n(&stateEnabled, __ATOMIC_ACQUIRE);
}

extern "C" uint32_t native_aks_lock_state() { return 3; } // No passcode on the RAM Data volume.
extern "C" void native_keyevents_open_refused() { __atomic_add_fetch(&refused, 1, __ATOMIC_RELAXED); }
extern "C" void native_aks_opened() { __atomic_add_fetch(&opens, 1, __ATOMIC_RELAXED); }
extern "C" void native_aks_call(uint32_t, int32_t) { __atomic_add_fetch(&calls, 1, __ATOMIC_RELAXED); }

extern "C" size_t native_aks_device_id(uint8_t *out, size_t capacity)
{
    if (capacity < sizeof(identity) || !native_aks_state_enabled()) return 0;
    memcpy(out, &identity, sizeof(identity));
    return sizeof(identity);
}

extern "C" void elish_key_store_snapshot(char *out, size_t size)
{
    snprintf(out, size, "AKS %x opens %u calls %u refused %u",
        static_cast<unsigned>(__atomic_load_n(&status, __ATOMIC_RELAXED)),
        __atomic_load_n(&opens, __ATOMIC_RELAXED), __atomic_load_n(&calls, __ATOMIC_RELAXED),
        __atomic_load_n(&refused, __ATOMIC_RELAXED));
}

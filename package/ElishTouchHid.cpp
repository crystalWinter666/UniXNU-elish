// SPDX-License-Identifier: BSD-2-Clause-Patent
// Board-owned digitizer reports; the original IOHIDFamily parses and delivers them.
#include "ElishTouchFrame.h"
#include "OriginalHidDeviceABI.h"

#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOTimerEventSource.h>
#include <IOKit/IOWorkLoop.h>
#include <libkern/c++/OSIterator.h>
#include <libkern/c++/OSKext.h>
#include <libkern/c++/OSString.h>
#include <libkern/c++/OSSymbol.h>
#include <mach/kmod.h>

extern kmod_info_t g_kernel_kmod_info;
extern "C" kern_return_t OSKextLoadKextWithIdentifier(const char *);
extern "C" void elish_touch_hid_init_class() __asm__("__GLOBAL__sub_I_ElishTouchHid.cpp");

namespace {
constexpr unsigned kContacts = 10;
constexpr uint8_t kReportID = 3;
constexpr unsigned kReportBytes = 1 + 6 * kContacts + 1;
static_assert(sizeof(ElishTouchFrame::points) / sizeof(ElishTouchPoint) == kContacts);

// The retained 21H461 codeless EventDriver has a keyboard personality but no
// generic SPI touchscreen personality. Match as a keyboard first, like Sheng;
// there is no code path that emits keyboard (report 1) events.
#define FINGER_DESC \
    0x09,0x22,0xa1,0x02, /* Finger, logical collection */ \
    0x09,0x42,0x15,0x00,0x25,0x01,0x75,0x01,0x95,0x01,0x81,0x02, /* Tip Switch */ \
    0x09,0x47,0x81,0x02,0x75,0x06,0x81,0x03, /* Touch Valid, padding */ \
    0x09,0x51,0x75,0x08,0x95,0x01,0x15,0x00,0x26,0xff,0x00,0x81,0x02, /* Contact Identifier */ \
    0x05,0x01,0x09,0x30,0x15,0x00,0x26,0xff,0x7f,0x75,0x10,0x95,0x01,0x81,0x02, \
    0x09,0x31,0x81,0x02,0x05,0x0d,0xc0 /* X, Y (0..32767) */
constexpr uint8_t kDescriptor[] = {
    0x05,0x01,0x09,0x06,0xa1,0x01,0x85,0x01,
    0x05,0x07,0x19,0xe0,0x29,0xe7,0x15,0x00,0x25,0x01,0x75,0x01,0x95,0x08,0x81,0x02,
    0x75,0x08,0x95,0x01,0x81,0x03,
    0x19,0x00,0x29,0x65,0x15,0x00,0x25,0x65,0x75,0x08,0x95,0x06,0x81,0x00,0xc0,
    0x05,0x0d,0x09,0x04,0xa1,0x01,0x85,kReportID,
    FINGER_DESC, FINGER_DESC, FINGER_DESC, FINGER_DESC, FINGER_DESC,
    FINGER_DESC, FINGER_DESC, FINGER_DESC, FINGER_DESC, FINGER_DESC,
    0x09,0x54,0x75,0x08,0x95,0x01,0x15,0x00,0x26,0xff,0x00,0x81,0x02,0xc0,
};
#undef FINGER_DESC

enum StartStage : unsigned { NotStarted, Starting, FamilyLoaded, ResourceFound, ClassRegistered, Published, Failed };
unsigned gStage;
int gFamilyStatus = -1, gPersonalityStatus = -1, gClassStatus = -1;
unsigned gClassSize, gMatchPolls, gMatched, gSubmissions, gAttempts, gReports, gLifts;
unsigned gBeforeMatch, gInvalidFrames;
unsigned gEdgeHook, gEdgeBegins, gEdgeEvents;
int gLastReport = -1;

template <typename T> T load(const T &value) { return __atomic_load_n(&value, __ATOMIC_RELAXED); }
template <typename T> void store(T &value, T update) { __atomic_store_n(&value, update, __ATOMIC_RELAXED); }
void increment(unsigned &counter, unsigned amount = 1) { __atomic_fetch_add(&counter, amount, __ATOMIC_RELAXED); }

// Return a retained match strictly below this bridge's provider tree.
IORegistryEntry *copyEventDriver(IORegistryEntry *parent, unsigned depth)
{
    if (!depth) return nullptr;
    auto children = parent->getChildIterator(gIOServicePlane);
    if (!children) return nullptr;
    IORegistryEntry *found = nullptr;
    while (auto *object = children->getNextObject()) {
        auto *child = OSDynamicCast(IOService, object);
        if (!child || child->isInactive()) continue;
        if (child->metaCast("IOHIDEventDriver")) {
            if (child->getState() & kIOServiceRegisteredState) {
                child->retain();
                found = child;
                break;
            }
            continue;
        }
        if ((found = copyEventDriver(child, depth - 1))) break;
    }
    OSSafeReleaseNULL(children);
    return found;
}

// The ordinary 21H461 event driver does not annotate edge gestures. Wrap only
// the transducer service matched to this device; never change Apple's table or
// add input. The builder verifies these private ABI pins against original code.
namespace edge {
using namespace ElishHidABI;
constexpr uint32_t kDigitizer = 11, kX = 0xb0000, kY = 0xb0001, kIndex = 0xb0005;
constexpr uint32_t kMask = 0xb0007, kTouch = 0xb0009, kMaskTouch = 2, kFromEdgeTip = 0x800;
constexpr int32_t kEdgeX = 48 * 65536 / kElishTouchWidth;
constexpr int32_t kEdgeY = 48 * 65536 / kElishTouchHeight;
constexpr unsigned kHeaderWords = 2;
uintptr_t gTable[kHeaderWords + kDriverSlots];
unsigned gFromEdge[256];

const uintptr_t *vtable(const void *object)
{
    return *reinterpret_cast<const uintptr_t *const *>(object);
}

template <typename R, typename... Args>
R call(uintptr_t function, OSObject *event, Args... args)
{
    return reinterpret_cast<R (*)(OSObject *, Args..., IOOptionBits)>(function)(event, args..., 0);
}

bool isDigitizer(OSObject *event)
{
    if (!event) return false;
    const auto *table = vtable(event);
    return reinterpret_cast<uintptr_t>(table) == kEventVtable && table[18] == kEventMethod18 &&
        table[23] == kEventMethod23 && table[24] == kEventMethod24 && table[25] == kEventMethod25 &&
        call<uint32_t>(kEventMethod18, event) == kDigitizer;
}

void setMask(OSObject *event, uint32_t mask)
{
    call<void>(kEventMethod24, event, kMask, static_cast<int32_t>(mask | kFromEdgeTip));
}

bool markContact(OSObject *event)
{
    if (!isDigitizer(event)) return false;
    const auto index = static_cast<uint32_t>(call<int32_t>(kEventMethod23, event, kIndex));
    if (index >= sizeof(gFromEdge) / sizeof(gFromEdge[0])) return false;
    const auto mask = static_cast<uint32_t>(call<int32_t>(kEventMethod23, event, kMask));
    const bool down = call<int32_t>(kEventMethod23, event, kTouch) != 0;
    if ((mask & kMaskTouch) && down) {
        const int32_t x = call<int32_t>(kEventMethod25, event, kX);
        const int32_t y = call<int32_t>(kEventMethod25, event, kY);
        const bool beganAtEdge = x < kEdgeX || x > 65536 - kEdgeX || y < kEdgeY || y > 65536 - kEdgeY;
        store(gFromEdge[index], unsigned(beganAtEdge));
        if (beganAtEdge) increment(gEdgeBegins);
    }
    const bool edged = load(gFromEdge[index]) != 0;
    if (edged) setMask(event, mask);
    if ((mask & kMaskTouch) && !down) store(gFromEdge[index], 0U);
    return edged;
}

void dispatch(IOService *self, OSObject *event, IOOptionBits options)
{
    if (isDigitizer(event)) {
        auto *children = *reinterpret_cast<OSArray **>(reinterpret_cast<uint8_t *>(event) + kEventChildren);
        bool edged = false;
        if (auto *array = OSDynamicCast(OSArray, children)) {
            for (unsigned i = 0; i < array->getCount(); ++i) edged |= markContact(array->getObject(i));
            if (edged) setMask(event, static_cast<uint32_t>(call<int32_t>(kEventMethod23, event, kMask)));
        } else if (!children) {
            edged = markContact(event);
        }
        if (edged) increment(gEdgeEvents);
    }
    reinterpret_cast<void (*)(IOService *, OSObject *, IOOptionBits)>(kDispatch)(self, event, options);
}

bool install(IORegistryEntry *driver)
{
    const auto *original = vtable(driver);
    if (reinterpret_cast<uintptr_t>(original) != kDriverVtable || original[kDispatchSlot] != kDispatch)
        return false;
    memcpy(gTable, original - kHeaderWords, sizeof(gTable));
    gTable[kHeaderWords + kDispatchSlot] = reinterpret_cast<uintptr_t>(dispatch);
    __atomic_store_n(reinterpret_cast<uintptr_t **>(driver), gTable + kHeaderWords, __ATOMIC_RELEASE);
    return true;
}
} // namespace edge

bool publishDigitizerProperties(IORegistryEntry *entry)
{
    // Leave the device's matching usages alone. After the keyboard personality
    // binds, classify this interface and its event service as direct touch.
    return entry->setProperty("Built-In", true) && entry->setProperty("DisplayIntegrated", true) &&
        entry->setProperty("PrimaryUsagePage", 0x0dULL, 32) && entry->setProperty("PrimaryUsage", 4ULL, 32);
}
} // namespace

class ElishTouchHid final : public IOHIDDevice {
    OSDeclareDefaultStructors(ElishTouchHid)
    IOWorkLoop *loop = nullptr;
    IOTimerEventSource *matchTimer = nullptr;
    IORegistryEntry *eventDriver = nullptr;
    ElishTouchFrame previous{};

    bool match()
    {
        if (isInactive()) return false;
        if (eventDriver) {
            const bool ready = !static_cast<IOService *>(eventDriver)->isInactive();
            if (!ready) store(gMatched, 0U);
            return ready;
        }
        increment(gMatchPolls);
        auto *driver = copyEventDriver(this, 4);
        if (!driver) return false;
        auto interface = driver->copyParentEntry(gIOServicePlane);
        const bool classified = interface && interface->metaCast("IOHIDInterface") &&
            publishDigitizerProperties(interface) && publishDigitizerProperties(driver);
        OSSafeReleaseNULL(interface);
        if (classified) {
            eventDriver = driver;
            const bool edgeHook = edge::install(driver);
            store(gEdgeHook, edgeHook ? 1U : 2U);
            store(gMatched, 1U);
            IOLog("elish-touch-hid: matched IOHIDEventDriver id=%llu edge=%u\n",
                static_cast<unsigned long long>(driver->getRegistryEntryID()), unsigned(edgeHook));
            return true;
        }
        driver->release();
        return false;
    }

    static void pollMatch(OSObject *owner, IOTimerEventSource *timer)
    {
        auto *self = static_cast<ElishTouchHid *>(owner);
        if (!self->match()) timer->setTimeoutMS(250);
    }

    static IOReturn submitAction(OSObject *owner, void *frame, void *, void *, void *)
    {
        return static_cast<ElishTouchHid *>(owner)->send(*static_cast<const ElishTouchFrame *>(frame));
    }

    IOReturn send(const ElishTouchFrame &frame)
    {
        if (!match()) {
            increment(gBeforeMatch);
            return kIOReturnNotReady;
        }
        for (const auto &point : frame.points) {
            if (point.active && (point.x >= kElishTouchWidth || point.y >= kElishTouchHeight)) {
                increment(gInvalidFrames);
                return kIOReturnBadArgument;
            }
        }

        uint8_t report[kReportBytes] = {kReportID};
        unsigned entries = 0;
        auto append = [&](unsigned id, const ElishTouchPoint &point, bool touch) {
            const unsigned x = point.x * 32767U / (kElishTouchWidth - 1);
            const unsigned y = point.y * 32767U / (kElishTouchHeight - 1);
            auto *finger = report + 1 + 6 * entries++;
            finger[0] = touch ? 3 : 2; // Keep Touch Valid set on both down and lift.
            finger[1] = static_cast<uint8_t>(id);
            finger[2] = static_cast<uint8_t>(x);
            finger[3] = static_cast<uint8_t>(x >> 8);
            finger[4] = static_cast<uint8_t>(y);
            finger[5] = static_cast<uint8_t>(y >> 8);
        };
        for (unsigned i = 0; i < kContacts; ++i)
            if (frame.points[i].active) append(i + 1, frame.points[i], true);
        const unsigned down = entries;
        // The frame has stable IDs (slot + 1). Active and lifted IDs are
        // disjoint, so even a ten-contact transition fits in one report.
        for (unsigned i = 0; i < kContacts; ++i)
            if (!frame.points[i].active && previous.points[i].active) append(i + 1, previous.points[i], false);
        if (!entries) return kIOReturnSuccess;
        report[kReportBytes - 1] = static_cast<uint8_t>(entries);

        increment(gAttempts);
        auto buffer = IOBufferMemoryDescriptor::withBytes(report, sizeof(report), kIODirectionNone);
        const IOReturn result = buffer ? handleReport(buffer, 0, 0) : kIOReturnNoMemory;
        OSSafeReleaseNULL(buffer);
        store(gLastReport, result);
        if (result == kIOReturnSuccess) {
            previous = frame;
            increment(gReports);
            increment(gLifts, entries - down);
        } else {
            // An error may follow partial delivery. Retain all potentially
            // down contacts so that the next valid frame still sends lifts.
            for (unsigned i = 0; i < kContacts; ++i)
                if (frame.points[i].active) previous.points[i] = frame.points[i];
        }
        return result;
    }

public:
    bool submit(const ElishTouchFrame &frame)
    {
        return loop->runAction(submitAction, this, const_cast<ElishTouchFrame *>(&frame)) == kIOReturnSuccess;
    }
    IOWorkLoop *getWorkLoop() const override { return loop; }
    OSString *newTransportString() const override { return OSString::withCString("SPI"); }
    OSString *newProductString() const override { return OSString::withCString("Elish NT36523 Touchscreen"); }
    IOReturn newReportDescriptor(IOMemoryDescriptor **out) const override
    {
        if (!out) return kIOReturnBadArgument;
        *out = IOBufferMemoryDescriptor::withBytes(kDescriptor, sizeof(kDescriptor), kIODirectionNone);
        return *out ? kIOReturnSuccess : kIOReturnNoMemory;
    }
    bool start(IOService *provider) override
    {
        loop = IOWorkLoop::workLoop();
        if (!loop || !setProperty("HIDDefaultBehavior", true) ||
            !setProperty("Built-In", true) || !setProperty("DisplayIntegrated", true) ||
            !setProperty("DigitizerCollectionDispatch", true) ||
            !setProperty("AbsoluteAxisBoundsRemovalPercentage", 0ULL, 32)) return false;
        matchTimer = IOTimerEventSource::timerEventSource(this, pollMatch);
        if (!matchTimer) return false;
        if (loop->addEventSource(matchTimer) != kIOReturnSuccess) return false;
        if (!IOHIDDevice::start(provider)) {
            loop->removeEventSource(matchTimer);
            return false;
        }
        // Native matching is asynchronous; do not wait for a real contact to
        // publish the direct-touch properties of the matched event service.
        matchTimer->setTimeoutMS(1);
        return true;
    }
    void stop(IOService *provider) override
    {
        matchTimer->cancelTimeout();
        loop->removeEventSource(matchTimer);
        store(gMatched, 0U);
        IOHIDDevice::stop(provider);
    }
    void free() override
    {
        OSSafeReleaseNULL(eventDriver);
        OSSafeReleaseNULL(matchTimer);
        OSSafeReleaseNULL(loop);
        IOHIDDevice::free();
    }
};
OSDefineMetaClassAndStructors(ElishTouchHid, IOHIDDevice)

namespace {
ElishTouchHid *gDevice;

bool startFailed(const char *where)
{
    store(gStage, unsigned(Failed));
    IOLog("elish-touch-hid: startup failed at %s\n", where);
    return false;
}
} // namespace

// Called by the opt-in transport's sleepable thread, after IOKit startup.
bool elish_touch_hid_start()
{
    unsigned expected = NotStarted;
    if (!__atomic_compare_exchange_n(&gStage, &expected, unsigned(Starting), false,
            __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) return __atomic_load_n(&gDevice, __ATOMIC_ACQUIRE) != nullptr;

    const auto family = OSKextLoadKextWithIdentifier("com.apple.iokit.IOHIDFamily");
    store(gFamilyStatus, family);
    if (family != KERN_SUCCESS) return startFailed("IOHIDFamily");
    const auto personality = OSKext::loadKextWithIdentifier("com.apple.iokit.IOHIDEventDriver",
        true, false, kOSKextExcludeNone, kOSKextExcludeNone, nullptr);
    store(gPersonalityStatus, personality);
    store(gStage, unsigned(FamilyLoaded));

    auto matching = IOService::serviceMatching("IOHIDResource");
    auto resource = matching ? IOService::waitForMatchingService(matching, 5000000000ULL) : nullptr;
    OSSafeReleaseNULL(matching);
    if (!resource) return startFailed("IOHIDResource");
    store(gStage, unsigned(ResourceFound));

    auto name = OSSymbol::withCString("IOHIDDevice");
    const auto *meta = name ? OSMetaClass::getMetaClassWithName(name) : nullptr;
    OSSafeReleaseNULL(name);
    const unsigned size = meta ? meta->getClassSize() : 0;
    store(gClassSize, size);
    const char *error = nullptr;
    ElishTouchHid *device = nullptr;
    if (size != sizeof(IOHIDDevice)) {
        error = "IOHIDDevice class size";
    } else if (void *loadHandle = OSMetaClass::preModLoad(g_kernel_kmod_info.name)) {
        elish_touch_hid_init_class();
        const auto status = OSMetaClass::postModLoad(loadHandle);
        store(gClassStatus, status);
        if (status != KERN_SUCCESS) {
            error = "postModLoad";
        } else {
            store(gStage, unsigned(ClassRegistered));
            device = new ElishTouchHid;
            if (!device) error = "allocate";
            else if (!device->init(nullptr) || !device->attach(resource)) error = "init/attach";
            else if (!device->start(resource)) {
                device->detach(resource);
                error = "device start";
            }
        }
    } else {
        error = "preModLoad";
    }
    OSSafeReleaseNULL(resource);
    if (error) {
        OSSafeReleaseNULL(device);
        return startFailed(error);
    }
    // The built-in transport and its HID child live for the duration of boot.
    __atomic_store_n(&gDevice, device, __ATOMIC_RELEASE);
    store(gStage, unsigned(Published));
    IOLog("elish-touch-hid: device published id=%llu personality=%d\n",
        static_cast<unsigned long long>(device->getRegistryEntryID()), personality);
    return true;
}

bool elish_touch_hid_submit(const ElishTouchFrame &frame)
{
    increment(gSubmissions);
    auto *device = __atomic_load_n(&gDevice, __ATOMIC_ACQUIRE);
    return device && device->submit(frame);
}

void elish_touch_hid_snapshot(char *out, size_t size)
{
    snprintf(out, size, "HID stage=%u family=%d personality=%d\n"
             "Class=%d match=%u reports=%u lifts=%u\n"
             "Last=%x edge=%u begins=%u events=%u",
             load(gStage), load(gFamilyStatus), load(gPersonalityStatus), load(gClassStatus), load(gMatched),
             load(gReports), load(gLifts), static_cast<unsigned>(load(gLastReport)), load(gEdgeHook),
             load(gEdgeBegins), load(gEdgeEvents));
}

void elish_touch_hid_log()
{
    IOLog("elish-touch-hid: stage=%u family=%d personality=%d class=%d size=%u match=%u polls=%u "
        "submits=%u unmatched=%u invalid=%u attempts=%u reports=%u lifts=%u last=%#x edge=%u begins=%u events=%u\n",
        load(gStage), load(gFamilyStatus), load(gPersonalityStatus), load(gClassStatus), load(gClassSize),
        load(gMatched), load(gMatchPolls), load(gSubmissions), load(gBeforeMatch), load(gInvalidFrames),
        load(gAttempts), load(gReports), load(gLifts), static_cast<unsigned>(load(gLastReport)),
        load(gEdgeHook), load(gEdgeBegins), load(gEdgeEvents));
}

// SPDX-License-Identifier: BSD-2-Clause-Patent
#pragma once
#include <stdint.h>

struct ElishTouchPoint {
    uint16_t x, y;
    bool active;
};

// Slot i always denotes NT36523 contact identifier i+1. Unlisted IDs lift.
struct ElishTouchFrame {
    ElishTouchPoint points[10];
};

constexpr unsigned kElishTouchWidth = 1600;
constexpr unsigned kElishTouchHeight = 2560;
constexpr unsigned kElishTouchFingerBytes = 65;
constexpr unsigned kElishTouchEventBytes = kElishTouchFingerBytes + 14; // stock elish enables the pen tail

enum class ElishTouchPacketResult { Valid, Recovery, Checksum, Contact };

inline ElishTouchPacketResult elish_decode_touch(const uint8_t (&data)[kElishTouchEventBytes], ElishTouchFrame &out)
{
    bool watchdog = true, esd = true;
    for (unsigned i = 0; i < 6; ++i) {
        watchdog &= data[i] == 0xfd || data[i] == 0xfe;
        esd &= data[i] == 0x77;
    }
    if (watchdog || esd) return ElishTouchPacketResult::Recovery;
    uint8_t sum = 0;
    for (unsigned i = 0; i < kElishTouchFingerBytes; ++i) sum += data[i];
    if (sum) return ElishTouchPacketResult::Checksum;

    ElishTouchFrame frame{};
    for (unsigned i = 0; i < 10; ++i) {
        const uint8_t *slot = data + i * 6;
        const unsigned state = slot[0] & 7;
        if (state != 1 && state != 2) continue;
        const unsigned id = slot[0] >> 3;
        const unsigned x = (slot[1] << 4) | (slot[3] >> 4);
        const unsigned y = (slot[2] << 4) | (slot[3] & 15);
        if (!id || id > 10 || frame.points[id - 1].active || x >= kElishTouchWidth || y >= kElishTouchHeight)
            return ElishTouchPacketResult::Contact;
        frame.points[id - 1] = {static_cast<uint16_t>(x), static_cast<uint16_t>(y), true};
    }
    out = frame;
    return ElishTouchPacketResult::Valid;
}

// SPDX-License-Identifier: BSD-2-Clause-Patent
#include "../package/ElishTouchClock.h"
#include "../package/ElishTouchFrame.h"
#include <cassert>
#include <cstdio>

using Packet = uint8_t[kElishTouchEventBytes];

static void put(Packet &data, unsigned slot, unsigned id, unsigned status, unsigned x, unsigned y)
{
    auto *p = data + slot * 6;
    p[0] = (id << 3) | status;
    p[1] = x >> 4;
    p[2] = y >> 4;
    p[3] = (x & 15) << 4 | (y & 15);
}

static void checksum(Packet &data)
{
    unsigned sum = 0;
    for (unsigned i = 0; i < 64; ++i) sum += data[i];
    data[64] = -sum;
}

int main()
{
    constexpr uint32_t off = 0x80000000;
    constexpr uint32_t hwcg = 2;
    constexpr uint32_t votes = kElishTouchClockVotes | 1;
    assert(elish_touch_clocks_ready(votes, 0, off | hwcg, 0));
    assert(!elish_touch_clocks_ready(votes, 0, off, 0));
    assert(!elish_touch_clocks_ready(votes, off, off | hwcg, 0));
    assert(!elish_touch_clocks_ready(votes, 0, off | hwcg, off));
    assert(!elish_touch_clocks_ready(votes & ~(1u << 14), 0, 0, 0));
    std::puts("PASS: elish Kona QUP0 clock vote, slave HWCG and halt checks");

    Packet down{}, move{}, up{};
    ElishTouchFrame frame{};
    put(down, 0, 10, 1, 1599, 2559);
    put(down, 8, 1, 1, 0, 0);
    checksum(down);
    down[65] = 0xf0;
    down[78] = 0x23; // the pen bytes are outside the finger checksum
    assert(elish_decode_touch(down, frame) == ElishTouchPacketResult::Valid);
    assert(frame.points[0].active && frame.points[9].active);
    assert(frame.points[9].x == 1599 && frame.points[9].y == 2559);
    put(move, 7, 10, 2, 1599, 1600);
    checksum(move);
    assert(elish_decode_touch(move, frame) == ElishTouchPacketResult::Valid);
    assert(!frame.points[0].active && frame.points[9].active && frame.points[9].y == 1600);
    assert(elish_decode_touch(up, frame) == ElishTouchPacketResult::Valid);
    for (const auto &p : frame.points) assert(!p.active);
    move[7 * 6 + 1] ^= 1;
    assert(elish_decode_touch(move, frame) == ElishTouchPacketResult::Checksum);
    put(move, 7, 10, 2, 1600, 1600);
    checksum(move);
    assert(elish_decode_touch(move, frame) == ElishTouchPacketResult::Contact);
    put(down, 1, 10, 1, 10, 10);
    checksum(down);
    assert(elish_decode_touch(down, frame) == ElishTouchPacketResult::Contact);
    for (unsigned i = 0; i < 6; ++i) down[i] = 0xfd + (i & 1);
    assert(elish_decode_touch(down, frame) == ElishTouchPacketResult::Recovery);
    std::puts("PASS: elish NT36523 pen-tail isolation, finger checksum, stable IDs, bounds, movement and release");
}

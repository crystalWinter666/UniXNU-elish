// SPDX-License-Identifier: BSD-2-Clause-Patent
#pragma once
#include <stdint.h>

constexpr uint32_t kElishTouchClockVotes = (1u << 14) | (1u << 6) | (1u << 7);

inline bool elish_touch_clocks_ready(uint32_t votes, uint32_t master, uint32_t slave, uint32_t engine)
{
    constexpr uint32_t off = 1u << 31;
    constexpr uint32_t slaveHardwareGate = 1u << 1;
    // MiCode Kona gcc_qupv3_wrap_0_s_ahb_clk has HWCG at CBCR bit 1.
    // clk_branch_wait skips the halt check for this branch in hardware mode.
    return (votes & kElishTouchClockVotes) == kElishTouchClockVotes &&
           !((master | engine) & off) && (!(slave & off) || (slave & slaveHardwareGate));
}

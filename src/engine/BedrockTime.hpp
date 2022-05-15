#pragma once

#include "chrono"

namespace MFA::Time
{
    std::chrono::steady_clock::time_point Now() {
        return std::chrono::steady_clock::now();
    }

    template <
        class result_t   = std::chrono::milliseconds,
        class clock_t    = std::chrono::steady_clock,
        class duration_t = std::chrono::milliseconds
    >
    auto Since(std::chrono::time_point<clock_t, duration_t> const& start)
    {
        return std::chrono::duration_cast<result_t>(clock_t::now() - start);
    }
    
}
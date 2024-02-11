//
// Date       : 10/02/2024
// Project    : octvis
// Author     : -Ry
//

#include "Utility.h"

#include <chrono>

namespace octvis {

    using Clock = std::chrono::high_resolution_clock;
    using Instant = Clock::time_point;
    using FloatDuration = std::chrono::duration<float>;

    static Instant s_Start;
    static float s_Elapsed;

    void start_timer() noexcept {
        s_Start = Clock::now();
    }

    float elapsed() noexcept {
        s_Elapsed = FloatDuration{ Clock::now() - s_Start }.count();
        return s_Elapsed;
    }

} // octvis
#pragma once

#include <cstdint>
#include <functional>

namespace MFA::JobSystem {
    using ThreadNumber = uint32_t;
    using Task = std::function<void(ThreadNumber threadNumber, ThreadNumber totalThreadCount)>;
    using OnFinishCallback = std::function<void()>;
}

namespace MFA
{
    namespace JS = JobSystem;
}

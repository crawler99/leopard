#ifndef LEOPARD_UTILS_REACTOR_HXX_
#define LEOPARD_UTILS_REACTOR_HXX_

#include "FdAggregator.hxx"

#include <sys/epoll.h>
#include <atomic>

namespace leopard { namespace utils {

template <typename... Aggregators>
class Reactor : private NonCopyable, public Aggregators...
{
public:
    void Run()
    {
        while (!_stop.load(std::memory_order::memory_order_acquire))
        {
            (static_cast<void>(Aggregators::CollectEvents()), ...);
        }
    }

    void Stop()
    {
        _stop.store(true, std::memory_order_release);
    }

private:
    std::atomic<bool> _stop{false};
};

}} // namespace utils::leopard

#endif // LEOPARD_UTILS_REACTOR_HXX_
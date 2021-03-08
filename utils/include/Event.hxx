#ifndef LEOPARD_UTILS_EVENT_HXX_
#define LEOPARD_UTILS_EVENT_HXX_

#include "NonCopyable.hxx"
#include <mutex>

namespace leopard { namespace utils {

//
// - Thread safe.
// - Attach observers with arbitrary arguments (perfect-forwarding).
//
template <typename Func>
class Event : private NonCopyable
{
public:
    template <typename F>
    uint32_t Connect(F&& f)
    {
        std::lock_guard<std::mutex> guard(_mutex);
        uint32_t k = _observer_id++;
        _connections.emplace(k, std::forward<F>(f));
        return k;
    }

    void Disconnect(uint32_t key)
    {
        std::lock_guard<std::mutex> guard(_mutex);
        _connections.erase(key);
    }

    template <typename... Args>
    void Notify(Args&&... args)
    {
        std::lock_guard<std::mutex> guard(_mutex);
        for (auto &it : _connections)
        {
            it.second(std::forward<Args>(args)...);
        }
    }

private:
    std::mutex _mutex;
    uint32_t _observer_id = { 0 };
    std::map<uint32_t, Func> _connections;
};

}} // namespace utils::leopard

#endif // LEOPARD_UTILS_EVENT_HXX_

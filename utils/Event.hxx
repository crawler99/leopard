#ifndef LEOPARD_UTILS_EVENT_HXX_
#define LEOPARD_UTILS_EVENT_HXX_

#include "NonCopyable.hxx"

namespace leopard { namespace utils {

//
// - Thread safe.
// - Create instance with arbitrary arguments (perfect-forwarding).
//
template <typename Func>
class Event : private NonCopyable
{
public:
    uint32_t Connect(Func&& f)
    {
        return Assign(f);
    }

    uint32_t Connect(const Func &f)
    {
        return Assign(f);
    }

    void Disconnect(uint32_t key)
    {
        _connections.erase(key);
    }

    template <typename... Args>
    void Notify(Args&&... args)
    {
        for (auto &it : _connections)
        {
            it.second(std::forward<Args>(args)...);
        }
    }

private:
    template <typename F>
    uint32_t Assign(F&& f)
    {
        uint32_t k = _observer_id++;
        _connections.emplace(k, std::forward<F>(f));
        return k;
    }

    uint32_t _observer_id = { 0 };
    std::map<uint32_t, Func> _connections;
};

} // namespace utils
} // namespace leopard

#endif
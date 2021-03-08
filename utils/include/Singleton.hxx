#ifndef LEOPARD_UTILS_SINGLETON_HXX_
#define LEOPARD_UTILS_SINGLETON_HXX_

#include "NonCopyable.hxx"
#include <mutex>

namespace leopard { namespace utils {

//
// - Thread safe.
// - Create instance with arbitrary arguments (perfect-forwarding).
//
template <typename T>
class Singleton : private NonCopyable
{
public:
    template <typename... Args>
    static T* Instance(Args&&... args)
    {
        std::lock_guard<std::mutex> guard(_mutex);
        if (_instance)
        {
            throw std::logic_error("Instance has already been created, use get_instance() to access it.");
        }
        _instance = new T(std::forward<Args>(args)...);
        return _instance;
    }

    static T* GetInstance()
    {
        if (!_instance)
        {
            throw std::logic_error("Instance has not been initialized.");
        }
        return _instance;
    } 

private:
    static std::mutex _mutex;
    static T* _instance;
};

template <typename T>
std::mutex Singleton<T>::_mutex;

template <typename T>
T* Singleton<T>::_instance = { nullptr };

} // namespace utils
} // namespace leopard

#endif
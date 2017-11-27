#ifndef _SINGLETON_HXX_
#define _SINGLETON_HXX_

#include "NonCopyable.hxx"
#include <mutex>

namespace utils {

template <typename T>
class Singleton : private NonCopyable
{
public:
    template <typename... Args>
    static T* instance(Args&&... args)
    {
        std::lock_guard<std::mutex> guard(_mutex);
        if (_instance)
        {
            throw std::logic_error("Instance has already been created, use get_instance() to access it.");
        }
        _instance = new T(std::forward<Args>(args)...);
        return _instance;
    }

    static T* get_instance()
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

}

#endif
#ifndef LEOPARD_UTILS_THREADRAII_HXX_
#define LEOPARD_UTILS_THREADRAII_HXX_

#include <thread>

namespace leopard { namespace utils {

//
// - NOT thread safe. It means user should guarantee that there's no race condtion 
//   calling the thread object passed in as well as calling this wrapper object.
//
class ThreadRAII
{
public:
    enum class DtorAction
    {
        join, detach
    };

    ThreadRAII(std::thread &&t, DtorAction a)
        : action(a), t(std::move(t)) // Init the thread member last so that it runs with full preparation.
    {
    }

    ~ThreadRAII()
    {
        if (t.joinable()) // Invoking join or detach on an unjoinable thread yields undefined behaviour.
        {
            if (DtorAction::join == action)
            {
                t.join();
            }
            else
            {
                t.detach();
            }
        }
    }

    // For a class which declares an explicit destructor, there will be no default move operations generated
    // by compiler. Therefore we should declare them manually.
    ThreadRAII(ThreadRAII&&)              = default;
    ThreadRAII& operator = (ThreadRAII&&) = default;

    std::thread& get() { return t; }

private:
    DtorAction action;
    std::thread t;
};

} // namespace utils
} // namespace leopard

#endif
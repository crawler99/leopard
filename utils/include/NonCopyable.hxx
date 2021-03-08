#ifndef LEOPARD_UTILS_NONCOPYABLE_HXX_
#define LEOPARD_UTILS_NONCOPYABLE_HXX_

namespace leopard { namespace utils {

class NonCopyable 
{
public:
    NonCopyable() = default;
    ~NonCopyable() = default;

    // Disable copy-constructing
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator = (const NonCopyable&) = delete;
};

} // namespace utils
} // namespace leopard

#endif
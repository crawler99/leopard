#ifndef _NONCOPYABLE_HXX_
#define _NONCOPYABLE_HXX_

namespace utils {

class NonCopyable 
{
public:
    NonCopyable() = default;
    ~NonCopyable() = default;

    // Disable copy-constructing
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator = (const NonCopyable&) = delete;
};

}

#endif
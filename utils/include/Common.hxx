#ifndef LEOPARD_UTILS_COMMON_HXX_
#define LEOPARD_UTILS_COMMON_HXX_

#ifdef __GNUC__
    #ifdef NDEBUG
        #define ALWAYS_INLINE __attribute__((always_inline))
    #else
        #define ALWAYS_INLINE // otherwise difficult to debug
    #endif
    #define LIKELY(x)   __builtin_expect(!!(x),1)
    #define UNLIKELY(x) __builtin_expect(!!(x),0)
#else
    #define FORCE_INLINE
    #define FORCE_NOINLINE
    #define LIKELY(x)   (x)
    #define UNLIKELY(x) (x)
#endif

#endif // LEOPARD_UTILS_COMMON_HXX_
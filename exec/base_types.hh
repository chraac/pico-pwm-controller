#pragma once

#include <cstdint>

#ifndef DISALLOW_COPY
#define DISALLOW_COPY(clz)     \
    clz(const clz &) = delete; \
    void operator=(const clz &) = delete

#define DISALLOW_MOVE(clz) \
    clz(clz &&) = delete;  \
    void operator=(clz &&) = delete
#endif

namespace utility {
template <typename _Ty>
class Singleton {
   public:
    typedef _Ty TypeName;

    static TypeName &GetInstance() {
        static TypeName s_instance;
        return s_instance;
    }

   protected:
    Singleton() = default;

   private:
    DISALLOW_COPY(Singleton);
    DISALLOW_MOVE(Singleton);
};
}  // namespace utility

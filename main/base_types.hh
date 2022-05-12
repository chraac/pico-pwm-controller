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
        static TypeName instance;
        return instance;
    }

protected:
    Singleton() = default;

private:
    DISALLOW_COPY(Singleton);
    DISALLOW_MOVE(Singleton);
};

class IGpioBase {
public:
    virtual ~IGpioBase() {}

    virtual uint32_t GetGpioPin() const noexcept = 0;
};

class GpioBase : public IGpioBase {
public:
    GpioBase(uint32_t gpio_pin) : gpio_pin_(gpio_pin) {}

    uint32_t GetGpioPin() const noexcept override { return gpio_pin_; }

protected:
    uint32_t gpio_pin_;
};

}  // namespace utility

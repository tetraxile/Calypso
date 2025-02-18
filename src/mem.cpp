#include "hk/diag/diag.h"
#include <cstddef>
#include <new>

extern "C" {

void* hk_Znwm(std::size_t size);
void* hk_ZnwmRKSt9nothrow_t(std::size_t size, const std::nothrow_t&);
void* hk_Znam(std::size_t size);
void* hk_ZnamRKSt9nothrow_t(std::size_t size, const std::nothrow_t&);
void hk_ZdlPv(void* ptr);
void hk_ZdlPvRKSt9nothrow_t(void* ptr, const std::nothrow_t&);
void hk_ZdlPvm(void* ptr, std::size_t size);
void hk_ZdaPv(void* ptr);
void hk_ZdaPvRKSt9nothrow_t(void* ptr, const std::nothrow_t&);
void hk_ZdaPvm(void* ptr, std::size_t size);
}

constexpr bool cAllowAllocations = true;

hk_alwaysinline void checkAllocAllowed() {
    if constexpr (cAllowAllocations == false)
        HK_ABORT("Tried to allocate when cAllowAllocations = %d", cAllowAllocations);
}

void* operator new(std::size_t size) {
    checkAllocAllowed();
    return hk_Znwm(size);
}

void* operator new(std::size_t size, const std::nothrow_t& t) noexcept {
    checkAllocAllowed();
    return hk_ZnwmRKSt9nothrow_t(size, t);
}

void* operator new[](std::size_t size) {
    checkAllocAllowed();
    return hk_Znam(size);
}

void* operator new[](std::size_t size, const std::nothrow_t& t) noexcept {
    checkAllocAllowed();
    return hk_ZnamRKSt9nothrow_t(size, t);
}

void operator delete(void* ptr) {
    checkAllocAllowed();
    return hk_ZdlPv(ptr);
}

void operator delete(void* ptr, const std::nothrow_t& t) {
    checkAllocAllowed();
    return hk_ZdlPvRKSt9nothrow_t(ptr, t);
}

void operator delete(void* ptr, std::size_t size) {
    checkAllocAllowed();
    return hk_ZdlPvm(ptr, size);
}

void operator delete[](void* ptr) {
    checkAllocAllowed();
    return hk_ZdaPv(ptr);
}

void operator delete[](void* ptr, const std::nothrow_t& t) {
    checkAllocAllowed();
    return hk_ZdaPvRKSt9nothrow_t(ptr, t);
}

void operator delete[](void* ptr, std::size_t size) {
    checkAllocAllowed();
    return hk_ZdaPvm(ptr, size);
}

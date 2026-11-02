#include "gba_types.hpp"

// Standard ARM EABI integer division and modulo functions

extern "C" {

u32 __aeabi_uidiv(u32 num, u32 den) {
    if (den == 0) return 0;
    u32 quot = 0;
    u32 rem = 0;
    for (int i = 31; i >= 0; --i) {
        rem = (rem << 1) | ((num >> i) & 1);
        if (rem >= den) {
            rem -= den;
            quot |= (1U << i);
        }
    }
    return quot;
}

struct uidivmod_res {
    u32 quot;
    u32 rem;
};

uidivmod_res __aeabi_uidivmod(u32 num, u32 den) {
    uidivmod_res res = { 0, 0 };
    if (den == 0) return res;
    for (int i = 31; i >= 0; --i) {
        res.rem = (res.rem << 1) | ((num >> i) & 1);
        if (res.rem >= den) {
            res.rem -= den;
            res.quot |= (1U << i);
        }
    }
    return res;
}

s32 __aeabi_idiv(s32 num, s32 den) {
    if (den == 0) return 0;
    int sign = 0;
    u32 unum = num;
    u32 uden = den;
    if (num < 0) { sign ^= 1; unum = (u32)(-num); }
    if (den < 0) { sign ^= 1; uden = (u32)(-den); }

    u32 q = __aeabi_uidiv(unum, uden);
    return sign ? -(s32)q : (s32)q;
}

struct idivmod_res {
    s32 quot;
    s32 rem;
};

idivmod_res __aeabi_idivmod(s32 num, s32 den) {
    idivmod_res res = { 0, 0 };
    if (den == 0) return res;
    int sign_q = 0;
    int sign_r = 0;
    u32 unum = num;
    u32 uden = den;
    if (num < 0) { sign_q ^= 1; sign_r = 1; unum = (u32)(-num); }
    if (den < 0) { sign_q ^= 1; uden = (u32)(-den); }

    uidivmod_res ures = __aeabi_uidivmod(unum, uden);
    res.quot = sign_q ? -(s32)ures.quot : (s32)ures.quot;
    res.rem  = sign_r ? -(s32)ures.rem  : (s32)ures.rem;
    return res;
}

struct ldivmod_res {
    s64 quot;
    s64 rem;
};

ldivmod_res __aeabi_ldivmod(s64 num, s64 den) {
    ldivmod_res res = { 0, 0 };
    if (den == 0) return res;

    int sign_q = 0;
    int sign_r = 0;
    u64 unum = num;
    u64 uden = den;
    if (num < 0) { sign_q ^= 1; sign_r = 1; unum = (u64)(-num); }
    if (den < 0) { sign_q ^= 1; uden = (u64)(-den); }

    u64 quot = 0;
    u64 rem = 0;
    for (int i = 63; i >= 0; --i) {
        rem = (rem << 1) | ((unum >> i) & 1);
        if (rem >= uden) {
            rem -= uden;
            quot |= (1ULL << i);
        }
    }

    res.quot = sign_q ? -(s64)quot : (s64)quot;
    res.rem  = sign_r ? -(s64)rem  : (s64)rem;
    return res;
}

// Memory functions required by compiler
void* memset(void* dest, int c, size_t n) {
    u8* d = (u8*)dest;
    for (size_t i = 0; i < n; ++i) d[i] = (u8)c;
    return dest;
}

void* memcpy(void* dest, const void* src, size_t n) {
    u8* d = (u8*)dest;
    const u8* s = (const u8*)src;
    for (size_t i = 0; i < n; ++i) d[i] = s[i];
    return dest;
}

} // extern "C"

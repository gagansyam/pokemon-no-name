#pragma once

#include "gba_types.hpp"

// 16.16 Fixed-Point representation
struct Fixed {
    s32 val;

    constexpr Fixed() : val(0) {}
    constexpr Fixed(s32 v, bool /*raw*/) : val(v) {}
    constexpr Fixed(int i) : val(i << 16) {}

    static constexpr Fixed from_raw(s32 raw) {
        return Fixed(raw, true);
    }

    static constexpr Fixed from_fraction(int num, int den) {
        return Fixed((s32)(((s64)num << 16) / den), true);
    }

    int to_int() const {
        return val >> 16;
    }

    Fixed operator+(Fixed o) const { return Fixed(val + o.val, true); }
    Fixed operator-(Fixed o) const { return Fixed(val - o.val, true); }
    Fixed operator*(Fixed o) const {
        return Fixed((s32)(((s64)val * o.val) >> 16), true);
    }
    Fixed operator/(Fixed o) const {
        if (o.val == 0) return Fixed(0, true);
        return Fixed((s32)(((s64)val << 16) / o.val), true);
    }

    Fixed& operator+=(Fixed o) { val += o.val; return *this; }
    Fixed& operator-=(Fixed o) { val -= o.val; return *this; }
    Fixed& operator*=(Fixed o) {
        val = (s32)(((s64)val * o.val) >> 16);
        return *this;
    }
    Fixed& operator/=(Fixed o) {
        if (o.val != 0) val = (s32)(((s64)val << 16) / o.val);
        return *this;
    }

    Fixed operator-() const { return Fixed(-val, true); }

    bool operator<(Fixed o) const { return val < o.val; }
    bool operator>(Fixed o) const { return val > o.val; }
    bool operator<=(Fixed o) const { return val <= o.val; }
    bool operator>=(Fixed o) const { return val >= o.val; }
    bool operator==(Fixed o) const { return val == o.val; }
    bool operator!=(Fixed o) const { return val != o.val; }
};

inline Fixed fx_abs(Fixed f) {
    return f.val < 0 ? -f : f;
}

// Fast integer square root for distance checks
inline u32 isqrt(u32 val) {
    u32 temp, g = 0;
    u32 b = 0x8000;
    u32 bshft = 15;
    do {
        if (val >= (temp = (((g << 1) + b) << bshft--))) {
            g += b;
            val -= temp;
        }
    } while (b >>= 1);
    return g;
}

// Distance approximation without float
inline Fixed fx_dist(Fixed dx, Fixed dy) {
    s32 x = fx_abs(dx).to_int();
    s32 y = fx_abs(dy).to_int();
    u32 sum = (u32)(x * x + y * y);
    return Fixed((int)isqrt(sum));
}

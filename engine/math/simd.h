#pragma once

#include "core/types.h"
#include "math/vec4.h"
#include "math/mat4.h"

#ifdef __SSE__
#include <xmmintrin.h> // SSE
#include <emmintrin.h>  // SSE2
#endif

namespace kairo {
namespace simd {

#ifdef __SSE__

// --- Load / Store helpers ---
// Convert between our math types and SSE registers.
// Mat4 columns are contiguous floats so we can load them directly.

inline __m128 load_vec4(const Vec4& v) {
    return _mm_set_ps(v.w, v.z, v.y, v.x);
}

inline Vec4 store_vec4(__m128 r) {
    alignas(16) float f[4];
    _mm_store_ps(f, r);
    return { f[0], f[1], f[2], f[3] };
}

// Load a column from a Mat4. m[col] is 4 contiguous floats (row 0..3).
inline __m128 load_mat4_col(const Mat4& mat, int col) {
    return _mm_loadu_ps(&mat.m[col][0]);
}

// --- Vec4 arithmetic ---

inline Vec4 add(const Vec4& a, const Vec4& b) {
    return store_vec4(_mm_add_ps(load_vec4(a), load_vec4(b)));
}

inline Vec4 sub(const Vec4& a, const Vec4& b) {
    return store_vec4(_mm_sub_ps(load_vec4(a), load_vec4(b)));
}

inline Vec4 mul(const Vec4& a, float s) {
    return store_vec4(_mm_mul_ps(load_vec4(a), _mm_set1_ps(s)));
}

// Dot product using SSE2-compatible horizontal sum (no _mm_hadd_ps needed).
// Strategy: multiply, then reduce with shuffle+add pairs.
inline float dot(const Vec4& a, const Vec4& b) {
    __m128 t = _mm_mul_ps(load_vec4(a), load_vec4(b));
    // [x*x, y*y, z*z, w*w] -> swap adjacent pairs and add
    __m128 shuf = _mm_shuffle_ps(t, t, _MM_SHUFFLE(2, 3, 0, 1));
    __m128 sums = _mm_add_ps(t, shuf);
    // now sums = [x+y, x+y, z+w, z+w] — move high half down and add
    shuf = _mm_movehl_ps(shuf, sums);
    sums = _mm_add_ss(sums, shuf);
    return _mm_cvtss_f32(sums);
}

// --- Mat4 * Mat4 ---
// For each result column: broadcast each element of the rhs column across
// a register, multiply by the corresponding lhs column, and sum the four
// products. This replaces the scalar 4x4x4 loop with 16 muls + 12 adds.
inline Mat4 mat4_multiply(const Mat4& a, const Mat4& b) {
    // Pre-load all columns of A
    __m128 a0 = load_mat4_col(a, 0);
    __m128 a1 = load_mat4_col(a, 1);
    __m128 a2 = load_mat4_col(a, 2);
    __m128 a3 = load_mat4_col(a, 3);

    Mat4 result;
    for (int col = 0; col < 4; ++col) {
        __m128 bc = load_mat4_col(b, col);

        // Broadcast each element of b's column
        __m128 b0 = _mm_shuffle_ps(bc, bc, _MM_SHUFFLE(0, 0, 0, 0));
        __m128 b1 = _mm_shuffle_ps(bc, bc, _MM_SHUFFLE(1, 1, 1, 1));
        __m128 b2 = _mm_shuffle_ps(bc, bc, _MM_SHUFFLE(2, 2, 2, 2));
        __m128 b3 = _mm_shuffle_ps(bc, bc, _MM_SHUFFLE(3, 3, 3, 3));

        // result_col = a0*b[0] + a1*b[1] + a2*b[2] + a3*b[3]
        __m128 r = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(a0, b0), _mm_mul_ps(a1, b1)),
            _mm_add_ps(_mm_mul_ps(a2, b2), _mm_mul_ps(a3, b3))
        );

        _mm_storeu_ps(&result.m[col][0], r);
    }
    return result;
}

// --- Mat4 * Vec4 ---
// Broadcast each component of v, multiply by the corresponding matrix
// column, and sum. Equivalent to: m.col[0]*v.x + m.col[1]*v.y + ...
inline Vec4 mat4_transform(const Mat4& m, const Vec4& v) {
    __m128 vr = load_vec4(v);

    __m128 vx = _mm_shuffle_ps(vr, vr, _MM_SHUFFLE(0, 0, 0, 0));
    __m128 vy = _mm_shuffle_ps(vr, vr, _MM_SHUFFLE(1, 1, 1, 1));
    __m128 vz = _mm_shuffle_ps(vr, vr, _MM_SHUFFLE(2, 2, 2, 2));
    __m128 vw = _mm_shuffle_ps(vr, vr, _MM_SHUFFLE(3, 3, 3, 3));

    __m128 r = _mm_add_ps(
        _mm_add_ps(_mm_mul_ps(load_mat4_col(m, 0), vx),
                   _mm_mul_ps(load_mat4_col(m, 1), vy)),
        _mm_add_ps(_mm_mul_ps(load_mat4_col(m, 2), vz),
                   _mm_mul_ps(load_mat4_col(m, 3), vw))
    );

    return store_vec4(r);
}

// --- Batch transform ---
// Apply the same matrix to an array of vec4s. Pre-loads the matrix columns
// once, then processes each vector with 4 broadcast-multiply-add sequences.
inline void mat4_transform_batch(const Mat4& m, const Vec4* in, Vec4* out, size_t count) {
    __m128 c0 = load_mat4_col(m, 0);
    __m128 c1 = load_mat4_col(m, 1);
    __m128 c2 = load_mat4_col(m, 2);
    __m128 c3 = load_mat4_col(m, 3);

    for (size_t i = 0; i < count; ++i) {
        __m128 v = load_vec4(in[i]);

        __m128 vx = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
        __m128 vy = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
        __m128 vz = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));
        __m128 vw = _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 3, 3, 3));

        __m128 r = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(c0, vx), _mm_mul_ps(c1, vy)),
            _mm_add_ps(_mm_mul_ps(c2, vz), _mm_mul_ps(c3, vw))
        );

        out[i] = store_vec4(r);
    }
}

#else

// --- Scalar fallbacks for non-SSE platforms ---

inline Vec4 add(const Vec4& a, const Vec4& b) { return a + b; }
inline Vec4 sub(const Vec4& a, const Vec4& b) { return a - b; }
inline Vec4 mul(const Vec4& a, float s)        { return a * s; }
inline float dot(const Vec4& a, const Vec4& b) { return a.dot(b); }

inline Mat4 mat4_multiply(const Mat4& a, const Mat4& b) { return a * b; }

inline Vec4 mat4_transform(const Mat4& m, const Vec4& v) {
    return {
        m.m[0][0]*v.x + m.m[1][0]*v.y + m.m[2][0]*v.z + m.m[3][0]*v.w,
        m.m[0][1]*v.x + m.m[1][1]*v.y + m.m[2][1]*v.z + m.m[3][1]*v.w,
        m.m[0][2]*v.x + m.m[1][2]*v.y + m.m[2][2]*v.z + m.m[3][2]*v.w,
        m.m[0][3]*v.x + m.m[1][3]*v.y + m.m[2][3]*v.z + m.m[3][3]*v.w
    };
}

inline void mat4_transform_batch(const Mat4& m, const Vec4* in, Vec4* out, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        out[i] = mat4_transform(m, in[i]);
    }
}

#endif

} // namespace simd
} // namespace kairo

#include "test_framework.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include "math/mat4.h"
#include "math/math_utils.h"

using namespace kairo;

// ============================================================
// Vec2 tests
// ============================================================

TEST(vec2_default_construction) {
    Vec2 v;
    ASSERT_NEAR(v.x, 0.0f, 1e-6f);
    ASSERT_NEAR(v.y, 0.0f, 1e-6f);
}

TEST(vec2_value_construction) {
    Vec2 v(3.0f, 4.0f);
    ASSERT_NEAR(v.x, 3.0f, 1e-6f);
    ASSERT_NEAR(v.y, 4.0f, 1e-6f);
}

TEST(vec2_scalar_construction) {
    Vec2 v(5.0f);
    ASSERT_NEAR(v.x, 5.0f, 1e-6f);
    ASSERT_NEAR(v.y, 5.0f, 1e-6f);
}

TEST(vec2_addition) {
    Vec2 a(1.0f, 2.0f);
    Vec2 b(3.0f, 4.0f);
    Vec2 c = a + b;
    ASSERT_NEAR(c.x, 4.0f, 1e-6f);
    ASSERT_NEAR(c.y, 6.0f, 1e-6f);
}

TEST(vec2_subtraction) {
    Vec2 a(5.0f, 7.0f);
    Vec2 b(2.0f, 3.0f);
    Vec2 c = a - b;
    ASSERT_NEAR(c.x, 3.0f, 1e-6f);
    ASSERT_NEAR(c.y, 4.0f, 1e-6f);
}

TEST(vec2_scalar_multiply) {
    Vec2 a(2.0f, 3.0f);
    Vec2 b = a * 4.0f;
    ASSERT_NEAR(b.x, 8.0f, 1e-6f);
    ASSERT_NEAR(b.y, 12.0f, 1e-6f);

    // commutative: scalar * vec
    Vec2 c = 4.0f * a;
    ASSERT_NEAR(c.x, 8.0f, 1e-6f);
    ASSERT_NEAR(c.y, 12.0f, 1e-6f);
}

TEST(vec2_scalar_divide) {
    Vec2 a(10.0f, 20.0f);
    Vec2 b = a / 2.0f;
    ASSERT_NEAR(b.x, 5.0f, 1e-6f);
    ASSERT_NEAR(b.y, 10.0f, 1e-6f);
}

TEST(vec2_compound_add) {
    Vec2 a(1.0f, 2.0f);
    a += Vec2(3.0f, 4.0f);
    ASSERT_NEAR(a.x, 4.0f, 1e-6f);
    ASSERT_NEAR(a.y, 6.0f, 1e-6f);
}

TEST(vec2_compound_sub) {
    Vec2 a(5.0f, 7.0f);
    a -= Vec2(2.0f, 3.0f);
    ASSERT_NEAR(a.x, 3.0f, 1e-6f);
    ASSERT_NEAR(a.y, 4.0f, 1e-6f);
}

TEST(vec2_compound_mul) {
    Vec2 a(3.0f, 4.0f);
    a *= 2.0f;
    ASSERT_NEAR(a.x, 6.0f, 1e-6f);
    ASSERT_NEAR(a.y, 8.0f, 1e-6f);
}

TEST(vec2_negation) {
    Vec2 a(3.0f, -4.0f);
    Vec2 b = -a;
    ASSERT_NEAR(b.x, -3.0f, 1e-6f);
    ASSERT_NEAR(b.y, 4.0f, 1e-6f);
}

TEST(vec2_length) {
    Vec2 a(3.0f, 4.0f);
    ASSERT_NEAR(a.length(), 5.0f, 1e-6f);
}

TEST(vec2_length_sq) {
    Vec2 a(3.0f, 4.0f);
    ASSERT_NEAR(a.length_sq(), 25.0f, 1e-6f);
}

TEST(vec2_normalized) {
    Vec2 a(3.0f, 4.0f);
    Vec2 n = a.normalized();
    ASSERT_NEAR(n.length(), 1.0f, 1e-5f);
    ASSERT_NEAR(n.x, 0.6f, 1e-5f);
    ASSERT_NEAR(n.y, 0.8f, 1e-5f);
}

TEST(vec2_normalized_zero) {
    Vec2 z;
    Vec2 n = z.normalized();
    ASSERT_NEAR(n.x, 0.0f, 1e-6f);
    ASSERT_NEAR(n.y, 0.0f, 1e-6f);
}

TEST(vec2_dot) {
    Vec2 a(1.0f, 0.0f);
    Vec2 b(0.0f, 1.0f);
    ASSERT_NEAR(a.dot(b), 0.0f, 1e-6f);

    Vec2 c(2.0f, 3.0f);
    Vec2 d(4.0f, 5.0f);
    ASSERT_NEAR(c.dot(d), 23.0f, 1e-6f);
}

TEST(vec2_perp) {
    Vec2 a(1.0f, 0.0f);
    Vec2 p = a.perp();
    ASSERT_NEAR(p.x, 0.0f, 1e-6f);
    ASSERT_NEAR(p.y, 1.0f, 1e-6f);

    // perpendicular should be orthogonal
    ASSERT_NEAR(a.dot(p), 0.0f, 1e-6f);

    Vec2 b(3.0f, 7.0f);
    Vec2 q = b.perp();
    ASSERT_NEAR(b.dot(q), 0.0f, 1e-5f);
}

// ============================================================
// Vec3 tests
// ============================================================

TEST(vec3_default_construction) {
    Vec3 v;
    ASSERT_NEAR(v.x, 0.0f, 1e-6f);
    ASSERT_NEAR(v.y, 0.0f, 1e-6f);
    ASSERT_NEAR(v.z, 0.0f, 1e-6f);
}

TEST(vec3_value_construction) {
    Vec3 v(1.0f, 2.0f, 3.0f);
    ASSERT_NEAR(v.x, 1.0f, 1e-6f);
    ASSERT_NEAR(v.y, 2.0f, 1e-6f);
    ASSERT_NEAR(v.z, 3.0f, 1e-6f);
}

TEST(vec3_scalar_construction) {
    Vec3 v(7.0f);
    ASSERT_NEAR(v.x, 7.0f, 1e-6f);
    ASSERT_NEAR(v.y, 7.0f, 1e-6f);
    ASSERT_NEAR(v.z, 7.0f, 1e-6f);
}

TEST(vec3_addition) {
    Vec3 c = Vec3(1.0f, 2.0f, 3.0f) + Vec3(4.0f, 5.0f, 6.0f);
    ASSERT_NEAR(c.x, 5.0f, 1e-6f);
    ASSERT_NEAR(c.y, 7.0f, 1e-6f);
    ASSERT_NEAR(c.z, 9.0f, 1e-6f);
}

TEST(vec3_subtraction) {
    Vec3 c = Vec3(5.0f, 7.0f, 9.0f) - Vec3(1.0f, 2.0f, 3.0f);
    ASSERT_NEAR(c.x, 4.0f, 1e-6f);
    ASSERT_NEAR(c.y, 5.0f, 1e-6f);
    ASSERT_NEAR(c.z, 6.0f, 1e-6f);
}

TEST(vec3_scalar_multiply) {
    Vec3 a(1.0f, 2.0f, 3.0f);
    Vec3 b = a * 3.0f;
    ASSERT_NEAR(b.x, 3.0f, 1e-6f);
    ASSERT_NEAR(b.y, 6.0f, 1e-6f);
    ASSERT_NEAR(b.z, 9.0f, 1e-6f);

    Vec3 c = 3.0f * a;
    ASSERT_NEAR(c.x, 3.0f, 1e-6f);
}

TEST(vec3_scalar_divide) {
    Vec3 a(6.0f, 9.0f, 12.0f);
    Vec3 b = a / 3.0f;
    ASSERT_NEAR(b.x, 2.0f, 1e-6f);
    ASSERT_NEAR(b.y, 3.0f, 1e-6f);
    ASSERT_NEAR(b.z, 4.0f, 1e-6f);
}

TEST(vec3_negation) {
    Vec3 a(1.0f, -2.0f, 3.0f);
    Vec3 b = -a;
    ASSERT_NEAR(b.x, -1.0f, 1e-6f);
    ASSERT_NEAR(b.y, 2.0f, 1e-6f);
    ASSERT_NEAR(b.z, -3.0f, 1e-6f);
}

TEST(vec3_compound_assignment) {
    Vec3 a(1.0f, 2.0f, 3.0f);
    a += Vec3(1.0f, 1.0f, 1.0f);
    ASSERT_NEAR(a.x, 2.0f, 1e-6f);

    a -= Vec3(0.5f, 0.5f, 0.5f);
    ASSERT_NEAR(a.y, 2.5f, 1e-6f);

    a *= 2.0f;
    ASSERT_NEAR(a.z, 7.0f, 1e-6f);
}

TEST(vec3_length) {
    Vec3 a(2.0f, 3.0f, 6.0f);
    ASSERT_NEAR(a.length(), 7.0f, 1e-5f);
    ASSERT_NEAR(a.length_sq(), 49.0f, 1e-5f);
}

TEST(vec3_normalized) {
    Vec3 a(0.0f, 0.0f, 5.0f);
    Vec3 n = a.normalized();
    ASSERT_NEAR(n.x, 0.0f, 1e-6f);
    ASSERT_NEAR(n.y, 0.0f, 1e-6f);
    ASSERT_NEAR(n.z, 1.0f, 1e-6f);
}

TEST(vec3_normalized_zero) {
    Vec3 z;
    Vec3 n = z.normalized();
    ASSERT_NEAR(n.length(), 0.0f, 1e-6f);
}

TEST(vec3_dot) {
    Vec3 a(1.0f, 0.0f, 0.0f);
    Vec3 b(0.0f, 1.0f, 0.0f);
    ASSERT_NEAR(a.dot(b), 0.0f, 1e-6f);

    Vec3 c(1.0f, 2.0f, 3.0f);
    Vec3 d(4.0f, 5.0f, 6.0f);
    ASSERT_NEAR(c.dot(d), 32.0f, 1e-6f);
}

TEST(vec3_cross_basis_vectors) {
    Vec3 i(1, 0, 0), j(0, 1, 0), k(0, 0, 1);

    // i x j = k
    Vec3 ixj = i.cross(j);
    ASSERT_NEAR(ixj.x, 0.0f, 1e-6f);
    ASSERT_NEAR(ixj.y, 0.0f, 1e-6f);
    ASSERT_NEAR(ixj.z, 1.0f, 1e-6f);

    // j x k = i
    Vec3 jxk = j.cross(k);
    ASSERT_NEAR(jxk.x, 1.0f, 1e-6f);
    ASSERT_NEAR(jxk.y, 0.0f, 1e-6f);
    ASSERT_NEAR(jxk.z, 0.0f, 1e-6f);

    // k x i = j
    Vec3 kxi = k.cross(i);
    ASSERT_NEAR(kxi.x, 0.0f, 1e-6f);
    ASSERT_NEAR(kxi.y, 1.0f, 1e-6f);
    ASSERT_NEAR(kxi.z, 0.0f, 1e-6f);
}

TEST(vec3_cross_anticommutative) {
    Vec3 a(1, 2, 3), b(4, 5, 6);
    Vec3 axb = a.cross(b);
    Vec3 bxa = b.cross(a);
    ASSERT_NEAR(axb.x, -bxa.x, 1e-6f);
    ASSERT_NEAR(axb.y, -bxa.y, 1e-6f);
    ASSERT_NEAR(axb.z, -bxa.z, 1e-6f);
}

TEST(vec3_cross_self_is_zero) {
    Vec3 a(3, 7, -2);
    Vec3 c = a.cross(a);
    ASSERT_NEAR(c.x, 0.0f, 1e-5f);
    ASSERT_NEAR(c.y, 0.0f, 1e-5f);
    ASSERT_NEAR(c.z, 0.0f, 1e-5f);
}

TEST(vec3_cross_perpendicular) {
    Vec3 a(1, 2, 3), b(4, 5, 6);
    Vec3 c = a.cross(b);
    ASSERT_NEAR(a.dot(c), 0.0f, 1e-4f);
    ASSERT_NEAR(b.dot(c), 0.0f, 1e-4f);
}

// ============================================================
// Vec4 tests
// ============================================================

TEST(vec4_default_construction) {
    Vec4 v;
    ASSERT_NEAR(v.x, 0.0f, 1e-6f);
    ASSERT_NEAR(v.y, 0.0f, 1e-6f);
    ASSERT_NEAR(v.z, 0.0f, 1e-6f);
    ASSERT_NEAR(v.w, 0.0f, 1e-6f);
}

TEST(vec4_value_construction) {
    Vec4 v(1, 2, 3, 4);
    ASSERT_NEAR(v.x, 1.0f, 1e-6f);
    ASSERT_NEAR(v.w, 4.0f, 1e-6f);
}

TEST(vec4_scalar_construction) {
    Vec4 v(3.0f);
    ASSERT_NEAR(v.x, 3.0f, 1e-6f);
    ASSERT_NEAR(v.w, 3.0f, 1e-6f);
}

TEST(vec4_from_vec3) {
    Vec3 v3(1, 2, 3);
    Vec4 v4(v3, 1.0f);
    ASSERT_NEAR(v4.x, 1.0f, 1e-6f);
    ASSERT_NEAR(v4.y, 2.0f, 1e-6f);
    ASSERT_NEAR(v4.z, 3.0f, 1e-6f);
    ASSERT_NEAR(v4.w, 1.0f, 1e-6f);
}

TEST(vec4_addition) {
    Vec4 a(1, 2, 3, 4);
    Vec4 b(5, 6, 7, 8);
    Vec4 c = a + b;
    ASSERT_NEAR(c.x, 6.0f, 1e-6f);
    ASSERT_NEAR(c.w, 12.0f, 1e-6f);
}

TEST(vec4_subtraction) {
    Vec4 a(5, 6, 7, 8);
    Vec4 b(1, 2, 3, 4);
    Vec4 c = a - b;
    ASSERT_NEAR(c.x, 4.0f, 1e-6f);
    ASSERT_NEAR(c.w, 4.0f, 1e-6f);
}

TEST(vec4_scalar_multiply) {
    Vec4 a(1, 2, 3, 4);
    Vec4 b = a * 2.0f;
    ASSERT_NEAR(b.x, 2.0f, 1e-6f);
    ASSERT_NEAR(b.w, 8.0f, 1e-6f);

    Vec4 c = 2.0f * a;
    ASSERT_NEAR(c.x, 2.0f, 1e-6f);
}

TEST(vec4_dot) {
    Vec4 a(1, 2, 3, 4);
    Vec4 b(5, 6, 7, 8);
    // 5 + 12 + 21 + 32 = 70
    ASSERT_NEAR(a.dot(b), 70.0f, 1e-6f);
}

// ============================================================
// Mat4 tests
// ============================================================

TEST(mat4_identity_construction) {
    Mat4 I = Mat4::identity();
    for (int c = 0; c < 4; c++) {
        for (int r = 0; r < 4; r++) {
            float expected = (c == r) ? 1.0f : 0.0f;
            ASSERT_NEAR(I.m[c][r], expected, 1e-6f);
        }
    }
}

TEST(mat4_identity_times_identity) {
    Mat4 I = Mat4::identity();
    Mat4 I2 = I * I;
    for (int c = 0; c < 4; c++) {
        for (int r = 0; r < 4; r++) {
            float expected = (c == r) ? 1.0f : 0.0f;
            ASSERT_NEAR(I2.m[c][r], expected, 1e-6f);
        }
    }
}

TEST(mat4_multiply_by_identity) {
    Mat4 A = Mat4::translate(Vec3(3, 4, 5));
    Mat4 I = Mat4::identity();
    Mat4 AI = A * I;
    Mat4 IA = I * A;
    for (int c = 0; c < 4; c++) {
        for (int r = 0; r < 4; r++) {
            ASSERT_NEAR(AI.m[c][r], A.m[c][r], 1e-6f);
            ASSERT_NEAR(IA.m[c][r], A.m[c][r], 1e-6f);
        }
    }
}

TEST(mat4_translation) {
    Mat4 T = Mat4::translate(Vec3(10, 20, 30));
    // column 3 holds the translation
    ASSERT_NEAR(T.m[3][0], 10.0f, 1e-6f);
    ASSERT_NEAR(T.m[3][1], 20.0f, 1e-6f);
    ASSERT_NEAR(T.m[3][2], 30.0f, 1e-6f);

    // verify via data() pointer
    const float* d = T.data();
    // column-major: element [3][0] is at index 12
    ASSERT_NEAR(d[12], 10.0f, 1e-6f);
    ASSERT_NEAR(d[13], 20.0f, 1e-6f);
    ASSERT_NEAR(d[14], 30.0f, 1e-6f);
}

TEST(mat4_scale) {
    Mat4 S = Mat4::scale(Vec3(2, 3, 4));
    ASSERT_NEAR(S.m[0][0], 2.0f, 1e-6f);
    ASSERT_NEAR(S.m[1][1], 3.0f, 1e-6f);
    ASSERT_NEAR(S.m[2][2], 4.0f, 1e-6f);
    ASSERT_NEAR(S.m[3][3], 1.0f, 1e-6f);
    // off-diagonals should be zero
    ASSERT_NEAR(S.m[0][1], 0.0f, 1e-6f);
    ASSERT_NEAR(S.m[1][0], 0.0f, 1e-6f);
}

TEST(mat4_rotate_z_90) {
    float angle = radians(90.0f);
    Mat4 R = Mat4::rotate_z(angle);
    // rotating (1,0,0) by 90 degrees CCW => (0,1,0)
    // new_x = R[0][0]*1 + R[1][0]*0 = cos(90) ~ 0
    // new_y = R[0][1]*1 + R[1][1]*0 = sin(90) ~ 1
    ASSERT_NEAR(R.m[0][0], 0.0f, 1e-5f);  // cos(90)
    ASSERT_NEAR(R.m[0][1], 1.0f, 1e-5f);  // sin(90)
    ASSERT_NEAR(R.m[1][0], -1.0f, 1e-5f); // -sin(90)
    ASSERT_NEAR(R.m[1][1], 0.0f, 1e-5f);  // cos(90)
}

TEST(mat4_rotate_z_360_is_identity) {
    Mat4 R = Mat4::rotate_z(radians(360.0f));
    Mat4 I = Mat4::identity();
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++)
            ASSERT_NEAR(R.m[c][r], I.m[c][r], 1e-4f);
}

TEST(mat4_ortho_basic) {
    Mat4 O = Mat4::ortho(0, 800, 0, 600, -1, 1);
    // m[0][0] = 2/(right-left)
    ASSERT_NEAR(O.m[0][0], 2.0f / 800.0f, 1e-6f);
    // m[1][1] = 2/(top-bottom)
    ASSERT_NEAR(O.m[1][1], 2.0f / 600.0f, 1e-6f);
    // m[3][3] = 1
    ASSERT_NEAR(O.m[3][3], 1.0f, 1e-6f);
}

TEST(mat4_ortho_maps_center_to_origin) {
    // For symmetric ortho, the center of the view should map to origin
    Mat4 O = Mat4::ortho(-400, 400, -300, 300, -1, 1);
    // translation components should be 0 for symmetric bounds
    ASSERT_NEAR(O.m[3][0], 0.0f, 1e-6f);
    ASSERT_NEAR(O.m[3][1], 0.0f, 1e-6f);
}

TEST(mat4_perspective_nonzero) {
    Mat4 P = Mat4::perspective(radians(60.0f), 16.0f / 9.0f, 0.1f, 100.0f);
    ASSERT_TRUE(P.m[0][0] != 0.0f);
    ASSERT_TRUE(P.m[1][1] != 0.0f);
    ASSERT_TRUE(P.m[2][2] != 0.0f);
    ASSERT_TRUE(P.m[2][3] != 0.0f);
    ASSERT_TRUE(P.m[3][2] != 0.0f);
    // m[3][3] should be 0 for perspective
    ASSERT_NEAR(P.m[3][3], 0.0f, 1e-6f);
}

TEST(mat4_data_pointer) {
    Mat4 I = Mat4::identity();
    const float* d = I.data();
    // column-major: d[0]=1, d[1..3]=0, d[4]=0, d[5]=1, etc.
    ASSERT_NEAR(d[0], 1.0f, 1e-6f);
    ASSERT_NEAR(d[1], 0.0f, 1e-6f);
    ASSERT_NEAR(d[5], 1.0f, 1e-6f);
    ASSERT_NEAR(d[10], 1.0f, 1e-6f);
    ASSERT_NEAR(d[15], 1.0f, 1e-6f);
}

// ============================================================
// Math utils tests
// ============================================================

TEST(math_radians_conversion) {
    ASSERT_NEAR(radians(0.0f), 0.0f, 1e-6f);
    ASSERT_NEAR(radians(180.0f), PI, 1e-5f);
    ASSERT_NEAR(radians(360.0f), TAU, 1e-5f);
    ASSERT_NEAR(radians(90.0f), PI / 2.0f, 1e-5f);
}

TEST(math_degrees_conversion) {
    ASSERT_NEAR(degrees(0.0f), 0.0f, 1e-6f);
    ASSERT_NEAR(degrees(PI), 180.0f, 1e-3f);
    ASSERT_NEAR(degrees(TAU), 360.0f, 1e-3f);
}

TEST(math_radians_degrees_roundtrip) {
    float original = 45.0f;
    ASSERT_NEAR(degrees(radians(original)), original, 1e-4f);
}

TEST(math_clamp) {
    ASSERT_NEAR(clamp(5.0f, 0.0f, 10.0f), 5.0f, 1e-6f);
    ASSERT_NEAR(clamp(-1.0f, 0.0f, 10.0f), 0.0f, 1e-6f);
    ASSERT_NEAR(clamp(15.0f, 0.0f, 10.0f), 10.0f, 1e-6f);
    ASSERT_NEAR(clamp(0.0f, 0.0f, 10.0f), 0.0f, 1e-6f);
    ASSERT_NEAR(clamp(10.0f, 0.0f, 10.0f), 10.0f, 1e-6f);
}

TEST(math_lerp) {
    ASSERT_NEAR(lerp(0.0f, 10.0f, 0.0f), 0.0f, 1e-6f);
    ASSERT_NEAR(lerp(0.0f, 10.0f, 1.0f), 10.0f, 1e-6f);
    ASSERT_NEAR(lerp(0.0f, 10.0f, 0.5f), 5.0f, 1e-6f);
    ASSERT_NEAR(lerp(-10.0f, 10.0f, 0.5f), 0.0f, 1e-6f);
    // extrapolation
    ASSERT_NEAR(lerp(0.0f, 10.0f, 2.0f), 20.0f, 1e-6f);
}

TEST(math_smoothstep) {
    // below edge0
    ASSERT_NEAR(smoothstep(0.0f, 1.0f, -0.5f), 0.0f, 1e-6f);
    // above edge1
    ASSERT_NEAR(smoothstep(0.0f, 1.0f, 1.5f), 1.0f, 1e-6f);
    // at edge0
    ASSERT_NEAR(smoothstep(0.0f, 1.0f, 0.0f), 0.0f, 1e-6f);
    // at edge1
    ASSERT_NEAR(smoothstep(0.0f, 1.0f, 1.0f), 1.0f, 1e-6f);
    // midpoint should be 0.5
    ASSERT_NEAR(smoothstep(0.0f, 1.0f, 0.5f), 0.5f, 1e-6f);
}

TEST(math_approx_equal) {
    ASSERT_TRUE(approx_equal(1.0f, 1.0f));
    ASSERT_TRUE(approx_equal(1.0f, 1.0f + 1e-7f));
    ASSERT_FALSE(approx_equal(1.0f, 1.1f));
    ASSERT_TRUE(approx_equal(1.0f, 1.01f, 0.1f));
    ASSERT_FALSE(approx_equal(1.0f, 2.0f, 0.5f));
}

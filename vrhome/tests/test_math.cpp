#include "test.h"

#include "math/mat4.h"
#include "math/head.h"
#include "common/config.h"

static void checkIdentity(const Mat4& m) {
    for (int i = 0; i < 16; ++i)
        CHECK_F(m.m[i], (i % 5 == 0) ? 1.0f : 0.0f, 1e-5f);
}

static void checkEqual(const Mat4& a, const Mat4& b, float eps = 1e-5f) {
    for (int i = 0; i < 16; ++i) CHECK_F(a.m[i], b.m[i], eps);
}

// quaternion for a given 3x3 rotation, row-major r[9]
static void matToQuat(const float* r, float q[4]) {
    const float tr = r[0] + r[4] + r[8];
    q[3] = sqrtf(1.0f + tr) / 2.0f;
    q[0] = (r[7] - r[5]) / (4.0f * q[3]);
    q[1] = (r[2] - r[6]) / (4.0f * q[3]);
    q[2] = (r[3] - r[1]) / (4.0f * q[3]);
}

static void quatMul(const float* a, const float* b, float* o) {
    o[3] = a[3]*b[3] - a[0]*b[0] - a[1]*b[1] - a[2]*b[2];
    o[0] = a[3]*b[0] + a[0]*b[3] + a[1]*b[2] - a[2]*b[1];
    o[1] = a[3]*b[1] - a[0]*b[2] + a[1]*b[3] + a[2]*b[0];
    o[2] = a[3]*b[2] + a[0]*b[1] - a[1]*b[0] + a[2]*b[3];
}

void testMat4() {
    Mat4 I = identity();
    checkIdentity(I);

    // multiply by identity is a no-op, both sides
    Mat4 rz = rotZ(37.0f);
    checkEqual(multiply(I, rz), rz);
    checkEqual(multiply(rz, I), rz);

    // inverse rotations cancel
    checkEqual(multiply(rotZ(30.0f), rotZ(-30.0f)), I);
    checkEqual(multiply(rotX(25.0f), rotX(-25.0f)), I);

    // rotZ 90: +x -> +y (column 0 holds the transformed basis vector)
    Mat4 z90 = rotZ(90.0f);
    CHECK_F(z90.m[0], 0.0f, 1e-5f);
    CHECK_F(z90.m[1], 1.0f, 1e-5f);
    // rotX 90: +y -> +z (column 1)
    Mat4 x90 = rotX(90.0f);
    CHECK_F(x90.m[5], 0.0f, 1e-5f);
    CHECK_F(x90.m[6], 1.0f, 1e-5f);

    // perspective: straight ahead maps to clip origin, top of fov to y=w
    Mat4 p = perspective(90.0f, 1.0f, 0.1f, 100.0f);
    CHECK_F(p.m[5], 1.0f, 1e-4f);   // f = 1/tan(45)
    CHECK_F(p.m[11], -1.0f, 1e-6f);

    // quatToMat: identity quat -> identity
    const float qi[4] = {0, 0, 0, 1};
    checkIdentity(quatToMat(qi, false));
    checkIdentity(quatToMat(qi, true));

    // +90 deg about Y maps +x -> -z
    const float s = 0.70710678f;
    const float qy[4] = {0, s, 0, s};
    Mat4 r = quatToMat(qy, false);
    CHECK_F(r.m[0], 0.0f, 1e-5f);
    CHECK_F(r.m[2], -1.0f, 1e-5f);

    // inv=true is the transpose (the head-tracking bug lived here)
    Mat4 rt = quatToMat(qy, true);
    for (int c = 0; c < 4; ++c)
        for (int i = 0; i < 4; ++i)
            CHECK_F(rt.m[c*4+i], r.m[i*4+c], 1e-6f);

    // non-identity axis: transpose still holds, matrix stays orthonormal
    float qn[4] = {0.2f, -0.4f, 0.6f, 0.7f};
    const float ql = sqrtf(qn[0]*qn[0] + qn[1]*qn[1] + qn[2]*qn[2] +
                           qn[3]*qn[3]);
    for (int i = 0; i < 4; ++i) qn[i] /= ql;
    Mat4 rn = quatToMat(qn, false), rnt = quatToMat(qn, true);
    checkEqual(multiply(rn, rnt), I, 1e-4f);

    // viewDirToWorld on a view matrix V returns V^T * in (rotation only)
    float out[3];
    viewDirToWorld(rt, (const float[]){1, 0, 0}, out);
    CHECK_F(out[0], rt.m[0], 1e-6f);
    CHECK_F(out[1], rt.m[4], 1e-6f);
    CHECK_F(out[2], rt.m[8], 1e-6f);

    CHECK_F(wrapPi(3.5f), 3.5f - 2.0f * (float)M_PI, 1e-5f);
    CHECK_F(wrapPi(-3.5f), -3.5f + 2.0f * (float)M_PI, 1e-5f);
    CHECK_F(wrapPi(1.0f), 1.0f, 1e-6f);
}

void testHead() {
    // w reconstruction: present, omitted but recoverable, degenerate
    CHECK_F(quatW((const float[]){0.1f, 0.2f, 0.3f, 0.9f}), 0.9f, 1e-6f);
    CHECK_F(quatW((const float[]){0.5f, 0.0f, 0.0f, 0.0f}),
            sqrtf(0.75f), 1e-6f);
    CHECK_F(quatW((const float[]){0.9f, 0.9f, 0.0f, 0.0f}), 0.0f, 1e-6f);

    // sensor off -> static roll only
    const float qi[4] = {0, 0, 0, 1};
    checkEqual(headMatrix(qi, true, 0, kWorldX, 33.0f, false),
               rotZ(33.0f));

    // chain order: roll * quatMat * sensRoll * worldX
    const float q[4] = {0.1f, -0.3f, 0.2f, 0.9f};
    Mat4 expect = multiply(rotZ(90.0f),
                  multiply(quatToMat(q, true),
                  multiply(rotZ(5.0f), rotX(90.0f))));
    checkEqual(headMatrix(q, true, 5.0f, 90.0f, 90.0f, true), expect, 1e-4f);

    // REGRESSION for the yaw-into-roll bug. Physical mount pose is
    // Rmount = X90 * Z90; with the transpose path the corrected chain
    // (Z90 * R^T * X90) must collapse to identity at neutral.
    const float rm[9] = {0,-1,0,  0,0,-1,  1,0,0};
    float qMount[4];
    matToQuat(rm, qMount);
    Mat4 neutral = headMatrix(qMount, true, 0.0f, 90.0f, 90.0f, true);
    checkIdentity(neutral);

    // same pose through the old broken path must NOT be identity
    Mat4 broken = headMatrix(qMount, false, 0.0f, 90.0f, 90.0f, true);
    bool isIdent = true;
    for (int i = 0; i < 16; ++i)
        if (fabsf(broken.m[i] - ((i % 5 == 0) ? 1.0f : 0.0f)) > 1e-4f)
            isIdent = false;
    CHECK(!isIdent);

    // head yaw is a rotation about the sensor world's Z (up) axis.
    // Through the fixed chain it must come out as a view-space yaw:
    // V' = Ryaw(-d) exactly.
    const float d = 0.3f;
    const float qz[4] = {0, 0, sinf(d / 2), cosf(d / 2)};
    float qYawed[4];
    quatMul(qz, qMount, qYawed);
    Mat4 v = headMatrix(qYawed, true, 0.0f, 90.0f, 90.0f, true);
    Mat4 ry = quatToMat((const float[]){0, -sinf(d / 2), 0, cosf(d / 2)},
                        false);
    checkEqual(v, ry, 1e-4f);

    // eye shift: left eye at -ipd/2 on view x, right at +ipd/2
    Mat4 I = identity();
    Mat4 el = eyeMatrix(I, 0.064f, 0), er = eyeMatrix(I, 0.064f, 1);
    CHECK_F(el.m[12], -0.032f, 1e-6f);
    CHECK_F(er.m[12], 0.032f, 1e-6f);

    // gaze dir of identity head is -z, yaw 0
    float g[3];
    gazeDir(I, g);
    CHECK_F(g[0], 0.0f, 1e-6f);
    CHECK_F(g[1], 0.0f, 1e-6f);
    CHECK_F(g[2], -1.0f, 1e-6f);
    float gy = 99.0f;
    CHECK(gazeYaw(I, &gy));
    CHECK_F(gy, 0.0f, 1e-6f);

    // straight up: no horizontal yaw to report
    Mat4 up = quatToMat((const float[]){0.7071f, 0, 0, 0.7071f}, false);
    CHECK(!gazeYaw(up, &gy));

    // euler extraction: identity quat -> all zero
    float yaw, pitch, roll;
    quatToYpr(qi, &yaw, &pitch, &roll);
    CHECK_F(yaw, 0.0f, 1e-4f);
    CHECK_F(pitch, 0.0f, 1e-4f);
    CHECK_F(roll, 0.0f, 1e-4f);
}

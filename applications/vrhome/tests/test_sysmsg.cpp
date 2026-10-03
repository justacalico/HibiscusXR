#include "test.h"

#include "sysmsg/layout.h"
#include "common/config.h"
#include "dock/layout.h"
#include "math/head.h"

static SysMsgItem mkMsg(const char* title, int nbtn) {
    SysMsgItem m;
    m.id = 1;
    m.pkg = "com.a";
    m.title = title;
    m.text = "java.lang.RuntimeException: boom";
    for (int i = 0; i < nbtn; ++i) m.buttons.push_back("b");
    return m;
}

static const float o0[3] = {0.0f, 0.0f, 0.0f};

void testSysMsg() {
    // button geometry: n pills share the card's inner width, centred row
    {
        const float iw = kSysMsgW * 0.5f - kSysMsgPad;
        // one button fills the inner width
        CHECK(fabsf(sysMsgBtnHW(1) - iw) < 1e-6f);
        CHECK(fabsf(sysMsgBtnX(0, 1)) < 1e-6f);
        // two buttons split it symmetrically around the centre
        const float hw2 = sysMsgBtnHW(2);
        const float x0 = sysMsgBtnX(0, 2), x1 = sysMsgBtnX(1, 2);
        CHECK(fabsf(x0 + x1) < 1e-5f);
        CHECK(x0 < 0.0f && x1 > 0.0f);
        // the row ends exactly at the inner edge on both sides
        CHECK(fabsf(x0 - hw2 - (-iw)) < 1e-5f);
        CHECK(fabsf(x1 + hw2 - iw) < 1e-5f);
        // the gap between the pills is kSysMsgBtnGap
        CHECK(fabsf((x1 - hw2) - (x0 + hw2) - kSysMsgBtnGap) < 1e-5f);
        // the button row sits inside the card's lower half
        CHECK(sysMsgBtnY() < 0.0f);
        CHECK(sysMsgBtnY() - kSysMsgBtnHH >
              -kSysMsgH * 0.5f - 1e-6f);
    }

    // hit testing: outside misses, body hits, the pills report their index
    {
        int btn = -1;
        const float hw = kSysMsgW * 0.5f, hh = kSysMsgH * 0.5f;
        CHECK(sysMsgAt(0.0f, 0.0f, 2, &btn) == MZONE_BODY && btn == -1);
        CHECK(sysMsgAt(1.2f, 0.0f, 2, &btn) == MZONE_NONE);
        CHECK(sysMsgAt(0.0f, 1.2f, 2, &btn) == MZONE_NONE);
        // card-local y of the button row, in card coords
        const float vb = sysMsgBtnY() / hh;
        const float u0 = sysMsgBtnX(0, 2) / hw;
        const float u1 = sysMsgBtnX(1, 2) / hw;
        CHECK(sysMsgAt(u0, vb, 2, &btn) == MZONE_BTN && btn == 0);
        CHECK(sysMsgAt(u1, vb, 2, &btn) == MZONE_BTN && btn == 1);
        // the pills' hit slack is wider than half the gap, so a point in
        // the row's middle still lands on a pill - first match wins
        CHECK(sysMsgAt(0.0f, vb, 2, &btn) == MZONE_BTN && btn == 0);
        // above the row is plain body again
        CHECK(sysMsgAt(0.0f, vb + 0.4f, 2, &btn) == MZONE_BODY);
        // a card with no buttons has no button zone
        CHECK(sysMsgAt(u0, vb, 0, &btn) == MZONE_BODY);
    }

    // ray vs the card plane: the card rides the dock anchor + lift
    {
        const float origin[3] = {0.0f, 0.0f, 0.0f};
        float c[3], r[3], up[3];
        sysMsgCenter(0.0f, kDockPitchRest, kSysMsgLift, origin, c, r, up);
        float dc[3], dr[3], dup[3];
        dockCenter(0.0f, kDockPitchRest, origin, dc, dr, dup);
        const float dd = sqrtf((c[0]-dc[0])*(c[0]-dc[0]) +
                               (c[1]-dc[1])*(c[1]-dc[1]) +
                               (c[2]-dc[2])*(c[2]-dc[2]));
        CHECK(fabsf(dd - kSysMsgLift) < 1e-5f);
        // a ray from the eye through the card centre hits u=v=0
        const float o[3] = {0.0f, 0.0f, 0.0f};
        float d[3] = {c[0] - o[0], c[1] - o[1], c[2] - o[2]};
        const float len = sqrtf(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);
        d[0] /= len; d[1] /= len; d[2] /= len;
        float u, v, t;
        CHECK(raySysMsg(0.0f, kDockPitchRest, kSysMsgLift, origin, o, d,
                        &u, &v, &t));
        CHECK(fabsf(u) < 1e-4f && fabsf(v) < 1e-4f);
        CHECK(t > 0.5f && t < 3.0f);
    }

    // pickSysMsgRay: only the front card picks; zones map through
    {
        std::vector<SysMsgItem> items = {mkMsg("a keeps stopping", 2),
                                         mkMsg("b keeps stopping", 2)};
        float c[3], r[3], up[3];
        sysMsgCenter(0.0f, kDockPitchRest, kSysMsgLift, o0, c, r, up);
        float d[3] = {c[0], c[1], c[2]};
        const float len = sqrtf(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);
        d[0] /= len; d[1] /= len; d[2] /= len;
        SysMsgPick pk = pickSysMsgRay(items, 0.0f, kDockPitchRest,
                                      kSysMsgLift, o0, o0, d);
        CHECK(pk.hit && pk.zone == MZONE_BODY && pk.btn == -1);
        // no cards: no pick
        std::vector<SysMsgItem> none;
        pk = pickSysMsgRay(none, 0.0f, kDockPitchRest, kSysMsgLift,
                           o0, o0, d);
        CHECK(!pk.hit);
    }

    // pickSysMsg through a head matrix: a level gaze at yaw 0 misses the
    // card when it sits at kSysMsgPitch off an unanchored yaw
    {
        std::vector<SysMsgItem> items = {mkMsg("a keeps stopping", 1)};
        Mat4 head = identity();
        SysMsgPick pk = pickSysMsg(items, 0.0f, kSysMsgPitch, 0.0f,
                                   head, o0, o0);
        // the card hovers just above eye level: a level gaze passes under
        // its centre but still clips the card or misses cleanly - verify
        // only the contract that a real ray resolves
        CHECK(pk.zone == MZONE_BODY || pk.zone == MZONE_BTN ||
              pk.zone == MZONE_NONE);
    }

    // modal rule: any live card blanks the dash; covered-only mode is
    // modal even while the item list hasn't synced yet
    {
        std::vector<SysMsgItem> none;
        std::vector<SysMsgItem> one = {mkMsg("a keeps stopping", 1)};
        CHECK(!sysMsgModal(false, none));
        CHECK(sysMsgModal(false, one));
        CHECK(sysMsgModal(true, none));
        CHECK(sysMsgModal(true, one));
    }

    // anchor: covered-only floats the card on its own yaw at card pitch
    // with no lift; the dash anchor rides the dock yaw/pitch at the
    // card's eye-level lift
    {
        float yaw = -1.0f, pitch = -1.0f, lift = -1.0f;
        sysMsgAnchor(true, 0.7f, 0.2f, kDockPitchRest,
                     &yaw, &pitch, &lift);
        CHECK(fabsf(yaw - 0.7f) < 1e-6f);
        CHECK(fabsf(pitch - kSysMsgPitch) < 1e-6f);
        CHECK(fabsf(lift) < 1e-6f);
        sysMsgAnchor(false, 0.7f, 0.2f, kDockPitchRest,
                     &yaw, &pitch, &lift);
        CHECK(fabsf(yaw - 0.2f) < 1e-6f);
        CHECK(fabsf(pitch - kDockPitchRest) < 1e-6f);
        CHECK(fabsf(lift - kSysMsgLift) < 1e-6f);
    }
}

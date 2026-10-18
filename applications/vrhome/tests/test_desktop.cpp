#include "test.h"

#include "../src/desktop/desktopfile.h"
#include "../src/desktop/icontheme.h"
#include "../src/desktop/popuppos.h"
#include "../src/desktop/pixconv.h"
#include "../src/desktop/sysinfo.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

static void putFile(const std::string& path, const std::string& text) {
    fs::create_directories(fs::path(path).parent_path());
    FILE* f = fopen(path.c_str(), "wb");
    fwrite(text.data(), 1, text.size(), f);
    fclose(f);
}

static void testDesktopFile() {
    const std::string text =
        "[Desktop Entry]\n"
        "Type=Application\n"
        "Name=Firefox\n"
        "Name[de]=Feuerfuchs\n"
        "Exec=/usr/lib/firefox/firefox %u %i\n"
        "Icon=firefox\n"
        "StartupWMClass=firefox\n"
        "[Desktop Action new-window]\n"
        "Name=New Window\n"
        "Exec=firefox --new-window\n";
    DeskEntry e;
    CHECK(parseDesktopFile(text, "firefox.desktop", &e));
    CHECK(e.id == "firefox.desktop");
    CHECK(e.name == "Firefox");
    CHECK(e.icon == "firefox");
    CHECK(e.wmClass == "firefox");
    CHECK(!e.argv.empty());
    CHECK(e.argv[0] == "/usr/lib/firefox/firefox");
    // %u drops out, %i adds --icon firefox
    CHECK(e.argv[1] == "--icon");
    CHECK(e.argv[2] == "firefox");

    DeskEntry bad;
    CHECK(!parseDesktopFile("[Desktop Entry]\nType=Application\n"
                            "NoDisplay=true\nExec=x\n",
                            "x.desktop", &bad));
    CHECK(!parseDesktopFile("[Desktop Entry]\nType=Application\n"
                            "Name=x\n", "x.desktop", &bad));
    CHECK(!parseDesktopFile("[Desktop Action a]\nType=Application\n"
                            "Exec=x\n", "x.desktop", &bad));
}

static void testExecArgv() {
    std::vector<std::string> a = execArgv("foo --open %F --name=%c %%",
                                          "My App", "ic", "id.desktop");
    CHECK(a.size() == 4);
    CHECK(a[0] == "foo");
    CHECK(a[1] == "--open");
    CHECK(a[2] == "--name=My App");
    CHECK(a[3] == "%");

    a = execArgv("\"env\" FOO=bar run", "n", "", "p");
    CHECK(a.size() == 3);
    CHECK(a[0] == "env");
    CHECK(a[1] == "FOO=bar");
}

static void testScanDesktopFiles() {
    const std::string root = "/tmp/dash-test-apps";
    fs::remove_all(root);
    putFile(root + "/a1/firefox.desktop",
            "[Desktop Entry]\nType=Application\nName=FF\nExec=ff\n");
    putFile(root + "/a1/notdesk.txt", "junk");
    putFile(root + "/a2/firefox.desktop",
            "[Desktop Entry]\nType=Application\nName=Dup\nExec=ff2\n");
    putFile(root + "/a2/term.desktop",
            "[Desktop Entry]\nType=Application\nName=T\nExec=t\n");
    const std::vector<DeskEntry> out =
        scanDesktopFiles({root + "/a1", root + "/a2"});
    CHECK(out.size() == 2);
    CHECK(out[0].name == "FF");
    CHECK(out[1].name == "T");
    fs::remove_all(root);
}

static void testIndexTheme() {
    IconTheme t;
    CHECK(parseIndexTheme(
        "[Icon Theme]\nName=Test\nInherits=hicolor,other\n"
        "Directories=48x48/apps,scalable/apps\n"
        "[48x48/apps]\nSize=48\nType=Fixed\n"
        "[scalable/apps]\nMinSize=16\nMaxSize=256\nType=Scalable\n"
        "[32x32/apps]\nSize=32\nType=Threshold\nThreshold=4\n", &t));
    CHECK(t.parents.size() == 2);
    CHECK(t.parents[0] == "hicolor");
    CHECK(t.subs.size() == 3);

    IconSubdir fixed; fixed.size = 48; fixed.type = 0;
    CHECK(iconSizeFit(fixed, 48) == 0);
    CHECK(iconSizeFit(fixed, 32) > 0);

    IconSubdir scal; scal.minSize = 16; scal.maxSize = 256; scal.type = 1;
    CHECK(iconSizeFit(scal, 64) == 0);
    CHECK(iconSizeFit(scal, 512) == 256);
    CHECK(iconSizeFit(scal, 8) == 8);

    IconSubdir th; th.size = 32; th.type = 2; th.threshold = 4;
    CHECK(iconSizeFit(th, 35) == 0);
    CHECK(iconSizeFit(th, 48) > 0);
}

static void testFindIconFile() {
    const std::string root = "/tmp/dash-test-icons";
    fs::remove_all(root);
    putFile(root + "/Test/index.theme",
            "[Icon Theme]\nName=Test\nDirectories=48x48/apps,"
            "scalable/apps\n"
            "[48x48/apps]\nSize=48\nType=Fixed\n"
            "[scalable/apps]\nMinSize=16\nMaxSize=256\nType=Scalable\n");
    putFile(root + "/Test/48x48/apps/foo.png", "png");
    putFile(root + "/Test/scalable/apps/bar.svg", "svg");
    putFile(root + "/hicolor/index.theme",
            "[Icon Theme]\nName=hc\nDirectories=16x16/apps\n"
            "[16x16/apps]\nSize=16\n");
    putFile(root + "/hicolor/16x16/apps/fallback.png", "png");

    CHECK(findIconFile({root}, {"Test"}, "foo", 48) ==
          root + "/Test/48x48/apps/foo.png");
    // no exact 64px: scalable subdir matches the range
    CHECK(findIconFile({root}, {"Test"}, "bar", 64) ==
          root + "/Test/scalable/apps/bar.svg");
    // not in Test at all: hicolor is always the last stop
    CHECK(findIconFile({root}, {"Test"}, "fallback", 16) ==
          root + "/hicolor/16x16/apps/fallback.png");
    CHECK(findIconFile({root}, {"Test"}, "nope", 48).empty());
    fs::remove_all(root);
}

static void testSubdirFromName() {
    IconSubdir s;
    CHECK(subdirFromName("48x48", &s));
    CHECK(s.type == 0 && s.size == 48 && s.scale == 1);
    CHECK(subdirFromName("16x16@2x", &s));
    CHECK(s.type == 0 && s.size == 16 && s.scale == 2);
    CHECK(subdirFromName("32x32@2", &s));
    CHECK(s.scale == 2);
    CHECK(subdirFromName("scalable", &s));
    CHECK(s.type == 1);
    CHECK(subdirFromName("scalable@2x", &s));
    CHECK(s.type == 1 && s.scale == 2);
    CHECK(!subdirFromName("apps", &s));
    CHECK(!subdirFromName("cursors", &s));
    CHECK(!subdirFromName("32x", &s));
    CHECK(!subdirFromName("x32", &s));
    CHECK(!subdirFromName("48x48@x", &s));
}

static void testIndexlessTheme() {
    const std::string root = "/tmp/dash-test-icons-bare";
    fs::remove_all(root);
    // hicolor under ~/.local/share/icons ships no index.theme - Steam
    // drops game icons straight into its sized dirs
    putFile(root + "/Bare/32x32/apps/foo.png", "png");
    putFile(root + "/Bare/48x48@2x/apps/hi.png", "png");
    putFile(root + "/Bare/scalable/apps/bar.svg", "svg");
    putFile(root + "/Bare/apps/nope.png", "png");

    // exact size hits pass 0, anything else the distance fallback
    CHECK(findIconFile({root}, {"Bare"}, "foo", 32) ==
          root + "/Bare/32x32/apps/foo.png");
    CHECK(findIconFile({root}, {"Bare"}, "foo", 96) ==
          root + "/Bare/32x32/apps/foo.png");
    // 96 at scale 2 wants a 48px asset
    CHECK(findIconFile({root}, {"Bare"}, "hi", 96) ==
          root + "/Bare/48x48@2x/apps/hi.png");
    CHECK(findIconFile({root}, {"Bare"}, "bar", 64) ==
          root + "/Bare/scalable/apps/bar.svg");
    // names without size info are not icon dirs
    CHECK(findIconFile({root}, {"Bare"}, "nope", 48).empty());
    // still works as the appended hicolor fallback
    fs::rename(root + "/Bare", root + "/hicolor");
    CHECK(findIconFile({root}, {"Test"}, "foo", 32) ==
          root + "/hicolor/32x32/apps/foo.png");
    fs::remove_all(root);
}

static void testPopupPos() {
    int x, y;
    // menu under a button: anchor bottom-left corner, popup grows
    // down-right
    popupPos({100, 50, 80, 30}, 200, 100,
             7 /*bottom_left*/, 8 /*bottom_right*/, 0, 0, &x, &y);
    CHECK(x == 100); CHECK(y == 80);

    // popup above the anchor point, centred
    popupPos({100, 50, 80, 30}, 200, 100, 1 /*top*/, 1 /*top*/,
             0, 0, &x, &y);
    CHECK(x == 40); CHECK(y == -50);

    // right of the rect, vertically centred, plus offset
    popupPos({100, 50, 80, 30}, 40, 20, 4 /*right*/, 4 /*right*/,
             5, 3, &x, &y);
    CHECK(x == 185); CHECK(y == 58);

    // centered on the rect
    popupPos({0, 0, 100, 100}, 20, 20, 9 /*center*/, 9 /*center*/,
             0, 0, &x, &y);
    CHECK(x == 40); CHECK(y == 40);
}

static void testPixconv() {
    unsigned char src[16];
    // one XRGB pixel: b=10 g=20 r=30
    src[0] = 10; src[1] = 20; src[2] = 30; src[3] = 0;
    unsigned char dst[16] = {};
    pxBlit(src, 16, PX_XRGB8888, 0, 0, dst, 16, 2, 2, 1, 1, 1, 1);
    CHECK(dst[20] == 30); CHECK(dst[21] == 20); CHECK(dst[22] == 10);
    CHECK(dst[23] == 255);

    // ABGR: a=128 r=40 g=50 b=60 premultiplied by a/255
    unsigned char p2[4];
    const unsigned a = 128;
    p2[0] = (unsigned char)(60 * a / 255);
    p2[1] = (unsigned char)(50 * a / 255);
    p2[2] = (unsigned char)(40 * a / 255);
    p2[3] = a;
    unsigned char d2[4] = {};
    pxBlit(p2, 4, PX_ABGR8888, 0, 0, d2, 4, 1, 1, 0, 0, 1, 1);
    // unpremultiplied values, ABGR->RGBA order
    CHECK(abs(d2[0] - 60) <= 2);
    CHECK(abs(d2[2] - 40) <= 2);
    CHECK(d2[3] == 128);

    // clip: blit overhanging the dst right edge drops the extra column
    unsigned char s3[8] = {1,2,3,4, 5,6,7,8};
    unsigned char d3[4] = {};
    pxBlit(s3, 8, PX_XRGB8888, 0, 0, d3, 4, 1, 1, 0, 0, 2, 1);
    CHECK(d3[0] == 3);   // xrgb -> r

    unsigned char fill[16] = {};
    pxFill(fill, 16, 2, 2, 0, 0, 2, 2, 9, 8, 7, 6);
    CHECK(fill[0] == 9); CHECK(fill[3] == 6);
}

static void testSysinfo() {
    CHECK(parseCapacity("87\n") == 87);
    CHECK(parseCapacity("") == -1);
    CHECK(parseCharging("Charging\n"));
    CHECK(parseCharging("Full"));
    CHECK(!parseCharging("Discharging"));

    CHECK(parseWifiSsid("no:old\nyes:HomeNet\nno:x\n") == "HomeNet");
    CHECK(parseWifiSsid("no:a\nno:b\n").empty());

    CHECK(parseNmRadio("enabled\n"));
    CHECK(!parseNmRadio("disabled\n"));

    CHECK_F(parseWpctlVol("Volume: 0.63\n"), 0.63f, 0.01f);
    CHECK(parseWpctlMuted("Volume: 0.63 [MUTED]"));
    CHECK(!parseWpctlMuted("Volume: 0.63"));

    CHECK_F(parseBrightness("amdgpu,backlight,192,75%,255"), 0.75f,
            0.01f);
}

void testDesktop() {
    testDesktopFile();
    testExecArgv();
    testScanDesktopFiles();
    testIndexTheme();
    testFindIconFile();
    testSubdirFromName();
    testIndexlessTheme();
    testPopupPos();
    testPixconv();
    testSysinfo();
}

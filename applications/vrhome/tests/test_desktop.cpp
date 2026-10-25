#include "test.h"

#include "../src/desktop/cmdline.h"
#include "../src/desktop/desktopfile.h"
#include "../src/desktop/icontheme.h"
#include "../src/desktop/svgicon.h"
#include "../src/desktop/popuppos.h"
#include "../src/desktop/pixconv.h"
#include "../src/desktop/proctrack.h"
#include "../src/desktop/sysinfo.h"
#include "../src/shell/backend.h"

#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

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

static void testConfGroup() {
    const std::string kde =
        "[General]\nName=x\n[Icons]\nTheme=breeze\nColor=y\n";
    CHECK(confGroupValue(kde, "Icons", "Theme") == "breeze");
    CHECK(confGroupValue(kde, "Icons", "Missing").empty());
    CHECK(confGroupValue(kde, "General", "Theme").empty());

    // gtkrc-2.0 has no sections and double-quoted values
    const std::string rc =
        "gtk-theme-name=\"Breeze\"\n"
        "gtk-icon-theme-name=\"Papirus-Dark\"\n";
    CHECK(confGroupValue(rc, "", "gtk-icon-theme-name") ==
          "Papirus-Dark");

    const std::string ini =
        "[Settings]\ngtk-icon-theme-name=Adwaita\n"
        "[Other]\ngtk-icon-theme-name=Wrong\n";
    CHECK(confGroupValue(ini, "Settings", "gtk-icon-theme-name") ==
          "Adwaita");
    CHECK(confGroupValue("", "Settings", "x").empty());
}

static void testSvgIcon() {
    const std::string dir = "/tmp/dash-test-svg";
    fs::remove_all(dir);
    putFile(dir + "/red.svg",
            "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"48\" "
            "height=\"48\"><rect width=\"48\" height=\"48\" "
            "fill=\"#ff0000\"/></svg>");
    putFile(dir + "/wide.svg",
            "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"48\" "
            "height=\"24\"><rect width=\"48\" height=\"24\" "
            "fill=\"#00ff00\"/></svg>");
    putFile(dir + "/bad.svg", "not xml at all");

    IconPixels px;
    CHECK(decodeSvgFile(dir + "/red.svg", 32, &px));
    CHECK(px.w == 32 && px.h == 32 && px.rgba.size() == 32 * 32 * 4);
    const unsigned char* c = px.rgba.data() + (16 * 32 + 16) * 4;
    CHECK(c[0] == 255 && c[1] == 0 && c[2] == 0 && c[3] == 255);

    // non-square art letterboxes into the square output
    CHECK(decodeSvgFile(dir + "/wide.svg", 48, &px));
    const unsigned char* top = px.rgba.data() + (4 * 48 + 24) * 4;
    const unsigned char* mid = px.rgba.data() + (24 * 48 + 24) * 4;
    CHECK(top[3] == 0);
    CHECK(mid[0] == 0 && mid[1] == 255 && mid[3] == 255);

    CHECK(!decodeSvgFile(dir + "/bad.svg", 32, &px));
    CHECK(!decodeSvgFile(dir + "/nope.svg", 32, &px));
    CHECK(!decodeSvgFile(dir + "/red.svg", 0, &px));
    fs::remove_all(dir);
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

    // ABGR stays premultiplied: the bytes only get swizzled to RGBA
    unsigned char p2[4];
    const unsigned a = 128;
    p2[0] = (unsigned char)(60 * a / 255);
    p2[1] = (unsigned char)(50 * a / 255);
    p2[2] = (unsigned char)(40 * a / 255);
    p2[3] = a;
    unsigned char d2[4] = {};
    pxBlit(p2, 4, PX_ABGR8888, 0, 0, d2, 4, 1, 1, 0, 0, 1, 1);
    CHECK(d2[0] == p2[0]);
    CHECK(d2[1] == p2[1]);
    CHECK(d2[2] == p2[2]);
    CHECK(d2[3] == 128);

    // clip: blit overhanging the dst right edge drops the extra column
    unsigned char s3[8] = {1,2,3,4, 5,6,7,8};
    unsigned char d3[4] = {};
    pxBlit(s3, 8, PX_XRGB8888, 0, 0, d3, 4, 1, 1, 0, 0, 2, 1);
    CHECK(d3[0] == 3);   // xrgb -> r

    // ARGB blit is a straight row copy
    unsigned char s4[8] = {9, 8, 7, 6, 5, 4, 3, 2};
    unsigned char d4[8] = {};
    pxBlit(s4, 8, PX_ARGB8888, 0, 0, d4, 8, 2, 1, 0, 0, 2, 1);
    CHECK(d4[0] == 9); CHECK(d4[7] == 2);

    // premult over: half-alpha red (ARGB = B,G,R,A bytes) above opaque
    // blue leaves dst's share under the blend
    unsigned char bs[4] = {0, 0, 128, 128};
    unsigned char bd[4] = {0, 0, 200, 255};
    pxBlend(bs, 4, PX_ARGB8888, 0, 0, bd, 4, 1, 1, 0, 0, 1, 1);
    CHECK(bd[0] == 128);
    CHECK(bd[2] == 99);
    CHECK(bd[3] == 255);

    // transparent src leaves dst alone; opaque src overwrites it
    unsigned char ts[4] = {0, 0, 0, 0};
    pxBlend(ts, 4, PX_ARGB8888, 0, 0, bd, 4, 1, 1, 0, 0, 1, 1);
    CHECK(bd[0] == 128 && bd[2] == 99 && bd[3] == 255);
    unsigned char os[4] = {11, 22, 33, 255};
    pxBlend(os, 4, PX_ARGB8888, 0, 0, bd, 4, 1, 1, 0, 0, 1, 1);
    CHECK(bd[0] == 33 && bd[1] == 22 && bd[2] == 11 && bd[3] == 255);

    unsigned char fill[16] = {};
    pxFill(fill, 16, 2, 2, 0, 0, 2, 2, 9, 8, 7, 6);
    CHECK(fill[0] == 9); CHECK(fill[3] == 6);

    int cw = 0, ch = 0;
    const unsigned char* cur = cursorDefaultPx(&cw, &ch);
    CHECK(cur && cw > 0 && ch > 0);
    CHECK(cur[3] == 255);              // the hotspot tip is solid
    CHECK(cur[(ch - 1) * cw * 4 + 3] == 0);  // bottom-left stays clear
    bool solid = true;
    for (int i = 0; i < cw * ch; ++i)
        if (cur[i * 4 + 3] != 0 && cur[i * 4 + 3] != 255) solid = false;
    CHECK(solid);                      // hard-edged shape, no fringe
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

    CHECK(parseEthLink("wifi:connected\nethernet:connected\n"
                       "loopback:unmanaged\n"));
    CHECK(parseEthLink("ethernet:connected (externally)\n"));
    CHECK(!parseEthLink("ethernet:unavailable\nwifi:connected\n"));
    CHECK(!parseEthLink(""));

    CHECK_F(parseWpctlVol("Volume: 0.63\n"), 0.63f, 0.01f);
    CHECK(parseWpctlMuted("Volume: 0.63 [MUTED]"));
    CHECK(!parseWpctlMuted("Volume: 0.63"));

    CHECK_F(parseBrightness("amdgpu,backlight,192,75%,255"), 0.75f,
            0.01f);

    CHECK(parseHibiscusRelease("alpha-v2026.10.19-r19\n") ==
          "alpha-v2026.10.19-r19");
    CHECK(parseHibiscusRelease("VERSION=\"alpha-v2\"\n") == "alpha-v2");
    CHECK(parseHibiscusRelease("VERSION=alpha-v2\njunk\n") == "alpha-v2");
    CHECK(parseHibiscusRelease("bare\nVERSION=alpha-v2\n") == "alpha-v2");
    CHECK(parseHibiscusRelease("").empty());
    CHECK(parseHibiscusRelease("\n\n").empty());

    CHECK(parseCpuModel("processor\t: 0\nmodel name\t: AMD Ryzen 9 7950X\n"
                        "processor\t: 1\n") == "AMD Ryzen 9 7950X");
    CHECK(parseCpuModel("processor\t: 0\nHardware\t: BCM2835\n") ==
          "BCM2835");
    CHECK(parseCpuModel("processor\t: 0\n").empty());

    CHECK(parseMemTotal("MemTotal:       65777756 kB\nMemFree: x\n") ==
          "63 GB");
    CHECK(parseMemTotal("MemTotal:        1048576 kB\n") == "1 GB");
    CHECK(parseMemTotal("MemFree: 1 kB\n").empty());

    CHECK(cleanGpuName("NVIDIA GeForce RTX 3080/PCIe/SSE2") ==
          "NVIDIA GeForce RTX 3080");
    CHECK(cleanGpuName("AMD Radeon RX 7900 (radeonsi, navi31)") ==
          "AMD Radeon RX 7900");
    CHECK(cleanGpuName("llvmpipe") == "llvmpipe");

    CHECK(specLine("CPU", "63 GB", "GPU") == "CPU, 63 GB, GPU");
    CHECK(specLine("CPU", "", "GPU") == "CPU, GPU");
    CHECK(specLine("", "", "").empty());

    std::vector<SetWifi> nets;
    parseWifiList("yes:HomeNet:78:WPA2\n"
                  "no:Cafe\\:Guest:30:\n"
                  "no::12:WPA2\n"
                  "no:OldNet:5:WEP\n",
                  {"HomeNet", "OtherCon"}, &nets);
    CHECK(nets.size() == 3);
    CHECK(nets[0].conn && nets[0].ssid == "HomeNet");
    CHECK(nets[0].sec == WSEC_WPA && nets[0].level == 3);
    CHECK(nets[0].saved == 0);
    CHECK(nets[1].ssid == "Cafe:Guest" && nets[1].sec == WSEC_OPEN);
    CHECK(nets[1].saved == -1);
    CHECK(nets[2].sec == WSEC_WEP && nets[2].level == 0);

    CHECK(parseConNames("HomeNet\nCorp 5G\n\n").size() == 2);

    CHECK(parseBtPowered("\tName: desk\n\tPowered: yes\n"));
    CHECK(!parseBtPowered("\tPowered: no\n"));
    CHECK(!parseBtPowered("No default controller available\n"));

    std::vector<SetBtDev> devs;
    parseBtDevs("Device 3C:01:02:03:04:05 Keyboard K380\n"
                "Device 4C:06:07:08:09:0A MX Master 3S\n"
                "Device 5C:0B:0C:0D:0E:0F\n",
                "Device 3C:01:02:03:04:05 Keyboard K380\n", &devs);
    CHECK(devs.size() == 3);
    CHECK(devs[0].bonded && devs[0].conn);
    CHECK(devs[0].name == "Keyboard K380");
    CHECK(!devs[1].conn && devs[1].name == "MX Master 3S");
    CHECK(devs[2].name.empty() && devs[2].addr == "5C:0B:0C:0D:0E:0F");
}

static void testTaskMatch() {
    CHECK(deskIdBase("org.telegram.desktop.desktop") ==
          "org.telegram.desktop");
    CHECK(deskIdBase("noext") == "noext");
    CHECK(deskIdBase("a.desktop.desktop") == "a.desktop");

    CHECK(appIdMatch("org.telegram.desktop", "TelegramDesktop",
                     "org.telegram.desktop"));
    CHECK(appIdMatch("firefox", "", "firefox"));
    CHECK(appIdMatch("org.foo.Bar.Sub", "", "org.foo.Bar"));
    CHECK(appIdMatch("org.foo", "", "org.foo.Bar"));
    CHECK(!appIdMatch("org.foo.Bar2", "", "org.foo.Bar"));
    CHECK(!appIdMatch("", "TelegramDesktop", "org.telegram.desktop"));
    CHECK(!appIdMatch("org.telegram.desktop", "TelegramDesktop", ""));

    CHECK(spawnPidMatch(123, 122, 123));
    CHECK(spawnPidMatch(456, 123, 123));
    CHECK(!spawnPidMatch(456, 789, 123));
    CHECK(!spawnPidMatch(0, 123, 123));
    CHECK(!spawnPidMatch(456, 123, -1));
}

static void testShellJoin() {
    CHECK(shellJoin({"foo", "--bar", "a b"}) == "'foo' '--bar' 'a b'");
    CHECK(shellJoin({"it's"}) == "'it'\\''s'");
    CHECK(shellJoin({}).empty());
}

static void testTerminalWrap() {
    std::vector<std::string> w = terminalWrap("foot", {"btop"});
    CHECK(w.size() == 3);
    CHECK(w[0] == "foot" && w[1] == "--" && w[2] == "btop");
    w = terminalWrap("xterm", {"htop", "-n", "t"});
    CHECK(w.size() == 5 && w[1] == "-e" && w[4] == "t");
    // an emulator we don't know still wraps, just without a flag
    w = terminalWrap("oddterm", {"t"});
    CHECK(w.size() == 2 && w[0] == "oddterm" && w[1] == "t");
}

static void testFindTerminal() {
    const std::string dir = "/tmp/dash-test-path";
    fs::remove_all(dir);
    putFile(dir + "/foot", "#!/bin/sh\n");
    chmod((dir + "/foot").c_str(), 0755);
    putFile(dir + "/notexec.txt", "x");
    const char* old = getenv("PATH");
    const std::string oldp = old ? old : "";
    setenv("PATH", dir.c_str(), 1);
    CHECK(findTerminal() == "foot");
    setenv("PATH", "", 1);
    CHECK(findTerminal().empty());
    setenv("PATH", oldp.c_str(), 1);
    fs::remove_all(dir);
}

static void testArgFlag() {
    char* a[] = {(char*)"dash", (char*)"-mcp", (char*)"-headless"};
    CHECK(argFlag(3, a, "-mcp"));
    CHECK(argFlag(3, a, "-headless"));
    CHECK(!argFlag(3, a, "-fullscreen"));
    CHECK(!argFlag(3, a, "-mcp2"));
    // argv[0] is the program name, never a flag
    char* b[] = {(char*)"-headless"};
    CHECK(!argFlag(1, b, "-headless"));
}

static void testSpawnEnvAssigns() {
    CHECK(confinedEnvName("DISPLAY"));
    CHECK(confinedEnvName("DBUS_SESSION_BUS_ADDRESS"));
    CHECK(confinedEnvName("WAYLAND_DISPLAY"));
    CHECK(confinedEnvName("GDK_BACKEND"));
    CHECK(!confinedEnvName("HOME"));
    CHECK(!confinedEnvName("PATH"));
    CHECK(!confinedEnvName(""));

    const std::vector<std::string> a = spawnEnvAssigns("wl-4", "");
    auto has = [&](const std::vector<std::string>& v,
                   const std::string& s) {
        for (const std::string& e : v) if (e == s) return true;
        return false;
    };
    CHECK(has(a, "WAYLAND_DISPLAY=wl-4"));
    CHECK(has(a, "DISPLAY="));
    CHECK(has(a, "XAUTHORITY="));
    CHECK(has(a, "DBUS_SESSION_BUS_ADDRESS="));
    CHECK(has(a, "MOZ_ENABLE_WAYLAND=1"));
    CHECK(has(a, "MOZ_DISABLE_WAYLAND="));
    CHECK(has(a, "QT_QPA_PLATFORM=wayland"));
    const std::vector<std::string> b =
        spawnEnvAssigns("wl-9", "unix:abstract=/tmp/dbus-x");
    CHECK(has(b, "DBUS_SESSION_BUS_ADDRESS=unix:abstract=/tmp/dbus-x"));
}

static void testSessionBusAddr() {
    pid_t pid = 0;
    CHECK(sessionBusAddr("unix:abstract=/tmp/dbus-aaa,guid=x\n4321\n",
                         &pid) == "unix:abstract=/tmp/dbus-aaa,guid=x");
    CHECK(pid == 4321);
    CHECK(sessionBusAddr("", &pid).empty());
    CHECK(pid == -1);
    CHECK(sessionBusAddr("not an address\n", &pid) == "not an address");
    CHECK(pid == -1);
    // pid first still parses
    CHECK(sessionBusAddr("77\nunix:path=/run/bus\n", &pid) ==
          "unix:path=/run/bus");
    CHECK(pid == 77);
}

static void testConfineExec() {
    const std::vector<std::string> env = spawnEnvAssigns("wl-2", "");
    // plain argv passes through untouched
    const std::vector<std::string> plain = {"zen", "--new-window"};
    CHECK(confineExec(plain, env) == plain);
    CHECK(confineExec({}, env).empty());

    // env prefix: user assigns kept, forced assigns land before the cmd
    std::vector<std::string> b =
        confineExec({"env", "FOO=bar", "zen"}, env);
    CHECK(b.size() == 3 + env.size());
    CHECK(b[0] == "env" && b[1] == "FOO=bar");
    CHECK(b[2 + env.size()] == "zen");
    // a confined assign in the Exec line gets dropped outright
    b = confineExec({"env", "DISPLAY=:0", "FOO=bar", "zen"}, env);
    CHECK(b.size() == 3 + env.size());
    CHECK(b[1] == "FOO=bar");
    CHECK(b.back() == "zen");
    // -u of a confined var goes too
    b = confineExec({"env", "-u", "WAYLAND_DISPLAY", "app"}, env);
    CHECK(b.size() == 1 + env.size() + 1);
    CHECK(b.back() == "app");
    CHECK(b[1].rfind("WAYLAND_DISPLAY=", 0) == 0);
    // -u of a foreign var survives
    b = confineExec({"env", "-u", "FOO", "app"}, env);
    CHECK(b[1] == "-u" && b[2] == "FOO");
    // -i keeps its meaning and the session vars still reach the cmd
    b = confineExec({"env", "-i", "app"}, env);
    CHECK(b.size() == 3 + env.size());
    CHECK(b[1] == "-i");
    CHECK(b.back() == "app");
    // -S payloads are spliced before the scan
    b = confineExec({"env", "-S", "FOO=bar app --x"}, env);
    CHECK(b[1] == "FOO=bar");
    CHECK(b[b.size() - 2] == "app" && b.back() == "--x");
    // a confined assign inside -S is dropped too
    b = confineExec({"env", "-S", "DISPLAY=:0 app"}, env);
    CHECK(b.back() == "app");
    for (const std::string& s : b) CHECK(s != "DISPLAY=:0");

    // flatpak run gets the wayland socket pinned
    b = confineExec({"flatpak", "run", "app.zen_browser.zen"}, env);
    CHECK(b.size() == 6);
    CHECK(b[0] == "flatpak" && b[1] == "run");
    CHECK(b[2] == "--socket=wayland" && b[3] == "--nosocket=x11");
    CHECK(b[4] == "--nosocket=fallback-x11" &&
          b[5] == "app.zen_browser.zen");
    // socket and env flags that name routing vars are dropped, others
    // stay - even when they sit after the app id
    b = confineExec({"flatpak", "run", "--env=FOO=1", "app.id",
                     "--env=DISPLAY=:0", "--socket=x11"}, env);
    CHECK(b.size() == 5 + 1 + 1);
    CHECK(b[5] == "--env=FOO=1" && b[6] == "app.id");
    for (const std::string& s : b) CHECK(s != "--env=DISPLAY=:0");
    for (const std::string& s : b) CHECK(s != "--socket=x11");
    // args after -- belong to the app and are left alone
    b = confineExec({"flatpak", "run", "app.id", "--",
                     "--env=DISPLAY=:0"}, env);
    CHECK(b.back() == "--env=DISPLAY=:0");
    CHECK(b[b.size() - 2] == "--");
    // split-form option value gets filtered too
    b = confineExec({"flatpak", "run", "--env", "DISPLAY=:0",
                     "app.id"}, env);
    for (const std::string& s : b) CHECK(s != "DISPLAY=:0");
    // not `run`: untouched
    const std::vector<std::string> inst = {"flatpak", "install", "x"};
    CHECK(confineExec(inst, env) == inst);
    // absolute path still matches
    b = confineExec({"/usr/bin/flatpak", "run", "a.b"}, env);
    CHECK(b[2] == "--socket=wayland");
}

static void testSpawnGone() {
    pid_t p = fork();
    if (p == 0) _exit(0);
    usleep(50000);
    CHECK(spawnGone(p));

    // still running: not gone; after kill + group empty: gone
    p = fork();
    if (p == 0) { setsid(); for (;;) usleep(100000); }
    usleep(50000);
    CHECK(!spawnGone(p));
    kill(p, SIGKILL);
    usleep(50000);
    CHECK(spawnGone(p));

    // spawn exits but its process group stays through a grandchild:
    // the slot must wait for the group, not the direct pid
    p = fork();
    if (p == 0) {
        setsid();
        if (fork() == 0) { usleep(300000); _exit(0); }
        _exit(0);
    }
    usleep(80000);
    CHECK(!spawnGone(p));
    usleep(400000);
    CHECK(spawnGone(p));
}

void testDesktop() {
    testDesktopFile();
    testExecArgv();
    testScanDesktopFiles();
    testIndexTheme();
    testFindIconFile();
    testSubdirFromName();
    testIndexlessTheme();
    testConfGroup();
    testSvgIcon();
    testPopupPos();
    testPixconv();
    testSysinfo();
    testTaskMatch();
    testShellJoin();
    testTerminalWrap();
    testFindTerminal();
    testArgFlag();
    testSpawnEnvAssigns();
    testSessionBusAddr();
    testConfineExec();
    testSpawnGone();
}

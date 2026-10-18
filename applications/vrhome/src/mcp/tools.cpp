#include "tools.h"

#include "enc.h"
#include "../common/config.h"
#include "../input/keys.h"

namespace {

Json prop(const char* type, const char* desc) {
    Json p = Json::obj();
    p.set("type", Json::str(type));
    p.set("description", Json::str(desc));
    return p;
}

Json propEnum(std::initializer_list<const char*> vals, const char* desc) {
    Json p = prop("string", desc);
    Json e = Json::arr();
    for (const char* v : vals) e.push(Json::str(v));
    p.set("enum", e);
    return p;
}

Json toolDef(const char* name, const char* desc, const Json& props,
             std::initializer_list<const char*> req = {}) {
    Json t = Json::obj();
    t.set("name", Json::str(name));
    t.set("description", Json::str(desc));
    Json s = Json::obj();
    s.set("type", Json::str("object"));
    s.set("properties", props);
    Json r = Json::arr();
    for (const char* k : req) r.push(Json::str(k));
    if (!r.a.empty()) s.set("required", r);
    t.set("inputSchema", s);
    return t;
}

Json textRes(const std::string& text, bool isErr = false) {
    Json r = Json::obj();
    Json c = Json::arr();
    Json item = Json::obj();
    item.set("type", Json::str("text"));
    item.set("text", Json::str(text));
    c.push(item);
    r.set("content", c);
    if (isErr) r.set("isError", Json::boolean(true));
    return r;
}

Json stateJson(const McpState& s) {
    Json j = Json::obj();
    Json cam = Json::obj();
    cam.set("x", Json::num(s.pos[0]));
    cam.set("y", Json::num(s.pos[1]));
    cam.set("z", Json::num(s.pos[2]));
    cam.set("yawDeg", Json::num(s.yawDeg));
    cam.set("pitchDeg", Json::num(s.pitchDeg));
    j.set("camera", cam);
    j.set("grid", Json::boolean(s.grid));
    j.set("keyboard", Json::boolean(s.kbd));
    j.set("hover", Json::num(s.hover));
    j.set("hoverZone", Json::num(s.hoverZone));
    Json ps = Json::arr();
    for (const McpPanelInfo& p : s.panels) {
        Json pj = Json::obj();
        pj.set("pkg", Json::str(p.pkg));
        pj.set("label", Json::str(p.label));
        pj.set("displayId", Json::num(p.displayId));
        pj.set("taskId", Json::num(p.taskId));
        pj.set("minimized", Json::boolean(p.minimized));
        pj.set("floating", Json::boolean(p.floating));
        pj.set("yawDeg", Json::num(p.yawDeg));
        pj.set("pitchDeg", Json::num(p.pitchDeg));
        pj.set("scale", Json::num(p.scale));
        ps.push(pj);
    }
    j.set("panels", ps);
    return j;
}

bool reqStr(const Json& args, const char* k, std::string* out) {
    const Json* v = args.get(k);
    if (!v || !v->isStr() || v->s.empty()) return false;
    *out = v->s;
    return true;
}

bool reqNum(const Json& args, const char* k, double* out) {
    const Json* v = args.get(k);
    if (!v || !v->isNum()) return false;
    *out = v->n;
    return true;
}

int keyCode(const std::string& name) {
    if (name == "confirm") return kPicoConfirm;
    if (name == "enter") return kKeyEnter;
    if (name == "back") return kKeyBack;
    if (name == "dpad_center") return kKeyDpadCenter;
    if (name == "button_a") return kKeyButtonA;
    return -1;
}

} // namespace

Json mcpToolList() {
    Json tools = Json::arr();

    tools.push(toolDef("dash_state",
        "Snapshot of the dash: camera pose, open panels, grid and "
        "keyboard visibility.",
        Json::obj()));

    {
        Json p = Json::obj();
        p.set("package", prop("string",
            "package name to open, e.g. gitlab.neosalsa.settings"));
        tools.push(toolDef("dash_launch",
            "Launch a package onto a new dash panel.", p, {"package"}));
    }

    {
        Json p = Json::obj();
        p.set("key", propEnum(
            {"confirm", "enter", "back", "dpad_center", "button_a"},
            "key to send; confirm is the headset click at the gaze point"));
        p.set("action", propEnum({"tap", "down", "up"},
            "tap sends down then up; default tap"));
        tools.push(toolDef("dash_key", "Send a key event to the dash.",
                           p, {"key"}));
    }

    {
        Json p = Json::obj();
        p.set("yaw", prop("number", "camera yaw in degrees"));
        p.set("pitch", prop("number", "camera pitch in degrees"));
        tools.push(toolDef("dash_look",
            "Point the preview camera. The gaze ray follows it.", p,
            {"yaw", "pitch"}));
    }

    {
        Json p = Json::obj();
        p.set("x", prop("number", "camera x in metres"));
        p.set("y", prop("number", "camera y in metres"));
        p.set("z", prop("number", "camera z in metres"));
        tools.push(toolDef("dash_move",
            "Move the preview camera to an absolute position.", p,
            {"x", "y", "z"}));
    }

    tools.push(toolDef("dash_recenter",
        "Re-anchor the dash ring in front of the camera.", Json::obj()));

    {
        Json p = Json::obj();
        p.set("shown", prop("boolean", "true to open, false to close"));
        tools.push(toolDef("dash_grid",
            "Show or hide the app-grid overlay.", p, {"shown"}));
    }

    {
        Json p = Json::obj();
        p.set("delta", prop("number",
            "wheel detents; positive scrolls further down the list"));
        tools.push(toolDef("dash_scroll",
            "Scroll the app-grid overlay, like a wheel turn.", p,
            {"delta"}));
    }

    {
        Json p = Json::obj();
        p.set("shown", prop("boolean", "true to show, false to hide"));
        tools.push(toolDef("dash_keyboard",
            "Show or hide the floating keyboard quad.", p, {"shown"}));
    }

    {
        Json p = Json::obj();
        p.set("shown", prop("boolean", "true to show, false to hide"));
        tools.push(toolDef("dash_quick",
            "Show or hide the quick-settings card.", p, {"shown"}));
    }

    tools.push(toolDef("dash_screenshot",
        "Capture the dash window; returns a PNG image.", Json::obj()));

    return tools;
}

bool mcpToolCall(McpDash* d, const std::string& name, const Json& args,
                 Json* result, std::string* errMsg) {
    if (!args.isNull() && !args.isObj()) {
        *errMsg = "arguments must be an object";
        return false;
    }
    const Json a = args.isObj() ? args : Json::obj();

    if (name == "dash_state") {
        McpState st;
        if (!d->state(&st)) {
            *result = textRes("dash state unavailable", true);
            return true;
        }
        const Json j = stateJson(st);
        *result = textRes(jsonWrite(j));
        result->set("structuredContent", j);
        return true;
    }
    if (name == "dash_launch") {
        std::string pkg;
        if (!reqStr(a, "package", &pkg)) {
            *errMsg = "package is required";
            return false;
        }
        d->launch(pkg);
        *result = textRes("launched " + pkg);
        return true;
    }
    if (name == "dash_key") {
        std::string kn;
        if (!reqStr(a, "key", &kn)) {
            *errMsg = "key is required";
            return false;
        }
        const int code = keyCode(kn);
        if (code < 0) {
            *errMsg = "unknown key: " + kn;
            return false;
        }
        const std::string act =
            a.get("action") ? a.get("action")->asStr("tap") : "tap";
        if (act == "tap") {
            d->key(code, kMcpDown);
            d->key(code, kMcpUp);
        } else if (act == "down") {
            d->key(code, kMcpDown);
        } else if (act == "up") {
            d->key(code, kMcpUp);
        } else {
            *errMsg = "action must be tap, down or up";
            return false;
        }
        *result = textRes("sent " + kn + " " + act);
        return true;
    }
    if (name == "dash_look") {
        double yaw, pitch;
        if (!reqNum(a, "yaw", &yaw) || !reqNum(a, "pitch", &pitch)) {
            *errMsg = "yaw and pitch are required";
            return false;
        }
        d->look((float)yaw, (float)pitch);
        *result = textRes("looking " + std::to_string(yaw) + "," +
                          std::to_string(pitch));
        return true;
    }
    if (name == "dash_move") {
        double x, y, z;
        if (!reqNum(a, "x", &x) || !reqNum(a, "y", &y) ||
            !reqNum(a, "z", &z)) {
            *errMsg = "x, y and z are required";
            return false;
        }
        d->move((float)x, (float)y, (float)z);
        *result = textRes("moved");
        return true;
    }
    if (name == "dash_recenter") {
        d->recenter();
        *result = textRes("recentered");
        return true;
    }
    if (name == "dash_scroll") {
        double delta;
        if (!reqNum(a, "delta", &delta)) {
            *errMsg = "delta is required";
            return false;
        }
        d->scroll((float)delta);
        *result = textRes("scrolled");
        return true;
    }
    if (name == "dash_grid" || name == "dash_keyboard" ||
            name == "dash_quick") {
        const Json* v = a.get("shown");
        if (!v || !v->isBool()) {
            *errMsg = "shown is required";
            return false;
        }
        if (name == "dash_grid") d->setGrid(v->b);
        else if (name == "dash_quick") d->setQuick(v->b);
        else d->setKbd(v->b);
        *result = textRes(v->b ? "shown" : "hidden");
        return true;
    }
    if (name == "dash_screenshot") {
        std::vector<unsigned char> px;
        int w = 0, h = 0;
        if (!d->screenshot(&px, &w, &h) || w <= 0 || h <= 0) {
            *result = textRes("no frame captured", true);
            return true;
        }
        const std::vector<unsigned char> png =
            pngEncodeRgba(px.data(), w, h);
        if (png.empty()) {
            *result = textRes("png encode failed", true);
            return true;
        }
        Json img = Json::obj();
        img.set("type", Json::str("image"));
        img.set("data", Json::str(b64encode(png.data(), png.size())));
        img.set("mimeType", Json::str("image/png"));
        Json c = Json::arr();
        c.push(img);
        Json r = Json::obj();
        r.set("content", c);
        *result = r;
        return true;
    }
    *errMsg = "unknown tool: " + name;
    return false;
}

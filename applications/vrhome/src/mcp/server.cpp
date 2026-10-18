#include "server.h"

#include "json.h"
#include "tools.h"

namespace {

constexpr const char* kProto = "2024-11-05";

Json reply(const Json& id) {
    Json r = Json::obj();
    r.set("jsonrpc", Json::str("2.0"));
    r.set("id", id);
    return r;
}

std::string errLine(const Json& id, int code, const std::string& msg) {
    Json r = reply(id);
    Json e = Json::obj();
    e.set("code", Json::num(code));
    e.set("message", Json::str(msg));
    r.set("error", e);
    return jsonWrite(r);
}

Json initResult(const Json& params) {
    const Json* pv = params.get("protocolVersion");
    Json r = Json::obj();
    r.set("protocolVersion",
          pv && pv->isStr() && !pv->s.empty() ? *pv : Json::str(kProto));
    Json caps = Json::obj();
    caps.set("tools", Json::obj());
    r.set("capabilities", caps);
    Json info = Json::obj();
    info.set("name", Json::str("hibiscus-dash-preview"));
    info.set("version", Json::str("1.0.0"));
    r.set("serverInfo", info);
    r.set("instructions", Json::str(
        "Dash desktop preview. dash_state reports camera and panels; "
        "dash_look, dash_move, dash_key and dash_launch drive it; "
        "dash_screenshot returns the rendered frame."));
    return r;
}

} // namespace

std::string McpServer::handleLine(const std::string& line) {
    bool blank = true;
    for (char c : line)
        if (c != ' ' && c != '\t' && c != '\r') { blank = false; break; }
    if (blank) return "";

    Json req;
    if (!jsonParse(line, &req))
        return errLine(Json(), -32700, "Parse error");
    const Json* idp = req.get("id");
    const Json id = idp ? *idp : Json();
    if (!req.isObj())
        return errLine(id, -32600, "Invalid Request");
    const Json* mp = req.get("method");
    if (!mp || !mp->isStr())
        return errLine(id, -32600, "Invalid Request");
    const std::string& method = mp->s;

    if (!idp) {
        // notifications: valid but never answered
        return "";
    }

    const Json* pp = req.get("params");
    const Json params = (pp && pp->isObj()) ? *pp : Json::obj();

    Json r = reply(id);
    if (method == "initialize") {
        r.set("result", initResult(params));
    } else if (method == "ping") {
        r.set("result", Json::obj());
    } else if (method == "tools/list") {
        Json res = Json::obj();
        res.set("tools", mcpToolList());
        r.set("result", res);
    } else if (method == "tools/call") {
        const Json* np = params.get("name");
        if (!np || !np->isStr())
            return errLine(id, -32602, "params.name is required");
        const Json* ap = params.get("arguments");
        Json result;
        std::string errMsg;
        if (!mcpToolCall(dash, np->s, ap ? *ap : Json(), &result, &errMsg))
            return errLine(id, -32602, errMsg);
        r.set("result", result);
    } else if (method == "resources/list") {
        Json res = Json::obj();
        res.set("resources", Json::arr());
        r.set("result", res);
    } else if (method == "resources/templates/list") {
        Json res = Json::obj();
        res.set("resourceTemplates", Json::arr());
        r.set("result", res);
    } else if (method == "prompts/list") {
        Json res = Json::obj();
        res.set("prompts", Json::arr());
        r.set("result", res);
    } else if (method == "logging/setLevel") {
        r.set("result", Json::obj());
    } else {
        return errLine(id, -32601, "Method not found");
    }
    return jsonWrite(r);
}

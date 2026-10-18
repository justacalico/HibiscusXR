#include "test.h"

#include "mcp/json.h"
#include "mcp/enc.h"
#include "mcp/server.h"
#include "mcp/tools.h"
#include "input/keys.h"
#include "common/config.h"

#include <cstring>
#include <zlib.h>

class FakeDash : public McpDash {
public:
    std::vector<std::string> log;
    McpState st;
    bool stateOk = true, shotOk = true;

    void launch(const std::string& pkg) override {
        log.push_back("launch " + pkg);
    }
    void key(int code, int action) override {
        log.push_back("key " + std::to_string(code) + " " +
                      std::to_string(action));
    }
    void look(float yawDeg, float pitchDeg) override {
        st.yawDeg = yawDeg; st.pitchDeg = pitchDeg;
        log.push_back("look");
    }
    void move(float x, float y, float z) override {
        st.pos[0] = x; st.pos[1] = y; st.pos[2] = z;
        log.push_back("move");
    }
    void recenter() override { log.push_back("recenter"); }
    void setGrid(bool shown) override {
        st.grid = shown;
        log.push_back("grid " + std::to_string(shown));
    }
    void setKbd(bool shown) override {
        st.kbd = shown;
        log.push_back("kbd " + std::to_string(shown));
    }
    void scroll(float delta) override {
        log.push_back("scroll " + std::to_string(delta));
    }
    bool state(McpState* out) override {
        if (!stateOk) return false;
        *out = st;
        return true;
    }
    bool screenshot(std::vector<unsigned char>* rgba,
                    int* w, int* h) override {
        if (!shotOk) return false;
        *w = 2; *h = 1;
        rgba->assign({255, 0, 0, 255, 0, 255, 0, 255});
        return true;
    }
};

static Json parse(const std::string& line) {
    Json j;
    CHECK(jsonParse(line, &j));
    return j;
}

void testMcp() {
    // json: values, escapes, nesting, strict rejects
    {
        Json j;
        CHECK(jsonParse("{\"a\":1,\"b\":[true,null,\"x\\ny\"],\"c\":-2.5}",
                        &j));
        CHECK(j.isObj());
        CHECK(j.get("a")->n == 1.0);
        CHECK(j.get("b")->a.size() == 3);
        CHECK(j.get("b")->a[0].b && j.get("b")->a[1].t == Json::Null);
        CHECK(j.get("b")->a[2].s == "x\ny");
        CHECK(j.get("c")->n == -2.5);
        CHECK(j.get("missing") == nullptr);
        CHECK(jsonParse("\"\\u0041\\ud83d\\ude00\"", &j));
        CHECK(j.s == "A\xF0\x9F\x98\x80");
        CHECK(!jsonParse("{", &j));
        CHECK(!jsonParse("[1,]", &j));
        CHECK(!jsonParse("1 2", &j));
        CHECK(!jsonParse("\"\\ud800x\"", &j));
        CHECK(!jsonParse("nul", &j));
    }
    // json writer round-trips and escapes
    {
        Json j = Json::obj();
        j.set("k", Json::str("a\"b\nc"));
        j.set("n", Json::num(1.5));
        Json arr = Json::arr();
        arr.push(Json::num(1));
        arr.push(Json::boolean(false));
        j.set("l", arr);
        const std::string w = jsonWrite(j);
        Json back;
        CHECK(jsonParse(w, &back));
        CHECK(back.get("k")->s == "a\"b\nc");
        CHECK(back.get("n")->n == 1.5);
        CHECK(back.get("l")->a.size() == 2);
        CHECK(w.find("\\\"") != std::string::npos);
        CHECK(w.find("\\n") != std::string::npos);
    }
    // base64: rfc vectors
    {
        CHECK(b64encode((const unsigned char*)"", 0).empty());
        CHECK(b64encode((const unsigned char*)"f", 1) == "Zg==");
        CHECK(b64encode((const unsigned char*)"fo", 2) == "Zm8=");
        CHECK(b64encode((const unsigned char*)"foo", 3) == "Zm9v");
        CHECK(b64encode((const unsigned char*)"foob", 4) == "Zm9vYg==");
        CHECK(b64encode((const unsigned char*)"foobar", 6) == "Zm9vYmFy");
    }
    // png: signature, IHDR, IDAT inflates back to the pixels
    {
        const unsigned char px[8] = {9, 8, 7, 255, 6, 5, 4, 255};
        const std::vector<unsigned char> png = pngEncodeRgba(px, 2, 1);
        CHECK(png.size() > 40);
        CHECK(!memcmp(png.data(), "\x89PNG\r\n\x1a\n", 8));
        CHECK(png[16] == 0 && png[17] == 0 && png[18] == 0 &&
              png[19] == 2);                    // IHDR width
        CHECK(png[20] == 0 && png[23] == 1);    // IHDR height
        CHECK(png[24] == 8 && png[25] == 6);    // 8-bit RGBA
        const size_t idatLen = ((size_t)png[33] << 24) | (png[34] << 16) |
                               (png[35] << 8) | png[36];
        CHECK(!memcmp(png.data() + 37, "IDAT", 4));
        unsigned char raw[16] = {};
        uLongf rawLen = sizeof(raw);
        CHECK(uncompress(raw, &rawLen, png.data() + 41,
                         (uLong)idatLen) == Z_OK);
        CHECK(rawLen == 9);                     // filter byte + 2px
        CHECK(raw[0] == 0 && !memcmp(raw + 1, px, 8));
        CHECK(!memcmp(png.data() + png.size() - 8, "IEND", 4));
    }
    // protocol: handshake, ping, list, notification, errors
    {
        FakeDash d;
        McpServer srv(&d);
        std::string out = srv.handleLine(
            "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\","
            "\"params\":{\"protocolVersion\":\"2024-11-05\"}}");
        Json r = parse(out);
        CHECK(r.get("id")->n == 1);
        const Json* res = r.get("result");
        CHECK(res->get("protocolVersion")->s == "2024-11-05");
        CHECK(res->get("serverInfo")->get("name")->s ==
              "hibiscus-dash-preview");
        CHECK(res->get("capabilities")->get("tools") != nullptr);

        r = parse(srv.handleLine(
            "{\"jsonrpc\":\"2.0\",\"id\":\"x\",\"method\":\"ping\"}"));
        CHECK(r.get("id")->s == "x");
        CHECK(r.get("result")->isObj());

        r = parse(srv.handleLine(
            "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/list\"}"));
        const Json* tools = r.get("result")->get("tools");
        CHECK(tools->a.size() == 10);
        CHECK(tools->a[0].get("name")->s == "dash_state");
        CHECK(tools->a[9].get("name")->s == "dash_screenshot");

        CHECK(srv.handleLine(
            "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/"
            "initialized\"}").empty());
        CHECK(srv.handleLine("").empty());
        CHECK(srv.handleLine("   ").empty());

        r = parse(srv.handleLine("not json"));
        CHECK(r.get("error")->get("code")->n == -32700);
        CHECK(r.get("id")->t == Json::Null);
        r = parse(srv.handleLine(
            "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"nope\"}"));
        CHECK(r.get("error")->get("code")->n == -32601);
        r = parse(srv.handleLine("{\"id\":4}"));
        CHECK(r.get("error")->get("code")->n == -32600);
    }
    // tools/call: dispatch, args, isError
    {
        FakeDash d;
        McpServer srv(&d);
        auto call = [&](const char* inner) {
            return parse(srv.handleLine(std::string(
                "{\"jsonrpc\":\"2.0\",\"id\":9,\"method\":\"tools/call\","
                "\"params\":{") + inner + "}}"));
        };
        Json r = call("\"name\":\"dash_launch\",\"arguments\":{\"package\":\"a.b\"}");
        CHECK(r.get("result") != nullptr);
        CHECK(d.log.size() == 1 && d.log[0] == "launch a.b");

        r = call("\"name\":\"dash_key\",\"arguments\":{\"key\":\"confirm\"}");
        CHECK(d.log.size() == 3);
        CHECK(d.log[1] == "key " + std::to_string(kPicoConfirm) + " " +
                          std::to_string(kMcpDown));
        CHECK(d.log[2] == "key " + std::to_string(kPicoConfirm) + " " +
                          std::to_string(kMcpUp));

        r = call("\"name\":\"dash_key\",\"arguments\":{\"key\":\"back\",\"action\":\"down\"}");
        CHECK(d.log.back() == "key " + std::to_string(kKeyBack) + " " +
                              std::to_string(kMcpDown));

        r = call("\"name\":\"dash_key\",\"arguments\":{\"key\":\"nope\"}");
        CHECK(r.get("error")->get("code")->n == -32602);
        r = call("\"name\":\"dash_look\",\"arguments\":{\"yaw\":45}");
        CHECK(r.get("error")->get("code")->n == -32602);
        r = call("\"name\":\"dash_look\",\"arguments\":{\"yaw\":45,\"pitch\":-10}");
        CHECK(d.st.yawDeg == 45.0f && d.st.pitchDeg == -10.0f);
        r = call("\"name\":\"dash_move\",\"arguments\":{\"x\":1,\"y\":2,\"z\":3}");
        CHECK(d.st.pos[0] == 1 && d.st.pos[2] == 3);
        r = call("\"name\":\"dash_grid\",\"arguments\":{\"shown\":true}");
        CHECK(d.st.grid);
        r = call("\"name\":\"dash_keyboard\",\"arguments\":{\"shown\":false}");
        CHECK(!d.st.kbd);
        r = call("\"name\":\"dash_recenter\"");
        CHECK(d.log.back() == "recenter");

        r = call("\"name\":\"dash_scroll\",\"arguments\":{\"delta\":2}");
        CHECK(d.log.back() == "scroll 2.000000");
        r = call("\"name\":\"dash_scroll\"");
        CHECK(r.get("error")->get("code")->n == -32602);

        r = call("\"name\":\"dash_state\"");
        const Json* sc = r.get("result")->get("structuredContent");
        CHECK(sc != nullptr);
        CHECK(sc->get("camera")->get("yawDeg")->n == 45);
        CHECK(sc->get("grid")->b);

        r = call("\"name\":\"dash_screenshot\"");
        const Json* c = r.get("result")->get("content")->a.data();
        CHECK(c[0].get("type")->s == "image");
        CHECK(c[0].get("mimeType")->s == "image/png");
        CHECK(c[0].get("data")->s.rfind("iVBORw0KGgo", 0) == 0);

        r = call("\"name\":\"dash_nope\"");
        CHECK(r.get("error")->get("code")->n == -32602);
        r = call("\"name\":\"dash_launch\"");
        CHECK(r.get("error")->get("code")->n == -32602);

        d.shotOk = false;
        r = call("\"name\":\"dash_screenshot\"");
        CHECK(r.get("result")->get("isError")->b);
    }
}

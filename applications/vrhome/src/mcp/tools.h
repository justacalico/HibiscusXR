#pragma once

#include <string>
#include <vector>

#include "json.h"

struct McpPanelInfo {
    int displayId = -1, taskId = -1;
    std::string pkg, label;
    bool minimized = false, floating = false;
    float yawDeg = 0.0f, pitchDeg = 0.0f, scale = 1.0f;
};

struct McpState {
    float pos[3] = {0.0f, 0.0f, 0.0f};
    float yawDeg = 0.0f, pitchDeg = 0.0f;
    int hover = -1;
    int hoverZone = 0;
    bool grid = false, kbd = false, settings = false;
    std::vector<McpPanelInfo> panels;
};

// key action values, mirroring kKeyDown/kKeyUp in shell/backend.h -
// kept local so this header stays free of the dash backend interface
constexpr int kMcpDown = 0;
constexpr int kMcpUp = 1;

// What the MCP tools need from the running dash. The preview implements
// this by posting onto the render loop, so every call is synchronous and
// sees one consistent frame.

class McpDash {
public:
    virtual ~McpDash() = default;
    virtual void launch(const std::string& pkg) = 0;
    virtual void key(int code, int action) = 0;
    virtual void look(float yawDeg, float pitchDeg) = 0;
    virtual void move(float x, float y, float z) = 0;
    virtual void recenter() = 0;
    virtual void setGrid(bool shown) = 0;
    virtual void setKbd(bool shown) = 0;
    virtual void setQuick(bool shown) = 0;
    virtual void setSettings(bool shown) = 0;
    virtual void scroll(float delta) = 0;
    virtual bool state(McpState* out) = 0;
    virtual bool screenshot(std::vector<unsigned char>* rgba,
                            int* w, int* h) = 0;
};

Json mcpToolList();

// Runs one tools/call. On success fills result with the MCP result object
// ({content: [...]}). Returns false with errMsg set for a JSON-RPC
// -32602 reply: unknown tool or invalid arguments.
bool mcpToolCall(McpDash* dash, const std::string& name, const Json& args,
                 Json* result, std::string* errMsg);

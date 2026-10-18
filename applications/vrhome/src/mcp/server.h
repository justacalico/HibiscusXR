#pragma once

#include <string>

class McpDash;

// MCP server on the stdio transport: one JSON-RPC message per line.
// handleLine returns the response line, or "" for notifications and
// blank input.
class McpServer {
public:
    explicit McpServer(McpDash* dash) : dash(dash) {}
    std::string handleLine(const std::string& line);

private:
    McpDash* dash;
};

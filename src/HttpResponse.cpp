#include "HttpResponse.h"
#include <sstream>

std::string HttpResponse::get_status_message(StatusCode code) {
    switch (code) {
        case OK: return "OK";
        case BAD_REQUEST: return "Bad Request";
        case NOT_FOUND: return "Not Found";
        case INTERNAL_ERROR: return "Internal Server Error";
        default: return "Unknown";
    }
}

std::unordered_map<std::string, std::string> HttpResponse::get_default_headers(bool keep_alive) {
    std::unordered_map<std::string, std::string> headers;
    headers["Server"] = "High-Performance-HTTP-Server/1.0";
    headers["Connection"] = keep_alive ? "keep-alive" : "close";
    if (keep_alive) {
        headers["Keep-Alive"] = "timeout=5, max=1000";
    }
    return headers;
}

std::string HttpResponse::build_response(StatusCode code, const std::string& body, bool keep_alive) {
    std::ostringstream oss;
    
    // Status line
    oss << "HTTP/1.1 " << code << " " << get_status_message(code) << "\r\n";
    
    // Headers
    auto headers = get_default_headers(keep_alive);
    headers["Content-Length"] = std::to_string(body.size());
    headers["Content-Type"] = "text/html; charset=utf-8";
    
    for (const auto& header : headers) {
        oss << header.first << ": " << header.second << "\r\n";
    }
    
    oss << "\r\n";
    oss << body;
    
    return oss.str();
}

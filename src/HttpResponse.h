#pragma once

#include <string>
#include <unordered_map>

/**
 * Gestionnaire de réponses HTTP
 */
class HttpResponse {
public:
    enum StatusCode {
        OK = 200,
        BAD_REQUEST = 400,
        NOT_FOUND = 404,
        INTERNAL_ERROR = 500
    };

    static std::string build_response(StatusCode code, const std::string& body = "", bool keep_alive = true);
    static std::string get_status_message(StatusCode code);

private:
    static std::unordered_map<std::string, std::string> get_default_headers(bool keep_alive);
};

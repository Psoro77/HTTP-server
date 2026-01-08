#pragma once

#include <string>
#include <unordered_map>
#include <sstream>

/**
 * Représentation d'une requête HTTP
 */
class HttpRequest {
public:
    std::string method;
    std::string path;
    std::string version;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    bool keep_alive = false;

    // Parser une requête HTTP depuis un buffer
    static bool parse(const std::string& raw_request, HttpRequest& req);
    
    // Obtenir une valeur de header
    std::string get_header(const std::string& key) const;
};

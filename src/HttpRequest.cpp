#include "HttpRequest.h"
#include <algorithm>
#include <cctype>

bool HttpRequest::parse(const std::string& raw_request, HttpRequest& req) {
    std::istringstream stream(raw_request);
    std::string line;
    
    // Parser la ligne de requête
    if (!std::getline(stream, line)) {
        return false;
    }
    
    // Enlever \r si présent
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
    
    std::istringstream request_line(line);
    if (!(request_line >> req.method >> req.path >> req.version)) {
        return false;
    }
    
    // Parser les headers
    while (std::getline(stream, line) && line != "\r" && line != "") {
        if (line.back() == '\r') {
            line.pop_back();
        }
        
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 1);
            
            // Trim
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            // Convertir en lowercase pour les headers
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            req.headers[key] = value;
        }
    }
    
    // Vérifier keep-alive
    std::string connection = req.get_header("connection");
    std::transform(connection.begin(), connection.end(), connection.begin(), ::tolower);
    req.keep_alive = (connection == "keep-alive") || 
                     (req.version == "HTTP/1.1" && connection != "close");
    
    return true;
}

std::string HttpRequest::get_header(const std::string& key) const {
    auto it = headers.find(key);
    if (it != headers.end()) {
        return it->second;
    }
    return "";
}

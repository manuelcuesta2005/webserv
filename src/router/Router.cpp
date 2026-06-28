#include "router/Router.hpp"

const LocationConfig* matchLocation(const ServerConfig& server, const std::string& uri) {
    const LocationConfig* best = NULL;
    size_t bestLength = 0;
    for (std::vector<LocationConfig>::const_iterator it = server.locations.begin();
         it != server.locations.end(); ++it) {
        if (uri.compare(0, it->path.length(), it->path) == 0) {
            bool exactMatch = (uri.length() == it->path.length());
            bool slashAfter = (uri.length() > it->path.length() && uri[it->path.length()] == '/');
            bool isRoot = (it->path == "/");
            if ((exactMatch || slashAfter || isRoot) && it->path.length() > bestLength) {
                best = &(*it);
                bestLength = it->path.length();
            }
        }
    }
    return best;
}

bool isMethodAllowed(const LocationConfig* location, const std::string& method) {
    if (location->allowed_methods.empty()) {
        return method == "GET";
    }
    return location->allowed_methods.find(method) != location->allowed_methods.end();
}

std::string resolvePath(const LocationConfig* location, const std::string& uri) {
    std::string remainder = uri.substr(location->path.length());
    if (!remainder.empty() && remainder[0] != '/') {
        remainder = "/" + remainder;
    }
    return location->root + remainder;
}
#include "http/HttpRequestParser.hpp"

static std::string toLower(const std::string& str) {
    std::string result = str;
    for (size_t i = 0; i < result.length(); i++) {
        result[i] = tolower(result[i]);
    }
    return result;
}

ParseResult feedData(HttpRequest& req, const char* data, size_t len) {
    req.buffer.append(data, len);

    while (req.state != COMPLETE && req.state != ERROR_STATE) {
        if (req.state == READING_REQUEST_LINE) {
            ParseResult r = parseRequestLine(req);
            if (r != PARSE_COMPLETE) return r;
        } else if (req.state == READING_HEADERS) {
            ParseResult r = parseHeaders(req);
            if (r != PARSE_COMPLETE) return r;
        } else if (req.state == READING_BODY) {
            ParseResult r = parseBody(req);
            if (r != PARSE_COMPLETE) return r;
        }
    }
    if (req.state == ERROR_STATE) return PARSE_ERROR;
    return PARSE_COMPLETE;
}

ParseResult parseRequestLine(HttpRequest& req) {
    size_t pos = req.buffer.find("\r\n");
    if (pos == std::string::npos) {
        return PARSE_INCOMPLETE;
    }

    std::string line = req.buffer.substr(0, pos);

    size_t firstSpace = line.find(' ');
    if (firstSpace == std::string::npos) {
        req.state = ERROR_STATE;
        return PARSE_ERROR;
    }

    size_t secondSpace = line.find(' ', firstSpace + 1);
    if (secondSpace == std::string::npos) {
        req.state = ERROR_STATE;
        return PARSE_ERROR;
    }

    std::string method  = line.substr(0, firstSpace);
    std::string uri     = line.substr(firstSpace + 1, secondSpace - firstSpace - 1);
    std::string version = line.substr(secondSpace + 1);

    if (method != "GET" && method != "POST" && method != "DELETE") {
        req.state = ERROR_STATE;
        return PARSE_ERROR;
    }

    if (uri.empty() || uri[0] != '/') {
        req.state = ERROR_STATE;
        return PARSE_ERROR;
    }

    if (version != "HTTP/1.1" && version != "HTTP/1.0") {
        req.state = ERROR_STATE;
        return PARSE_ERROR;
    }

    req.method  = method;
    req.uri     = uri;
    req.version = version;

    req.buffer.erase(0, pos + 2);
    req.state = READING_HEADERS;
    return PARSE_COMPLETE;
}

ParseResult parseHeaders(HttpRequest& req) {
    while (true) {
        size_t pos = req.buffer.find("\r\n");
        if (pos == std::string::npos) {
            return PARSE_INCOMPLETE;
        }

        if (pos == 0) {
            req.buffer.erase(0, 2);

            if (req.headers.find("content-length") != req.headers.end()) {
                req.bodyType = BODY_CONTENT_LENGTH;
                req.contentLength = std::atoi(req.headers["content-length"].c_str());
                req.state = READING_BODY;
            } else if (req.headers.find("transfer-encoding") != req.headers.end()) {
                req.bodyType = BODY_CHUNKED;
                req.state = READING_BODY;
            } else {
                req.bodyType = BODY_NONE;
                req.state = COMPLETE;
            }
            break;
        }

        std::string line = req.buffer.substr(0, pos);

        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            req.state = ERROR_STATE;
            return PARSE_ERROR;
        }

        std::string name  = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);

        size_t start = 0;
        while (start < value.length() && value[start] == ' ') {
            start++;
        }
        value = value.substr(start);

        req.headers[toLower(name)] = value;

        req.buffer.erase(0, pos + 2);
    }
    return PARSE_COMPLETE;
}

ParseResult parseBody(HttpRequest& req) {
    if (req.bodyType == BODY_CONTENT_LENGTH) {
        if (req.buffer.length() < req.contentLength) {
            return PARSE_INCOMPLETE;
        }
        req.body = req.buffer.substr(0, req.contentLength);
        req.buffer.erase(0, req.contentLength);
        req.state = COMPLETE;
        return PARSE_COMPLETE;
    }

    if (req.bodyType == BODY_CHUNKED) {
        while (true) {
            size_t pos = req.buffer.find("\r\n");
            if (pos == std::string::npos) {
                return PARSE_INCOMPLETE;
            }

            std::string sizeLine = req.buffer.substr(0, pos);
            size_t chunkSize = strtoul(sizeLine.c_str(), NULL, 16);

            if (req.buffer.length() < pos + 2 + chunkSize + 2) {
                return PARSE_INCOMPLETE;
            }

            if (chunkSize == 0) {
                req.buffer.erase(0, pos + 2 + 2);
                req.state = COMPLETE;
                return PARSE_COMPLETE;
            }

            req.body += req.buffer.substr(pos + 2, chunkSize);
            req.buffer.erase(0, pos + 2 + chunkSize + 2);
        }
    }

    req.state = ERROR_STATE;
    return PARSE_ERROR;
}
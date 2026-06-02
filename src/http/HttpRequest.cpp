#include "HttpRequest.hpp"

HttpRequest::HttpRequest() {
    reset();
}

void HttpRequest::reset() {
    state         = READING_REQUEST_LINE;
    contentLength = 0;
    bodyType      = BODY_NONE;
    buffer.clear();
    method.clear();
    uri.clear();
    version.clear();
    headers.clear();
    body.clear();
}
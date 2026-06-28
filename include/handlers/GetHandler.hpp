#ifndef GET_HANDLER_HPP
#define GET_HANDLER_HPP

#include "webserv.hpp"
#include "parser/ConfigTypes.hpp"
#include "handlers/HandlerResult.hpp"

HandlerResult handleGet(const LocationConfig* location, const std::string& fullPath);

#endif
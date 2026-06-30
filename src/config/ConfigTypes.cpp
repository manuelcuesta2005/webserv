#include "webserv.hpp"

// -------------------------------------------------------------
//  Constructores con valores por defecto sensatos.
//  Estos structs son contenedores de datos: el parser los va
//  rellenando, y el resto del programa los consulta en modo
//  solo-lectura.
// -------------------------------------------------------------

LocationConfig::LocationConfig()
    : path()
    , allowed_methods()
    , redirect_code(0)             // 0 = sin redirect
    , redirect_target()
    , root()
    , autoindex(false)
    , index_files()
    , upload_enabled(false)
    , upload_store()
    , requires_auth(false)
    , cgi_handlers()
    , client_max_body_size(-1)     // -1 = hereda del server
{}


ListenEndpoint::ListenEndpoint()
    : host(), port(0)
{}

ListenEndpoint::ListenEndpoint(const std::string& h, uint16_t p)
    : host(h), port(p)
{}


ServerConfig::ServerConfig()
    : listen()
    , server_names()
    , error_pages()
    , client_max_body_size(1048576)   // 1 MB por defecto si no se especifica
    , locations()
{}

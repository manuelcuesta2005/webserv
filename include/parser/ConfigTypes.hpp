#ifndef CONFIG_TYPES_HPP
# define CONFIG_TYPES_HPP

# include <string>
# include <vector>
# include <map>
# include <set>
# include <stdint.h>

// =============================================================
//  ConfigTypes.hpp
//  Estructuras de datos que produce el parser de configuración.
//  Todo el resto del servidor (event loop, request handler, CGI...git)
//  consulta estas estructuras en modo solo-lectura.
// =============================================================


// -------------------------------------------------------------
//  LocationConfig
//  Reglas para una ruta concreta (ej. "/", "/uploads", "/cgi-bin")
// -------------------------------------------------------------
struct LocationConfig
{
    // Ruta del bloque location (ej. "/", "/images", "/cgi-bin")
    std::string                 path;

    // Métodos HTTP permitidos en esta ruta.
    // Si está vacío -> usar default (típicamente solo GET).
    std::set<std::string>       allowed_methods;

    // Redirección HTTP. Si redirect_code != 0, el servidor responde
    // con ese código y Location: redirect_target, sin servir nada más.
    int                         redirect_code;     // 0 = sin redirect
    std::string                 redirect_target;

    // Directorio raíz donde buscar archivos para esta location.
    // Ej: location /kapouet con root /tmp/www -> /kapouet/x/y busca /tmp/www/x/y
    std::string                 root;

    // Listado automático del contenido de un directorio si no hay index.
    bool                        autoindex;         // default false

    // Archivo(s) por defecto cuando se pide un directorio.
    // Se prueban en orden hasta encontrar uno que exista.
    std::vector<std::string>    index_files;

    // Subida de archivos
    bool                        upload_enabled;    // default false
    std::string                 upload_store;      // ruta donde guardar uploads
    bool                        requires_auth;     // default false

    // CGI: mapa extensión -> intérprete
    // Ej: ".php" -> "/usr/bin/php-cgi", ".py" -> "/usr/bin/python3"
    std::map<std::string, std::string>   cgi_handlers;

    // Tamaño máximo del cuerpo de la request, heredable del server.
    // -1 significa "no definido aquí, usar el del server".
    long                        client_max_body_size;

    LocationConfig();
};


// -------------------------------------------------------------
//  ListenEndpoint
//  Un par host:port donde el server escucha.
// -------------------------------------------------------------
struct ListenEndpoint
{
    std::string     host;       // "0.0.0.0", "127.0.0.1", "localhost"...
    uint16_t        port;       // 1-65535

    ListenEndpoint();
    ListenEndpoint(const std::string& h, uint16_t p);
};


// -------------------------------------------------------------
//  ServerConfig
//  Un bloque "server { ... }" del archivo de configuración.
// -------------------------------------------------------------
struct ServerConfig
{
    // Endpoints donde escucha este server.
    // Mínimo 1; el subject pide soportar varios host:port en total
    // (no necesariamente varios en el mismo bloque server).
    std::vector<ListenEndpoint>     listen;

    // Nombres del servidor (para virtual hosts; opcional).
    // Si lo dejáis sin usar al hacer el matching, no pasa nada.
    std::vector<std::string>        server_names;

    // Páginas de error personalizadas: código -> ruta del archivo HTML
    // Ej: 404 -> "/var/www/errors/404.html"
    std::map<int, std::string>      error_pages;

    // Tamaño máximo del body de la request a nivel de server.
    // En bytes. 0 = ilimitado (decisión vuestra). Default razonable: 1 MB.
    long                            client_max_body_size;

    // Bloques location de este server. Se evalúan eligiendo la
    // que tenga el "path" prefijo más largo de la URI pedida (longest match).
    std::vector<LocationConfig>     locations;

    ServerConfig();
};


// -------------------------------------------------------------
//  GlobalConfig
//  Todo el archivo de configuración parseado.
//  Es lo que devuelve el parser y lo que recibe el resto del programa.
// -------------------------------------------------------------
struct GlobalConfig
{
    std::vector<ServerConfig>       servers;
};

#endif

#include "webserv.hpp"

GlobalConfig createDefaultConfig() {
    GlobalConfig config;
    
    // Configuración del Servidor 1 (Ejemplo: Puerto 8080)
    ServerConfig server1;
    ListenEndpoint listen1;
    listen1.host = "127.0.0.1";
    listen1.port = 8080;
    server1.listen.push_back(listen1);
    server1.server_names.push_back("localhost");
    config.servers.push_back(server1);

    // Configuración del Servidor 2 (Ejemplo: Puerto 9090 para pruebas de multipuerto)
    ServerConfig server2;
    ListenEndpoint listen2;
    listen2.host = "127.0.0.1";
    listen2.port = 9090;
    server2.listen.push_back(listen2);
    server2.server_names.push_back("alternativo");
    config.servers.push_back(server2);

    return config;
}

int main(int argc, char** argv) {
    GlobalConfig config;

    if (argc > 2) {
        std::cerr << "❌ Uso correcto: ./webserv [archivo_de_configuracion.conf]" << std::endl;
        return 1;
    }

    try {
        if (argc == 2) {
            std::cout << "📖 Leyendo archivo de configuración: " << argv[1] << "..." << std::endl;
            // Aquí llamarías a la función de tu parser de archivos, ej:
            // config = parseConfigFile(argv[1]);
            
            // NOTA: Por ahora usamos la por defecto para que puedas compilar e ir probando
            config = createDefaultConfig();
        } else {
            std::cout << "⚠️ No se especificó archivo. Cargando configuración dinámica por defecto..." << std::endl;
            config = createDefaultConfig();
        }

        // 2. Instanciar el Core de Red (Tu clase Socket)
        Socket server;

        std::cout << "⚙️ Inicializando puertos y endpoints..." << std::endl;
        server.setPorts(config);

        std::cout << "🔌 Configurando sockets de escucha (Bind & Listen)..." << std::endl;
        server.configSocket();

        // 3. Lanzar el Servidor en el Event Loop (epoll)
        // Este método es un bucle infinito, el servidor se quedará aquí corriendo.
        server.runServer();

    } 
    catch (const std::exception& e) {
        std::cerr << "💥 Error crítico en la ejecución del servidor: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
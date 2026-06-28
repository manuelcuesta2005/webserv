#include "webserv.hpp"
#include "core/socket.hpp"
#include "core/Monitor.hpp"

GlobalConfig createDefaultConfig() {
    GlobalConfig config;
    
    ServerConfig server1;
    ListenEndpoint listen1;
    listen1.host = "127.0.0.1";
    listen1.port = 8080;
    server1.listen.push_back(listen1);
    server1.server_names.push_back("localhost");
    config.servers.push_back(server1);

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
            ConfigParser parser;
            config = parser.parse(argv[1]);
        } else {
            std::cout << "⚠️ No se especificó archivo. Cargando configuración dinámica por defecto..." << std::endl;
            config = createDefaultConfig();
        }

        // 1. Instanciar la Capa de Red Pura
        Socket serverSockets;

        std::cout << "⚙️ Inicializando puertos y endpoints..." << std::endl;
        serverSockets.setPorts(config);

        std::cout << "🔌 Configurando sockets de escucha (Bind & Listen)..." << std::endl;
        serverSockets.configSocket();

        // 2. Instanciar e inicializar el Monitor (Event Loop / epoll)
        Monitor eventLoop;
        
        std::cout << "🎛️ Inicializando el Monitor asíncrono (epoll)..." << std::endl;
        // Le pasamos los sockets ya configurados de la red al bucle de eventos
        eventLoop.init(serverSockets.getListeners());

        // 3. Lanzar el Servidor en el bucle infinito
        eventLoop.runServer();

    } 
    catch (const std::exception& e) {
        std::cerr << "💥 Error crítico en la ejecución del servidor: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
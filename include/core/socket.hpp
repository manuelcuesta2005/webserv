#ifndef SOCKET_HPP
#define SOCKET_HPP

#include "webserv.hpp"

struct HttpRequest;

struct ListeningSocket {
    int									_fd;
    std::string							_host;
    uint16_t							_port;
    struct sockaddr_in					_adreess;
	std::vector<const ServerConfig*>	_servers;
};

class Socket
{
	private:
		std::vector<ListeningSocket>	_listeners;

	public:
		Socket();
		~Socket();

		void								setPorts(const GlobalConfig& config);
		void								configSocket(void);
		const std::vector<ListeningSocket>& getListeners() const;
};

#endif
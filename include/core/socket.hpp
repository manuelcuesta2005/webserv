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
		std::vector<ListeningSocket>	_listenters;
		std::map<int, HttpRequest>		_requests;
		std::map<int, std::string>		_responses;
		int								_epoll_fd;

	public:
		Socket();
		Socket(const Socket& other);
		Socket& operator=(const Socket& other);
		~Socket();

		void	handleNewConnection(int listen_fd);
		void	handleClientRead(int client_fd);
		void	handleClientWrite(int client_fd);
		void	disconnectClient(int client_fd);
		void	setPorts(const GlobalConfig& config);
		void	configSocket(void);
		void	runServer();
};

#endif
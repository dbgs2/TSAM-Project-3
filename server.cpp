#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <algorithm>
#include <map>
#include <vector>

#include <iostream>
#include <sstream>
#include <thread>
#include <map>

// Allowed length of queue of waiting connections
#define BACKLOG 5

// Simple class for handling connections from clients.
//
// Client(int socket) - socket to send/receive traffic from client.
class Client
{
public:
    int sock;         // socket of client connection
    std::string name; // Limit length of name of client's user

    Client(int socket) : sock(socket) {}

    ~Client() {} // Virtual destructor defined for base class
};

// Simple class for handling connections from servers.
//
// Server(int socket) - socket to send/receive traffic from client.
class Server
{
public:
    int sock;
    std::string name;
    std::string ip;
    std::string port;

    Server(int socket) : sock(socket) {}

    ~Server() {} // Virtual destructor defined for base class
};

// Lookup tables for per Client or Server information
std::map<int, Client *> clients;
std::map<int, Server *> servers;

// Stores all msg that are outgoing, TODO::Reform to circular buffer, or more efficient
// storage, that can be reused.
std::multimap<std::string, std::string> clientMsg;

// Open socket for specified port.
//
// Returns -1 if unable to create the socket for any reason.
int open_socket(int portno)
{
    struct sockaddr_in sk_addr; // address settings for bind()
    int sock;                   // socket opened for this port
    int set = 1;                // for setsockopt

    // Create socket for connection. Set to be non-blocking, so recv will
    // return immediately if there isn't anything waiting to be read.

    if ((sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0)
    {
        perror("Failed to open socket");
        return (-1);
    }

    // Turn on SO_REUSEADDR to allow socket to be quickly reused after
    // program exit.

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0)
    {
        perror("Failed to set SO_REUSEADDR:");
    }

    memset(&sk_addr, 0, sizeof(sk_addr));

    sk_addr.sin_family = AF_INET;
    sk_addr.sin_addr.s_addr = INADDR_ANY;
    sk_addr.sin_port = htons(portno);

    // Bind to socket to listen for connections from clients

    if (bind(sock, (struct sockaddr *)&sk_addr, sizeof(sk_addr)) < 0)
    {
        perror("Failed to bind to socket:");
        return (-1);
    }
    else
    {
        return (sock);
    }
}

void closeClient(int clientSocket, fd_set *openSockets, int *maxfds)
{
    // Remove client from the clients list
    clients.erase(clientSocket);

    // If this client's socket is maxfds then the next lowest
    // one has to be determined. Socket fd's can be reused by the Kernel,
    // so there aren't any nice ways to do this.

    if (*maxfds == clientSocket)
    {
        for (auto const &p : clients)
        {
            *maxfds = std::max(*maxfds, p.second->sock);
        }
    }

    // And remove from the list of open sockets.

    FD_CLR(clientSocket, openSockets);
}

void closeServer(int serverSocket, fd_set *openSockets, int *maxfds)
{
    // Remove client from the clients list
    servers.erase(serverSocket);

    // If this client's socket is maxfds then the next lowest
    // one has to be determined. Socket fd's can be reused by the Kernel,
    // so there aren't any nice ways to do this.

    if (*maxfds == serverSocket)
    {
        for (auto const &p : servers)
        {
            *maxfds = std::max(*maxfds, p.second->sock);
        }
    }

    // And remove from the list of open sockets.

    FD_CLR(serverSocket, openSockets);
}

void connectToServer(const char *dst_groupname, const char *dst_ip, const char *dst_port)
{
    struct addrinfo hints, *svr;
    int serverSocket;
    int set = 1;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    memset(&hints, 0, sizeof(hints));

    if (getaddrinfo(dst_ip, dst_port, &hints, &svr) != 0)
    {
        perror("getaddrinfo failed: ");
        exit(0);
    }

    serverSocket = socket(svr->ai_family, svr->ai_socktype, svr->ai_protocol);

    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0)
    {
        printf("Failed to set SO_REUSEADDR for port %s\n", dst_port);
        perror("setsockopt failed: ");
    }

    if (connect(serverSocket, svr->ai_addr, svr->ai_addrlen) < 0)
    {
        printf("Failed to open socket to server: %s\n", dst_ip);
        perror("Connect failed: ");
        exit(0);
    }

    servers[serverSocket] = new Server(serverSocket);
    servers[serverSocket]->name = dst_groupname;
    servers[serverSocket]->ip = dst_ip;
    servers[serverSocket]->port = dst_port;

    // TODO MAKE THIS DYNAMIC
    //std::string server_name = "V_GROUP_42";
    //std::string server_ip = "192.168.100.222";
    //std::string server_port = "4042";
    //std::string msg = "CONNECT," + server_name + "," + server_ip + "," + server_port;
    std::string msg = "CONNECT,V_GROUP_42,192.168.100.222,4042";

    if (send(serverSocket, msg.c_str(), msg.length(), 0) < 0)
    {
        printf("Failed to send connect msg to server: %s\n", dst_ip);
        perror("Connect failed: ");
        exit(0);
    }
}

// Split up buffer into tokens, use the delimiter ','
// removes \n if in ned of tokens.
//
// Returns vector with all tokens.
std::vector<std::string> getTokens(char *buffer)
{
    std::vector<std::string> tokens;
    std::string token;

    // Split command from client into tokens for parsing
    std::stringstream stream(buffer);

    while (std::getline(stream, token, ','))
    {
        token.erase(std::remove(token.begin(), token.end(), '\n'), token.end());
        tokens.push_back(token);
    }
    return tokens;
}

// Process command from client on the server
int clientCommand(int clientSocket, fd_set *openSockets, int *maxfds, char *buffer)
{
    std::vector<std::string> tokens;
    tokens = getTokens(buffer);

    if ((tokens[0].compare("SENDMSG") == 0) && (tokens.size() == 3))
    {
        clientMsg.insert(std::pair<std::string, std::string>(tokens[1], tokens[2]));
        printf("Geting msg in now\n");
    }
    else if ((tokens[0].compare("GETMSG") == 0) && (tokens.size() == 2))
    {
        std::string msg;

        for (auto const &m : clientMsg)
        {
            msg += m.second + ",";
        }

        send(clientSocket, msg.c_str(), msg.length() - 1, 0);
    }
    else if ((tokens[0].compare("CONNECT") == 0) && (tokens.size() == 4))
    {
        connectToServer(tokens[1].c_str(), tokens[2].c_str(), tokens[3].c_str());
    }
    else if ((tokens[0].compare("CONNECT") == 0) && (tokens.size() != 4))
    {
        std::string msg = "Parameters not right, use this format: CONNECT,IP,PORT \n";
    }
    else if (tokens[0].compare("LISTSERVERS") == 0)
    {
        std::string msg = "SERVERS,";
        for (auto const &s : servers)
        {
            msg += s.second->name + "," + s.second->ip + "," + s.second->port + ";";
        }
        send(clientSocket, msg.c_str(), msg.length(), 0);
    }
    else
    {
        std::cout << "Unknown command from client:" << buffer << std::endl;
    }

    return 0;
}

// Process command from server to server
int serverCommand(int serverSocket, fd_set *openSockets, int *maxfds, char *buffer)
{
    std::vector<std::string> tokens;

    tokens = getTokens(buffer);

    // CONNECT,Group_Name,IP_addr,Port_num;
    if ((tokens[0].compare("CONNECT") == 0) && (tokens.size() == 4)) // was 2
    {
        servers[serverSocket]->name = tokens[1];
        servers[serverSocket]->ip = tokens[2];
        servers[serverSocket]->port = tokens[3];

        std::string msg = "Connection successful TODO this msg";
        std::cout << msg << std::endl;
        send(serverSocket, msg.c_str(), msg.length(), 0);
    }
    else if ((tokens[0].compare("CONNECT") == 0) && (tokens.size() != 4))
    {
        std::string msg = "Parameters not right, use this format: CONNECT,GROUP_ID,IP,PORT \n";
        send(serverSocket, msg.c_str(), msg.length(), 0);
    }

    // Leave with no parameters allowed at all ?
    /*
    else if (tokens[0].compare("LEAVE") == 0)
    {
        // Close the socket, and leave the socket handling
        // code to deal with tidying up clients etc. when
        // select() detects the OS has torn down the connection.

        closeServer(serverSocket, openSockets, maxfds);
    }
    */
    // LEAVE,SERVER IP,PORT
    else if ((tokens[0].compare("LEAVE") == 0) && (tokens.size() == 3))
    {

        for (auto i = servers.begin(); i != servers.end(); i++)
        {
            std::cout << "IP: " << tokens[1] << " PORT: " << tokens[2] << std::endl;
            std::cout << "IP: " << i->second->ip << " PORT: " << i->second->port << std::endl;
            if ((tokens[1].compare(i->second->ip) == 0) && (tokens[2].compare(i->second->port) == 0))
            {
                // found ip and port match.
                std::cout << "Found the ip and port match" << std::endl;
                closeServer(serverSocket, openSockets, maxfds);
                break;
            }
            else
            {
                /* code */
                std::string msg = "No match for ip: " + tokens[1] + " and port: " + tokens[2] + "\n";
                send(serverSocket, msg.c_str(), msg.length(), 0);
            }
        }

        // Close the socket, and leave the socket handling
        // code to deal with tidying up clients etc. when
        // select() detects the OS has torn down the connection.
        closeClient(serverSocket, openSockets, maxfds);
    }
    else if (tokens[0].compare("WHO") == 0)
    {
        std::cout << "Who is logged on" << std::endl;
        std::string msg;

        for (auto const &names : clients)
        {
            msg += names.second->name + ",";
        }
        // Reducing the msg length by 1 loses the excess "," - which
        // granted is totally cheating.
        send(serverSocket, msg.c_str(), msg.length() - 1, 0);
    }
    // This is slightly fragile, since it's relying on the order
    // of evaluation of the if statement.
    else if ((tokens[0].compare("MSG") == 0) && (tokens[1].compare("ALL") == 0))
    {
        char SOH = 0x01;
        char EOT = 0x04;
        std::string msg;
        for (auto i = tokens.begin() + 2; i != tokens.end(); i++)
        {
            msg += *i + " ";
        }
        msg = SOH + msg + EOT;
        for (auto const &pair : clients)
        {
            send(pair.second->sock, msg.c_str(), msg.length(), 0);
        }
    }
    else if (tokens[0].compare("SEND_MSG") == 0)
    {
        char SOH = 0x01;
        char EOT = 0x04;
        for (auto const &pair : clients)
        {
            if (pair.second->name.compare(tokens[1]) == 0)
            {
                std::string msg;
                for (auto i = tokens.begin() + 2; i != tokens.end(); i++)
                {
                    msg += *i + " ";
                }
                msg = SOH + msg + EOT;
                send(pair.second->sock, msg.c_str(), msg.length(), 0);
            }
        }
    }
    else
    {
        std::cout << "Unknown command from server:" << buffer << std::endl;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    bool finished;
    int listenSockClient; // Socket for connections to server
    int listenSockServer; // Socket for connections to server
    int clientSock;       // Socket of connecting client
    int serverSock;       // Socket of connecting client
    fd_set openSockets;   // Current open sockets
    fd_set readSockets;   // Socket list for select()
    fd_set exceptSockets; // Exception socket list
    int maxfds;           // Passed to select() as max fd in set
    struct sockaddr_in client;
    socklen_t clientLen;
    char buffer[1025]; // buffer for reading from clients

    if (argc != 2)
    {
        printf("Usage: chat_server <ip port>\n");
        exit(0);
    }

    // Setup socket for server to listen to
    listenSockServer = open_socket(atoi(argv[1]));
    printf("Listening on port: %d\n", listenSockServer); // Currently printing out socket, not port as it says it is.

    if (listen(listenSockServer, BACKLOG) < 0)
    {
        printf("Listen failed on port %s\n", argv[1]);
        exit(0);
    }
    else
    // Add listen socket to socket set we are monitoring
    {
        FD_SET(listenSockServer, &openSockets);
        maxfds = listenSockServer;
    }

    // WORK IN PROGRESS HERE FOR SEPERATE TCP CLIENT ONLY
    int client_sock = 4242;
    listenSockClient;
    // doing this so I can easily run multiple servers, eventually we only
    // run 1 server so this can be static if needed.
    std::cout << "enter client sock standard is 4242: ";
    std::cin >> client_sock;

    listenSockClient = open_socket(client_sock);

    if (listen(listenSockClient, BACKLOG) < 0)
    {
        printf("Listen failed on port %s\n", "4242");
        exit(0);
    }
    else
    // Add listen socket to socket set we are monitoring
    {
        FD_SET(listenSockClient, &openSockets);
        maxfds = listenSockClient;
    }

    finished = false;

    while (!finished)
    {
        // Get modifiable copy of readSockets
        readSockets = exceptSockets = openSockets;
        memset(buffer, 0, sizeof(buffer));

        // Look at sockets and see which ones have something to be read()
        int n = select(maxfds + 1, &readSockets, NULL, &exceptSockets, NULL);

        if (n < 0)
        {
            perror("select failed - closing down\n");
            finished = true;
        }
        else
        {
            // First, accept  any new connections to the server on the listening socket
            if (FD_ISSET(listenSockServer, &readSockets))
            {

                serverSock = accept(listenSockServer, (struct sockaddr *)&client, &clientLen);

                // Add new client to the list of open sockets
                FD_SET(serverSock, &openSockets);

                // And update the maximum file descriptor
                maxfds = std::max(maxfds, serverSock);

                // create a new client to store information.
                servers[serverSock] = new Server(serverSock);

                // Decrement the number of sockets waiting to be dealt with
                n--;

                printf("Server connected to server: %d\n", serverSock);
            }

            if (FD_ISSET(listenSockClient, &readSockets))
            {
                clientSock = accept(listenSockClient, (struct sockaddr *)&client, &clientLen);

                // Add new client to the list of open sockets
                FD_SET(clientSock, &openSockets);

                // And update the maximum file descriptor
                maxfds = std::max(maxfds, clientSock);

                // create a new client to store information.
                clients[clientSock] = new Client(clientSock);

                // Decrement the number of sockets waiting to be dealt with
                n--;
                printf("Client connected to server: %d\n", serverSock);
            }

            // Now check for commands from clients
            while (n-- > 0)
            {
                for (auto const &pair : clients)
                {
                    Client *client = pair.second;

                    if (FD_ISSET(client->sock, &readSockets))
                    {
                        // recv() == 0 means client has closed connection
                        if (recv(client->sock, buffer, sizeof(buffer), MSG_DONTWAIT) == 0)
                        {
                            printf("Client closed connection: %d", client->sock);
                            close(client->sock);

                            closeClient(client->sock, &openSockets, &maxfds);
                        }
                        // We don't check for -1 (nothing received) because select()
                        // only triggers if there is something on the socket for us.
                        else
                        {
                            std::cout << buffer << std::endl;
                            clientCommand(client->sock, &openSockets, &maxfds, buffer);
                        }
                    }
                }
                for (auto const &pair : servers)
                {
                    Server *server = pair.second;

                    if (FD_ISSET(server->sock, &readSockets))
                    {
                        // recv() == 0 means client has closed connection
                        if (recv(server->sock, buffer, sizeof(buffer), MSG_DONTWAIT) == 0)
                        {
                            printf("Client closed connection: %d", server->sock);
                            close(server->sock);

                            closeClient(server->sock, &openSockets, &maxfds);
                        }
                        // We don't check for -1 (nothing received) because select()
                        // only triggers if there is something on the socket for us.
                        else
                        {
                            std::cout << buffer << std::endl;
                            serverCommand(server->sock, &openSockets, &maxfds, buffer);
                        }
                    }
                }
            }
        }
    }
}
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

#include <net/if.h>
#include <ifaddrs.h>

// Allowed length of queue of waiting connections
#define BACKLOG 5

// TODO MAKE THIS DYNAMIC
std::string server_name = "V_GROUP_42";
std::string server_ip = "0";
std::string server_port = "0";

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

std::string getIp()
{
    struct ifaddrs *myaddrs, *ifa;
    void *in_addr;
    char buf[64];
    //std::string ip;

    if (getifaddrs(&myaddrs) != 0)
    {
        perror("getifaddrs");
        exit(1);
    }

    for (ifa = myaddrs; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;
        if (!(ifa->ifa_flags & IFF_UP))
            continue;

        switch (ifa->ifa_addr->sa_family)
        {
        case AF_INET:
        {
            struct sockaddr_in *s4 = (struct sockaddr_in *)ifa->ifa_addr;
            in_addr = &s4->sin_addr;
            break;
        }

        case AF_INET6:
        {
            struct sockaddr_in6 *s6 = (struct sockaddr_in6 *)ifa->ifa_addr;
            in_addr = &s6->sin6_addr;
            break;
        }

        default:
            continue;
        }

        if (!inet_ntop(ifa->ifa_addr->sa_family, in_addr, buf, sizeof(buf)))
        {
            printf("%s: inet_ntop failed!\n", ifa->ifa_name);
        }
        else
        {

            if (strcmp(ifa->ifa_name, "wlan0") == 0)
            {
                return inet_ntoa(((struct sockaddr_in *)ifa->ifa_addr)->sin_addr);
            }
        }
    }

    freeifaddrs(myaddrs);
    return "wlan0 not found";
}

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

    std::string msg = "CONNECT," + server_name + "," + server_ip + "," + server_port;
    //std::string msg = "CONNECT,V_GROUP_42,192.168.100.222,4042";

    if (send(serverSocket, msg.c_str(), msg.length(), 0) < 0)
    {
        printf("Failed to send connect msg to server: %s\n", dst_ip);
        perror("Connect failed: ");
        exit(0);
    }
}

void listReqToServer(const char *dst_groupname)
{

    for (auto const &s : servers)
    {
        if (s.second->name != dst_groupname)
        {
            std::string tmp = dst_groupname;
            std::string msg = "LISTSERVERS," + tmp;
            //servers_list += s.second->name + "," + s.second->ip + "," + s.second->port + ";";
            send(s.second->sock, msg.c_str(), msg.length(), 0);
        }
    }
}

// Goes over each connected server and adds them to a string
//
// returns a string, SERVERS,GROUP_ID,SERVER_IP,SERVER_PORT;GROUP_ID,SERVER_IP,SERVER_PORT;
std::string listServers(int serverSock)
{

    //Server *con_server = servers.at(serverSock);


    std::string servers_list = "SERVERS," + server_name+ "," + server_ip + "," + server_port + ";";
    for (auto const &s : servers)
    {
        servers_list += s.second->name + "," + s.second->ip + "," + s.second->port + ";";
    }
    return servers_list;
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
            if(m.first == tokens[1]){
                msg += m.second + ",";
            } 
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
    else if ((tokens[0].compare("LISTSERVERS") == 0) && (tokens.size() == 2))
    {
        //std::string servers_list = listServers();
        //send(clientSocket, servers_list.c_str(), servers_list.length(), 0);
        listReqToServer(tokens[1].c_str());
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

    // LISTSERVERS,<FROM GROUP ID>
    if ((tokens[0].compare("LISTSERVERS") == 0) && (tokens.size() == 2))
    {

        std::string servers_list = listServers(serverSocket);
        send(serverSocket, servers_list.c_str(), servers_list.length(), 0);
    }
    // SERVERS
    // e.g. SERVERS,V GROUP 1,130.208.243.61,8888;V GROUP 2,10.2.132.12,888;
    else if (tokens[0].compare("SERVERS") == 0)
    {
        /* code */
    }
    // KEEPALIVE,<No. of Messages>
    else if ((tokens[0].compare("KEEPALIVE") == 0) && (tokens.size() == 2))
    {
        /* code */
    }
    // GET_MSG,<GROUP ID>
    else if ((tokens[0].compare("GET_MSG") == 0) && (tokens.size() == 2))
    {
        std::string msg;
        clientMsg.insert(std::pair<std::string, std::string>("11", "bla1"));
        clientMsg.insert(std::pair<std::string, std::string>("11", "bla2"));
        clientMsg.insert(std::pair<std::string, std::string>("12", "bla3"));
        clientMsg.insert(std::pair<std::string, std::string>("12", "bla4"));

        for (auto const &m : clientMsg)
        {   
            if(m.first == tokens[1]){
                msg += m.second + ",";
            } 
        }

        send(serverSocket, msg.c_str(), msg.length() - 1, 0);
    }
    // SEND MSG,<FROM GROUP ID>,<TO GROUP ID>,<Message content>
    else if ((tokens[0].compare("SEND_MSG") == 0) && (tokens.size() == 4))
    {
        /* code */
    }
    // LEAVE,SERVER IP,PORT
    else if ((tokens[0].compare("LEAVE") == 0) && (tokens.size() == 3))
    {
        for (auto const &s : servers)
        {
            if(s.second->ip == tokens[1] && s.second->port == tokens[1])
            {
                closeServer(s.second->sock, openSockets, maxfds);
            }else
            {
                std::cout << "Could not leave server ???" << std::endl;
            }
            
        }
        
    }
    // STATUSREQ,FROM GROUP
    else if ((tokens[0].compare("STATUSREQ") == 0) && (tokens.size() == 2))
    {
        /* code */
    }
    // STATUSRESP,FROM GROUP,TO GROUP,<server, msgs held>,...
    // eg. STATUSRESP,V GROUP 2,I 1,V GROUP4,20,V GROUP71,2
    else if ((tokens[0].compare("STATUSRESP") == 0) && (tokens.size() == 4)) // needs more ?
    {
        /* code */
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

    server_port = argv[1];
    server_ip = getIp();

    std::cout << "Found ip: " << server_ip << " and port: " << server_port << std::endl;


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
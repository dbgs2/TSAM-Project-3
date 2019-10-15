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
#include <net/if.h>
#include <ifaddrs.h>
#include <ctime>
#include <time.h>
#include <fstream>
#include "randomMessage.h"

// Allowed length of queue of waiting connections
#define BACKLOG 5
// Maximum buffer for char arrays
#define BUF_MAX 1025
// Maximum connections allowed
#define CONNECTIONS_MAX 5

// Defined Constants used to store server infromation
std::string server_name = "P3_GROUP_42";
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
    time_t lastMsg;

    Server(int socket) : sock(socket) {}

    ~Server() {} // Virtual destructor defined for base class
};

// Struct to store msg's and the sender
struct Msg
{
    std::string sender;
    std::string message;
};

// Lookup tables for per Client or Server information
std::map<int, Client *> clients;
std::map<int, Server *> servers;

// Vector to store socket of servers to be removed
std::vector<int> serversToRemove;

// Store all msg, <name, <vector of msgs> >
std::map<std::string, std::vector<Msg>> clientMsg;

// Functions declerations
std::string getIp();
std::string listServers();
std::string statusRespond();
int open_socket(int portno);
int getServer(std::string servName);
int serverCommand(int serverSocket, fd_set *openSockets, int *maxfds, char *buffer);
void clientCommand(int clientSocket, fd_set *openSockets, int *maxfds, char *buffer);
void closeClient(int clientSocket, fd_set *openSockets, int *maxfds);
void closeServer(int serverSocket, fd_set *openSockets, int *maxfds);
void writeLog(char *msg, std::string inOrOut);
void sendKeepAlive();
bool sendToServer(int socket, std::string msg);
bool connectToServer(const char *dst_ip, const char *dst_port, fd_set *openSockets, int *maxfds);
std::vector<std::string> getTokens(char *buffer, const char delim);

// Gets current ip
//
// Returns string of current wlan0 or lo ip
std::string getIp()
{
    struct ifaddrs *myaddrs, *ifa;
    void *in_addr;
    char buf[64];

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
            std::string str = inet_ntoa(((struct sockaddr_in *)ifa->ifa_addr)->sin_addr);
            std::size_t found = str.find("130.");

            if (strcmp(ifa->ifa_name, "wlan0") == 0)
            {
                return inet_ntoa(((struct sockaddr_in *)ifa->ifa_addr)->sin_addr);
            }
            else if (found != std::string::npos)
            {
                return str;
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

    if ((bind(sock, (const sockaddr *)&sk_addr, sizeof(sk_addr))) < 0)
    {
        perror("Failed to bind to socket:");
        return -1;
    }
    else
    {
        return sock;
    }
}

// Closes connection to client
//
// No return
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

// Closes connection to server
//
// No return
void closeServer(int serverSocket, fd_set *openSockets, int *maxfds)
{
    //sendToServer(serverSocket, "Closing the connection now, over and out!");
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

// Sends msg to another server/client
//
// returns true if successful
bool sendToServer(int socket, std::string msg)
{
    std::string SOH = "\1";
    std::string EOT = "\4";
    std::string msg2 = SOH + msg + EOT;

    std::cout << "SERVER IS SENDING >>> " << msg2 << std::endl;

    if ((send(socket, msg2.c_str(), msg2.length(), 0)) < 0)
    {
        perror("Send failed: ");
        return 0;
    }

    return 1;
}

// Connects to another server, and sends LISTSERVERS
//
// Returns false if unable to connect to server for any reason.
bool connectToServer(const char *dst_ip, const char *dst_port, fd_set *openSockets, int *maxfds)
{

    struct addrinfo hints, *svr;
    int serverSocket;
    int set = 1;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(dst_ip, dst_port, &hints, &svr) != 0)
    {
        perror("getaddrinfo failed: ");
        return 0;
    }

    serverSocket = socket(svr->ai_family, svr->ai_socktype, svr->ai_protocol);

    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0)
    {
        printf("Failed to set SO_REUSEADDR for port %s\n", dst_port);
        perror("setsockopt failed: ");
        return 0;
    }

    if (connect(serverSocket, svr->ai_addr, svr->ai_addrlen) < 0)
    {
        printf("Failed to open socket to server: %s\n", dst_ip);
        perror("Connect failed: ");
        return 0;
    }

    FD_SET(serverSocket, openSockets);
    *maxfds = serverSocket;

    servers[serverSocket] = new Server(serverSocket);
    servers[serverSocket]->lastMsg = time(0);

    std::string msg = "LISTSERVERS," + server_name;

    if ((sendToServer(serverSocket, msg)) == 0)
    {
        return 0;
    }

    return 1;
}

// Goes over each connected server and adds them to a string
//
// returns a string, SERVERS,GROUP_ID,SERVER_IP,SERVER_PORT;GROUP_ID,SERVER_IP,SERVER_PORT;
std::string listServers()
{
    std::string servers_list = "SERVERS," + server_name + "," + server_ip + "," + server_port + ";";

    if (!servers.empty())
    {
        for (auto const &s : servers)
        {
            if (!s.second->name.empty())
                servers_list += s.second->name + "," + s.second->ip + "," + s.second->port + ";";
        }
        return servers_list;
    }

    return servers_list;
}

// Sends Keepalive msg to all connected servers, with how many msg are waiting
//
// No return
void sendKeepAlive()
{
    for (auto const &s : servers)
    {
        std::string cnt = std::to_string(clientMsg[s.second->name].size());
        std::string msg = "KEEPALIVE," + cnt;
        sendToServer(s.second->sock, msg.c_str());
    }
}

// Goes through the client msg, and addst the count for each server that has a msg to a list.
//
// returns a string with statusresp like : STATUSRESP,V GROUP 2,I 1,V GROUP4,20,V GROUP71,2
std::string statusRespond(std::string to_server)
{
    std::string msg = "STATUSRESP," + server_name + "," + to_server;

    for (auto const &s : clientMsg)
    {
        std::string cnt = std::to_string(s.second.size());

        if (s.second.size() > 0)
        {
            msg += "," + s.first + "," + cnt;
        }
    }
    return msg;
}

// Gets server sock by server name
//
// returns socket and if none found returns -1 for error handling
int getServer(std::string servName)
{
    for (auto const &s : servers)
    {
        if (servName.compare(s.second->name) == 0)
        {
            return s.second->sock;
        }
    }
    return -1;
}

// Split up buffer into tokens, by a delimiter, and removes it
// removes \n if in the of tokens.
//
// Returns vector with all tokens.
std::vector<std::string> getTokens(char *buffer, const char delim)
{
    std::vector<std::string> tokens;
    std::string token;

    // Split command from client into tokens for parsing
    std::stringstream stream(buffer);

    while (std::getline(stream, token, delim))
    {
        token.erase(std::remove(token.begin(), token.end(), '\n'), token.end());
        tokens.push_back(token);
    }
    return tokens;
}

// Writes a log msg to a file
//
// No return
void writeLog(char *msg, std::string inOrOut)
{
    time_t rawtime;
    time(&rawtime);
    std::ofstream outfile;
    outfile.open("serverMsgLog.txt", std::ios::app);

    outfile << ctime(&rawtime);
    outfile << inOrOut;
    outfile << msg;
    outfile << '\n';
    outfile << '\n';
}

// Process command from client on the server
//
// No return
void clientCommand(int clientSocket, fd_set *openSockets, int *maxfds, char *buffer)
{
    std::vector<std::string> tokens;
    tokens = getTokens(buffer, ',');
    randomMessage randMessage;
    randMessage.initializeVector();

    // SENDMSG,TO_GROUP
    if ((tokens[0].compare("SENDMSG") == 0) && (tokens.size() == 2))
    {
        std::cout << "Executing CLIENT command SENDMSG" << std::endl;
        std::cout << "-->>" << randMessage.getRandomMessage() << std::endl;

        Msg m;
        m.sender = server_name;
        m.message = randMessage.getRandomMessage();

        clientMsg[tokens[1]].push_back(m);
    }
    // SENDMSG,TO_GROUP,MSG
    else if ((tokens[0].compare("SENDMSG") == 0) && (tokens.size() == 3))
    {
        std::cout << "Executing CLIENT command SENDMSG" << std::endl;
        Msg m;
        m.sender = server_name;
        m.message = tokens[2];

        clientMsg[tokens[1]].push_back(m);
    }
    // SENDTOSERVER,TO_GROUP
    else if ((tokens[0].compare("SENDTOSERVER") == 0) && (tokens.size() == 2))
    {
        randomMessage randMessage;
        std::cout << "Executing CLIENT command SENDTOSERVER" << std::endl;
        std::string msg = "SEND_MSG," + server_name + "," + tokens[1] + "," + randMessage.getRandomMessage() + ";"; //tokens[2]

        int sock = getServer(tokens[1]);
        if (sock < 0)
        {
            std::cout << "failed" << std::endl;
        }
        else
        {
            sendToServer(sock, msg);
        }
    }
    // SENDTOSERVER,TO_GROUP,MSG
    else if ((tokens[0].compare("SENDTOSERVER") == 0) && (tokens.size() == 3))
    {
        randomMessage randMessage;
        std::cout << "Executing CLIENT command SENDTOSERVER" << std::endl;
        std::string msg = "SEND_MSG," + server_name + "," + tokens[1] + "," + tokens[2] + ";"; //tokens[2]

        int sock = getServer(tokens[1]);
        if (sock < 0)
        {
            std::cout << "failed" << std::endl;
        }
        else
        {
            sendToServer(sock, msg);
        }
    }
    // GETMSG,FROM_GROUP
    else if ((tokens[0].compare("GETMSG") == 0) && (tokens.size() == 2))
    {
        std::cout << "Executing CLIENT command GETMSG" << std::endl;
        std::string msg;

        std::cout << "size : " << clientMsg.size() << std::endl;

        for (auto const &m : clientMsg)
        {
            if (tokens[1].compare(m.first) == 0)
            {
                for (auto ms : m.second)
                {
                    msg += ms.sender + "," + ms.message + ",";
                }
            }
        }
        sendToServer(clientSocket, msg);

        clientMsg.erase(tokens[1]);
    }
    // GETMSGTOSERVER,TO_GROUP,FROM_GROUP
    else if ((tokens[0].compare("GETMSGTOSERVER") == 0) && (tokens.size() == 3))
    {
        std::cout << "Executing CLIENT command GETMSGTOSERVER" << std::endl;
        std::string msg = "GET_MSG," + tokens[2];

        int sock = getServer(tokens[1]);
        if (sock < 0)
        {
            std::cout << "failed" << std::endl;
        }
        else
        {
            sendToServer(sock, msg);
        }
    }
    // CONNECT,TO_IP,TO_PORT
    else if ((tokens[0].compare("CONNECT") == 0) && (tokens.size() == 3))
    {
        std::cout << "SERVER SIZE: -> " << servers.size() << std::endl;
        if (servers.size() < CONNECTIONS_MAX)
        {
            std::cout << "Executing CLIENT command CONNECT" << std::endl;
            if (connectToServer(tokens[1].c_str(), tokens[2].c_str(), openSockets, maxfds))
            {
                std::string msg = "Connection to server successful";
                sendToServer(clientSocket, msg);
            }
            else
            {
                std::string msg = "Connection to server failed";
                sendToServer(clientSocket, msg);
            }
        }
        else
        {
            std::cout << "Too many servers connected" << std::endl;
            std::string cantConnect = "Too many servers connected, try again later!";
            sendToServer(clientSocket, cantConnect);
        }
    }
    // LISTSERVERS
    else if (tokens[0].compare("LISTSERVERS") == 0)
    {
        std::cout << "Executing CLIENT command LISTSERVERS" << std::endl;

        std::string servers_list = listServers();
        sendToServer(clientSocket, servers_list);
    }
    // LEAVE,TO_GROUP
    else if ((tokens[0].compare("LEAVE") == 0) && (tokens.size() == 2))
    {
        int sock = getServer(tokens[1]);
        if (sock < 0)
        {
            std::cout << "Failed to get server" << std::endl;
        }
        else
        {
            std::string msg = "LEAVE," + server_ip + "," + server_port;

            sendToServer(sock, msg);
        }
    }
    // STATUSREQ,TO_GROUP
    else if ((tokens[0].compare("STATUSREQ") == 0) && (tokens.size() == 2))
    {
        std::cout << "Executing CLIENT command STATUSREQ" << std::endl;
        std::string msg = "STATUSREQ," + server_name;

        int sock = getServer(tokens[1]);
        sendToServer(sock, msg);
    }
    // UNKNOWN COMMAND
    else
    {
        std::cout << "Unknown command from client:" << buffer << std::endl;
        std::string msg = "Unknown command, try again";
        sendToServer(clientSocket, msg);
    }
}

// Process command from server to server
//
// No return
int serverCommand(int serverSocket, fd_set *openSockets, int *maxfds, char *buffer)
{
    std::vector<std::string> tokens;
    tokens = getTokens(buffer, ',');

    // LISTSERVERS,<FROM GROUP ID>
    if ((tokens[0].compare("LISTSERVERS") == 0) && (tokens.size() == 2))
    {
        std::cout << "Executing SERVER command LISTSERVERS" << std::endl;
        std::string servers_list = listServers();
        sendToServer(serverSocket, servers_list);
    }
    // e.g. SERVERS,V GROUP 1,130.208.243.61,8888;V GROUP 2,10.2.132.12,888;
    else if (tokens[0].compare("SERVERS") == 0)
    {
        std::cout << "Executing SERVER command SERVERS" << std::endl;

        std::vector<std::string> serverTokens = getTokens(buffer, ';');
        char char_array[BUF_MAX];
        char toLog[BUF_MAX];
        memset(&char_array, 0, sizeof(char_array));
        strcpy(char_array, serverTokens[0].c_str());
        memset(&toLog, 0, sizeof(char_array));
        std::vector<std::string> serverTokensSplit = getTokens(char_array, ',');

        if (servers[serverSocket]->name.empty() && servers[serverSocket]->ip.empty() && servers[serverSocket]->port.empty())
        {
            servers[serverSocket]->name = serverTokensSplit[1];
            servers[serverSocket]->ip = serverTokensSplit[2];
            servers[serverSocket]->port = serverTokensSplit[3];

            // THIS IS USED FOR AUTOMATICLY SEND A MSG TO ANOTHER SERVER ON SUCCESSFUL CONNECTION
            /*
            randomMessage randMessage;
            randMessage.initializeVector();
            std::string msg = "SEND_MSG," + server_name + "," + serverTokensSplit[1] + "," + randMessage.getRandomMessage();

            if ((sendToServer(serverSocket, msg)) == 0)
            {
                std::cout << "Failed to send to server" << std::endl;
            }
            strcpy(toLog, msg.c_str());
            writeLog(toLog, "SENDING MESSAGE\n");
            */
        }
    }
    // KEEPALIVE,<No. of Messages>
    else if ((tokens[0].compare("KEEPALIVE") == 0) && (tokens.size() == 2))
    {
        std::cout << "Executing SERVER command KEEPALIVE" << std::endl;
        std::string msg = "GET_MSG," + server_name;

        int cnt = std::stoi(tokens[1]);
        if (cnt > 0)
        {
            sendToServer(serverSocket, msg);
        }
    }
    // GET_MSG,<GROUP ID>
    else if ((tokens[0].compare("GET_MSG") == 0) && (tokens.size() == 2))
    {
        char toSend[BUF_MAX];
        std::cout << "Executing SERVER command GET_MSG" << std::endl;
        std::string msg = "SEND_MSG,";

        for (auto const &m : clientMsg)
        {

            if (m.first == tokens[1])
            {
                for (auto ms : m.second)
                {
                    msg += ms.sender + "," + m.first + "," + ms.message; // + ","
                }
            }
        }

        memset(&toSend, 0, sizeof(toSend));
        strcpy(toSend, msg.c_str());

        if ((sendToServer(serverSocket, msg)) == 0)
        {
            std::cout << "Failed to send to server" << std::endl;
        }
        else
        {
            writeLog(toSend, "SENDING MESSAGE\n");
        }

        /*
        if (clientMsg.erase(tokens[1]))
        {
        }
        */
    }
    // SEND MSG,<FROM GROUP ID>,<TO GROUP ID>,<Message content>
    else if ((tokens[0].compare("SEND_MSG") == 0) && (tokens.size() >= 4))
    {
        std::cout << "Executing SERVER command SEND_MSG" << std::endl;
        std::string msg;
        Msg m;
        m.sender = tokens[1];

        for (size_t i = 3; i < tokens.size(); i++)
        {
            msg += tokens[i];
        }

        m.message = msg;
        writeLog(buffer, "GETING MESSAGE\n");

        clientMsg[tokens[2]].push_back(m);
    }
    // LEAVE,SERVER IP,PORT
    else if ((tokens[0].compare("LEAVE") == 0) && (tokens.size() == 3))
    {
        std::cout << "Executing SERVER command LEAVE" << std::endl;
        for (auto const &s : servers)
        {
            if ((tokens[1].compare(s.second->ip) == 0) && (tokens[2].compare(s.second->port) == 0))
            {
                serversToRemove.push_back(s.first);
            }
            else
            {
                std::cout << "Could not leave server ???" << std::endl;
            }
        }
    }
    // STATUSREQ,FROM GROUP
    else if ((tokens[0].compare("STATUSREQ") == 0) && (tokens.size() == 2))
    {
        std::cout << "Executing SERVER command STATUSREQ" << std::endl;
        std::string msg = statusRespond(servers[serverSocket]->name);
        sendToServer(serverSocket, msg);
    }
    // STATUSRESP,FROM GROUP,TO GROUP,<server, msgs held>,...
    // eg. STATUSRESP,V GROUP 2,I 1,V GROUP4,20,V GROUP71,2
    else if ((tokens[0].compare("STATUSRESP") == 0) && (tokens.size() >= 3)) // needs more ?
    {
        std::cout << "Executing SERVER command STATUSRESP" << std::endl;

        for (size_t i = 3; i < tokens.size(); i = i + 2)
        {
            int sock = getServer(tokens[i]);
            if (sock < 0)
            {
                //std::cout << "failed" << std::endl;
            }
            else
            {
                std::cout << "Found one" << std::endl;
                std::string msg = "GET_MSG," + tokens[i];
                sendToServer(serverSocket, msg);
            }
        }
    }
    // UNKNOWN COMMAND
    else
    {
        std::cout << "Unknown command from server:" << buffer << std::endl;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    bool finished;
    int listenSockServer; // Socket for connections to server
    int listenSockClient; // Socket for connections to client
    int sock;             // Socket of connecting client
    fd_set openSockets;   // Current open sockets
    fd_set readSockets;   // Socket list for select()
    fd_set exceptSockets; // Exception socket list
    int maxfds;           // Passed to select() as max fd in set
    struct sockaddr_in adddr_in;
    socklen_t adddr_in_len;
    char buffer[BUF_MAX]; // buffer for reading from clients

    if (argc != 2)
    {
        printf("Usage: chat_server <ip port>\n");
        exit(0);
    }

    server_port = argv[1];
    server_ip = getIp();

    std::cout << "Server ip: " << server_ip << std::endl;
    std::cout << "Server port: " << server_port << std::endl;

    // Setup socket for server to listen to other servers
    listenSockServer = open_socket(atoi(argv[1]));

    if (listen(listenSockServer, BACKLOG) < 0)
    {
        printf("Listen failed on port %s\n", argv[1]);
        exit(0);
    }
    else
    // Add listen socket to socket set we are monitoring
    {
        FD_SET(listenSockServer, &openSockets);
    }

    // Setup socket for server to listen to other clients
    std::cout << "Select client port: ";
    std::string clientPort;
    std::cin >> clientPort;
    listenSockClient = open_socket(atoi(clientPort.c_str()));

    // FOR DEBUGING MULTIPLE SERVERS
    
    std::cout << "Select server name: ";
    std::cin >> server_name;
    
    

    // Setup socket for server to listen to other servers
    if (listen(listenSockClient, BACKLOG) < 0)
    {
        //printf("Listen failed on port %s\n", clientPort);
        exit(0);
    }
    else
    {
        FD_SET(listenSockClient, &openSockets);
        maxfds = listenSockClient;
    }

    finished = false;

    struct timeval TV;
    TV.tv_sec = 60;
    TV.tv_usec = 0;
    time_t timeOut = 300;
    time_t timeInterval = 60;
    time_t KA = time(0);

    while (!finished)
    {
        // Get modifiable copy of readSockets
        readSockets = exceptSockets = openSockets;
        memset(buffer, 0, sizeof(buffer));

        for (auto &s : servers)
        {
            time_t keepAliveRecv = time(0) - (s.second->lastMsg);

            if (keepAliveRecv >= timeOut)
            {
                std::cout << "Time ran out for server: " << s.second->name << std::endl;
                serversToRemove.push_back(s.second->sock);
            }
        }

        for (auto &s : serversToRemove)
        {
            std::cout << "Closing server: " << servers[s]->name << std::endl;
            closeServer(s, &openSockets, &maxfds);
        }

        serversToRemove.clear();

        if (time(0) - KA > timeInterval)
        {
            sendKeepAlive();
            KA = time(0);
        }

        // Look at sockets and see which ones have something to be read()
        int n = select(maxfds + 1, &readSockets, NULL, &exceptSockets, &TV); //&TV

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
                sock = accept(listenSockServer, (struct sockaddr *)&adddr_in, &adddr_in_len);

                if (servers.size() < CONNECTIONS_MAX)
                {
                    // Add new client to the list of open sockets
                    FD_SET(sock, &openSockets);

                    // And update the maximum file descriptor
                    maxfds = std::max(maxfds, sock);
                    // create a new client to store information.
                    servers[sock] = new Server(sock);
                    servers[sock]->lastMsg = time(0);

                    std::string lisServStr = "LISTSERVERS," + server_name;
                    sendToServer(sock, lisServStr);
                    printf("Server connected to server: %d\n", sock);
                }
                else
                {
                    std::cout << "Too many servers connected" << std::endl;
                    std::string cantConnect = "Too many servers connected, try again later!";
                    sendToServer(sock, cantConnect);
                }

                // Decrement the number of sockets waiting to be dealt with
                n--;
            }
            // First, accept  any new connections to the server on the listening socket
            if (FD_ISSET(listenSockClient, &readSockets))
            {
                sock = accept(listenSockClient, (struct sockaddr *)&adddr_in, &adddr_in_len);

                // Add new client to the list of open sockets
                FD_SET(sock, &openSockets);

                // And update the maximum file descriptor
                maxfds = std::max(maxfds, sock);

                // create a new client to store information.
                clients[sock] = new Client(sock);

                // Decrement the number of sockets waiting to be dealt with
                n--;

                printf("Client connected to server: %d\n", sock);
            }
            // Now check for commands from clients or servers
            while (n-- > 0)
            {

                // Go through each server connected and check for commands
                for (auto const &pair : servers)
                {

                    Server *server = pair.second;
                    if (server == NULL)
                    {
                        break;
                    }

                    // Check for server commands
                    if (FD_ISSET(server->sock, &readSockets))
                    {
                        
                        int sizeofcurrentdata = recv(server->sock, buffer, sizeof(buffer), MSG_PEEK);
                        //TODO:: check buffer after MSG_PEEK and make sure it has SOH,EOT transmission, instead
                        // of doing it here below, its a better way of doing it.

                        // recv() == 0 means client has closed connection
                        if (recv(server->sock, buffer, sizeof(buffer), MSG_DONTWAIT) == 0)
                        {
                            printf("Server closed connection: %d \n", server->sock);
                            close(server->sock);

                            closeServer(server->sock, &openSockets, &maxfds);
                        }
                        // We don't check for -1 (nothing received) because select()
                        // only triggers if there is something on the socket for us.
                        else
                        {
                            servers[server->sock]->lastMsg = time(0);
                            std::cout << "SERVER IS RECIVING from server: " << server->name << " <<< " << buffer << std::endl;
                            std::cout << std::endl;

                            std::vector<std::string> tokens = getTokens(buffer, '\1');

                            char char_array[BUF_MAX];

                            
                            for (size_t i = 1; i < tokens.size(); i++)//auto t : tokens
                            {
                                memset(&char_array, 0, sizeof(char_array));
                                //std::cout << "Token: " << t << std::endl;
                                if (tokens[i].back() == '\4')
                                {
                                    tokens[i].pop_back();
                                    strcpy(char_array, tokens[i].c_str());
                                    serverCommand(server->sock, &openSockets, &maxfds, char_array);
                                }
                                else
                                {
                                    std::cout << "Wrong format on command" << std::endl;
                                }
                            }
                        }
                    }
                }

                // Go through each client connected and check for commands
                for (auto const &pair : clients)
                {
                    Client *client = pair.second;
                    if (client == NULL)
                    {
                        break;
                    }
                    // Check for client commands
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
                            std::cout << std::endl;
                            clientCommand(client->sock, &openSockets, &maxfds, buffer);
                        }
                    }
                }
            }
        }
        TV.tv_sec = 60;
    }
}
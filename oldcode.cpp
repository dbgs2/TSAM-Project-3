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





    /*
int main(int argc, char *argv[])
{
    bool finished;
    int listenSockClient;     // Socket for connections to server
    int listenSockServer;     // Socket for connections to server
    int clientSock;           // Socket of connecting client
    int serverSock;           // Socket of connecting client
    fd_set openServerSockets; // Current open sockets
    fd_set openClientSockets;
    fd_set readSockets;   // Socket list for select()
    fd_set exceptSockets; // Exception socket list

    int maxfdsServ; // Passed to select() as max fd in set
    int maxfdsCli;
    struct sockaddr_in client;
    socklen_t clientLen;

    struct sockaddr_in server;
    socklen_t serverLen;
    char buffer[1025]; // buffer for reading from clients

    if (argc != 2)
    {
        printf("Usage: chat_server <ip port>\n");
        exit(0);
    }

    server_port = argv[1];
    server_ip = getIp();

    std::cout << "Server ip: " << server_ip << std::endl;
    std::cout << "Server port: " << server_port << std::endl;

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
        FD_SET(listenSockServer, &openServerSockets);
        maxfdsServ = listenSockServer;
    }

    // WORK IN PROGRESS HERE FOR SEPERATE TCP CLIENT ONLY
    int client_sock = 4242;
    //listenSockClient;
    // doing this so I can easily run multiple servers, eventually we only
    // run 1 server so this can be static if needed.
    std::cout << "enter client sock standard is 4242: ";
    std::cin >> client_sock;

    listenSockClient = open_socket(client_sock);

    if (listen(listenSockClient, 1) < 0)
    {
        printf("Listen failed on port %s\n", "4242");
        exit(0);
    }
    else
    // Add listen socket to socket set we are monitoring
    {
        FD_SET(listenSockClient, &openClientSockets);
        maxfdsCli = listenSockClient;
    }

    finished = false;

    while (!finished)
    {
        // Get modifiable copy of readSockets
        readSockets = exceptSockets = openServerSockets;
        memset(buffer, 0, sizeof(buffer));

        std::cout << "before select" << std::endl;
        // Look at sockets and see which ones have something to be read()
        int n = select(maxfdsServ + 1, &readSockets, NULL, &exceptSockets, NULL);
        std::cout << "after select" << std::endl;

        std::cout << "N: " << n << std::endl;

        
        // Get modifiable copy of readSockets
        readSockets = exceptSockets = openClientSockets;
        memset(buffer, 0, sizeof(buffer));

        // Look at sockets and see which ones have something to be read()
        int m = select(maxfdsCli + 1, &readSockets, NULL, &exceptSockets, NULL);
        

        // SERVERS
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

                serverSock = accept(listenSockServer, (struct sockaddr *)&server, &serverLen);

                // Add new client to the list of open sockets
                FD_SET(serverSock, &openServerSockets);

                // And update the maximum file descriptor
                maxfdsServ = std::max(maxfdsServ, serverSock);

                // create a new client to store information.
                servers[serverSock] = new Server(serverSock);

                // Decrement the number of sockets waiting to be dealt with
                n--;

                printf("Server connected to server: %d\n", serverSock);
            }

            
            // First, accept  any new connections to the server on the listening socket
            if (FD_ISSET(listenSockClient, &readSockets))
            {
                clientSock = accept(listenSockClient, (struct sockaddr *)&client, &clientLen);

                // Add new client to the list of open sockets
                FD_SET(clientSock, &openClientSockets);

                // And update the maximum file descriptor
                maxfdsCli = std::max(maxfdsCli, clientSock);

                // create a new client to store information.
                clients[clientSock] = new Client(clientSock);

                // Decrement the number of sockets waiting to be dealt with
                n--;
                printf("Client connected to server: %d\n", serverSock);
            }
            

            // Now check for commands from server
            while (n-- > 0)
            {
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

                            closeClient(server->sock, &openServerSockets, &maxfdsServ);
                        }
                        // We don't check for -1 (nothing received) because select()
                        // only triggers if there is something on the socket for us.
                        else
                        {
                            std::cout << buffer << std::endl;

                            serverCommand(server->sock, &openServerSockets, &maxfdsServ, buffer);
                        }
                    }
                }
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

                            closeClient(client->sock, &openClientSockets, &maxfdsCli);
                        }
                        // We don't check for -1 (nothing received) because select()
                        // only triggers if there is something on the socket for us.
                        else
                        {
                            std::cout << buffer << std::endl;
                            clientCommand(client->sock, &openClientSockets, &maxfdsCli, buffer);
                        }
                    }
                }
            }
        }
    }
}
*/
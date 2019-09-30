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
#include<stdio.h>
#include<winsock2.h>
#include<iostream>
//#include <cygwin/in.h>
#include <thread>
#include <sstream>


#pragma comment(lib, "ws2_32.lib") //Winsock Library
#define MAX_CLIENT 30
#define MAX_RECV 1024

int main(int argc , char *argv[])
{
    WSADATA wsa;
    SOCKET master , new_socket , client_socket[MAX_CLIENT] , s;
    struct sockaddr_in server, address;
    int max_clients = 30 , activity, addrlen, i, valread, numberofclients = 0;
    char message[MAX_RECV], status_message[MAX_RECV];
    boolean run = true;

    //Struct for select timeout
    struct __ms_timeval timeout;
    timeout.tv_usec = 5;
    timeout.tv_sec = 0;

    //set of socket descriptors
    fd_set readfds;
    //1 extra for null character, string termination
    char *buffer;
    buffer =  (char*) malloc((MAX_RECV + 1) * sizeof(char));

    //initialising sockets in socket array
    for(i = 0 ; i < 30;i++)
    {
        client_socket[i] = 0;
    }

    //Startup for Winsock
    printf("\nInitialising Winsock...");
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
    {
        printf("Failed. Error Code : %d",WSAGetLastError());
        exit(EXIT_FAILURE);
    }

    printf("Initialised.\n");

    //Create master socket
    if((master = socket(AF_INET , SOCK_STREAM , 0 )) == INVALID_SOCKET)
    {
        printf("Could not create socket : %d" , WSAGetLastError());
        exit(EXIT_FAILURE);
    }

    printf("Socket created.\n");

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 8888 );

    //Bind
    if( bind(master ,(struct sockaddr *)&server , sizeof(server)) == SOCKET_ERROR)
    {
        printf("Bind failed with error code : %d" , WSAGetLastError());
        exit(EXIT_FAILURE);
    }

    puts("Bind done");

    //Listen to incoming connections
    listen(master , 3);

    //Accept and incoming connection
    puts("Waiting for incoming connections...");

    addrlen = sizeof(struct sockaddr_in);

    //Lambda thread for server messages
    std::thread sender([&] {
        while (run) {
            std::cin.getline(message, 1024);

            //When message == q, runs et to false and stops the server
            if (*message != 'q') {
                for (int j = 0; j < MAX_CLIENT; j++) {
                    send(client_socket[j], message, strlen(message), 0);
                }
            } else {
                std::cout << "shutting down" << std::endl;
                run = false;
            }
        }
    });

    while(run)
    {

        //clear the socket fd set
        FD_ZERO(&readfds);

        //add master socket to fd set
        FD_SET(master, &readfds);

        //add child sockets to fd set
        for (  i = 0 ; i < MAX_CLIENT ; i++)
        {
            s = client_socket[i];
            if(s > 0)
            {
                FD_SET( s , &readfds);
            }
        }

        //wait for an activity on any of the sockets
        activity = select( 0 , &readfds , NULL , NULL , &timeout);
        if ( activity == SOCKET_ERROR )
        {
            printf("select call failed with error code : %d" , WSAGetLastError());
            exit(EXIT_FAILURE);
        }

        //If something happened on the master socket , then its an incoming connection
        if (FD_ISSET(master , &readfds))
        {
            if ((new_socket = accept(master , (struct sockaddr *)&address, (int *)&addrlen))<0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            //inform user of socket number - used in send and receive commands
            printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

            //increase number of clients and send a message to all clients
            numberofclients++;
            std::cout <<"Antall klienter: " << numberofclients << std::endl;

            snprintf(status_message,sizeof(status_message), "Antall klienter: %d",numberofclients);
            for(int j = 0; j < MAX_CLIENT; j++)
            {
                send( client_socket[j] , status_message , strlen(status_message) , 0 );
            }

            //send new connection greeting message
            if( send(new_socket, message, strlen(message), 0) != strlen(message) )
            {
                perror("send failed");
            }

            puts("Welcome message sent successfully");

            //add new socket to array of sockets
            for (i = 0; i < max_clients; i++)
            {
                if (client_socket[i] == 0)
                {
                    client_socket[i] = new_socket;
                    printf("Adding to list of sockets at index %d \n" , i);
                    break;
                }
            }
        }

        //else its some IO operation on some other socket :)
        for (i = 0; i < max_clients; i++)
        {
            s = client_socket[i];
            //if client presend in read sockets
            if (FD_ISSET( s , &readfds))
            {
                //get details of the client
                getpeername(s , (struct sockaddr*)&address , (int*)&addrlen);

                //Check if it was for closing , and also read the incoming message
                //recv does not place a null terminator at the end of the string (whilst printf %s assumes there is one).
                valread = recv( s , buffer, MAX_RECV, 0);

                if( valread == SOCKET_ERROR)
                {
                    int error_code = WSAGetLastError();
                    if(error_code == WSAECONNRESET)
                    {
                        //Somebody disconnected , get his details and print and gives new number of client message
                        printf("Host disconnected unexpectedly , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
                        numberofclients--;
                        std::cout << "Antall klienter: " << numberofclients << std::endl;

                        //Close the socket and mark as 0 in list for reuse
                        closesocket( s );
                        client_socket[i] = 0;
                    }
                    else
                    {
                        printf("recv failed with error code : %d" , error_code);
                    }
                }
                if ( valread == 0)
                {
                    //Somebody disconnected , get his details and print, new number of client message given
                    printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
                    numberofclients--;
                    std::cout << "Antall klienter: " << numberofclients << std::endl;

                    //Close the socket and mark as 0 in list for reuse
                    closesocket( s );
                    client_socket[i] = 0;
                }

                //Echo back the message that came in to all clients
                else
                {
                    //add null character, if you want to use with printf/puts or other string handling functions

                    buffer[valread] = '\0';

                    printf("%s:%d - %s \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port), buffer);
                    for(int j = 0; j < MAX_CLIENT; j++)
                    {
                        if(j != i)
                        {
                            send( client_socket[j] , buffer , valread + 1, 0 );
                        }
                    }
                }
            }
        }
    }
sender.join();

    //Shutdowns all connections

for(i = 0; i < numberofclients; i++) {
    std::cout << "Shutting down client: " << i << std::endl;
    shutdown(client_socket[i], SD_SEND);
}
    closesocket(s);
    WSACleanup();

    return 0;
}
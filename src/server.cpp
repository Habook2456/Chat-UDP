#include <string.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <iostream>
#include <vector>
#include "utils.h"

#define BUF_SIZE 1024
#define USERNAME_LEN 64

using namespace std;

struct client
{
    struct sockaddr_in address;
    string username;
};

// global variables
int sockfd;
std::vector<client> clientList;
char requestBuffer[BUF_SIZE];
char responseBuffer[BUF_SIZE];
char sender_name[USERNAME_LEN];

client player1, player2;

int clientCompare(struct sockaddr_in client1, struct sockaddr_in client2)
{
    return (memcmp(&client1, &client2, sizeof(struct sockaddr_in)) == 0);
}

void broadcast(struct sockaddr_in sender, int global)
{
    for (const client &cli : clientList)
    {
        if (!clientCompare(sender, cli.address) || global)
        {

            if ((sendto(sockfd, responseBuffer, strlen(responseBuffer), 0,
                        (struct sockaddr *)&cli.address, sizeof(struct sockaddr))) == -1)
            {
                perror("sendto");
                close(sockfd);
                exit(EXIT_FAILURE);
            }
        }
    }
}

void sendToClient(const client &recipient, const std::string &message)
{
    if (sendto(sockfd, message.c_str(), message.length(), 0,
               (struct sockaddr *)&recipient.address, sizeof(struct sockaddr)) == -1)
    {
        perror("sendto");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
}

bool isConnected(struct sockaddr_in newClient)
{
    for (const client &element : clientList)
    {
        if (clientCompare(element.address, newClient))
        {
            strncpy(sender_name, element.username.c_str(), USERNAME_LEN);
            return true;
        }
    }
    return false;
}

int connectClient(struct sockaddr_in newClient, char *username)
{
    cout << "Client connected: " << username << endl;

    for (const client &element : clientList)
    {
        if (element.username == username)
        {
            cout << "Cannot connect client, user already exists" << endl;
            return -1;
        }
    }

    client newClientInfo;
    newClientInfo.address = newClient;
    newClientInfo.username = string(username);
    clientList.push_back(newClientInfo);

    return 0;
}

int disconnectClient(struct sockaddr_in oldClient)
{
    cout << "Attempting to disconnect client" << endl;
    for (auto it = clientList.begin(); it != clientList.end(); ++it)
    {
        if (clientCompare(oldClient, it->address))
        {
            clientList.erase(it);
            cout << "Client disconnected" << endl;
            return 0;
        }
    }

    cout << "Client was not disconnected properly" << endl;
    return -1;
}

void printClientList()
{
    int count = 1;

    for (const client &cli : clientList)
    {
        cout << "Client " << count << endl;
        cout << cli.username << endl;
        cout << "ip: " << inet_ntoa(cli.address.sin_addr) << endl;
        cout << "port: " << ntohs(cli.address.sin_port) << endl;
        count++;
    }
}

void sendClientList(struct sockaddr_in sender)
{
    std::string block;
    for (const client &cli : clientList)
    {
        if (!clientCompare(sender, cli.address))
        {
            block = complete_digits(cli.username.length(), 2) + cli.username + block;
            /*
            strcat(responseBuffer, cli.username);
            */
        }
    }
    block = "L" + block;

    if ((sendto(sockfd, block.c_str(), block.length(), 0, (struct sockaddr *)&sender,
                sizeof(struct sockaddr))) == -1)
    {
        perror("sendto");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
}

bool findClient(string friendName, client &sender)
{
    for (const client &cli : clientList)
    {
        if (cli.username == friendName)
        {
            sender = cli;
            return true;
        }
    }
    return false;
}

int main()
{

    memset(requestBuffer, 0, BUF_SIZE);
    memset(responseBuffer, 0, BUF_SIZE);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd == -1)
    {
        close(sockfd);
        cerr << "Failed to get socket file descriptor" << endl;
        exit(EXIT_FAILURE);
    }

    int server_port = 1234;
    struct sockaddr_in server_addr;

    // Configurar la dirección del servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(server_addr.sin_zero), '\0', 8);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        close(sockfd);
        cerr << "Failed to bind on socket!" << endl;
        exit(EXIT_FAILURE);
    }

    system("clear");

    struct sockaddr_in client_addr;
    socklen_t client_addrLen = sizeof(client_addr);

    while (true)
    {
        memset(responseBuffer, 0, BUF_SIZE);
        ssize_t nbytes = recvfrom(sockfd, requestBuffer, sizeof(requestBuffer), 0, (struct sockaddr *)&client_addr, &client_addrLen);

        if (nbytes == -1)
        {
            perror("recvfrom");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        requestBuffer[nbytes] = '\0';

        char option = requestBuffer[0];

        if (isConnected(client_addr))
        {
            // get client username
            strcat(responseBuffer, sender_name);

            switch (option)
            {
            case 'M':

                memmove(responseBuffer + 1, responseBuffer, strlen(responseBuffer) + 1);
                responseBuffer[0] = 'M';

                strcat(responseBuffer, ": ");
                strcat(responseBuffer, requestBuffer + 1);
                broadcast(client_addr, 0);
                break;
            case 'L':
                sendClientList(client_addr);
                break;
            case 'P':
            {
                string friendName(requestBuffer + 1);
                cout << friendName << endl;

                client friendInfo;
                if (!findClient(friendName, friendInfo))
                {
                    cout << "client not found" << endl;
                    cout << "send message to " << sender_name << endl;
                    findClient(sender_name, friendInfo);
                    string tempBuffer = "SClient not found";
                    sendToClient(friendInfo, tempBuffer);

                    break;
                }
                cout << "Enviando invitacion" << endl;
                memmove(responseBuffer + 1, responseBuffer, strlen(responseBuffer) + 1);
                responseBuffer[0] = 'I';

                // invitar a jugar a cliente
                strcat(responseBuffer, " has invited you to play a game.\nTo accept the invitation, please press 3 and then respond with either YES or NO.");

                sendToClient(friendInfo, responseBuffer);

                // recibir respuesta
                char confirmationBuffer[BUF_SIZE];
                nbytes = recvfrom(sockfd, confirmationBuffer, sizeof(confirmationBuffer), 0, (struct sockaddr *)&client_addr, &client_addrLen);
                confirmationBuffer[nbytes] = '\0';
                cout << "Respuesta: " << confirmationBuffer << endl;

                player1 = friendInfo;
                findClient(sender_name, player2);

                cout << "PLAYER 1 INFO: " << inet_ntoa(player1.address.sin_addr) << ":" << ntohs(player1.address.sin_port) << endl;
                cout << "PLAYER 2 INFO: " << inet_ntoa(player2.address.sin_addr) << ":" << ntohs(player2.address.sin_port) << endl;

                if (strcmp(confirmationBuffer, "YES") == 0)
                {
                    cout << "INICIANDO JUEGO" << endl;
                    // iniciar juego
                    char gameBuffer[BUF_SIZE];
                    memset(gameBuffer, 0, BUF_SIZE);
                    strcat(gameBuffer, "IGAME ACCEPTED\nPress 0 to start the game\n");
                    sendToClient(player1, gameBuffer);
                    sendToClient(player2, gameBuffer);
                }
                else
                {
                    cout << "JUEGO CANCELADO" << endl;
                    break;
                }

                break;
            }
            case 'G':
            {
                cout << "JUEGO INICIADO" << endl;
                bool gameStarted = true;

                recvfrom(sockfd, requestBuffer, sizeof(requestBuffer), 0, (struct sockaddr *)&client_addr, &client_addrLen);
                requestBuffer[nbytes] = '\0';

                memset(responseBuffer, 0, BUF_SIZE);

                int round = 1;
                while (gameStarted)
                {
                    cout << "ROUND: " << round << endl;
                    // Recibir respuestas de player1
                    char opBuffer[BUF_SIZE];
                    nbytes = recvfrom(sockfd, opBuffer, sizeof(opBuffer), 0, (struct sockaddr *)&client_addr, &client_addrLen);
                    opBuffer[nbytes] = '\0';

                    if (nbytes == -1)
                    {
                        perror("recvfrom player1");
                        close(sockfd);
                        exit(EXIT_FAILURE);
                    }

                    cout << "OPCION RECIBIDA p1: " << opBuffer << endl;

                    int userp1Size = atoi(string(opBuffer, 0, 2).c_str());
                    string userP1 = string(opBuffer, 2, userp1Size);
                    int userp1Opt = atoi(string(opBuffer, 2 + userp1Size, 1).c_str());

                    nbytes = recvfrom(sockfd, opBuffer, sizeof(opBuffer), 0, (struct sockaddr *)&client_addr, &client_addrLen);
                    opBuffer[nbytes] = '\0';

                    if (nbytes == -1)
                    {
                        perror("recvfrom player1");
                        close(sockfd);
                        exit(EXIT_FAILURE);
                    }

                    cout << "OPCION RECIBIDA p2: " << opBuffer << endl;

                    int userp2Size = atoi(string(opBuffer, 0, 2).c_str());
                    string userP2 = string(opBuffer, 2, userp2Size);
                    int userp2Opt = atoi(string(opBuffer, 2 + userp2Size, 1).c_str());

                    cout << "Información recibida de player1:" << endl;
                    cout << "Tamaño del nombre: " << userp1Size << endl;
                    cout << "Nombre: " << userP1 << endl;
                    cout << "Opción: " << userp1Opt << endl;
                    cout << "Información recibida de player2:" << endl;
                    cout << "Tamaño del nombre: " << userp2Size << endl;
                    cout << "Nombre: " << userP2 << endl;
                    cout << "Opción: " << userp2Opt << endl;

                    // logica del juego
                    findClient(userP1, player1);
                    findClient(userP2, player2);
                    if (userp1Opt == userp2Opt)
                    {
                        // empate
                        // send message to players
                        string tempBuffer = "GEmpate!";
                        sendToClient(player1, tempBuffer);
                        sendToClient(player2, tempBuffer);
                    }
                    else if ((userp1Opt - userp2Opt) == 1)
                    {
                        // gana player1
                        string tempBuffer = "GGanaste!";
                        sendToClient(player1, tempBuffer);
                        tempBuffer = "SPerdiste!";
                        sendToClient(player2, tempBuffer);
                    }
                    else if ((userp1Opt - userp2Opt) == -2)
                    {
                        // gane player1
                        string tempBuffer = "GGanaste!";
                        sendToClient(player1, tempBuffer);
                        tempBuffer = "GPerdiste!";
                        sendToClient(player2, tempBuffer);
                    }
                    else
                    {
                        // gana player2
                        string tempBuffer = "GGanaste!";
                        sendToClient(player2, tempBuffer);
                        tempBuffer = "GPerdiste!";
                        sendToClient(player1, tempBuffer);
                    }
                    round++;
                    // gameStarted = false;
                }

                break;
            }
            default:
                break;
            }
            /*
            strcat(responseBuffer, sender_name);
            cout << "responseBuffer despues de strcat: " << responseBuffer << endl;
            if (strcmp("close", requestBuffer) == 0)
            {
                if (disconnectClient(client_addr) == 0)
                {
                    strcat(responseBuffer, " disconnected \n");
                    broadcast(client_addr, 1);
                }
                // printClientList();
            }
            else if (strcmp("exit", requestBuffer) == 0)
            {
                strcat(responseBuffer, "shutdown the server \n");
                broadcast(client_addr, 1);
                cout << "Exiting Server" << endl;
                close(sockfd);
                exit(0);
            }
            else if (strcmp("list", requestBuffer) == 0)
            {
                sendClientList(client_addr);
            }
            else
            {
                strcat(responseBuffer, ": ");
                strcat(responseBuffer, requestBuffer);
                // cout << "[" << responseBuffer << "]" << endl;
                broadcast(client_addr, 0);
            }
            */
        }
        else
        {
            connectClient(client_addr, requestBuffer);
            cout << "CONECTAR CLIENTE" << endl;
        }
    }

    close(sockfd);
    return 0;
}

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <string>
#include <vector>
#include <iostream>
#include "utils.h"

#define USERNAME_LEN 64
#define BUF_SIZE 1024

using namespace std;

int sockfd;
char sendBuffer[BUF_SIZE];
char receiveBuffer[BUF_SIZE];

// yan ken po
bool game = false;

void readMessage()
{
    int nbytes;

    while (1)
    {
        bzero(receiveBuffer, BUF_SIZE);

        if ((nbytes = recv(sockfd, receiveBuffer, BUF_SIZE - 1, 0)) == -1)
        {
            perror("recv");
        }

        receiveBuffer[nbytes] = '\0';

        char option = receiveBuffer[0];

        switch (option)
        {
        case 'L':
            if (nbytes > 1)
            {
                std::string clientListMessage(&receiveBuffer[1]); // Ignora el primer carácter 'L'

                int currentIndex = 0;
                std::cout << "Users connected: " << std::endl;

                while (currentIndex < clientListMessage.length())
                {
                    int usernameLength = std::stoi(clientListMessage.substr(currentIndex, 2));
                    currentIndex += 2;
                    std::string username = clientListMessage.substr(currentIndex, usernameLength);
                    currentIndex += usernameLength;

                    std::cout << "  - " << username << std::endl;
                }
            }
            else
            {
                std::cout << "No clients connected." << std::endl;
            }
            break;
        case 'M':
        {
            std::string message(&receiveBuffer[1]);
            std::cout << message << std::endl;
            // mensaje de respuesta del servidor

            break;
        }
        case 'I':
        {
            system("clear");
            std::string message(&receiveBuffer[1]);
            std::cout << message << std::endl;
            game = true;
            break;
        }
        case 'S':
        {
            std::string message(&receiveBuffer[1]);
            std::cout << message << std::endl;
            break;
        }
        case 'G':
        {
            std::string message(&receiveBuffer[1]);
            std::cout << message << std::endl;
            std::cout << "Press Enter to continue to the next round..." << std::endl;
            break;
        }
        default:
            break;
        }
    }
}

int main()
{
    memset(sendBuffer, 0, BUF_SIZE);
    memset(receiveBuffer, 0, BUF_SIZE);

    string username;

    system("clear");
    std::cout << "Welcome to the chat." << std::endl;
    std::cout << "Enter your nickname: ";
    std::getline(std::cin, username);

    const char *server_ip = "127.0.0.1";
    int serverPort = 1234;

    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1)
    {
        close(sockfd);
        fprintf(stderr, "Failed to get the socket file descriptor\n");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(serverPort);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    memset(&(server_addr.sin_zero), '\0', 8);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
    {
        close(sockfd);
        fprintf(stderr, "Failed to connect to the remote server\n");
        exit(EXIT_FAILURE);
    }

    strcpy(sendBuffer, username.c_str());
    if (send(sockfd, sendBuffer, strlen(sendBuffer), 0) == -1)
    {
        perror("send");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    thread(readMessage).detach();

    bool quit = true;
    while (quit)
    {
        memset(sendBuffer, 0, BUF_SIZE);
        if (!game)
        {
            std::cout << "Menu Options:\n"
                      << "(1) Write a message\n"
                      << "(2) Send an invitation to play Rock, Paper, Scissors\n"
                      << "(3) Accept an invitation to play Rock, Paper, Scissors\n"
                      << "(4) List\n"
                      << "(5) Exit" << std::endl;
        }

        int option;
        cin >> option;
        string block;
        switch (option)
        {
        case 1:
            std::cout << "Message: ";
            cin.ignore(); // Clear the keyboard buffer
            cin.getline(sendBuffer, BUF_SIZE);
            block = "M" + string(sendBuffer);
            if (send(sockfd, block.c_str(), block.length(), 0) == -1)
            {
                perror("send");
            }
            break;

        case 2:
            std::cout << "Play Rock, Paper, Scissors" << std::endl;

            std::cout << "Invite Friend: ";
            cin.ignore();
            cin.getline(sendBuffer, BUF_SIZE);
            block = "P" + string(sendBuffer);
            if (send(sockfd, block.c_str(), block.length(), 0) == -1)
            {
                perror("send");
            }

            break;
        case 3:
            if (game)
            {
                std::cout << "You have a pending invitation to play Rock, Paper, Scissors." << std::endl;
                std::cout << "Please enter your response: ";

                cin.ignore();
                cin.getline(sendBuffer, BUF_SIZE);

                if (send(sockfd, sendBuffer, strlen(sendBuffer), 0) == -1)
                {
                    perror("send");
                }
            }
            else
            {
                std::cout << "You don't have any pending invitations to play Rock, Paper, Scissors." << std::endl;
            }
            break;

        case 4:
            block = "L";
            if (send(sockfd, block.c_str(), block.length(), 0) == -1)
            {
                perror("send");
            }
            break;

        case 5:
            quit = false;
            break;

        case 0:
            std::cout << "GAME STARTED" << endl;
            sleep(2);
            // enviar señal de inicio de juego al servidor
            block = "G";
            if (send(sockfd, block.c_str(), block.length(), 0) == -1)
            {
                perror("send");
            }
            while (true)
            {
                system("clear");
                std::cout << "Elige una opción:" << std::endl;
                std::cout << "0. Piedra" << std::endl;
                std::cout << "1. Papel" << std::endl;
                std::cout << "2. Tijeras" << std::endl;

                int playerChoice;
                cin.ignore();
                std::cin >> playerChoice;

                if (playerChoice < 0 || playerChoice > 2)
                {
                    std::cout << "Opción inválida." << std::endl;
                    continue;
                }

                // enviar opcion
                std::string optionMessage = complete_digits(username.length(), 2) + username + std::to_string(playerChoice);
                cout << optionMessage << endl;
                if (send(sockfd, optionMessage.c_str(), optionMessage.length(), 0) == -1)
                {
                    perror("send");
                    // Puedes manejar el error aquí si es necesario.
                }
                // Pausa hasta que el usuario presione Enter
                cin.ignore(); // Descarta el carácter Enter presionado
                cin.get();
            }
            break;
        default:
            std::cout << "Invalid option." << std::endl;
        }

        // cout << "VARIABLE RESPONSE --> " << response << endl;
    }

    close(sockfd);
    std::cout << "Exiting the chat. Goodbye!" << std::endl;
    return 0;
}

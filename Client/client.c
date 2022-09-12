/**
 * @file client.c
 * @author Harshavardhan Naidu Gangavarapu (gangavah@uwindsor.ca) & Vamshy Pagadala (pagadal1@uwindsor.ca)
 * @brief FTP Client Simulation
 * @version 0.1
 * @date 2022-08-15
 *
 * @copyright Copyright (c) 2022 - COMP8567-3-R-2022S Advanced Systems Programming , School of Computer Science - University of Windsor.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// method declarations
void userNotLogged(char *buffer);
void downloadFileToClient(char *tempBuffer, char *buffer);

// bind to port 3111
#define PORT 3111

int main(int argc, char *argv[])
{
    // Intialize variables
    char buffer[1024];
    struct sockaddr_in serverAddressData;

    // create socket connection
    int ftpClientSocket = socket(AF_INET, SOCK_STREAM, 0);
    // on successful socket creation
    if (ftpClientSocket >= 0)
    {
        printf("Sucessfully created client socket connection...)\n");
    }
    // on failure to create socket connection
    else
    {
        printf("Failed to connect to ftp server...(\n");
        exit(1);
    }
    // clear the memory of address data
    memset(&serverAddressData, '\0', sizeof(serverAddressData));
    serverAddressData.sin_family = AF_INET;
    serverAddressData.sin_port = htons(PORT);
    serverAddressData.sin_addr.s_addr = inet_addr("127.0.0.1");

    // create the connection
    int ftpClientConnectionStatus = connect(ftpClientSocket, (struct sockaddr *)&serverAddressData, sizeof(serverAddressData));
    // if failed to connect
    if (ftpClientConnectionStatus <= -1)
    {
        printf("Failed to establish connection with ftp server...(\n");
        exit(1);
    }
    // On successful connection
    else
    {
        printf("Successfully connected to ftp server...)\n");
    }
    // loop infinte times
    while (1)
    {
        // Initalize variables
        char tempBuffer[1024];
        printf("$ ftp client: \t");
        // read the commands from the standard input
        fgets(buffer, 1024, stdin);
        // add the string null terminator
        buffer[strlen(buffer)] = '\0';
        // send the commands to the server
        send(ftpClientSocket, buffer, strlen(buffer), 0);
        // clear the tempbuffer and buffer char arrays
        bzero(tempBuffer, sizeof(tempBuffer));
        strcpy(tempBuffer, buffer);
        bzero(buffer, sizeof(buffer));
        // On receiving QUIT and ABOR command
        if (strncmp(tempBuffer, "QUIT", 4) == 0 || strncmp(tempBuffer, "quit", 4) == 0 || strncmp(tempBuffer, "ABOR", 4) == 0 || strncmp(tempBuffer, "abor", 4) == 0)
        {
            close(ftpClientSocket);
            printf("Successfully disconnected from ftp server...)\n");
            exit(1);
        }
        else
        {
            // recieve the meesage from server
            int clientRecieveStatus = recv(ftpClientSocket, buffer, 1024, 0);
            // on failure to receive
            if (clientRecieveStatus <= -1)
            {
                printf("Failed to receive data from ftp server...(\n");
            }
            // check for RETR/retr command to downlaod the file to the client
            else if (strncmp(tempBuffer, "RETR ", 5) == 0 || strncmp(tempBuffer, "retr ", 5) == 0)
            {
                // If user is not logged in
                if (strncmp(buffer, "Code[530]", 9) == 0)
                {
                    // print the user not logged error message
                    userNotLogged(buffer);
                }
                else
                {
                    // downlaod the file to the client
                    downloadFileToClient(tempBuffer, buffer);
                }
                // clear the buffer
                bzero(buffer, sizeof(buffer));
            }
            else
            {
                // for other commands display the response status code with message
                printf("$ ftp server: \t%s\n", buffer);
                bzero(buffer, sizeof(buffer));
            }
        }
    }
    return 0;
}

/**
 * @brief This method will download the file to the client from the ftp server.
 *
 * @param tempBuffer
 * @param buffer
 */
void downloadFileToClient(char *tempBuffer, char *buffer)
{
    tempBuffer[strcspn(tempBuffer, "\n")] = 0;
    // Intialize the variables
    char clientFilePath[256];
    // extract the fileName from the input
    char *fileName = malloc(strlen(tempBuffer) - 4);
    strncpy(fileName, tempBuffer + 5, strlen(tempBuffer) - 1);
    getcwd(clientFilePath, 256);
    strcat(clientFilePath, "/");
    strcat(clientFilePath, fileName);
    // open the destination file to write the data
    int destinationFileDesc = open(clientFilePath, O_CREAT | O_WRONLY, 0755);
    // on failure to open
    if (destinationFileDesc == -1)
    {
        printf("Code[348]: Failed to open %s file from ftp server...(\n", fileName);
    }
    else
    {
        // write the data to the file
        int noOfBytes = write(destinationFileDesc, buffer, strlen(buffer) - 2);
        // if failed to write
        if (noOfBytes == -1)
        {
            printf("Code[349]: Failed to download %s file from ftp server...(\n", fileName);
        }
        // on succesful file download, print the message
        else
        {
            printf("Code[200]: Successfully saved %s file to client...)\n", fileName);
        }
    }
    // close the file descriptor
    close(destinationFileDesc);
    // clear the char buffer
    bzero(clientFilePath, strlen(clientFilePath));
}

/**
 * @brief This method will print the user not logged error message
 *
 * @param buffer
 */
void userNotLogged(char *buffer)
{
    // clear the buffer
    memset(buffer, '\0', strlen(buffer));
    printf("Code[530]: No user logged in...:(\n");
}
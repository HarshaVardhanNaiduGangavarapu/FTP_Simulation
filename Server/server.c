/**
 * @file server.c
 * @author Harshavardhan Naidu Gangavarapu (gangavah@uwindsor.ca) & Vamshy Pagadala (pagadal1@uwindsor.ca)
 * @brief FTP Server Simulation
 * @version 0.1
 * @date 2022-08-15
 *
 * @copyright Copyright (c) 2022 - COMP8567-3-R-2022S Advanced Systems Programming , School of Computer Science - University of Windsor.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/dir.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>

// method declarations
void resetBufferMemory(char *buffer);
void pwdCommand(int serverSocket, char *buffer);
void mkdCommand(int serverSocket, char *buffer);
void rmdCommand(int serverSocket, char *buffer);
void quitCommand(struct sockaddr_in addressData);
void userCommand(int serverSocket, char *buffer);
void userNotLogged(int serverSocket, char *buffer);
void invalidCommand(int serverSocket, char *buffer);
void sentDataToClient(int serverSocket, char *buffer);
void cwdCommand(int serverSocket, char *buffer, char *serverHomeDirectory);
void storCommand(int serverSocket, char *buffer, char *serverHomeDirectory);
void listCommand(char *serverHomeDirectory, char *buffer, int serverSocket);
void retrCommand(int ftpServerSocket, char *buffer, char *serverHomeDirectory);

// bind the server with port 3111
#define PORT 3111

int main(int argc, char *argv[])
{
  // Intialize the varibales required for socket connection and socket communications
  int ftpServerSocket, serverSocketFileDesc;
  char buffer[1024];
  char *serverHomeDirectory;
  struct sockaddr_in ftpServerAddress;
  struct sockaddr_in addressData;
  socklen_t ftpServerSocketSize;
  pid_t serverChildProcessId;

  // Check conditions to make sure the client will start the server with required arguments
  if (argc != 3)
  {
    printf("Invalid command format. Please type in following format - %s [<./server> -d <HomeDirectory>]\n", argv[0]);
    exit(1);
  }

  // If 2nd arg is '-d' then server home directory will be 3rd arg
  if (strcmp(argv[1], "-d") != 0)
  {
    printf("Invalid command format. Please type in following format - %s [<./server> -d <HomeDirectory>]\n", argv[0]);
    exit(1);
  }
  else
  {
    // assign the home directory value
    serverHomeDirectory = argv[2];
  }

  // create the socket connection
  serverSocketFileDesc = socket(AF_INET, SOCK_STREAM, 0);

  // check for socket connection status and return respective messages
  if (serverSocketFileDesc >= 0)
  {
    printf("Server socket connection is success...:)\n");
  }
  else
  {
    printf("Unable to create connection...:(\n");
    exit(1);
  }

  // clear the ftpServerAddress memory before the initialization
  memset(&ftpServerAddress, '\0', sizeof(ftpServerAddress));
  ftpServerAddress.sin_family = AF_INET;
  ftpServerAddress.sin_port = htons(PORT);
  ftpServerAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

  // using the address data bind to the socket
  int serverBindStatus = bind(serverSocketFileDesc, (struct sockaddr *)&ftpServerAddress, sizeof(ftpServerAddress));
  // if binding is success
  if (serverBindStatus >= 0)
  {
    printf("Sucessfully binded to %d port...:)\n", PORT);
  }
  // display failed to bind error message
  else
  {
    printf("Failed to bind to %d port...:(\n", PORT);
    exit(1);
  }

  // start listening on the socket connection
  int listenStatus = listen(serverSocketFileDesc, 10);
  // If failed to listen
  if (listenStatus != 0)
  {
    printf("Failed to bind to %d port...:(\n", PORT);
    exit(1);
  }
  // On successful listening
  else
  {
    printf("Server is listening on %d port!\n", PORT);
  }

  // start accepting client connections
  while (1)
  {
    // accept the client connection
    ftpServerSocket = accept(serverSocketFileDesc, (struct sockaddr *)&addressData, &ftpServerSocketSize);
    // on failure to accept the connection
    if (ftpServerSocket <= -1)
    {
      exit(1);
    }
    // on successfull socket connection acceptance
    else
    {
      printf("Connection successfully accepted from Port: %d.\n", ntohs(addressData.sin_port));
    }
    // create a child process to run the FTP commands
    if ((serverChildProcessId = fork()) == 0)
    {
      // close the previous socket file descriptor
      close(serverSocketFileDesc);
      // initlaize user logged varibale to false
      int userLogged = 0;
      // run the FTP commands in a loop
      while (1)
      {
        int recieveStatus = recv(ftpServerSocket, buffer, 1024, 0);
        // check for FTP QUIT/quit and ABOR/abor command
        if (strncmp(buffer, "QUIT", 4) == 0 || strncmp(buffer, "quit", 4) == 0 || strncmp(buffer, "ABOR", 4) == 0 || strncmp(buffer, "abor", 4) == 0)
        {
          quitCommand(addressData);
          break;
        }
        else
        {
          // clear the newline terminator from buffer
          buffer[strcspn(buffer, "\n")] = 0;
          // check for used logged or not and throw error message
          if (userLogged == 0 && !(strncmp(buffer, "USER", 4) == 0 || strncmp(buffer, "user", 4) == 0))
          {
            userNotLogged(ftpServerSocket, buffer);
          }
          // If ftp command is USER/user
          else if (strncmp(buffer, "USER", 4) == 0 || strncmp(buffer, "user", 4) == 0)
          {
            userCommand(ftpServerSocket, buffer);
            // make user login flag to true
            userLogged = 1;
          }
          // If ftp command is PWD/pwd
          else if (strncmp(buffer, "PWD", 3) == 0 || strncmp(buffer, "pwd", 3) == 0)
          {
            pwdCommand(ftpServerSocket, buffer);
          }
          // If ftp command is LIST/list
          else if (strncmp(buffer, "LIST", 4) == 0 || strncmp(buffer, "list", 4) == 0)
          {
            listCommand(serverHomeDirectory, buffer, ftpServerSocket);
          }
          // If ftp command is MKD/mkd
          else if (strncmp(buffer, "MKD ", 4) == 0 || strncmp(buffer, "mkd ", 4) == 0)
          {
            mkdCommand(ftpServerSocket, buffer);
          }
          // If ftp command is RMD/rmd
          else if (strncmp(buffer, "RMD ", 4) == 0 || strncmp(buffer, "rmd ", 4) == 0)
          {
            rmdCommand(ftpServerSocket, buffer);
          }
          // If ftp command is CWD/cwd
          else if (strncmp(buffer, "CWD ", 4) == 0 || strncmp(buffer, "cwd ", 4) == 0)
          {
            cwdCommand(ftpServerSocket, buffer, serverHomeDirectory);
          }
          // If ftp command is STOR/stor
          else if (strncmp(buffer, "STOR ", 5) == 0 || strncmp(buffer, "stor ", 5) == 0)
          {
            storCommand(ftpServerSocket, buffer, serverHomeDirectory);
          }
          // If ftp command is RETR/retr
          else if (strncmp(buffer, "RETR ", 5) == 0 || strncmp(buffer, "retr ", 5) == 0)
          {
            retrCommand(ftpServerSocket, buffer, serverHomeDirectory);
          }
          // If other ftp commands and invalid commands are passed
          else
          {
            invalidCommand(ftpServerSocket, buffer);
          }
          // clear the buffer memory
          bzero(buffer, sizeof(buffer));
          resetBufferMemory(buffer);
        }
      }
    }
  }
  // close the socket
  close(ftpServerSocket);
  return 0;
}

/**
 * @brief This method will send message to client on quit command.
 *
 * @param addressData
 */
void quitCommand(struct sockaddr_in addressData)
{
  printf("Successfully disconnected client connection from Port: %d.\n", ntohs(addressData.sin_port));
}

/**
 * @brief This method will reset the character array to null termintaors
 *
 * @param buffer
 */
void resetBufferMemory(char *buffer)
{
  memset(buffer, '\0', strlen(buffer));
}

/**
 * @brief This method will send the message to the client with respective data
 *
 * @param ftpServerSocket
 * @param buffer
 */
void sentDataToClient(int ftpServerSocket, char *buffer)
{
  send(ftpServerSocket, buffer, strlen(buffer), 0);
}

/**
 * @brief This method will send message to client on user command.
 *
 * @param ftpServerSocket
 * @param buffer
 */
void userCommand(int ftpServerSocket, char *buffer)
{
  resetBufferMemory(buffer);
  strcpy(buffer, "Code[230]: User successfully logged in...:)");
  sentDataToClient(ftpServerSocket, buffer);
}

/**
 * @brief This method will send message to client id user not logged in.
 *
 * @param ftpServerSocket
 * @param buffer
 */
void userNotLogged(int ftpServerSocket, char *buffer)
{
  resetBufferMemory(buffer);
  strcpy(buffer, "Code[530]: No user logged in...:(");
  sentDataToClient(ftpServerSocket, buffer);
}

/**
 * @brief This method will send message to client on pwd command.
 *
 * @param ftpServerSocket
 * @param buffer
 */
void pwdCommand(int ftpServerSocket, char *buffer)
{
  // initlaize char array to hold path
  char pwdTemp[256];
  resetBufferMemory(buffer);
  // get current working directory
  if (getcwd(buffer, 1024) != NULL)
  {
    strcpy(pwdTemp, "Code[257]: ");
    strcat(pwdTemp, buffer);
    sentDataToClient(ftpServerSocket, pwdTemp);
    resetBufferMemory(pwdTemp);
  }
  // on failure
  else
  {
    strcpy(buffer, "Code[530]: Failed to fetch present working directory...:(");
    // send data to client
    sentDataToClient(ftpServerSocket, buffer);
  }
}

/**
 * @brief This method will send message to client on list command.
 *
 * @param serverHomeDirectory
 * @param buffer
 * @param ftpServerSocket
 */
void listCommand(char *serverHomeDirectory, char *buffer, int ftpServerSocket)
{
  // intialize variables
  struct dirent *directoryContent;
  DIR *directory;
  resetBufferMemory(buffer);
  // check if the directory is openable or not
  if ((directory = opendir(serverHomeDirectory)) == NULL)
  {
    strcpy(buffer, "Code[304]: Failed to fetch list...:(");
    sentDataToClient(ftpServerSocket, buffer);
  }
  // try to open the server current working directory
  else if ((directory = opendir("./")) == NULL)
  {
    strcpy(buffer, "Code[304]: Failed to fetch list using current directory...:(");
    sentDataToClient(ftpServerSocket, buffer);
  }
  // add folder contents to the buffer
  strcpy(buffer, "\nCode[250]:\n");
  while ((directoryContent = readdir(directory)) != NULL)
  {
    // check if contents are not of type. , .. and .DS_Store file.
    if ((strcmp(directoryContent->d_name, "..") != 0) && (strcmp(directoryContent->d_name, ".") != 0) && (strcmp(directoryContent->d_name, ".DS_Store") != 0))
    {
      strcat(buffer, "=> ");
      strcat(buffer, directoryContent->d_name);
      strcat(buffer, "\n");
    }
  }
  // send the data to client
  sentDataToClient(ftpServerSocket, buffer);
  // close directory
  closedir(directory);
}

/**
 * @brief This method will send message to client on mkd command.
 *
 * @param ftpServerSocket
 * @param buffer
 */
void mkdCommand(int ftpServerSocket, char *buffer)
{
  // extract the folder from input
  char *directoryName = malloc(strlen(buffer) - 4);
  strncpy(directoryName, buffer + 4, strlen(buffer) - 1);
  // create the directory
  mkdir(directoryName, 0755);
  resetBufferMemory(buffer);
  strcpy(buffer, "Code[336]: Successfully created directory ");
  strcat(buffer, directoryName);
  strcat(buffer, "...:)");
  // send message to client
  sentDataToClient(ftpServerSocket, buffer);
}

/**
 * @brief This method will send message to client on rmd command.
 *
 * @param ftpServerSocket
 * @param buffer
 */
void rmdCommand(int ftpServerSocket, char *buffer)
{
  // extract the folder from input
  char *directoryName = malloc(strlen(buffer) - 4);
  strncpy(directoryName, buffer + 4, strlen(buffer) - 1);
  // remove the directory
  rmdir(directoryName);
  resetBufferMemory(buffer);
  strcpy(buffer, "Code[344]: Successfully removed directory ");
  strcat(buffer, directoryName);
  strcat(buffer, "...:)");
  // send message to client
  sentDataToClient(ftpServerSocket, buffer);
}

/**
 * @brief This method will send message to client on cwd command.
 *
 * @param ftpServerSocket
 * @param buffer
 * @param serverHomeDirectory
 */
void cwdCommand(int ftpServerSocket, char *buffer, char *serverHomeDirectory)
{
  // exract the path from input
  char *directoryPath = malloc(strlen(buffer) - 4);
  strncpy(directoryPath, buffer + 4, strlen(buffer) - 1);
  resetBufferMemory(buffer);
  // if failed to change the directory
  if (chdir(directoryPath) == -1)
  {
    strcpy(buffer, "Code[341]: Failed to change directory to ");
    strcat(buffer, directoryPath);
    strcat(buffer, "...:(");
  }
  // if succesfully changed
  else
  {
    // change the home directory
    serverHomeDirectory = directoryPath;
    strcpy(buffer, "Code[200]: Successfully changed directory to ");
    strcat(buffer, directoryPath);
    strcat(buffer, "...:)");
  }
  // send message to client
  sentDataToClient(ftpServerSocket, buffer);
}

/**
 * @brief This method on revieving stor command will upload the file on to the server.
 *
 * @param ftpServerSocket
 * @param buffer
 * @param serverHomeDirectory
 */
void storCommand(int ftpServerSocket, char *buffer, char *serverHomeDirectory)
{
  // Intialize variables
  int size, noOfBytes;
  char fileContent[1024];
  // read the current working directory
  getcwd(serverHomeDirectory, 1024);
  strcat(serverHomeDirectory, "/");
  // extract the file path from buffer
  char *sourceFilePath = malloc(strlen(buffer) - 4);
  strncpy(sourceFilePath, buffer + 5, strlen(buffer) - 1);
  // extract fileName
  char *sourceFileName = basename(sourceFilePath);
  // create the destination file path
  char *destinationFilePath = malloc(strlen(serverHomeDirectory) + strlen(sourceFileName) + 1);
  strcpy(destinationFilePath, serverHomeDirectory);
  strcat(destinationFilePath, sourceFileName);
  // open source file in read only mode
  int sourceFileDesc = open(sourceFilePath, O_RDONLY, 0777);
  // open destination folder in create or write mode
  int destinationFileDesc = open(destinationFilePath, O_CREAT | O_WRONLY, 0755);
  while (1)
  {
    // read the data from source file
    noOfBytes = read(sourceFileDesc, fileContent, 1024);
    // if failed to read data from file
    if (noOfBytes == -1)
    {
      resetBufferMemory(buffer);
      strcpy(buffer, "Code[350]: Failed to read file ");
      strcat(buffer, sourceFileName);
      strcat(buffer, "...:(");
      strcat(buffer, serverHomeDirectory);
      // send message to client
      sentDataToClient(ftpServerSocket, buffer);
      break;
    }
    size = noOfBytes;
    if (size == 0)
      break;
    // write the data to the file
    noOfBytes = write(destinationFileDesc, fileContent, size);
    // on failure to write data
    if (noOfBytes == -1)
    {
      resetBufferMemory(buffer);
      strcpy(buffer, "Code[349]: Failed to write to file ");
      strcat(buffer, sourceFileName);
      strcat(buffer, "...:(");
      // send message to client
      sentDataToClient(ftpServerSocket, buffer);
      break;
    }
    else
    {
      resetBufferMemory(buffer);
      strcpy(buffer, "Code[200]: Sucessfully saved file ");
      strcat(buffer, sourceFileName);
      strcat(buffer, " to server...:)");
      // send message to client
      sentDataToClient(ftpServerSocket, buffer);
    }
  }
  // close the file descriptors
  close(sourceFileDesc);
  close(destinationFileDesc);
}

/**
 * @brief This method will downlaod the file to the client on recieving retr command
 *
 * @param ftpServerSocket
 * @param buffer
 * @param serverHomeDirectory
 */
void retrCommand(int ftpServerSocket, char *buffer, char *serverHomeDirectory)
{
  // intialize variables
  int size, noOfBytes;
  char fileContent[1024];
  // allocate file path memory size
  char *fileName = malloc(strlen(buffer) - 4);
  // copy the file path
  strncpy(fileName, buffer + 5, strlen(buffer) - 1);
  // allocate file path memory size pointing to the server
  char *filePathInServer = malloc(strlen(serverHomeDirectory) + strlen(fileName) + 1);
  strcpy(filePathInServer, serverHomeDirectory);
  strcat(filePathInServer, fileName);
  // open the file
  int serverFileDesc = open(filePathInServer, O_RDONLY, 0777);
  while (1)
  {
    // read the file contents
    noOfBytes = read(serverFileDesc, fileContent, 1024);
    // if failed to read file, send the error meesage
    if (noOfBytes == -1)
    {
      resetBufferMemory(buffer);
      strcpy(buffer, "code[350]: Failed to read from file ");
      strcat(buffer, fileName);
      strcat(buffer, " present in server...:(");
      // send message to client
      sentDataToClient(ftpServerSocket, buffer);
      break;
    }
    size = noOfBytes;
    // if size is zero, file read completly
    if (size == 0)
      break;
    else
    {
      resetBufferMemory(buffer);
      strcpy(buffer, fileContent);
      // send file content to the client to download
      sentDataToClient(ftpServerSocket, buffer);
    }
  }
  // close the file descriptor
  close(serverFileDesc);
}

/**
 * @brief This method will be invoked on receiving invalid command or commands which were not implemented.
 *
 * @param ftpServerSocket
 * @param buffer
 */
void invalidCommand(int ftpServerSocket, char *buffer)
{
  resetBufferMemory(buffer);
  strcpy(buffer, "Code[503]: Invalid Command...( Try again with valid commands...:)");
  // send message to client
  sentDataToClient(ftpServerSocket, buffer);
}
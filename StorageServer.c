#include "defs.h"

int portForNSBind;
int myIndex;
int portForSelfBind;
pthread_t NMthread;

sem_t mutex;
sem_t readerLock[capacity], writerLock[capacity];
int readerval[capacity], writerval[capacity];

char *parsePath(char *path, char *homeDirectory, char *parsedPath);
int readDir(char *path, char *outputBuffer);
int writeDir(char *path, char *mode, char *writeBuffer);
int SPout(char *path, char *permsBuffer);
int deleteFileDir(char *path);
int createFilDir(char *path, int isFile);
void *sourceOfCopy(void *arg);
void *destOfCopy(void *arg);
int recurfile(int socket, int isFile, char *givenPath, char *pathToRemove);
int filter(const struct dirent *entry);
int readPacketDir(char *path, char *outputBuffer, int idx);
void signalHandler(int signal);
int isFileOrDir(char *path);

int hashingFunc(char *s)
{
    long long bigNumber = 1e17;
    int n = strlen(s);
    long long hash = 1;
    for (int i = 0; i < n; i++)
    {
        hash = (hash * PRIME) % bigNumber;
        hash = (hash + s[i]) % bigNumber;
    }
    return (int)(hash % capacity);
}

// dir returns 2, file returns 1
int isFileOrDir(char *path)
{
    struct stat fileStat;

    if (stat(path, &fileStat) == -1)
    {
        return 0;
    }

    if (S_ISREG(fileStat.st_mode))
    {
        return 1;
    }
    else if (S_ISDIR(fileStat.st_mode))
    {
        return 2;
    }
    return 0;
}

int filter(const struct dirent *entry)
{
    return (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0);
}

int alphasort(const struct dirent **a, const struct dirent **b)
{
    return strcoll((*a)->d_name, (*b)->d_name);
}

int recurfile(int socket, int isFile, char *givenPath, char *pathToRemove) ////////////////////////////////////////
{
    dataStruct recurData;
    recurData.datF = cpyReq;
    recurData.destF = StoN;
    char pathToSend[MAXSIZES];
    char buffer[MAXSIZES + 1];
    struct stat fileStat;

    if (stat(givenPath, &fileStat) == -1)
    {
        perror("Error in stat");
        return 1;
    }

    if (S_ISREG(fileStat.st_mode))
    {
        isFile = 1;
    }
    else if (S_ISDIR(fileStat.st_mode))
    {
        isFile = 0;
    }
    if (isFile)
    {
        int i = 0;
        int n;

        while ((n = readPacketDir(givenPath, buffer, i)) == MAXSIZES)
        {
            strcpy(pathToSend, givenPath + strlen(pathToRemove));
            strcpy(recurData.stringData[0], pathToSend);
            strcpy(recurData.stringData[1], buffer);
            recurData.intData[0] = isFile;
            recurData.intData[1] = i;
            recurData.intData[2] = 0; // not last write
            send(socket, &recurData, sizeof(recurData), 0);
            i++;
        }
        strcpy(pathToSend, givenPath + strlen(pathToRemove));
        strcpy(recurData.stringData[0], pathToSend);
        strcpy(recurData.stringData[1], buffer);
        recurData.intData[0] = isFile;
        recurData.intData[1] = i;
        recurData.intData[2] = 1; // last write
        send(socket, &recurData, sizeof(recurData), 0);
    }
    else
    {
        strcpy(pathToSend, givenPath + strlen(pathToRemove));
        strcpy(recurData.stringData[0], pathToSend);
        recurData.intData[0] = isFile;
        send(socket, &recurData, sizeof(recurData), 0);
        struct dirent **entries;
        int numEntries = scandir(givenPath, &entries, filter, alphasort);

        if (numEntries >= 0)
        {
            for (int i = 0; i < numEntries; ++i)
            {
                char path[MAXSIZES];
                snprintf(path, sizeof(path), "%s/%s", givenPath, entries[i]->d_name);
                struct stat statbuf;

                if (stat(path, &statbuf) == 0)
                {
                    if (S_ISDIR(statbuf.st_mode))
                    {
                        recurfile(socket, 0, path, pathToRemove);
                    }
                    else if (S_ISREG(statbuf.st_mode))
                    {
                        recurfile(socket, 1, path, pathToRemove);
                    }
                    else
                    {
                        perror("unknown file type");
                    }
                }
                else
                {
                    perror("Error reading entry");
                }
                free(entries[i]);
            }

            free(entries);
        }
        else
        {
            perror("Error scanning directory");
            exit(EXIT_FAILURE);
        }
    }
}

void *sourceOfCopy(void *arg) /////////////////////////////
{
    dataStruct argument = *(dataStruct *)arg;
    int PORT = argument.intData[2];
    int isFile = argument.intData[0];
    char givenPath[MAXSIZES];
    strcpy(givenPath, argument.stringData[0]);
    char pathToRemove[MAXSIZES];
    char *temp = strrchr(givenPath, '/');
    temp[0] = '\0';
    strcpy(pathToRemove, givenPath);
    strcat(pathToRemove, "/");
    temp[0] = '/';

    int server_socket, Socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_len = sizeof(client_address);

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Bind socket to an address and port
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(IP);
    server_address.sin_port = htons(PORT);
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        perror("bind");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    if (listen(server_socket, MAX_CONNECTIONS) == -1)
    {
        perror("listen");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    sem_wait(&mutex);
    printf("Server is listening on port %d...\n", PORT);
    sem_post(&mutex);

    // Accept and handle incoming connections

    Socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);
    if (Socket == -1)
    {
        perror("accept");
        return NULL;
    }
    recurfile(Socket, isFile, givenPath, pathToRemove);
    dataStruct endconv;
    endconv.datF = conditionCode;
    endconv.destF = STOP;
    send(Socket, &endconv, sizeof(endconv), 0);
    close(Socket);
    close(server_socket);
}

void *destOfCopy(void *arg) ///////////////////////////////////////
{
    dataStruct argument = *(dataStruct *)arg;
    int PORT = argument.intData[2];

    int server_socket, Socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_len = sizeof(client_address);

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Bind socket to an address and port
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(IP);
    server_address.sin_port = htons(PORT);
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        perror("bind");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    if (listen(server_socket, MAX_CONNECTIONS) == -1)
    {
        perror("listen");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    sem_wait(&mutex);
    printf("Server is listening on port %d...\n", PORT);
    sem_post(&mutex);

    // Accept and handle incoming connections
    Socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);
    if (Socket == -1)
    {
        perror("accept");
        return NULL;
    }
    while (1)
    {
        dataStruct instruction;
        int n;
        if ((n = recv(Socket, &instruction, sizeof(instruction), 0)) == -1)
        {
            perror("recv");
            continue;
        }
        if (instruction.datF == conditionCode && instruction.destF == STOP)
            break;
        int isFile = instruction.intData[0];
        char givenPath[MAXSIZES];
        strcpy(givenPath, instruction.stringData[0]);
        if (isFile)
        {
            char content[MAXSIZES + 1];
            strcpy(content, instruction.stringData[1]);
            if (instruction.intData[1] == 0)

                writeDir(givenPath, "w", content);
            else
                writeDir(givenPath, "a", content);
        }
        else
        {
            createFilDir(givenPath, 0);
        }
    }
    close(Socket);
    close(server_socket);
}

void *ListenOnAPort(void *arg)
{
    int PORT = *(int *)arg;

    int server_socket, Socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_len = sizeof(client_address);

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Bind socket to an address and port
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(IP);
    server_address.sin_port = htons(PORT);
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        perror("bind");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    if (listen(server_socket, MAX_CONNECTIONS) == -1)
    {
        perror("listen");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    sem_wait(&mutex);
    printf("Server is listening on port %d...\n", PORT);
    sem_post(&mutex);

    // Accept and handle incoming connections
    while (1)
    {
        Socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);
        if (Socket == -1)
        {
            perror("accept");
            continue; // Continue to the next iteration of the loop
        }
        dataStruct instruction;
        int n;
        if ((n = recv(Socket, &instruction, sizeof(instruction), 0)) == -1)
        {
            perror("recv failed");
            continue;
        }
        // create Thread for client comm for RWSP
        if (instruction.destF == NtoS && (instruction.datF == readReq || instruction.datF == writeReq || instruction.datF == getSP))
        {
            int portForClient = instruction.intData[0];
            char path[MAXSIZES];
            strcpy(path, instruction.stringData[0]);
            char absPath[MAXSIZES];
            parsePath(path, NULL, absPath);
            strcpy(path, absPath);
            int n = isFileOrDir(path);
            dataStruct fileavaila;
            if (n == 0)
            {
                printf("bad file informed NS\n");
                fileavaila.destF = StoN;
                fileavaila.datF = conditionCode;
                fileavaila.intData[0] = BADFILEPATH;
            }
            if (n == 2)
            {
                fileavaila.destF = StoN;
                fileavaila.datF = conditionCode;
                fileavaila.intData[0] = EXPPATHDIR;
            }
            else
            {
                fileavaila.destF = StoN;
                fileavaila.datF = conditionCode;
                fileavaila.intData[0] = SUCCESS;
            }
            send(Socket, &fileavaila, sizeof(fileavaila), 0);
            sem_wait(&mutex);
            printf("Client will connect over port %d\n", portForClient);
            sem_post(&mutex);
            pthread_t thread;
            pthread_create(&thread, NULL, ListenOnAPort, &portForClient);
        }
        // create Thread for NM comm CreateDelCopy
        if (instruction.destF == NtoS && (instruction.datF == portbhejrhahoon))
        {
            int portForNM = instruction.intData[0];
            sem_wait(&mutex);
            printf("NS will connect over port %d\n", portForNM);
            sem_post(&mutex);
            pthread_t thread;
            pthread_create(&thread, NULL, ListenOnAPort, &portForNM);
        }
        // talking to NM for priority operations
        if (instruction.destF == NtoS && (instruction.datF == createReq || instruction.datF == deltReq || instruction.datF == cpyReq))
        {
            if (instruction.datF == deltReq)
            {
                char path[MAXSIZES];
                strcpy(path, instruction.stringData[0]);
                char absPath[MAXSIZES];
                parsePath(path, NULL, absPath);
                strcpy(path, absPath);
                deleteFileDir(path);
                instruction.intData[0] = SUCCESS;
                instruction.intData[1] = myIndex;
                send(Socket, &instruction, sizeof(instruction), 0);
            }
            if (instruction.datF == createReq)
            {
                char path[(MAXSIZES + 1) * 2];
                sprintf(path, "%s/%s", instruction.stringData[0], instruction.stringData[1]);
                char absPath[MAXSIZES];
                parsePath(path, NULL, absPath);
                strcpy(path, absPath);
                int isFile = instruction.intData[1];
                createFilDir(path, isFile);
                instruction.intData[0] = SUCCESS;
                instruction.intData[1] = myIndex;

                send(Socket, &instruction, sizeof(instruction), 0);
            }
            if (instruction.datF == cpyReq)
            {
                dataStruct *argStruct = (dataStruct *)malloc(sizeof(dataStruct));
                argStruct->datF = instruction.datF;
                argStruct->destF = instruction.destF;
                argStruct->intData[0] = instruction.intData[0];
                argStruct->intData[1] = instruction.intData[1];
                argStruct->intData[2] = instruction.intData[2];
                strcpy(argStruct->stringData[0], instruction.stringData[0]);
                pthread_t threadd;
                if (argStruct->intData[1] == TUSRCHAI)
                {
                    pthread_create(&threadd, NULL, sourceOfCopy, argStruct);
                }
                else if (argStruct->intData[1] == TUDESTHAI)
                {
                    pthread_create(&threadd, NULL, destOfCopy, argStruct);
                }
            }
        }
        // Read Write GetSP
        if (instruction.destF == CtoS && (instruction.datF == readReq || instruction.datF == writeReq || instruction.datF == getSP))
        {
            char path[MAXSIZES];
            char outputBuff[MAXSIZES + 1];
            int success;
            strcpy(path, instruction.stringData[0]);
            char temPath[MAXSIZES * 2];
            strcpy(temPath, path);
            if (instruction.intData[1] == COPYFLAG)
            {
                sprintf(temPath, "COPY%d/%s", myIndex, path);
            }
            strcpy(path, temPath);
            char absPath[MAXSIZES];
            parsePath(path, NULL, absPath);
            strcpy(path, absPath);
            int toLock = hashingFunc(path);
            if (instruction.datF == writeReq)
            {
                char mode[10];
                strncpy(mode, instruction.stringData[1], 10);
                char writeBuffer[MAXSIZES];
                writerval[toLock]++;
                if (readerval[toLock] != 0)
                    sem_wait(&readerLock[toLock]);
                sem_wait(&writerLock[toLock]);
                strcpy(writeBuffer, instruction.stringData[2]);
                sem_post(&writerLock[toLock]);

                success = writeDir(path, mode, writeBuffer);
            }
            else if (instruction.datF == readReq)
            {

                int i = 0;
                int n;
                readerval[toLock]++;
                while (n = readPacketDir(path, outputBuff, i) == MAXSIZES)
                {
                    if (n <= 0)
                        break;
                    instruction.datF = readReq;
                    strcpy(instruction.stringData[0], outputBuff);
                    send(Socket, &instruction, sizeof(instruction), 0);
                    i++;
                }
                readerval[toLock]--;
                if (readerval[toLock] == 0)
                {
                    while (writerval[toLock] > 0)
                    {
                        writerval[toLock]--;
                        sem_post(&readerLock[toLock]);
                    }
                }

                instruction.datF = readReq;
                strcpy(instruction.stringData[0], outputBuff);
                send(Socket, &instruction, sizeof(instruction), 0);
                success = 1;
                instruction.datF = conditionCode;
            }
            else if (instruction.datF == getSP)
            {
                success = SPout(path, outputBuff);
            }
            if (success != 0)
            {
                instruction.datF = conditionCode;
                instruction.intData[0] = RWSERROR;
            }
            else
            {
                instruction.intData[0] = SUCCESS;
                if (instruction.datF == readReq || instruction.datF == getSP)
                {
                    strcpy(instruction.stringData[0], outputBuff);
                }
            }
            instruction.destF = STOP;
            int n = send(Socket, &instruction, sizeof(instruction), 0);
            if (n < 0)
            {
                perror("Cudn't Send");
            }
            close(Socket);
            break;
        }

        // Close the client socket
        close(Socket);
    }

    // Close the server socket
    close(server_socket);
    sem_wait(&mutex);
    printf("Closed the Socket for Client Instance\n");
    sem_post(&mutex);

    return 0;
}

void signalHandler(int signal)
{
    printf("\nInterrupt signal received (%d).Reaching put to NM and exiting.\n", signal);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portForNSBind);
    inet_pton(AF_INET, IP, &(server_addr.sin_addr));
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    dataStruct dyingData;
    dyingData.datF = imdying;
    dyingData.destF = StoN;
    dyingData.intData[0] = myIndex;
    send(sockfd, &dyingData, sizeof(dyingData), 0);
    close(sockfd);
    char path[MAXSIZES];
    sprintf(path, "COPY%d", myIndex);
    char absPath[MAXSIZES];
    parsePath(path, NULL, absPath);
    deleteFileDir(absPath);
    exit(EXIT_SUCCESS);
}

int main()
{
    sem_init(&mutex, 0, 1);
    signal(SIGINT, signalHandler);
    for (int i = 0; i < capacity; i++)
    {
        sem_init(&readerLock[i], 0, 0);
        sem_init(&writerLock[i], 0, 1);
        readerval[i] = 0;
        writerval[i] = 0;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(NS_MASTER_PORT);
    inet_pton(AF_INET, IP, &(server_addr.sin_addr));

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Communication with the NS
    dataStruct initialRequest;
    initialRequest.datF = conditionCode;
    initialRequest.destF = StoN;
    initialRequest.intData[0] = SS_INIT;
    char absPath[MAXSIZES];
    getcwd(absPath, MAXSIZES);
    strcpy(initialRequest.stringData[0], absPath);

    printf("sending Data\n");

    if (send(sockfd, &initialRequest, sizeof(initialRequest), 0) == -1)
    {
        perror("send");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_received = recv(sockfd, &initialRequest, sizeof(initialRequest), 0);
    if (bytes_received == -1)
    {
        perror("recv");
    }
    else
    {
        if (initialRequest.datF == conditionCode && initialRequest.destF == NtoS)
        {
            portForNSBind = initialRequest.intData[1];
            portForSelfBind = initialRequest.intData[0];
            myIndex = initialRequest.intData[2];

            int pid = fork();
            if (pid == 0)
            {
                char path[100];
                sprintf(path, "COPY%d", myIndex);
                char *arg[] = {"mkdir", "-p", path, NULL};
                execvp(arg[0], arg);
                exit(0);
            }
            wait(0);

            printf("Recieved Port: %d and %d\nMy Index: %d\n", portForSelfBind, portForNSBind, myIndex);
        }
        else
        {
            perror("handle later");
        }
    }
    close(sockfd);
    // List of paths
    int numPaths;
    char path[MAXSIZES];
    pthread_create(&NMthread, NULL, ListenOnAPort, &portForSelfBind);
    usleep(100);
    sem_wait(&mutex);
    printf("Enter number of Paths: ");
    sem_post(&mutex);
    scanf("%d", &numPaths);
    for (int i = 0; i < numPaths; i++)
    {
        sem_wait(&mutex);
        printf("Enter the Paths:");
        sem_post(&mutex);
        scanf("%s", path);
        // char absPath[MAXSIZES];
        // parsePath(path, NULL, absPath);
        // strcpy(path, absPath);
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);

        if (sockfd == -1)
        {
            perror("socket");
            exit(EXIT_FAILURE);
        }

        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(portForNSBind);
        inet_pton(AF_INET, IP, &(server_addr.sin_addr));

        // Connect to the server
        if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        {
            perror("connect");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        // Communication with the NS
        dataStruct PathData;
        PathData.datF = conditionCode;
        PathData.destF = StoN;
        PathData.intData[0] = SS_DEFPATH;
        PathData.intData[1] = myIndex;
        strcpy(PathData.stringData[0], path);

        printf("sending this Data to NM\n");

        if (send(sockfd, &PathData, sizeof(PathData), 0) == -1)
        {
            perror("send");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        close(sockfd);
    }
    while (1)
        ;
    return 0;
}

char *parsePath(char *path, char *homeDirectory, char *parsedPath)
{
    char curDirectory[MAXSIZES];
    getcwd(curDirectory, MAXSIZES);
    if (homeDirectory == NULL)
    {
        homeDirectory = getenv("HOME");
    }
    if (parsedPath == NULL)
    {
        parsedPath = (char *)malloc(sizeof(char) * MAXSIZES);
        parsedPath[0] = '\0';
    }
    strcpy(parsedPath, curDirectory);
    if (path[0] == '/')
    {
        strcpy(parsedPath, "/");
    }
    char delim[] = "/\n";
    char *token = strtok(path, delim);
    while (token != NULL)
    {
        if (strcmp(token, "..") == 0)
        {
            if (strcmp(token, "/home") == 0 || strcmp(token, "/") == 0)
            {
                strcpy(parsedPath, "/");
                continue;
            }

            char *lastSlash = strrchr(parsedPath, '/');

            if (lastSlash != NULL)
            {
                *lastSlash = '\0';
            }
        }
        else if (strcmp(token, ".") == 0)
        {
            strcpy(parsedPath, curDirectory);
            token = strtok(NULL, delim);
            continue;
        }
        else if (strcmp(token, "~") == 0)
        {
            strcpy(parsedPath, homeDirectory);
        }
        else
        {
            if (parsedPath[strlen(parsedPath) - 1] != '/')
                strcat(parsedPath, "/");
            strcat(parsedPath, token);
        }
        token = strtok(NULL, delim);
    }
    char *ret = strdup(parsedPath);
    return ret;
}

int readDir(char *path, char *outputBuffer)
{
    FILE *file = fopen(path, "r");
    fseek(file, 0, SEEK_END);
    int size = (int)ftell(file);
    rewind(file);

    int bitesRead = fread(outputBuffer, 1, (size_t)size, file);
    if (bitesRead != size)
    {
        perror("read Error");
        fclose(file);
        return -1;
    }
    outputBuffer[size] = '\0';
    fclose(file);
    return 0;
}

int readPacketDir(char *path, char *outputBuffer, int idx)
{
    FILE *file;
    file = fopen(path, "r");
    if (file == NULL)
    {
        perror("fopen");
        return FAILURE;
    }
    fseek(file, MAXSIZES * idx, SEEK_SET);
    int bitesRead = fread(outputBuffer, sizeof(char), (MAXSIZES), file);
    if (bitesRead < 0)
    {
        perror("read Error");
        fclose(file);
        return -1;
    }
    outputBuffer[bitesRead] = '\0';
    fclose(file);
    return bitesRead;
}

int writeDir(char *path, char *mode, char *writeBuffer)
{
    // printf("-%s-%s-%s-", path, mode, writeBuffer);
    FILE *file = fopen(path, mode);
    int size = strlen(writeBuffer);
    int bytesWritten = fwrite(writeBuffer, 1, size, file);

    if (bytesWritten != size)
    {
        perror("Error fwrite");
        fclose(file);
        return -1;
    }
    fclose(file);
    return 0;
}
int SPout(char *path, char *permsBuffer)
{
    struct stat fileStat;

    if (stat(path, &fileStat) == 0)
    {
        sprintf(permsBuffer, "Size: %ld bytes\nPermissions: %o\n", fileStat.st_size, fileStat.st_mode & 0777);
        // printf("**%s**\n", permsBuffer);
    }
    else
    {
        perror("Error getting file information");
        return -1;
    }

    return 0;
}

int createFilDir(char *path, int isFile)
{
    if (isFile)
    {
        char *temp = (char *)malloc(sizeof(char) * (strlen(path) + 1));
        strcpy(temp, path);
        char *lastSlash = strrchr(temp, '/');
        if (lastSlash)
        {
            lastSlash[0] = '\0';
            char *cmd[] = {"mkdir", "-p", temp, NULL};
            int pid = fork();
            if (pid == 0)
            {
                if (execvp(cmd[0], cmd) == -1)
                {
                    perror("execvp");
                    exit(0);
                }
            }
            else
                wait(0);
        }
        char *cmd[] = {"touch", path, NULL};
        int pid = fork();
        if (pid == 0)
        {
            if (execvp(cmd[0], cmd) == -1)
            {
                perror("execvp");
                exit(0);
            }
        }
        else
            wait(0);
    }
    else
    {
        char *cmd[] = {"mkdir", "-p", path, NULL};
        int pid = fork();
        if (pid == 0)
        {
            if (execvp(cmd[0], cmd) == -1)
            {
                perror("execvp");
                exit(0);
            }
        }
        else
            wait(0);
    }
}

int deleteFileDir(char *path)
{
    char *cmd[] = {"rm", "-r", path, NULL};
    int pid = fork();
    if (pid == 0)
    {
        if (execvp(cmd[0], cmd) == -1)
        {
            perror("execvp");
            exit(0);
        }
    }
    else
        wait(0);
    return 0;
}

// int deleteAPath(char *pathToDelete)
// {
//     int req_SS = searchPath(pathToDelete, h);
//     StorageServer *Temp = listOfSS;
//     char temp_path[MAXSIZES];
//     int port_to_send_to;
//     int valid = 0;
//     if (req_SS != -1)
//     {
//         while (Temp->next != NULL)
//         {
//             if (Temp->next->id == req_SS)
//             {
//                 strcpy(temp_path, Temp->next->path);
//                 port_to_send_to = Temp->next->portForSSBind;
//                 break;
//             }
//             Temp = Temp->next;
//         }
//     }
//     strcat(temp_path, "/");
//     strcat(temp_path, pathToDelete);
//     printf("--%s--\n", pathToDelete);

//     // if (isFileOrDir(temp_path) == -1)
//     // {
//     //     perror("File path not valid!");
//     //     printf("Informing client...\n");
//     // }
//     // else
//     {
//         int portnext = 10000;
//         while (isPortFree(portnext) != 1)
//             portnext++;
//         dataStruct toSSForSomeReason;
//         toSSForSomeReason.datF = portbhejrhahoon;
//         toSSForSomeReason.destF = NtoS;
//         toSSForSomeReason.intData[0] = portnext++;
//         int sockfd = socket(AF_INET, SOCK_STREAM, 0);
//         if (sockfd == -1)
//         {
//             perror("socket");
//             exit(EXIT_FAILURE);
//         }
//         struct sockaddr_in server_addr;
//         memset(&server_addr, 0, sizeof(server_addr));
//         server_addr.sin_family = AF_INET;
//         server_addr.sin_port = htons(port_to_send_to);
//         inet_pton(AF_INET, IP, &(server_addr.sin_addr));

//         // Connect to the server
//         if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
//         {
//             perror("connect");
//             close(sockfd);
//             exit(EXIT_FAILURE);
//         }
//         send(sockfd, &toSSForSomeReason, sizeof(toSSForSomeReason), 0);
//         close(sockfd);
//         sockfd = socket(AF_INET, SOCK_STREAM, 0);
//         if (sockfd == -1)
//         {
//             perror("socket");
//             exit(EXIT_FAILURE);
//         }
//         server_addr.sin_family = AF_INET;
//         server_addr.sin_port = htons(portnext - 1);
//         inet_pton(AF_INET, IP, &(server_addr.sin_addr));
//         usleep(1000);
//         if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
//         {
//             perror("connect");
//             close(sockfd);
//             exit(EXIT_FAILURE);
//         }
//         toSSForSomeReason.datF = deltReq;
//         toSSForSomeReason.destF = NtoS;
//         strcpy(toSSForSomeReason.stringData[0], pathToDelete);
//         send(sockfd, &toSSForSomeReason, sizeof(toSSForSomeReason), 0);
//         recv(sockfd, &toSSForSomeReason, sizeof(toSSForSomeReason), 0);
//         close(sockfd);
//     }
// }
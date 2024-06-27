#include "defs.h"

int main()
{
    char mode[10];
    char flag[10];
    char path[MAXSIZES];
    char message[MAXSIZES];

    char where[MAXSIZES];
    char what[MAXSIZES];

    struct dataStruct *fetch_data;
    fetch_data = malloc(sizeof(struct dataStruct));

    char ip[100];
    strcpy(ip, IP);
    int port = NS_MASTER_PORT;

    int sock;
    struct sockaddr_in addr;
    socklen_t addr_size;
    char buffer[MAXSIZES];

    while (1)
    {
        int priorityProcess = 0;
        printf(GREEN"Enter operation : "RESET);
        scanf("%s", mode);
        for (int i = 0; i < strlen(mode); i++)
        {
            mode[i] = toupper(mode[i]);
        }

        if (strcmp("READ", mode) == 0)
        {
            printf(YELLOW"Enter path : "RESET);
            scanf("%s", path);

            fetch_data->destF = CtoN;
            fetch_data->datF = readReq;
            strcpy(fetch_data->stringData[0], path);
        }
        else if (strcmp("WRITE", mode) == 0)
        {
            printf(YELLOW"Enter path : "RESET);
            scanf("%s", path);
            printf(YELLOW"Enter flag : "RESET);
            scanf("%s", flag);
            printf(YELLOW"Enter content : "RESET);
            fgets(message, MAXSIZES, stdin);
            fgets(message, MAXSIZES, stdin);

            printf("%s", message);
            fetch_data->destF = CtoN;
            fetch_data->datF = writeReq;
            strcpy(fetch_data->stringData[0], path);
        }

        else if (strcmp("GETSP", mode) == 0)
        {
            printf(YELLOW"Enter path : "RESET);
            scanf("%s", path);
            fetch_data->destF = CtoN;
            fetch_data->datF = getSP;
            strcpy(fetch_data->stringData[0], path);
        }

        else if (strcmp("CREATE", mode) == 0)
        {
            printf(YELLOW"Create Where : "RESET);
            scanf("%s", where);
            printf(YELLOW"Create What : "RESET);
            scanf("%s", what);
            int isFile;
            printf(BLUE"0 for Dir, 1 for File: "RESET);
            scanf("%d", &isFile);
            fetch_data->datF = createReq;
            fetch_data->destF = CtoN;
            fetch_data->intData[1] = isFile;
            strcpy(fetch_data->stringData[0], where);
            strcpy(fetch_data->stringData[1], what);
            priorityProcess = 1;
        }
        else if (strcmp("COPY", mode) == 0)
        {
            printf(YELLOW"Copy What : "RESET);
            scanf("%s", what);
            printf(YELLOW"Copy To : "RESET);
            scanf("%s", where);
            int isFile;
            printf(BLUE"0 for Dir, 1 for File: "RESET);
            scanf("%d", &isFile);
            fetch_data->datF = cpyReq;
            fetch_data->destF = CtoN;
            fetch_data->intData[0] = isFile;
            strcpy(fetch_data->stringData[0], what);
            strcpy(fetch_data->stringData[1], where);
            priorityProcess = 1;
        }
        else if (strcmp("DELETE", mode) == 0)
        {
            printf(YELLOW"Delete what : "RESET);
            scanf("%s", what);
            fetch_data->datF = deltReq;
            fetch_data->destF = CtoN;
            strcpy(fetch_data->stringData[0], what);
            priorityProcess = 1;
        }
        else if (strcmp("LS", mode) == 0)
        {
            int choice;
            printf(YELLOW"Do you want to search a particular path? (0 for NO, 1 for YES):"RESET);
            scanf("%d", &choice);
            if (choice == 1)
            {
                char subs[MAXSIZES];
                printf(YELLOW"Enter the directory: "RESET);
                scanf("%s", subs);
                // int l=strlen(subs);
                // if(subs[l-1]!='/')
                //     strcat(subs,"/");
                strcpy(fetch_data->stringData[0], subs);
            }
            fetch_data->datF = LSreq;
            fetch_data->destF = CtoN;
            fetch_data->intData[0] = choice;
            priorityProcess = 1;
        }
        else if (strcmp("LOG", mode) == 0)
        {
            fetch_data->datF = logreq;
            fetch_data->destF = CtoN;
            priorityProcess = 1;
        }
        //------------------- input ends ---------------------//
        if (!priorityProcess)
        {
            sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0)
            {
                perror("Socket error");
                return 1;
            }
            printf("TCP client socket created.\n");

            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            addr.sin_addr.s_addr = inet_addr(ip);

            if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
            {
                perror("Connection error");
                close(sock);
                return 1;
            }
            printf("Connected to the server.\n");

            int n = send(sock, fetch_data, sizeof(*fetch_data), 0);
            if (n < 0)
            {
                perror("Send error");
                close(sock);
                return 1;
            }

            n = recv(sock, fetch_data, sizeof(*fetch_data), 0);

            if (fetch_data->datF == conditionCode)
            {
                printf(RED"%d : Error Detected here\n"RESET, fetch_data->intData[0]);
                close(sock);
                continue;
            }

            close(sock);

            int new_port = fetch_data->intData[0];
            printf("%d\n", new_port);
            if (fetch_data->intData[0] == SERVERDISCONN)
                printf(CYAN"The Server has Disconnected, Read maybe Available\n"RESET);
            usleep(1000);
            int new_sock;
            struct sockaddr_in addr1;
            socklen_t addr1_size;
            new_sock = socket(AF_INET, SOCK_STREAM, 0);
            if (new_sock < 0)
            {
                perror("Socket error");
                return 1;
            }
            memset(&addr1, 0, sizeof(addr1));
            addr1.sin_family = AF_INET;
            addr1.sin_port = htons(new_port);
            addr1.sin_addr.s_addr = inet_addr(ip);

            if (connect(new_sock, (struct sockaddr *)&addr1, sizeof(addr1)) < 0)
            {
                perror("Connection error");
                close(new_sock);
                return 1;
            }

            fetch_data->destF = CtoS;
            strcpy(fetch_data->stringData[0], path);

            if (fetch_data->datF == writeReq)
            {
                strcpy(fetch_data->stringData[1], flag);
                strcpy(fetch_data->stringData[2], message);
            }
            n = send(new_sock, fetch_data, sizeof(*fetch_data), 0);
            if (n < 0)
            {
                perror("Send error");
                close(new_sock);
                return 1;
            }
            if (fetch_data->datF == readReq)
            {
                while (1)
                {
                    n = recv(new_sock, fetch_data, sizeof(*fetch_data), 0);
                    if (n < 0)
                    {
                        perror(RED"Send error"RESET);
                        close(new_sock);
                        return 1;
                    }
                    if (fetch_data->datF == conditionCode && fetch_data->destF == STOP)
                        break;
                    else
                    {
                        printf("%s", fetch_data->stringData[0]);
                    }
                }
            }
            else
            {
                n = recv(new_sock, fetch_data, sizeof(*fetch_data), 0);
                if (n < 0)
                {
                    perror("Receiving error");
                    close(new_sock);
                    return 1;
                }

                if (fetch_data->intData[0] != 300 && fetch_data->destF == STOP && fetch_data->datF == conditionCode)
                {

                    printf(RED"%d : Error executing operation\n"RESET, fetch_data->intData[0]);
                }
                else if (fetch_data->destF == STOP)
                {
                    if (fetch_data->datF == readReq)
                    {
                        printf(GREEN"SUCCESS Reading\n"RESET);
                        printf("%s\n", fetch_data->stringData[0]);
                    }
                    else if (fetch_data->datF == getSP)
                    {
                        printf(GREEN"SUCCESS Fetching\n"RESET);
                        printf("%s\n", fetch_data->stringData[0]);
                    }
                    else if (fetch_data->datF == writeReq)
                    {
                        printf(GREEN"SUCCESS Writing\n"RESET);
                    }
                }
                else
                {
                    perror(RED"Shouldn't Reach Case"RESET);
                }
                close(new_sock);
            }
        }
        else
        {
            sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0)
            {
                perror(RED"Socket error"RESET);
                return 1;
            }
            printf("TCP client socket created.\n");

            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            addr.sin_addr.s_addr = inet_addr(ip);

            if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
            {
                perror(RED"Connection error"RESET);
                close(sock);
                return 1;
            }
            printf("Connected to the server.\n");

            int n = send(sock, fetch_data, sizeof(*fetch_data), 0);
            if (n < 0)
            {
                perror(RED"Send error"RESET);
                close(sock);
                return 1;
            }
            n = recv(sock, fetch_data, sizeof(*fetch_data), 0);

            if (fetch_data->datF == conditionCode)
            {
                if (fetch_data->intData[0] == SERVERDISCONN)
                {
                    printf(ORANGE"The Server has Disconnected, Read maybe Available\n"RESET);
                }
                printf(RED"%d : Error Detected\n"RESET, fetch_data->intData[0]);
                close(sock);
                continue;
            }
            else
            {
                int port = fetch_data->intData[0];
                int server_socket, new_socket;
                struct sockaddr_in server_addr, client_addr;
                socklen_t addr_len = sizeof(client_addr);

                if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
                {
                    perror(RED"Socket creation failed"RESET);
                    exit(EXIT_FAILURE);
                }

                server_addr.sin_family = AF_INET;
                server_addr.sin_addr.s_addr = inet_addr(IP);
                server_addr.sin_port = htons(port);

                // Bind the socket
                if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
                {
                    perror(RED"Bind failed"RESET);
                    exit(EXIT_FAILURE);
                }

                if (listen(server_socket, MAX_CONNECTIONS) < 0)
                {
                    perror(RED"Listen failed"RESET);
                    exit(EXIT_FAILURE);
                }

                printf("Server listening on port %d\n", port);

                if ((new_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len)) < 0)
                {
                    perror(RED"Accept failed"RESET);
                    exit(EXIT_FAILURE);
                }
                if (strcmp(mode, "LS") != 0 && strcmp(mode,"LOG")!=0)
                {
                    n = recv(new_socket, fetch_data, sizeof(*fetch_data), 0);
                }
                else
                {
                    while (1)
                    {
                        n = recv(new_socket, fetch_data, sizeof(*fetch_data), 0);
                        if (fetch_data->destF == STOP)
                        {
                            break;
                        }
                        else if (fetch_data->datF == LSreq && fetch_data->destF == NtoC)
                        {
                            if (strncmp(fetch_data->stringData[0], "COPY", 4) != 0)
                                printf("%s\n", fetch_data->stringData[0]);
                        }
                        else if (fetch_data->datF == logreq && fetch_data->destF == NtoC)
                        {
                            if (fetch_data->intData[0] == 0)
                            {
                                if (fetch_data->intData[1] == readReq)
                                {
                                    printf(ORANGE"Operation : READ\n"RESET);
                                }
                                if (fetch_data->intData[1] == writeReq)
                                {
                                    printf(ORANGE"Operation : WRITE\n"RESET);
                                }
                                if (fetch_data->intData[1] == getSP)
                                {
                                    printf(ORANGE"Operation : GETSP\n"RESET);
                                }
                                printf(CYAN"Communication between server and client happened over: %d\n"RESET, fetch_data->intData[2]);
                                printf(CYAN"Server involved : %d\n"RESET, fetch_data->intData[5]);
                            }
                            else
                            {
                                if (fetch_data->intData[1] == deltReq)
                                {
                                    printf(ORANGE"Operation : DELETE\n"RESET);
                                    printf(CYAN"Communication between server and NM happened over: %d\n"RESET, fetch_data->intData[3]);
                                    printf(CYAN"Communication between client and NM happened over: %d\n"RESET, fetch_data->intData[2]);
                                    printf(CYAN"Server involved : %d\n"RESET, fetch_data->intData[5]);
                                }
                                if (fetch_data->intData[1] == createReq)
                                {
                                    printf(ORANGE"Operation : CREATE\n"RESET);
                                    printf(CYAN"Communication between server and NM happened over: %d\n"RESET, fetch_data->intData[3]);
                                    printf(CYAN"Communication between client and NM happened over: %d\n"RESET, fetch_data->intData[2]);
                                    printf(CYAN"Server involved : %d\n"RESET, fetch_data->intData[5]);
                                }
                                if (fetch_data->intData[1] == cpyReq)
                                {
                                    printf(ORANGE"Operation : COPY\n"RESET);
                                    printf(CYAN"Communication between source server and NM happened over: %d\n"RESET,fetch_data->intData[3]);
                                    printf(CYAN"Communication between destination server and NM happened over: %d\n"RESET,fetch_data->intData[4]);
                                    printf(CYAN"Communication between client and NM happened over: %d\n"RESET,fetch_data->intData[2]);
                                    printf(CYAN"Source server: %d, Destination server : %d\n"RESET,fetch_data->intData[5],fetch_data->intData[6]);
                                }
                            }
                        }
                    }
                }
                if (fetch_data->datF == conditionCode && fetch_data->intData[0] == SUCCESS)
                {
                    printf(GREEN"Operation executed successfully\n"RESET);
                }
                else if (fetch_data->datF == conditionCode && fetch_data->intData[0] == FAILURE)
                {
                    printf(GREEN"Operation executed unsuccessfully\n"RESET);
                }

                close(new_socket);
            }
            close(sock);
        }
    }
    printf(RED"Disconnected from the server.\n"RESET);

    return 0;
}
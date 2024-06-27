#include "defs.h"

void *Redudancy(void *args);

pthread_t redundantT;
HT h;
Queue q;
int book_count = 0;
Log Book;
typedef struct StorageServer
{
    int id; // unique for each server
    char ip[200];
    int portForNS;
    int portForSSBind;
    char path[MAXSIZES];
    int copyid1;
    int copyid2;
    struct StorageServer *next;
} StorageServer;

int isPortFree(int port);

int port_assign = 10001;

int count_server = 0;
int cur_alive_server = 0;

sem_t mutex;

StorageServer *S;
StorageServer *listOfSS;
StorageServer *listOfDeadSS;
StorageServer *Sdead;
StorageServer *addNode(int portForNS, int portForSSBind, StorageServer *S, char *path, int idx)
{
    while (S->next != NULL)
    {
        S = S->next;
    }
    S->next = (StorageServer *)malloc(sizeof(StorageServer));
    S->next->id = idx;
    S->next->next = NULL;
    strcpy(S->next->ip, IP);
    S->next->portForSSBind = portForSSBind;
    S->next->portForNS = portForNS;
    strcpy(S->next->path, path);
    S->next->copyid1 = -1;
    S->next->copyid2 = -1;
    return S->next;
}
void *LOGREQUEST(void *arg)
{
    int port = *((int *)arg);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, IP, &(server_addr.sin_addr));
    usleep(1000);
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    int count;
    if (book_count < 20)
        count = book_count;
    else
        count = 20;
    int i = 0;
    printf("Count of Log:%d\n", count);
    while (i != count)
    {
        dataStruct toCli;
        toCli.datF = logreq;
        toCli.destF = NtoC;
        toCli.intData[0] = Book[i].flag;
        toCli.intData[1] = Book[i].datF;
        toCli.intData[2] = Book[i].port_used_by_client;
        toCli.intData[3] = Book[i].port_used_by_serverSrc;
        toCli.intData[4] = Book[i].port_used_by_serverDest;
        toCli.intData[5] = Book[i].SS_idxsrc;
        toCli.intData[6] = Book[i].SS_idxdest;
        send(sockfd, &toCli, sizeof(toCli), 0);
        i++;
    }
    printf("Sending Stop Signal\n");
    dataStruct toCli;
    toCli.datF = conditionCode;
    toCli.destF = STOP;
    toCli.intData[0] = SUCCESS;
    send(sockfd, &toCli, sizeof(toCli), 0);
    close(sockfd);
}
int disConnCheck1(int idx)
{
    StorageServer *T = listOfDeadSS->next;
    while (T != NULL)
    {
        if (T->id == idx)
        {
            // printf("=100=100=100=100++++%d\n", T->id);
            break;
        }
        T = T->next;
    }
    if (T == NULL)
    {
        return -2; // server is present
    }
    else
        return T->copyid1; // server has disconnected
}
int disConnCheck2(int idx)
{
    StorageServer *T = listOfDeadSS->next;
    while (T != NULL)
    {
        if (T->id == idx)
        {
            break;
        }
        T = T->next;
    }
    if (T == NULL)
    {
        return -1; // server is present
    }
    else
        return T->copyid2; // server has disconnected
}
int alphasort(const struct dirent **a, const struct dirent **b)
{
    return strcoll((*a)->d_name, (*b)->d_name);
}
// returns -1 if not valid,0 if file,1 if dir
int isFileOrDir(char *path)
{
    FILE *file = fopen(path, "r");
    if (file == NULL)
    {
        struct dirent **namelist;
        int num_entries;

        num_entries = scandir(path, &namelist, NULL, alphasort);
        if (num_entries < 0)
        {
            return -1;
        }
        return 1;
    }
    return 0;
}

void *LSrequest(void *arg)
{
    MainArgs *args = (MainArgs *)arg;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(args->portForCli);
    inet_pton(AF_INET, IP, &(server_addr.sin_addr));
    usleep(1000);
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    dataStruct Info;
    Info.datF = LSreq;
    Info.destF = NtoC;
    for (int i = 0; i < capacity; i++)
    {
        Node T = h->arr[i]->next;
        while (T != NULL)
        {
            if (args->Main->intData[0] == 1)
            {
                // printf("--%s--%s--\n",args->Main->stringData[0],T->s);
                if (strncmp(T->s, args->Main->stringData[0], strlen(args->Main->stringData[0])) == 0)
                {
                    strcpy(Info.stringData[0], T->s);
                    send(sockfd, &Info, sizeof(Info), 0);
                }
            }
            else
            {
                strcpy(Info.stringData[0], T->s);
                send(sockfd, &Info, sizeof(Info), 0);
            }
            T = T->next;
        }
    }
    Info.datF = conditionCode;
    Info.destF = STOP;
    Info.intData[0] = SUCCESS;
    send(sockfd, &Info, sizeof(Info), 0);
    close(sockfd);
}

int copyPathtoPath(char *srcPath, char *destPath, int isFile)
{
    int portnext = 10000;
    while (isPortFree(portnext) != 1)
        portnext++;
    int PortForSrc = portnext++;
    StorageServer *Temp = listOfSS;
    char temp_path[MAXSIZES];
    int port_to_send_to;
    int SSsrc = searchPath(srcPath, h);
    if (SSsrc != -1)
    {
        while (Temp->next != NULL)
        {
            if (Temp->next->id == SSsrc)
            {
                strcpy(temp_path, Temp->next->path);
                port_to_send_to = Temp->next->portForSSBind;
                break;
            }
            Temp = Temp->next;
        }
    }
    dataStruct toSS;
    toSS.datF = cpyReq;
    toSS.destF = NtoS;
    toSS.intData[0] = isFile;
    toSS.intData[1] = TUSRCHAI;
    toSS.intData[2] = PortForSrc;
    strcpy(toSS.stringData[0], temp_path);
    strcat(toSS.stringData[0], "/");
    strcat(toSS.stringData[0], srcPath);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_to_send_to);
    inet_pton(AF_INET, IP, &(server_addr.sin_addr));
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    send(sockfd, &toSS, sizeof(toSS), 0);
    close(sockfd);
    while (isPortFree(portnext) != 1)
        portnext++;
    int PortForDest = portnext++;
    Temp = listOfSS;
    port_to_send_to;
    int SSdest = searchPath(srcPath, h);
    if (SSdest != -1)
    {
        while (Temp->next != NULL)
        {
            if (Temp->next->id == SSdest)
            {
                strcpy(temp_path, Temp->next->path);
                port_to_send_to = Temp->next->portForSSBind;
                break;
            }
            Temp = Temp->next;
        }
    }
    toSS.datF = cpyReq;
    toSS.destF = NtoS;
    toSS.intData[0] = isFile;
    toSS.intData[1] = TUDESTHAI;
    toSS.intData[2] = PortForDest;
    strcpy(toSS.stringData[0], temp_path);
    strcat(toSS.stringData[0], "/");
    strcat(toSS.stringData[0], destPath);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    // struct sockaddr_in server_addr;
    // memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_to_send_to);
    inet_pton(AF_INET, IP, &(server_addr.sin_addr));
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    send(sockfd, &toSS, sizeof(toSS), 0);
    close(sockfd);
    // ------------------PORTSENDING FINISHED -----------------------------------------------
    usleep(1000);
    int sockfdsrc = socket(AF_INET, SOCK_STREAM, 0);
    int sockfddest = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfdsrc == -1)
    {
        perror("socket");
        // error = 1;
        exit(EXIT_FAILURE);
    }
    if (sockfddest == -1)
    {
        perror("socket");
        // error = 1;
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in server_addr1;
    memset(&server_addr1, 0, sizeof(server_addr1));
    server_addr1.sin_family = AF_INET;
    server_addr1.sin_port = htons(PortForSrc);
    inet_pton(AF_INET, IP, &(server_addr1.sin_addr));

    // Connect to the server
    if (connect(sockfdsrc, (struct sockaddr *)&server_addr1, sizeof(server_addr1)) == -1)
    {
        perror("connect");
        close(sockfdsrc);
        // error = 1;
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in server_addr2;
    memset(&server_addr2, 0, sizeof(server_addr2));
    server_addr2.sin_family = AF_INET;
    server_addr2.sin_port = htons(PortForDest);
    inet_pton(AF_INET, IP, &(server_addr2.sin_addr));

    // Connect to the server
    if (connect(sockfddest, (struct sockaddr *)&server_addr2, sizeof(server_addr2)) == -1)
    {
        perror("connect");
        close(sockfddest);
        // error = 1;
        exit(EXIT_FAILURE);
    }
    while (1)
    {
        dataStruct recStr;
        int br = recv(sockfdsrc, &recStr, sizeof(recStr), 0);
        if (br < 0)
        {
            perror("Receive ERROR!!!\n");
            // error = 1;
            break;
        }
        if (recStr.datF == conditionCode && recStr.destF == STOP)
            break;
        else
        {
            dataStruct T;
            T.datF = cpyReq;
            T.destF = NtoS;
            T.intData[0] = recStr.intData[0];
            T.intData[1] = recStr.intData[1];
            T.intData[2] = recStr.intData[2];

            strcpy(T.stringData[0], temp_path);
            strcat(T.stringData[0], "/");
            strcat(T.stringData[0], destPath);
            strcat(T.stringData[0], "/");
            strcat(T.stringData[0], recStr.stringData[0]);
            char toInsert[MAXSIZES * 3];
            sprintf(toInsert, "%s/%s", destPath, recStr.stringData[0]);
            insert(searchPath(destPath, h), toInsert, h);
            // printf("Inserted into HT: %s\n",toInsert);
            strcpy(T.stringData[1], recStr.stringData[1]);
            // printf("----%d %d %d- %s---\n",T.datF,T.destF,T.intData[0],T.stringData[0]);
            send(sockfddest, &T, sizeof(T), 0);
            usleep(1000);
        }
    }
    // printf("iii\n");
    dataStruct toSSFin;
    toSSFin.datF = conditionCode;
    toSSFin.destF = STOP;
    send(sockfddest, &toSSFin, sizeof(toSSFin), 0);
    close(sockfdsrc);
    close(sockfddest);
}

void *cpyopn(void *arg)
{
    MainArgs *args = (MainArgs *)arg;
    sem_wait(&mutex);
    Book[book_count % 20].flag = 1;
    Book[book_count % 20].datF = args->Main->datF;
    Book[book_count % 20].port_used_by_client = args->portForCli;
    Book[book_count % 20].port_used_by_serverSrc = args->srcPortnew;
    Book[book_count % 20].port_used_by_serverDest = args->destPortnew;
    book_count++;
    sem_post(&mutex);
    int error = 0;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("socket");
        error = 1;
        exit(EXIT_FAILURE);
    }
    dataStruct toSS;
    toSS.datF = cpyReq;
    toSS.destF = NtoS;
    toSS.intData[0] = args->Main->intData[0];
    toSS.intData[1] = TUSRCHAI;
    toSS.intData[2] = args->srcPortnew;
    strcpy(toSS.stringData[0], args->homepathSrc);
    strcat(toSS.stringData[0], "/");
    strcat(toSS.stringData[0], args->Main->stringData[0]);

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(args->srcPortold);
    inet_pton(AF_INET, IP, &(server_addr.sin_addr));

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect");
        close(sockfd);
        error = 1;
        exit(EXIT_FAILURE);
    }
    send(sockfd, &toSS, sizeof(toSS), 0);
    close(sockfd);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        error = 1;
        perror("socket");
        exit(EXIT_FAILURE);
    }
    toSS.intData[1] = TUDESTHAI;
    toSS.intData[2] = args->destPortnew;
    strcpy(toSS.stringData[0], args->homepathDest);
    strcat(toSS.stringData[0], "/");
    strcat(toSS.stringData[0], args->Main->stringData[1]);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(args->destPortold);
    inet_pton(AF_INET, IP, &(server_addr.sin_addr));

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect");
        close(sockfd);
        error = 1;
        exit(EXIT_FAILURE);
    }
    send(sockfd, &toSS, sizeof(toSS), 0);
    close(sockfd);

    usleep(1000);
    printf("Port Sent\n");

    // ----------------------Port sending finished--------------------------------

    int sockfdsrc = socket(AF_INET, SOCK_STREAM, 0);
    int sockfddest = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfdsrc == -1)
    {
        perror("socket");
        error = 1;
        exit(EXIT_FAILURE);
    }
    if (sockfddest == -1)
    {
        perror("socket");
        error = 1;
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in server_addr1;
    memset(&server_addr1, 0, sizeof(server_addr1));
    server_addr1.sin_family = AF_INET;
    server_addr1.sin_port = htons(args->srcPortnew);
    inet_pton(AF_INET, IP, &(server_addr1.sin_addr));

    // Connect to the server
    if (connect(sockfdsrc, (struct sockaddr *)&server_addr1, sizeof(server_addr1)) == -1)
    {
        perror("connect");
        close(sockfdsrc);
        error = 1;
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in server_addr2;
    memset(&server_addr2, 0, sizeof(server_addr2));
    server_addr2.sin_family = AF_INET;
    server_addr2.sin_port = htons(args->destPortnew);
    inet_pton(AF_INET, IP, &(server_addr2.sin_addr));

    // Connect to the server
    if (connect(sockfddest, (struct sockaddr *)&server_addr2, sizeof(server_addr2)) == -1)
    {
        perror("connect");
        close(sockfddest);
        error = 1;
        exit(EXIT_FAILURE);
    }
    while (1)
    {
        dataStruct recStr;
        int br = recv(sockfdsrc, &recStr, sizeof(recStr), 0);
        if (br < 0)
        {
            perror("Receive ERROR!!!\n");
            error = 1;
            break;
        }
        if (recStr.datF == conditionCode && recStr.destF == STOP)
            break;
        else
        {
            dataStruct T;
            T.datF = cpyReq;
            T.destF = NtoS;
            T.intData[0] = recStr.intData[0];
            T.intData[1] = recStr.intData[1];
            T.intData[2] = recStr.intData[2];

            strcpy(T.stringData[0], args->homepathDest);
            strcat(T.stringData[0], "/");
            strcat(T.stringData[0], args->Main->stringData[1]);
            strcat(T.stringData[0], "/");
            strcat(T.stringData[0], recStr.stringData[0]);
            char toInsert[MAXSIZES * 3];
            sprintf(toInsert, "%s/%s", args->Main->stringData[1], recStr.stringData[0]);
            insert(args->destIdx, toInsert, h);
            // printf("Inserted into HT: %s\n",toInsert);
            strcpy(T.stringData[1], recStr.stringData[1]);
            // printf("----%d %d %d- %s---\n",T.datF,T.destF,T.intData[0],T.stringData[0]);
            send(sockfddest, &T, sizeof(T), 0);
            usleep(1000);
        }
    }
    // printf("iii\n");
    dataStruct toSSFin;
    toSSFin.datF = conditionCode;
    toSSFin.destF = STOP;
    send(sockfddest, &toSSFin, sizeof(toSSFin), 0);
    close(sockfdsrc);
    close(sockfddest);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(args->portForCli);
    inet_pton(AF_INET, IP, &(server_addr.sin_addr));
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect");
        close(sockfd);
        error = 1;
        exit(EXIT_FAILURE);
    }
    dataStruct toCli;
    if (error == 0)
    {
        toCli.datF = conditionCode;
        toCli.destF = NtoC;
        toCli.intData[0] = SUCCESS;
        send(sockfd, &toCli, sizeof(toCli), 0);
    }
    else
    {
        toCli.datF = conditionCode;
        toCli.destF = NtoC;
        toCli.intData[0] = FAILURE;
        send(sockfd, &toCli, sizeof(toCli), 0);
    }
    return NULL;
    close(sockfd);
}

void *prioropn(void *arg)
{
    MainArgs *args = (MainArgs *)arg;
    sem_wait(&mutex);
    Book[book_count % 20].flag = 1;
    Book[book_count % 20].datF = args->Main->datF;
    Book[book_count % 20].port_used_by_client = args->portForCli;
    Book[book_count % 20].port_used_by_serverSrc = args->newPortForSS;
    book_count++;
    sem_post(&mutex);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    dataStruct toSS;
    toSS.datF = portbhejrhahoon;
    toSS.destF = NtoS;
    toSS.intData[0] = args->newPortForSS;
    toSS.intData[1] = args->Main->intData[1];
    strcpy(toSS.stringData[0], args->Main->stringData[0]);
    strcpy(toSS.stringData[1], args->Main->stringData[1]);

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(args->oldSSPort);
    inet_pton(AF_INET, IP, &(server_addr.sin_addr));

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    send(sockfd, &toSS, sizeof(toSS), 0);
    close(sockfd);
    usleep(1000);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // struct sockaddr_in server_addr;
    // memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(args->newPortForSS);
    inet_pton(AF_INET, IP, &(server_addr.sin_addr));

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    toSS.datF = args->Main->datF;
    send(sockfd, &toSS, sizeof(toSS), 0);
    recv(sockfd, &toSS, sizeof(toSS), 0);
    int flag = 0;
    if (toSS.datF == deltReq)
    {
        if (toSS.intData[0] == SUCCESS)
        {
            flag = 1;
            int check = deletePath(toSS.stringData[0], h);
            if (check == -1)
            {
                perror("Error removing from HASHTABLE");
                flag = 0;
            }
        }
        else
        {
            perror("Error performing operation!\n");
        }
    }
    else if (toSS.datF == createReq)
    {
        if (toSS.intData[0] == SUCCESS)
        {
            flag = 1;
            char jaapath[MAXSIZES * 2];
            strcpy(jaapath, toSS.stringData[0]);
            strcat(jaapath, "/");
            strcat(jaapath, toSS.stringData[1]);
            insert(toSS.intData[1], jaapath, h);
        }
        else
        {
            perror("Error performing operation!\n");
        }
    }
    close(sockfd);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // struct sockaddr_in server_addr;
    // memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(args->portForCli);
    inet_pton(AF_INET, IP, &(server_addr.sin_addr));

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    dataStruct toCli;
    toCli.datF = conditionCode;
    toCli.destF = NtoC;
    if (flag == 1)
        toCli.intData[0] = SUCCESS;
    else
        toCli.intData[0] = FAILURE;
    send(sockfd, &toCli, sizeof(toCli), 0);
    close(sockfd);
}
void *ListenOnAPort(void *arg)
{
    int port = *(int *)arg;
    int server_socket, new_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(IP);
    server_addr.sin_port = htons(port);

    // Bind the socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAX_CONNECTIONS) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    sem_wait(&mutex);
    printf("NM listening on port %d\n", port);
    sem_post(&mutex);

    while (1)
    {
        if ((new_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len)) < 0)
        {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }
        dataStruct SS_Temp;
        ssize_t bytes_received = recv(new_socket, &SS_Temp, sizeof(SS_Temp), 0);

        if (bytes_received < 0)
        {
            perror("Receive failed");
            exit(EXIT_FAILURE);
        }
        // initialising SS
        if (SS_Temp.datF == conditionCode && SS_Temp.destF == StoN && SS_Temp.intData[0] == SS_INIT)
        {
            sem_wait(&mutex);
            while (isPortFree(port_assign) != 1)
            {
                port_assign++;
            }
            sem_post(&mutex);
            dataStruct toSend;
            toSend.datF = conditionCode;
            toSend.destF = NtoS;
            sem_wait(&mutex);
            toSend.intData[0] = port_assign;
            toSend.intData[2] = count_server;
            int new_id = count_server;
            char copyPath[100];
            sprintf(copyPath, "COPY%d", new_id);
            insert(new_id, copyPath, h);
            int new_portForSSBind = port_assign++;
            while (isPortFree(port_assign) != 1)
            {
                port_assign++;
            }
            int new_portForNS = port_assign++;
            S = addNode(new_portForNS, new_portForSSBind, S, SS_Temp.stringData[0], count_server);
            count_server++;
            cur_alive_server++;
            toSend.intData[1] = port_assign - 1;
            int *a = (int *)malloc(sizeof(int));
            *a = port_assign - 1;
            sem_post(&mutex);
            send(new_socket, &toSend, sizeof(toSend), 0);
            pthread_t thread;
            pthread_create(&thread, NULL, ListenOnAPort, a);
        }
        // accepting paths from SS
        else if (SS_Temp.datF == conditionCode && SS_Temp.destF == StoN && SS_Temp.intData[0] == SS_DEFPATH)
        {
                        // pthread_create(&redundantT,NULL,Redudancy,NULL);

            sem_wait(&mutex);
            insert(SS_Temp.intData[1], SS_Temp.stringData[0], h);
            printf("Path received: %s\n", SS_Temp.stringData[0]);
            printf("Index: %d\n", SS_Temp.intData[1]);
            int a = searchPath(SS_Temp.stringData[0], h);
            sem_post(&mutex);
        }
        // communicating with SS for client operations
        else if ((SS_Temp.datF == readReq || SS_Temp.datF == writeReq || SS_Temp.datF == getSP) && SS_Temp.destF == CtoN)
        {
            // assumed that path will be sent
            int value = hashingFunc(SS_Temp.stringData[0]);
            int req_SS = searchPath(SS_Temp.stringData[0], h);
            StorageServer *Temp = listOfSS;
            char temp_path[MAXSIZES] = "";
            int valid = 1;
            {
                int port_next = 10000;

                int value = hashingFunc(SS_Temp.stringData[0]);

                char *path = malloc(sizeof(char) * MAXSIZES);
                strcpy(path, SS_Temp.stringData[0]);
                int in_LRU = LRU(path, q);
                int cache_flag = 0;
                int req_SS;
                if (in_LRU >= 0)
                {
                    req_SS = in_LRU;
                    cache_flag = 1;
                }
                else
                {
                    req_SS = searchPath(SS_Temp.stringData[0], h);
                    QNode node_temp = malloc(sizeof(struct que));
                    node_temp->s = malloc(sizeof(struct element));
                    strcpy(node_temp->s->cached_path, SS_Temp.stringData[0]);
                    node_temp->s->quidx = req_SS;
                    enqueue(q, node_temp);
                }
                if (req_SS == -1)
                {
                    dataStruct toCli;
                    toCli.datF = conditionCode;
                    toCli.destF = NtoC;
                    toCli.intData[0] = PATHNOTFOUND;
                    if (send(new_socket, &toCli, sizeof(toCli), 0) == -1)
                    {
                        perror("send");
                    }
                }
                else
                {
                    // check if the server has disconnected or not
                    int checkForDiscon1 = disConnCheck1(req_SS);
                    if (checkForDiscon1 == -2)
                    {
                        StorageServer *Temp = listOfSS;
                        int port_to_bind;
                        while (Temp->next != NULL)
                        {
                            if (Temp->next->id == req_SS)
                            {
                                port_to_bind = Temp->next->portForSSBind;
                                break;
                            }
                            Temp = Temp->next;
                        }
                        if (cache_flag == 1)
                            printf("Cache hit!\n");
                        else
                            printf("Cache miss!\n");
                        printf("Contacting SS %d on %d to create a new port\n", req_SS, port_to_bind);
                        int sockfd = socket(AF_INET, SOCK_STREAM, 0);

                        if (sockfd == -1)
                        {
                            perror("socket");
                            exit(EXIT_FAILURE);
                        }

                        struct sockaddr_in server_addr;
                        memset(&server_addr, 0, sizeof(server_addr));
                        server_addr.sin_family = AF_INET;
                        server_addr.sin_port = htons(port_to_bind);
                        inet_pton(AF_INET, IP, &(server_addr.sin_addr));

                        // Connect to the server
                        if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
                        {
                            perror("connect");
                            close(sockfd);
                            exit(EXIT_FAILURE);
                        }

                        dataStruct initialRequest;
                        initialRequest.datF = SS_Temp.datF;
                        initialRequest.destF = NtoS;
                        strcpy(initialRequest.stringData[0], SS_Temp.stringData[0]);
                        initialRequest.intData[1] = -1;
                        // book keeping(to be lock in lock)
                        Book[book_count % 20].flag = 0;
                        Book[book_count % 20].datF = SS_Temp.datF;
                        Book[book_count % 20].SS_idxsrc = req_SS;
                        // book keeping (to be done in lock)
                        sem_wait(&mutex);
                        while (isPortFree(port_next) != 1)
                        {
                            port_next++;
                        }
                        initialRequest.intData[0] = port_next;
                        Book[book_count % 20].port_used_by_client = port_next;
                        book_count++;
                        printf("sending Data\n");

                        if (send(sockfd, &initialRequest, sizeof(initialRequest), 0) == -1)
                        {
                            perror("send");
                            close(sockfd);
                            exit(EXIT_FAILURE);
                        }
                        usleep(100);
                        sem_post(&mutex);
                        dataStruct fileAvailability;
                        recv(sockfd, &fileAvailability, sizeof(fileAvailability), 0);
                        printf("Server will validate file path!\n");
                        if (fileAvailability.datF == conditionCode && (fileAvailability.intData[0] == BADFILEPATH || fileAvailability.intData[0] == EXPPATHDIR))
                        {
                            fileAvailability.destF = NtoC;
                            send(new_socket, &fileAvailability, sizeof(fileAvailability), 0);
                            printf("Path error packet sent to client.\n");
                        }
                        else
                        {
                            initialRequest.destF = NtoC;
                            if (send(new_socket, &initialRequest, sizeof(initialRequest), 0) == -1)
                            {
                                perror("send");
                                close(new_socket);
                                exit(EXIT_FAILURE);
                            }
                            printf("Information about port sent to client.\n");
                        }
                        close(sockfd);
                    }
                    else if (checkForDiscon1 != -1)
                    {
                        // shud check copy/path? dk dc might see...
                        int checkForCon2 = disConnCheck2(req_SS);
                        if (SS_Temp.datF == readReq)
                        {
                            int secondcheck1 = disConnCheck1(checkForDiscon1);
                            int secondcheck2 = disConnCheck1(checkForCon2);
                            req_SS = -10;
                            if (secondcheck1 == -2)
                            {
                                req_SS = checkForDiscon1;
                            }
                            else if (secondcheck2 == -2)
                            {
                                req_SS = checkForCon2;
                            }
                            if (req_SS != -10)
                            {
                                StorageServer *Temp = listOfSS;
                                int port_to_bind;
                                while (Temp->next != NULL)
                                {
                                    if (Temp->next->id == req_SS)
                                    {
                                        port_to_bind = Temp->next->portForSSBind;
                                        break;
                                    }
                                    Temp = Temp->next;
                                }
                                ///
                                printf("READ will happen from the replicated paths.\n");
                                printf("Contacting SS %d on %d to create a new port\n", req_SS, port_to_bind);
                                int sockfd = socket(AF_INET, SOCK_STREAM, 0);

                                if (sockfd == -1)
                                {
                                    perror("socket");
                                    exit(EXIT_FAILURE);
                                }

                                struct sockaddr_in server_addr;
                                memset(&server_addr, 0, sizeof(server_addr));
                                server_addr.sin_family = AF_INET;
                                server_addr.sin_port = htons(port_to_bind);
                                inet_pton(AF_INET, IP, &(server_addr.sin_addr));

                                // Connect to the server
                                if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
                                {
                                    perror("connect");
                                    close(sockfd);
                                    exit(EXIT_FAILURE);
                                }

                                dataStruct initialRequest;
                                initialRequest.datF = SS_Temp.datF;
                                initialRequest.destF = NtoS;
                                initialRequest.intData[1] = COPYFLAG;

                                Book[book_count % 20].flag = 0;
                                Book[book_count % 20].datF = SS_Temp.datF;
                                Book[book_count % 20].SS_idxsrc = req_SS;
                                sem_wait(&mutex);
                                port_next = 10000;
                                while (isPortFree(port_next) != 1)
                                {
                                    port_next++;
                                }
                                initialRequest.intData[0] = port_next;
                                Book[book_count % 20].port_used_by_client = port_next;
                                book_count++;
                                printf("sending Data\n");

                                if (send(sockfd, &initialRequest, sizeof(initialRequest), 0) == -1)
                                {
                                    perror("send");
                                    close(sockfd);
                                    exit(EXIT_FAILURE);
                                }
                                usleep(100);
                                sem_post(&mutex);
                                close(sockfd);
                                ///
                                initialRequest.destF = NtoC;
                                /// sending to client
                                if (send(new_socket, &initialRequest, sizeof(initialRequest), 0) == -1)
                                {
                                    perror("send");
                                    close(new_socket);
                                    exit(EXIT_FAILURE);
                                }
                            }
                            else
                            {
                                dataStruct toCli;
                                toCli.datF = conditionCode;
                                toCli.destF = NtoC;
                                toCli.intData[0] = SERVERDISCONN;
                                send(new_socket, &toCli, sizeof(toCli), 0);
                            }
                        }
                        else
                        {
                            dataStruct toCli;
                            toCli.datF = conditionCode;
                            toCli.destF = NtoC;
                            toCli.intData[0] = SERVERDISCONN;
                            send(new_socket, &toCli, sizeof(toCli), 0);
                        }
                    }
                }
            }
        }
        else if (SS_Temp.datF == LSreq && SS_Temp.destF == CtoN)
        {
            {

                // no path provided
                sem_wait(&mutex);
                int nextport = 10000;
                while (isPortFree(nextport) != 1)
                {
                    nextport++;
                }
                dataStruct toCli;
                toCli.datF = LSreq;
                toCli.destF = NtoC;
                toCli.intData[0] = nextport++;
                send(new_socket, &toCli, sizeof(toCli), 0);
                usleep(100);
                MainArgs *MA = (MainArgs *)malloc(sizeof(MainArgs));
                MA->Main = &SS_Temp;
                MA->portForCli = toCli.intData[0];
                sem_post(&mutex);
                pthread_t new_thread;
                pthread_create(&new_thread, NULL, LSrequest, MA);
            }
        }
        else if (SS_Temp.datF == logreq && SS_Temp.destF == CtoN)
        {

            int nextport = 10000;
            sem_wait(&mutex);
            while (isPortFree(nextport) != 1)
                nextport++;
            dataStruct toCli;
            toCli.datF = logreq;
            toCli.destF = NtoC;
            toCli.intData[0] = nextport;
            send(new_socket, &toCli, sizeof(toCli), 0);
            usleep(100);
            pthread_t new_threads;
            sem_post(&mutex);
            pthread_create(&new_threads, NULL, LOGREQUEST, &nextport);
        }
        else if ((SS_Temp.datF == createReq || SS_Temp.datF == deltReq || SS_Temp.datF == cpyReq) && SS_Temp.destF == CtoN)
        {
            // pthread_create(&redundantT,NULL,Redudancy,NULL);

            int value = hashingFunc(SS_Temp.stringData[0]);
            int req_SS = searchPath(SS_Temp.stringData[0], h);
            StorageServer *Temp = listOfSS;
            char temp_path[MAXSIZES];
            int valid = 0;
            if (req_SS != -1)
            {
                while (Temp->next != NULL)
                {
                    if (Temp->next->id == req_SS)
                    {
                        strcpy(temp_path, Temp->next->path);
                        break;
                    }
                    Temp = Temp->next;
                }
            }
            strcat(temp_path, "/");
            strcat(temp_path, SS_Temp.stringData[0]);

            if (disConnCheck1(req_SS) == -2)
            {
                {
                    if (SS_Temp.datF == cpyReq)
                    {
                        char *path = malloc(sizeof(char) * MAXSIZES);
                        strcpy(path, SS_Temp.stringData[0]);
                        int in_LRU = LRU(path, q);

                        int req_SS;
                        if (in_LRU >= 0)
                        {
                            req_SS = in_LRU;
                            printf("Cache hit!\n");
                        }
                        else
                        {
                            printf("Cache miss!\n");
                            req_SS = searchPath(SS_Temp.stringData[0], h);
                            QNode node_temp = malloc(sizeof(struct que));
                            node_temp->s = malloc(sizeof(struct element));
                            strcpy(node_temp->s->cached_path, SS_Temp.stringData[0]);
                            node_temp->s->quidx = req_SS;
                            enqueue(q, node_temp);
                        }
                        StorageServer *Temp = listOfSS;
                        char temp_path[MAXSIZES];
                        int valid = 0;
                        if (req_SS != -1)
                        {
                            while (Temp->next != NULL)
                            {
                                if (Temp->next->id == req_SS)
                                {
                                    strcpy(temp_path, Temp->next->path);
                                    break;
                                }
                                Temp = Temp->next;
                            }
                        }
                        strcat(temp_path, "/");
                        strcat(temp_path, SS_Temp.stringData[1]);
                        if (disConnCheck1(req_SS) == -2)
                        {
                            if (isFileOrDir(temp_path) == -1)
                            {
                                perror("File path not valid!");
                                printf("Informing client...\n");
                                dataStruct toCli;
                                toCli.datF = conditionCode;
                                toCli.destF = NtoC;
                                toCli.intData[0] = EXPPATHDIR;
                                send(new_socket, &toCli, sizeof(toCli), 0);
                            }
                            else
                            {
                                int flag;
                                int req_SS = searchPath(SS_Temp.stringData[0], h);
                                Book[book_count % 20].SS_idxsrc = req_SS;
                                int port_next = 10000;
                                while (isPortFree(port_next) != 1)
                                {
                                    port_next++;
                                }
                                int ack_to_cli_port = port_next;
                                dataStruct toCli;
                                toCli.datF = SS_Temp.datF;
                                toCli.destF = NtoC;
                                toCli.intData[0] = port_next;
                                send(new_socket, &toCli, sizeof(toCli), 0);
                                pthread_t new_thread;
                                MainArgs *MA = (MainArgs *)malloc(sizeof(MainArgs));
                                MA->Main = &SS_Temp;
                                MA->portForCli = port_next++;
                                while (isPortFree(port_next) != 1)
                                {
                                    port_next++;
                                }
                                MA->srcPortnew = port_next++;
                                while (isPortFree(port_next) != 1)
                                {
                                    port_next++;
                                }
                                MA->destPortnew = port_next++;
                                StorageServer *Temp = listOfSS;
                                int port_to_bind;
                                while (Temp->next != NULL)
                                {
                                    if (Temp->next->id == req_SS)
                                    {
                                        port_to_bind = Temp->next->portForSSBind;
                                        strcpy(MA->homepathSrc, Temp->next->path);
                                        break;
                                    }
                                    Temp = Temp->next;
                                }
                                MA->srcPortold = port_to_bind;
                                req_SS = searchPath(SS_Temp.stringData[1], h);
                                MA->destIdx = req_SS;
                                while (Temp->next != NULL)
                                {
                                    if (Temp->next->id == req_SS)
                                    {
                                        port_to_bind = Temp->next->portForSSBind;
                                        strcpy(MA->homepathDest, Temp->next->path);
                                        break;
                                    }
                                    Temp = Temp->next;
                                }
                                MA->destPortold = port_to_bind;
                                Book[book_count % 20].SS_idxdest = req_SS;
                                pthread_create(&new_thread, 0, cpyopn, MA);
                            }
                        }
                        else
                        {
                            dataStruct toCli;
                            toCli.datF = conditionCode;
                            toCli.destF = NtoC;
                            toCli.intData[0] = SERVERDISCONN;
                            send(new_socket, &toCli, sizeof(toCli), 0);
                        }
                    }
                    else
                    {
                        char *path = malloc(sizeof(char) * MAXSIZES);
                        strcpy(path, SS_Temp.stringData[0]);
                        int in_LRU = LRU(path, q);

                        int req_SS;
                        int cache_flag=0;
                        if (in_LRU >= 0)
                        {
                            req_SS = in_LRU;
                            printf("Cache hit!\n");
                        }
                        else
                        {
                            printf("Cache miss!\n");
                            req_SS = searchPath(SS_Temp.stringData[0], h);
                            QNode node_temp = malloc(sizeof(struct que));
                            node_temp->s = malloc(sizeof(struct element));
                            strcpy(node_temp->s->cached_path, SS_Temp.stringData[0]);
                            node_temp->s->quidx = req_SS;
                            enqueue(q, node_temp);
                        }
                        if (req_SS == -1)
                        {
                            dataStruct toCli;
                            toCli.datF = conditionCode;
                            toCli.destF = NtoC;
                            toCli.intData[0] = PATHNOTFOUND;
                            if (send(new_socket, &toCli, sizeof(toCli), 0) == -1)
                            {
                                perror("send");
                            }
                        }
                        else
                        {
                            int port_next = 10000;
                            while (isPortFree(port_next) != 1)
                            {
                                port_next++;
                            }
                            int ack_to_cli_port = port_next;
                            dataStruct toCli;
                            toCli.datF = SS_Temp.datF;
                            toCli.destF = NtoC;
                            toCli.intData[0] = port_next;
                            send(new_socket, &toCli, sizeof(toCli), 0);
                            pthread_t new_thread;
                            MainArgs *MA = (MainArgs *)malloc(sizeof(MainArgs));
                            MA->Main = &SS_Temp;
                            MA->portForCli = port_next++;
                            while (isPortFree(port_next) != 1)
                            {
                                port_next++;
                            }
                            MA->newPortForSS = port_next++;
                            StorageServer *Temp = listOfSS;
                            int port_to_bind;
                            while (Temp->next != NULL)
                            {
                                if (Temp->next->id == req_SS)
                                {
                                    port_to_bind = Temp->next->portForSSBind;
                                    break;
                                }
                                Temp = Temp->next;
                            }
                            MA->oldSSPort = port_to_bind;
                            // MA->flag = flag;
                            Book[book_count % 20].SS_idxsrc = req_SS;
                            pthread_create(&new_thread, 0, prioropn, MA);
                        }
                    }
                }
            }
            else
            {
                dataStruct toCli;
                toCli.datF = conditionCode;
                toCli.destF = NtoC;
                toCli.intData[0] = SERVERDISCONN;
                send(new_socket, &toCli, sizeof(toCli), 0);
            }
        }
        else if (SS_Temp.datF == imdying && SS_Temp.destF == StoN)
        {
            int idx = SS_Temp.intData[0];
            StorageServer *T = listOfSS->next;
            StorageServer *prev = listOfSS;
            while (1)
            {
                if (T->id == idx)
                {
                    Sdead = addNode(T->portForNS, T->portForSSBind, Sdead, "", idx);
                    Sdead->copyid1 = T->copyid1;
                    Sdead->copyid2 = T->copyid2;
                    break;
                }
                prev = T;
                T = T->next;
            }
            prev->next = T->next;
            cur_alive_server--;
            printf("Server %d has disconnected!\n",idx);
            free(T);
        }
        else
        {
            perror("Handle later");
        }
        close(new_socket);
    }
}

int isPortFree(int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == 0)
    {
        close(sockfd);
        return 1;
    }
    else
    {
        close(sockfd);
        return 0;
    }
}

void *Redudancy(void *args)
{
    while (1)
    {
        sleep(20);
        if (cur_alive_server < 2)
            continue;
        else if (cur_alive_server == 2)
        {
            for (int i = 0; i < capacity; i++)
            {
                Node T = h->arr[i]->next;
                while (T != NULL)
                {
                    char curPath[MAXSIZES];
                    strcpy(curPath, T->s);
                    if (strncmp(curPath, "COPY", 4) == 0)
                    {
                        T = T->next;
                        continue;
                    }
                    int idx = T->val;
                    StorageServer *temp = listOfSS->next;
                    while (temp != NULL)
                    {
                        if (temp->id == idx)
                            break;
                        temp = temp->next;
                    }
                    if (temp)
                    {
                        int id1, id2;
                        if (temp->next != NULL)
                        {
                            id1 = temp->next->id;
                        }
                        else
                        {
                            id1 = listOfSS->next->id;
                        }
                        id2 = -1;
                        printf("Copied %s to %d %d\n", curPath, id1, id2);
                        char path1[MAXSIZES], path2[MAXSIZES];
                        sprintf(path1, "COPY%d", id1);
                        // sprintf(path2, "COPY%d", id2);
                        copyPathtoPath(curPath, path1, 0);
                        // copyPathtoPath(curPath, path2, 0);
                        temp->copyid1 = id1;
                        temp->copyid2 = id2;
                    }

                    T = T->next;
                }
            }
        }
        else
        {
            for (int i = 0; i < capacity; i++)
            {
                Node T = h->arr[i]->next;
                while (T != NULL)
                {
                    char curPath[MAXSIZES];
                    strcpy(curPath, T->s);
                    if (strncmp(curPath, "COPY", 4) == 0)
                    {
                        T = T->next;
                        continue;
                    }
                    int idx = T->val;
                    // printf("%d***\n",idx);
                    StorageServer *temp = listOfSS->next;
                    while (temp != NULL)
                    {
                        if (temp->id == idx)
                            break;
                        temp = temp->next;
                    }
                    if (temp)
                    {
                        int id1, id2;
                        if (temp->next != NULL)
                        {
                            id1 = temp->next->id;
                        }
                        else
                        {
                            id1 = listOfSS->next->id;
                        }
                        if (temp->next && temp->next->next != NULL)
                        {
                            id2 = temp->next->next->id;
                        }
                        else
                        {
                            if (temp->next == NULL)
                                id2 = listOfSS->next->next->id;
                            else
                                id2 = listOfSS->next->id;
                        }
                        printf("Copied %s to %d %d\n", curPath, id1, id2);
                        char path1[MAXSIZES], path2[MAXSIZES];
                        sprintf(path1, "COPY%d", id1);
                        sprintf(path2, "COPY%d", id2);
                        copyPathtoPath(curPath, path1, 0);
                        copyPathtoPath(curPath, path2, 0);
                        temp->copyid1 = id1;
                        temp->copyid2 = id2;
                    }

                    T = T->next;
                }
            }
        }
    }
    printf("EXITED\n");
}

int main()
{
    sem_init(&mutex, 0, 1);
    printf("Naming Server Initialised!\n");
    printf("NS master port: %d\n", NS_MASTER_PORT);
    S = malloc(sizeof(StorageServer));
    q = malloc(sizeof(struct que));
    Sdead = malloc(sizeof(StorageServer));
    Sdead->next = NULL;
    int maxx = LRU_MAX;
    init(q, maxx);
    S->next = NULL;
    listOfSS = S; // stores the beginning of linked list, points to the NULL linked list
    listOfDeadSS = Sdead;
    Book = (Log)malloc(sizeof(LogStruct) * 20);
    pthread_t thread;
    int *a = (int *)malloc(sizeof(int));
    *a = NS_MASTER_PORT;
    pthread_create(&thread, NULL, ListenOnAPort, a);
    h = initTable();
    pthread_t pppp;
    pthread_create(&pppp, NULL, Redudancy, NULL);
    // sleep(20);
    // copyPathtoPath("client.c","d2",1);
    while (1)
        ;
}

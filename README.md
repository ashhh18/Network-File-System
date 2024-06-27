# About the project

The project aims at implementing a distributed file system from scratch. The distributed system consists of of a NFS ecosystem that comprises of the following components : 

1. Clients : They represent the users requesting access to files within the network file system and can perform multiple commands like Read, Write, Create, Delete etc.

2. Naming Server: The Naming Server serves as a central hub, managing communication between clients and the storage servers. Its primary function is to provide clients with crucial information about the specific storage server where a requested file or folder resides. 

3. Storage Servers: Storage Servers are responsible for the physical storage and retrieval of files and folders. They manage data persistence and distribution across the network, ensuring that files are stored securely and efficiently.


## Implementation : 

The ecosystem works involves a client that requests for a particular operation on a file/directory. This request is then forwarded to the Naming server which acts as a guide and finds the exact details of the storage server that stores the file/directory that the client has demanded for. Once the required server details is fetched, it returns the storage server details to the client. The client on recieving the details of the server, starts a new communication with that server and then the operation is performed that the client demanded for. 

The operations that a client can perform are :

    1. Read a file
    2. Write a file
    3. Get size & permissions of a file
    4. Create empty file / directory
    5. Delete file / directory
    6. Copy file / directory

The program is capable of handling multiple clients concurrently by using the concept of thrreads and locks. Any number of clients can read a given file at a particular moment, however only 1 client is allowed to write in a file at the same time. In order to count for the time complexity and improve the efficiency of the system, we have an integrated Hashmap (HashTable) that gives us an O(const) complexity to search for the server details. To further improve the performance, there is a queue based implementation of a cache named LRU that stores the details of the recent searches, which helps in reducing any unnecessary search and thereby improving the run time. 

It is further capable of avoiding any failure in the data stored in servers. Every set of data is stored in more than 1 storage server so that in the case of a server failure, the data remains stored. However data in the extra servers has only Read access. And once the server is back again, the data is updated accordingly. 

In order to have a better user-experience for clients, the program correctly displays the exact reasons of any error caused. Each error has an attached error code that makes it more modular. Some of the common possible errors or useful insights along with there error codes are :
    
    SUCCESS 300 
    PATHNOTFOUND 303
    RWSERROR 402
    BADFILEPATH 401
    EXPPATHDIR 405
    FAILURE 299
    TUSRCHAI 69
    TUDESTHAI 79
    SERVERDISCONN 80
    COPYFLAG 81

The output displays are colour coded to visually enhance the program.

## Assumption :
In order to write in a file, the file must be present in the server, i.e if the file does not exist already then we must first create the file and then write.


## Contributors

1. Ameya S Rathod - 2022111021
2. Anish R Joishy - 2022111014
3. Ashwin Kumar   - 2022101091

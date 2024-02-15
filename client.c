#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>

// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define MAX_IP_LENGTH 16


int main() {

 char IP_ADDRESS[MAX_IP_LENGTH];
    int HTTP_PORT;

    printf("Enter the IP Address of the HTTP server: ");
    scanf("%s", IP_ADDRESS);
    printf("%s\n", IP_ADDRESS);

    printf("Enter the HTTP Port of the HTTP server: ");
    scanf("%d", &HTTP_PORT);
    printf("%d\n", HTTP_PORT);
    //----------------------
    // Declare and initialize variables.
    int iResult;
    WSADATA wsaData;

    SOCKET ConnectSocket = INVALID_SOCKET;
    struct sockaddr_in clientService;
 char request[DEFAULT_BUFLEN];
    char host[DEFAULT_BUFLEN];

    printf("Enter the request path: ");
    scanf("%s", request);

    printf("Enter the host: ");
    scanf("%s", host);

    char sendbuf[DEFAULT_BUFLEN];
    snprintf(sendbuf, sizeof(sendbuf), "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", request, host);
    printf("%s\n", sendbuf);

   int recvbuflen = DEFAULT_BUFLEN;
    char recvbuf[DEFAULT_BUFLEN] = "";

    //----------------------
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR) {
        wprintf(L"WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    //----------------------
    // Create a SOCKET for connecting to the server
    ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ConnectSocket == INVALID_SOCKET) {
        wprintf(L"socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    //----------------------
    // The sockaddr_in structure specifies the address family,
    // IP address, and port of the server to be connected to.
    clientService.sin_family = AF_INET;
    clientService.sin_addr.s_addr = inet_addr(IP_ADDRESS);
    clientService.sin_port = htons(HTTP_PORT);

    //----------------------
    // Connect to the server.
    iResult = connect(ConnectSocket, (SOCKADDR*)&clientService, sizeof(clientService));
    if (iResult == SOCKET_ERROR) {
        wprintf(L"connect failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    //----------------------
    // Send an HTTP GET request.
    iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
    if (iResult == SOCKET_ERROR) {
        wprintf(L"send failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    printf("Bytes Sent: %d\n", iResult);

    //----------------------
    // Receive the server's response.
    do {
        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            wprintf(L"Bytes received: %d\n", iResult);
            printf("%.*s", iResult, recvbuf); // Print the received data
        } else if (iResult == 0) {
            wprintf(L"Connection closed\n");
        } else {
            wprintf(L"recv failed with error: %d\n", WSAGetLastError());
        }
    } while (iResult > 0);

    //----------------------
    // Shutdown the connection since no more data will be sent.
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        wprintf(L"shutdown failed with error: %d\n", WSAGetLastError());
    }

    // Close the socket
    closesocket(ConnectSocket);
    WSACleanup();
    return 0;
}

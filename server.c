#define _WIN32_WINNT 0x501
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define MAXCONN 10

// Function to accept a client connection on a given socket.
int accept_client(int sockfd, struct sockaddr *client_addr, socklen_t *addr_size) {
    // Use the accept function to accept a client connection.
    int client_fd = accept(sockfd, client_addr, addr_size);

    // Check if the accept call was successful.
    if (client_fd == INVALID_SOCKET) {
        // Print an error message using perror if accept fails.
        perror("accept failed");

        // Return -1 to indicate failure.
        return -1;
    }

    // Return the socket descriptor for the accepted client on success.
    return client_fd;
}



// Function to receive data from a connected client.

int receive_client(int client_fd, char *buffer, int buffer_size) {
    // Use the recv function to receive data from the connected client.
    int n = recv(client_fd, buffer, buffer_size - 1, 0);

    // Check if recv call encountered an error.
    if (n == SOCKET_ERROR) {
        // Print an error message using perror.
        perror("recv failed");

        // Close the socket and return an error value indicating failure.
        closesocket(client_fd);
        return -1;
    }

    // Check if the client closed the connection.
    if (n == 0) {
        // Print a message indicating the connection closure by the client.
        perror("Connection closed by the client");

        // Close the socket and return a specific value to indicate connection closed.
        closesocket(client_fd);
        return -2; 
    }

    // Null-terminate the received data to treat it as a C-style string.
    buffer[n] = '\0';

    // Return the number of bytes received on success.
    return n;
}



// Function to send an HTTP response to a connected client.
void send_response(int client_fd, const char *headers, const char *html) {
    // Calculate the total length needed for headers and html, plus space for a null terminator.
    size_t total_length = strlen(headers) + strlen(html) + 1;

    // Allocate a buffer dynamically based on the total length.
    char *data = (char *)malloc(total_length);

    // Check if memory allocation was successful.
    if (data == NULL) {
        // Print an error message and close the socket on memory allocation failure.
        perror("Memory allocation failed");
        closesocket(client_fd);
        return;
    }

    // Concatenate headers and html into the dynamically allocated buffer.
    snprintf(data, total_length, "%s %s", headers, html);

    // Send the data over the socket.
    int sent_bytes = send(client_fd, data, total_length - 1, 0);

    // Check if the send operation encountered an error.
    if (sent_bytes == SOCKET_ERROR) {
        // Print an error message if the send operation failed.
        perror("send failed");
    }

    // Free the dynamically allocated buffer.
    free(data);

    // Optionally handle the socket error by closing the socket.
    if (sent_bytes == SOCKET_ERROR) {
        closesocket(client_fd);
    }
}



// Function to serve an image file to a connected client.
void serve_image(int client_fd, FILE *file, const char *filename) {
    // Define a fixed buffer size for headers and data.
    const size_t buffer_size = 2048;

    // Allocate memory for headers and data buffers dynamically.
    char *headers = (char *)malloc(buffer_size);
    char *data = (char *)malloc(buffer_size);

    // Check if memory allocation was successful.
    if (headers == NULL || data == NULL) {
        // Print an error message, close the file and socket, and return.
        perror("Memory allocation failed");
        fclose(file);
        closesocket(client_fd);
        return;
    }

    // Retrieve the file size by seeking to the end and back to the beginning.
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Check for errors in file size calculation.
    if (file_size == -1L) {
        // Print an error message, free allocated memory, close the file and socket, and return.
        perror("Error getting file size");
        free(headers);
        free(data);
        fclose(file);
        closesocket(client_fd);
        return;
    }

//Headers for png
    if (strstr(filename, ".png")) {
        snprintf(headers, buffer_size, "HTTP/1.0 200 OK\r\nServer: C\r\nContent-type: image/png\r\nContent-Length: %ld\r\n\r\n", file_size);
    }

    //Headers for jpg
    else if (strstr(filename, ".jpg") || strstr(filename, ".jpeg")) {
        snprintf(headers, buffer_size, "HTTP/1.0 200 OK\r\nServer: C\r\nContent-type: image/jpeg\r\nContent-Length: %ld\r\n\r\n", file_size);
    }

    //headers for jpeg
    else if (strstr(filename, ".jpeg")) {
        snprintf(headers, buffer_size, "HTTP/1.0 200 OK\r\nServer: C\r\nContent-type: image/jpeg\r\nContent-Length: %ld\r\n\r\n", file_size);
    }

    //headers for gif
    else if (strstr(filename, ".gif")) {
        snprintf(headers, buffer_size, "HTTP/1.0 200 OK\r\nServer: C\r\nContent-type: image/gif\r\nContent-Length: %ld\r\n\r\n", file_size);
    }

    

    // Send the headers over the socket.
    if (send(client_fd, headers, strlen(headers), 0) == SOCKET_ERROR) {
        // Print an error message, free allocated memory, close the file and socket, and return.
        perror("Error sending headers");
        free(headers);
        free(data);
        fclose(file);
        closesocket(client_fd);
        return;
    }

    // Read and send the image file in chunks.
    size_t bytesRead;
    while ((bytesRead = fread(data, 1, buffer_size, file)) > 0) {
        // Send each chunk of data over the socket.
        if (send(client_fd, data, bytesRead, 0) == SOCKET_ERROR) {
            // Print an error message, but continue cleanup without returning immediately.
            perror("Error sending data");
            break;
        }
    }

    // Free the dynamically allocated memory.
    free(headers);
    free(data);

    // Close the file and socket.
    fclose(file);
    closesocket(client_fd);
}




// Function to serve an HTML file to a connected client.
void serve_html(int client_fd, FILE *file) {
    // Define fixed buffer sizes for headers and data.
    char headers[2048];
    char data[2048];

    // Retrieve the file size by seeking to the end and back to the beginning.
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Check for errors in file size calculation.
    if (file_size == -1L) {
        // Print an error message, close the file and socket, and return.
        perror("Error getting file size");
        fclose(file);
        closesocket(client_fd);
        return;
    }

    // Construct headers with information about the HTML file.
    snprintf(headers, sizeof(headers), "HTTP/1.0 200 OK\r\nServer: C\r\nContent-type: text/html\r\nContent-Length: %ld\r\n\r\n", file_size);

    // Send the headers over the socket.
    if (send(client_fd, headers, strlen(headers), 0) == SOCKET_ERROR) {
        // Print an error message, close the file and socket, and return.
        perror("Error sending headers");
        fclose(file);
        closesocket(client_fd);
        return;
    }

    // Read and send the HTML file in chunks.
    size_t bytesRead;
    while ((bytesRead = fread(data, 1, sizeof(data), file)) > 0) {
        // Send each chunk of data over the socket.
        if (send(client_fd, data, bytesRead, 0) == SOCKET_ERROR) {
            // Print an error message, close the file and socket, and return.
            perror("Error sending data");
            fclose(file);
            closesocket(client_fd);
            return;
        }
    }

    // Close the file and socket after successful transmission.
    fclose(file);
    closesocket(client_fd);
}


// Function to handle a client request.


int serve_client(int client_fd) {
    // Define a fixed buffer size for receiving the client's request.
    const size_t buffer_size = 2048;

    // Allocate memory for the request buffer dynamically.
    char *buffer = (char *)malloc(buffer_size);

    // Check if memory allocation was successful.
    if (buffer == NULL) {
        // Print an error message, close the socket, and return with failure.
        perror("Memory allocation failed");
        closesocket(client_fd);
        return EXIT_FAILURE;
    }

    // Receive the client's request into the buffer.
    int n = receive_client(client_fd, buffer, buffer_size);

    // Print the received request for debugging purposes.
    printf("Request: %s\n", buffer);

    // Check if the client closed the connection.
    if (n == 0) {
        // Print a message and return with failure as the connection is closed by the client.
        printf("Connection closed by the client\n");
        free(buffer);
        closesocket(client_fd);
        return EXIT_FAILURE;
    }

    // Parse the request to extract the filename.
  chdir(".//www");
char filename[100];
if (sscanf(buffer, "GET /%s", filename) != 1) {
    // Print an error message, free allocated memory, close the socket, and return with failure.
    printf("Error parsing request\n");
    free(buffer);
    closesocket(client_fd);
    return EXIT_FAILURE;
}

// Check if the requested filename contains ".." indicating an attempt to access parent directory
if (strstr(filename, "..") != NULL) {
    // Send a response indicating unauthorized access
    char headers[] = "HTTP/1.0 403 Forbidden\r\nServer: C\r\nContent-type: text/html\r\n\r\n";
    char html[] = "<html><body><h1>403 Forbidden - Unauthorized Access</h1></body></html>";
    send_response(client_fd, headers, html);

    // Free allocated memory, close the socket, and return with failure.
    free(buffer);
    closesocket(client_fd);
    return EXIT_FAILURE;
}

 


    // Handle the case where the filename is "HTTP/1.1" (usually a placeholder for default requests).
    if (strcmp(filename, "HTTP/1.1") == 0) {
        strcpy(filename, "index.html");
    }

    // Open the file based on the filename and content type.
    FILE *file = fopen(filename, strstr(filename, ".png") || strstr(filename, ".jpg") || strstr(filename, ".jpeg") || strstr(filename, ".gif")? "rb" : "r");

    // Check if the file opening was successful.
    if (file == NULL) {
        perror("Error opening file");
        char headers[] = "HTTP/1.0 404 Not Found\r\nServer: C\r\nContent-type: text/html\r\n\r\n";
        char html[] = "<html><body><h1>404 Not Found</h1></body></html>";
        send_response(client_fd, headers, html);
    } else {
        // Check the file extension and serve the appropriate response.
        if (strstr(filename, ".jpg") || strstr(filename, ".jpeg") || strstr(filename, ".png") || strstr(filename, ".gif")) {
            serve_image(client_fd, file, filename);
        } else {
            serve_html(client_fd, file);
        }
        // Close the file after serving.
        fclose(file);
    }

    // Print the received request for debugging purposes.
    printf("Request: %s\n", buffer);

    // Free the dynamically allocated buffer.
    free(buffer);

    // Close the client socket.
    closesocket(client_fd);

    // Return with success.
    return EXIT_SUCCESS;
}



int main(int argc, char **argv)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        perror("WSAStartup failed");
        return EXIT_FAILURE;
    }

    struct addrinfo hints, *server;
    memset(&hints, 0, sizeof hints);

    hints.ai_family = AF_INET; // AF_INET means IPv4 only addresses
    hints.ai_socktype = SOCK_STREAM; // SOCK_STREAM means TCP
    hints.ai_flags = AI_PASSIVE; 
    hints.ai_protocol = IPPROTO_TCP; // IPPROTO_TCP means TCP

    if (getaddrinfo(NULL, "80", &hints, &server) != 0)
    {
        perror("getaddrinfo failed");
        WSACleanup();
        return EXIT_FAILURE;
    }

    int sockfd = socket(server->ai_family, server->ai_socktype, server->ai_protocol); // Create a socket
    if (sockfd == INVALID_SOCKET)
    {
        perror("socket failed");
        freeaddrinfo(server);
        WSACleanup();
        return EXIT_FAILURE;
    }

    if (bind(sockfd, server->ai_addr, server->ai_addrlen) == SOCKET_ERROR)// Bind the socket to the address and port
    {
        perror("bind failed");
        closesocket(sockfd);
        freeaddrinfo(server);
        WSACleanup();
        return EXIT_FAILURE;
    }

    if (listen(sockfd, MAXCONN) == SOCKET_ERROR) // Listen for incoming connections
    {
        perror("listen failed");
        closesocket(sockfd);
        freeaddrinfo(server);
        WSACleanup();
        return EXIT_FAILURE;
    }

    struct sockaddr_storage client_addr;
    socklen_t addr_size = sizeof client_addr;
    char buffer[2048];
    int client_id = 0;
    while (1)
    {
        int client_fd = accept_client(sockfd, (struct sockaddr*)&client_addr, &addr_size);

        if (client_fd < 0)
        {
           perror("accept failed");
           continue;
        }
         
         
   

        serve_client(client_fd);
        closesocket(client_fd);
        client_id++;
        printf("Client %d served\n", client_id);
    }

    closesocket(sockfd);
    freeaddrinfo(server);
    WSACleanup();
    return EXIT_SUCCESS;
}

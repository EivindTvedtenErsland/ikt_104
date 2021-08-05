#include "mbed.h"
#include "rapidjson/document.h"

using namespace rapidjson;

nsapi_size_or_error_t send_request(TCPSocket &socket, const char *request) {
    // The request might not be fully sent in one go, so keep track of how much we have sent
    nsapi_size_t bytes_to_send = strlen(request);
    nsapi_size_or_error_t bytes_sent = 0;

    printf("Sending message: \n%s", request);

    // Loop as long as there are more data to send
    while (bytes_to_send) {
        // Try to send the remaining data. send() returns how many bytes were actually sent
        bytes_sent = socket.send(request + bytes_sent, bytes_to_send);

        // Negative return values from send() are errors
        if (bytes_sent < 0) {
            return bytes_sent;
        } else {
            printf("sent %d bytes\n", bytes_sent);
        }

        bytes_to_send -= bytes_sent;
    }

    printf("Complete message sent\n");

    return 0;
}

nsapi_size_or_error_t read_response(TCPSocket &socket, char buffer[], int buffer_length) {
    memset(buffer, 0, buffer_length);

    int remaining_bytes = 1000;
    int received_bytes = 0;

    // Loop as long as there are more data to read. We might not read all in one call to recv()
    while (remaining_bytes > 0) {
        nsapi_size_or_error_t result = socket.recv(buffer + received_bytes, remaining_bytes);

        // If the result is 0 there are no more bytes to read
        if (result == 0) {
            break;
        }

        // Negative return values from recv() are errors
        if (result < 0) {
            return result;
        }

        received_bytes += result;
        remaining_bytes -= result;
    }

    printf("received %d bytes:\n%.*s\n", received_bytes,
                 strstr(buffer, "\n") - buffer, buffer);

    return 0;
}

int main() {
    // Get pointer to default network interface
    NetworkInterface *network = NetworkInterface::get_default_instance();

    if (!network) {
        printf("Failed to get default network interface\n");
        while (1);
    }

    // Connect to network
    printf("Connecting to the network...\n");
    nsapi_size_or_error_t result = network->connect();

    // Check if the connection is successful
    if (result != 0) {
        printf("Failed to connect to network: %d\n", result);
        while (1);
    }

    printf("Connection to network successful!\n");

    // Create and allocate resources for socket
    TCPSocket socket;
    socket.open(network);

    // Create destination address
    SocketAddress address;

    // Get IP address of host by name
    result = network->gethostbyname("www.mocky.io", &address);

    // Check result
    if (result != 0) {
        printf("Failed to get IP address of host: %d\n", result);
        while (1);
    }

    printf("Got address of host\n");

    // Set server port number
    address.set_port(80);

    // Connect to server at the given address
    result = socket.connect(address);

    // Check result
    if (result != 0) {
        printf("Failed to connect to server: %d\n", result);
        while (1);
    }

    printf("Connection to server successful!\n");

    // Create HTTP request
    const char request[] = "GET /v2/5e37e64b3100004c00d37d03 HTTP/1.1\r\n"
                           "Host: www.mocky.io\r\n"
                           "Connection: close\r\n"
                           "\r\n";

    // Send request
    result = send_request(socket, request);

    // Check result
    if (result != 0) {
        printf("Failed to send request: %d\n", result);
        while (1);
    }

    static char buffer[4096];
    // Read response
    result = read_response(socket, buffer, sizeof(buffer));
    
    // Check result
    if (result != 0) {
        printf("Failed to read response: %d\n", result);
        while (1);
    }

    // Close and free socket. Without this RapidJSON will crash.
    socket.close();
    //delete socket;

    // Find the start of the JSON data
    char* json = strstr(buffer, "{");

    // Check if we actually got JSON in the response
    if (json == nullptr) {
        printf("Failed to find JSON in response\n");
        while (1);
    }

    printf("JSON response:\n");
    printf("%s\n", json);

    // Parse response as JSON, starting from the first {
    Document document;
    document.Parse(json);

    printf("First name from JSON data: %s\n", document["first name"].GetString());
    printf("Last name from JSON data: %s\n", document["last name"].GetString());
    printf("Age from JSON data: %d\n", (int) document["age"].GetInt());

    while (true) {
        ThisThread::sleep_for(1s);
    }
}

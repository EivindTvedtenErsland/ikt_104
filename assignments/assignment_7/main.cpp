/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
*/

#include "mbed.h"
#include "HTS221Sensor.h"
#include "rapidjson/document.h"
#include <stdint.h>

#define WAIT_TIME_MS 500 

using namespace rapidjson;

DigitalOut led1(LED1);

DevI2C i2c(PB_11, PB_10);
HTS221Sensor sensor(&i2c);

char DeviceID[] = "<Your ID>";
char DeviceToken[] = "<Your Token>";
char ThingBoardsAddress[] = "";
char TBEndPoint[] = "";

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

int main()
{
    uint8_t id;
    float humidity, temperature;

    printf("This is the bare metal blinky example running on Mbed OS %d.%d.%d.\n", MBED_MAJOR_VERSION, MBED_MINOR_VERSION, MBED_PATCH_VERSION);

    sensor.init(nullptr);
    sensor.enable();

    sensor.read_id(&id);
    printf("HTS221 humidity & temperature = 0x%X\r\n", id);

       /*
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
    result = network->gethostbyname(ThingBoardsAddress, &address);

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

    //TBEndPoint
    // Create HTTP request
    const char request[] = "POST /api/v1/0dRrL7DmUAk63GnBFe8q/telemetry HTTP/1.1\r\n"
                           "Host: localhost:9090 \r\n"
                           "Content-Type: application/x-www-form-urlencoded \r\n"
                           "Content-Length: 21 \r\n"
                           "{ 'temperature' : 23 } \r\n"
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
    //Document document;
    //document.Parse(json);

    //printf("First name from JSON data: %s\n", document["first name"].GetString());
    */

    while (true)
    {   
       
        sensor.get_humidity(&humidity);
        sensor.get_temperature(&temperature);
        printf("HTS221: [temp] %.2f C, [hum] %.2f\r\n", temperature, humidity);     
        led1 = !led1;
        thread_sleep_for(WAIT_TIME_MS);
    }
}



 

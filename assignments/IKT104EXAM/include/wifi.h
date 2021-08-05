#include "mbed.h"

nsapi_size_or_error_t send_request(TLSSocket *socket, const char *request);
nsapi_size_or_error_t read_response(TLSSocket *socket, char buffer[], int buffer_length);
nsapi_size_or_error_t send_request_tcp(TCPSocket &socket, const char *request);
nsapi_size_or_error_t read_response_tcp(TCPSocket &socket, char buffer[], int buffer_length);
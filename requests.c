#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

char *recv_post_req(int socket, char host[16], char *command, char *user[1], char *token) {
	char *message = compute_post_request(host, command, "application/json", user, 1, NULL, 0, token);
	send_to_server(socket, message);

	return receive_from_server(socket);
}

char *recv_get_req(int socket, char host[16], char *command, char *token, char *cookies[1], char *get_delete) {
	char *message = compute_get_request(host, command, NULL, cookies, 1, token, get_delete);
	send_to_server(socket, message);

	return receive_from_server(socket);
}

char* compute_get_request(char* host, char* url, char* query_params,
    char** cookies, int cookies_count, char *token, char *command)
{
    char* message = calloc(BUFLEN, sizeof(char));
    char* line = calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL, request params (if any) and protocol type
	memset(line, 0, LINELEN);
    if (query_params != NULL) {

        if(strcmp(command, "get") == 0)
            sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
        else
            sprintf(line, "DELETE %s?%s HTTP/1.1", url, query_params);
    }
    else {
        if(strcmp(command, "get") == 0) 
            sprintf(line, "GET %s HTTP/1.1", url);
        else
            sprintf(line, "DELETE %s HTTP/1.1", url);
    }
    compute_message(message, line);

    // Step 2: add the host
    memset(line, 0, strlen(line));
    sprintf(line, "Host: %s", host);
    compute_message(message, line);


    // Step 3 (optional): add headers and/or cookies, according to the protocol format
    if (cookies != NULL) {
		sprintf(line, "Cookie: %s", cookies[0]);
        compute_message(message, line);
    }

	if(token != NULL) {
        memset(line, 0, strlen(line));
		sprintf(line, "Authorization: Bearer %s", token);
		compute_message(message, line);
	}
    // Step 4: add final new line
    compute_message(message, "\n");
    return message;
}

char* compute_post_request(char* host, char* url, char* content_type, char** body_data,
    int body_data_fields_count, char** cookies, int cookies_count, char *token)
{
    char* message = calloc(BUFLEN, sizeof(char));
    char* line = calloc(LINELEN, sizeof(char));
    char* body_data_buffer = calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL and protocol type
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);

	if(token != NULL) {
        memset(line, 0, strlen(line));
		sprintf(line, "Authorization: Bearer %s", token);
		compute_message(message, line);
	}
    // Step 2: add the host
    memset(line, 0, strlen(line));
    sprintf(line, "Host: %s", host);
    compute_message(message, line);
    
    //
    
    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    if (body_data != NULL) {
		memset(body_data_buffer, 0, LINELEN);
    	strcat(body_data_buffer, "");
    	strcat(body_data_buffer, body_data[0]);
	}

    memset(line, 0, LINELEN);
    sprintf(line, "Content-Length: %ld", strlen(body_data_buffer));
    compute_message(message, line);

    // Step 4 (optional): add cookies
    if (cookies != NULL) {
    }

    // Step 5: add new line at end of header
    strcat(message, "\n");

    // Step 6: add the actual payload data
	
	compute_message(message, body_data_buffer);

    free(line);
    return message;
}
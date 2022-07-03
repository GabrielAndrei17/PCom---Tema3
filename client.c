#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include <stdbool.h>
#include <ctype.h>

#include "helpers.h"
#include "requests.h"
#include "parson.h"

int main(int argc, char *argv[]) {
	int connected = 0, in_library = 0, exit = 0;
	char host[16] = "34.241.4.235";
	int port = 8080;
	int socket = open_connection(host, port, AF_INET, SOCK_STREAM, 0);

	char *user[1], *cookies[1], token[BUFLEN], *addbook[1];
	char command[BUFLEN];
	while(!exit) {

		fgets(command, BUFLEN, stdin);
		
		// Se deschide conexiuna la server pentru fiecare comanda
		socket = open_connection(host, port, AF_INET, SOCK_STREAM, 0);

		
		if(strncmp(command, "register", 8) == 0 || strncmp(command, "login", 5) == 0) {
			// Daca vrem sa ne inregistram sau sa ne logam, se preia parola si numele

			char *username = calloc(BUFLEN, sizeof(char));
			char *password = calloc(BUFLEN, sizeof(char));

			printf("username=");
			scanf("%s", username);
			printf("password=");
			scanf("%s", password);

			JSON_Value *value = json_value_init_object();
			JSON_Object *object = json_value_get_object(value);
			json_object_set_string(object, "username", username);
			json_object_set_string(object, "password", password);
			user[0] = json_serialize_to_string(value);

			if (strncmp(command, "register", 8) == 0) { //register

				if(strstr(recv_post_req(socket, host, REGISTER, user, NULL), "is taken") != NULL) {
					error("TAKEN USERNAME! Try something else.\n");
				} else {
					printf("SUCCESS! You are now registred.\n");
				}

			}
			else { //login

				char *cookie = strstr(recv_post_req(socket, host, LOGIN, user, NULL), "Set-Cookie: ");

				if (cookie == NULL) {
					error("WRONG PASSWORD!\n");
					connected = 0;
					in_library = 0;
					continue;
				}
				//Se va retine cookie-ul prin care demonstram ca suntem logati

				strtok(cookie, ";");
				cookie += 12;
				
				cookies[0] = cookie;

				if(cookie != NULL) {
					printf("SUCCESS! You are now logged in!\n");
				}
				connected = 1;
			}
		}

		else if (strncmp(command, "logout", 6) == 0) {
			if (connected == 1) {
				// Daca suntem conectati, trimitem comanda de delogare
	
				connected = 0;
				in_library = 0;
				recv_get_req(socket, host, LOGOUT, token, cookies, "get");
				printf("Success!\n");
			} else 
				error("You are not logged in!\n");
		}

		else if (strncmp(command, "enter_library", 13) == 0) {

			if(connected && !in_library) {
				// Daca suntem logati si nu suntem in librarie, se retine tokenul pentru verificarea accesului
				
				char *tok = strstr(recv_get_req(socket, host, ACCESS, token, cookies, "get"), "token");

				if (tok == NULL)
					error("NO ACCESS IN LIBRARY!\n");
				 else {

					tok += 8;
					memset(token, 0, BUFLEN);
					strcpy(token, tok);
					token[strlen(token) - 2] = '\0';

					in_library = 1;
					printf("SUCCESS!\n");
				}
			}
			else if(!connected) 
				error("You are not logged in.\n");
			else
				printf("You are already in library.\n");
		}

		else if (strncmp(command, "get_books", 9) == 0) {
			if (in_library == 1) {

				// Daca suntem in librarie, se cauta cartile retinute
				char* res = recv_get_req(socket, host, BOOKS, token, cookies, "get");
				printf("%s\n", strstr(res, "["));
			} else
				error("You are not in library.\n");

		}
		else if (strncmp(command, "get_book", 8) == 0) {
			if (in_library == 1) {
				// Daca suntem in librarie, se preia id-ul

				char route[BUFLEN], bok[BUFLEN];
				int book_id = 0;

				printf("id=");
				scanf("%s", bok);
				book_id = atoi(bok);
				if(book_id <= 0) {
					error("WRONG FORMAT! Try again!\n");
					continue;
				}

				sprintf(route, "%s/%d", BOOKS, book_id);

				// Se cauta cartile retinute cartile retinute
				char* res = recv_get_req(socket, host, route, token, cookies, "get");

				if (strstr( res, "No book was found!") != NULL)
					error("NO BOOK WAS FOUND!\n");
				else 
					printf("%s\n", strstr(res, "[") );
			}
			else 
				error("You are not in library.\n");
		}

		else if (strncmp(command, "add_book", 8) == 0) {
			if (in_library == 1) {
				// Daca suntem in librarie, se preiau informatiile despre carte

				char title[BUFLEN], author[BUFLEN], genre[BUFLEN], publisher[BUFLEN], pg[BUFLEN];
				int pages;

				printf("title=");
				scanf("%s", title);
				printf("author=");
				scanf("%s", author);
				printf("genre=");
				scanf("%s", genre);
				printf("publisher=");
				scanf("%s", publisher);
				printf("pages=");
				scanf("%s", pg);
				
				pages = atoi(pg);
				if(pages <= 0){
					error("WRONG FORMAT! Try again!\n");
					continue;
				}
				// Se construieste payload si se trimite request

				JSON_Value *value = json_value_init_object();
				JSON_Object *object = json_value_get_object(value);
				json_object_set_string(object, "title", title);
				json_object_set_string(object, "author", author);
				json_object_set_string(object, "genre", genre);
				json_object_set_string(object, "page_count", pg);
				json_object_set_string(object, "publisher", publisher);
				addbook[0] = json_serialize_to_string(value);

				recv_post_req(socket, host, BOOKS, addbook, token);
				printf("Success!\n");
			}
			else
				error("You are not in library.\n");
		}
		else if (strncmp(command, "delete_book", 11) == 0) {
			if (in_library == 1) {
				// Daca suntem in librarie, se preia id-ul

				char route[BUFLEN], bok[BUFLEN];
				int book_id = 0;

				printf("id=");
				scanf("%s", bok);
				book_id = atoi(bok);
				if(book_id <= 0) {
					error("WRONG FORMAT! Try again!\n");
					continue;
				}

				sprintf(route, "%s/%d", BOOKS, book_id);

				// Se sterge cartea cu id-ul dat(daca exista)
				char *deleted = strstr(recv_get_req(socket, host, route, token, cookies, "delete"), "No book was deleted!");
				if (deleted != NULL)
					error("NO BOOK! Entered id is not valid!\n");
				else
					printf("Success!\n");
			} 
			else 
				error("You are not in library.\n");

		}

		else if(strncmp(command, "exit", 4) == 0) {
			// Se iese din while
			exit = 1;
		}
		close_connection(socket);
	}
}
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

char *compute_get_request(char *host, char *url, char *query_params,
                            char **cookies, int cookies_count)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    //Se adauga in mesaj tipul cererii, url-ul si tipul continutului
    if (query_params != NULL) {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }
    compute_message(message, line);

    //Se adauga adresa ip a server-ului
    strcpy(line, "Host: ");
    strcat(line, host);
    compute_message(message, line);

    //Se adauga cookie-urile
    if (cookies != NULL) {
        strcpy(line, "Cookie: ");
        for(int i = 0; i < cookies_count; i++) {
            strcat(line, cookies[i]);
            if(i + 1  < cookies_count) {
                strcat(line, ";");
            }
        }
        compute_message(message, line);
    }

    //Se adauga token-ul jwt care va fi in vectorul de cookies la final
    strcpy(line, "");
    sprintf(line, "Authorization: Bearer %s", cookies[cookies_count]);
    compute_message(message, line);
    
    compute_message(message, "");
    
    free(line);
    return message;
}

char *compute_delete_request(char *host, char *url, char *query_params,
                            char **cookies, int cookies_count)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    //Se adauga in mesaj tipul cererii, url-ul si tipul continutului
    if (query_params != NULL) {
        sprintf(line, "DELETE %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "DELETE %s HTTP/1.1", url);
    }

    compute_message(message, line);

    //Se adauga adresa server-ului
    strcpy(line, "Host: ");
    strcat(line, host);
    compute_message(message, line);

    //Se adauga cookie-urile si token-ul jwt dupa acestea
    if (cookies != NULL) {
        strcpy(line, "Cookie: ");
        for(int i = 0; i < cookies_count; i++) {
            strcat(line, cookies[i]);
            if(i + 1  < cookies_count) {
                strcat(line, ";");
            }
        }
        compute_message(message, line);
    }
    
    strcpy(line, "");
    sprintf(line, "Authorization: Bearer %s", cookies[cookies_count]);
    compute_message(message, line);
    
    compute_message(message, "");
    
    free(line);
    return message;
}

char *compute_post_request(char *host, char *url, char* content_type, char *body_data,
                            int body_data_len, char **cookies, int cookies_count)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    //Se adauga in mesaj tipul cererii si url-ul
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);
    
    //Se adauga adresa server-ului
    strcpy(line, "Host: ");
    strcat(line, host);
    compute_message(message, line);
    
    //Se adauga tipul continutului si lungimea acestuia
    strcpy(line, "Content-Type: application/");
    strcat(line, content_type);
    compute_message(message, line);  
    
    sprintf(line, "Content-Length: %d", body_data_len);
    compute_message(message, line);
    
    //Se adauga token-ul jwt
    if (cookies != NULL) {
        strcpy(line, "");
        sprintf(line, "Authorization: Bearer %s", cookies[cookies_count]);
        compute_message(message, line);
    }
    
    compute_message(message, "");

    //Se adauga continutul de tip json
    memset(line, 0, LINELEN);
    strcpy(line, body_data);
    compute_message(message, line);

    free(line);
    return message;
}

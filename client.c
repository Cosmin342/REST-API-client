#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"

//Functie ce intoarce un numar care identifica o anumita comanda
int console(char* command) {
    if (strcmp(command, "register") == 0) return 1;
    if (strcmp(command, "login") == 0) return LOGIN;
    if (strcmp(command, "enter_library") == 0) return ENTER_LIB;
    if (strcmp(command, "get_books") == 0) return BOOKS;
    if (strcmp(command, "get_book") == 0) return BOOK;
    if (strcmp(command, "add_book") == 0) return ADD_BOOK;
    if (strcmp(command, "delete_book") == 0) return DEL;
    if (strcmp(command, "logout") == 0) return LOGOUT;
    if (strcmp(command, "exit") == 0) return EXIT_SV;
    return 0;
}

//Functie ce intoarce tip-ul raspunsului dat de server-ul HTTP
char* process_response(char* response) {
    char* type = calloc(TYPE, sizeof(char));
    //Se extrage folosind sscanf pe primul rand
    sscanf(response, "HTTP/1.1 %s", type);
    return type;
}

//Functie ce extrage un cookie dintr-un raspuns dat de server
char* extract_cookie(char* response) {
    char* cookie = calloc(LINELEN, sizeof(char));
    char response_copy[BUFLEN];
    strcpy(response_copy, response);
    char* token = strtok(response_copy, "\n");
    while (token != NULL)
    {
        /*
        Daca se gaseste un cookie in raspuns, acesta este extras utilizand
        sscanf
        */
        if (strstr(token, "Set-Cookie:") != NULL) {
            sscanf(token, "Set-Cookie: %s", cookie);
            cookie[strlen(cookie) - 1] = '\0';
            break;
        }
        token = strtok(NULL, "\n");
    }
    return cookie;
}

//Functie pentru extragerea token-ului jwt dintr-un raspuns al server-ului
char* extract_jwt(char* response) {
    char* jwt = calloc(LINELEN, sizeof(char));
    char response_copy[BUFLEN];
    strcpy(response_copy, response);
    char* token = strtok(response_copy, "\n");
    while (token != NULL)
    {
        /*
        Daca in linia curenta apare caracterul {, inseamna ca exista si un
        obiect de tip json ce contine token-ul
        */
        if (strchr(token, '{') != NULL) {
            //Se parseaza string-ul cu json-ul si se extrage token-ul
            JSON_Value *value = json_parse_string(token);
            JSON_Object *object = json_value_get_object(value);
            strcpy(jwt, json_object_get_string(object, "token"));
            json_value_free(value);
            break;
        }
        token = strtok(NULL, "\n");
    }
    return jwt;
}

//Functie pentru extragerea cartilor din raspunsul dat de server
char* get_books(char* response) {
    char* books = calloc(BUFLEN, sizeof(char));
    char response_copy[BUFLEN];
    strcpy(response_copy, response);
    char* token = strtok(response_copy, "\n");
    while (token != NULL)
    {
        /*
        Daca se gasete caracterul [ ce semnifica inceputul unui vector de
        carti, se extrage vectorul si se opreste cautarea
        */
        if (strchr(token, '[') != NULL) {
            strcpy(books, token);
            break;
        }
        token = strtok(NULL, "\n");
    }
    return books;
}

//Functie pentu inregistrarea unui utilizator in sistem
void register_user(int sockfd) {
    char username[VAR_LEN], password[VAR_LEN], *message, *response;
    char *serialized_string;
    //Se citesc numele si parola
    printf("username=");
    fgets(username, VAR_LEN, stdin);
    username[strlen(username) - 1] = '\0';
    printf("password=");
    fgets(password, VAR_LEN, stdin);
    password[strlen(password) - 1] = '\0';
    //Se creeaza un obiect json cu cele doua valori si se converteste in string
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    json_object_set_string(root_object, "username", username);
    json_object_set_string(root_object, "password", password);
    serialized_string = json_serialize_to_string_pretty(root_value);
    //Se creeaza mesajul de tip post, utilizand comanda din laborator
    message = compute_post_request("34.118.48.238",
        "/api/v1/tema/auth/register", "json", serialized_string,
        strlen(serialized_string), NULL, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    //Se extrage si se verifica tipul raspunsului
    response = process_response(response);
    //Daca este de tip 400, numele dat este deja folosit
    if (strcmp(response, "400") == 0) {
        printf("The username %s is taken!\n", username);
    }
    //Daca este 201, inregistrarea s-a realizat cu succes
    else if (strcmp(response, "201") == 0) {
        printf("Successful registration!\n");
    }
    free(response);
    free(message);
    json_free_serialized_string(serialized_string);
    json_value_free(root_value);
}

//Functie pentru accesarea bibliotecii
char* enter_library(int sockfd, char* cookie, char* username,
    char **body_data) {
    char *jwt, *message, *response;
    //Daca cookie-ul dat nu este null, se realizeaza mesajul de tip get
    if (cookie != NULL) {
        strcpy(body_data[0], cookie);
        message = compute_get_request("34.118.48.238",
            "/api/v1/tema/library/access", NULL, body_data, 1);
    }
    //Altfel, se afiseaza un mesaj de eroare
    else {
        printf("No user logged in!\n");
        return NULL;
    }
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    /*
    Se extrag token-ul jwt din raspuns, daca este possibil, si tipul raspunsului
    */
    jwt = extract_jwt(response);
    response = process_response(response);
    /*
    Daca tipul este 400, se va afisa un mesaj de eroare. In caz contrar,
    accesul la biblioteca s-a realizat cu succes
    */
    if (strcmp(response, "400") == 0) {
        printf("Bad Request!\n");
    }
    else if (strcmp(response, "200") == 0) {
        printf("The user %s accessed the libray!\n", username);
    }
    free(response);
    free(message);
    return jwt;
}

//Functie pentru adaugarea unei carti in biblioteca
void add_book(int sockfd, char* jwt, char **body_data) {
    char author[BOOK_DET], genre[BOOK_DET], publisher[VAR_LEN], id[TYPE];
    char *message, *response, *serialized_string, title[BOOK_DET];
    int pages;
    //Se citesc datele cartii
    printf("title=");
    fgets(title, BOOK_DET, stdin);
    title[strlen(title) - 1] = '\0';
    printf("author=");
    fgets(author, BOOK_DET, stdin);
    author[strlen(author) - 1] = '\0';
    printf("genre=");
    fgets(genre, BOOK_DET, stdin);
    genre[strlen(genre) - 1] = '\0';
    printf("publisher=");
    fgets(publisher, VAR_LEN, stdin);
    publisher[strlen(publisher) - 1] = '\0';
    printf("page_count=");
    fgets(id, TYPE, stdin);
    id[strlen(id) - 1] = '\0';
    pages = atoi(id);
    //Se creeaza obiectul json cu datele citite anterior
    JSON_Value* root_value = json_value_init_object();
    JSON_Object* root_object = json_value_get_object(root_value);
    json_object_set_string(root_object, "title", title);
    json_object_set_string(root_object, "author", author);
    json_object_set_string(root_object, "genre", genre);
    json_object_set_number(root_object, "page_count", pages);
    json_object_set_string(root_object, "publisher", publisher);
    serialized_string = json_serialize_to_string_pretty(root_value);
    //Se verifica daca actualul utilizator are acces la biblioteca
    if (jwt == NULL) {
        printf("The user isn't authorized!\n");
        return;
    }
    //Se creeaza si se trimite mesajul
    strcpy(body_data[0], jwt);
    message = compute_post_request("34.118.48.238",
        "/api/v1/tema/library/books", "json", serialized_string,
            strlen(serialized_string), body_data, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    response = process_response(response);
    //Se afiseaza un mesaj in functie de tipul raspunsului
    if (strcmp(response, "400") == 0) {
        printf("Bad Request!\n");
    }
    else if (strcmp(response, "200") == 0) {
        printf("The book has been added!\n");
    }
    json_free_serialized_string(serialized_string);
    json_value_free(root_value);
    free(response);
    free(message);
}

//Functie pentru delogarea unui utilizator
void logout(int sockfd, char* cookie, char** body_data, char* jwt) {
    char *message, *response;
    //Se verifica daca exista un utilizator logat
    if (cookie != NULL) {
        strcpy(body_data[0], cookie);
        message = compute_get_request("34.118.48.238",
            "/api/v1/tema/auth/logout", NULL, body_data, 1);
    }
    else {
        printf("No user logged in!\n");
        return;
    }
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    response = process_response(response);
    //Se verifica daca delogarea a avut loc cu succes
    if (strcmp(response, "400") == 0) {
        printf("Bad Request!\n");
    }
    else if (strcmp(response, "200") == 0) {
        printf("You have successfully logged out!\n");
    }
    free(response);
    free(message);
}

//Functie pentru aflarea informatiilor despre o carte
void get_book(int sockfd, char* jwt, char** body_data) {
    char id[TYPE], book_way[VAR_LEN];
    char *message, *response;
    memset(book_way, 0, VAR_LEN);
    //Se citeste id-ul carti si se creeaza url-ul pentru cerere
    printf("id=");
    fgets(id, TYPE, stdin);
    id[strlen(id) - 1] = '\0';
    sprintf(book_way, "/api/v1/tema/library/books/%s", id);
    //Se verifica daca utilizatorul curent are acces la biblioteca
    if (jwt == NULL) {
        printf("The user isn't authorized!\n");
        return;
    }
    strcpy(body_data[0], jwt);
    //Se creeaza si se trimite mesajul
    message = compute_get_request("34.118.48.238", book_way, NULL,
        body_data, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    char* type = process_response(response);
    //Se verifica daca exista o carte cu id-ul cerut
    if (strcmp(type, "404") == 0) {
        printf("The book wasn't found!\n");
    }
    else if (strcmp(type, "200") == 0) {
        //Daca exista, se afiseaza datele despre aceasta
        response = get_books(response);
        printf("%s\n", response);
        free(response);
    }
    free(type);
    free(message);
}

//Functie pentru stergerea unei carti din biblioteca
void delete_book(int sockfd, char* jwt, char** body_data) {
    char book_way[VAR_LEN], id[TYPE], *message, *response;
    //Se citeste id-ul cartii si se verifica accesul la biblioteca
    printf("id=");
    fgets(id, TYPE, stdin);
    id[strlen(id) - 1] = '\0';
    memset(book_way, 0, VAR_LEN);
    sprintf(book_way, "/api/v1/tema/library/books/%s", id);
    if (jwt == NULL) {
        printf("The user isn't authorized!\n");
        return;
    }
    /*
    Se trimite mesajul de stergere si se verifica rezultatul, afisandu-se un
    mesaj corespunzator
    */
    strcpy(body_data[0], jwt);
    message = compute_delete_request("34.118.48.238", book_way, NULL,
        body_data, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    response = process_response(response);
    if (strcmp(response, "404") == 0) {
        printf("The book was not found!\n");
    }
    else if (strcmp(response, "200") == 0) {
        printf("The book has been deleted!\n");
    }
    free(message);
    free(response);
}

/*
Functie care creeaza un obiect json cu numele si parola unui utilizator si il
logheaza
*/
char* make_json_and_login(int sockfd, char* username, char* password) {
    JSON_Value* root_value = json_value_init_object();
    JSON_Object* root_object = json_value_get_object(root_value);
    json_object_set_string(root_object, "username", username);
    json_object_set_string(root_object, "password", password);
    //Se converteste json-ul in string si se creeaza mesajul
    char* serialized_string = json_serialize_to_string_pretty(root_value);
    char* message = compute_post_request("34.118.48.238",
        "/api/v1/tema/auth/login", "json", serialized_string,
            strlen(serialized_string), NULL, 0);
    send_to_server(sockfd, message);
    char* response = receive_from_server(sockfd);
    /*
    Se extrage cookie-ul din raspunsul server-ului si se verifica tipul
    raspunsului
    */
    char* cookie = extract_cookie(response);
    response = process_response(response);
    if (strcmp(response, "400") == 0) {
        printf("Credentials are not good!\n");
    }
    else if (strcmp(response, "200") == 0) {
        printf("Login successful!\n");
    }
    free(response);
    free(message);
    json_free_serialized_string(serialized_string);
    json_value_free(root_value);
    return cookie;
}

int main() {
    char command[VAR_LEN], username[VAR_LEN], password[VAR_LEN], *response;
    char *cookie = NULL, *jwt = NULL, *message;
    int loop = 0, sockfd;
    /*
    body_data se va utiliza pentru adaugarea in mesaj a cookie-urilor sau a
    tokenului jwt
    */
    char **body_data = calloc(BOOKS, sizeof(char *));
    for(int i = 0; i < BOOKS; i++) {
        body_data[i] = calloc(1500, sizeof(char));
    }
    while (!loop)
    {
        //Se deschide conexiunea catre server si se citeste comanda
        sockfd = open_connection("34.118.48.238", 8080, AF_INET,
            SOCK_STREAM, 0);
        fgets(command, VAR_LEN, stdin);
        command[strlen(command) - 1] = '\0';
        int command_number = console(command);
        //In functie de numarul ei, se executa anumite comenzi
        switch (command_number)
        {
            //Pentru 1, se va inregistra un nou utilizator
            case 1:
                register_user(sockfd);
                break;
            //Pentru 2, se va loga un utilizator deja existent
            case LOGIN:
                memset(username, 0, VAR_LEN);
                memset(password, 0, VAR_LEN);
                //Se citesc numele si parola utilizatorului
                printf("username=");
                fgets(username, VAR_LEN, stdin);
                username[strlen(username) - 1] = '\0';
                printf("password=");
                fgets(password, VAR_LEN, stdin);
                password[strlen(password) - 1] = '\0';
                //Se pune in cookie rezultatul functiei
                cookie = make_json_and_login(sockfd, username, password);
                break;
            //Pentru 3, utilizatorul curent va cere acces in biblioteca
            case ENTER_LIB:
                jwt = enter_library(sockfd, cookie, username, body_data);
                break;
            //Pentru 4, utilizatorul va cere titlurile cartilor
            case BOOKS:
                /*
                Daca token-ul jwt este null, utilizatorul nu are acces la carti
                */
                if (jwt == NULL) {
                    printf("The user isn't authorized!\n");
                    break;
                }
                strcpy(body_data[0], jwt);
                //Se trimite mesajul de tip get la server
                message = compute_get_request("34.118.48.238",
                    "/api/v1/tema/library/books", NULL, body_data, 0);
                send_to_server(sockfd, message);
                response = receive_from_server(sockfd);
                //Se extrag datele despre carti din raspuns si se afiseaza
                response = get_books(response);
                printf("%s\n", response);
                free(response);
                free(message);
                break;
            //Pentru 5, se cer date despre o anumita carte
            case BOOK:
                get_book(sockfd, jwt, body_data);
                break;
            //Pentru 6, se adauga o carte in biblioteca
            case ADD_BOOK:
                add_book(sockfd, jwt, body_data);
                break;
            //Pentru 7, se sterge o carte din biblioteca
            case DEL:
                delete_book(sockfd, jwt, body_data);
                break;
            /*
            Pentru 8, utilizatorul curent se va deloga, iar cookie si jwt se
            vor sterge
            */
            case LOGOUT:
                logout(sockfd, cookie, body_data, jwt);
                free(cookie);
                cookie = NULL;
                if (jwt != NULL) {
                    free(jwt);
                    jwt = NULL;
                }
                break;
            //9 reprezinta exit, iar loop se va seta pe 1 pentru a opri while-ul
            case EXIT_SV:
                loop = 1;
                break;
            //Orice alta comanda este necunoscuta
            default:
                printf("Unknown command\n");
                break;
            }
        }
    for (int i = 0; i < BOOKS; i++) {
        free(body_data[i]);
    }
    free(body_data);
    return 0;
}
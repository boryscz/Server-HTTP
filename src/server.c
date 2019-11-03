#include "server.h"

#define BUFF_SIZE 1024
#define DATA_LINES 9 //linie danych od { do }
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t endpoints_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t database_mutex = PTHREAD_MUTEX_INITIALIZER;
// pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
// pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
// pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//usluga GET HTTP/1.1
void get_method(int socket,char *request_method, char *request, char *request_data ){

    //deklaracja zmiennych
    //otwarcie plikow

    char socketCLI[12];
    memset(socketCLI, '0', 24);
    sprintf(socketCLI, "%d", socket);
    char responseQ[24];
    strcpy(responseQ, socketCLI);
    strcat(responseQ, "response.txt");

    FILE *response = fopen(responseQ, "a");
    fseek(response,0, SEEK_END);

    pthread_mutex_lock(&endpoints_mutex);
    FILE* file = fopen("endpoints_url.txt", "r");

    int istnieje = -1;
    //sprawdzenie endpointa
    while(1){
        char url[30];
        memset(url, 0, 30);
        fscanf(file, "%s\n", url);
        if(strcmp(url, request) == 0){
            istnieje = 1;
            break;
        }else if(strcmp(url, "EOF") == 0){
            istnieje = -1;
            break;
        }
    }
    fclose(file);
    pthread_mutex_unlock(&endpoints_mutex);

    char *line= NULL;
    ssize_t read;
    size_t len = 0;

    //zamiana numeru identyfikacji ksiazki na long
    char *end = NULL;
    long id = strtol(request_data, &end, 10);   

    while(1){
        //sprawdzenie czy dany request url istnieje w bazie
        if(istnieje == 1){
            //jesli id <= 0 to oznacza ze zwrcamy cala liste books.json
            if(id<=0){
                //naglowek HTTP/1.1
                pthread_mutex_lock(&database_mutex);
                FILE *f = fopen("books.json", "r");
                fprintf(response, "HTTP/1.1 200 OK\n");
                fprintf(response, "Content-type: application/json\n");
                fprintf(response, "\n");    
                fseek(f, 0, SEEK_END);
                long fsize = ftell(f);
                fseek(f, 0, SEEK_SET);

                char *string = malloc(fsize + 1);
                fread(string, 1, fsize, f);
                fclose(f);
                string[fsize] = 0;
                fputs(string,response);
                //zamkniecie deskryptorow plikow - trzeba pamietac
                fclose(response);
                pthread_mutex_unlock(&database_mutex);
                break;
            }else{
                int number;
                //czytanie books.json linia po lini w poszukiwaniu konkretnego "id"
                pthread_mutex_lock(&database_mutex);
                FILE *f = fopen("books.json", "r");
                while((read = getline(&line, &len, f))!= -1){
                    sscanf( line, "\t\t\"id\": %d,\n", &number);
                    if(number == id){ //znaleziono id - tworzenie HTTP response
                        fprintf(response, "HTTP/1.1 200 OK\n");
                        fprintf(response, "Content-type: application/json\n");
                        fprintf(response, "\n");
                        
                        fprintf(response,"{\n");
                        fputs(line, response);
                        if(response == NULL){
                            printf("Opening response.txt file error.");
                            exit(1);
                        }
                        //ten while bedzie iterowal po konkretnym elemencie json'a
                        while((read = getline(&line, &len, f))!= -1){
                            //jak wykryje koniec elementu to zapisze znako jego konca i wyjdzie z pentli
                            if(strcmp(line, "    }\n") == 0){
                                fprintf(response, "}");
                                break;
                                
                            }           
                            fputs(line, response);
                        }
                        //zamknie deskryptory plikow
                        fclose(response);
                        break;

                    }
                }
                fclose(f);
                pthread_mutex_unlock(&database_mutex);
                //przypadek gdy nie zostanie znaleziony identyfikator ksiazki
                if(read == -1){ 
                    fprintf(response, "HTTP/1.1 404 Not Found\n"); //powinno byc 204 No Content ale Postman tego nie obsługuje
                    fprintf(response, "Content-type: text/html\n\n");
                    fprintf(response, "<!DOCTYPE html><html><head><title>Not Found 404</title></head><div id=\"main\"><div class=\"fof\"><h1>Record does not exist.</h1></div></div></html>");
                    fclose(response);     
                }
                break;
            }
        }else if(istnieje == -1){
            //przypadek gdy nie znajdzie takiego url na serwerze
        
            fprintf(response, "HTTP/1.1 501 Not Implemented\nContent-type: text/html\n\n");
            fprintf(response, "<!DOCTYPE html><html><head><title>Not Implemented 501</title></head><div id=\"main\"><div class=\"fof\"><h1>URL is Not implemented.</h1></div></div></html>");
            fclose(response);
            break;
        
        }
    }
    //zapisanie response do pliku tekstowego
    FILE *readf = fopen(responseQ, "r");
    fseek(readf, 0, SEEK_END);
    long fsize = ftell(readf);
    fseek(readf, 0, SEEK_SET);

    char *string = malloc(fsize + 1);
    fread(string, 1, fsize, readf);
    fclose(readf);
    string[fsize] = 0;

    //wyslanie odpowiedzi z serwera do klienta
    if(write(socket, string, fsize) < 0){
        printf("Write content error.");
    }
    //usuniecie pliku response.txt
    remove(responseQ);
}

//usługa PUT HTTP/1.1
void put_method(int socket, char *request_method, char *request, char *request_data){
    //prepair request_data to write in db;
    //---------------------------------------
    char socketCLI[12];
    memset(socketCLI, '0', 24);
    sprintf(socketCLI, "%d", socket);
    char responseQ[24];
    strcpy(responseQ, socketCLI);
    strcat(responseQ, "response.txt");

    char requestCLI_data[29];
    strcpy(requestCLI_data, socketCLI);
    strcat(requestCLI_data, "request_data.txt");

    char requestCLI[24];
    strcpy(requestCLI, socketCLI);
    strcat(requestCLI, "request.txt");

    char tmp_buffer[BUFF_SIZE];
    strcpy(tmp_buffer, request_data);
    FILE *request_file = fopen(requestCLI, "a");
    fputs(tmp_buffer, request_file);
    fclose(request_file);
    FILE *request_read_file = fopen(requestCLI, "r");
    FILE *request_data_file = fopen(requestCLI_data, "a");
    char *line= NULL;
    ssize_t read;
    size_t len = 0;
    int flag = 0;
    int id = 0;
    while((read = getline(&line, &len, request_read_file)) != -1){
        sscanf( line, "\t\t\"id\": %d,\n", &id);
        if(strcmp(line, "{\n") == 0){
            flag = 1;
        }
        if(strcmp(line, "}") == 0){
            break;
        }
        if(flag){
            fputs(line, request_data_file);
        }
    }
    remove(requestCLI);
    
    fclose(request_data_file);
    fclose(request_read_file);
    //------------------------------------- 
    //otwarcie wymaganych plikow
    pthread_mutex_lock(&endpoints_mutex);
    FILE* file = fopen("endpoints_url.txt", "r");

    int istnieje = -1;
    //sprawdzenie endpointa
    while(1){
        char url[30];
        memset(url, 0, 30);
        fscanf(file, "%s\n", url);
        if(strcmp(url, request) == 0){
            istnieje = 1;
            break;
        }else if(strcmp(url, "EOF") == 0){
            istnieje = -1;
            break;
        }
    }
    fclose(file);
    pthread_mutex_unlock(&endpoints_mutex);

    int number;
    FILE *read_request = fopen(requestCLI_data, "r");
    FILE *response = fopen(responseQ, "a");
    while(1){
        //poszukiwanie url'a
        if(istnieje == 1){ //happy-path
            pthread_mutex_lock(&database_mutex);
            FILE *read_db = fopen("books.json", "r+");
            int line_number_start = 1;
            int line_number_end;
            while((read = getline(&line, &len, read_db)) != -1){ 
                //znalezienie linii ktore nalezy modyfikowac w bazie
                sscanf( line, "\t\t\"id\": %d,\n", &number);
                if(number == id){
                    line_number_start--;
                    line_number_end = line_number_start + DATA_LINES;
                    break;
                }
                line_number_start++;
                
                
            }
            fclose(read_db);
            pthread_mutex_unlock(&database_mutex);
            if(read == -1){// dodanie nowej ksiazki; nie istnieje ona wczesniej w bazie
                fprintf(response, "HTTP/1.1 201 Created\n");
                fprintf(response, "Content-type: application/json\n");
                fprintf(response, "\n");
                pthread_mutex_lock(&database_mutex);
                FILE *read_books_db = fopen("books.json", "r+");
                fseek(read_books_db, -3, SEEK_END);
                fprintf(read_books_db, "\n    ,");
                
                
                fseek(read_request, 0, SEEK_END);
                long fsize = ftell(read_request);
                fseek(read_request, 0, SEEK_SET);

                char *string = malloc(fsize + 1);
                fread(string, 1, fsize, read_request);
                fclose(read_request);
                string[fsize] = 0;

                fputs(string, read_books_db);
                fprintf(read_books_db, "\n]\n");

                fclose(read_books_db);
                fputs(string, response);
                fclose(response);
                pthread_mutex_unlock(&database_mutex);

            }else{//dokonanie modyfikacji obiekut ktory dostal podany w ciele zapytania PUT
                fprintf(response, "HTTP/1.1 200 OK\n");
                fprintf(response, "Content-type: application/json\n");
                fprintf(response, "\n");
                pthread_mutex_lock(&database_mutex);
                FILE * db = fopen("books.json", "r+");
                //plik tymczasowy posiada kopie bazy
                FILE * tmp = fopen("temp.json", "w");
                char buffer[BUFF_SIZE];
                rewind(db);
                int line_count = 1;
                while ((fgets(buffer, BUFF_SIZE, db)) != NULL){
                    if(line_number_start == 2){
                        if(line_count == 2){
                            fputs("    {\n", tmp);
                            line_count++;
                            continue;
                        }
                        if (line_count < line_number_start || line_count > line_number_end+1){
                            fputs(buffer, tmp);
                        }
                    }else{
                        if (line_count < line_number_start || line_count > line_number_end){
                            fputs(buffer, tmp);
                        }
                    }
                    line_count++;
                }
                fclose(tmp);
                fclose(db);
                remove("books.json");
                rename("temp.json", "books.json");
                pthread_mutex_unlock(&database_mutex);

                pthread_mutex_lock(&database_mutex);
                //zapisanie odpowiedzi do pliku tymczasowego reponse.txt
                FILE *read_books_db = fopen("books.json", "r+");
                fseek(read_books_db, -3, SEEK_END);
                fprintf(read_books_db, "\n    ,");
                
                
                fseek(read_request, 0, SEEK_END);
                long fsize = ftell(read_request);
                fseek(read_request, 0, SEEK_SET);

                char *string = malloc(fsize + 1);
                fread(string, 1, fsize, read_request);
                fclose(read_request);
                string[fsize] = 0;

                fputs(string, read_books_db);
                fprintf(read_books_db, "\n]\n");

                //zamkniecie plikow
                fclose(read_books_db);
                pthread_mutex_unlock(&database_mutex);
                fputs(string, response);
                fclose(response);
            }
            break;
        }else if(istnieje == -1){ //error-path
                
            fprintf(response, "HTTP/1.1 501 Not Implemented\nContent-type: text/html\n\n");
            fprintf(response, "<!DOCTYPE html><html><head><title>ERROR 501</title></head><div id=\"main\"><div class=\"fof\"><h1>URL is not implemented.</h1></div></div></html>");
            fclose(response);
            break;
        }
    }
    //zapisanie response do pliku tekstowego
    FILE *readf = fopen(responseQ, "r");
    fseek(readf, 0, SEEK_END);
    long fsize = ftell(readf);
    fseek(readf, 0, SEEK_SET);

    char *string = malloc(fsize + 1);
    fread(string, 1, fsize, readf);
    fclose(readf);
    string[fsize] = 0;

    //wyslanie odpowiedzi z serwera do klienta
    if(write(socket, string, fsize) < 0){
        printf("Write content error.");
    }

    //usuniecie i zamkniecie zbednych plikow
    remove(responseQ);
    remove(requestCLI_data);
}

//usługa POST HTTP/1.1
void post_method(int socket, char *request_method, char *request, char *request_data){
    //prepair request_data to write in db;
    //---------------------------------------
    char socketCLI[12];
    memset(socketCLI, '0', 24);
    sprintf(socketCLI, "%d", socket);
    char responseQ[24];
    strcpy(responseQ, socketCLI);
    strcat(responseQ, "response.txt");

    char requestCLI_data[29];
    strcpy(requestCLI_data, socketCLI);
    strcat(requestCLI_data, "request_data.txt");

    char requestCLI[24];
    strcpy(requestCLI, socketCLI);
    strcat(requestCLI, "request.txt");

    char tmp_buffer[BUFF_SIZE];
    strcpy(tmp_buffer, request_data);
    FILE *request_file = fopen(requestCLI, "a");
    fputs(tmp_buffer, request_file);
    fclose(request_file);
    FILE *request_read_file = fopen(requestCLI, "r");
    FILE *request_data_file = fopen(requestCLI_data, "a");
    char *line= NULL;
    ssize_t read;
    size_t len = 0;
    int flag = 0;
    int id = 0;
    while((read = getline(&line, &len, request_read_file)) != -1){
        sscanf( line, "\t\t\"id\": %d,\n", &id);
        if(strcmp(line, "{\n") == 0){
            flag = 1;
        }
        if(strcmp(line, "}") == 0){
            break;
        }
        if(flag){
            fputs(line, request_data_file);
        }
    }
    remove(requestCLI);
    
    fclose(request_data_file);
    fclose(request_read_file);
    //-------------------------------------

    //otwarcie wymaganych plikow
    pthread_mutex_lock(&endpoints_mutex);
    FILE* file = fopen("endpoints_url.txt", "r");

    int istnieje = -1;
    //sprawdzenie endpointa
    while(1){
        char url[30];
        memset(url, 0, 30);
        fscanf(file, "%s\n", url);
        if(strcmp(url, request) == 0){
            istnieje = 1;
            break;
        }else if(strcmp(url, "EOF") == 0){
            istnieje = -1;
            break;
        }
    }
    fclose(file);
    pthread_mutex_unlock(&endpoints_mutex);

    int number;
    FILE *read_request = fopen(requestCLI_data, "r");
    FILE *response = fopen(responseQ, "a");
    while(1){
        //poszukiwanie url'a
        if(istnieje == 1){ //happy-path
            pthread_mutex_lock(&database_mutex);
            FILE *read_db = fopen("books.json", "r+");
            while((read = getline(&line, &len, read_db)) != -1){ 
                sscanf( line, "\t\t\"id\": %d,\n", &number);
                if(number == id){
                    //dopisać error patha że taki rekord istnieje i nie można go dodać :)
                    fprintf(response, "HTTP/1.1 201 No Content\n");
                    fprintf(response, "Content-type: text/html\n\n");
                    fprintf(response, "<!DOCTYPE html><html><head><title>No Content 201</title></head><div id=\"main\"><div class=\"fof\"><h1>This record exists in data base.</h1></div></div></html>");
                    fclose(response);     
                    break;
                }
            }
            fclose(read_db);
            pthread_mutex_unlock(&database_mutex);
            if(read == -1){// dodanie nowej ksiazki; nie istnieje ona wczesniej w bazie
                fprintf(response, "HTTP/1.1 201 Created\n");
                fprintf(response, "Content-type: application/json\n");
                fprintf(response, "\n");
                pthread_mutex_lock(&database_mutex);
                FILE *read_books_db = fopen("books.json", "r+");
                fseek(read_books_db, -3, SEEK_END);
                fprintf(read_books_db, "\n    ,");
                
                
                fseek(read_request, 0, SEEK_END);
                long fsize = ftell(read_request);
                fseek(read_request, 0, SEEK_SET);

                char *string = malloc(fsize + 1);
                fread(string, 1, fsize, read_request);
                fclose(read_request);
                string[fsize] = 0;

                fputs(string, read_books_db);
                fprintf(read_books_db, "\n]\n");

                fclose(read_books_db);
                pthread_mutex_unlock(&database_mutex);
                // fprintf(response, string); //dodanie utworzonego pola do odpowiedzi
                fclose(response);
            }
            break;
        }else if(istnieje == -1){ //error-path
                
            fprintf(response, "HTTP/1.1 501 Not Implemented\nContent-type: text/html\n\n");
            fprintf(response, "<!DOCTYPE html><html><head><title>501 Not Implemented</title></head><div id=\"main\"><div class=\"fof\"><h1>URL is not implemented.</h1></div></div></html>");
            fclose(response);
            
            break;
        }
    }
    //zapisanie response do pliku tekstowego
    FILE *readf = fopen(responseQ, "r");
    fseek(readf, 0, SEEK_END);
    long fsize = ftell(readf);
    fseek(readf, 0, SEEK_SET);

    char *string = malloc(fsize + 1);
    fread(string, 1, fsize, readf);
    fclose(readf);
    string[fsize] = 0;

    //wyslanie odpowiedzi z serwera do klienta
    if(write(socket, string, fsize) < 0){
        printf("Write content error.");
    }

    //usuniecie i zamkniecie zbednych plikow
    remove(responseQ);
    remove(requestCLI_data);
}

//usluga HEAD HTTP/1.1
void head_method(int socket, char *request_method, char*request, char *request_data){
    //deklaracja zmiennych
    //otwarcie plikow
    char socketCLI[12];
    memset(socketCLI, '0', 24);
    sprintf(socketCLI, "%d", socket);
    char responseQ[24];
    strcpy(responseQ, socketCLI);
    strcat(responseQ, "response.txt");

    pthread_mutex_lock(&endpoints_mutex);
    FILE* file = fopen("endpoints_url.txt", "r");

    int istnieje = -1;
    //sprawdzenie endpointa
    while(1){
        char url[30];
        memset(url, 0, 30);
        fscanf(file, "%s\n", url);
        if(strcmp(url, request) == 0){
            istnieje = 1;
            break;
        }else if(strcmp(url, "EOF") == 0){
            istnieje = -1;
            break;
        }
    }
    fclose(file);
    pthread_mutex_unlock(&endpoints_mutex);

    FILE *response = fopen(responseQ, "a"); //plik odpowiedzi
    fseek(response,0, SEEK_END);

    char *line= NULL;
    ssize_t read;
    size_t len = 0;

    //konwersja id na typ long
    char *end = NULL;
    long id = strtol(request_data, &end, 10);  

    while(1){
        //sprawdzenie czy dany request url istnieje w bazie
        if(istnieje == 1){
            //jesli id <= 0 to oznacza ze zwrcamy cala liste books.json - brak id
            if(id<=0){
                //naglowek HTTP/1.1
                fprintf(response, "HTTP/1.1 200 OK\n");
                fprintf(response, "Content-type: application/json\n");
                fprintf(response, "\n");    
                
                fclose(response);   //zamkniecie pliku odpwiedzi
                break;
            }else{
                int number;
                //czytanie books.json linia po lini w poszukiwaniu konkretnego "id"
                pthread_mutex_lock(&database_mutex);
                FILE *f = fopen("books.json", "r");
                while((read = getline(&line, &len, f))!= -1){
                    sscanf( line, "\t\t\"id\": %d,\n", &number);
                    if(number == id){ //znaleziono id - tworzenie HTTP response
                        fprintf(response, "HTTP/1.1 200 OK\n");
                        fprintf(response, "Content-type: application/json\n");
                        fprintf(response, "\n");
                        //zamknie deskryptory plikow
                        fclose(f);
                        fclose(response);
                        break;

                    }
                }
                pthread_mutex_unlock(&database_mutex);
                //przypadek gdy nie zostanie znaleziony identyfikator ksiazki
                if(read == -1){ 
                    fprintf(response, "HTTP/1.1 404 Not Found\n"); //powinno byc 204 No Content ale Postman tego nie obsługuje
                    fprintf(response, "Content-type: text/html\n\n");
                    fclose(response);     
                }
                break;
            }
        }else if(istnieje == -1){
            //przypadek gdy nie znajdzie takiego url na serwerze
                fprintf(response, "HTTP/1.1 501 Not Implemented\nContent-type: text/html\n\n");
                fclose(response);
                break;
        }
    }
    //zapisanie response do pliku tekstowego
    FILE *readf = fopen(responseQ, "r");
    fseek(readf, 0, SEEK_END);
    long fsize = ftell(readf);
    fseek(readf, 0, SEEK_SET);

    char *string = malloc(fsize + 1);
    fread(string, 1, fsize, readf);
    fclose(readf);
    string[fsize] = 0;

    //wyslanie odpowiedzi z serwera do klienta
    if(write(socket, string, fsize) < 0){
        printf("Write content error.");
    }
    //usuniecie pliku response.txt
    remove(responseQ);
}

//usluga DELETE HTTP/1.1
void delete_method(int socket, char *request_method, char *request, char *request_data){
    //deklaracja zmiennych
    //otwarcie plikow
    char socketCLI[12];
    memset(socketCLI, '0', 24);
    sprintf(socketCLI, "%d", socket);
    char responseQ[24];
    strcpy(responseQ, socketCLI);
    strcat(responseQ, "response.txt");

    pthread_mutex_lock(&endpoints_mutex);
    FILE* file = fopen("endpoints_url.txt", "r");

    int istnieje = -1;
    //sprawdzenie endpointa
    while(1){
        char url[30];
        memset(url, 0, 30);
        fscanf(file, "%s\n", url);
        if(strcmp(url, request) == 0){
            istnieje = 1;
            break;
        }else if(strcmp(url, "EOF") == 0){
            istnieje = -1;
            break;
        }
    }
    fclose(file);
    pthread_mutex_unlock(&endpoints_mutex);

    FILE *response = fopen(responseQ, "a"); //plik odpowiedzi
    fseek(response,0, SEEK_END);

    char *line= NULL;
    ssize_t read;
    size_t len = 0;

    //konwersja id na typ long
    char *end = NULL;
    long id = strtol(request_data, &end, 10); 

     while(1){
        //sprawdzenie czy dany request url istnieje w bazie
        if(istnieje == 1){
            //jesli id <= 0 to oznacza ze zwrcamy cala liste books.json - brak id
            if(id<=0){
                //naglowek HTTP/1.1
                fprintf(response, "HTTP/1.1 403 Forbidden\n");
                fprintf(response, "Content-type: text/html\n");
                fprintf(response, "\n");    //linia przerwy oddziela dane od naglowka
                fprintf(response, "<!DOCTYPE html><html><head><title>Forbidden 403</title></head><div id=\"main\"><div class=\"fof\"><h1>URL does not exist.</h1></div></div></html>");
                
                fclose(response);   //zamkniecie pliku odpwiedzi
                break;
            }else{
                int number;
                int line_number_start = 1;
                int line_number_end;
                pthread_mutex_lock(&database_mutex);
                FILE *f = fopen("books.json", "r");
                while((read = getline(&line, &len, f)) != -1){ 
                    //znalezienie linii ktore nalezy modyfikowac w bazie
                    sscanf( line, "\t\t\"id\": %d,\n", &number);
                    if(number == id){
                        line_number_start--;
                        line_number_end = line_number_start + DATA_LINES;
                        break;
                    }
                    line_number_start++;
                    
                    
                }
                fclose(f);
                pthread_mutex_unlock(&database_mutex);
                
                //przypadek gdy nie zostanie znaleziony identyfikator ksiazki
                if(read == -1){ 
                    fprintf(response, "HTTP/1.1 201 No Content\n"); //powinno byc 204 No Content ale Postman tego nie obsługuje
                    fprintf(response, "Content-type: text/html\n\n");
                    fprintf(response, "<!DOCTYPE html><html><head><title>No Content 204</title></head><div id=\"main\"><div class=\"fof\"><h1>Book does not exist.</h1></div></div></html>");
                    fclose(response);     
                }else{
                    //zmaleziono obiekt
                    fprintf(response, "HTTP/1.1 200 OK\n");
                    fprintf(response, "Content-type: application/json\n");
                    fprintf(response, "\n");
                    pthread_mutex_lock(&database_mutex);
                    FILE * db = fopen("books.json", "r+");
                    //plik tymczasowy posiada kopie bazy
                    FILE * tmp = fopen("temp.json", "w");
                    char buffer[BUFF_SIZE];
                    rewind(db);                 //cofniecie na poczatek pliku
                    int line_count = 1;
                    while ((fgets(buffer, BUFF_SIZE, db)) != NULL){
                        if(line_number_start == 2){
                            if(line_count == 2){
                                fputs("    {\n", tmp);
                                line_count++;
                                continue;
                            }
                            if (line_count < line_number_start || line_count > line_number_end+1){
                                fputs(buffer, tmp);
                            }
                        }else{
                            if (line_count < line_number_start || line_count > line_number_end){
                                fputs(buffer, tmp);
                            }
                            
                        }
                        line_count++;
                    }
                    fclose(tmp);
                    fclose(db);
                
                    remove("books.json");
                    rename("temp.json", "books.json");
                    pthread_mutex_unlock(&database_mutex);

                    pthread_mutex_lock(&database_mutex);
                    FILE *ff = fopen("books.json","r");
                    fseek(ff, 0, SEEK_END);
                    long fsize = ftell(ff);
                    fseek(ff, 0, SEEK_SET);

                    char *string = malloc(fsize + 1);
                    fread(string, 1, fsize, ff);
                    fclose(ff);
                    pthread_mutex_unlock(&database_mutex);
                    string[fsize] = 0;
                    fputs(string, response);

                    fclose(response);
                }
                break;
            }
        }else if(istnieje == -1){
            //przypadek gdy nie znajdzie takiego url na serwerze
                fprintf(response, "HTTP/1.1 501 Not Implemented\nContent-type: text/html\n\n");
                fprintf(response, "<!DOCTYPE html><html><head><title>ERROR 501</title></head><div id=\"main\"><div class=\"fof\"><h1>URL is not implemented.</h1></div></div></html>");
                fclose(response);
                break;
        }
    }
    //zapisanie response do pliku tekstowego
    FILE *readf = fopen(responseQ, "r");
    fseek(readf, 0, SEEK_END);
    long fsize = ftell(readf);
    fseek(readf, 0, SEEK_SET);

    char *string = malloc(fsize + 1);
    fread(string, 1, fsize, readf);
    fclose(readf);
    string[fsize] = 0;

    //wyslanie odpowiedzi z serwera do klienta
    if(write(socket, string, fsize) < 0){
        printf("Write content error.");
    }

    //usuniecie i zamkniecie zbednych plikow
    remove(responseQ);
}

// return Internal Server Error (HTTP method is not implemented)
void not_implemented_method(int socket){
    char socketCLI[12];
    memset(socketCLI, '0', 24);
    sprintf(socketCLI, "%d", socket);
    char responseQ[24];
    strcpy(responseQ, socketCLI);
    strcat(responseQ, "response.txt");

    FILE *response = fopen(responseQ, "a");
    fprintf(response, "HTTP/1.1 500 Internal Server Error\n");
    fprintf(response, "Content-type: text/html\n");
    fprintf(response, "\n");    //linia przerwy oddziela dane od naglowka
    fprintf(response, "<!DOCTYPE html><html><head><title>Internal Server Error 500</title></head><div id=\"main\"><div class=\"fof\"><h1>SERVER ERROR!!!</h1></div></div></html>");
    fclose(response);
    FILE *rfile = fopen(responseQ, "r");
    fseek(rfile, 0, SEEK_END);
    long fsize = ftell(rfile);
    fseek(rfile, 0, SEEK_SET);

    char *string = malloc(fsize + 1);
    fread(string, 1, fsize, rfile);
    fclose(rfile);
    string[fsize] = 0;

    //wyslanie odpowiedzi z serwera do klienta
    if(write(socket, string, fsize) < 0){
        printf("Write content error.");
    }

    //usuniecie i zamkniecie zbednych plikow
    remove(responseQ);
}

// HTTP request parser
void build_request(int socket,char *request_path){
    char request_method[10];
    memset(request_method, 0, 10);
    char request_URL[11];
    memset(request_URL, 0, 11);
    char url_data[4];
    memset(url_data, 0, 4);
    char url[10];
    memset(url, 0, 10);
    url[0] = '/';
    
    sscanf(request_path,"%s %s\n", request_method, request_URL);
    int tmp = 0;
    int iterator = 0;
    for(int i = 1; i < 11 ; i++){
        if(request_URL[i] == '/'){
            tmp = i;
            i++;
        }
        if(tmp > 0){
            url_data[iterator] = request_URL[i];
            request_URL[i] = '\0';
            iterator++;
            continue;
        }

        url[i] = request_URL[i];
    }

    if(strcmp("GET", request_method) == 0){
        get_method(socket,request_method, url, url_data);
    }else if(strcmp("PUT", request_method) == 0){
        put_method(socket, request_method, url, request_path);       
    }else if(strcmp("POST", request_method) == 0){
        post_method(socket, request_method, url, request_path);
    }else if(strcmp("HEAD", request_method) == 0){
        head_method(socket, request_method, url, url_data);
    }else if(strcmp("DELETE", request_method) == 0){
        delete_method(socket, request_method, url, url_data);
    }else{
        not_implemented_method(socket);
    }
    
}

//thread structure
struct thread_data_t {
    int socket_descriptor;
    char buf[BUFF_SIZE];
};

//thread function
void *ThreadBehaviour(void *t_data) {
    pthread_detach(pthread_self());
    struct thread_data_t *th_data = (struct thread_data_t*) t_data;
    memset(th_data -> buf, 0, sizeof(th_data -> buf));
    
    int number_of_chars;
    char buffer_tmp[1];
    char read_request_buf[BUFF_SIZE];
    memset(read_request_buf, '\0', BUFF_SIZE);
    int flag = 0;
    int type = 0; //if 0 \r if 1 \n
    int iterator = 0;
    int number_of_rn = 0;
    while(1){
        number_of_chars = read(th_data -> socket_descriptor, buffer_tmp, 1);
        if(number_of_chars < 0){
            printf("Read error.\n");
            close(th_data->socket_descriptor);
            exit(-1);
        }
        if(iterator == 0 && buffer_tmp[0] == 'P'){
            flag = 1;
        }

        if(flag){
            if(number_of_rn == 2){
                if(buffer_tmp[0] == '}'){
                    read_request_buf[iterator] = buffer_tmp[0];
                    break;
                }
            }else{
                if(buffer_tmp[0] == '\r' && type == 0){
                    type = 1;
                }else if(buffer_tmp[0] == '\n' && type == 1){
                    number_of_rn++;
                    type = 0;
                }else{
                    number_of_rn = 0;
                }
            }

        }else{
            if(buffer_tmp[0] == '\r' && type == 0){
                type = 1;
            }else if(buffer_tmp[0] == '\n' && type == 1){
                number_of_rn++;
                type = 0;
            }else{
                number_of_rn = 0;
                type = 0;
            }
            if(number_of_rn == 2){
                read_request_buf[iterator] = buffer_tmp[0];
                break;
            }
        }
        //zapis do tablicy request;
        read_request_buf[iterator] = buffer_tmp[0];
        iterator++;

    }
    strcat(th_data->buf, read_request_buf);
    build_request(th_data -> socket_descriptor, th_data->buf);
    free(th_data);
    pthread_exit(NULL);   
}

//thread and connection
void handleConnection(int connection_socket) {
    int create_result = 0;
    struct thread_data_t *t_data = malloc(sizeof(struct thread_data_t));
    pthread_t thread1;

    t_data -> socket_descriptor = connection_socket;
    create_result = pthread_create(&thread1, NULL, ThreadBehaviour, (void *)t_data);
    if(create_result < 0){
        printf("Blad utworzenia watku");
        exit(-1);
    }
}


int main(){

    //-----------------------------
    //server configuration
    struct sockaddr_in server_addr;
    int serv_sock, cli_sock;

    memset(&server_addr, 0, sizeof(struct sockaddr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(serv_sock < 0){
        printf("Opening socket error.");
    }
    
    char reuse_addr_val = 1;
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (char*) &reuse_addr_val, sizeof(reuse_addr_val));

    if(bind(serv_sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0){
        printf("Binding socket error.");
    }

    if(listen(serv_sock, 5) < 0) {
        printf("Listening error");
    }

    while(1) {
        cli_sock = accept(serv_sock, NULL, NULL);
        if(cli_sock < 0){
            printf("Accept Client error.");
        }
        pthread_mutex_lock(&mutex);
        handleConnection(cli_sock);
        sleep(1);
        close(cli_sock);
        pthread_mutex_unlock(&mutex);
    }
    
    close(serv_sock);
    return 0;
}
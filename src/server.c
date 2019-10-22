#include "server.h"

#define BUFF_SIZE 1024
#define DATA_LINES 9 //linie danych od { do }

//usluga GET HTTP/1.1
void get_method(int socket,char *request_method, char *request, char *request_data ){

    //deklaracja zmiennych
    //otwarcie plikow
    FILE* file = fopen("endpoints_url.txt", "r");
    FILE *response = fopen("response.txt", "a");
    fseek(response,0, SEEK_END);

    FILE *f = fopen("books.json", "r");
    char *line= NULL;
    ssize_t read;
    size_t len = 0;

    //zamiana numeru identyfikacji ksiazki na long
    char *end = NULL;
    long id = strtol(request_data, &end, 10);   

    while(1){
        char url[30];
        memset(url, 0, 30);
        fscanf(file, "%s\n", url);
        //sprawdzenie czy dany request url istnieje w bazie
        if(strcmp(url,request) == 0){
            //jesli id <= 0 to oznacza ze zwrcamy cala liste books.json
            if(id<=0){
                //naglowek HTTP/1.1
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
                break;
            }else{
                int number;
                //czytanie books.json linia po lini w poszukiwaniu konkretnego "id"
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
                        fclose(f);
                        fclose(response);
                        break;

                    }
                }
                //przypadek gdy nie zostanie znaleziony identyfikator ksiazki
                if(read == -1){ 
                    fprintf(response, "HTTP/1.1 201 No Content\n"); //powinno byc 204 No Content ale Postman tego nie obsługuje
                    fprintf(response, "Content-type: text/html\n\n");
                    fprintf(response, "<!DOCTYPE html><html><head><title>No Content 204</title></head><div id=\"main\"><div class=\"fof\"><h1>Record does not exist.</h1></div></div></html>");
                    fclose(response);     
                    fclose(f);
                }
                break;
            }
        }else{
            //przypadek gdy nie znajdzie takiego url na serwerze
            if(strcmp(url,"EOF") == 0){
                
                fprintf(response, "HTTP/1.1 501 Not Implemented\nContent-type: text/html\n\n");
                fprintf(response, "<!DOCTYPE html><html><head><title>Not Implemented 501</title></head><div id=\"main\"><div class=\"fof\"><h1>URL is Not implemented.</h1></div></div></html>");
                fclose(response);
                fclose(f);
                
                break;
            }
        }
    }
    fclose(file);
    //zapisanie response do pliku tekstowego
    FILE *readf = fopen("response.txt", "r");
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
    remove("response.txt");
}

//usługa PUT HTTP/1.1
void put_method(int socket, char *request_method, char *request, char *request_data){
    //prepair request_data to write in db;
    //---------------------------------------
    char tmp_buffer[BUFF_SIZE];
    strcpy(tmp_buffer, request_data);
    FILE *request_file = fopen("request.txt", "a");
    fputs(tmp_buffer, request_file);
    fclose(request_file);
    FILE *request_read_file = fopen("request.txt", "r");
    FILE *request_data_file = fopen("request_data.txt", "a");
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
    remove("request.txt");
    
    fclose(request_data_file);
    fclose(request_read_file);
    //------------------------------------- 

    //otwarcie wymaganych plikow
    FILE* file = fopen("endpoints_url.txt", "r");
    int number;
    FILE *read_db = fopen("books.json", "r+");
    FILE *read_request = fopen("request_data.txt", "r");
    FILE *response = fopen("response.txt", "a");
    while(1){
        char url[30];
        //poszukiwanie url'a
        fscanf(file, "%s\n", url);
        if(strcmp(url,request) == 0){ //happy-path
            
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
            if(read == -1){// dodanie nowej ksiazki; nie istnieje ona wczesniej w bazie
                fprintf(response, "HTTP/1.1 201 Created\n");
                fprintf(response, "Content-type: application/json\n");
                fprintf(response, "\n");
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

            }else{//dokonanie modyfikacji obiekut ktory dostal podany w ciele zapytania PUT
                fprintf(response, "HTTP/1.1 200 OK\n");
                fprintf(response, "Content-type: application/json\n");
                fprintf(response, "\n");
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
                fputs(string, response);
                fclose(response);
            }
            break;
        }else{ //error-path
            if(strcmp(url,"EOF") == 0){
                
                fprintf(response, "HTTP/1.1 501 Not Implemented\nContent-type: text/html\n\n");
                fprintf(response, "<!DOCTYPE html><html><head><title>ERROR 501</title></head><div id=\"main\"><div class=\"fof\"><h1>URL is not implemented.</h1></div></div></html>");
                fclose(response);
                
                break;
            }
        }
    }
    
    //zapisanie response do pliku tekstowego
    FILE *readf = fopen("response.txt", "r");
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
    remove("response.txt");
    remove("request_data.txt");
    fclose(file);

}

//usługa POST HTTP/1.1
void post_method(int socket, char *request_method, char *request, char *request_data){
    //prepair request_data to write in db;
    //---------------------------------------
    char tmp_buffer[BUFF_SIZE];
    strcpy(tmp_buffer, request_data);
    FILE *request_file = fopen("request.txt", "a");
    fputs(tmp_buffer, request_file);
    fclose(request_file);
    FILE *request_read_file = fopen("request.txt", "r");
    FILE *request_data_file = fopen("request_data.txt", "a");
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
    remove("request.txt");
    
    fclose(request_data_file);
    fclose(request_read_file);
    //-------------------------------------

    //otwarcie wymaganych plikow
    FILE* file = fopen("endpoints_url.txt", "r");
    int number;
    FILE *read_db = fopen("books.json", "r+");
    FILE *read_request = fopen("request_data.txt", "r");
    FILE *response = fopen("response.txt", "a");
    while(1){
        char url[30];
        //poszukiwanie url'a
        fscanf(file, "%s\n", url);
        if(strcmp(url,request) == 0){ //happy-path
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
            if(read == -1){// dodanie nowej ksiazki; nie istnieje ona wczesniej w bazie
                fprintf(response, "HTTP/1.1 201 Created\n");
                fprintf(response, "Content-type: application/json\n");
                fprintf(response, "\n");
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
                // fprintf(response, string); //dodanie utworzonego pola do odpowiedzi
                fclose(response);
            }
            break;
        }else{ //error-path
            if(strcmp(url,"EOF") == 0){
                
                fprintf(response, "HTTP/1.1 501 Not Implemented\nContent-type: text/html\n\n");
                fprintf(response, "<!DOCTYPE html><html><head><title>501 Not Implemented</title></head><div id=\"main\"><div class=\"fof\"><h1>URL is not implemented.</h1></div></div></html>");
                fclose(response);
                
                break;
            }
        }
    }
    //zapisanie response do pliku tekstowego
    FILE *readf = fopen("response.txt", "r");
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
    remove("response.txt");
    remove("request_data.txt");
    fclose(file);
}

//usluga HEAD HTTP/1.1
void head_method(int socket, char *request_method, char*request, char *request_data){
    //deklaracja zmiennych
    //otwarcie plikow
    FILE* file = fopen("endpoints_url.txt", "r"); 
    FILE *response = fopen("response.txt", "a"); //plik odpowiedzi
    fseek(response,0, SEEK_END);

    FILE *f = fopen("books.json", "r");
    char *line= NULL;
    ssize_t read;
    size_t len = 0;

    //konwersja id na typ long
    char *end = NULL;
    long id = strtol(request_data, &end, 10);  

    while(1){
        char url[30];
        fscanf(file, "%s\n", url);
        //sprawdzenie czy dany request url istnieje w bazie
        if(strcmp(url,request) == 0){
            //jesli id <= 0 to oznacza ze zwrcamy cala liste books.json - brak id
            if(id<=0){
                //naglowek HTTP/1.1
                fprintf(response, "HTTP/1.1 200 OK\n");
                fprintf(response, "Content-type: application/json\n");
                fprintf(response, "\n");    
                
                fclose(f);          //zamkniecie pliku json
                fclose(response);   //zamkniecie pliku odpwiedzi
                break;
            }else{
                int number;
                //czytanie books.json linia po lini w poszukiwaniu konkretnego "id"
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
                //przypadek gdy nie zostanie znaleziony identyfikator ksiazki
                if(read == -1){ 
                    fprintf(response, "HTTP/1.1 201 No Content\n"); //powinno byc 204 No Content ale Postman tego nie obsługuje
                    fprintf(response, "Content-type: text/html\n\n");
                    fclose(response);     
                    fclose(f);
                }
                break;
            }
        }else{
            //przypadek gdy nie znajdzie takiego url na serwerze
            if(strcmp(url,"EOF") == 0){
                
                fprintf(response, "HTTP/1.1 501 Not Implemented\nContent-type: text/html\n\n");
                fclose(response);
                fclose(f);
                
                break;
            }
        }
    }
    fclose(file);
    //zapisanie response do pliku tekstowego
    FILE *readf = fopen("response.txt", "r");
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
    remove("response.txt");

    
}

//usluga DELETE HTTP/1.1
void delete_method(int socket, char *request_method, char *request, char *request_data){
    //deklaracja zmiennych
    //otwarcie plikow
    FILE* file = fopen("endpoints_url.txt", "r"); 
    FILE *response = fopen("response.txt", "a"); //plik odpowiedzi
    fseek(response,0, SEEK_END);

    FILE *f = fopen("books.json", "r");
    char *line= NULL;
    ssize_t read;
    size_t len = 0;

    //konwersja id na typ long
    char *end = NULL;
    long id = strtol(request_data, &end, 10); 

     while(1){
        char url[30];
        fscanf(file, "%s\n", url);
        //sprawdzenie czy dany request url istnieje w bazie
        if(strcmp(url,request) == 0){
            //jesli id <= 0 to oznacza ze zwrcamy cala liste books.json - brak id
            if(id<=0){
                //naglowek HTTP/1.1
                fprintf(response, "HTTP/1.1 403 Forbidden\n");
                fprintf(response, "Content-type: text/html\n");
                fprintf(response, "\n");    //linia przerwy oddziela dane od naglowka
                fprintf(response, "<!DOCTYPE html><html><head><title>Forbidden 403</title></head><div id=\"main\"><div class=\"fof\"><h1>URL does not exist.</h1></div></div></html>");
                
                fclose(f);          //zamkniecie pliku json
                fclose(response);   //zamkniecie pliku odpwiedzi
                break;
            }else{
                int number;
                int line_number_start = 1;
                int line_number_end;
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

                    FILE *ff = fopen("books.json","r");
                    fseek(ff, 0, SEEK_END);
                    long fsize = ftell(ff);
                    fseek(ff, 0, SEEK_SET);

                    char *string = malloc(fsize + 1);
                    fread(string, 1, fsize, ff);
                    fclose(ff);
                    string[fsize] = 0;
                    fputs(string, response);

                    fclose(response);
                }
                break;
            }
        }else{
            //przypadek gdy nie znajdzie takiego url na serwerze
            if(strcmp(url,"EOF") == 0){
                
                fprintf(response, "HTTP/1.1 501 Not Implemented\nContent-type: text/html\n\n");
                fprintf(response, "<!DOCTYPE html><html><head><title>ERROR 501</title></head><div id=\"main\"><div class=\"fof\"><h1>URL is not implemented.</h1></div></div></html>");
                fclose(response);
                fclose(f);
                
                break;
            }
        }
    }
    //zapisanie response do pliku tekstowego
    FILE *readf = fopen("response.txt", "r");
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
    remove("response.txt");
    remove("request_data.txt");
    fclose(file);

}

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
    }//TODO dorobic write informacyjny na nieobslugiwanych methodach.
    
}
int clients[3];
int ile = 0;
struct thread_data_t {
    int socket_descriptor;
    char buf[BUFF_SIZE];
};

void *ThreadBehaviour(void *t_data) {
    pthread_detach(pthread_self());
    struct thread_data_t *th_data = (struct thread_data_t*) t_data;
    memset(th_data -> buf, 0, BUFF_SIZE);
    
    if(read(th_data -> socket_descriptor, th_data -> buf, BUFF_SIZE) < 0){
        printf("Read error.");
    }
    build_request(th_data -> socket_descriptor, th_data->buf);

    free(t_data);
    pthread_exit(NULL);   
}

void handleConnection(int connection_socket) {
    int create_result = 0;
    struct thread_data_t *t_data = malloc(sizeof(struct thread_data_t));
    pthread_t thread1;
    t_data -> socket_descriptor = connection_socket;
    clients[ile++] = connection_socket;
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

    char buffer[BUFF_SIZE] ={'\0'};
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


    //clilen = sizeof(cli_addr);
    cli_sock = accept(serv_sock, NULL, NULL);
    if(cli_sock < 0){
        printf("Accept Client error.");
    }
    while(1) {
        
        handleConnection(cli_sock);
         
        //End of server configuration

        // printf("Zdechlem");
        
       
        //printf("Zdechlem");
        //build_request(cli_sock,buffer);
       //break;

    }
    

    malloc(sizeof(buffer) * 64);
    close(serv_sock);
    return 0;
}
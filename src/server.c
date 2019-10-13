#include "server.h"

#define BUFF_SIZE 1024

//request endpoint
//response generator
//zwrotka plików JSON

void get_method(int socket,char *request_method, char *request, char *request_data ){

    FILE* file = fopen("endpoints_url.txt", "r");

    // printf(request_method);
    // printf("\n");
    // printf(request);
    // printf("\n");
    // printf(request_data);
    // printf("\n");
    char *end = NULL;
    long id = strtol(request_data, &end, 10);   

    while(1){
        char url[30];
        fscanf(file, "%s\n", url);
        if(strcmp(url,request) == 0){
            if(id<=0){
                FILE *f = fopen("books.txt", "r");
                fseek(f, 0, SEEK_END);
                long fsize = ftell(f);
                fseek(f, 0, SEEK_SET);

                char *string = malloc(fsize + 1);
                fread(string, 1, fsize, f);
                fclose(f);
                string[fsize] = 0;

                write(socket, string, fsize);
                printf("HELLO.");
                break;
            }else{
                FILE *f = fopen("books.json", "r");
                char *line= NULL;
                ssize_t read;
                size_t len = 0;
                int number;
                while((read = getline(&line, &len, f))!= -1){
                    sscanf( line, "\t\t\"id\": %d,\n", &number);
                    if(number == id){
                        printf("ISTNIEJE TAKI ID w bazie: %d\n", number);
                        FILE *response = fopen("response-element.txt", "a");
                        fprintf(response,"{\n");
                        fseek(response,0, SEEK_END);
                        fprintf(response, line);
                        if(response == NULL){
                            printf("Opening response-element.txt file error.");
                            exit(1);
                        }
                        
                        while((read = getline(&line, &len, f))!= -1){
                            printf(line);          
                            if(strcmp(line, "    },\n") == 0){
                                fprintf(response, "}");
                                break;
                                
                            }           
                            fprintf(response, line);
                        }
                        fseek(response, 0, SEEK_END);
                        long response_size = ftell(response);
                        fseek(response, 0, SEEK_SET);

                        char *response_string = malloc(response_size + 1);
                        fread(response_string, 1, response_size, response);
                        fclose(response);
                        response_string[response_size] = 0;

                        write(socket, response_string, response_size);

                        break;
                        //ogarnac error this endpoint does not exist.!!!
                    }
                }
            }
        }else{
            FILE *f = fopen("eror404.html", "r");
            fseek(f, 0, SEEK_END);
            long fsize = ftell(f);
            fseek(f, 0, SEEK_SET);

            char *string = malloc(fsize + 1);
            fread(string, 1, fsize, f);
            fclose(f);
            string[fsize] = 0;
        
            if(strcmp(url,"EOF") == 0){
                if(write(socket, string, fsize) < 0){
                    printf("Write error404 page.");
                }
                printf("This endpoint does not exist.");
                break;
            }
        }
    }
}

void build_request(int socket,char *request_path){
    char request_method[10];
    char request_URL[11];
    char url_data[4] = {0};
    char url[10];
    url[0] = '/';

    // printf(request_path);

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
    }
}

int main(){

    //-----------------------------
    //server configuration
    struct sockaddr_in server_addr, cli_addr;
    socklen_t clilen;
    char buffer[BUFF_SIZE];
    int serv_sock, cli_sock;

    serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(serv_sock < 0){
        printf("Opening socket error.");
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if(bind(serv_sock, (struct sockaddr *) &server_addr, sizeof(server_addr))){
        printf("Binding socket error.");
    }

    listen(serv_sock, 5);
    clilen = sizeof(cli_addr);

    cli_sock = accept(serv_sock, (struct sockaddr *) &cli_addr, &clilen);
    if(cli_sock < 0){
        printf("Accept Client error.");
    }
    //End of server configuration


    if(read(cli_sock, buffer, sizeof(buffer)) < 0){
        printf("Read error.");
    }
    build_request(cli_sock,buffer);
    // char response[BUFF_SIZE] = "{{\"author\": \"Roberto Bolano\"}}";
    // if(write(cli_sock, response, BUFF_SIZE)  < 0){
    //     error("Write response error.");
    // }

    // __bzero(buffer, BUFF_SIZE);
    malloc(sizeof(buffer) * 64);

    return 0;
}
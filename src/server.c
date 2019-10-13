#include "server.h"

#define BUFF_SIZE 1024

//request endpoint
//response generator
//zwrotka plików JSON

void get_method(int socket,char *request_method, char *request, char *request_data ){

    FILE* file = fopen("endpoints_url.txt", "r");
    FILE *response = fopen("response.txt", "a");
    fseek(response,0, SEEK_END);

    char *end = NULL;
    long id = strtol(request_data, &end, 10);   

    while(1){
        char url[30];
        fscanf(file, "%s\n", url);
        if(strcmp(url,request) == 0){ // nie dziala poprawnie
            fprintf(response, "HTTP/1.1 200 OK\n");
            fprintf(response, "Content-type: application/json\n");
            fprintf(response, "\n");
            if(id<=0){
                FILE *f = fopen("books.json", "r");
                char *line= NULL;
                ssize_t read;
                size_t len = 0;
                
                while((read = getline(&line, &len, f))!= -1){
                    if(strcmp(line, "[\n") == 0){
                        fprintf(response,"{\n"); 
                    }else if(strcmp(line, "]\n") == 0){
                        fprintf(response, "}\n");  
                    }else{
                        fprintf(response, line);
                    }
                }

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
                        
                        fprintf(response,"{\n");
                        fprintf(response, line);
                        if(response == NULL){
                            printf("Opening response.txt file error.");
                            exit(1);
                        }
                        
                        while((read = getline(&line, &len, f))!= -1){

                            if(strcmp(line, "    },\n") == 0 || strcmp(line, "    }\n") == 0){
                                fprintf(response, "}");
                                break;
                                
                            }           
                            fprintf(response, line);
                        }
                        
                        fclose(response);
                        break;

                    }
                }
                if(read == -1){ //nie dziala poprawnie
                    fseek(response, 0,SEEK_SET);
                    fprintf(response, "HTTP/1.1 204 No Content\nContent-type: text/html\n\n");
                    fprintf(response, "<!DOCTYPE html><html><head><title>OK 204</title></head><div id=\"main\"><div class=\"fof\"><h1>Record does not exist.</h1></div></div></html>");
                    fclose(response);     
                }
                break;
            }
        }else{
            if(strcmp(url,"EOF") == 0){
                
                fprintf(response, "HTTP/1.1 404 ERROR\nContent-type: text/html\n\n");
                fprintf(response, "<!DOCTYPE html><html><head><title>ERROR 404</title></head><div id=\"main\"><div class=\"fof\"><h1>Page does not exist.</h1></div></div></html>");
                fclose(response);
                
                break;
            }
        }
    }
    FILE *f = fopen("response.txt", "r");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *string = malloc(fsize + 1);
    fread(string, 1, fsize, f);
    fclose(f);
    string[fsize] = 0;

    if(write(socket, string, fsize) < 0){
        printf("Write content error.");
    }
    // remove("response.txt");
}

void build_request(int socket,char *request_path){
    char request_method[10];
    char request_URL[11];
    char url_data[4] = {0};
    char url[10];
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

    malloc(sizeof(buffer) * 64);

    return 0;
}
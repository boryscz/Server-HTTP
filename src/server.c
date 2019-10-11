#include "server.h"

#define BUFF_SIZE 1024

char * build_request(char *request_path){
    char request_method[10];
    char request_URL[100];
    char http_version[20];

    printf(&request_path);
    // sscanf(request_path,"%s %s %s\n", request_method, request_URL, http_version);
    // printf(request_method);
    // if(request_method[0] == 'G'){
    //     printf("GET.");
    // }
    return request_method;

}

int main(){
    struct sockaddr_in server_addr, cli_addr;
    socklen_t clilen;
    char buffer[BUFF_SIZE];
    int serv_sock, cli_sock;

    serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(serv_sock < 0){
        error("Opening socket error.");
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if(bind(serv_sock, (struct sockaddr *) &server_addr, sizeof(server_addr))){
        error("Binding socket error.");
    }

    listen(serv_sock, 5);
    clilen = sizeof(cli_addr);

    cli_sock = accept(serv_sock, (struct sockaddr *) &cli_addr, &clilen);
    if(cli_sock < 0){
        error("Accept Client error.");
    }

    if(read(cli_sock, buffer, sizeof(buffer)) < 0){
        error("Read error.");
    }
    build_request(buffer);
    if(write(cli_sock, buffer, BUFF_SIZE)  < 0){
        error("Write response error.");
    }

    printf("JESTEM TU");
}
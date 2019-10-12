#include "server.h"

#define BUFF_SIZE 1024

//request endpoint
//response generator
//zwrotka plików JSON

void get_method(char *request_URL){

    char endpoints[100];

    FILE* file = fopen("endpoints_url.txt", "r");
    while(1){
        char url[30];
        fscanf(file, "%s\n", url);
        if(strcmp(url,request_URL) == 0){
            //TODO zwrotka z endpointa (JSON)
            printf("HELLO.");
            break;
        }else{
            // TODO blad 404
            if(strcmp(url,"EOF") == 0){
                printf("This endpoint does not exist.");
                break;
            }
        }
    }
}
void build_request(char *request_path){
    char request_method[10];
    char request_URL[100];
    char http_version[400];

    sscanf(request_path,"%s %s %s\n", request_method, request_URL, http_version);
    if(strcmp("GET", request_method) == 0){
        get_method(request_URL);
        printf(request_URL);
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
    //End of server configuration


    if(read(cli_sock, buffer, sizeof(buffer)) < 0){
        error("Read error.");
    }


    

    build_request(buffer);
    char response[BUFF_SIZE] = "{{\"author\": \"Roberto Bolano\"}}";
    if(write(cli_sock, response, BUFF_SIZE)  < 0){
        error("Write response error.");
    }

    __bzero(buffer, BUFF_SIZE);

    return 0;
}
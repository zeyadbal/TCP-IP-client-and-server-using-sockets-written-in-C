#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "../line_parser.h"
#include "../common.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#define max_chars_to_read 2048
#define MYPORT "2018"  // the port users will be connecting to
#define BACKLOG 10     // how many pending connections queue will hold

int exec(cmd_line* parsed_line);
int handle_conn(cmd_line* parsed_line);
int handle_disconn(cmd_line* parsed_line);
int handle_ls(cmd_line* parsed_line);
int handle_get(cmd_line* parsed_line);

client_state* state = NULL;
int sockfd = -1;

int main(){

    char line[max_chars_to_read];
    
    state = malloc(sizeof(client_state));
    state->server_addr = "nil";
    state->conn_state = IDLE;
    state->client_id = NULL;
    state->sock_fd = -1;
    
    while(1){
        
        printf("server:%s>\n", state->server_addr  );
        
        fgets(line, max_chars_to_read, stdin);
    
        if(strcmp(line,"quit\n")==0){
            _exit(-1);
        }
        if(strcmp(line,"\n")==0){
            continue;
        }
        
    cmd_line* parsed_line = parse_cmd_lines(line);
    
    if(parsed_line->arg_count==0)
        continue;
    if(exec(parsed_line) < 0)
        printf("failed\n");
    
    free_cmd_lines(parsed_line);
    
    }
    
    free(state);
    return 0;
} 

int exec(cmd_line* parsed_line){
    if(strcmp(parsed_line->arguments[0], "conn")==0)
        return handle_conn(parsed_line);
    if(strcmp(parsed_line->arguments[0], "ls")==0)
        return handle_ls(parsed_line);
    if(strcmp(parsed_line->arguments[0], "get")==0)
        return handle_get(parsed_line);
    if(strcmp(parsed_line->arguments[0], "bye")==0)
        return handle_disconn(parsed_line);
    return 0;
}

int handle_conn(cmd_line* parsed_line){
    if(parsed_line->arg_count!=2)
        return -1;
    
    struct addrinfo hints, *res;

    // !! don't forget your error checking for these calls !!

    // first, load up address structs with getaddrinfo():

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets (and not UDP)
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me (this tells getaddrinfo() to assign the address of my local host to the socket structures.)

    /**int getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);
     * 
     * The node parameter is the host name to connect to, or an IP address.
     * service, which can be a port number, like "80".
     * hints parameter points to a struct addrinfo that you've already filled out with relevant information.
     * res: and it gives you a pointer to a linked-list, res, of results.
     */
    getaddrinfo(NULL, MYPORT, &hints, &res); // it gives you a pointer to a linked-list, res, of results.

    // make a socket, bind it, and listen on it:

    /*But what are these arguments?
    They allow you to say what kind of socket you want (IPv4 or IPv6, stream or datagram, and TCP or UDP).
    socket() simply returns to you a socket descriptor that you can use in later system calls, or -1 on error,
    The global variable errno is set to the error's value.
    sockfd is the socket file descriptor returned by socket(). */
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);  
    
    if(sockfd == -1){
        perror("Client, socket syscall: ");
        exit(-1);
    }
    
    if(state->conn_state != IDLE)
        exit(-2);
    
    if( connect(sockfd, res->ai_addr, res->ai_addrlen) == -1 ){
        perror("Client, connect syscall: ");
        exit(-1);
    }
    
    // ready to communicate on socket descriptor new_fd!
    
    /*int send(int sockfd, const void *msg, int len, int flags); 
    sockfd is the socket descriptor you want to send data to (whether it's the one returned by socket() or the one you got with accept().)
    msg is a pointer to the data you want to send, and len is the length of that data in bytes. Just set flags to 0.
    send() returns the number of bytes actually sent out—this might be less than the number you told it to send!
    See, sometimes you tell it to send a whole gob of data and it just can't handle it. It'll fire off as much of the data as it can,
    and trust you to send the rest later. Remember, if the value returned by send() doesn't match the value in len,
    it's up to you to send the rest of the string. The good news is this:
    if the packet is small (less than 1K or so) it will probably manage to send the whole thing all in one go.
    Again, -1 is returned on error, and errno is set to the error number.*/
    char *msg = "hello";
    int len, bytes_sent;
    len = strlen(msg);
    if( (bytes_sent = send(sockfd, msg, len, 0)) == -1 ){
        perror("Client, send syscall: ");
        exit(-1);
    }
    
    state->conn_state = CONNECTING;/*The client will then set its conn_state to CONNECTING and wait for a response*/
    
    /*int recv(int sockfd, void *buf, int len, int flags);
    sockfd is the socket descriptor to read from, buf is the buffer to read the information into,
    len is the maximum length of the buffer, and flags can again be set to 0. (See the recv() man page for flag information.)
    recv() returns the number of bytes actually read into the buffer, or -1 on error (with errno set, accordingly.)
    Wait! recv() can return 0. This can mean only one thing: the remote side has closed the connection on you!
    A return value of 0 is recv()'s way of letting you know this has occurred.*/
    char buff[max_chars_to_read];
    memset(buff, 0, sizeof(buff));// reset the buffer
    int bytes_recieved;
    if( (bytes_recieved = recv(sockfd, buff, max_chars_to_read, 0)) == -1){
        perror("Client, recv syscall: ");
        exit(-1);
    }
    
    printf("Client: recieved [%s]\n", buff);
    
    char hello[max_chars_to_read];
    char client_id[max_chars_to_read];
    sscanf(buff, "%s, %c", hello, client_id );
    if(strcmp(hello, "hello")!=0)
        printf("Client: received illegal answer\n");
    state->client_id = client_id;
    state->conn_state = CONNECTED;
    state->sock_fd = sockfd;
    char* server_addr = malloc(strlen(parsed_line->arguments[1]));
    sprintf(server_addr, "%s", parsed_line->arguments[1]);
    state->server_addr = server_addr;
    
    return 0;
}

int handle_disconn(cmd_line* parsed_line){
    if(state->conn_state != CONNECTED)
        exit(-2);
    /*int send(int sockfd, const void *msg, int len, int flags); 
    sockfd is the socket descriptor you want to send data to (whether it's the one returned by socket() or the one you got with accept().)
    msg is a pointer to the data you want to send, and len is the length of that data in bytes. Just set flags to 0.
    send() returns the number of bytes actually sent out—this might be less than the number you told it to send!
    See, sometimes you tell it to send a whole gob of data and it just can't handle it. It'll fire off as much of the data as it can,
    and trust you to send the rest later. Remember, if the value returned by send() doesn't match the value in len,
    it's up to you to send the rest of the string. The good news is this:
    if the packet is small (less than 1K or so) it will probably manage to send the whole thing all in one go.
    Again, -1 is returned on error, and errno is set to the error number.*/
    char *msg = "bye";
    int len, bytes_sent;
    len = strlen(msg);
    if( (bytes_sent = send(sockfd, msg, len, 0)) == -1 ){
        perror("Client, send syscall: ");
        exit(-1);
    }
    
    if( close(sockfd) ){
        perror("Client, close syscall: ");
        exit(-1);
    }
    sockfd = -1;
    state->client_id = NULL;
    state->conn_state = IDLE;
    state->sock_fd = -1;
    state->server_addr = "nil";
    
    return 0;
}

int handle_ls(cmd_line* parsed_line){
    if(parsed_line->arg_count!=1)
        return -1;
    if(state->conn_state != CONNECTED)
        exit(-2);
    /*int send(int sockfd, const void *msg, int len, int flags); 
    sockfd is the socket descriptor you want to send data to (whether it's the one returned by socket() or the one you got with accept().)
    msg is a pointer to the data you want to send, and len is the length of that data in bytes. Just set flags to 0.
    send() returns the number of bytes actually sent out—this might be less than the number you told it to send!
    See, sometimes you tell it to send a whole gob of data and it just can't handle it. It'll fire off as much of the data as it can,
    and trust you to send the rest later. Remember, if the value returned by send() doesn't match the value in len,
    it's up to you to send the rest of the string. The good news is this:
    if the packet is small (less than 1K or so) it will probably manage to send the whole thing all in one go.
    Again, -1 is returned on error, and errno is set to the error number.*/
    char *msg = "ls";
    int len, bytes_sent;
    len = strlen(msg);
    if( (bytes_sent = send(sockfd, msg, len, 0)) == -1 ){
        perror("Client, send syscall: ");
        exit(-1);
    }
    
    /*int recv(int sockfd, void *buf, int len, int flags);
    sockfd is the socket descriptor to read from, buf is the buffer to read the information into,
    len is the maximum length of the buffer, and flags can again be set to 0. (See the recv() man page for flag information.)
    recv() returns the number of bytes actually read into the buffer, or -1 on error (with errno set, accordingly.)
    Wait! recv() can return 0. This can mean only one thing: the remote side has closed the connection on you!
    A return value of 0 is recv()'s way of letting you know this has occurred.*/
    char buff[LS_RESP_SIZE];
    memset(buff, 0, sizeof(buff));// reset the buffer
    int bytes_recieved;
    if( (bytes_recieved = recv(sockfd, buff, 2, 0)) == -1){
        perror("Client, recv syscall: ");
        exit(-1);
    }
    
    if( strcmp(buff, "ok") != 0 ){
        printf("Client: sent ls but didnt recieved ok\n");
        exit(-1);
    }

    //other wise, we recieved ok. now we wait for the list of files.
    memset(buff, 0, sizeof(buff));// reset the buffer
    if( (bytes_recieved = recv(sockfd, buff, max_chars_to_read, 0)) == -1){
        perror("Client, recv syscall: ");
        exit(-1);
    }
    
    printf("Client: revieved = \n%s\n", buff);
    
    return 0;
}

int handle_get(cmd_line* parsed_line){
    if(parsed_line->arg_count!=2)
        return -1;
    if(state->conn_state != CONNECTED)
        exit(-2);
    char* file_name = parsed_line->arguments[1];
    /*int send(int sockfd, const void *msg, int len, int flags); 
    sockfd is the socket descriptor you want to send data to (whether it's the one returned by socket() or the one you got with accept().)
    msg is a pointer to the data you want to send, and len is the length of that data in bytes. Just set flags to 0.
    send() returns the number of bytes actually sent out—this might be less than the number you told it to send!
    See, sometimes you tell it to send a whole gob of data and it just can't handle it. It'll fire off as much of the data as it can,
    and trust you to send the rest later. Remember, if the value returned by send() doesn't match the value in len,
    it's up to you to send the rest of the string. The good news is this:
    if the packet is small (less than 1K or so) it will probably manage to send the whole thing all in one go.
    Again, -1 is returned on error, and errno is set to the error number.*/
    char msg[4 + strlen(file_name)];
    sprintf(msg, "get %s", file_name);
    int len, bytes_sent;
    len = strlen(msg);
    if( (bytes_sent = send(sockfd, msg, len, 0)) == -1 ){
        perror("Server, send syscall: ");
        return 0;
    }
    /*int recv(int sockfd, void *buf, int len, int flags);
    sockfd is the socket descriptor to read from, buf is the buffer to read the information into,
    len is the maximum length of the buffer, and flags can again be set to 0. (See the recv() man page for flag information.)
    recv() returns the number of bytes actually read into the buffer, or -1 on error (with errno set, accordingly.)
    Wait! recv() can return 0. This can mean only one thing: the remote side has closed the connection on you!
    A return value of 0 is recv()'s way of letting you know this has occurred.*/
    char buff[max_chars_to_read];
    memset(buff, 0, sizeof(buff));// reset the buffer
    int bytes_recieved;
    if( (bytes_recieved = recv(sockfd, buff, 3, 0)) == -1){
        perror("Client, recv syscall: ");
        exit(-1);
    }
    
    char ok[2];
    sscanf(buff, "%s", ok);
    if(strcmp(ok, "ok")!=0){ // we recieved "nok file" and have already read "nok", we read the rest " file".
        if( (bytes_recieved = recv(sockfd, buff, 5, 0)) == -1){
            perror("Client, recv syscall: ");
            exit(-1);
        }
        printf("Cleint: recieved a nok\n");
        return 0;
    }
    //other wise, we recieved ok. now we wait for the size (4 bytes)
    memset(buff, 0, sizeof(buff));// reset the buffer
    if( (bytes_recieved = recv(sockfd, buff, 4, 0)) == -1){
        perror("Client, recv syscall: ");
        exit(-1);
    }
    int file_size;
    sscanf(buff, "%d", &file_size);
    
    state->conn_state = DOWNLOADING;
    
    char file_name_tmp[strlen(file_name) + 4];
    sprintf(file_name_tmp, "%s.tmp", file_name);
    FILE* fp = fopen(file_name_tmp ,"w+");
    
    int read_bytes = 0;
    
    char read_buffer[file_size];
    
    while(read_bytes < file_size){
        memset(read_buffer, 0, sizeof(read_buffer));// reset the buffer
        if( (bytes_recieved = recv(sockfd, read_buffer, file_size, 0)) == -1){
            perror("Client, recv syscall: ");
            exit(-1);
        }
        fwrite(read_buffer , 1 , bytes_recieved , fp );
        read_bytes += bytes_recieved;
    }
    
    fclose(fp);
    
    char* done = "done";
    memset(buff, 0, sizeof(buff));// reset the buffer
    len = strlen(done);
    if( (bytes_sent = send(sockfd, done, len, 0)) == -1 ){
        perror("Server, send syscall: ");
        return 0;
    }
    
    memset(buff, 0, sizeof(buff));// reset the buffer
    if( (bytes_recieved = recv(sockfd, buff, max_chars_to_read, 0)) == -1){
        perror("Client, recv syscall: ");
        exit(-1);
    }
    
    if(strcmp(buff, "ok")!=0){
        printf("Client, didnt recieved ok from server after sent [done]\n");
        return 0;
    }

    return 0;
    
}
























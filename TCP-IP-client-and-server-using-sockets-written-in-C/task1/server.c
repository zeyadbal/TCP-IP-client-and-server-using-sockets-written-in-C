#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../common.h"
#include "../line_parser.h"
#include <sys/sendfile.h>
#include <fcntl.h>
#define MYPORT "2018"  // the port users will be connecting to
#define BACKLOG 10     // how many pending connections queue will hold
#define max_chars_to_read 2048

client_state* state = NULL;

int handle_accept(int sockfd, struct sockaddr_storage their_addr, socklen_t addr_size);
char client_id = '0';

int main(){
    
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    struct addrinfo hints, *res;
    int sockfd, new_fd;

    // !! don't forget your error checking for these calls !!

    // first, load up address structs with getaddrinfo():

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    getaddrinfo(NULL, MYPORT, &hints, &res); // it gives you a pointer to a linked-list, res, of results.

    // make a socket, bind it, and listen on it:

    /*But what are these arguments?
    They allow you to say what kind of socket you want (IPv4 or IPv6, stream or datagram, and TCP or UDP).
    socket() simply returns to you a socket descriptor that you can use in later system calls, or -1 on error,
    The global variable errno is set to the error's value.
    sockfd is the socket file descriptor returned by socket(). 
    my_addr (here as res->ai_addr) is a pointer to a struct sockaddr that contains information about your address, namely, port and IP address.
    addrlen (here as res->ai_addrlen) is the length in bytes of that address.*/
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    
    if(sockfd == -1){
        perror("Server, socket syscall: ");
        return 0;
    }
        
    /*Once you have a socket, you might have to associate that socket with a port on your local machine.
    bind() also returns -1 on error and sets errno to the error's value.*/
    if( bind(sockfd, res->ai_addr, res->ai_addrlen) == -1){
        perror("Server, bind syscall: ");
        return 0;
    }
    
    /*listen() returns -1 and sets errno on error.*/
    if( listen(sockfd, BACKLOG) == -1 ){
        perror("Server, listen syscall: ");
        return 0;
    }

    // now accept an incoming connection:

    addr_size = sizeof their_addr;
    
    new_fd = handle_accept(sockfd, their_addr, addr_size);
        
    //printf("Server: new client connected\n");
    
    // ready to communicate on socket descriptor new_fd (handle new client).

    
    state = malloc(sizeof(client_state));

    char host_name[max_chars_to_read];
    gethostname(host_name, max_chars_to_read);

    state->server_addr = host_name;
    state->conn_state = IDLE;
    state->client_id = NULL;
    state->sock_fd = -1;
    
    while(1){
        
        /*int recv(int sockfd, void *buf, int len, int flags);
        sockfd is the socket descriptor to read from, buf is the buffer to read the information into,
        len is the maximum length of the buffer, and flags can again be set to 0. (See the recv() man page for flag information.)
        recv() returns the number of bytes actually read into the buffer, or -1 on error (with errno set, accordingly.)
        Wait! recv() can return 0. This can mean only one thing: the remote side has closed the connection on you!
        A return value of 0 is recv()'s way of letting you know this has occurred.*/
        
        char buff[max_chars_to_read];
        memset(buff, 0, sizeof(buff)); // reset the buffer
        int bytes_recieved;
        if( (bytes_recieved = recv(new_fd, buff, max_chars_to_read, 0)) == -1){
            perror("Server, recv syscall: ");
            return 0;
        }
        
        
        cmd_line* parsed_line = parse_cmd_lines(buff);
        
        if(strcmp(parsed_line->arguments[0], "hello")==0){
            state->conn_state = CONNECTED;
            state->client_id = client_id;
            state->sock_fd = new_fd;
            
            /*int send(int sockfd, const void *msg, int len, int flags); 
            sockfd is the socket descriptor you want to send data to (whether it's the one returned by socket() or the one you got with accept().)
            msg is a pointer to the data you want to send, and len is the length of that data in bytes. Just set flags to 0.
            send() returns the number of bytes actually sent out—this might be less than the number you told it to send!
            See, sometimes you tell it to send a whole gob of data and it just can't handle it. It'll fire off as much of the data as it can,
            and trust you to send the rest later. Remember, if the value returned by send() doesn't match the value in len,
            it's up to you to send the rest of the string. The good news is this:
            if the packet is small (less than 1K or so) it will probably manage to send the whole thing all in one go.
            Again, -1 is returned on error, and errno is set to the error number.*/

            char msg[7];
            sprintf(msg, "hello %c", client_id);
            int len, bytes_sent;
            len = strlen(msg);
            if( (bytes_sent = send(new_fd, msg, len, 0)) == -1 ){
                perror("Server, send syscall: ");
                free(parsed_line);
                return 0;
            }
            
            free(parsed_line);
            continue;
            
        }
        
        if(strcmp(parsed_line->arguments[0], "bye")==0){
            if( close(new_fd) == -1 )
                perror("Server, close syscall: ");
            printf("Client %c disconnected\n", client_id);
            client_id = client_id + 1;
            free(state);
            free(parsed_line);
            //accept
            new_fd = handle_accept(sockfd, their_addr, addr_size);
            
            state = malloc(sizeof(client_state));

            char host_name[max_chars_to_read];
            gethostname(host_name, max_chars_to_read);

            state->server_addr = host_name;
            state->conn_state = IDLE;
            state->client_id = NULL;
            state->sock_fd = -1;
            
            continue;
        }
        
        if(strcmp(parsed_line->arguments[0], "ls")==0){
            char* first_msg = "ok";
            int len, bytes_sent;
            len = strlen(first_msg);
            if( (bytes_sent = send(new_fd, first_msg, len, 0)) == -1 ){
                perror("Server, send syscall: ");
                free(parsed_line);
                return 0;
            }
            char* second_msg = NULL;
            char * listing = list_dir();
            if(listing != NULL)
                second_msg = (char*) listing;
            else
                second_msg = "nok filesystem";
            
            len = strlen(second_msg);
            if( (bytes_sent = send(new_fd, second_msg, len, 0)) == -1 ){
                perror("Server, send syscall: ");
                free(parsed_line);
                return 0;
            }
            free(parsed_line);
            continue;
        }
        
        if(strcmp(parsed_line->arguments[0], "get")==0){
            
            int size = -1;
            if( (parsed_line->arg_count != 2) || (size = file_size(parsed_line->arguments[1])) ==-1){
                char* first_msg = "nok file";
                int len, bytes_sent;
                len = strlen(first_msg);
                if( (bytes_sent = send(new_fd, first_msg, len, 0)) == -1 ){
                    perror("Server, send syscall: ");
                    free(parsed_line);
                    return 0;
                }
                continue;
            }
            
            char* file_name = parsed_line->arguments[1];
            
            char first_msg[3 + sizeof(int)];
            sprintf(first_msg, "ok %d", size);
            int len, bytes_sent;
            len = strlen(first_msg);
            if( (bytes_sent = send(new_fd, first_msg, len, 0)) == -1 ){
                perror("Server, send syscall: ");
                free(parsed_line);
                return 0;
            }
            state->conn_state = DOWNLOADING;
            
            off_t offset = 0;      // byte offset used by sendfile 

            int src = open(file_name, O_RDONLY);

            if( sendfile(new_fd, src, &offset, size) == -1 ){
                perror("Server, sendfile syscall: ");
                free(parsed_line);
                return 0;
            }
            
            close(src);
            
            char buff[max_chars_to_read];
            memset(buff, 0, sizeof(buff)); // reset the buffer
            int bytes_recieved;
            if( (bytes_recieved = recv(new_fd, buff, max_chars_to_read, 0)) == -1){
                perror("Server, recv syscall: ");
                free(parsed_line);
                return 0;
            }
            
            if(strcmp(buff, "done")!=0){
                char* nok_done = "nok done";
                len = strlen(nok_done);
                if( (bytes_sent = send(new_fd, nok_done, len, 0)) == -1 ){
                    perror("Server, send syscall: ");
                    free(parsed_line);
                    return 0;
                }
                free(parsed_line);
                return 0;
            }
            
            state->conn_state = CONNECTED;
            
            char* ok = "ok";
            len = strlen(ok);
            if( (bytes_sent = send(new_fd, ok, len, 0)) == -1 ){
                perror("Server, send syscall: ");
                free(parsed_line);
                return 0;
            }
            
            printf("Server: sent file <%s>\n", file_name);
        
            free(parsed_line);
            continue;
        }

    }
    
    if( close(sockfd) == -1 )
        perror("server, close syscall: ");
    
    return 0;

}

int handle_accept(int sockfd, struct sockaddr_storage their_addr, socklen_t addr_size){
    /* int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen); 
    sockfd is the listen()ing socket descriptor. Easy enough. addr will usually be a pointer to a local struct sockaddr_storage.
    This is where the information about the incoming connection will go (and with it you can determine which host is calling you from which port). 
    addrlen is a local integer variable that should be set to sizeof(struct sockaddr_storage) before its address is passed to accept().
    accept() will not put more than that many bytes into addr. If it puts fewer in, it'll change the value of addrlen to reflect that.
    Guess what? accept() returns -1 and sets errno if an error occurs. Betcha didn't figure that.*/
    int new_fd;
    if( (new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size)) == -1 ){
        perror("Server, accept syscall: ");
        return 0;
    }
    printf("Client %c connected\n", client_id);
    
    return new_fd;
}

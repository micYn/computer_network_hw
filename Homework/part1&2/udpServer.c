#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

#include "tcp_segment.h"
#define BUF_SIZE 524288
#define INITIAL_RTT 30      // Initial RTT in ms
#define MSS 1024            // Maximum Segment Size (1 KB)
#define THRESHOLD 65536     // Threshold (64 KB)

int clients[11]={0};
void handle_connection(int sockfd, struct sockaddr_in *client_addr, socklen_t addr_len, int *num_clients, tcp_segment_t syn_seg) {
	(*num_clients)++;
	tcp_segment_t syn_ack_seg, ack_seg;

	// Send SYN-ACK
	syn_ack_seg.src_port = syn_seg.dest_port;
	syn_ack_seg.dest_port = syn_seg.src_port;
	syn_ack_seg.seq_num = rand() % 10000 + 1;
	syn_ack_seg.ack_num = syn_seg.seq_num + 1;
	syn_ack_seg.flags = 0x12;  // SYN and ACK flags
	sendto(sockfd, &syn_ack_seg, sizeof(tcp_segment_t), MSG_CONFIRM, (const struct sockaddr *)client_addr, addr_len);
	printf("\tsend packet (%d) : SYN : ACK : SEQ=%u : ACK=%u\n",*num_clients,syn_ack_seg.seq_num,syn_ack_seg.ack_num);
	
	
	// Receive ACK
	recvfrom(sockfd, &ack_seg, sizeof(tcp_segment_t), MSG_WAITALL, (struct sockaddr *)client_addr, &addr_len);
	printf("receive packet (%d) : ACK : SEQ=%u : ACK=%u\n", *num_clients,ack_seg.seq_num,ack_seg.ack_num);
	printf("(%d) connected\n",*num_clients);
	printf("(%d) slow start mode\n", *num_clients);
}
char *handle_dns(char *hostname) {
    struct hostent *he;
    struct in_addr **addr_list;
    char *ip = malloc(INET_ADDRSTRLEN);

    if ((he = gethostbyname(hostname)) == NULL){
        herror("gethostbyname");
        return NULL;
    }

    addr_list = (struct in_addr **)he->h_addr_list;

    for (int i = 0; addr_list[i] != NULL; i++) {
        // Return the first IP address found
        strcpy(ip, inet_ntoa(*addr_list[i]));
        break;
    }

    return ip;
}
int power(int num1, int num2){
	if(num2==1)	return num1;
	return num1*power(num1,num2-1);
}
int handle_operation(char *operation) {
    int num1, num2, result;
    char operator;
	
    // Parse the operation
    if (sscanf(operation, "%d%c%d", &num1, &operator, &num2) != 3) {
    	if(sscanf(operation, "sqrt(%d)", &num1) == 1){
    		return sqrt(num1);
    	}
    	else{
    		printf("Invalid operation format\n");
        	return 0;
        }
    }
    // Perform the arithmetic operation
    switch (operator) {
        case '+':
            result = num1 + num2;
            break;
        case '-':
            result = num1 - num2;
            break;
        case '*':
            result = num1 * num2;
            break;
        case '/':
            if (num2 != 0) {
                result = num1 / num2;
            } else {
                printf("Division by zero error\n");
                return -1;
            }
            break;
        case '^':
        	result = pow(num1,num2);
		    break;
		default:
            printf("Invalid operator\n");
            return -1;
    }
    return result;
}
void printError(char *msg){
	perror(msg);
	exit(0);
}

int main(int argc, char *argv[]){
	srand(time(NULL));
	
	int my_socket, length, fromlen, n, num_clients = 0;
	struct sockaddr_in server, from;
	char buf[BUF_SIZE];
	
	if(argc!=2){printf("Usage : %s <port>\n", argv[0]);exit(0);}
	
	my_socket = socket(AF_INET,SOCK_DGRAM,0);
	if(my_socket==-1)	printError("Socket Creation Error");
	
	length = sizeof(server);
	bzero(&server, length);
	
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(atoi(argv[1]));
	
	if(bind(my_socket,(struct sockaddr *)&server, length)<0){
		printError("bind() Error");
	}
	fromlen = sizeof(struct sockaddr_in);
	printf("my port : %s\n\n", argv[1]);
	
	while(1){
			memset(buf, 0, BUF_SIZE);
			tcp_segment_t seg;
			// Receive from client (may be an PSH-ACK ,an operation request or a SYN for connection)
			n = recvfrom(my_socket, &seg, sizeof(tcp_segment_t), MSG_WAITALL, (struct sockaddr *)&from, &fromlen);
			if (n < 0)	printError("Error reading from socket");
            
            strcpy(buf, seg.data);
	        
			if(seg.flags==16){	// an PSH-ACK from client (client's ack for a task's answer)
				printf("receive packet (%d) : ACK : SEQ=%u : ACK=%u\n",num_clients, seg.seq_num, seg.ack_num);
			}
			//disconnection segment:
			else if(seg.flags==1){
				printf("(%d) disconnecting\n", num_clients);
				tcp_segment_t fin_ack_seg, ack_seg, ackack_seg;
				
				// Send FIN-ACK
				fin_ack_seg.src_port = seg.dest_port;
				fin_ack_seg.dest_port = seg.src_port;
				fin_ack_seg.seq_num = seg.ack_num;
				fin_ack_seg.ack_num = seg.seq_num+1;
				fin_ack_seg.flags = 0x11;  // FIN-ACK flag
				sendto(my_socket, &fin_ack_seg, sizeof(tcp_segment_t), MSG_CONFIRM, (struct sockaddr *)&from, fromlen);
				printf("\tsend packet : FIN : ACK : SEQ=%u : ACK=%u\n", fin_ack_seg.seq_num, fin_ack_seg.ack_num);

				// Receive ACK
				recvfrom(my_socket, &ack_seg, sizeof(tcp_segment_t), MSG_WAITALL, (struct sockaddr *)&from, &fromlen);
				if (ack_seg.flags & 0x10) {  // ACK pkt
					printf("receive packet : ACK : SEQ=%u : ACK=%u\n", ack_seg.seq_num, ack_seg.ack_num);
					int i;
					for(i=1;i<11;i++){
						if(clients[i] == ack_seg.src_port)	break;
					}
					clients[i] = 0;
					printf("(%d) disconnected\n",i);
					printf("(del client (%d))\n",i);
				}
			}
			/*else if (strncmp(buf, "FILE:", 5) == 0) {
				printf("Receive packet (%d) : PSH : SEQ=%u : ACK=%u\n",num_clients, seg.seq_num, seg.ack_num);
				printf("(%d) %s\n",num_clients,buf+5);
				
				// Handle file transmission request
				int fd, n;
				char filebuffer[MSS];
				tcp_segment_t answer_seg;
				// Open file for reading
				fd = open(buf+5, O_RDONLY);
				if (fd < 0) {
					printError("Error opening file for reading");
				}
				// Send file content to server
				while ((n = read(fd, filebuffer, MSS)) > 0) {
					memcpy(answer_seg.data, filebuffer, n);
					answer_seg.seq_num = ++(*seq_num);
					send_tcp_segment(sockfd, &segment, server_addr, addr_len);
				}

				snprintf(segment.data, MSS, "END");
				segment.seq_num = ++(*seq_num);
				send_tcp_segment(sockfd, &segment, server_addr, addr_len);

				// Close file
				close(fd);

			}*/
			else if(strncmp(buf, "DNS:", 4) == 0){
				printf("Receive packet (%d) : PSH : SEQ=%u : ACK=%u\n",num_clients, seg.seq_num, seg.ack_num);
				printf("(%d) %s\n",num_clients,buf+4);
				
				// Handle DNS request
				tcp_segment_t answer_seg;
				char *ip = handle_dns(buf + 4);
				strcpy(answer_seg.data, ip);
				printf("(%d) try file transmission\n(%d) file not exist\n(%d) try dns lookup\n(%d) success\n(%d) %s\n",num_clients,num_clients,num_clients,num_clients,num_clients, ip);
				if(ip != NULL){
					// Send DNS response back to client
					answer_seg.src_port = seg.dest_port;
					answer_seg.dest_port = seg.src_port;
					answer_seg.seq_num = seg.ack_num;
					answer_seg.ack_num = seg.seq_num + strlen(ip);
					
					n = sendto(my_socket, &answer_seg, sizeof(tcp_segment_t), MSG_CONFIRM, (struct sockaddr *)&from, fromlen);
					printf("\tsend packet (%d) : PSH : ACK : SEQ=%u : ACK=%u\n",num_clients, answer_seg.seq_num,answer_seg.ack_num);
					if(n < 0)	printError("Error writing to socket");
					free(ip);
				}
				else	printError("Failed to perform DNS lookup\n");
			}
			else if(strncmp(buf, "Arithmetic:", 11) == 0){
				printf("Receive packet (%d) : PSH : SEQ=%u : ACK=%u\n",num_clients, seg.seq_num, seg.ack_num);
				printf("(%d) %s\n",num_clients,seg.data+11);
				
				// Handle arithmetic operation
				int result = handle_operation(buf+11);
				char result_str[BUF_SIZE];
				// Convert the arithmetic result to string format
				snprintf(result_str, BUF_SIZE, "%d", result);
				// Send result back to client
				tcp_segment_t answer_seg;
				strcpy(answer_seg.data, result_str);
				printf("(%d) try file transmission\n(%d) file not exist\n(%d) try dns lookup\n(%d) fail\n(%d) try calculation\n(%d) %s\n",num_clients,num_clients,num_clients,num_clients,num_clients,num_clients, result_str);
				if(result != -1){					
					answer_seg.src_port = seg.dest_port;
					answer_seg.dest_port = seg.src_port;
					answer_seg.seq_num = seg.ack_num;
					answer_seg.ack_num = seg.seq_num + strlen(result_str);
					n = sendto(my_socket, &answer_seg, sizeof(tcp_segment_t), MSG_CONFIRM, (struct sockaddr *)&from, fromlen);
					printf("\tsend packet (%d) : PSH : ACK : SEQ=%u : ACK=%u\n",num_clients, answer_seg.seq_num,answer_seg.ack_num);
					if(n < 0){
							printError("Error writing to socket");
					}
				}
				else	printError("Failed to perform arithmetic calculation\n");
			}
			else{	//a SYN from client
				clients[num_clients+1] = seg.src_port;
	        	char ip_str[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &(from.sin_addr), ip_str, INET_ADDRSTRLEN);
				printf("receive packet %s : %u : SYN : SEQ=%u : ACK=%u\n", ip_str,seg.src_port,seg.seq_num,seg.ack_num);
				printf("(add client %s : %u : (%d))\n",ip_str,seg.src_port,num_clients+1);
				printf("(%d) connecting\n",num_clients+1);
				printf("(%d) cwnd=%d, rwnd=%d, threshold=%d\n",num_clients+1,MSS,BUF_SIZE,THRESHOLD);
				
				// the rest of three-way handshake:
				// send SYN-ACK & receive ACK:
				handle_connection(my_socket, &from, fromlen, &num_clients, seg);
			}
            
			
	}
	close(my_socket);
	
	return 0;
}

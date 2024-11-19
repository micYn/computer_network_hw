#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

#include "tcp_segment.h"

#define BUF_SIZE 524288
#define MSS 1024

void initiate_connection(int sockfd, struct sockaddr_in *server_addr, socklen_t addr_len, uint32_t *seq_num, uint32_t *ack_num, uint32_t *src_port) {
    tcp_segment_t syn_seg, syn_ack_seg, ack_seg;

    // Send SYN
    syn_seg.src_port = rand() % 65535 + 1024;
    syn_seg.dest_port = server_addr->sin_port;
    *seq_num = rand() % 10000 + 1;
    syn_seg.seq_num = *seq_num;
    syn_seg.ack_num = 0;
    syn_seg.flags = 0x02;  // SYN flag (SYN 1, ACK 0)
    sendto(sockfd, &syn_seg, sizeof(tcp_segment_t), MSG_CONFIRM, (struct sockaddr *)server_addr, addr_len);
    printf("\tsend packet : SYN : SEQ=%u : ACK=%u\n", syn_seg.seq_num, syn_seg.ack_num);

    // Receive SYN-ACK
    recvfrom(sockfd, &syn_ack_seg, sizeof(tcp_segment_t), MSG_WAITALL, (struct sockaddr *)server_addr, &addr_len);
    if (syn_ack_seg.flags & 0x12) {  // SYN and ACK flags-> SYN-ACK pkt
        printf("\treceive packet : SYN : ACK : SEQ=%u : ACK=%u\n", syn_ack_seg.seq_num, syn_ack_seg.ack_num);
        
        // Send ACK
        *ack_num = syn_ack_seg.seq_num + 1;
    	*seq_num = syn_ack_seg.ack_num;
    	*src_port = syn_ack_seg.dest_port;
        ack_seg.src_port = *src_port;
        ack_seg.dest_port = syn_ack_seg.src_port;
        ack_seg.seq_num = *seq_num;
        ack_seg.ack_num = *ack_num;
        ack_seg.flags = 0x10;  // ACK flag (SYN 0, ACK 1)
        sendto(sockfd, &ack_seg, sizeof(tcp_segment_t), MSG_CONFIRM, (struct sockaddr *)server_addr, addr_len);
        printf("\tsend packet : ACK : SEQ=%u : ACK=%u\n", ack_seg.seq_num,ack_seg.ack_num);
    }
}
void initiate_disconnection(int sockfd, struct sockaddr_in *server_addr, socklen_t addr_len, uint32_t *seq_num, uint32_t *ack_num, uint32_t *src_port) {
    tcp_segment_t fin_seg, fin_ack_seg, ack_seg;

    // Send FIN
    fin_seg.src_port = *src_port;
    fin_seg.dest_port = server_addr->sin_port;
    fin_seg.seq_num = *seq_num;
    fin_seg.ack_num = *ack_num;
    fin_seg.flags = 0x01;  // FIN flag
    sendto(sockfd, &fin_seg, sizeof(tcp_segment_t), MSG_CONFIRM, (struct sockaddr *)server_addr, addr_len);
    printf("\tsend packet : FIN : SEQ=%u : ACK=%u\n", fin_seg.seq_num, fin_seg.ack_num);

    // Receive FIN-ACK
    recvfrom(sockfd, &fin_ack_seg, sizeof(tcp_segment_t), MSG_WAITALL, (struct sockaddr *)server_addr, &addr_len);
    if (fin_ack_seg.flags & 0x11) {  // FIN and ACK flags-> FIN-ACK pkt
        printf("\treceive packet : FIN : ACK : SEQ=%u : ACK=%u\n", fin_ack_seg.seq_num, fin_ack_seg.ack_num);
        
        // Send ACK
        *ack_num = fin_ack_seg.seq_num + 1;
    	*seq_num = fin_ack_seg.ack_num;
    	*src_port = fin_ack_seg.dest_port;
        ack_seg.src_port = *src_port;
        ack_seg.dest_port = fin_ack_seg.src_port;
        ack_seg.seq_num = *seq_num;
        ack_seg.ack_num = *ack_num;
        ack_seg.flags = 0x10;  // ACK flag (SYN 0, ACK 1)
        sendto(sockfd, &ack_seg, sizeof(tcp_segment_t), MSG_CONFIRM, (struct sockaddr *)server_addr, addr_len);
        printf("\tsend packet : ACK : SEQ=%u : ACK=%u\n", ack_seg.seq_num,ack_seg.ack_num);
        
        close(sockfd);
    }
}
void send_tcp_segment(int sockfd, tcp_segment_t *segment, struct sockaddr_in *server_addr, socklen_t addr_len) {
    int n = sendto(sockfd, segment, sizeof(tcp_segment_t), 0, (struct sockaddr *)server_addr, addr_len);
    if (n < 0) {
        perror("Error sending TCP segment");
        exit(1);
    }
}
void printError(char *msg){
	perror(msg);
	exit(0);
}
int more_than_one_dot(char *str){
	int cnt = 0,i;
	for(i=0;i<strlen(str);i++){
		if(str[i] == '.')	cnt++;
	}
	if(cnt>1)	return 1;
	else return 0;
}
int main(int argc, char *argv[]){
	srand(time(NULL));
	
	int my_socket, length, n;
	struct sockaddr_in server;
	struct hostent *hp;
	char buf[BUF_SIZE];
	uint32_t seq_num, ack_num, src_port;
	
	// command line error detection
	if(argc<3){printf("Usage : %s <IP> <port>\n", argv[0]); exit(0);}
	
	// Socket Creation
	my_socket = socket(AF_INET,SOCK_DGRAM,0);
	if(my_socket==-1)	printError("Socket Creation Error");
	
	// sockaddr_in server settings 
	server.sin_family = AF_INET;
	hp = gethostbyname(argv[1]);
	if(hp==0)	printError("Unknown Host\n");
	bcopy((char *)hp->h_addr, (char *)&server.sin_addr,hp->h_length);
	server.sin_port = htons(atoi(argv[2]));
	length=sizeof(struct sockaddr_in);
	
	// target server's info (ip addr, port #)
	printf("server's ip address : ");
	for (char **addr_list = hp->h_addr_list; *addr_list != NULL; addr_list++) {
		struct in_addr addr;
		memcpy(&addr, *addr_list, sizeof(struct in_addr));
		printf("%s\n", inet_ntoa(addr));
	}
	printf("server's port       : %s\n\n",argv[2]);
	
	// initiating three-way handshake:
	printf("(connecting)\n");
	initiate_connection(my_socket, &server, length, &seq_num, &ack_num, &src_port);
	printf("(connected)\n");
	printf("(requested tasks)\n");
	
	// Send multiple requests to the server
	int i;
	for (i = 3; i < argc; i++) {
		printf("(task %d : %s)\n", i-3+1, argv[i]);
		memset(buf, 0, BUF_SIZE);
		int total_bytes_received = 0;
		
		// Check if it's a DNS request (contains more than one dot)
	    if (more_than_one_dot(argv[i])) {
	        // Prepend "DNS:" to indicate DNS request, and send the packet
	        tcp_segment_t segment;
		    snprintf(segment.data, MSS, "DNS:%s", argv[i]);
		    segment.src_port = src_port;
		    segment.dest_port = htons(atoi(argv[2])); 
		    segment.seq_num = seq_num;
		    segment.ack_num = ack_num;
		    segment.flags = 0x18;  // PSH and ACK flag
		    send_tcp_segment(my_socket, &segment, &server, sizeof(server));
		    printf("\tsend packet : PSH : SEQ=%u : ACK=%u\n",segment.seq_num,segment.ack_num);
		    
		    // Receive and print the responses from the server, also send an ACK to server for each response
			tcp_segment_t response_seg;
			while (1) {
				n = recvfrom(my_socket, &response_seg, sizeof(tcp_segment_t), MSG_WAITALL, (struct sockaddr *)&server, &length);
				if (n < 0)	printError("Error reading from socket");
				else{
					strcpy(buf,response_seg.data);
					printf("\treceive packet : PSH : ACK : SEQ=%u : ACK=%u\n",response_seg.seq_num,response_seg.ack_num);
					
					// Send ACK for the response
					seq_num = response_seg.ack_num;
					ack_num = response_seg.seq_num + strlen(response_seg.data);
					tcp_segment_t ack_seg;
					ack_seg.src_port = response_seg.dest_port;
					ack_seg.dest_port = response_seg.src_port;
					ack_seg.seq_num = seq_num;
					ack_seg.ack_num = ack_num;
					ack_seg.flags = 0x10;  // ACK flag
					send_tcp_segment(my_socket, &ack_seg, &server, sizeof(server));
					printf("\tsend packet : ACK : SEQ=%u : ACK=%u\n",ack_seg.seq_num,ack_seg.ack_num);
					break;
				}
			}
			printf("result:\n%s\n(task %d end)\n",buf,i+1-3);
	    }
		else {
			// Check if the argument is a file name (has exactly one dot)
			if (strchr(argv[i], '.') != NULL) {
				// Send file transmission request to server
				/*
				tcp_segment_t segment;
		        snprintf(segment.data, MSS, "FILE:%s", argv[i]);
		        segment.src_port = src_port; // Set appropriate values as needed
		        segment.dest_port = htons(atoi(argv[2])); // Set appropriate values as needed
		        segment.seq_num = seq_num;
		        segment.ack_num = ack_num;
		        segment.flags = 0x18;  // PSH and ACK flag
		        send_tcp_segment(my_socket, &segment, &server, sizeof(server));
		        printf("\tsend packet : PSH : SEQ=%u : ACK=%u\n",segment.seq_num,segment.ack_num);
		        
		        // Receive and print the response from the server, also send an ACK to server
				tcp_segment_t response_seg;
				while (1) {
					n = recvfrom(my_socket, &response_seg, sizeof(tcp_segment_t), MSG_WAITALL, (struct sockaddr *)&server, &length);
					if (n < 0)	printError("Error reading from socket");
					else{
						// Print the response from the server
						printf("\treceive packet : PSH : ACK : SEQ=%u : ACK=%u\n",response_seg.seq_num,response_seg.ack_num);
						//printf("Received response: %s\n", response_seg.data);
						int data_len = strnlen(segment.data, MSS);
						if (total_bytes_received + data_len >= BUF_SIZE)	printError("Buffer overflow, data is too large to fit in buffer\n");
						
						memcpy(buf + total_bytes_received, segment.data, data_len);
						total_bytes_received += data_len;
						
						// Send ACK for the response
						seq_num = response_seg.ack_num;
						ack_num = response_seg.seq_num + strlen(response_seg.data);
						tcp_segment_t ack_seg;
						ack_seg.src_port = response_seg.dest_port;
						ack_seg.dest_port = response_seg.src_port;
						ack_seg.seq_num = seq_num;
						ack_seg.ack_num = ack_num;
						ack_seg.flags = 0x10;  // ACK flag
						send_tcp_segment(my_socket, &ack_seg, &server, sizeof(server));
						printf("\tsend packet : ACK : SEQ=%u : ACK=%u\n",ack_seg.seq_num,ack_seg.ack_num);
						break;
					}
				}
				printf("result:\n%s\n(task %d end)\n",buf,i+1-3);
				*/
			}
		    else {
		        // Send arithmetic operation request
		        tcp_segment_t segment;
		        snprintf(segment.data, MSS, "Arithmetic:%s", argv[i]);
		        segment.src_port = src_port; // Set appropriate values as needed
		        segment.dest_port = htons(atoi(argv[2])); // Set appropriate values as needed
		        segment.seq_num = seq_num;
		        segment.ack_num = ack_num;
		        segment.flags = 0x18;  // PSH and ACK flag
		        send_tcp_segment(my_socket, &segment, &server, sizeof(server));
		        printf("\tsend packet : PSH : SEQ=%u : ACK=%u\n",segment.seq_num,segment.ack_num);
		        
		        // Receive and print the response from the server, also send an ACK to server
				tcp_segment_t response_seg;
				while (1) {
					n = recvfrom(my_socket, &response_seg, sizeof(tcp_segment_t), MSG_WAITALL, (struct sockaddr *)&server, &length);

					if (n < 0)	printError("Error reading from socket");
					else{
						strcpy(buf,response_seg.data);
						// Print the response from the server
						printf("\treceive packet : PSH : ACK : SEQ=%u : ACK=%u\n",response_seg.seq_num,response_seg.ack_num);
						
						// Send ACK for the response
						seq_num = response_seg.ack_num;
						ack_num = response_seg.seq_num + strlen(response_seg.data);
						tcp_segment_t ack_seg;
						ack_seg.src_port = response_seg.dest_port;
						ack_seg.dest_port = response_seg.src_port;
						ack_seg.seq_num = seq_num;
						ack_seg.ack_num = ack_num;
						ack_seg.flags = 0x10;  // ACK flag
						send_tcp_segment(my_socket, &ack_seg, &server, sizeof(server));
						printf("\tsend packet : ACK : SEQ=%u : ACK=%u\n",ack_seg.seq_num,ack_seg.ack_num);
						break;
					}
				}
				printf("result:\n%s\n(task %d end)\n",buf,i+1-3);
		    }
		    if (n < 0) {
		        printError("Error writing to socket");
		    }
		}
	}
	printf("(out of tasks)\n");
	printf("(disconnecting)\n");
	// initiate disconnection
	initiate_disconnection(my_socket, &server, length, &seq_num, &ack_num, &src_port);
	printf("(disconnected)\n");
	
	
	return 0;
}

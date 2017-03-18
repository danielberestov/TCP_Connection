
/*
 A simple client in the internet domain using TCP
 Usage: ./client hostname port (./client 192.168.0.151 10000)
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>      // define structures like hostent
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <time.h> 
#include <string.h>


void error(char *msg)
{
    perror(msg);
    exit(0);
}

struct header{
  short src_port;
  short dest_port;
  short flags;
  short wnd_size;
  short checksum;
  short size;
  int seq_num;
  int ack_num;
};

/*
flags=1  SYN
flags=2  ACK
flags=3  SYN+ACK
flags=4  FIN
flags=6  FIN+ACK
flags=7  EOF
*/


int rand_range(int min_n, int max_n)
{
    return rand() % (max_n - min_n + 1) + min_n;
}

int main(int argc, char *argv[])
{

    FILE *f = fopen("file.txt", "wb");
    if (f == NULL)
    {
        printf("Error opening file!\n");
        exit(1);
    }

    int sockfd; //Socket descriptor
    int portno, n;
    struct sockaddr_in serv_addr;
    socklen_t clilen = sizeof(serv_addr);
    struct hostent *server; //contains tons of information, including the server's IP address

    time_t t;
    srand((unsigned) time(&t));
    int starting_window = rand_range(0,30720);
    int window_size = 5120;
    int cur_window = starting_window;
    

    char data_array[5][1004];
    int window_flag[5]; 
    int seq_numrcv_array[5];

     for(int i = 0; i < sizeof(window_flag) / 4; i++) {
        window_flag[i] = -1;
        seq_numrcv_array[i] = -1;
        memset(data_array[i], 0, 1004);
     }


    char buffer[1024];
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_DGRAM, 0); //create a new socket
    if (sockfd < 0) 
        error("ERROR opening socket");
    
    server = gethostbyname(argv[1]); //takes a string like "www.yahoo.com", and returns a struct hostent which contains information, as IP address, address type, the length of the addresses...
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET; //initialize server's address
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    
    printf("%s\n",argv[3]);

    //establish connection



    struct header send_header, recv_header;
    struct header header1;
    header1.src_port = atoi(argv[2]);
    header1.dest_port = serv_addr.sin_port;
    header1.flags = 1;

    char* packet_read;
    char* packet_write;
    packet_read = malloc(20 * sizeof(char));
    packet_write = malloc((20 * sizeof(char)) + (strlen(argv[3]) * sizeof(char)));
    memcpy(packet_read, &header1, 20);


    if (sendto(sockfd, packet_read, 20, 0, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) < 0) {
        perror("sendto failed");
        return 0;
    }  

    printf("Sending packet SYN\n");
    cur_window = (cur_window + 20 ) % 30720;


    for(;;) {
        int recvlen = recvfrom(sockfd, packet_read, 20, 0, (struct sockaddr *)&serv_addr, &clilen);
          printf("received %d bytes\n", recvlen);

          memcpy(&recv_header, packet_read, 20);

          
          //ACK received
          if(recv_header.flags == 3) {

            printf("received SYN and ACK from server\n");


            send_header.src_port = atoi(argv[2]);
            send_header.dest_port = recv_header.src_port;
            send_header.flags = 2;
            cur_window = (cur_window + 1024) % 30720; 
            send_header.seq_num = cur_window;

            memcpy(packet_write, &send_header, 20);
            char*temp = malloc(strlen(argv[3]) * sizeof(char));
            memcpy(temp, argv[3], strlen(argv[3]));
            memcpy(packet_write + 20, temp, strlen(argv[3]));
            printf("sending ACK to server\n");


            sendto(sockfd, packet_write, 20 * sizeof(char) + strlen(argv[3]) * sizeof(char), 0, (struct sockaddr *)&serv_addr, clilen);
            printf("Sending packet %d %d \n", cur_window, window_size);

            seq_numrcv_array[0] = (recv_header.seq_num + 1024) % 30720;
            for(int i = 0; i < sizeof(window_flag) / 4 - 1; i++) {
                seq_numrcv_array[i+1] =  (seq_numrcv_array[i] + 1024) % 30720;
            }
            break;
          }
    } 
  
    free(packet_read);
    packet_read = malloc(1024 * sizeof(char));
    memset(packet_read, 0, 1024);

    for (;;) {

        int recvlen = recvfrom(sockfd, packet_read, 1024, 0, (struct sockaddr *)&serv_addr, &clilen);
        memcpy(&recv_header, packet_read, 20);

        memset(packet_write, 0, 1024);
        memset(&send_header, 0, 20);


        if (recvlen > 0) {
          printf("received packet %d\n", recv_header.seq_num);
        }

        if(recvlen > 0) {
        
            int recv_retransmit = 0;

            for(int i=0; i<5; i++){
                if(recv_header.seq_num == seq_numrcv_array[i])
                    recv_retransmit = 1;
            }

            if(recv_retransmit == 0) {

              send_header.src_port = atoi(argv[1]);
              send_header.dest_port = recv_header.src_port;
              send_header.flags = 2;
              send_header.ack_num = recv_header.seq_num;
              send_header.seq_num = cur_window;
              memcpy(packet_write, &send_header, 20);
              sendto(sockfd, packet_write, 1024, 0, (struct sockaddr *)&serv_addr, clilen);
            }

            else {

                send_header.src_port = atoi(argv[1]);
                send_header.dest_port = recv_header.src_port;
                send_header.flags = 2;

                if(recv_header.flags == 7)
                  send_header.flags = 7;

                send_header.ack_num = recv_header.seq_num;
                cur_window = (cur_window + 1024) % 30720; 
                send_header.seq_num = cur_window;

              int numRemoved = 0;
                for(int beginning_arr = 0; beginning_arr < sizeof(window_flag) / 4; beginning_arr++) {
                    if(seq_numrcv_array[beginning_arr] == recv_header.seq_num) {
                        
                        if(window_flag[beginning_arr] == -1)
                          window_flag[beginning_arr] = 1;

                        memcpy(data_array[beginning_arr], packet_read + 20, 1004);
                        break;

                    }
                  }
              

                if(window_flag[0] == 1) {

                    while(window_flag[0]==1){
                        fwrite(data_array[0], sizeof(char), recv_header.size, f);
\
                        for(int i = 0; i < (sizeof(window_flag) / 4) - 1; i++) {
                          window_flag[i] = window_flag[i+1];
                          seq_numrcv_array[i] = seq_numrcv_array[i+1];
                          memcpy(data_array[i], data_array[i+1], 1004);
                        }

                        numRemoved++;
                        if(numRemoved == 5)
                          break;
                  }

                    for(int i = 1; i <= numRemoved; i++) 
                      window_flag[sizeof(window_flag) / 4 - i] = -1;
                    

                     for(int i = 1; i < sizeof(window_flag) / 4-1; i++) 
                      seq_numrcv_array[i+1] =  (seq_numrcv_array[i] + 1024) % 30720;
               
                    
                }


              printf("Sending packet %d\n", cur_window);

              memcpy(packet_write, &send_header, 20);
              sendto(sockfd, packet_write, 1024, 0, (struct sockaddr *)&serv_addr, clilen);

              if(recv_header.flags == 7)
                  break;
            }

              memset(packet_read, 0, 1024);
          }
      }

    

    close(sockfd); //close socket
    fclose(f);
    return 0;
}

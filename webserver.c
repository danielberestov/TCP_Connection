
/* A simple server in the internet domain using TCP
   The port number is passed as an argument 
   This version runs forever, forking off a separate 
   process for each connection
*/
#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <sys/wait.h>	/* for the waitpid() system call */
#include <signal.h>	/* signal name macros, and the kill() prototype */
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/time.h> 



void error(char *msg)
{
    perror(msg);
    exit(1);
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

int rand_range(int min_n, int max_n)
{
    return rand() % (max_n - min_n + 1) + min_n;
}

int main(int argc, char *argv[])
{

     struct timeval tv;
     time_t t[5];

     int sockfd, portno, pid;
     struct sockaddr_in serv_addr, cli_addr;
     socklen_t clilen = sizeof(serv_addr);
     struct header send_header, recv_header;
     send_header.flags = -1;

     int window_array[5]; 
     int seq_num_array[5];
     char data_array[5][1004];
     for(int i = 0; i < sizeof(window_array) / 4; i++) {
        window_array[i] = -1;
        seq_num_array[i] = -1;
        memset(data_array[i], 0, 1004);

     }


     long tsec[5];
     long tusec[5];
     int starting_window = rand_range(0,30720);
     int window_size = 5120;
     int cur_window = starting_window;

     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }

     sockfd = socket(AF_INET, SOCK_DGRAM, 0);	//create socket
     int status = fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK);

     if (status == -1){
        perror("calling fcntl");
      }

     if (sockfd < 0) 
      error("ERROR opening socket");

     memset((char *) &serv_addr, 0, sizeof(serv_addr));	//reset memory
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     printf("Address is: %u\n", serv_addr.sin_addr.s_addr );
     serv_addr.sin_port = htons(portno);
     
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");

       int n;
     	 char* packet_read;
       packet_read = malloc(20 * sizeof(char));
       char* packet_write;
       packet_write = malloc(1024 * sizeof(char));


   		 listen(sockfd,5);  //5 simultaneous connection at most

       //receiving ffrom the client 
       packet_read = malloc(1024 * sizeof(char));
       for (;;) {

          int recvlen = recvfrom(sockfd, packet_read, 1024, 0, (struct sockaddr *)&serv_addr, &clilen);

          if(recvlen > 0) {
            printf("%c\n", *packet_read);
            printf("received %d bytes\n", recvlen);

            memcpy(&recv_header, packet_read, 20);
            printf("Header flag is: %d\n", recv_header.flags);
            
            //ACK received
            if(recv_header.flags == 1) {

              printf("received syn from client\n");

              send_header.src_port = atoi(argv[1]);
              send_header.dest_port = recv_header.src_port;
              send_header.flags = 3;
              send_header.size = recvlen;

              printf("Sending packet %d %d SYN\n", cur_window, window_size);
              cur_window = (cur_window + 20) % 30720;


              send_header.seq_num = cur_window;
              memcpy(packet_write, &send_header, 20);
              sendto(sockfd, packet_write, 20, 0, (struct sockaddr *)&serv_addr, clilen);


              memset(&send_header, 0, 20);
              memset(packet_read, 0, 1024);

            }

            //ack from client received and request for file received 
            if(recv_header.flags == 2) {
              printf("received ack from server along with data file, can start transmitting\n");
              break;
            }
          }

      }


      //taking care of new line in file name
      char temp[1024];
      memcpy(&temp, packet_read, 1024);
        for(int i = 0; i < sizeof(temp); i++) {

          if(temp[i] == 10)
            temp[i] = 0;
        }

      memcpy(packet_read, &temp, 1024);

       int curOffset = 0;
       if(access(packet_read+20, F_OK ) != -1 ) {

            FILE * filp = fopen(packet_read+20, "r"); 

            fseek(filp, 0L, SEEK_END);
            int sz = ftell(filp);
            rewind(filp);

            free(packet_read);
            packet_read = malloc(1024 * sizeof(char));
            memset(packet_read, 0, 1024);


            for(;;) {         


                //non blocking for udp socket

                int bytes_read;
                for(int i = 0; i < sizeof(window_array) / 4; i++) {

                  if(window_array[i] == -1) {

                    bytes_read = fread(packet_write+20, sizeof(char), 1004, filp);
                    curOffset += bytes_read;
                    fseek(filp, curOffset, bytes_read);

                    send_header.src_port = atoi(argv[1]);
                    send_header.dest_port = recv_header.src_port;
                    send_header.flags = 0;
                    send_header.size = bytes_read;
                    if(curOffset == sz) {
                     send_header.flags = 7;
                    }
                    cur_window = (cur_window + 1024) % 30720;
                    send_header.size = bytes_read;
                    send_header.seq_num = cur_window;
                    send_header.ack_num = recv_header.seq_num;

                    printf("Sending packet %d %d\n", cur_window, window_size);
                    memcpy(packet_write, &send_header, 20);
                    
                    sendto(sockfd, packet_write, 1024, 0, (struct sockaddr *)&serv_addr, clilen);


                    gettimeofday(&tv,NULL); 
                    tsec[i] = tv.tv_sec;
                    tusec[i] = tv.tv_usec;

                    window_array[i] = 0;
                    seq_num_array[i] = send_header.seq_num;
                    memcpy(data_array[i], packet_write + 20, 1004);

                  }

                  memset(packet_write, 0, 1024);

                }

                for (int i = 0; i < 5; i++)
                {
                    int milliSeconds;

                    gettimeofday(&tv,NULL); 
                    //milliSeconds=700;
                    
                    if(tsec[i]==tv.tv_sec)
                      milliSeconds = (tv.tv_usec - tusec[i]) % 1000;
                    
                    else
                      milliSeconds = ((1000000 - tusec[i]) % 1000) + (1000 * (tv.tv_sec - tsec[i] + 1)) + (tv.tv_usec % 1000);
                    
                    if(milliSeconds > 500 && seq_num_array[i] != -1) {

                      send_header.src_port = atoi(argv[1]);
                      send_header.dest_port = recv_header.src_port;
                      send_header.seq_num = seq_num_array[i];
                      memcpy(packet_write, &send_header, 20);
                      memcpy(packet_write + 20, data_array[i], 1004);
                      sendto(sockfd, packet_write, 1024, 0, (struct sockaddr *)&serv_addr, clilen);
                      
                      gettimeofday(&tv,NULL); 
                      tsec[i] = tv.tv_sec;
                      tusec[i] = tv.tv_usec;

                      memset(&send_header, 0 , 20);
                      printf("RETRANSMIT ");
                      printf("Didn't get packet: %d\n", seq_num_array[i]);
                    }
                }

                memset(packet_read, 0, 1024);

                int recvlen = recvfrom(sockfd, packet_read, 1024, 0, (struct sockaddr *)&serv_addr, &clilen);

                if(recvlen > 0) {
                  memcpy(&recv_header, packet_read, 20);

                  if(recv_header.flags == 7)
                    break;

                  printf("Receiving packet %d\n", recv_header.seq_num);

                  if(recv_header.flags == 4)
                    break;

                  int numRemoved = 0;
                  for(int beginning_arr = 0; beginning_arr < sizeof(window_array) / 4; beginning_arr++) {
                    if(window_array[beginning_arr] == 0) {
                      if(seq_num_array[beginning_arr] == recv_header.ack_num) {
                        window_array[beginning_arr] = 1;
                        if(beginning_arr == 0) {
                          while(window_array[0] == 1) {
                            for(int i = 0; i < (sizeof(window_array) / 4) - 1; i++) {
                              window_array[i] = window_array[i+1];
                              seq_num_array[i] = seq_num_array[i+1];
                              memcpy(data_array[i], data_array[i+1], 1004);
                              tsec[i] = tsec[i+1];
                              tusec[i] = tusec[i+1];
                            }
                            numRemoved++;
                          }

                          for(int i = 1; i <= numRemoved; i++) {
                            window_array[sizeof(window_array) / 4 - i] = -1;
                            seq_num_array[sizeof(window_array) / 4 - i] = -1;
                            memset(data_array[sizeof(window_array) / 4 - i], 0, 1004);
                          }
                        }
                      } 
                      break;
                    }
                  }
                  memset(packet_read, 0, 1024);
                }
  
            } 

          //else if file doesn't exist we need to know to do something 
          //}*/
        }

            
     printf("curOffset: %d\n", curOffset);
     close(sockfd);
         
     return 0; 
}


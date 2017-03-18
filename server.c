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

void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno, pid;
     socklen_t clilen;
     struct sockaddr_in serv_addr, cli_addr;

     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     sockfd = socket(AF_INET, SOCK_STREAM, 0);	//create socket
     if (sockfd < 0) 
        error("ERROR opening socket");
     memset((char *) &serv_addr, 0, sizeof(serv_addr));	//reset memory
     //fill in address info
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     
     listen(sockfd,5);	//5 simultaneous connection at most
     
     //accept connections
     newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
         
     if (newsockfd < 0) 
       error("ERROR on accept");
         
     int n;
   	 char buffer[256];
   			 
   	 memset(buffer, 0, 256);	//reset memory
      
 		 //read client's message
   	 n = read(newsockfd,buffer,255);
   	 if (n < 0) error("ERROR reading from socket");
   	 printf("Here is the message: %s\n",buffer);
   	 
     bool slashFound = false;
     bool dotFound = false;

     char fname[256];
     char type[256];
     int curIndex = 0;
     int typeIndex = 0;

     for(int i =0; i < strlen(buffer); i++) {
        if(slashFound && buffer[i] == ' ')
          break;

        if(slashFound) {

            //add name of file to fname 
            char strTemp;
            strTemp = buffer[i];
            fname[curIndex] = strTemp;
            curIndex++;

            if(dotFound) {  
              //add file type to type
              type[typeIndex] = strTemp;
              typeIndex++;

            }

        }

        //set flag to alert the parser that the next characters will be the name of the file 
        if(buffer[i] == '/')
            slashFound = true;

        //set flag to alert the parser that the next characters will hold the file type
        if(buffer[i] == '.')
            dotFound = true;
     }
     
     //define the type of header we'll be sending to the client preceding the contents of the file 
     char * header;
     if(strcmp("gif", type) == 0)
        header = "HTTP/1.1 200 OK\nContent-Type: image/gif\n\n";
     if(strcmp("jpg", type) == 0)
        header = "HTTP/1.1 200 OK\nContent-Type: image/jpg\n\n";
     if(strcmp("html", type) == 0)
        header = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n";
     if(strcmp("txt", type) == 0)
        header = "HTTP/1.1 200 OK\nContent-Type: text/txt\n\n";


     //file exists
     if( access(fname, F_OK ) != -1 ) {       

        //if file is a gif file 
        if(strcmp("gif", type) == 0) {

          //open as a binary file 
          FILE * filp = fopen(fname, "rb"); 

          //get size of file in bytes
          fseek(filp, 0L, SEEK_END);
          int sz = ftell(filp);
          rewind(filp);

          char buffer1[sz];

          int bytes_read = fread(buffer1, sizeof(char), sz, filp);

          //send header and contents of file 
          n = write(newsockfd, header, strlen(header));
          if (n < 0) error("ERROR writing to socket");
          n = write(newsockfd, buffer1, bytes_read);
          if (n < 0) error("ERROR writing to socket");
          fclose(filp);

        }

        else if(strcmp("jpg", type) == 0) {

          //open as a binary file 
          FILE * filp = fopen(fname, "rb"); 

          //get size of file in bytes
          fseek(filp, 0L, SEEK_END);
          int sz = ftell(filp);
          rewind(filp);

          char buffer1[sz]; 
          int bytes_read = fread(buffer1, sizeof(char), sz, filp);

          //send header and contents of file 
          n = write(newsockfd, header, strlen(header));
          if (n < 0) error("ERROR writing to socket");
          n = write(newsockfd, buffer1, bytes_read);
          if (n < 0) error("ERROR writing to socket");
          fclose(filp);

        }

        else {

          //open file as a text file 
          FILE * filp = fopen(fname, "r"); 

          //get size of file in bytes
          fseek(filp, 0L, SEEK_END);
          int sz = ftell(filp);
          rewind(filp);

          char buffer1[sz]; 
          int bytes_read = fread(buffer1, sizeof(char), sz, filp);

          //send header and contents of file 
          n = write(newsockfd, header, strlen(header));
          if (n < 0) error("ERROR writing to socket");
          n = write(newsockfd, buffer1, bytes_read);
          if (n < 0) error("ERROR writing to socket");
          fclose(filp);

        }
 
     }

     //file doesn't exists
     else {
      header = "HTTP/1.1 404 Not Found\n\n";
      n = write(newsockfd, header, strlen(header));
      if (n < 0) error("ERROR writing to socket");
     }


     close(newsockfd);
     close(sockfd);
         
     return 0; 
}


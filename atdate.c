/*
 * Based on the stream socket client demo 
 *contained in Beej's Guide to Network Programming
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>


#define BACKLOG 10
#define MAX_DATA_SIZE 100
#define CLIENT_UDP_MODE 2
#define CLIENT_TCP_MODE 1

int sockfd;
int acceptfd;
int debug;

void signal_handler(int signal) {
  printf("[CTRL+C] Closing socket.\n");
  close(sockfd);
  close(acceptfd);
  exit(1);
  
}

void sigpipe_handler(int signal) {
  int pid = getpid();
  printf("[pid=%d] Closed connection.\n", pid);
  close(sockfd);
  exit(1);
}

void sigchld_handler(int s)
{
  while(waitpid(-1, NULL, WNOHANG)>0);
}

void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void client( char *host, int op_mode, char * port)
{
  
  struct addrinfo hints, *servinfo, *p;
  int rv;
  char s[INET6_ADDRSTRLEN];
  char date[MAX_DATA_SIZE];
  size_t size=MAX_DATA_SIZE;
  struct tm *time;
  
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  if (op_mode==CLIENT_TCP_MODE){
    
    hints.ai_socktype = SOCK_STREAM;
  }else if (op_mode == CLIENT_UDP_MODE){
    hints.ai_socktype = SOCK_DGRAM;
  }
  signal(SIGINT, signal_handler);
  
  if(debug)
    printf("Looking for: %s %s \n", host,  port);
  
  /*
   * addrinfo -> DNS
   */
  if ((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    exit(-1);
  }
  
  
  if(debug)
    printf("Trying to create socket\n");
  
  // loop through all the results and connect to the first we can
    
    for(p = servinfo; p != NULL; p = p->ai_next) {
      //printf("entra en for\n");
      if ((sockfd = socket(p->ai_family, p->ai_socktype,
	p->ai_protocol)) == -1) {
	perror("client: socket");
      continue;
	}
	
	//f(op_mode==CLIENT_TCP_MODE){
	  if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
	    close(sockfd);
	    perror("client: connect");
	continue;
	  }
	  //}
	  break;
    }
    
    if (p == NULL) {
      fprintf(stderr, "client: failed to connect\n");
      exit(-1);
    }
    
    if(debug)
      printf("I've got my socket\n");
    
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
	      s, sizeof(s));
    
    freeaddrinfo(servinfo); // all done with this structure
    
    
    if(op_mode == CLIENT_TCP_MODE){
      time_t t;       
      int     n;           
      int     outchars=MAX_DATA_SIZE;
      
      if(debug)
	printf("Waits for packet\n");
      
      while(1)//reception loooooop
        {
	  n = recv(sockfd, &t, outchars, 0);
	  
	  if (n < 0)
	    fprintf(stderr, "***Socket read failed: %s\n", strerror(errno));
	  t=ntohl(t);

	  //if(n==4)
	  //{
	    if(debug)
	      printf("Received bytes: %d\n", n);
	    
	    time=localtime(&t);
	    t= t - 2208988800u;
	    time=localtime(&t);
            strftime ( date, size , "%a %b %d  %H:%M:%S %Z %Y\n", time);

	    printf("%s\n", date);
	    
	 // }
	  sleep(1);
	  
	}
    }
    
    if(op_mode == CLIENT_UDP_MODE){
      
      struct addrinfo * server=p; 
      /* 
       * addrinfo struct -> socket descriptor
       * all we need: server struct and sockfd
       */
      
      time_t buf;  
      int n;
      int read_out;
      
      // if((n= sendto(sockfd, &buf, 0 , 0, server->ai_addr, server->ai_addrlen))==-1)
      if((n= send(sockfd, &buf, 0 , 0))==-1)
	
      {
	printf("Error when sending to the server\n");
	exit(-1);
      }
      
      if(debug)
	printf("Packet sent to %s \n", s);
      
      read_out=4;
      
      n = recvfrom(sockfd, &buf, read_out, 0, server->ai_addr, &server->ai_addrlen  );
      if (n < 0)
	fprintf(stderr, "****Socket read failed: %s\n", strerror(errno));
      
      if(debug)
	printf("%d bytes received. Date: %d\n", n, (int)buf);
      
      buf= ntohl(buf);
      time=localtime(&buf);
      buf= buf - 2208988800u;
      time=localtime(&buf);
            strftime ( date, size , "%a %b %d  %H:%M:%S %Z %Y\n", time);
      printf("%s\n", date);
	
	close(sockfd);
    }
    
    
    exit(0);
}


void server(char* port)
{
  printf("Starting server\n");
  int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr; // connector's address information
  socklen_t sin_size;
  struct sigaction sa;
  int yes=1;
  char s[INET6_ADDRSTRLEN];
  int rv;
  
  memset(&hints, 0, sizeof hints); //copy attr for future address struct
  hints.ai_family = AF_UNSPEC; // any add family
  hints.ai_socktype = SOCK_STREAM; //TCP
  hints.ai_flags = AI_PASSIVE; // use my IP
  
  if(debug)
    printf("Looking for %s \n", port);
  
  if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) 
  {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    exit(-1);
  }
  
  if(debug)
    printf("Trying to create socket\n");
  
  //Look for all possible addresses and bind
  for(p = servinfo; p != NULL; p = p->ai_next) 
  {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
      p->ai_protocol)) == -1) {
      perror("server: socket");
    continue;
      }
      
      if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
	sizeof(int)) == -1) {
	perror("setsockopt");
      exit(1);
	}
	
	if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
	  close(sockfd);
	  perror("server: bind");
	  continue;
	}
	
	break;
  }
  
  if (p == NULL)  {
    fprintf(stderr, "server: failed to bind\n");
    exit(-1);   
  }
  
  freeaddrinfo(servinfo); 
  
  if (listen(sockfd, BACKLOG) == -1) {
    perror("listen");
    exit(1);
  }
  
  sa.sa_handler = sigchld_handler; // reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }
  
  while(1) { 
    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
      perror("accept");
      continue;
    }
    
    inet_ntop(their_addr.ss_family,
	      get_in_addr((struct sockaddr *)&their_addr),
	      s, sizeof s);
    if(debug)   
      printf("New connection: %s, socket: %d\n", s, new_fd);
    
    printf("New connection  %s!\n", s);
    
    if (!fork()) 
    {
      close(sockfd); 
      time_t buff;
      size_t size=sizeof(time_t);
      if(debug)
	printf("Time is: %d\n", (int)buff);
      
      while(1)
      {
	/*
	 * Get time, transform to network format
	 * and send through socket new_fd
	 */
	long int t= time(NULL);
	time_t ti;
	time(&ti);
	buff=ti + 2208988800u;
	buff=htonl(buff);
	if(debug)
	  printf("Send packet to %s, sockfd: %d\n", s, new_fd);
	if (send(new_fd, &buff, size, 0) ==-1)
	{
	  fprintf(stderr, "Error: send %s\n", strerror(errno));
	  exit(1);
	}
	else
	{
	  if(debug)
	    printf("Packet has been sent!\n");
	}
	
      sleep(1);
	
      }
      
      close(new_fd);
    }
    close(new_fd);
  }
}


int main(int argc, char *argv[]) {
  
  if (argc == 1) {
    printf("Help: %s [-h serverhost] [-p port] [-m cu | ct | s ] [-d]\n", argv[0]);
    return -1;
  }
  signal(SIGPIPE, sigpipe_handler);
  signal(SIGINT, signal_handler);
  
  int param_c;
  char* serverhost_str;
  char* port_str;
  char* mode_str;
  //char* port;
    port_str = strdup("37");

  for (param_c = 1; param_c < argc; param_c++) {
    if (strcmp("-d", argv[param_c]) == 0) {
      debug = 1;
    } else {
      if (param_c + 2 > argc) {
	printf("ERROR: invalid parameters.\n");
	return -1;
      }
      if (strcmp("-h", argv[param_c]) == 0) {
	serverhost_str = strdup(argv[param_c + 1]);
      } else if (strcmp("-m", argv[param_c]) == 0) {
	mode_str = strdup(argv[param_c + 1]);
      } else if (strcmp("-p", argv[param_c]) == 0) {
	port_str = strdup(argv[param_c + 1]);
	//port = atoi(port_str);
      } else {
	printf("ERROR: Parameter %s not recognized.\n", argv[param_c]);
	return -1;
      }
      
      param_c++;
    }
    
  }
  if (strcmp(mode_str, "cu") == 0) {
    client(serverhost_str, CLIENT_UDP_MODE, port_str);
  }else if (strcmp(mode_str, "ct") == 0) {
    client(serverhost_str, CLIENT_TCP_MODE, port_str);
  }else if (strcmp(mode_str, "s") == 0) {
    port_str = strdup("6283");
    //port = prt;
    server(port_str);
  }else{
    printf("Incorrect mode.\n");
  }
  return 0;
  
  
}
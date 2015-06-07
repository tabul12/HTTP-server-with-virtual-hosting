/*
	Tornike Abuladze
	assign5
	Lector: George Kobiashvili
*/
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h> 
#include <err.h>
#include <pthread.h> 
#include <semaphore.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <fcntl.h>


#define _GNU_SOURCE
#include <string.h>	
char *strcasestr(const char *haystack, const char *needle);

#define MAXEVENTS 64

char response[] = "HTTP/1.1 200 OK\r\n"
"Content-Type: text/html; charset=UTF-8\r\n\r\n";

//damxmare structura romelsac gadavcem sredis metods
struct thr_info{
	char** v_hosting;
	int argc;
	int client_fd;
};
//es ubralod bechdavs dros
void print_time()
{
	time_t current_time;
    char* c_time_string;
 
    /* Obtain current time as seconds elapsed since the Epoch. */
    current_time = time(NULL);
 
    if (current_time == ((time_t)-1))
    {
        fprintf(stderr, "Failure to compute the current time.");
    }
 
    /* Convert to local time format. */
    c_time_string = ctime(&current_time);
 
    if (c_time_string == NULL)
    {
        fprintf(stderr, "Failure to convert the current time.");
    }
 
    /* Print to stdout. */
    printf("%s", c_time_string);
}
//sockets gardaqmnis non_blocking ad..// 
static int make_socket_non_blocking (int sfd)
{
	int flags, s;
	flags = fcntl (sfd, F_GETFL, 0);
	if (flags == -1)
	{
		perror ("fcntl");
		return -1;
	}
	flags |= O_NONBLOCK;
	s = fcntl (sfd, F_SETFL, flags);
	if (s == -1)
	{
	perror ("fcntl");
	return -1;
	}
	return 0;
}
//sredis metodi romelic gamoizaxeba yoveli conectionis momsaxurebistvis
//roca epool idan "amova"
void * worker_thread(void * info)
{
	
	int client_fd=((struct thr_info *)info)->client_fd;
	int argc=((struct thr_info *)info)->argc;
	char** v_hosting=((struct thr_info *)info)->v_hosting;

	
    char buff[4096];
    
    struct sockaddr_in from_addr;
    uint32_t from_len;
    from_len = sizeof(from_addr);
    
    getpeername(client_fd,(struct sockaddr *)&from_addr,&from_len);
    char *some_addr=inet_ntoa(from_addr.sin_addr);
    
    read(client_fd,buff,4096);

    char* host_p=strcasestr(buff,"Host");
	char* line_h=strtok(host_p,"\r");
    char* host_print=strcasestr(line_h," ");
    host_print++;

    char* user_a=strcasestr(buff,"User-Agent");
    char* line_u=strtok(user_a,"\r");
    char* user_ag_print=strcasestr(line_u," ");
    user_ag_print++;

    char* path=strcasestr(buff,"Get");
    char* path_print=strtok(path," ");
    path_print=strtok(NULL," ");

    printf("Got Connection, INFO:\n");
    print_time();
    printf("%s ",some_addr);
    printf("%s",host_print);
    printf("%s ",path_print);
    printf("%s\n",user_ag_print);
    printf("%s\n","");
    char * full_path=malloc(1024);

    int i;
    for(i = 0; i<argc-1; i++)
    {
    	if(strcmp(v_hosting[i],host_print)==0)
    	{
    		strcat(full_path,v_hosting[i+1]);
    		break;
    	}
    }
    char* eror_path=malloc(1024);
	strcpy(eror_path,full_path);
	strcat(eror_path,"/error.html");

    strcat(full_path,path_print);
    int file_fd;
    file_fd = open(full_path,O_RDONLY);

    
    if(file_fd==-1)
    	file_fd=open(eror_path,O_RDONLY);
    
    struct stat stat_buf;
    fstat(file_fd, &stat_buf);
    off_t offset = 0;  

    write(client_fd, response, sizeof(response) - 1); 
    sendfile(client_fd,file_fd, &offset, stat_buf.st_size);
    close(client_fd);

}
//mtavari gamshvebi saidanac xdeba ganawileba 
//samushaosi
int main(int argc, char const *argv[])
{
	struct thr_info* info=malloc(sizeof(struct thr_info));
	info->argc=argc;
	info->v_hosting=malloc(sizeof(char*)*(argc-1));
  	int i;
  	for(i = 1; i<argc; i++) info->v_hosting[i-1]=(char*)argv[i]; //shevinaxet hostebi da gzebi
	
	int efd;//epol file descriptor;	
  	struct epoll_event event;
  	struct epoll_event *events;
 	struct sockaddr_in svr_addr, cli_addr;
  	socklen_t sin_len = sizeof(cli_addr);

  	int sock = socket(AF_INET, SOCK_STREAM, 0);
  	if (sock < 0)
    	err(1, "can't open socket");
 
  	int port = 8080;
  	svr_addr.sin_family = AF_INET;
  	svr_addr.sin_addr.s_addr = INADDR_ANY;
  	svr_addr.sin_port = htons(port);
 
 	if (bind(sock, (struct sockaddr *) &svr_addr, sizeof(svr_addr)) == -1) {
    	close(sock);
    	err(1, "Can't bind");
  	}

  	if(make_socket_non_blocking(sock)==-1)
  		perror("non_blocking_err");

  	if(listen(sock,SOMAXCONN)==-1)
  		perror("listen");

 	efd=epoll_create1(0);
  	if(efd==-1)
  		perror("error epol_create");

  	event.data.fd=sock;
  	event.events = EPOLLIN | EPOLLET;

  	if(epoll_ctl (efd, EPOLL_CTL_ADD, sock, &event))
  		perror("epoll_ctl");

  	events = calloc (MAXEVENTS, sizeof event); //gamovyavit adgili

 	while(1)
  	{
  		int n=epoll_wait (efd, events, MAXEVENTS, -1);
  		int i;
		//gadavuyvebit n ives
  		for(i = 0; i<n; i++)
  		{
  			if ((events[i].events & EPOLLERR) ||
				(events[i].events & EPOLLHUP) ||
				(!(events[i].events & EPOLLIN))) continue;
  			
  			if(sock==events[i].data.fd)
  			{
  				while(1)
  				{
  					int client_fd;
					client_fd = accept(sock, (struct sockaddr *) &cli_addr, &sin_len);
   					if (client_fd == -1)
      					break;
    				if(make_socket_non_blocking (client_fd)==-1)
    					perror("socket_non_blocking");
    			
    				event.data.fd=client_fd;
    				event.events=EPOLLIN | EPOLLET;

    				if(epoll_ctl(efd,EPOLL_CTL_ADD,client_fd,&event)==-1)
    					perror("epoll_ctl");

  				}
  			}
			else
			{   //davstartoto sredi
				pthread_t con_thread;
				info->client_fd=events[i].data.fd;
				pthread_create(&con_thread, NULL ,worker_thread,(void*)info);

  			}
  		}
  	}
  	pthread_exit(NULL);
  	free(info);
}
//aq ra portzec gveqneba, urlshic unda mivuwerot, magalitad example.com:8080/... 

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

/* definition */
#define MAX_CLIENT   10000
#define DEFAULT_PORT 9000
#define MAX_EVENTS   10000
#define MAX_BUFSIZE  512

/* global definition */
int g_svr_sockfd;              /* global server socket fd */
int g_svr_port;                /* global server port number */

struct {
    int  cli_sockfd;  /* client socket fds */
    char cli_ip[20];              /* client connection ip */
} g_client[MAX_CLIENT];

int g_epoll_fd;                /* epoll fd */

int completeNumber = 0;

struct epoll_event g_events[MAX_EVENTS];
FILE* fin;
char buf[MAX_BUFSIZE];

/* function prototype */
void init_data0(void);            /* initialize data. */
void init_server0(int svr_port);  /* server socket bind/listen */
void epoll_init(void);            /* epoll fd create */
void epoll_cli_add(int cli_fd);   /* client fd add to epoll set */

void userpool_add(int cli_fd,char *cli_ip);

/*--------------------------------------------------------------*/
/* FUNCTION PART
 ---------------------------------------------------------------*/

/*---------------------------------------------------------------
 function : init_data0
 io: none
 desc: initialize global client structure values
 ----------------------------------------------------------------*/
void init_data0(void)
{
    register int i;
    
    for(i = 0 ; i < MAX_CLIENT ; i++)
    {
        g_client[i].cli_sockfd = -1;
    }
}

/*-------------------------------------------------------------
 function: init_server0
 io: input : integer - server port (must be positive)
 output: none
 desc : tcp/ip listening socket setting with input variable
 ----------------------------------------------------------------*/

void init_server0(int svr_port)
{
    struct sockaddr_in serv_addr;
    
    /* Open TCP Socket */
    if( (g_svr_sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0 )
    {
        printf("[ETEST] Server Start Fails. : Can't open stream socket \n");
        exit(0);
    }
    
    /* Address Setting */
    memset( &serv_addr , 0 , sizeof(serv_addr)) ;
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(svr_port);
    
    /* Set Socket Option  */
    int nSocketOpt = 1;
    if( setsockopt(g_svr_sockfd, SOL_SOCKET, SO_REUSEADDR, &nSocketOpt, sizeof(nSocketOpt)) < 0 )
    {
        printf("[ETEST] Server Start Fails. : Can't set reuse address\n");
        close(g_svr_sockfd);
        exit(0);
    }
    
    /* Bind Socket */
    if(bind(g_svr_sockfd,(struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("[ETEST] Server Start Fails. : Can't bind local address\n");
        close(g_svr_sockfd);
        exit(0);
    }
    
    /* Listening */
    listen(g_svr_sockfd,30); /* connection queue is 15. */
    
    printf("[ETEST][START] Now Server listening on port %d\n",svr_port);
}
/*------------------------------- end of function init_server0 */

void epoll_init(void)
{
    struct epoll_event events;
    
    g_epoll_fd = epoll_create(MAX_EVENTS);
    if(g_epoll_fd < 0)
    {
        printf("[ETEST] Epoll create Fails.\n");
        close(g_svr_sockfd);
        exit(0);
    }
    printf("[ETEST][START] epoll creation success\n");
    
    /* event control set */
    events.events = EPOLLIN;
    events.data.fd = g_svr_sockfd;
    
    /* server events set(read for accept) */
    if( epoll_ctl(g_epoll_fd, EPOLL_CTL_ADD, g_svr_sockfd, &events) < 0 )
    {
        printf("[ETEST] Epoll control fails.\n");
        close(g_svr_sockfd);
        close(g_epoll_fd);
        exit(0);
    }
    
    printf("[ETEST][START] epoll events set success for server\n");
}
/*------------------------------- end of function epoll_init */

void epoll_cli_add(int cli_fd)
{
    
    struct epoll_event events;
    
    /* event control set for read event */
    events.events = EPOLLIN;
    events.data.fd = cli_fd;
    
    if( epoll_ctl(g_epoll_fd, EPOLL_CTL_ADD, cli_fd, &events) < 0 )
    {
        printf("[ETEST] Epoll control fails.in epoll_cli_add\n");
    }
    
}

void userpool_add(int cli_fd,char *cli_ip)
{
    /* get empty element */
    register int i;
    
    for( i = 0 ; i < MAX_CLIENT ; i++ )
    {
        if(g_client[i].cli_sockfd == -1) break;
    }
    if( i >= MAX_CLIENT ) close(cli_fd);
    
    g_client[i].cli_sockfd = cli_fd;
    memset(&g_client[i].cli_ip[0],0,20);
    strcpy(&g_client[i].cli_ip[0],cli_ip);
    
}

void userpool_delete(int cli_fd)
{
    register int i;
    
    for( i = 0 ; i < MAX_CLIENT ; i++)
    {
        if(g_client[i].cli_sockfd == cli_fd)
        {
            g_client[i].cli_sockfd = -1;
            break;
        }
    }
}

void userpool_send(char *buffer)
{
    register int i;
    int len;
    
    len = strlen(buffer);
    
    for( i = 0 ; i < MAX_CLIENT ; i ++)
    {
        if(g_client[i].cli_sockfd != -1 )
        {
            len = send(g_client[i].cli_sockfd, buffer, len,0);
            printf("send to %d : %s\n", g_client[i].cli_sockfd, buffer);
            /* more precise code needed here */
        }
    }
    
}


void client_recv(int event_fd)
{
    char r_buffer[512]; /* for test.  packet size limit 1K */
    int len;
    /* there need to be more precise code here */
    /* for example , packet check(protocol needed) , real recv size check , etc. */
    
    /* read from socket */
    len = recv(event_fd,r_buffer,512,0);
        
    if( len < 0 || len == 0 )
    {
        userpool_delete(event_fd);
        close(event_fd); /* epoll set fd also deleted automatically by this call as a spec */
        return;
    }
    
    if(r_buffer[0] != '@'){
        fwrite(buf, MAX_BUFSIZE, 1, fin);
    }else{
        completeNumber++;
    }
    
    
    //userpool_send(r_buffer);
    
}

void server_process(void)
{
    struct sockaddr_in cli_addr;
    int i,nfds;
    int cli_sockfd;
    int cli_len = sizeof(cli_addr);
    int isFirst = 1;
    
    nfds = epoll_wait(g_epoll_fd,g_events,MAX_EVENTS,100); /* timeout 100ms */
    
    if(nfds == 0) return; /* no event , no work */
    if(nfds < 0)
    {
        printf("[ETEST] epoll wait error\n");
        return; /* return but this is epoll wait error */
    }
    
    for( i = 0 ; i < nfds ; i++ )
    {
        if(g_events[i].data.fd == g_svr_sockfd)
        {
            cli_sockfd = accept(g_svr_sockfd, (struct sockaddr *)&cli_addr,(socklen_t *)&cli_len);
            if(cli_sockfd < 0) /* accept error */
            {
            }
            else
            {
                printf("[ETEST][Accpet] New client connected. fd:%d,ip:%s\n",cli_sockfd,inet_ntoa(cli_addr.sin_addr));
                userpool_add(cli_sockfd,inet_ntoa(cli_addr.sin_addr));
                epoll_cli_add(cli_sockfd);
            }
            continue; /* next fd */
        }
        /* if not server socket , this socket is for client socket, so we read it */
        if((nfds >= 20) && (isFirst == 1)){
            userpool_send("@");
            isFirst = 0;
        }
        
        
        
        client_recv(g_events[i].data.fd);
        
        
    } /* end of for 0-nfds */
    
    if(completeNumber == 20){
        for(i = 0; i < 20; i++){
            fseek(fin, 0, SEEK_SET);
            while(1){
                int n;
                n = fread(buf, 1, MAX_BUFSIZE, fin);
                if (n <= 0) // End of file
                    break;
                userpool_send(buf);
            }
            
            completeNumber--;
        }
        
        fclose(fin);
    }
}
/*------------------------------- end of function server_process */

void end_server(int sig)
{
    close(g_svr_sockfd); /* close server socket */
    printf("[ETEST][SHUTDOWN] Server closed by signal %d\n",sig);
    exit(0);
}

int main( int argc , char *argv[])
{

    fin = fopen("input.txt", "w");
    printf("[ETEST][START] epoll test server v1.0 (simple epoll test server)\n");
    /* entry , argument check and process */
    if(argc < 3) g_svr_port = DEFAULT_PORT;
    else
    {
        if(strcmp("-port",argv[1]) ==  0 )
        {
            g_svr_port = atoi(argv[2]);
            if(g_svr_port < 1024) 
            {
                printf("[ETEST][STOP] port number invalid : %d\n",g_svr_port);
                exit(0);
            } 
        }
    }
    
    
    init_data0();  
    
    /* init server */
    init_server0(g_svr_port);
    epoll_init();    /* epoll initialize  */
    
    /* main loop */
    while(1)
    {
        server_process();  /* accept process. */
    } /* infinite loop while end. */
    
}

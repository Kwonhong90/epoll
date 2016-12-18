#include <stdio.h>
#include <strings.h>
#include <fcntl.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <string.h>

#define MAXLINE 512
#define MAX_SOCK 128
char *tmp;
char *escapechar = "exit";
char buf[128];

int main(int argc, char *argv[]) {
    char line[MAXLINE], message[MAXLINE+1];
    int n, pid;
    int nameLen = 0;
    struct sockaddr_in server_addr;
    int maxfdp1;
    int s;  /* 서버와 연결된 소켓번호 */
    fd_set read_fds;
    
    if(argc != 3) {
        printf("사용법 : %s sever_IP port name \n", argv[0]);
        exit(0);
    }
    
    /* 소켓 생성 */
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Client : Can't open stream socket.\n");
        exit(0);
    }
    
    /* 채팅 서버의 소켓주소 구조체 server_addr 초기화 */
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));
    
    /* 연결요청 */
    if(connect(s, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Client : Can't connect to server.\n");
        exit(0);
    }
    
    printf("서버에 접속되었습니다. \n");
    
    maxfdp1 = s + 1;
    FD_ZERO(&read_fds);
    FILE *fin, *fout;
    while(1) {
        FD_SET(0, &read_fds);//0 == stdin. 즉, keyboard도 감시대상으로 체크함.
        FD_SET(s, &read_fds);// 서버와 연결된 소켓이 감시 대상
        
        if(select(maxfdp1, &read_fds, (fd_set *)0, (fd_set *)0, (struct timeval *)0) < 0)  {
            printf("select error\n");
            exit(0);
        }
        
        //서버로부터 메세지가 있는 경우.
        if (FD_ISSET(s, &read_fds))  {
            int size;
            if ((size = recv(s, message, MAXLINE, 0)) > 0)  {
                message[size] = '\0';
                printf("%s \n", message);
                
                if(message[0] == '$'){
                    
                    fin = fopen("input.txt", "r");
                    while(1) {
                        int n;
                        n = fread(buf, 1, 128, fin);
                        if (n <= 0) // End of file
                            break;
                        if (send(s, buf, n, 0)<=0) {
                            printf("send error\n");
                            break;
                        }
                    }
                    
                    strcpy(buf, "@");
                    send(s, buf, n, 0);
                    fclose(fin);
                }
                else{
                    fout = fopen("output.txt", "w");
                    while (1) {
                        int i, len;
                        msg_size = recv(s, buf, BUF_LEN, 0);
                        if (msg_size<=0 || buf[0] == '%') // end of file
                            break;
                        if (fwrite(buf, msg_size, 1, fout)==NULL) {
                            printf("fwrite error\n");
                            break;
                        }
                        if(msg_size < BUF_LEN)
                            break;
                    }
                    fclose(fout);
                    exit(1);
                }
            }
        }
        
        //keyboard 입력이 있는 경우
        if (FD_ISSET(0, &read_fds)) {
            if(fgets(message, MAXLINE, stdin)) {
                tmp = strtok(message, "\n");
                strcpy(message, tmp);
                sprintf(line, "%s", message);
                
                if (send(s, message, MAXLINE, 0) < 0)
                    printf("Error : Write error on socket.\n");
                if (strstr(message, escapechar) != NULL ) { //message가 "exit"면 종료를 하게함.
                    printf("Good bye.\n");
                    close(s);
                    exit(0);
                }
            }
        }
    }
}


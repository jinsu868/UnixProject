#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/mman.h>
#define MAXSIZE 512

/* client의 request를 처리하는 함수들 */
void transaction(int sd);
void handler(int sd, char* filename, int filesize);

/* 에러 처리 함수 */
void errMsg(int fd, char* cause, char* errnum, char* msg, char* lmsg);


int main(int argc, char* argv[]) {
    //socket discripter
    int sd, csd;
    char buf[MAXSIZE];
    char hostname[MAXSIZE], port[MAXSIZE];
    struct sockaddr_in sin, cin;
    // client socket의 길이
    socklen_t len;
    pid_t pid;

    // port number를 입력하지 않으면 error
    if(argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    //server의 socket discripter를 연다.
    if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    memset((char*)&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(atoi(argv[1]));

    //socket에 ip address와 port number 할당
    if(bind(sd, (struct sockaddr*)&sin, sizeof(sin))) {
        perror("bind");
        exit(1);
    }

    //client의 request 대기
    if(listen(sd, 100) == -1) {
        perror("listen");
        exit(1);
    }

    //루프를 돌면서 client의 request를 처리함.
    while (1) {
        len = sizeof(cin);
        //accpet를 통해 client의 sd를 가져온다.
        if ((csd = accept(sd, (struct sockaddr *)&cin, &len)) == -1) {
            perror("accept");
            exit(1);
        }

        //child process를 생성해서 request를 처리한다.
        if((pid = fork()) < 0) {
            perror("fork");
            exit(1);
        }
        else if(pid == 0){
            //hostname과 port 확인
            getnameinfo((struct sockaddr *)&cin, len, hostname, MAXSIZE, port, MAXSIZE, 0);
            printf("hostname : %s, port : %s\n", hostname, port);
        
            //request를 처리하는 함수
            transaction(csd);
        }
        else {
            close(csd);
        }
    }
}

void transaction(int sd) {
    //client의 request를 buf에 저장, method, version, url은 buf에서 추출.
    char buf[MAXSIZE];
    char method[MAXSIZE];
    char version[MAXSIZE];
    char url[MAXSIZE];

    //html파일 이름
    char filename[MAXSIZE];

    struct stat statbuf;

    //buf에 client로부터 온 request를 채움.
    if((recv(sd, buf, MAXSIZE, 0)) < 0) {
        perror("recv");
        exit(1);
    }

    //request확인
    printf("##################request message######################\n");
    printf("%s", buf);
    printf("\n#######################################################\n");

    sscanf(buf, "%s %s %s", method, url, version);
    
    //url이 '/'라면 기본 페이지인 index.html로 filename을 설정
    if(strlen(url) == 1 && url[0] == '/') {
        strcpy(filename, "index.html");
    }
    else {
        /*
        url이 '/'이 아니라면 url의 앞에 .을 찍고 뒤에 .html을 붙여서 현재 디렉토리부터의 상대 경로명을 filename에 저장
        ex) url : test1 이면 filename : ./test1.html 이 된다.
        */
        strcpy(filename, ".");
        strcat(filename, url);
        strcat(filename, ".html");
    }
    
    
    //file의 정보를 statbuf에 저장, 실패시 error처리 routine
    if(stat(filename, &statbuf) < 0) {
        errMsg(sd, filename, "404", "Not found", "file doesn't exist");
        return;
    }

    
    if(!strcmp(method, "GET")) {
        //file의 정보와 socket descripter를 가지고 request를 처리.
        handler(sd, filename, statbuf.st_size);
    }
}

void handler(int sd, char* filename, int filesize) {
    //memory mapping을 위한 변수
    caddr_t addr;
    //file discripter
    int fd;
    char buf[MAXSIZE];

    //client에게 response header를 보냄.
    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    sprintf(buf, "%sConnection: close\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    write(sd, buf, strlen(buf));

    //filename 이름의 filediscripter를 연다.
    if((fd = open(filename, O_RDONLY, 0)) < 0) {
        perror("open");
        exit(1);
    }
    
    //mmap을 사용해서 memory에 mapping함.
    addr = mmap(0, filesize, PROT_READ, MAP_PRIVATE, fd, 0);
    if(addr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    close(fd);
    //mapping된 내용을 filesize만큼 socket discrpiter에 쓴다.
    write(sd, addr, filesize);

    //memory mapping 종료
    if(munmap(addr, filesize) == -1) {
        perror("munmap");
        exit(1);
    }
}
   
//error 처리 함수.
void errMsg(int fd, char* filename, char* errnum, char* msg, char* lmsg)
{
    char buf[MAXSIZE], body[MAXSIZE];
    
    //client에 보낼 resposne body
    sprintf(body, "<html><title>Error</title>\n");
    sprintf(body, "%s%s: %s\r\n", body, errnum, msg);
    sprintf(body, "%s<p>%s: %s\r\n", body, lmsg, filename);

    //client에 보낼 response header
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, msg);
    write(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    write(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));

    //client에 body, header를 씀.
    write(fd, buf, strlen(buf));
    write(fd, body, strlen(body));           
}

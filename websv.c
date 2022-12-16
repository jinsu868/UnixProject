#include "httpf.h"

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


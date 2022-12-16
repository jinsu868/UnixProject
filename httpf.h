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


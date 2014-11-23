#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <signal.h>

#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>


#define PORT 80
#define MAXLINE 1024
#define HEADERSIZE 1024

#define SERVERNAME "test http server"

struct httpRequest
{
	char method[20];
	char page[255];
	char http_ver[80];
};

char root[MAXLINE]; //root directory

int parse_request(char* str, struct httpRequest* request);
int response(int socket);
int send_response(int socket, char* file, char* http_ver, char *message);

int parse_request(char* str, struct httpRequest* request)
{
	char *ch;
	char token[] = "\r\n";
	int i;
	ch = strtok(str, token);
	for(i = 0; i < 3; i++)
	{
		if(ch == NULL)
			break;
		if(i == 0)
			strcpy(request->method, ch);
		else if(i == 1)
			strcpy(request->page, ch);
		else if(i == 2)
			strcpy(request->http_ver, ch);
		ch = strtok(NULL, token);
	}
	return 1;
}

int response(int socket)
{
	char buffer[MAXLINE];
	char page[MAXLINE];
	struct httpRequest request;

	memset(&request, 0x00, sizeof(request));
	memset(buffer, 0x00, MAXLINE);

	if(read(socket, buffer, MAXLINE) <= 0)
	{
		return -1;
	}

	parse_request(buffer, &request);
	if(strcmp(request.method, "GET") == 0)
	{
		sprintf(page, "%s%s", root, request.page);
		if(access(page, R_OK) != 0)
		{
			send_response(socket, NULL, request.http_ver, "404 Not Found");
		}
		else
		{
			send_response(socket, page, request.http_ver, "200 OK");
		}
	}
	else if(strcmp(request.method, "POST") == 0)
	{
		send_response(socket, NULL, request.http_ver, "Sorry, We don't support POST method!");
	}
	else if(strcmp(request.method, "PUT") == 0)
	{
		send_response(socket, NULL, request.http_ver, "Sorry, We don't support PUT method!");
	}
	else if(strcmp(request.method, "DELETE") == 0)
	{
		send_response(socket, NULL, request.http_ver, "Sorry, We don't support DELETE method!");
	}
	else
	{
		send_response(socket, NULL, request.http_ver, "500 Internal Error");
	}

	return 1;
}

int send_response(int socket, char* file, char* http_ver, char *message)
{
	struct tm* tm_ptr;
	time_t now;
	struct stat fstat;
	char header[HEADERSIZE];
	int fd;
	int readn;
	int content_length = 0;
	char buffer[MAXLINE];
	char date_str[80];
	char* daytable = "Sun\0Mon\0Tue\0Wed\0Thu\0Fri\0Sat\0";
	char* monthtable = "Jan\0Feb\0Mar\0Apr\0May\0Jun\0Jul\0Aug\0Sep\0Oct\0Nov\0Dec\0";

	memset(header, 0x00, HEADERSIZE);

	time(&now);
	tm_ptr = localtime(now);

	sprintf(date_str, "%s, %d %s %d %02d:%02d:%02d GMT",
			daytable+(tm_ptr->tm_wday*4),
			tm_ptr->tm_mday,
			monthtable+((tm_ptr->tm_mon)*4),
			tm_ptr->tm_year+1900,
			tm_ptr->tm_hour,
			tm_ptr->tm_min,
			tm_ptr->tm_sec
	);

	if(file != NULL)
	{
		stat(file, &fstat);
		content_length = (int)fstat.st_size;
	}
	else
	{
		content_length = strlen(message);
	}
	sprintf(header, "%s %s\nDate: %s\nServer : %s\nContent-Length: %d\nConnection: close\nContent-Type:text/html;charset=UTF8\n\n",
			http_ver, date_str, message, SERVERNAME, content_length);
	write(socket, header, strlen(header));

	if(file != NULL)
	{
		fd = open(file, O_RDONLY);
		memset(buffer, 0x00, MAXLINE);
		while((readn = read(fd, buffer, MAXLINE)) > 0)
		{
			write(socket, buffer, readn);
		}
		close(fd);
	}
	else
	{
		write(socket, message, strlen(message));
	}

	return 1;
}

int main(int argc, char **argv)
{
	int listenfd;
	int clientfd;
	socklen_t clientlen;
	int pid;
	int optval = 1;
	struct sockaddr_in addr, clientaddr;

	printf("Run HttpServer...");

	if(argc != 2)
	{
		printf("%s 뒤에 서버 루트로 사용할 디렉토리를 지정하세요\n", argv[0]);
		return 1;
	}
	memset(root, 0x00, MAXLINE);
	sprintf(root, "%s", argv[1]);

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if(listenfd == -1)
		return 1;

	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	memset(&addr, 0x00, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(PORT);

	if(bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
	{
		return 1;
	}
	if(listen(listenfd, 5) == -1)
	{
		return 1;
	}

	signal(SIGCHLD, SIG_IGN);
	while(1)
	{
		clientlen = sizeof(clientlen);
		clientfd = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);
		if(clientfd == -1)
		{
			return 1;
		}
		pid = fork();
		if(pid == 0)
		{
			response(clientfd);
			close(clientfd);
			exit(0);
		}
		if(pid == -1)
		{
			return 1;
		}
		close(clientfd);
	}

}










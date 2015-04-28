#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<sys/ioctl.h>
#include<sys/stat.h>
//#include<linux/sockios.h>
#include<netdb.h>
#include<stdio.h>
#include<fcntl.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>
#include<ctype.h>
#include<errno.h>
#include<threadpool.h>
#include<regex.h>

#define MAX_QUEUED 5
#define CONFIG "config.txt"
#define MAX_DOCUMENT_LEN 255
#define N 1024
#define PROTOCOL "HTTP/1.1 "
#define SERVER "Server: my-server\r\n"
#define CONTENT_LEN "Content-Length: "
#define CONNECT "Connection: "
#define CONTENT_TYPE "Content-Type: "

enum {
	OK, SOCKET_ERROR, METHOD_ERROR, DOCUMENT_ERROR, VERSION_ERROR
};

enum {
	USER_AGENT, HOST, CONNECTION
};

enum {
	GET, DELETE, HEAD
};

typedef struct {
	int type;
	char document[MAX_DOCUMENT_LEN];
	int keepAlive;
} request;

void readConfig();
int reformat(char*, int);
int getRequest(int fd, request *req);

char *basedir = "WWW";
int threads = 20;
int port = 55555;

int textType(const char *filename, char *text_type) {

	regex_t regex;
	int reti;
	char msgbuf[100];

	// For html
	if (regcomp(&regex, ".html", 0)) {
		printf("Could not compile regex\n");
		return -1;
	}
	reti = regexec(&regex, filename, 0, NULL, 0);
	if (!reti) {
		strcpy(text_type, "text/html\r\n");
		regfree(&regex);
		return 0;
	}
	regfree(&regex);

	//For image //
	if (regcomp(&regex, ".png", 0)) {
		printf("Could not compile regex\r\n");
		return -1;
	}
	if (!(reti = regexec(&regex, filename, 0, NULL, 0))) {
		strcpy(text_type, "image/png\r\n");
		regfree(&regex);
		return 0;
	}

	regfree(&regex);
	//For image //

	if (regcomp(&regex, ".jpeg", 0)) {
		printf("Could not compile regex\r\n");
		return -1;
	}
	if (!(reti = regexec(&regex, filename, 0, NULL, 0))) {
		strcpy(text_type, "image/jpeg\r\n");
		regfree(&regex);
		return 0;
	}
	regfree(&regex);

	// For Text //
	if (regcomp(&regex, ".xml", 0)) {
		printf("Could not compile regex\r\n");
		return -1;
	}
	if (!(reti = regexec(&regex, filename, 0, NULL, 0))) {
		strcpy(text_type, "text/xml\r\n");
		regfree(&regex);
		return 0;
	}
	regfree(&regex);

	// For Text //
	strcpy(text_type, "text/plain\r\n\r\n");

	return 0;
}

int notImplementHead(int fd,int keepalive) {

	char head[512];
	strcpy(head, PROTOCOL);
	strcat(head, "501 Not Implemented\r\n\0");
	strcat(head, SERVER);
	strcat(head, CONTENT_LEN);
	strcat(head, "24\r\n\0");
	strcat(head, CONNECT);
	strcat(head, "keep-alive\r\n\0");   // connection
	strcat(head, CONTENT_TYPE);
	strcat(head, "text/plain\r\n\r\n\0");
	strcat(head, "Not Implemented!\r\n\0");
	write(fd, head, strlen(head));
	return 0;
}

void toString(char str[], int num) {
	sprintf(str, "%d", num);
}

int okHead(int fd, int size, const char *text_type,int conn_type) {

	char buffer[16];
	char file_size[N];    //  in integer for size //
	char header[64];
	toString(buffer, size);
	strcpy(header, PROTOCOL);
	write(fd, header, strlen(header));

	strcpy(header, "200 OK\r\n\0");
	write(fd, header, strlen(header));
	strcpy(header, SERVER);
	write(fd, header, strlen(header));
	strcpy(header, CONTENT_LEN);
	write(fd, header, strlen(header));
	strcpy(header, buffer);
	write(fd, header, strlen(header));
	strcpy(header, "\r\n\0");
	write(fd, header, strlen(header));
	strcpy(header, CONNECT);
	write(fd, header, strlen(header));

	if (conn_type==1) 	strcat(header, "keep-alive\r\n\0");
	else 				strcat(header,"close\r\n\0");

	write(fd, header, strlen(header));

	strcpy(header, CONTENT_TYPE);
	write(fd, header, strlen(header));
	strcpy(header, text_type);
	write(fd, header, strlen(header));

	return 0;
}

int deleteHead(char *head,int conn_type) {

	strcpy(head, PROTOCOL);
	strcat(head, "204 No content\r\n\0");
	strcat(head, SERVER);
	strcat(head, CONTENT_LEN);
	strcat(head, "12\r\n\0");
	strcat(head, CONNECT);
	if (conn_type==1) 	strcat(head, "keep-alive\r\n\0");
	else 				strcat(head,"close\r\n\0");
	strcat(head, CONTENT_TYPE);
	strcat(head, "text/plain\r\n\0");
	strcat(head, "No content!\n\0");
	return 0;
}

int notFoundHead(char *header, int retLen,int conn_type) {

	strcpy(header, PROTOCOL);
	strcat(header, "404 Not Found\r\n\0");
	strcat(header, SERVER);
	if(retLen){
		strcat(header, CONTENT_LEN);
		strcat(header, "20\r\n\0");
	}
	strcat(header, CONNECT);
	if (conn_type==1) 	strcat(header, "keep-alive\r\n\0");  // We need the struct request
	else 				strcat(header,"close\r\n\0");

	strcat(header, CONTENT_TYPE);
	strcat(header, "text/plain\r\n\0");

	return 0;
}

int deleteMethod(const char *filename, int socket,int connection_type) {

	char header[254];
	char content[N];

	if (access(filename, F_OK) == -1) {
		notFoundHead(header, 1,connection_type);
		strcpy(content, header);
		strcat(content, "\r\nDocument not found!\r\n\0");
		if (write(socket, content, strlen(content)) < 0) {
			perror("Error in write().");
			return -1;
		}
		return 0;
	} else {
		if (unlink(filename) < 0) {
			notFoundHead(header, 1,connection_type);
			strcpy(content, header);
			strcat(content, "\r\nDocument not found!\r\n\0");
			if (write(socket, content, strlen(content)) < 0) {
				perror("Error in write().");
				return -1;
			}
			return 0;
		} else {
			deleteHead(header,connection_type);
			if (write(socket, header, strlen(header)) < 0) {
				perror("Error in write().");
				return -1;
			}
		}
	}
	return 0;
}

int headMethod(const char *filename, int socket,int connection_type) {

	char header[128];	//Malloc on this
	char content[N];   // Malloc on this
	struct stat buf;
	char text_type[128];
	int file;

	if ((file =open(filename, O_RDONLY)) == -1) {
		notFoundHead(header, 0,connection_type);
		strcpy(content, header);
		if (write(socket, content, strlen(content)) < 0) {
			perror("Error in open().");
			return -1;
		}
		return 0;
	}else{
			textType(filename, text_type);
			if (fstat(file, &buf) < 0  ) {
					perror("Error in write().");
					notFoundHead(header, 0,connection_type);
					strcpy(content, header);
					if (write(socket, content, strlen(content)) < 0) {
						perror("Error in write().");
						return -1;
					}
			}
			okHead(socket, buf.st_size, text_type,connection_type);
			//write(socket,header,strlen(header));					see something here
	}
	close(file);
	return 0;
}

int getMethod(const char *filename, int socket,int connection_type) {

	int file;
	char header[128];
	char content[N];
	char text_type[20];
	struct stat buf;

	if (access(filename, F_OK) == -1) {
		notFoundHead(header, 1,connection_type);
		strcpy(content, header);
		strcat(content, "\r\nDocument not found!\r\n\0");
		if (write(socket, content, strlen(content)) < 0) {
			perror("Error in write().");
			return -1;
		}
		return 0;
	} else {
		if ((file = open(filename, O_RDONLY)) < 0) {
					perror("Error in open()");
					return -1;
		}else{
			if (fstat(file, &buf) < 0) {
				perror("Error in write().");
				notFoundHead(header, 1,connection_type);
				strcpy(content, header);
				strcat(content, "\r\nDocument not found!\r\n\0");
				if (write(socket, content, strlen(content)) < 0) {
					perror("Error in write().");
					return -1;
				}
				return -1;
			}

			textType(filename, text_type);
			okHead(socket, buf.st_size, text_type,connection_type);
			//write(socket,header,strlen(header));
			int bytes = read(file, content, N);
			content[bytes] = '\0';
			while (bytes > 0) {
				if (write(socket, content, strlen(content)) < 0) {
					perror("Error in write()");
					return -1;
				}
				bytes = read(file, content, N);
				content[bytes] = '\0';
			}
			close(file);
			if (bytes < 0) {
				perror("Error in read()");
				return -1;
			}
		}
	}
	return 0;
}
void manageClient(int fd) {
	char file[512];
	int status;
	request r;

	do {
		status = getRequest(fd, &r);
		if (status != 0) {
			if (status == METHOD_ERROR)
				notImplementHead(fd,r.keepAlive);
			else if (status == DOCUMENT_ERROR)
				printf("Error in document\n");
			else if (status == VERSION_ERROR)
				printf("Error in Version\n");
			continue;
		}
		strcpy(file, basedir);
		if (strlen(r.document)==1)  strcpy(r.document,"/file.txt\0");
		strcpy(file + strlen(basedir), r.document);
		printf("%s %s %d %d\n", file, r.document, r.keepAlive, r.type);

		if (r.type == GET) {
			getMethod(file, fd,r.keepAlive);
		} else if (r.type == DELETE) {
			deleteMethod(file, fd,r.keepAlive);
		} else if (r.type == HEAD) {
			headMethod(file, fd,r.keepAlive);
		}
	} while (r.keepAlive == 1);
	//if(not exists file) printf("302 File Not Exists\r\n")
	//...

	sleep(2);
	close(fd);
}

int main(int argc, char* argv[]) {
	int sock, newsock, serverlen, clientlen;
	char buf[256];
	task t;
	struct sockaddr_in server, client;
	struct sockaddr* serverptr;
	struct sockaddr* clientptr;
	struct hostent* rem;
	threadpool tp;
	readConfig();

	threadpool_init(&tp, threads);

	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Error in socket().");
		exit(-1);
	}

	server.sin_family = PF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY );
	server.sin_port = htons(port);

	serverptr = (struct sockaddr*) &server;
	serverlen = sizeof(server);

	if (bind(sock, serverptr, serverlen) < 0) {
		perror("Error in bind().");
		exit(-1);
	}

	if (listen(sock, MAX_QUEUED)) {
		perror("Error in listen()");
		exit(-1);
	}
	printf("Now listening on %d\n", port);

	while (1) {
		clientptr = (struct sockaddr*) &client;
		clientlen = sizeof(client);

		if ((newsock= accept(sock, clientptr, &clientlen)) < 0) {
			perror("Error in accept().");
			exit(-1);
		}

		if ((rem = gethostbyaddr((char*) &client.sin_addr.s_addr,
				sizeof(client.sin_addr.s_addr), client.sin_family)) == NULL ) {
			herror("Error in gethostbyadd()");
			exit(-1);
		}
		printf("Connected with %s\n", rem->h_name);
		//manageClient(newsock);

		t.function = &manageClient;
		t.arg = newsock;
		enqueue(&tp, &t);
	}
}

int reformat(char *str, int len) {
	int i, p = 0, comment = 0, reset = 1;
	for (i = 0; i < len; i++) {
		if (reset && str[i] == '#')
			comment = 1;
		if ((!comment && !isspace(str[i]))|| str[i] == '\n'){
		str[p] = str[i];
		if (str[i] == '\n') {
			reset = 1;
			comment = 0;
		} else
		reset = 0;
		p++;
	}
}

	str[p] = '\0';
	return p;
}

void readConfig() {
	const char *options[] = { "THREADS", "PORT" };
	char *found, *token, *pos, *last;
	char buffer[BUFSIZ], tempchar;
	int fd = open(CONFIG, O_RDONLY), n = BUFSIZ - 1, loc = -1, i, ends = 0, c,
			value, info = 0;
	if (fd < 0)
		return;
	while (n == BUFSIZ - 1) {
		n = read(fd, buffer, BUFSIZ - 1);
		c = reformat(buffer, n);
		last = &buffer[c];
		for (i = 0; i < 2; i++) {
			pos = strstr(buffer, options[i]);
			if (pos != NULL ) {
				pos += strlen(options[i]) + 1;
				ends = 0;
				while (&pos[ends] != last && pos[ends] != '\n') {
					ends++;
				}
				tempchar = pos[ends];
				pos[ends] = '\0';
				value = atoi(pos);
				pos[ends] = tempchar;
				if (i == 0)
					threads = value;
				else
					port = value;
			}
		}
	}
}

int getRequest(int fd, request *req) {
	const char *headers[] = { "User-Agent", "Host", "Connection" };
	const char *reqTypes[] = { "GET", "DELETE", "HEAD" };

	req->keepAlive = 1;

	int n = BUFSIZ, newlines = 0, i = 0, readFirst = 1, lastline = 0, capacity,
			j, delim, info = 0;
	char buffer[BUFSIZ + 1];
	char *token, *delp;

	if (fd < 0)
		return SOCKET_ERROR;
	n = read(fd, buffer, BUFSIZ);
	capacity = n;

	while (n != 0) {
		i = 0;
		newlines = 0;
		lastline = 0;
		while (i < capacity) {
			if (buffer[i] == '\r') {
				newlines++;
				lastline = i;
			}
			i++;
		}
		if (newlines == 0)
			goto err;
		if (lastline == 0) {
			//consNewLines++;
			//if(consNewLines==2)
			goto finish;
		}
		buffer[lastline] = '\0';
		token = strtok(buffer, "\r");
		while (token) {
			if ((*(token + 1)) == '\0') {
				//consNewLines++;
				//if(consNewLines==2)
				goto finish;
			} else {
				if (readFirst) {
					readFirst = 0;
					delp = strstr(token, " ");
					if (!delp)
						return METHOD_ERROR;
					for (j = 0; j < sizeof(reqTypes) / sizeof(char*); j++) {
						if (strncmp(token, reqTypes[j],
								(delp - token) / sizeof(char)) == 0)
							break;
					}
					if (j == sizeof(reqTypes) / sizeof(char*))
						return METHOD_ERROR;
					req->type = j;
					token = delp + 1;
					delp = strstr(token, " ");
					if (!delp)
						return VERSION_ERROR;
					*delp = '\0';
					if (strlen(token) >= MAX_DOCUMENT_LEN)
						return DOCUMENT_ERROR;
					strcpy(req->document, token);
					token = delp + 1;
					if (strcmp(token, "HTTP/1.1") != 0)
						return VERSION_ERROR;
					info = 1;
				} else {
					if ((*token) == '\n')
						token++;
					delim = -1;
					for (j = 0; j < strlen(token) && delim == -1; j++)
						if (token[j] == ':')
							delim = j;
					if (delim == -1)
						continue;
					for (j = 0; j < sizeof(headers) / sizeof(char*); j++) {
						if (strncmp(token, headers[j], strlen(headers[j])) == 0)
							break;
					}
					if (j == sizeof(headers) / sizeof(char*))
						goto next;
					token += delim;
					while (!isalnum(*token))
						token++;
					//printf("v=%s\n", token);
					switch (j) {
					case USER_AGENT:
						break;
					case HOST:
						break;
					case CONNECTION:
						if (strcmp(token, "close") == 0)
							req->keepAlive = 0;
						break;
					}
					//printf("Headers: %s %s\n", token, headers[j]);
				}
			}
			next:
			//printf("token: %s\n", token+((*token)=='\n'?1:0));
			token = strtok(NULL, "\r");
		}
		int nextValidChar = lastline + 2;
		for (i = nextValidChar; i < capacity; i++) {
			buffer[i - nextValidChar] = buffer[i];
		}
		n = read(fd, buffer + capacity - nextValidChar,
				BUFSIZ - (capacity - nextValidChar));
		capacity = n + capacity - nextValidChar;
	}
	finish: if (info == 0)
		return METHOD_ERROR;
	return OK;

	err: return -1;
}

/**
 * This is a multi-threading server that can server
 * GET, DELETE, POST methods,
 *
 * In EPL371 AS4 we asked to implement a server.
 * The scope of the AS4 is to make us
 * understand who server works, who threads
 * server content to client how, processes comunicate
 * throw networks(Sockets).
 *

 * @author Barnabas Papaiwnnou
 * @author Theodoros Charalambous
 *
 **/



/**
 * Libraries and System Calls to implement
 * the server.
 *
 **/

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
#include <time.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>
#include<ctype.h>
#include<errno.h>
#include<threadpool.h>
#include<regex.h>
#include<signal.h>


/**
 * Constants For the server
 *
 *
 **/
#define MAX_QUEUED 5
#define MAX_TIME 15
#define MAX_REQUESTS 50
#define CONFIG "config.txt"
#define MAX_DOCUMENT_LEN 255
#define N 4096
#define PROTOCOL "HTTP/1.1 "
#define SERVER "Server: my-server\r\n"
#define CONTENT_LEN "Content-Length: "
#define CONNECT "Connection: "
#define CONTENT_TYPE "Content-Type: "

/**
 * Enum that defines if the request header
 * have any error.
 *
 **/

enum {
	OK, SOCKET_ERROR, METHOD_ERROR, DOCUMENT_ERROR, VERSION_ERROR
};

/**
 * Enumaration that defines what the name said
 *
 *
 *
 */

enum {
	USER_AGENT, HOST, CONNECTION
};

/**
 * Enumaration that defines what request we have and what
 * the response it will be.
 *
 */

enum {
	GET, DELETE, HEAD
};

/**
 * Structure that defines the request,
 * and the file from the request.
 *
 *
 */



typedef struct {
	int type;
	char document[MAX_DOCUMENT_LEN];
	int keepAlive;
} request;

void readConfig();
int reformat(char*, int);
int getRequest(int fd, request *req);


/**
 *
 *
 *
 */

char *basedir = "./www/";
int threads = 20;
int port = 55555;


int isOpen(int fd){
char a;
return write(fd, &a, 0)!=-1;
}

/**
 * Method below uses regular expresions
 * to define the content type for the file
 * that request(px text/plain )
 *
 * @param char *
 * @param char *
 */

int textType(const char *filename, char *text_type) {

	regex_t regex;
	int reti;
	char msgbuf[100];

	

	// For html
	if (regcomp(&regex, ".txt", 0)) {
		printf("Could not compile regex\n");
		return -1;
	}
	reti = regexec(&regex, filename, 0, NULL, 0);
	if (!reti) {
		strcpy(text_type, "text/txt\r\n\r\n\0");
		regfree(&regex);
		return 0;
	}
	regfree(&regex);


	// For html
	if (regcomp(&regex, ".htm", 0)) {
		printf("Could not compile regex\n");
		return -1;
	}
	reti = regexec(&regex, filename, 0, NULL, 0);
	if (!reti) {
		strcpy(text_type, "text/html\r\n\r\n\0");
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
		strcpy(text_type, "image/png\r\n\r\n\0");
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
		strcpy(text_type, "image/jpeg\r\n\r\n\0");
		regfree(&regex);
		return 0;
	}
	regfree(&regex);

	//For image //
	if (regcomp(&regex, ".jpg", 0)) {
		printf("Could not compile regex\r\n");
		return -1;
	}
	if (!(reti = regexec(&regex, filename, 0, NULL, 0))) {
		strcpy(text_type, "image/jpeg\r\n\r\n\0");
		regfree(&regex);
		return 0;
	}
	regfree(&regex);

	// For Text //
	if (regcomp(&regex, ".gif", 0)) {
		printf("Could not compile regex\r\n\r\n\0");
		return -1;
	}
	if (!(reti = regexec(&regex, filename, 0, NULL, 0))) {
		strcpy(text_type, "image/gif\r\n\r\n\0");
		regfree(&regex);
		return 0;
	}
	regfree(&regex);



	// For Text //
	if (regcomp(&regex, ".xml", 0)) {
		printf("Could not compile regex\r\n\r\n");
		return -1;
	}
	if (!(reti = regexec(&regex, filename, 0, NULL, 0))) {
		strcpy(text_type, "text/xml\r\n\r\n\0");
		regfree(&regex);
		return 0;
	}
	regfree(&regex);

	// For Text //
	if (regcomp(&regex, ".css", 0)) {
		printf("Could not compile regex\r\n\r\n");
		return -1;
	}
	if (!(reti = regexec(&regex, filename, 0, NULL, 0))) {
		strcpy(text_type, "text/css\r\n\r\n\0");
		regfree(&regex);
		return 0;
	}
	regfree(&regex);


	
	// For Text //
	if (regcomp(&regex, ".pdf", 0)) {
		printf("Could not compile regex\r\n\r\n");
		return -1;
	}
	if (!(reti = regexec(&regex, filename, 0, NULL, 0))) {
		strcpy(text_type, "application/pdf\r\n\r\n\0");
		regfree(&regex);
		return 0;
	}
	regfree(&regex);
	

	// For Text // 
	strcpy(text_type, "application/octet-stream\r\n\r\n\0");

	return 0;
}



/**
 *
 * Method that creates the header for the
 * not implemented request.
 *
 * Creates and write the header to the socket.
 *
 * @param int
 */

int notImplementHead(int fd) {

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

/**
 * Makes a integer to String.
 *
 * It useful for file size (content length in header)
 *
 */
void toString(char str[], int num) {
	sprintf(str, "%d", num);
}


/**
 * Method that create the 200 OK response
 * and reply it to the client throw the socket
 *
 *
 * @param char *
 * @param int
 * @param int
 * @param int
 */
int okHead(int fd, int size, const char *text_type, int keep) {

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
	write(fd, header, strlen(header)); // filesize
	if(keep){
		sprintf(header, "Keep-Alive: timeout=%d, max=%d\r\n\0", MAX_TIME, MAX_REQUESTS);
		write(fd, header, strlen(header));
	}
	strcpy(header, CONNECT);
	write(fd, header, strlen(header));
	if(keep) strcpy(header, "Keep-Alive\r\n\0");
	else strcpy(header, "close\r\n\0");
	write(fd, header, strlen(header));
	strcpy(header, CONTENT_TYPE);
	write(fd, header, strlen(header));
	strcpy(header, text_type);
	write(fd, header, strlen(header));

	return 0;
}

/**
  * Method that creates the deleted response
  * to the client.
  *
  *
  * @param char*
  **/

int deleteHead(char *head) {

	strcpy(head, PROTOCOL);
	strcat(head, "204 No content\r\n\0");
	strcat(head, SERVER);
	strcat(head, CONTENT_LEN);
	strcat(head, "12\r\n\0");
	strcat(head, CONNECT);
	strcat(head, "keep-alive\r\n\0");    //request or something
	strcat(head, CONTENT_TYPE);
	strcat(head, "text/plain\r\n\0");
	strcat(head, "No content!\n\0");

	return 0;
}


/**
 * Method that creates the not found header
 * for the client if a file that reuested not found.
 *
 * @param
 **/

int notFoundHead(char *header, int retLen) {

	strcpy(header, PROTOCOL);
	strcat(header, "404 Not Found\r\n\0");
	strcat(header, SERVER);
	if(retLen){
		strcat(header, CONTENT_LEN);
		strcat(header, "20\r\n\0");
	}
	strcat(header, CONNECT);
	strcat(header, "keep-alive\r\n\0");  // We need the struct request
	strcat(header, CONTENT_TYPE);
	strcat(header, "text/plain\r\n\0");

	return 0;
}


/**
 * Method that implements the delete request.
 *
 * Takes two parameters filename and socket
 * it checks for access on this file and
 * that unlink the file.
 *
 * If anything goes wrong file not found  response
 * send back to client
 *
 *
 * @param filename
 * @param int
 *
 */

int deleteMethod(const char *filename, int socket) {

	char header[128];	//Malloc on this
	char content[N];   // Malloc on this

	if (access(filename, F_OK) == -1) {
		notFoundHead(header, 1);
		strcpy(content, header);
		strcat(content, "\nDocument not found!\n\0");
		if (write(socket, content, strlen(content)) < 0) {
			perror("Error in write().");
			return -1;
		}
		return 0;
	} else {
		if (unlink(filename) < 0) {
			notFoundHead(header, 1);
			strcpy(content, header);
			strcat(content, "\nDocument not found!\n\0");
			if (write(socket, content, strlen(content)) < 0) {
				perror("Error in write().");
				return -1;
			}
			return 0;
		} else {
			deleteHead(header);
			if (write(socket, header, strlen(header)) < 0) {
				perror("Error in write().");
				return -1;
			}
		}
	}
	return 0;
}


/**
 * Method that iplements the head reuest.
 * It open the meta data of the file to take the
 * size of file and then response to client
 * with HEAD response
 *
 * @param int
 * @param int
 * @param int
 *
 *
 */



int headMethod(const char *filename, int socket, int keep) {

	char header[128];	//Malloc on this
	char content[N];   // Malloc on this
	struct stat buf;
	char text_type[128];

	if (access(filename, F_OK) == -1) {
		notFoundHead(header, 0);
		strcpy(content, header);
		//strcat(content,"\nDocument not found!\n\0");
		if (write(socket, content, strlen(content)) < 0) {
			perror("Error in write().");
			return -1;
		}
		return 0;
	} else {
		if (access(filename, F_OK) < 0) {
			perror("Error in write().");
			notFoundHead(header, 0);
			strcpy(content, header);
			//strcat(content,"\nDocument not found!\n\0");
			if (write(socket, content, strlen(content)) < 0) {
				perror("Error in write().");
				return -1;
			}
			return -1;
		} else {
			textType(filename, text_type);
			if (lstat(filename, &buf) < 0) {
				perror("Error in write().");
				notFoundHead(header, 0);
				strcpy(content, header);
				if (write(socket, content, strlen(content)) < 0) {
					perror("Error in write().");
					return -1;
				}
				return -1;
			}
			okHead(socket, buf.st_size, text_type, keep);
			//write(socket,header,strlen(header));
		}
	}
	return 0;
}


/**
 * Method that implements the ger request.
 * method opens the meta-data of the file
 * to take the size creates the 200 ok headed
 * response to client and then sents the content of the file.
 * if file not exist response with not found header.
 *
 * @param
 * @param
 *
 */

int getMethod(const char *filename, int socket, int keep) {

	int file;
	char header[128];	//Malloc on this
	char content[N];   // Malloc on this
	char text_type[20];
	struct stat buf;

	if (access(filename, F_OK) == -1) {
		notFoundHead(header, 1);
		strcpy(content, header);
		strcat(content, "\nDocument not found!\n\0");
		if (write(socket, content, strlen(content)) < 0) {
			perror("Error in write().");
			return -1;
		}
		return 0;
	} else {
		if (lstat(filename, &buf) < 0) {
			perror("Error in write().");
			notFoundHead(header, 1);
			strcpy(content, header);
			strcat(content, "\nDocument not found!\n\0");
			if (write(socket, content, strlen(content)) < 0) {
				perror("Error in write().");
				return -1;
			}
			return -1;
		}
		// and fstat
		if ((file = open(filename, O_RDONLY)) < 0) {
			perror("Error in open()");
			return -1;
		} else {

			textType(filename, text_type);
			okHead(socket, buf.st_size, text_type, keep);
			//write(socket,header,strlen(header));
			int bytes = read(file, content, N);
			content[bytes] = '\0';
			while (bytes > 0) {
				//printf("%s", content);
				if (write(socket, content, bytes) < 0) {
					perror("Error in write()");
					return -1;
				}
				bzero(content,strlen(content));
				bytes=read(file, content, N);
				content[bytes] = '\0';
			}

			if (bytes < 0) {
				perror("Error in read()");
				return -1;
			}
		}
	}
	return 0;
}

/**
 *
 * Method that the threads use to manage and
 * server clients.
 * Based on the request that client sent the manageClient
 * calls the method for this request.
 *
 * @param int
 */

void manageClient(int fd) {
	char file[512];
	int status,nreq=0;
	request r;
	time_t start = 0,end = 0, elapsed = 0;

	do {
		start = time(NULL);
		status = getRequest(fd, &r);
		nreq++;

		if (status != 0) {
			if (status == METHOD_ERROR)
				notImplementHead(fd);
			else if (status == DOCUMENT_ERROR)
				printf("Error in document\n");
			else if (status == VERSION_ERROR)
				printf("Error in Version\n");
			goto beforeloop;
		}
		if(strcmp(r.document, "/")==0) strcpy(r.document, "/index.html");
		strcpy(file, basedir);
		strcpy(file + strlen(basedir), r.document);
		printf("%s %d %d\n", file, r.keepAlive, r.type);
		if (r.type == GET) {
			getMethod(file, fd, r.keepAlive);
		} else if (r.type == DELETE) {
			deleteMethod(file, fd);
		} else if (r.type == HEAD) {
			headMethod(file, fd, r.keepAlive);
		}
		beforeloop:
		end = time(NULL);
		elapsed = end - start;
		printf("Elapsed: %d seconds\n", elapsed);
	} while (isOpen(fd) && r.keepAlive == 1 && nreq<MAX_REQUESTS && elapsed<MAX_TIME);
	//if(not exists file) printf("302 File Not Exists\r\n")
	//...

	sleep(2);
	close(fd);
}



/** Main Method where the server start running.
 * Server makes a threadpool to server clients and before
 * that server reads a configuration file for his settings
 *
 * @param int
 * @param int
 */
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

	signal(SIGPIPE, SIG_IGN);

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

		if ((newsock = accept(sock, clientptr, &clientlen)) < 0) {
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

/**
 * Method that reades server's congiguration file
 * to set up threads and etc.
 *
 *
 */
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

	while (n > 0) {
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

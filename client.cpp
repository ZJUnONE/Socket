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

#include <openssl/md5.h>
using namespace std;

#define PORT 8000
#define MAXBUF 1024
int ConnectionGenerate(string IP, int port);
string SendAuth(int socket);
void TKGenerate(char *md5 ,string A, string C, string K);
void CNounceGenerate(char *s); //8 bytes
int MyWrite(int socket, const string &s, struct timeval *time);
int MyRead(int socket, string & s, struct timeval *time);

int main(int argc, char *argv[]){
	string MasterKey = "123";
    string IP = "127.0.0.1";
    char buf[100];
    int s_fd, state = 0;
    string s;
    char an[17], cn[17];
    int r;
    char md5[17];
    char t[5];
    int mlen;
    struct timeval time = {10, 0};
    if (0)
    {
        cout << "Usage: [IP] [port] [MasterKey]" <<endl ;
        exit(1);
    }
    s_fd = ConnectionGenerate(IP, PORT);
    while(1)
    {
    	switch(state)
    	{
    		case 0:
    		{
				cout << "Send authentication" << endl;
    			s = SendAuth(s_fd);
    			cout << s <<endl;
    			if(s.substr(0, 4) == string("Msg1") && s.size() == 16)
    			{
    				cout << "Authentication succeeded!" << endl << "Receive message: " << s << endl;
    				r = *(int *)(s.c_str() + 4);
    				strcpy(an, s.c_str() + 8) ;
    				state++;
    				break;
    			}
    			else
    			{
    				cout<<"Authentication error!"<<endl;
    				close(s_fd);
    				exit(-1);
    			}
    		}
    		case 1:
    		{
    			*(int *)t = r;
				t[4] = 0;
    			time = {10, 0};
    			CNounceGenerate(cn);
    			TKGenerate(md5,string(an), string(cn), MasterKey);
    			md5[16] = 0;
    			s = string("Msg2") + string(t) + string(md5);
    			if( (mlen = MyWrite(s_fd, string(md5),&time)) != 24)
    			{
    				cout << "Ms2 send error: " << mlen << endl;
    				exit(-1);
    			}
    			else
    			{
					cout << "Send succeeded!" << endl;
					close(s_fd);
					exit(0);
    			}
    		}
    	}
    }

    

    
}

void TKGenerate(char *md5 ,string A, string B, string C)
{
	MD5_CTX ctx;
	string x = A + B + C;
	MD5_Init(&ctx);
	MD5_Update(&ctx, x.c_str(),x.size());
	MD5_Final((unsigned char *)md5, &ctx);
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
		cout << "Connect failed" << endl;
		exit(-1);
	}
	cout << "Connection built" <<endl;
	return s_fd;
}

string SendAuth(int socket)
{	
	struct timeval time;
	string buf;
	const string s = "Authentication_Request";
	time = {10,0};
	int mlen = MyWrite(socket, s, &time);
	if(mlen <= 0)
	{
		cout << "Write error"<<endl;
		exit(-1);
	}
	time = {10,0};
	mlen = MyRead(socket, buf, &time);
	if(mlen <= 0)
		return string("Error");
	else
		return buf;
}
int MyRead(int socket, string & s, struct timeval *time)
{
	int mlen;
	char buf[MAXBUF];
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(socket, &rfds);
	if(select(socket+1, &rfds, NULL, NULL, time) <= 0)
		return -1;
	else
		mlen = read(socket,buf,sizeof(buf));
	buf[mlen] = '\0';
	s = buf;
	return mlen;

}
int MyWrite(int socket, const string &s, struct timeval *time)
{
	int mlen;
	char buf[MAXBUF];
	fd_set wfds;
	FD_ZERO(&wfds);
	FD_SET(socket,&wfds);
	if(select(socket+1, NULL, &wfds, NULL, time) <= 0)
		return -1;
	else
		mlen = write(socket,s.c_str(),s.size());
	return mlen;
}

void CNounceGenerate(char *s)
{
	srand((int)time(0));
	int a = rand();
	int b = rand();
	int c = rand();
	int d = ranf();
	*(int *)s = a;
	*(int *)(s+4) = b;
	*(int *)(s+8) = c;
	*(int *)(s+12) = d;
	s[16] = 0;
}
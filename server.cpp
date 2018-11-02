#include <iostream>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <sys/select.h>
#define MAX_TIME 10		//Max waiting time


int Init(int port);
int Acception(int l_fd);
int RequestGet(int socket, int r, char *an);
void ANounceGenerate(char *s);
void CNounceReceive(int socket, int r, char *s);

using namespace std;
int main(int argc, char *argv[])
{
	int r = 0;
	int state;
    int l_fd, c_fd;
    char buf[1024];//content buff area

    int port = 8000;
	char an[17];
	char cn[17];
	int flag;
	int nfdp;
	fd_set rfds;
	fd_set wfds;
	struct timeval time; // 10s maximum delay.
    if (0)
    {
        printf("Usage with parameters [MasterKey] [port]\n");
        exit(1);
    }

    l_fd = Init(port);
    while(1)
	{
		c_fd = Acception(l_fd);
		flag = 1;
        state = 0;
        while(flag)
		{
			switch(state)
			{
				
				case 0:
				{
					flag = RequestGet(c_fd, r, an);
					if(!flag)
					{
						cout << "Error!" << endl;
						flag = 0;
						break;
					}
					else
						state++;
					break;
				}
				case 1:
				{
					CNounceReceive(c_fd, r,cn);
					cout << "Receive CN: "<< cn << endl;
					flag = 0;
				}
			}
		}
		close(c_fd);
    }

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
	
    listen(l_fd,10);//lisiening start
    cout << "Initiallization succeeded!"<<endl;
    return l_fd;
}

int Acception(int l_fd)
{
	struct sockaddr_in c_in;//client address structure
    socklen_t client_len;
    char ipstr[128];
	cout<<"Waiting for connection."<<endl;
    c_fd = accept(l_fd,(struct sockaddr *)&c_in,&client_len);
    cout << "Client IP: " << inet_ntop(AF_INET, &c_in.sin_addr.s_addr, ipstr, sizeof(ipstr)) << ". Port: " << ntohs(c_in.sin_port) << endl;
    return c_fd;
}
void ANounceGenerate(char *s)
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

int MyRead(int socket, string & s, struct timeval *time)
{
	int mlen;
	char buf[MAXBUF];
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(socket, &rfds);
	time = {10, 0};
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

int RequestGet(int socket, int r, char *an)
{
	int meln;
	char t[5];
	string s;
	struct timeval time = {10, 0};
	mlen = Myread(socket, s, NULL);
	if(mlen <= 0)
		return 0;
	cout << "Receive message: "<< s << endl;
	if(s == string("Authentication_Request"))
	{
		*(int *)t = r;
		t[4] = 0;
		ANounceGenerate(an);
		s = string("Msg1") + string(t) + string(an);
		mlen = MyWrite(socket, s, &time);
		if(mlen != 4+4+16)
		{
			cout << "Write Error!" << endl;
			return 0;
		}
	}
	return 1;
}

void CNounceReceive(int socket, int r, char *c)
{
	string s;
	struct timeval time = {10, 0};
	if(MyRead(socket, s, &time) <= 0)
	{
		cout << "Error" <<endl;
		exit(-1);
	}
	else if(s.substr(0,4)==string("Msg2"))
	{
		strcp(c, (s.c_str()+8));
	}
}
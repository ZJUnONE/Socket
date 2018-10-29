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

using namespace std;
int main(int argc, char *argv[])
{
    struct sockaddr_in s_in;//server address structure
    struct sockaddr_in c_in;//client address structure
    int l_fd,c_fd;
    socklen_t len;
    char buf[1024];//content buff area
    char addr_p[16];
    char ipstr[128];
    int port = 8000;
	
    int flag;
	
    int nfdp;
    fd_set rfds;
    fd_set wfds;
    struct timeval time = {MAX_TIME, 0}; // 10s maximum delay.
	
    if (0)
    {
        printf("Usage with parameters [MasterKey] [port]\n");
        exit(1);
    }
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
    while(1)
	{
		flag = 1;
        cout<<"Waiting for connection."<<endl;

        c_fd = accept(l_fd,(struct sockaddr *)&c_in,&len);
        cout << "Client IP: " << inet_ntop(AF_INET, &c_in.sin_addr.s_addr, ipstr, sizeof(ipstr)) << ". Port: " << ntohs(c_in.sin_port) << endl;
        while(flag)
		{
			FD_ZERO(&rfds);
			FD_SET(c_fd,&rfds);
			nfdp = c_fd + 1;
			int j = select(nfdp, &rfds, NULL, NULL, &time);
			switch(j)
			{
				case -1:
					exit(-1);
				case 0:
				{
					cout << "Connection closed because of 10s waiting!" << endl;
					close(c_fd);
					flag = 0;
					break;
				}
				default:
				{
					int n = read(c_fd,buf,1024);//read the message send by client
					if(n == 0)
					{
						cout << "Error!"<<endl;
						exit(-1);
					}
					buf[n] = '\0';
					if(n < 0)
						cout << "Read error" << endl;
					else if(!strcmp(buf, "exit"))
					{
						cout << "Connection closed" << endl;
						close(c_fd);
						flag = 0;
					}
					else
					{
						cout << "Client: "<< buf<<endl;
						write(c_fd,buf,n);//sent message back to client
					}
				}
			}
		}
    }

    return 0;
}

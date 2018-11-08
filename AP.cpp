#include <iostream>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <sys/select.h>
#include <openssl/md5.h>
#define MAX_TIME 10		//Max waiting time
#define MAXBUF 1024

int MyRead(int socket, char *s, int len = 1024, struct timeval *time = NULL);
int MyWrite(int socket, char *s, int len, struct timeval *time = NULL);
int Init(int port);
int Acception(int l_fd);
int RequestGet(int socket, int r, char *an);
void ANounceGenerate(char *s);
void CNounceReceive(int socket, int r, char *s);
void TKGenerate(char *md5 ,char *s1, char *s2, char *s3, int l1, int l2, int l3);
void decrypt(char *plaintext ,char *key, char *ciphertext);
void NounceClear(char *nounce);
void NounceUpdate(char *nounce);
void KeyGenetate(char *s, char *MAC, char *nounce, char *tk);
void PrintHex(char *s, int n);

using namespace std;
int main(int argc, char *argv[])
{
	int r = 0;
	int state;
    int l_fd, c_fd;
    char buf[1024];//content buff area
 	char MAC[] = "00:0c:29:db:e3:0d";
    int port;
	char an[17];
	char cn[17];
	char MasterKey[MAXBUF] = {0};
	char nounce[] = "00000000";
	char md5[17];
	char key[17];
	char plaintext[17];
	int mlen;
	int flag;
	int nfdp;
	fd_set rfds;
	fd_set wfds;
	struct timeval time; // 10s maximum delay.
    if (argc != 3)
    {
        printf("Usage with parameters [MasterKey] [port]\n");
        exit(1);
    }
    strcpy(MasterKey, argv[1]);
    port = atoi(argv[2]);


    l_fd = Init(port);
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
					break;
				}
				else
					state++;
				break;
			}
			case 1:
			{
				CNounceReceive(c_fd, r,cn);
				cout << "Receive CN: ";
				PrintHex(cn, 16);
				state++;
				break;
			}
			case 2:
			{
				TKGenerate(md5, an, cn, MasterKey, 16, 16, strlen(MasterKey));
				strcpy(buf, "ACK");
				time = {10, 0};
				mlen = MyWrite(c_fd, buf, 3, &time);
				if(mlen!=3)
				{
					cout << "Error"<<endl;
					exit(-1);
				}
				while(1)
				{
					time = {10, 0};
					mlen = MyRead(c_fd, buf, MAXBUF, &time);
					if(mlen > 0 && !strcmp("ACK", buf))
					{
						cout<<"Receive ACK from client: " <<buf<< endl;
						NounceClear(nounce);
						state++;
						break;
					}
					else
					{
						strcpy(buf, "ACK");
						time = {10, 0};
						mlen = MyWrite(c_fd, buf, 3, &time);
					}
				}
				break;
			}
			case 3:
			{
				cout << "MAC: " << MAC <<" TK: ";
				PrintHex(md5, 16);
				cout << endl;
				while(1)
				{
					time = {10, 0};
					mlen = MyRead(c_fd, buf, MAXBUF, &time);
					if(mlen <= 0)
					{
						cout<<"End"<<endl;
						state++;
						break;
					}
					else if(mlen == 3 && !strcmp(buf, "ACK"))
					{
						NounceClear(nounce);
						cout << "nounce reset!" << endl;
					}
					else
					{

						KeyGenetate(key, MAC, nounce, md5);
						cout << "nounce: " << nounce << " key: ";
						PrintHex(key, 16);
						cout<<"Reveive ciphertext: ";
						PrintHex(buf, mlen);
						decrypt(plaintext, key, buf);
						NounceUpdate(nounce);
						cout << "plaintext: " << plaintext << endl << endl;;
					}

				}
				
			}
			case 4:
			{
				flag = 0;
				break;
			}
		}
	}
	close(c_fd);
	close(l_fd);

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

int Acception(int l_fd)
{
    int c_fd;
	struct sockaddr_in c_in;//client address structure
    socklen_t client_len;
    char ipstr[128];
	cout<<"Waiting for connection."<<endl;
	client_len =  sizeof(c_in);
    c_fd = accept(l_fd,(struct sockaddr *)&c_in,&client_len);
    if(c_fd < 0)
    {
    	cout << "Acception error" << endl;
    	exit(-1);
    }
    cout << "Client IP: " << inet_ntop(AF_INET, &c_in.sin_addr.s_addr, ipstr, sizeof(ipstr)) << ". Port: " << ntohs(c_in.sin_port) << endl;
    return c_fd;
}
void ANounceGenerate(char *s)
{
	srand((int)time(0));
	int a = rand();
	int b = rand();
	int c = rand();
	int d = rand();
	*(int *)s = a;
	*(int *)(s+4) = b;
	*(int *)(s+8) = c;
	*(int *)(s+12) = d;
	s[16] = 0;
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

int RequestGet(int socket, int r, char *an)
{
	timeval time;
	cout << "Waiting for request"<<endl;
	char buf[MAXBUF];
	int mlen;
	mlen = MyRead(socket, buf, MAXBUF, NULL);
	if(mlen <= 0)
		return 0;
	buf[mlen] = 0;
	cout << "Receive message: "<< buf << endl;
	if(!strcmp(buf, "Authentication_Request"))
	{
		ANounceGenerate(an);
		mlen = MyWrite(socket, an, 16, &time);
		if(mlen != 16)
		{
			cout << "Write Error! " << mlen << endl;
			return 0;
		}
		else
		{
			cout<<"Send AN: ";
			PrintHex(an, 16);
		}
		return 1;
	}
	else
		return 0;
}

void CNounceReceive(int socket, int r, char *c)
{
	char t[MAXBUF];
	struct timeval time = {10, 0};
	if(MyRead(socket, t, MAXBUF,&time) != 16)
	{
		cout << "Error C" <<endl;
		exit(-1);
	}
	t[16] = 0;
	strncpy(c, t, 16);
	c[16] = 0;
}

void TKGenerate(char *md5 ,char *s1, char *s2, char *s3, int l1, int l2, int l3)
{
	MD5_CTX ctx;
	MD5_Init(&ctx);
	MD5_Update(&ctx, s1,l1);
	MD5_Update(&ctx, s2,l2);
	MD5_Update(&ctx, s3,l3);
	MD5_Final((unsigned char *)md5, &ctx);
}

void decrypt(char *plaintext ,char *key, char *ciphertext)
{
	for(int i = 0; i < 16; i++)
		plaintext[i] = key[i] ^ ciphertext[i];
	plaintext[16] = 0;
}

void NounceClear(char *nounce)
{
	for(int i = 0; i < 8; i++)
		nounce[i] = '0';
}

void NounceUpdate(char *nounce)
{
	int carry = 1;
	for(int i = 7; i >= 0; i--)
	{
		if(carry == 0)
		  return;
		else
		{
			if(nounce[i] == '9')
			{
				carry = 1;
				nounce[i] = '0';
			}
			else
            {
				nounce[i] = nounce[i] + 1;
                carry = 0;
            }
		}
	}
}


void KeyGenetate(char *s, char *MAC, char *nounce, char *tk)
{
	MD5_CTX ctx;
	MD5_Init(&ctx);
	MD5_Update(&ctx, MAC, strlen(MAC));
	MD5_Update(&ctx, nounce, strlen(nounce));
	MD5_Update(&ctx, tk, 16);
	MD5_Final((unsigned char *)s, &ctx);	
}

void PrintHex(char *s, int n)
{
	for(int i = 0; i < n; i++)
		printf("%02x ", (unsigned char)s[i]);
	cout << endl;
}
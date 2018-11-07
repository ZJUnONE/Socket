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

using namespace std;

#define MAXBUF 1024
int ConnectionGenerate(string IP, int port);
int SendAuth(int socket, char buf[]);
void TKGenerate(char *md5 ,char *s1, char *s2, char *s3, int l1, int l2, int l3);
void CNounceGenerate(char *s); //8 bytes
int MyRead(int socket, char *s, int len = 1024, struct timeval *time = NULL);
int MyWrite(int socket, char *s, int len, struct timeval *time = NULL);
void KeyGenetate(char *s, char *MAC, char *nounce, char *tk);
void encrypt(char *ciphertext ,char *key, char *plaintext);
void NounceClear(char *nounce);
void NounceUpdate(char *nounce);
void PrintHex(char *s, int n);

int main(int argc, char *argv[]){
	char MasterKey[MAXBUF] = {0};
    string IP;
    int PORT;
    char MAC[] = "00:0c:29:db:e3:0d";
    char nounce[] = "00000000";
    char filepath[MAXBUF] = {0};
    char buf[MAXBUF];
    int s_fd, state = 0;
    char an[17], cn[17];
    int r;
    char md5[17];
    char key[17];
    char ciphertext[17];
    char t[5];
    int mlen;
    int i;
    struct timeval time = {10, 0};
    if (argc != 5)
    {
        cout << "Usage: [IP] [port] [MasterKey] [file name]" <<endl ;
        exit(1);
    }
    IP = argv[1];
    PORT = atoi(argv[2]);
    strcpy(MasterKey, argv[3]);
    strcpy(filepath, argv[4]);

    ifstream file(filepath);
    if(!file)
    {
    	cout << "Open file error" <<endl;
    	exit(-1);
    }
    else
    	cout << "Open file succeeded" <<endl;
    sleep(1);
    s_fd = ConnectionGenerate(IP, PORT);
    sleep(1);
    while(1)
    {
    	switch(state)
    	{
    		case 0:
    		{
    			mlen = SendAuth(s_fd, buf);
    			//cout << mlen << endl;
    			if(mlen == 16)
    			{
    				//r = *(int *)(buf + 4);
    				strcpy(an, buf) ;
    				cout << "Authentication succeeded!" << endl << "Receive AN: ";
    				PrintHex(an, 16);
    				state++;
    				sleep(1);
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
    			time = {10, 0};
    			CNounceGenerate(cn);
    			//strcpy(cn, "c123456789012345");
    			TKGenerate(md5, an, cn, MasterKey, 16, 16, strlen(MasterKey));
    			md5[16] = 0;
    			
    			if( (mlen = MyWrite(s_fd, cn, 16, &time)) != 16)
    			{
    				cout << "Ms2 send error: " << mlen << endl;
    				exit(-1);
    			}
    			else
    			{
    				sleep(1);
					cout << "Send CN: ";
					PrintHex(cn, 16);
					state++;
    			}

    			break;
    		}
    		case 2:
    		{
    			time = {10, 0};
    			mlen = MyRead(s_fd, buf, MAXBUF, &time);
    			sleep(1);
    			if(mlen  <=0)
    			{
    				cout <<"Error" <<endl;
    				exit(-1);
    			}
    			if(!strcmp(buf, "ACK"))
    			{
    				cout <<"Receive ACK" <<endl;
    				strcpy(buf, "ACK");
    				time = {10, 0};
    				if(MyWrite(s_fd, buf, 3, &time) == 3)
                        cout << "ACK sent" << endl;
                    else
                    {
                        cout << "ACK Error!" << endl;
                        exit(-1);
                    }
                    NounceClear(nounce);
    				sleep(1);
    				state++;
    				break;
    			}
    		}
    		case 3:
    		{
                cout << "MAC: " << MAC <<" TK: ";
                PrintHex(md5, 16);
    			cout << "-------Transferring encrypted data-------" << endl;
    			while(1)
    			{
    				time = {0, 0};
    				if(MyRead(s_fd, buf, MAXBUF, &time) == 3)
    					if(!strcmp(buf, "ACK"))
    					{
    						cout << "nounce reset!" << endl;
    						MyWrite(s_fd, "ACK", 3, NULL);
    						NounceClear(nounce);
    					}
    				
    				if(!(file >> buf))
    					break;
    				for(i = strlen(buf); i < 16; i++)
    					buf[i] = 0;
    				KeyGenetate(key, MAC, nounce, md5);
    				cout << "nounce: " << nounce << " key: ";
    				PrintHex(key, 16);
    				encrypt(ciphertext, key, buf);
    				NounceUpdate(nounce);
    				time = {10, 0};
    				if( (mlen = MyWrite(s_fd, ciphertext, 16, &time)) !=16)
    				{
    				
    					cout << "Error" << endl;
    					exit(-1);
    				}
    				else
    				{
    					cout << "plaintext: " << buf << endl << "Sent ciphertext :" ;
    					PrintHex(ciphertext, 16);
    					cout << endl;
    				}

    				sleep(1);
    			}
    			cout << "-------Transferring done-------" << endl;
    			state++;
    			break;
    		}
    		case 4:
    		{
    			cout << "All succeeded!" <<endl;
    			file.close();
    			close(s_fd);
    			exit(0);
    		}
    	}
    }
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

int SendAuth(int socket, char buf[])
{	
	struct timeval time;
	char t[MAXBUF] = "Authentication_Request";
	time = {10,0};
	int mlen = MyWrite(socket, t,strlen(t), &time);
    cout << "Send authentication" << endl;
	sleep(1);
	if(mlen != strlen(t))
	{
		cout << "Write error"<<endl;
		exit(-1);
	}
	time = {10,0};
	mlen = MyRead(socket, buf, MAXBUF, &time);
	if(mlen <= 0)
		return -1;
	buf[mlen]=0;
	return mlen;
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

void CNounceGenerate(char *s)
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

void KeyGenetate(char *s, char *MAC, char *nounce, char *tk)
{
	MD5_CTX ctx;
	MD5_Init(&ctx);
	MD5_Update(&ctx, MAC, strlen(MAC));
	MD5_Update(&ctx, nounce, strlen(nounce));
	MD5_Update(&ctx, tk, 16);
	MD5_Final((unsigned char *)s, &ctx);	
}

void encrypt(char *ciphertext ,char *key, char *plaintext)
{
	for(int i = 0; i < 16; i++)
		ciphertext[i] = key[i] ^ plaintext[i];
	ciphertext[16] = 0;
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

void PrintHex(char *s, int n)
{
	for(int i = 0; i < n; i++)
		printf("%02x ", (unsigned char)s[i]);
	cout << endl;
}
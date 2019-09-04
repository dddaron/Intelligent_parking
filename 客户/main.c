#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <iconv.h>
#include "socket.h"

client_packet client_1;

void sdphoto(int client_sd)
{
	printf("sending photo\n");
	int fd,cnt;
	client_packet head;
	struct stat buf;
	char tmpfile[128] = {0},r_buf[1024]={0};

	sprintf(tmpfile,"%s%s","1",".jpg");
	fd = open(tmpfile,O_RDWR);	
	stat(tmpfile,&buf);
	head.pic_size = buf.st_size;
	head.client_num = 1;
	
	send(client_sd,&head,sizeof(client_packet),0);	
	printf("filelen is:%d\n",head.pic_size);

	memset(r_buf,0,sizeof(r_buf));
	while((cnt = read(fd,r_buf,1024))!=0)
	{
		send(client_sd,r_buf,cnt,0);
		memset(r_buf,0,sizeof(r_buf));
	}
	memset(&head,0,sizeof(client_packet));
				
//	head.pic_size= 0;
//	head.client_num = 0;
//	send(client_sd,&head,sizeof(client_packet),0);

	printf("sdphoto is over\n");
	memset(r_buf,0,sizeof(r_buf));
	recv(client_sd,r_buf,9,0);
	printf("r_buf:%s\n",r_buf);
	
}


//读语音文件
void rdvoice(int client_sd)
{

		int fd;
		server_packet head;
		recv(client_sd,&head,sizeof(server_packet),0);
		//读到都是0就断开
		
		fd = open("1.wav",O_RDWR|O_CREAT|O_TRUNC,0666);	
		if(fd < 0)
		{
			perror("open file fail\n");
			exit(1);
		}
		int cnt,writenum = 0;
		char r_buf[1024];
		writenum = head.voice_size;
		
		printf("writenum is:%d\n",writenum);
		while(writenum > 0)
		{
			memset(r_buf,0,sizeof(r_buf));
			if(writenum >= 1024)
				cnt = recv(client_sd,r_buf,sizeof(r_buf),0);
			else
				cnt = recv(client_sd,r_buf,writenum,0);
			if(cnt == 0)
			{
				printf("data is complete\n");
				//close(client_sd);
				break;
			}
			write(fd,r_buf,cnt);
			writenum -= cnt;
		}	
		printf("writenum is:%d\n",writenum);
		close(fd);
		memset(&head,0,sizeof(server_packet));
		memset(r_buf,0,sizeof(r_buf));
		//strcpy(r_buf,"rdvoice");
		//send(client_sd,r_buf,strlen(r_buf),0);
	
}

void rdtxt(int client_sd)
{
	
		int fd;
		server_packet head;
		read(client_sd,&head,sizeof(server_packet));
		//读到都是0就断开
		
		fd = open("car.txt",O_RDWR|O_CREAT|O_TRUNC,0666);	
		if(fd < 0)
		{
			perror("open file fail\n");
			exit(1);
		}
		int cnt,writenum = 0;
		char r_buf[1024];
		writenum = head.voice_size;
		
		printf("writenum is:%d\n",writenum);
		while(writenum > 0)
		{
			memset(r_buf,0,sizeof(r_buf));
			if(writenum >= 1024)
				cnt = read(client_sd,r_buf,sizeof(r_buf));
			else
				cnt = read(client_sd,r_buf,writenum);
			if(cnt == 0)
			{
				printf("data is complete\n");
				close(client_sd);
				break;
			}
			send(fd,r_buf,cnt,0);
			writenum -= cnt;
		}	
		printf("writenum is:%d\n",writenum);
		close(fd);
		memset(&head,0,sizeof(server_packet));
		memset(r_buf,0,sizeof(r_buf));
		//strcpy(r_buf,"rdtxt");
		//send(client_sd,r_buf,strlen(r_buf),0);
}

void connect_yun(char* jpg_path)
{
	int client_sd;
	char r_buf[128] = {0};
	
	client_sd  = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(client_sd < 0)
	{
		perror("socket");
		exit(1);
	}
	printf("socket create success,client socket fd:%d\n",client_sd);
	//takeCamPhoto("./1.jpg");
	client_1.client_num = 2;
	struct sockaddr_in  server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(8090);
	server_addr.sin_addr.s_addr = inet_addr("47.103.42.149");
//	server_addr.sin_addr.s_addr = inet_addr("192.168.1.142");
	

	int ret = connect(client_sd,(struct sockaddr *)&server_addr,sizeof(server_addr));
	if(ret<0)
	{
		perror("connet fail\n");
		exit(2);	
	}

	send(client_sd,&client_1,sizeof(client_packet),0);	
	printf("send\n");
	sleep(3);
	recv(client_sd,r_buf,3,0);
	printf("r_buf = %s\n",r_buf);
	if(strcmp(r_buf,"rec") != 0)
	{
		printf("receive rec failed\n");
		exit(1);
	}	
		
		

	sdphoto(client_sd);
		
	rdvoice(client_sd);
	sleep(1);
	rdtxt(client_sd);

	printf("data conver over\n");
	system("mplayer 1.wav");

	close(client_sd);	
}

int main()
{
	//takeCamPhoto("2.jpg");
	connect_yun("1.jpg");
	/*int listen_sd,client_fd;
	listen_sd  = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(listen_sd < 0)
	{
		perror("socket");
		exit(1);
	}
	 int opt = 1;
    	setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
		printf("socket create success,server socket fd:%d\n",listen_sd);

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(8090);//little to big
	server_addr.sin_addr.s_addr = htons(INADDR_ANY);	

	bind(listen_sd,(struct sockaddr *)&server_addr,sizeof(server_addr));
	
	listen(listen_sd,100);
	struct sockaddr_in client_addr;
	socklen_t addrlen;
	addrlen = sizeof(client_addr);
	client_fd = accept(listen_sd,(struct sockaddr *)&client_addr,&addrlen); 
	printf("connect success\n");

	send(client_fd,"open",4,0);
	sleep(2);
	send(client_fd,"close",5,0);*/
	return 0;
}

#include<iostream>
#include<unistd.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<stdio.h>
#include<errno.h>
#include<signal.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<MD5.h>
using namespace std;



class userinfo
{
private:
	string id;
	string username;
public:	
	userinfo(char *m_id,char *name)
	{
		id(m_id);
		username(name);
	}
};
//REGISTERED=0,LOGIN,UPLOAD_F,UPLOAD_S,DOWNLOAD,ADDFRIEND,SHAREDTOFRIEND,SNOPSHOT
class start_process
{
public:
	void RecvFriendAndFileList();
	void deal_registered();
	void RecvRetId();
	void error_login();
	void deal_login();
	void start();
	void file_start();
	void deal_upload();


	void deal_resumes_transmission(char *path);
	void upload_file_to_server(char *path);
public:
	void run()
	{
		start();
		file_start();
	}
	start_process(int msockfd)
	{
		sockfd=msockfd;
		pipe(pfd);
	}
private:
	userinfo m_user;
	int sockfd;
	int pfd[2];//splice用管道，记得初始化
};



char *mystrtok(char *tmp,char *s)
{
	char *p;
	char *temp=NULL;
	p = strtok(tmp, s);
	while (p != NULL)
	{
		temp = p;
		p = strtok(NULL, s);
	}
	return temp;
}
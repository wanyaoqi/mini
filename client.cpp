/*
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
#include<md5c.h>
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
	void RecvFriendAndFileList();
	void deal_registered();
	void RecvRetId();
	void error_login();
	void deal_login();
	void start();
	void file_start();
	void deal_upload();
public:
	void run()
	{
		start();
		file_start();
	}
	
private:
	userinfo m_user;
	int sockfd;
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
*/

#include"client.h"
#include"MD5.h"

#define FILEMAX 20000000000

class userinfo;
class start_process;
char *mystrtok(char *tmp,char *s);

enum tag_login{REGISTERED=0,LOGIN,UPLOAD_F,UPLOAD_S,DOWNLOAD,ADDFRIEND,SHAREDTOFRIEND,SNOPSHOT};
enum ser_ret{REGISTERED_RET=0,LOGIN_SUCCESS,FRIEND_LIST,FILE_LIST,ERROR_PASSWD,RESUME,SECONDUPLOADSUCCESS,NOT_HAVE_THIS_FILE,FILENOFIND,STARTLOADFILE};


void getmd5(char *buff,char *path);




void start_process::deal_shared()
{
	cout<<"shared to friend id:"<<endl;
	char fid[20]={0};
	char filename[30]={0};
	cin.getline(fid,20);
	cout<<"shared filename:"<<endl;
	cin.getline(filename,30);
	char buff[50]={0};
	strncpy(buff,fid,20);
	strncpy(buff,filename,30);
	int tag=SHAREDTOFRIEND;
	send(sockfd,&tag,4,0);
	send(sockfd,buff,50,0);
}

//////////////////////////////////////////////////////////////

void start_process::deal_addfriend()
{
	cout<<"input your friend id:"<<end;
	char buff[30]={0};
	cin.getline(buff,30);
	int tag = ADDFRIEND;
	send(sockfd,&tag,4,0);
	send(sockfd,buff,30,0);
}
///////////////////////////////////////////////////////////////
void start_process::deal_snopshot()
{
	int tag=SNOPSHOT;
	send(sockfd,&tag,4,0);
}
///////////////////////////////////////////////////////////////
void start_process::deal_download()
{
	char buff[30]={0};
	cout<<"input what you want download:"<<endl;
	cin.getline(buff,30);
	int tag=DOWNLOAD;
	send(sockfd,&tag,4,0);
	send(sockfd,buff,30,0);
	
	recv(sockfd,&tag,4,0);
	if(tag==FILENOTFIND)
	{
		cout<<"NOT HAVE THIS FILE"<<endl;
	}
	else if(tag == STARTLOADFILE)
	{
		cout<<"input where you want store(include filename):"<<endl;
		char pathname[50]={0};
		cin.getline(pathname,50);
		int filefd=open(pathname,O_WRONLY|O_CREAT,0600);
		
		int filesize=0;
		recv(sockfd,&size,4,0);
		
		while(filesize>0)
		{
			ret=splice(sockfd,NULL,pfd[1],NULL,32768,SPLICE_F_MORE|SPLICE_F_MOVE);
			filesize-=ret;
			if(ret>0)
			{
				splice(pfd[0],NULL,filefd,0,ret,SPLICE_F_MORE|SPLICE_F_MOVE);
			}		
		}		
	}
}
//REGISTERED_RET=0,LOGIN_SUCCESS,FRIEND_LIST,FILE_LIST,ERROR_PASSWD,RESUME,SECONDUPLOADSUCCESS,NOT_HAVE_THIS_FILE,FILENOFIND,STARTLOADFILE
void start_process::deal_resumes_transmission(char *path)
{
	int tag=UPLOAD_S;
	send(sockfd,&tag,4,0); 
	int size=0;
	recv(sockfd,&size,4,0);
	int filefd=open(path,O_RDONLY);
	assert(filefd>0);
	lseek(filefd,size,SEEK_SET);
	struct stat stat_buf;
	fstat(filefd,&stat_buf);
	int fsize=stat_buf.st_size;
	fsize-=size;
	send(sockfd,&fsize,4,0);
	int ret=0;
	while(fsize<0)
	{
		ret=sendfile(filefd,sockfd,NULL,FILEMAX);
		fsize-=ret;
	}
}
void start_process::upload_file_to_server(char *path)
{
	int tag=UPLOAD_S;
	send(sockfd,&tag,4,0); 
	int filefd=open(path,O_RDONLY);
	assert(filefd>0);
	struct stat stat_buf;
	fstat(filefd,&stat_buf);
	int fsize=stat_buf.st_size;
	send(sockfd,&fsize,4,0);

	int ret=0;
	while(fsize<0)
	{
		ret=sendfile(filefd,sockfd,NULL,FILEMAX);
		fsize-=ret;
	}
}
void start_process::deal_upload()
{
	char buff[50]={0};
	cout<<"input path of file: ";
	cin.getline(buff,50);
	char path[50]={0};
	strcpy(path,buff);
	char md5[17]={0};
	getmd5(buff,md5);
	char *p=my_strtok(buff,"/");
	char sendmsg[40]={0};
	strcpy(sendmsg,md5);
	strcpy(sendmsg+20,p);
	int tags=UPLOAD_F;
	send(sockfd,&tags,4,0)
	send(sockfd,sendmsg,40,0);
	int tag=0;
	recv(sockfd,&tag,4,0);
	switch(tag)
	{
		case SECONDUPLOADSUCCESS:
			cout<<"UPLOAD SUCCESS"<<endl;
			break;
		case RESUME:
			deal_resumes_transmission(path);
			break;
		case NOT_HAVE_THIS_FILE:
			upload_file_to_server(path);
			break;
		default:
			break;			
	}
}
///////////////////////////////////////////////
void start_process::file_start()
{
	while(1)
	{
		cout<<"1.upload file"<<endl;
		cout<<"2.download file"<<endl;
		cout<<"3.snopshot"<<endl;
		cout<<"4.add friend"<<endl;
		cout<<"5.shared file"<<endl;
		cout<<"6.exit"<<endl;
		cout<<"input your choice"<<endl;
		int tag=0;		
		cin>>tag;
		switch(tag)
		{
			case 1:	
				deal_upload();
				break;
			case 2:
				deal_download();
				break;
			case 3:
				deal_snopshot();
				break;
			case 4:
				deal_addfriend();
				break;
			case 5:
				deal_shared();
				break;
			case 6:
				exit(1);
			default:
				break;				
		}
	}
	
}
///////////////////////////////////////////////
void start_peocess::start()
{
	cout<<"1. login"<<endl;
	cout<<"2. registered"<<endl;
	cout<<"3. exit"<<endl;
	int tag;
	cin>>tag;
	switch(tag)
	{
		case 1:
			deal_login();
			break;
		case 2:
			deal_registered();
			break;
		case 3:
			exit(1);
			break;
		default:
			break;
	}
	int revc_tag;
	recv(sockfd,&recv_tag,4,0);
	//REGISTERED_RET=0,FRIEND_LIST,FILE_LIST,ERROR_PASSWD,RESUME,LOGIN_SUCCESS
	switch(revc_tag)
	{
		case LOGIN_SUCCESS:
			RecvFriendAndFileList();
			break;
		case REGISTERED_RET:
			RecvRetId();
			break;
		case ERROR_PASSWD:
			error_login();
		default:
			break;
	}
}
///////////////////////////////////////////////////////////
void start_process::deal_login()
{
	char buff[30] = { 0 };
	cout<<"input id: ";
	cin.getline(buff, 15);
	cout<<"input passwd: "<<endl;
	cin.getline(buff + 15, 15);
	m_user.id(buff);
	int send_tag=LOGIN;
	send(sockfd,&send_tag,4,0);
	send(sockfd,buff,30,0);
}
void start_peocess::deal_registered()
{
	char buff[30]={0};
	cout<<"input name: ";
	cin.getline(buff, 15);
	cout<<"input passwd: ";
	cin.getline(buff + 15, 15);
	int send_tag=REGISTERED;
	send(sockfd,&send_tag,4,0);
	send(sockfd,buff,30,0);
}
void start_process::RecvFriendAndFileList()
{
	cout<<"login success!"<<endl;
	int friendsize=0;;
	recv(sockfd,&friendsize,4,0);
	char *friend_list=new char[15*friendsize];
	assert(friend_list!=NULL);
	recv(sockfd,friend_list,15*friendsize,0);
	assert(file_list!=NULL);
	int filenumber=0;
	recv(sockfd,&filenumber,4,0);
	char *file_list=new char[20*filenumber];
		
	//////cout<<jiuchadayinyibian
		
}
void start_process::RecvRetId()
{
	char buff[15]={0};
	recv(sockfd,buff,15,0);
	cout<<"this is your id: ";
	cout<<buff<<endl;
	start();
}
void start_process::error_login()
{
	cout<<"id or passwd error"<<endl;
	start();
}
int main()
{
	int sockfd=socket(AF_INET,SOCK_STREAM,0);
	sockaddr_in saddr;
	memset(&saddr,0,sizeof(saddr));
	saddr.sin_family=AF_INET;
	saddr.sin_port=htons(6500);
	saddr.sin_addr.s_addr=inet_addr("127.0.0.1");
	int res=connect(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
	assert(res!=-1);
	
	start_process start(sockfd);
	start.run();
	return 0;
}
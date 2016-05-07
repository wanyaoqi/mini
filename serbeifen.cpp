#include"myprocesspool.h"

#include<iostream>

using namespace std;



#define FILEMAX 20000000000



////////////////////////////////////////////////////////////////////////////////

//          /home/wyq/filesys   /home/wyq/userfile      /home/wyq/snopshotfile   

enum tag_login{REGISTERED=0,LOGIN,UPLOAD_F,UPLOAD_S,DOWNLOAD,ADDFRIEND,SHAREDTOFRIEND,SNOPSHOT};

enum ser_ret{REGISTERED_RET=0,LOGIN_SUCCESS,FRIEND_LIST,FILE_LIST,ERROR_PASSWD,RESUME,SECONDUPLOADSUCCESS,NOT_HAVE_THIS_FILE,FILENOFIND,STARTLOADFILE};



class file_property

{

public:

	string m_filename;//?¦Ì¨ª3???t??

	string m_userfilename;//¨®??¡ì?D???t??

	string m_md5;

	int al_size;//¨°??-¨¦??????t?¨®D?

	int total_size;//¡Á¨¹?¨®D?

public:

	file_property(int m_total=0,string &filename,int m_already)

	{

		m_filename=filename;

		al_size=m_already;

		total_size=m_total;

	}

};







class user

{

public:

	string id;//¨®??¡ìid

	bool states;//¡Á?¨¬?

	file_property m_file;//???t¨º?D?

	int pfd[2];	//splice¨®?1¨¹¦Ì¨¤

};





class transfer

{

public:

	transfer(){};

	~transfer(){};

	void process();

	void init(int epollfd,int sockfd,int pipefd);

	//void send_friend_list(char *id);

	//void send_file_list(char *id);

	//void deal_registered();

private:

	static int m_epollfd;

	int m_connfd;

	//struct sockaddr_in client;

	user m_user;

	static int mps_pipe;





/////////////////////////////////////////////////////////////////////////////////////////////







/////////////////////////////////////////////////////////////////////////////////////////////

void deal_registered()

{

	char buff[30]={0};

	int res=recv(m_connfd,buff,30,0);//?¨®¨º?30??¡Á??¨²¡ê?¨®??¡ì??o¨ª?¨¹??

	assert(res==30);

	string name(buff);//?¨¨¨¨???¡Á?	

	string passwd(buff+15);//¨¨??¨¹??

	char id[15]={0};

	bool b_tag=add_registered(name.c_str(),passwd.c_str(),id);//¨¨?ID

	int tag=REGISTERED_RET;

	string path("/home/wyq/userfile/");

	string fileid(id);

	path+=fileid;

	mkdir(path.c_str(),0775);

	send(m_connfd,&tag,4,0);

	res=send(m_connfd,id,15,0);//¡¤¦Ì??ID

	assert(res>0);

	

	//2?¨¨?¨®??¡ì¡À¨ª

}

//////////////////////////////////////////////////////////////////////////////////////////////

void send_friend_list(char *id)

{

	int i=0;//i¡À¨ª¨º??¨¤¨¦¨´??o?¨®?i

	char **friend_list=get_friend_list(id,i);

	//int tag=FRIEND_LIST;

	//send(m_connfd,&tag,4,0);

	send(m_connfd,&i,4,0);

	send(m_connfd,friend_list,i*15,0);

	//¡¤¡é?¨ª¨ª¨º¨®???¨º¨ª¡¤?

}

void send_file_list(char *id)

{

	int i=0;

	char **file_list=get_file_list(id,i);

	//int tag=FILE_LIST;

	//send(m_connfd,&tag,4,0);

	send(m_connfd,&i,4,0);

	send(m_connfd,file_list,i*20,0);

}

void send_offmsg(char *id)

{

	int i=0;

	char **offmsg_list=get_offmsg(id,i);

	send(m_connfd,&i,4,0);

	send(m_connfd,offmsg_list,i*15,0);

}

void deal_login()

{

	char buff[30]={0};

	int res=recv(m_connfd,buff,30,0);//¨®??¡ì??o¨ª?¨¹??

	assert(res==30);

	string id(buff);

	string passwd(buff+15);

	bool logintag=user_login(id.c_str(),passwd.c_str());

	if(bool)

	{

		int login_success=LOGIN_SUCCESS;

		m_user.id=id;

		m_user.states=true;

		send_friend_list(id.c_str());

		send_file_list(id.c_str());

		//send_offmsg(id.c_str());

		//¦Ì?¨¨?o¨®1?¡Á¡é??????

		addfd_close(m_epollfd,m_connfd);

		int tag=ONLINEMSG;

		char idbuff[15]={0};

		strcpy(idbuff,id.c_str());

		send(mps_pipe,idbuf,15,0);

	}

	else

	{

		int tag=ERROR_PASSWD;

		send(m_connfd,&tag,4,0);

	}

}



///////////////////////////////////////////////////////////////////////////////////////////

//          /home/wyq/filesys   /home/wyq/userfile      /home/wyq/snopshotfile   

void deal_upload_f()

{

	char buff[40]={0};

	int res=recv(m_connfd,buff,40,0);

	assert(ret==40);	

	string md5(buff);

	string filename(buff+20);

	bool md5_tag=judge_md5(md5.c_str());

	string name_sys("/home/wyq/filesys/");//???t?¦Ì¨ª3?¡¤??

	name_sys+=md5;//

	string name_user("/home/wyq/userfile/");

	name_user+=m_user.id;

	name_user+='/';

	name_user+=filename;

	

	if(md5_tag)

	{

		int tag=SECONDUPLOADSUCCESS;

		send(m_connfd,&tag,4,0);

		res=link(name_sys.c_str(),name_user.c_str());

		assert(res!=-1);

		//?¨¹D?¨ºy?Y?a¡ê?

	}

	else

	{

		m_user.m_file.m_md5=md5;

		m_user.m_file.m_userfilename=name_user;

		int size=get_resume(m_user.id.c_str(),filename.c_str());

		if(size>0)

		{

			int tag=RESUME;

			send(m_connfd,&tag,4,0);

			send(m_connfd,&size,4,0);

			m_user.m_file.m_filename=name_sys;

			m_user.m_file.al_size=size;

		}

		else

		{

			int tag=NOT_HAVE_THIS_FILE;

			send(m_connfd,&tag,4,0);

			m_user.m_file.m_filename=name_sys;

			m_user.m_file.al_size=0;

		}		

	}

}

void deal_upload_s()

{

	int filesize=0;

	recv(m_connfd,&filesize,4,0);

	int size=filesize;

	filesize-=m_user.m_file.al_size;

	int ret=0;

	int new_fd=open(m_user.m_file.m_filename.c_str(),O_WRONLY|O_CREAT,0600);

	lseek(new_fd,0,SEEK_END);

	while(ret>0&&filesize>0)

	{

		ret=splice(m_connfd,NULL,m_user.pfd[1],NULL,32768,SPLICE_F_MORE|SPLICE_F_MOVE);

		filesize-=ret;

		if(ret>0)

		{

			splice(m_user.pfd[0],NULL,new_fd,0,ret,SPLICE_F_MORE|SPLICE_F_MOVE);

		}		

	}

	//?¨¹D?¨ºy?Y?aupload(id,name,size,md5)

	if(filesize==0)

	{

		link(m_user.m_file.m_filename.c_str(),m_user.m_file.m_userfilename.c_str());

		upload(m_user.id.c_str(),m_user.m_file.m_filename.c_str(),size,m_user.m_file.m_md5.c_str());//??¡Á??¨¦¨°?2?¨°a

	}

	else

	{

		add_resume(m_user.id.c_str(),m_user.m_file.m_filename.c_str(),size-filesize);//?¨®??¦Ì?/////////

	}

	

}

//////////////////////////////////////////////////////////////////////////////////

//          /home/wyq/filesys   /home/wyq/userfile      /home/wyq/snopshotfile   

void deal_download()

{

	char buff[30]={0};

	recv(m_connfd,buff,30,0);

	string filename(buff);

	string pathfile("/home/wyq/userfile/");

	pathfile+=m_user.id;

	pathfile+=filename;

	int filefd=open(pathfile.c_str(),O_RDONLY);

	if(fd<0)

	{

		int tag=FILENOFIND;

		send(m_confd,&tag,4,0);

	}

	else

	{

		int tag=STARTLOADFILE;

		struct stat stat_buf;

		fstat(filefd,&stat_buf);

		int size=stat_buf.st_size;

		send(m_connfd,&tag,4,0);

		send(m_connfd,&size,4,0);

		while(size>0)

		{

			sendfile(m_connfd,filefd,NULL,FILEMAX);

			size-=FILEMAX;

		}	

	}

}

///////////////////////////////////////////////////////////////////////////////////

void deal_addfriend()

{

	char buff[30]={0};

	int res=recv(m_connfd,buff,30,0);

	assert(res==30);

	string friendid(buff);

	

	int tag=JUDGEONLINE;

	

	add_friend(m_user.id.c_str(),friendid.c_str());//?¡À?¨®?¨®¨¦?¨¢?

}

//////////////////////////////////////////////////////////////////////////////////////

//          /home/wyq/filesys   /home/wyq/userfile      /home/wyq/snopshotfile 

void deal_snopshot()

{

	//char **tmp=get_md5(m_user.id.c_str());

	string path("/home/wyq/userfile/");

	path+=id;

	mkdir(path.c_str(),0775);

	DIR *user_file=opendir(path.c_str());

	assert(user_file!=NULL);

	struct dirent*entry;

	struct stat statbuf;

	string snopshotname("/home/wyq/snopshotfile/");

	snopshotname+=m_user.id;

	snopshotname+="/";

	char *sntime=add_snopshot(m_user.id.c_str());///////

	string snoptime(sntime);

	snopshotname+=snoptime;

	string path("/home/wyq/userfile/");

	path+=m_user.id;

	while(entry=readdir(user_file)!=NULL)

	{

		string tmp=snopshotname;

		string fname(entry.d_name);

		tmp+=fname;

		link(path.c_str(),tmp.c_str());

	}

}



void deal_shared()

{

	char buff[50]={0};

	string fid(buff);

	string filename(buff+20);

	string pathu("/home/wyq/userfile/");

	pathu+=m_user.id;

	pathu+="/";

	pathu+=filename;

	string pathf("/home/wyq/userfile/");

	pathf+=fid;

	pathf+="/";

	pathf+=filename;

	link(pathu.c_str(),pathf.c_str());

}



};



int transfer::m_epollfd=0;

int transfer::mps_pipe=0;









void transfer::init(int epollfd,int sockfd,int pipefd)

{

	m_epollfd=epollfd;

	m_connfd=sockfd;

	mps_pipe=pipefd;

}

void transfer::process()

{

	int tag;

	int res=recv(m_connfd,&tag,4,0);

	assert(res==4);

	switch(tag)

	{

		case REGISTERED:

			deal_registered();

			break;

		case LOGIN:

			deal_login();

			break;

		case UPLOAD_F:

			deal_upload_f();

			break;

		case UPLOAD_S:

			deal_upload_s();

			break;

		case DOWNLOAD:

			deal_download();

			break;

		case SHAREDTOFRIEND:

			deal_shared();

			break;

		case ADDFRIEND:

			deal_addfriend();

			break;

		case SNOPSHOT:

			deal_snopshot();

			break;

		default:

			break;

	}

}





































int main(int argc,char *argv[])

{

	if(argc<=2)

	{

		printf("argument error\n");

		return 1;

	}

	const char *ip=argv[1];

	int port=atoi(argv[2]);

	int listenfd=socket(AF_INET,SOCK_STREAM,0);

	assert(listenfd>=0);

	int ret=0;

	struct sockaddr_in address;

	memset(&address,0,sizeof(address));

	address.sin_family=AF_INET;

	address.sin_port=htons(port);

	saddr.sin_addr.s_addr=inet_addr(ip);

	int res=bind(listenfd,(struct sockaddr*)&address,sizeof(address));

	assert(res!=-1);

	res=listen(listenfd,5);

	assret(res!=-1);

	processpool<transfer> *pool=processpool<transfer>::create(listenfd);

	if(pool)

	{

		pool->run();

		delete pool;

	}

	close(listenfd);

}
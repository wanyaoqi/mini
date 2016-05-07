#ifndef _PROCESSPOOL_
#define _PROCESSPOOL_
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



class process
{
public:
	pid_t mpid;
	int m_pipefd[2];
public:
	process():mpid(-1){}		
};
///////////////////////////////////////////////////////////////////////
template<class T>
class processpool
{
private:
	processpool(int listenfd,int process_number=8);
public:
	static processpool<T> *create(int listenfd,int process_number=8)
	{
		if(!m_instance)
		{
			m_instance= new processpool<T>(listenfd,process_number);
		}
		return m_instance;
	}
	~processpool()
	{
		delete m_sub_process[];
	}
	void run();
private:
	void setup_single_pipe();
	void run_parent();
	void run_child();
private:
	static const int MAX_PROCESS_NUMBER=16;//最大进程数
	static const int USER_PER_PROCESS=65533;//每个进程的最大用户数
	static const int MAX_EVENT_NUMBER=10000;//epoll最大能处理的事件数量
	int m_process_number;//进程池内进程的数目
	int m_idx;//子进程在进程池中的序号
	int m_epollfd;//每个进程都有一个epoll事件表用m_epollfd标识
	int m_listenfd;//监听的socket
	int m_stop;//标明这个进程是否继续运行
	process *m_sub_process;//保存子进程的描述信息
	static processpool<T> *m_instance;//进程池的静态实例
	static set<string> m_user_state;
};
template<class T>
processpool<T> *processpool<T>::m_instance=NULL;
template<class T>
set<string> processpool<T>::m_user_state=set<string>();   
//////////////////////////////////////////////////////////////////////
static void addfd_in(int epollfd,int fd)
{
	epoll_event event;
	event.data.fd=fd;
	event.events=EPOLLIN|EPOLLET;
	epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
}
static void addfd_close(int epollfd,int fd)
{
	epoll_event event;
	event.data.fd=fd;
	event.events=EPOLLRDHUP|EPOLLET;
	epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
}
static void removefd(int epollfd,int fd)
{
	epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,0);
	close(fd);
}
//////////////////////////////////////////////////////////////////////

template<class T>
processpool<T>:: processpool(int listenfd, int process_number):m_listenfd(listenfd),m_process_number(process_number),m_idx(-1),m_stop(false)
{
	assert((process_number>0)&&(process_number<=MAX_PROCESS_NUMBER));

	m_sub_process= new process[process_number];
	assert(m_sub_process);

	for(int i=0;i<process_number;++i)
	{
		int ret=sockpair(PF_UNIX,SOCK_STREAM,0,m_sub_process[i].m_pipefd);
		assert(ret==0);

		m_sub_process[i].m_pid=fork();//
		assert(m_sub_process[i].m_pid>=0);
		
		
		if(m_sub_process[i].m_pid>0)
		{
			close(m_sub_process[i].m_pipefd[1]);
			continue;
		}
		else
		{
			close(m_sub_process[i].m_pipefd[0]);
			m_idx=i;
			break;
		}
	}
}	
///////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
void processpool<T>::run()
{
	if(m_idx!=-1)
	{
		run_child();
		return ;
	}
	run_parent();
}
///////////////////////////////////////////////////////////////////////////////////
template<class T>
void processpool<T>::run_child()
{
	m_epollfd=epoll_create(5);
	assert(m_epollfd!=-1);
	int pipefd=m_sub_process[m_idx].m_pipefd[1];
	addfd_in(m_epollfd,pipefd);
	T* users =new T[USER_PER_PROCESS];
	assert(users);
	epoll_event events[MAX_EVENT_NUMBER];
	int number=0;
	int ret=-1;
	while(!m_stop)
	{
		number=epoll_wait(m_epollfd,events,MAX_EVENT_NUMBER,-1);
		if((number<0)&&(errno!=EINTR))
		{
			printf("epoll fail\n");
			break;
		}
		for(int i=0;i<number;++i)
		{
			int sockfd=events[i].data.fd;
			if((sockfd==pipefd)&&(events[i].events&EPOLLIN)
			{
				int client=0;
				int ret=recv(sockfd,(char *)&client,sizeof(client),0);
				if(((ret<0)&&(errno!=EAGAIN))||ret==0)
				{
					continue;
				}
				else
				{
					struct sockaddr_in clinet_address;
					socklen_t client_addrlength =  sizeof(client_address);
					int connfd=accept(m_listenfd,(struct sockaddr *)&client_address,&client_addrlength);
					if(connfd<0)
					{
						continue;
					}
					addfd_in(m_epollfd,connfd);	
					//addfd_close(m_epollfd,connfd);不应该在这里监听他，应该在登录后关注是否下线
					users[connfd].init(m_epollfd,connfd,pipefd);						
				}
			}
			else if(events[i].events&EPOLLIN)
			{				
				users[sockfd].process();
			}
			else if(events[i].events&EPOLLRDHUP)
			{//有断开连接请求
				char uid[15]={0};
				users[events[i].data.fd].getid(uid);
				//向监听进程告知这个用户下线了，监听进程需从在线用户中删除这个用户
				int tag=OFFLINEMSG;
				send(m_sub_process[m_idx].m_pipefd[1],&tag,4,0);
				send(m_sub_process[m_idx].m_pipefd[1],uid,15,0);
			}
			else
			{
				continue;
			}
		}
	}
	delete users[];
	users=NULL;
	close(pipefd);
	close(m_epollfd);
}
////////////////////////////////////////////////////////////////////////
template<class T>
void processpool<T>::run_parent()
{
	m_epollfd=epoll_create(5);
	addfd_in(m_epollfd,m_listenfd);
	epoll_event events[MAX_EVENT_NUMBER];
	int sub_process_counter=0;
	int new_conn=1;
	int number=0;
	int ret=-1;

	while(!m_stop)
	{
		number=epoll_wait(m_epollfd,events,MAX_EVENT_NUMBER,-1);
		if((number<0)&&(errno!=EINTR))
		{
			break;
		}
		for(int i=0;i<number;++i)
		{
			int sockfd=events[i].data.fd;
			if(sockfd==m_listenfd)
			{
				int i=sub_process_counter;
				do
				{
					if(m_sub_process[i].m_pid!=-1)
					{
						break;
					}
					i=(i+1)%m_process_number;
				}while(i!=sub_process_counter);

				if(m_sub_process[i].m_pid==-1)
				{
					m_stop=true;
					break;
				}
				sub_process_counter=(i+1)%m_process_number;
				send(m_sub_process[i].m_pipefd[0],(char *)&new_conn,sizeof(new_conn),0);
				printf("send request to child %d\n",i);
			}
			else
			{
				int i=0;
				for(;i<process_number;++i)
				{
					if(sockfd==m_sub_process[i].pipefd[0])
					{
						int tag=0;
						recv(sockfd,&tag,4,0);
						char mid[15]={0};
						recv(sockfd,mid,15,0);
						if(tag==ONLINEMSG)
						{							
							m_user_state.insert(mid);							
						}
						else if(tag==OFFLINEMSG)
						{							
							m_user_state.erase(mid);
						}
						else if(tag==JUDGELOLINE)
						{
							if(m_user_state.find(mid)==m_user_state.end())
							{
								int tag=ISNOTONLINE;
								send(sockfd,&tag,4,0);
							}
							else
							{
								int tag=ISONLINE;
								send(sockfd,&tag,4,0);
							}							
						}	
					}
				}
			}
		}
	}
	close(m_epollfd);
}
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
	static const int MAX_PROCESS_NUMBER=16;
	static const int USER_PER_PROCESS=65533;
	static const int MAX_EVENT_NUMBER=10000;
	int m_process_number;
	int m_idx;
	int m_epollfd;
	int m_listenfd;
	int m_stop;
	process *m_sub_process;
	static processpool<T> *m_instance;//
};
template<class T>
processpool<T> *processpool<T>::minstance=NULL;


static int sig_pipefd[2];
static int setnonblocking(int fd)
{
	int old_option = fcntl(fd,F_GETFL);
	int new_option = old_option | O_NONBLOCK ;
	fcntl(fd,F_SETFL,new_option);
	return old_option;
}
static void addfd(int epollfd,int fd)
{
	epoll_event event;
	event.data.fd=fd;
	event.events=EPOLLIN|EPOLLET;
	epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
}

static void removefd(int epollfd,int fd)
{
	epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,0);
	close(fd);
}

static void sig_handler(int sig)
{
	int save_errno=errno;//errno 是记录系统的最后一次错误代码。代码是一个int型的值，在errno.h中定义
	int msg=sig;
	send(sig_pipefd[1],(char *)&msg,1,0);
	errno=save_errno;
}

/*
sigaction（查询或设置信号处理方式）
相关函数 signal，sigprocmask() ，sigpending，sigsuspend, sigemptyset
表头文件 #include<signal.h>
定义函数 int sigaction(int signum,const struct sigaction *act ,struct sigaction *oldact);
函数说明 sigaction()会依参数signum指定的信号编号来设置该信号的处理函数。参数signum可以指定SIGKILL和SIGSTOP以外的所有信号。
如参数结构sigaction定义如下
struct sigaction {
	void (*sa_handler)(int);
	void (*sa_sigaction)(int, siginfo_t *, void *);
	sigset_t sa_mask;
	int sa_flags;
	void (*sa_restorer)(void);
};
信号处理函数可以采用void (*sa_handler)(int)或void (*sa_sigaction)(int, siginfo_t *, void *)。到底采用哪个要看sa_flags中是否设置了SA_SIGINFO位，如果设置了就采用void (*sa_sigaction)(int, siginfo_t *, void *)，此时可以向处理函数发送附加信息；默认情况下采用void (*sa_handler)(int)，此时只能向处理函数发送信号的数值。
sa_handler此参数和signal()的参数handler相同，代表新的信号处理函数，其他意义请参考signal()。
sa_mask 用来设置在处理该信号时暂时将sa_mask 指定的信号集搁置。
sa_restorer 此参数没有使用。
sa_flags 用来设置信号处理的其他相关操作，下列的数值可用。
sa_flags还可以设置其他标志：
SA_RESETHAND：当调用信号处理函数时，将信号的处理函数重置为缺省值SIG_DFL
SA_RESTART：如果信号中断了进程的某个系统调用，则系统自动启动该系统调用
SA_NODEFER ：一般情况下， 当信号处理函数运行时，内核将阻塞该给定信号。但是如果设置了 SA_NODEFER标记， 那么在该信号处理函数运行时，内核将不会阻塞该信号
sigfillset()用来将参数set信号集初始化，然后把所有的信号加入到此信号集里即将所有的信号标志位置为1，屏蔽所有的信号。它是一个宏实现，如下所示：
#define sigfillset(ptr) ( *(ptr) = ~(sigset_t)0, 0)
*/
static void addsig(int sig,void (handler)(int),bool restart = true )
{
	struct sigaction sa;
	memset(&sa,0,sizeof(sa));
	sa.sa_handler = handler;
	if(restart)
	{
		sa.sa_flags |=SA_RESTART;
	}
	sigfillset(&sa.sa_mask);
	assert(sigaction(sig,&sa,NULL)!=-1);
}

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

		m_sub_process[i].m_pid=fork();
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

template<class T>
void processpool<T>:: setup_sig_pipe()
{
	m_epollfd=epoll_create(5);
	assert(m_epollfd!=-1);

	int ret=sockpair(PF_UNIX,SOCK_STREAM,0,sig_pipefd);
	assert(ret!=-1);

	setnonblocking(sig_piprfd[1]);
	addfd(epollfd,sig_pipefd[0]);

	addsig(SIGCHLD,sighandler);
	addsig(SIGINT,sig_handler);
	/*
	具体的分析可以结合TCP的"四次握手"关闭. TCP是全双工的信道, 可以看作两条单工信道, TCP连接两端的两个端点各负责一条. 当对端调用close时, 虽然本意是关闭整个两条信道, 但本端只是收到FIN包. 按照TCP协议的语义, 表示对端只是关闭了其所负责的那一条单工信道, 仍然可以继续接收数据. 也就是说, 因为TCP协议的限制, 一个端点无法获知对端的socket是调用了close还是shutdown.
对一个已经收到FIN包的socket调用read方法, 如果接收缓冲已空, 则返回0, 这就是常说的表示连接关闭. 但第一次对其调用write方法时, 如果发送缓冲没问题, 会返回正确写入(发送). 但发送的报文会导致对端发送RST报文, 因为对端的socket已经调用了close, 完全关闭, 既不发送, 也不接收数据. 所以, 第二次调用write方法(假设在收到RST之后), 会生成SIGPIPE信号, 导致进程退出.
为了避免进程退出, 可以捕获SIGPIPE信号, 或者忽略它, 给它设置SIG_IGN信号处理函数:
signal(SIGPIPE, SIG_IGN);
这样, 第二次调用write方法时, 会返回-1, 同时errno置为SIGPIPE. 程序便能知道对端已经关闭
	*/
	addsig(SIGPIPE,SIG_IGN);
}


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

template<class T>
void processpool<T>::run_child()
{
	setup_sig_pipe();
	int pipefd=m_sub_process[m_idx].m_pipefd[1];
	addfd(m_epollfd,pipefd);

	epoll_event events[MAX_EVENT_NUMBER];
	T* users =new T[USER_PER_PROCESS];
	assert(users);
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
			if((sockfd==pipefd)&&(events[i].events&EPOLLIN))
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
					addfd(mepollfd,connfd);
					users[connfd].init(m_epollfd,connfd,clinet_address);	
				}
			}
			else if((sockfd==sig_pipefd[0])&&(events[i].events&EPOLLIN))
			{
				int sig;
				char signals[1024];
				ret = recv(sig_pipefd[0],signals,sizeof(signals),0);
				if(ret<=0)
				{
					continue;
				}
				else
				{
					for(int i=0 ;i<ret;++i)
					{
						switch(signals[i])
						{
							case SIGCHLD:
								pid_t pid;
								int stat;
								while((pid=waitpid(-1,&stat,WNOHANG))>0)
								{
									continue;
								}
								break;
							case SIGTERM:
							case SIGINT:
								m_stop=true;
								break;
							default:
								break;
						}
					}
				}
			}
			else if(events[i].events&EPOLLIN)
			{
				users[sockfd].process();
			}
			else
			{
				continue;
			}
			delete users[];
			users=NULL;
			close(pipefd);
			close(m_epollfd);
		}
	}
}

template<class T>
void processpool<T>::run_parent()
{
	setup_single_pipe();
	addfd(m_epollfd,m_listenfd);

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
				printf("senf request to child %d\n",i);
			}
			else if((sockfd==sig_pipefd[0])&&(events[i].events&EPOLLIN))
			{
				int sig;
				char signals[1024];
				ret=recv(sig_pipefd[0],signals,sizeof(signals),0);
				if(ret<=0)
				{
					continue;
				}
				else
				{
					for(int i=0;i<ret;++i)
					{
						switch(signals[i])
						{
							case SIGCHLD:
								pid_t pid;
								int stat;
								while((pid=waitpid(-1,&stat,WNOHANG))>0)
								{
									for(int i=0;i<m_process_number;++i)
									{
										if(m_sub_process[i].m_pid==pid)
										{
											close(m_sub_process[i].m_pipefd[0]);
											m_sub_process[i].m_pid=-1;	
										}
									}
								}
								m_stop=true;
								for(int i=0;i<m_process_number;++i)
								{
									if(m_sub_process[i].m_pid!=-1)
									{
										m_stop=false;
									}
								}
								break;
							case SIGTERM:
							case SIGINT:
								for(int i=0;i<m_process_number;++i)
								{
									int pid = m_sub_process[i].m_pid;
									if(pid!=-1)
									{
										kill(pid,SIGTREM);
									}
								}
								break;
							default:
								break;
						}
					}
				}
			}
			else
			{
				continue;
			}
		}
	}
	close(m_epollfd);
}

#endif



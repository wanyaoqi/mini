#include"yunsql.h"

MySqlServer::MySqlServer(const char *host,const char *passwd)
{
	mysql=new MYSQL;
	char buff[256]={0};
	if(NULL==mysql_init(mysql))
	{
		cout<<"Init fail. "<<mysql_error(mysql)<<endl;
		exit(-1);
	}
	if(!mysql_real_connect(mysql,host,DBUSER,passwd,NULL,DBPORT,NULL,0))
	{
		cout<<"Connect mysql fail. "<<mysql_error(mysql)<<endl;
		exit(-1);
	}
	if(mysql_select_db(mysql,DBNAME))
	{
		cout<<DBNAME<<" is not exist,creating database!"<<endl;
		memset(buff,0,sizeof(buff));
		snprintf(buff,sizeof(buff),"create database %s",DBNAME);
		if(mysql_real_query(mysql,buff,strlen(buff)))
		{
			cout<<"Create database "<<DBNAME<<" fail. "<<mysql_error(mysql)<<endl;
			exit(-1);
		}
		cout<<"Create database "<<DBNAME<<" success"<<endl;
		mysql_select_db(mysql,DBNAME);
	}
	res=mysql_store_result(mysql);
	if(res!=NULL)
		mysql_free_result(res);

	memset(buff,0,sizeof(buff));
	snprintf(buff,sizeof(buff),"select * from %s",TABLENAME);
	if(mysql_real_query(mysql,buff,strlen(buff)))
	{
		memset(buff,0,sizeof(buff));
		snprintf(buff,sizeof(buff),"create table %s(%s int(4) unsigned not null primary key auto_increment,%s char(20) not null,%s char(20) not null,%s tinyint(1) not null default 0,%s timestamp )auto_increment=100000",TABLENAME,USERID,USERNAME,USERPASSWD,SNAPSHOT,TIME);
		res=mysql_store_result(mysql);
		if(res!=NULL)
			mysql_free_result(res);
		if(mysql_real_query(mysql,buff,strlen(buff)))
		{
			cout<<"Create table "<<TABLENAME<< "fail. "<<mysql_error(mysql)<<endl;
			exit(-1);
		}
		cout<<"Create table "<<TABLENAME<<" success"<<endl;
	}
	res=mysql_store_result(mysql);
	if(res!=NULL)
		mysql_free_result(res);

	memset(buff,0,sizeof(buff));
	snprintf(buff,sizeof(buff),"select * from %s",TABLENAME2);
	if(mysql_real_query(mysql,buff,strlen(buff)))
	{
		memset(buff,0,sizeof(buff));
		snprintf(buff,sizeof(buff),"create table %s(%s int(4) unsigned not null,%s int(4) unsigned not null, foreign key(%s) references %s(%s))",TABLENAME2,USRID,FRIENDID,FRIENDID,TABLENAME,USERID);
		res=mysql_store_result(mysql);
		if(res!=NULL)
			mysql_free_result(res);
		if(mysql_real_query(mysql,buff,strlen(buff)))
		{
			cout<<"Create table "<<TABLENAME2<< "fail. "<<mysql_error(mysql)<<endl;
			exit(-1);
		}
		cout<<"Create table "<<TABLENAME2<<" success"<<endl;
	}
	res=mysql_store_result(mysql);
	if(res!=NULL)
		mysql_free_result(res);	

	memset(buff,0,sizeof(buff));
	snprintf(buff,sizeof(buff),"select * from %s",TABLENAME3);
	if(mysql_real_query(mysql,buff,strlen(buff)))
	{
		memset(buff,0,sizeof(buff));
		snprintf(buff,sizeof(buff),"create table %s(%s char(129) not null,%s int(4) unsigned ,%s char(20),%s char(20) not null)",TABLENAME3,MD5,UID,FNAME,FSIZE);
		res=mysql_store_result(mysql);
		if(res!=NULL)
			mysql_free_result(res);
		if(mysql_real_query(mysql,buff,strlen(buff)))
		{
			cout<<"Create table "<<TABLENAME3<< "fail. "<<mysql_error(mysql)<<endl;
			exit(-1);
		}
		cout<<"Create table "<<TABLENAME3<<" success"<<endl;
	}
	res=mysql_store_result(mysql);
	if(res!=NULL)
		mysql_free_result(res);		

	memset(buff,0,sizeof(buff));
	snprintf(buff,sizeof(buff),"select * from %s",TABLENAME4);
	if(mysql_real_query(mysql,buff,strlen(buff)))
	{
		memset(buff,0,sizeof(buff));
		snprintf(buff,sizeof(buff),"create table %s(%s int(4) unsigned not null,%s char(20) not null,%s char(20) not null)",TABLENAME4,UID,FNAME,SIZE);
		res=mysql_store_result(mysql);
		if(res!=NULL)
			mysql_free_result(res);
		if(mysql_real_query(mysql,buff,strlen(buff)))
		{
			cout<<"Create table "<<TABLENAME4<< "fail. "<<mysql_error(mysql)<<endl;
			exit(-1);
		}
		cout<<"Create table "<<TABLENAME4<<" success"<<endl;
	}
	res=mysql_store_result(mysql);
	if(res!=NULL)
		mysql_free_result(res);	
	res=NULL;	
}

bool MySqlServer::user_reg(const char *name,const char *passwd,char *uid)
{
	if(mysql==NULL)
	{
		cout<<"Connect database fail! "<<mysql_error(mysql)<<endl;
		return false;
	}
	char buff[256]={0};
	snprintf(buff,sizeof(buff),"insert into %s(%s,%s) values('%s','%s')",TABLENAME,USERNAME,USERPASSWD,name,passwd);
	if(mysql_real_query(mysql,buff,strlen(buff)))
	{
		cout<<"Registration SQL failure! "<<mysql_error(mysql)<<endl;
		return false;
	}
	int id=mysql_insert_id(mysql);
	sprintf(uid,"%d",id);//£¿£¿
	res=mysql_store_result(mysql);
	if(res!=NULL)
		mysql_free_result(res);
	res=NULL;
	return true;
}

bool MySqlServer::user_login(const char *uid,const char *passwd)
{
	bool flag=false;
	if(mysql==NULL)
	{
		cout<<"Connect database fail! "<<mysql_error(mysql)<<endl;
		return false;
	}
	char buff[256]={0};
	snprintf(buff,sizeof(buff),"select *from %s where %s=%d",TABLENAME,USERID,atoi(uid));
	if(mysql_real_query(mysql,buff,strlen(buff)))
	{
		cout<<"Failed to execute SQL statements! "<<mysql_error(mysql)<<endl;
		return false;
	}
	res=mysql_store_result(mysql);
	while(row=mysql_fetch_row(res))
	{
		if((strcmp(uid,row[0])==0)&&(strcmp(passwd,row[2])==0))
		{
			flag=true;
		}
	}
	if(res!=NULL)
		mysql_free_result(res);
	res=NULL;
	return flag;
}

char* MySqlServer::get_friendlist(const char *id,int &count)
{	
	if(mysql==NULL)
	{
		cout<<"Connect database fail! "<<mysql_error(mysql)<<endl;
		return NULL;
	}
	char buff[256]={0};
	snprintf(buff,sizeof(buff),"select *from %s where %s=%d",TABLENAME2,USRID,atoi(id));
	if(mysql_real_query(mysql,buff,strlen(buff)))
	{
		cout<<"Failed to execute SQL statements! "<<mysql_error(mysql)<<endl;
		return NULL;
	}
	res=mysql_store_result(mysql);
	count=mysql_num_rows(res);
	char *tmp=new char[count*10];
	char *rtmp=tmp;
	while(row=mysql_fetch_row(res))
	{
		strcpy(tmp,row[1]);
		tmp+=10;
	}
	if(res!=NULL)
		mysql_free_result(res);
	res=NULL;
	return rtmp;
}

bool MySqlServer::add_friend(const char *uid,const char *fid)
{
	if(mysql==NULL)
	{
		cout<<"Connect database fail! "<<mysql_error(mysql)<<endl;
		return false;
	}
	char buff[256]={0};
	snprintf(buff,sizeof(buff),"select * from %s where %s=%d AND %s=%d",TABLENAME2,USRID,atoi(uid),FRIENDID,atoi(fid));
	if(mysql_real_query(mysql,buff,strlen(buff)))
	{
		cout<<"Failed to execute SQL statements! "<<mysql_error(mysql)<<endl; 
		return false;     
	}
	res=mysql_store_result(mysql);
	while(row=mysql_fetch_row(res))
	{
		if(strcmp(row[0],uid)==0)
		{
			return false;
		}			
	}
	if(res!=NULL)
		mysql_free_result(res);

	memset(buff,0,sizeof(buff));
	snprintf(buff,sizeof(buff),"insert into %s values(%d,%d)",TABLENAME2,atoi(uid),atoi(fid));
	if(mysql_real_query(mysql,buff,strlen(buff)))
	{
		cout<<"Failed to execute SQL statements! "<<mysql_error(mysql)<<endl;
		return false;
	}
	if(res!=NULL)
		mysql_free_result(res);
	res=NULL;
	return true;
}

char* MySqlServer::get_filelist(const char *id,int &count)
{
	if(mysql==NULL)
	{
		cout<<"Connect database fail! "<<mysql_error(mysql)<<endl;
		return NULL;
	}
	char buff[256]={0};
	snprintf(buff,sizeof(buff),"select *from %s where %s=%d",TABLENAME3,UID,atoi(id));
	if(mysql_real_query(mysql,buff,strlen(buff)))
	{
		cout<<"Failed to execute SQL statements! "<<mysql_error(mysql)<<endl;
		return NULL;
	}
	res=mysql_store_result(mysql);
	count=mysql_num_rows(res);
	char *tmp=new char[count*20];
	char *rtmp=tmp;
	while(row=mysql_fetch_row(res))
	{
		strcpy(tmp,row[2]);
		tmp+=20;
	}
	if(res!=NULL)
		mysql_free_result(res);
	res=NULL;
	return rtmp;
}

bool MySqlServer::judge_md5(const char *md5)
{
	bool flag=false;
	if(mysql==NULL)
	{
		cout<<"Connect database fail! "<<mysql_error(mysql)<<endl;
		return false;
	}
	char buff[256]={0};
	snprintf(buff,sizeof(buff),"select *from %s where %s='%s'",TABLENAME3,MD5,md5);
	if(mysql_real_query(mysql,buff,strlen(buff)))
	{
		cout<<"Failed to execute SQL statements! "<<mysql_error(mysql)<<endl;
		return false;
	}
	res=mysql_store_result(mysql);
	while(row=mysql_fetch_row(res))
	{
		if(strcmp(row[0],md5)==0)
			flag=true;	
	}
	if(res!=NULL)
		mysql_free_result(res);
	res=NULL;
	return flag;
}

bool MySqlServer::judge_snapshot(const char *id)
{
	bool flag=false;
	if(mysql==NULL)
	{
		cout<<"Connect database fail! "<<mysql_error(mysql)<<endl;
		return false;
	}
	char buff[256]={0};
	snprintf(buff,sizeof(buff),"select *from %s where %s=%d",TABLENAME,USERID,atoi(id));
	if(mysql_real_query(mysql,buff,strlen(buff)))
	{
		cout<<"Failed to execute SQL statements! "<<mysql_error(mysql)<<endl;
		return false;
	}
	res=mysql_store_result(mysql);
	while(row=mysql_fetch_row(res))
	{
		if(atoi(row[3])==1)
			flag=true;	
	}
	if(res!=NULL)
		mysql_free_result(res);
	res=NULL;
	return flag;
}

bool MySqlServer::insert_snapshot(const char *id)
{
	if(mysql==NULL)
	{
		cout<<"Connect database fail! "<<mysql_error(mysql)<<endl;
		return false;
	}
	char buff[256]={0};
	snprintf(buff,sizeof(buff),"update %s set %s=1 where %s=%d",TABLENAME,SNAPSHOT,USERID,atoi(id));
	if(mysql_real_query(mysql,buff,strlen(buff)))
	{
		cout<<"Failed to execute SQL statements! "<<mysql_error(mysql)<<endl;
		return false;
	}
	if(res!=NULL)
		mysql_free_result(res);
	res=NULL;
	return true;
}

bool MySqlServer::insert_resume(const char *id,const char *fname,const char *size)
{
	if(mysql==NULL)
	{
		cout<<"Connect database fail! "<<mysql_error(mysql)<<endl;
		return false;
	}
	char buff[256]={0};
	snprintf(buff,sizeof(buff),"insert into %s values(%d,'%s','%s') ",TABLENAME4,atoi(id),fname,size);
	if(mysql_real_query(mysql,buff,strlen(buff)))
	{
		cout<<"Failed to execute SQL statements! "<<mysql_error(mysql)<<endl;
		return false;
	}
	if(res!=NULL)
		mysql_free_result(res);
	res=NULL;
	return true;
}

char* MySqlServer::get_resume(const char *id,const char *fname)
{
	if(mysql==NULL)
	{
		cout<<"Connect database fail! "<<mysql_error(mysql)<<endl;
		return NULL;
	}
	char buff[256]={0};
	snprintf(buff,sizeof(buff),"select *from %s where %s=%d AND %s='%s'",TABLENAME4,UID,atoi(id),FNAME,fname);
	if(mysql_real_query(mysql,buff,strlen(buff)))
	{
		cout<<"Failed to execute SQL statements! "<<mysql_error(mysql)<<endl;
		return NULL;
	}
	res=mysql_store_result(mysql);
	char *tmp=new char[20];
	while(row=mysql_fetch_row(res))
		strcpy(tmp,row[2]);
	if(res!=NULL)
		mysql_free_result(res);
	res=NULL;
	return tmp;
}

bool MySqlServer::upload(const char *id,const char *fname,const char *size,const char *md5)
{
	if(mysql==NULL)
	{
		cout<<"Connect database fail! "<<mysql_error(mysql)<<endl;
		return false;
	}
	char buff[256]={0};
	snprintf(buff,sizeof(buff),"insert into %s values('%s',%d,'%s','%s') ",TABLENAME3,md5,atoi(id),fname,size);
	if(mysql_real_query(mysql,buff,strlen(buff)))
	{
		cout<<"Failed to execute SQL statements! "<<mysql_error(mysql)<<endl;
		return false;
	}
	if(res!=NULL)
		mysql_free_result(res);
	res=NULL;
	return true;
}

MySqlServer::~MySqlServer()
{
	if(res!=NULL)
	{
		mysql_free_result(res);
		res=NULL;
	}
	mysql_close(mysql);
	delete mysql;
	mysql=NULL;
}


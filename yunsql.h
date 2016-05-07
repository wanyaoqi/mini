#ifndef MySqlServer_H
#define MySqlServer_H

#include<iostream>
#include<stdio.h>
#include<string>
#include<stdlib.h>
#include<string.h>
#include<mysql.h>
using namespace std;

class MySqlServer
{
public:
	MySqlServer(const char *host,const char *passwd);
	bool user_reg(const char *name,const char *passwd,char *uid);
	bool user_login(const char *uid,const char *passwd);
    char *get_friendlist(const char *id,int &count);
    bool add_friend(const char *uid,const char *fid);
	char *get_filelist(const char *id,int &count);
	bool judge_md5(const char *md5);
	bool judge_snapshot(const char *id);
	bool insert_snapshot(const char *id);
	char *get_resume(const char *id,const char *fname);
	bool insert_resume(const char *id,const char *fname,const char *size);
	bool upload(const char *id,const char *fname,const char *size,const char *md5);
	~MySqlServer();
private:
	MYSQL *mysql;
	MYSQL_RES *res;
	MYSQL_ROW row;
};

#define DBPORT   	3306
#define DBUSER  	"root"
#define DBNAME  	"cloud"

#define TABLENAME   "user"
#define USERID	   	"userid"	
#define USERNAME   	"username"   
#define USERPASSWD 	"passwd"
#define SNAPSHOT	"snapshot"
#define TIME        "time"

#define TABLENAME2  "friendlist"
#define USRID       "uid"
#define FRIENDID    "fid"

#define TABLENAME3  "filelist"
#define MD5         "md5"
#define UID         "uid"
#define FNAME 		"fname"
#define FSIZE       "fsize"


#define TABLENAME4  "duandian"
#define UID         "uid"
#define FILENAME    "filename"
#define SIZE        "size"

#endif

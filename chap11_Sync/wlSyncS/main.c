#include    <stdio.h>
#include    <ctype.h>
#include    <unistd.h>
#include    <stdlib.h>
#include    <string.h>
#include    <errno.h>
#include    <signal.h>
#include    <sys/param.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include    <netinet/in.h>
#include    <arpa/inet.h>
#include    <netdb.h>
#include    <poll.h>
#include    <fcntl.h>
#include    <sys/mman.h>
#include    <sys/stat.h>
#include    <dirent.h>
#include    <pthread.h>
#include    "sock.h"
#include    "token.h"
#include    "jconv.h"
#include    "log.h"
#include    "lockf.h"
#define	WLSYNCS_MAIN
#include    "param.h"

THREAD_STATUS	*ThreadStatus=NULL;

PARAM	Param={
	0,
	NULL,
	0,
	NULL,
	10,
	5,
	0,
	8096,
	"./",
	LOG_DEBUG,
	1,
	"./wlSyncC.ver",
	"./wlSyncC.work"
};

int OnlyOne=0;

char	*MapPtr;
int	MapSize;

int	SigEnd=0;

void ending(int sig);
void endingFunc(int sig);
int DeleteVersionFile();
int MakeVersionFile();
int ThreadNotExistCheck();
int ThreadExistCheck();
int ExecWork();
int MakeFolderList(char *tpath,char *spath,FILE *fp);
void *WorkThread(void *arg);
int SendOneData(int soc,char *name,char *path);
int SendToRemote(int acc,char *data);
int CleanFilename(char *name);
int MakeDirectory(char *path);


void ending(int sig)
{
	SigEnd=sig;
}

void endingFunc(int sig)
{
	Syslog(LOG_ERR,"endingFunc:sig=%d\n",sig);

	exit(0);
}

int main(int argc,char *argv[])
{
int	ret;
time_t	now,beforeTime=0;
int	i;

	for(i=1;i<argc;i++){
		if(strcmp(argv[i],"-d")==0){
			daemon(1,0);
		}
		else if(strcmp(argv[i],"-1")==0){
			OnlyOne=1;
		}
	}

	DeleteVersionFile();

	/* パラメータ読み込み */
	ret=ReadParam(PARAM_PATH);
	if(ret==-1){
		Syslog(LOG_ERR,"main:Cannot read parameter\n");
		return(-1);
	}

	SetLogStderrOut(Param.StderrOut);
	SetLogPath(Param.LogPath);

	MakeDirectory(Param.LogPath);

	SetLogName(APP_NAME);
	SetLogLevel(-1,Param.LogLevel);
	InitSyslog(LOG_LOCAL7);

	ParamLogOut();

	ThreadStatus=(THREAD_STATUS *)calloc(Param.TargetHostCnt,sizeof(THREAD_STATUS));

	SetRecvTimeoutSec(Param.RecvTimeout*60);

	signal(SIGTERM,ending);
	signal(SIGQUIT,ending);
	signal(SIGTERM,ending);
	signal(SIGINT,ending);

	MakeVersionFile();

	Syslog(LOG_INFO,"main:wlSyncS:ready\n");

	if(OnlyOne){
		ExecWork();
		while(1){
			if(SigEnd){
				endingFunc(SigEnd);
				SigEnd=0;
			}
			if(ThreadNotExistCheck(0)){
				break;
			}
			sleep(1);
		}
	}
	else{
		while(1){
			if(SigEnd){
				endingFunc(SigEnd);
				SigEnd=0;
			}
			now=time(NULL);
Syslog(LOG_DEBUG,"now-beforeTime=%d:%d\n",now-beforeTime,Param.Interval*60);
			if(now-beforeTime>Param.Interval*60){
Syslog(LOG_DEBUG,"Time to work\n");
				if(ThreadNotExistCheck(0)){
Syslog(LOG_DEBUG,"No working thread\n");
					ExecWork();
				}
				else{
Syslog(LOG_WARNING,"Working thread exist\n");
				}
				beforeTime=now;
			}
			sleep(1);
		}
	}

	return(0);
}

int DeleteVersionFile()
{
	Syslog(LOG_DEBUG,"DeleteVersionFile:%s\n",Param.VersionFilePath);

	unlink(Param.VersionFilePath);

	return(0);
}

int MakeVersionFile()
{
FILE	*fp;

	Syslog(LOG_DEBUG,"MakeVersionFile:%s\n",Param.VersionFilePath);

	if((fp=fopen(Param.VersionFilePath,"w"))==NULL){
		Syslog(LOG_ERR,"MakeVersionFile:Cannot write version file(%s)\n",Param.VersionFilePath);
		return(-1);
	}

	fprintf(fp,"%s\n",VERSION_STRING);

	fclose(fp);

	return(0);
}

int ThreadNotExistCheck(int status)
{
int	i;

	for(i=0;i<Param.TargetHostCnt;i++){
		if(ThreadStatus[i].status!=status){
			return(0);
		}
	}
	return(1);
}

int ThreadExistCheck(int status)
{
int	i;

	for(i=0;i<Param.TargetHostCnt;i++){
		if(ThreadStatus[i].status==status){
			return(1);
		}
	}
	return(0);
}

int ExecWork()
{
int	i;
FILE	*fp;
int	fd;
struct stat	st;

	Syslog(LOG_DEBUG,"ExecWork:start\n");

	if((fp=fopen(Param.WorkFilePath,"w"))==NULL){
		Syslog(LOG_ERR,"ExecWork:fopen(%s):error\n",Param.WorkFilePath);
		return(-1);
	}
	for(i=0;i<Param.TargetFolderCnt;i++){
		fprintf(fp,"#Name=%s,%s\n",Param.TargetFolder[i].name,KANJI_CODE);
		MakeFolderList(Param.TargetFolder[i].path,"",fp);
	}
	fprintf(fp,"#End\n");
	fclose(fp);

	if((fd=open(Param.WorkFilePath,O_RDONLY))==-1){
		SyslogPerror(LOG_ERR,"ExecWork:open");
		unlink(Param.WorkFilePath);
		return(-1);
	}
	stat(Param.WorkFilePath,&st);
	MapSize=st.st_size;
	if((MapPtr=(char *)mmap(0,MapSize,PROT_READ,MAP_SHARED,fd,0))==(char *)-1){
		SyslogPerror(LOG_ERR,"ExecWork:mmap");
		close(fd);
		unlink(Param.WorkFilePath);
		return(-1);
	}

	for(i=0;i<Param.TargetHostCnt;i++){
		ThreadStatus[i].status=1;
		if(pthread_create(&ThreadStatus[i].thread_id,NULL,WorkThread,(void *)i)==0){
			Syslog(LOG_INFO,"ExecWork:pthread_create:thread_id=%u\n",ThreadStatus[i].thread_id);
		}
		else{
			ThreadStatus[i].status=0;
			SyslogPerror(LOG_ERR,"ExecWork:pthread_create");
		}
	}

	while(1){
		if(ThreadExistCheck(1)){
			sleep(1);
			continue;
		}
		else{
			break;
		}
	}

	munmap(MapPtr,MapSize);
	close(fd);

	unlink(Param.WorkFilePath);

	Syslog(LOG_DEBUG,"ExecWork:end\n");

	return(0);
}

int MakeFolderList(char *tpath,char *path,FILE *fp)
{
DIR	*dir;
struct dirent	*dp;
struct stat	st;
char	*fullpath,*spath;

	Syslog(LOG_DEBUG,"MakeFolderList:tpath=%s,path=%s:start\n",tpath,path);

	fullpath=(char *)malloc(strlen(tpath)+strlen(path)+2);
	sprintf(fullpath,"%s/%s",tpath,path);
	CleanFilename(fullpath);
	if((dir=opendir(fullpath))==NULL){
		Syslog(LOG_ERR,"Cannot opendir %s\n",fullpath);
		return(-1);
	}
	free(fullpath);

	for(dp=readdir(dir);dp!=NULL;dp=readdir(dir)){
		if(strcmp(dp->d_name,".")==0||strcmp(dp->d_name,"..")==0){
			continue;
		}
		if(strlen(path)>0){
			spath=(char *)malloc(strlen(path)+strlen(dp->d_name)+2);
			sprintf(spath,"%s/%s",path,dp->d_name);
			CleanFilename(spath);
		}
		else{
			spath=strdup(dp->d_name);
		}
		fullpath=(char *)malloc(strlen(tpath)+strlen(spath)+2);
		sprintf(fullpath,"%s/%s",tpath,spath);
		Syslog(LOG_INFO,"MakeFolderList:fullpath=%s\n",fullpath);
		if(stat(fullpath,&st)!=-1){
			if(S_ISDIR(st.st_mode)){
				fprintf(fp,"D:%s\n",spath);
				MakeFolderList(tpath,spath,fp);
			}
			else{
				fprintf(fp,"F:%u,%lld,0,%s\n",(unsigned int)st.st_mtime,st.st_size,spath);
			}
		}
		else{
			SyslogPerror(LOG_ERR,"MakeFolderList:stat");
		}
		free(fullpath);
		free(spath);
	}

	closedir(dir);

	Syslog(LOG_DEBUG,"MakeFolderList:tpath=%s,path=%s:end\n",tpath,path);

	return(0);
}

void *WorkThread(void *arg)
{
int	no=(int)arg;
char	*buf;
TOKEN	token;

	Syslog(LOG_DEBUG,"WorkThread:%d:start\n",no);

	pthread_detach(pthread_self());

	if((ThreadStatus[no].soc=ClientSocketNew(Param.TargetHost[no].host,Param.TargetHost[no].port))==-1){
		Syslog(LOG_ERR,"WorkThread:ClientSocketNew(%s,%s):-1\n",Param.TargetHost[no].host,Param.TargetHost[no].port);
		ThreadStatus[no].status=0;
		return((void *)-1);
	}

	SendSize(ThreadStatus[no].soc,MapPtr,MapSize);

	ThreadStatus[no].status=2;

	if(RecvOneLine_2(ThreadStatus[no].soc,&buf,0)<=0){
		Syslog(LOG_INFO,"WorkThread:RecvOneLine_2:error or closed\n");
		close(ThreadStatus[no].soc);
		ThreadStatus[no].status=0;
		return((void *)-1);
	}
	CutCrLf(buf);
	Syslog(LOG_DEBUG,"WorkThread:RecvOneLine_2:%s\n",buf);
	if(StrNCmp(buf,"#OK",strlen("#OK"))!=0){
		Syslog(LOG_INFO,"WorkThread:RecvOneLine_2 not \"#OK\"\n");
		free(buf);
		close(ThreadStatus[no].soc);
		ThreadStatus[no].status=0;
		return((void *)-1);
	}
	free(buf);

	while(1){
		if(RecvOneLine_2(ThreadStatus[no].soc,&buf,0)<=0){
			Syslog(LOG_INFO,"WorkThread:RecvOneLine_2:error or closed\n");
			break;
		}
		CutCrLf(buf);
		Syslog(LOG_INFO,"WorkThread:RecvOneLine_2:%s\n",buf);
		if(StrNCmp(buf,"#Get=",strlen("#Get="))==0){
			GetToken(buf,strlen(buf),&token,"\r\n","=,");
			if(token.no!=5){
				Syslog(LOG_ERR,"WorkTherad:GetToken:token.no(%d)!=5\n",token.no);
			}
			else if(strcmp(token.token[1],"=")!=0){
				Syslog(LOG_ERR,"WorkTherad:token.token[1](%s)!=\"=\"\n",token.token[1]);
			}
			else if(strcmp(token.token[3],",")!=0){
				Syslog(LOG_ERR,"WorkTherad:token.token[3](%s)!=\",\"\n",token.token[3]);
			}
			else{
				SendOneData(ThreadStatus[no].soc,token.token[2],token.token[4]);
			}
			FreeToken(&token);
		}
		else if(StrNCmp(buf,"#Noop",strlen("#Noop"))==0){
			SendToRemote(ThreadStatus[no].soc,"#OK\n");
		}
		free(buf);
	}

	close(ThreadStatus[no].soc);

	ThreadStatus[no].status=0;

	Syslog(LOG_DEBUG,"WorkThread:%d:end\n",no);

	return(0);
}

int SendOneData(int soc,char *name,char *path)
{
int	targetNo,i;
size_t	size,total;
int	fd;
char 	*fullpath,*buf;
char	msg[512];
struct stat	st;
int	lf;

	Syslog(LOG_DEBUG,"SendOneData:start:%s,%s\n",name,path);

	targetNo=-1;
	for(i=0;i<Param.TargetFolderCnt;i++){
		if(strcmp(name,Param.TargetFolder[i].name)==0){
			targetNo=i;
			break;
		}
	}
	if(targetNo==-1){
		Syslog(LOG_ERR,"SendOneData:Cannot find %s\n",name);
		SendToRemote(soc,"#Error=Cannot find target\n");
		return(-1);
	}

	fullpath=(char *)malloc(strlen(Param.TargetFolder[targetNo].path)+strlen(path)+2);
	sprintf(fullpath,"%s/%s",Param.TargetFolder[targetNo].path,path);
	CleanFilename(fullpath);
	Syslog(LOG_DEBUG,"SendOneData:fullpath=%s\n",fullpath);

	lf=LockFile(fullpath);
	if((fd=open(fullpath,O_RDONLY))==-1){
		Syslog(LOG_ERR,"SendOneData:Cannot open %s\n",fullpath);
		SendToRemote(soc,"#Error=Cannot open file\n");
		free(fullpath);
		UnlockFile(lf);
		return(-1);
	}
	fstat(fd,&st);

	buf=(char *)malloc(Param.SendSize);
	total=0;
	while(1){
		size=read(fd,buf,Param.SendSize);
		if(size<=0){
			SendToRemote(soc,"#Size=0,0\n");
			break;
		}
		sprintf(msg,"#Size=%d,0\n",size);
		SendToRemote(soc,msg);
		SendSize(soc,buf,size);
		total+=size;
		if(total>st.st_size){
			SendToRemote(soc,"#Size=0,0\n");
			break;
		}
	}
	free(buf);

	if(total!=st.st_size){
		Syslog(LOG_WARNING,"SendOneData:%s:size changed\n",fullpath);
	}

	free(fullpath);

	close(fd);
	UnlockFile(lf);

	Syslog(LOG_DEBUG,"SendOneData:end\n");

	return(0);
}

int SendToRemote(int acc,char *data)
{
int	ret;

	Syslog(LOG_DEBUG,"SendToRemote:acc=%d,data=%s\n",acc,data);

	ret=SendSize(acc,data,strlen(data));

	return(ret);
}

int CleanFilename(char *name)
{
char	*tmp;
int	len,i,c;

	/*Syslog(LOG_DEBUG,"CleanFilename:name=%s\n",name);*/

	len=strlen(name);
	tmp=(char *)calloc(len+1,sizeof(char));

	c=0;
	for(i=0;i<len;i++){
		if(i>0&&name[i]=='/'&&name[i-1]=='/'){
			continue;
		}
		tmp[c]=name[i];
		c++;
	}
	tmp[c]='\0';

	strcpy(name,tmp);
	free(tmp);

	len=strlen(name);
	if(name[len]=='/'){
		name[len]='\0';
	}

	return(0);
}

int MakeDirectory(char *path)
{
char	*ptr,*p;

	ptr=path;
	while(1){
		if((p=strchr(ptr,'/'))==NULL){
			break;
		}
		*p='\0';
		mkdir(path,0);
		*p='/';
		ptr=p+1;
	}
	mkdir(path,0777);

	return(0);
}



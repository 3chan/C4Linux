#include	<stdio.h>
#include	<ctype.h>
#include	<stdarg.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<string.h>
#include	<errno.h>
#include	<syslog.h>
#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<time.h>
#include	<pthread.h>
#include	"log.h"

/* ログ出力ディレクトリ */
static char	*LogPath=NULL;
/* ログ名 */
static char	*LogName=NULL;
/* ログファイル名 */
static char	*LogFilename=NULL;
/* syslog出力閾値 */
static int	SyslogLevel=LOG_WARNING;
/* ファイル出力閾値 */
static int	FileLevel=LOG_DEBUG;
/* 標準エラー出力フラグ */
static int	StderrOut=0;

/* ローテーション閾値 */
#define	LIMIT_SIZE	(1*1024*1024)
/* ログファイルディスクリプタ */
static int	LogFd=-1;

/* 排他用ミューテックス */
pthread_mutex_t	LogMutex=PTHREAD_MUTEX_INITIALIZER;

/* 内部関数プロトタイプ宣言 */
static int lcheck_size();
static char *GetLogLevelStrSimple(int level);
static int log_out(int level,char *msg);


int GetLogLevelValue(char *name)
{
	if(strcmp(name,"LOG_EMERG")==0){
		return(LOG_EMERG);
	}
	else if(strcmp(name,"LOG_ALERT")==0){
		return(LOG_ALERT);
	}
	else if(strcmp(name,"LOG_CRIT")==0){
		return(LOG_CRIT);
	}
	else if(strcmp(name,"LOG_ERR")==0){
		return(LOG_ERR);
	}
	else if(strcmp(name,"LOG_WARNING")==0){
		return(LOG_WARNING);
	}
	else if(strcmp(name,"LOG_NOTICE")==0){
		return(LOG_NOTICE);
	}
	else if(strcmp(name,"LOG_INFO")==0){
		return(LOG_INFO);
	}
	else if(strcmp(name,"LOG_DEBUG")==0){
		return(LOG_DEBUG);
	}
	else{
		return(-1);
	}
}

static char *GetLogLevelStrSimple(int level)
{
char	*levelStr[]={"EMERG","ALERT","CRIT","ERR","WARNING","NOTICE","INFO","DEBUG"};

	if(level>=0&&level<=7){
		return(levelStr[level]);
	}
	else{
		return("undefine");
	}
}

void SetLogPath(char *path)
{
	LogPath=strdup(path);
}

void SetLogName(char *name)
{
	LogName=strdup(name);
	if(LogPath==NULL){
		LogPath=strdup("./");
	}
	if(strlen(LogPath)>0){
		LogFilename=calloc(strlen(LogPath)+1+strlen(name)+3+1+1,sizeof(char));
		sprintf(LogFilename,"%s/%s.log",LogPath,LogName);
	}
	else{
		LogFilename=calloc(strlen(name)+3+1+1,sizeof(char));
		sprintf(LogFilename,"%s.log",LogName);
	}
}

void SetLogLevel(int syslogLevel,int fileLevel)
{
	SyslogLevel=syslogLevel;
	FileLevel=fileLevel;
}

void SetLogStderrOut(int flag)
{
	StderrOut=flag;
}

void InitSyslog(int facility)
{
	openlog(LogName,0,facility);
}

static int lcheck_size()
{
struct stat	st;
char	buf[1024];
static int	count=0;

	if(LogFilename==NULL){
		return(0);
	}

	if(LogFd!=-1){
		count++;
		if(count%500!=0){
			return(1);
		}
	}

	if(stat(LogFilename,&st)!=0){
		if((LogFd=open(LogFilename,O_WRONLY|O_CREAT,0666))==-1){
			perror("open:LOGFILE");
			LogFd=2;
			return(-1);
		}
		else{
			return(1);
		}
	}
	else{
                if(st.st_size>LIMIT_SIZE){
                        /* サイズが最大サイズを越える:バックアップし新規に */
			if(LogFd!=-1&&LogFd!=2){
				close(LogFd);
			}
                        sprintf(buf,"%s.bak",LogFilename);
                        rename(LogFilename,buf);

                        if((LogFd=open(LogFilename,O_WRONLY|O_CREAT,0666))==-1){
                                perror("open");
				LogFd=2;
                                return(-1);
                        }
			else{
				return(1);
			}
                }
                else{
			if(LogFd==-1){
				if((LogFd=open(LogFilename,O_WRONLY|O_CREAT|O_APPEND,0666))==-1){
					perror("open");
					LogFd=2;
					return(-1);
				}
				else{
					return(1);
				}
			}
			else{
				return(1);
			}
                }
        }
}

static int log_out(int level,char *msg)
{
char	buf[2048];
time_t	t;
struct tm	*tm;
char	c_time[40];

	pthread_mutex_lock(&LogMutex);

	lcheck_size();

	t=time(NULL);
        tm=localtime(&t);
        strftime(c_time,sizeof(c_time),"%Y/%m/%d %H:%M:%S",tm);

	snprintf(buf,sizeof(buf),"<%lu>%s:%s:%s",pthread_self(),c_time,GetLogLevelStrSimple(level),msg);

	buf[2043]='\0';
	if(LogFd!=-2){
		write(LogFd,buf,strlen(buf));
	}
	if(StderrOut){
		write(2,buf,strlen(buf));
	}

	pthread_mutex_unlock(&LogMutex);

	return(0);
}

void Syslog(int level,char *fmt,...)
{
va_list	args;
char	buf[1024];
int	len;

	va_start(args,fmt);
	vsnprintf(buf,sizeof(buf),fmt,args);
	buf[1023]='\0';
	len=strlen(buf);
	if(buf[len-1]!='\n'){
		if(len==1024){
			buf[1022]='\n';
		}
		else{
			buf[len]='\n';
			buf[len+1]='\0';
		}
	}
	if(level<=SyslogLevel){
		syslog(level,buf);
	}
	if(level<=FileLevel){
		log_out(level,buf);
	}
	va_end(args);
}

void SyslogFix(int level,char *msg)
{
	if(level<=SyslogLevel){
		syslog(level,msg);
	}
	if(level<=FileLevel){
		log_out(level,msg);
	}
}

void SyslogPerror(int level,char *str)
{
char    buf[513];

        snprintf(buf,sizeof(buf),"%s : %s\n",str,strerror(errno));
        buf[512]='\0';

	if(level<=SyslogLevel){
		syslog(level,buf);
	}
	if(level<=FileLevel){
		log_out(level,buf);
	}
}

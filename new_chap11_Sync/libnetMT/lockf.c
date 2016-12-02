#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/file.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<errno.h>
#include	"log.h"

int LockFile(char *path)
{
int	fd;
int	ret;

	if((fd=open(path,O_WRONLY))==-1){
		/*ファイルが存在しない場合*/
		if(errno!=ENOENT||(fd=creat(path,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH))==-1){
			SyslogPerror(LOG_ERR,"open");
			Syslog(LOG_ERR,"openfailure\n");
			return(-1);
		}
	}

	if((ret=lockf(fd,F_LOCK,0))==-1){
		SyslogPerror(LOG_ERR,"lockf");
		close(fd);
		return(-1);
	}
	Syslog(LOG_DEBUG,"lockf(lock)=%d\n",ret);

	return(fd);
}

int UnlockFile(int fd)
{
int	ret;

	if(fd<0){
		return(0);
	}

	ret=lockf(fd,F_ULOCK,0);
	Syslog(LOG_DEBUG,"lockf(unlock)=%d\n",ret);

	close(fd);

	return(0);
}

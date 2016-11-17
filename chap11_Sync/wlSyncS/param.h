#ifndef	_PARAM_H_
#define	_PARAM_H_

#define	APP_NAME	"wlSyncS"

#define	VERSION_STRING	"1.0.0"

#define	PARAM_PATH	"./wlSyncS.ini"

#define	KANJI_CODE	("EUC")

typedef struct	{
	int	status;
	pthread_t	thread_id;
	int	soc;
}THREAD_STATUS;

typedef struct	{
	char	*name;
	char	*path;
}FOLDER;

typedef struct	{
	char	*host;
	char	*port;
}HOST;

typedef struct	{
	int	TargetFolderCnt;
	FOLDER	*TargetFolder;
	int	TargetHostCnt;
	HOST	*TargetHost;
	int	Interval;
	int	RecvTimeout;
	int	UseDigest;
	int	SendSize;
	char	*LogPath;
	int	LogLevel;
	int	StderrOut;
	char	*VersionFilePath;
	char	*WorkFilePath;
}PARAM;

#ifdef	WLSYNCS_MAIN
#else
extern PARAM    Param;
#endif

int ReadParam(char *filename);
void ParamLogOut();

#endif

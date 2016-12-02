#ifndef	_PARAM_H_
#define	_PARAM_H_

#define	APP_NAME	"wlSyncC"

#define	VERSION_STRING	"1.0.0"

#define	PARAM_PATH	"./wlSyncC.ini"

typedef struct	{
	char	*name;
	char	*path;
}FOLDER;

typedef struct	{
	int	TargetFolderCnt;
	FOLDER	*TargetFolder;
	char	*Port;
	int	RecvTimeout;
	int	UseDigest;
	char	*LogPath;
	int	LogLevel;
	int	StderrOut;
	char	*VersionFilePath;
	char	*WorkFilePath;
	int	DeleteFlag;
}PARAM;

#ifdef	WLSYNCC_MAIN
#else
extern PARAM    Param;
#endif

int ReadParam(char *filename);
void ParamLogOut();

#endif

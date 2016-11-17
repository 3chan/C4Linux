#ifndef	_LOG_H_
#define	_LOG_H_

#include	<syslog.h>

int GetLogLevelValue(char *name);
void SetLogPath(char *path);
void SetLogName(char *name);
void SetLogLevel(int syslogLevel,int fileLevel);
void SetLogStderrOut(int flag);
void InitSyslog(int facility);
void Syslog(int priority,char *fmt,...);
void SyslogFix(int priority,char *msg);
void SyslogPerror(int priority,char *str);

#endif

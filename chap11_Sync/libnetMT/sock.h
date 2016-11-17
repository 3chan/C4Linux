#ifndef	_SOCK_H_
#define	_SOCK_H_

/* recvのデフォルトタイムアウト（秒） */
#define	RECV_TIMEOUT_SEC	(30)

/* プロトタイプ */
int ServerSocketNew(char *portnm,int s[],int maxsock);
int ClientSocketNew(char *hostnm,char *portnm);
int SetRecvTimeoutSec(int sec);
int RecvTimeoutPoll(int soc,char *buf,int bufsize,int flag);
int RecvOneLine_1(int soc,char *buf,int bufsize,int flag);
int RecvOneLine_2(int soc,char **ret_buf,int flag);
int SendSize(int soc,char *buf,int size);

#endif

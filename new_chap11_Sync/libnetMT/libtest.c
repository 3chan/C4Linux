#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>
#include	<netdb.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<arpa/inet.h>
#include	<netinet/in.h>
#include	<syslog.h>
#include	<poll.h>
#include	<errno.h>
#include	"token.h"
#include	"sock.h"
#include	"log.h"

int main()
{

	SetLogName("libtest");
	SetLogLevel(LOG_WARNING,LOG_DEBUG);
	InitSyslog(LOG_LOCAL7);
	SetLogStderrOut(1);

	return(TokenTest());
}

int TokenTest()
{
TOKEN	token;
char	buf[512];

	while(1){
		fgets(buf,sizeof(buf),stdin);
		if(feof(stdin)){
			break;
		}
		GetToken(buf,strlen(buf),&token," \t\r\n",",");
		fprintf(stderr,"token.no=%d\n",token.no);
		DebugToken(stderr,&token);
		GetToken(buf,strlen(buf),&token,", \t\r\n","");
		fprintf(stderr,"token.no=%d\n",token.no);
		DebugToken(stderr,&token);
		FreeToken(&token);
	}

	return(0);
}

int DebugToken(FILE *fp,TOKEN *token)
{
int	i;
char	buf[512];

	for(i=0;i<token->no;i++){
		sprintf(buf,"[%s]",token->token[i]);
		fprintf(fp,"%s",buf);
	}
	fprintf(fp,"\r\n");

	return(0);
}


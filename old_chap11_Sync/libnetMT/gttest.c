#include        <stdio.h>
#include        "token.h"

int main()
{
TOKEN   token;
char    buf[512];

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
		fprintf(stderr,"\n");
	}

	return(0);
}

int DebugToken(FILE *fp,TOKEN *token)
{
int     i;
char    buf[512];

	for(i=0;i<token->no;i++){
		sprintf(buf,"[%s]",token->token[i]);
		fprintf(fp,"%s",buf);
	}
	fprintf(fp,"\r\n");

	return(0);
}

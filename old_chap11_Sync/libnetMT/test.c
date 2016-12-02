#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        <unistd.h>
#include        <netdb.h>
#include        <sys/types.h>
#include        <sys/socket.h>
#include        <arpa/inet.h>
#include        <netinet/in.h>
#include        <syslog.h>
#include        <poll.h>
#include        <errno.h>
#include        "token.h"
#include        "sock.h"

int main()
{
Ipv6ServerTest("55557");

}

#define MAXSOCK 20

int Ipv6ServerTest(char *portnm)
{
int     ret;
int     s[MAXSOCK];
int     smax;
struct pollfd   targets[MAXSOCK];
int     end;
int     nready,acc;
struct sockaddr_storage from;
socklen_t       fromlen;
int     i;
char    hbuf[NI_MAXHOST],sbuf[NI_MAXSERV];
int     error;

        ret=0;

        smax=ServerSocketNew(portnm,s,MAXSOCK);
        if(smax==-1){
                return(-1);
        }

        for(i=0;i<smax;i++){
                targets[i].fd=s[i];
                targets[i].events=POLLIN|POLLERR;
        }

        do{
                end=0;
                switch((nready=poll(targets,smax,-1))){
                        case    -1:
                                if(errno!=EINTR){
                                        SyslogPerror(LOG_ERR,"poll");
                                        ret=-1;
                                        end=1;
                                }
                                break;
                        case    0:
                                Syslog(LOG_DEBUG,"timeout\n");
                                break;
                        default:
                                for(i=0;i<smax;i++){
                                        if(targets[i].revents&(POLLIN|POLLERR)){
                                                fromlen=sizeof(from);
                                                acc=accept(s[i],(struct sockaddr *)&from,&fromlen);
                                                if(acc<0){
                                                        continue;
                                                }
                                                error=getnameinfo((struct sockaddr *)&from,fromlen,hbuf,sizeof(hbuf),sbuf,sizeof(sbuf),NI_NUMERICHOST|NI_NUMERICSERV);
                                                if(error){
                                                        Syslog(LOG_ERR,"getnameinfo:error\n");
                                                }
                                                else{
                                                        Syslog(LOG_INFO,"accept (%d) from %s %s\n",i,hbuf,sbuf);
                                                }
                                                do{
							char	*buffer;
							int	ret;
							ret=RecvOneLine_2(acc,&buffer,0);
							fprintf(stderr,"ret=%d\n",ret);
						}while(ret!=-1);
                                                close(acc);
                                        }
                                }
                                break;
                }
        }while(end!=1);

        for(i=0;i<smax;i++){
                close(s[i]);
        }

        return(ret);
}

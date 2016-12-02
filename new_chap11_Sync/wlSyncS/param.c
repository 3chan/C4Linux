#include	<stdio.h>
#include	<ctype.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<string.h>
#include	<errno.h>
#include	<signal.h>
#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<arpa/inet.h>
#include	<netdb.h>
#include	<poll.h>
#include	<pthread.h>
#include	"sock.h"
#include	"token.h"
#include	"log.h"
#include	"param.h"

void ParamLogOut()
{
int	i;

	Syslog(LOG_INFO,"ParamLogOut:VERSION_STRING=%s\n",VERSION_STRING);
	Syslog(LOG_INFO,"ParamLogOut:TargetFolderCnt=%d\n",Param.TargetFolderCnt);
	for(i=0;i<Param.TargetFolderCnt;i++){
		Syslog(LOG_INFO,"ParamLogOut:TargetFolder%d=%s,%s\n",i,Param.TargetFolder[i].name,Param.TargetFolder[i].path);
	}
	Syslog(LOG_INFO,"ParamLogOut:TargetHostCnt=%d\n",Param.TargetHostCnt);
	for(i=0;i<Param.TargetHostCnt;i++){
		Syslog(LOG_INFO,"ParamLogOut:TargetHost%d=%s:%s\n",i,Param.TargetHost[i].host,Param.TargetHost[i].port);
	}
	Syslog(LOG_INFO,"ParamLogOut:Interval=%d\n",Param.Interval);
	Syslog(LOG_INFO,"ParamLogOut:RecvTimeout=%d\n",Param.RecvTimeout);
	Syslog(LOG_INFO,"ParamLogOut:UseDigest=%d\n",Param.UseDigest);
	Syslog(LOG_INFO,"ParamLogOut:SendSize=%d\n",Param.SendSize);
	Syslog(LOG_INFO,"ParamLogOut:LogPath=%s\n",Param.LogPath);
	Syslog(LOG_INFO,"ParamLogOut:LogLevel=%d\n",Param.LogLevel);
	Syslog(LOG_INFO,"ParamLogOut:StderrOut=%d\n",Param.StderrOut);
	Syslog(LOG_INFO,"ParamLogOut:VersionFilePath=%s\n",Param.VersionFilePath);
	Syslog(LOG_INFO,"ParamLogOut:WorkFilePath=%s\n",Param.WorkFilePath);
	return;
}

int ReadParam(char *filename)
{
FILE    *fp;
char    buf[1024],*ptr,org[1024];
int	no,i;

        if((fp=fopen(filename,"r"))==NULL){
                fprintf(stderr,"cannot read %s\n",filename);
                return(-1);
        }

	Param.TargetFolder=NULL;
	Param.TargetFolderCnt=0;

        while(1){
                if(fgets(buf,sizeof(buf),fp)==NULL){
                        break;
                }
                if(buf[0]=='#'){
                        continue;
                }
		strcpy(org,buf);
                if((ptr=strtok(buf,"="))==NULL){
                        continue;
                }
                if(strcmp(buf,"TargetFolderCnt")==0){
                        if((ptr=strtok(NULL,"\r\n"))!=NULL){
                                Param.TargetFolderCnt=atoi(ptr);
				Param.TargetFolder=(FOLDER *)calloc(Param.TargetFolderCnt,sizeof(FOLDER));
                        }
                }
                else if(strncmp(buf,"TargetFolder",strlen("TargetFolder"))==0){
			no=atoi(&buf[strlen("TargetFolder")]);
			if(no<0){
				Syslog(LOG_ERR,"ReadParam:TargetFolder:%d < 0\n",no);
				fclose(fp);
				return(-1);
			}
			else if(no>=Param.TargetFolderCnt){
				Syslog(LOG_ERR,"ReadParam:TargetFolder:%d >= TargetFolderCnt(%d)\n",no,Param.TargetFolderCnt);
				fclose(fp);
				return(-1);
			}
			else{
				if((ptr=strtok(NULL,","))!=NULL){
					Param.TargetFolder[no].name=strdup(ptr);
					if((ptr=strtok(NULL,"\r\n"))!=NULL){
						Param.TargetFolder[no].path=strdup(ptr);
					}
					else{
						Syslog(LOG_ERR,"ReadParam:TargetFolder%d:%s:no path\n",no,Param.TargetFolder[no].name);
						free(Param.TargetFolder[no].name);
						Param.TargetFolder[no].name=NULL;
						fclose(fp);
						return(-1);
					}
				}
				else{
					Syslog(LOG_ERR,"ReadParam:TargetFolder%d:no ','\n",no);
					fclose(fp);
					return(-1);
				}
			}
                }
                if(strcmp(buf,"TargetHostCnt")==0){
                        if((ptr=strtok(NULL,"\r\n"))!=NULL){
                                Param.TargetHostCnt=atoi(ptr);
				Param.TargetHost=(HOST *)calloc(Param.TargetHostCnt,sizeof(HOST));
                        }
                }
                else if(strncmp(buf,"TargetHost",strlen("TargetHost"))==0){
			no=atoi(&buf[strlen("TargetHost")]);
			if(no<0){
				Syslog(LOG_ERR,"ReadParam:TargetHost:%d < 0\n",no);
				fclose(fp);
				return(-1);
			}
			else if(no>=Param.TargetHostCnt){
				Syslog(LOG_ERR,"ReadParam:TargetHost:%d >= TargetHosterCnt(%d)\n",no,Param.TargetHostCnt);
				fclose(fp);
				return(-1);
			}
			else{
				if(strchr(org,'[')!=NULL&&strchr(org,']')!=NULL){
					if((ptr=strtok(NULL,"]"))!=NULL&&*ptr=='['){
						Param.TargetHost[no].host=strdup(ptr+1);
						if((ptr=strtok(NULL,":\r\n"))!=NULL){
							Param.TargetHost[no].port=strdup(ptr);
						}
						else{
							Syslog(LOG_ERR,"ReadParam:TargetHost%d:%s:no port\n",no,Param.TargetHost[no].host);
							free(Param.TargetHost[no].host);
							Param.TargetHost[no].host=NULL;
							fclose(fp);
							return(-1);
						}
					}
				}
				else if((ptr=strtok(NULL,":"))!=NULL){
					Param.TargetHost[no].host=strdup(ptr);
					if((ptr=strtok(NULL,"\r\n"))!=NULL){
						Param.TargetHost[no].port=strdup(ptr);
					}
					else{
						Syslog(LOG_ERR,"ReadParam:TargetHost%d:%s:no port\n",no,Param.TargetHost[no].host);
						free(Param.TargetHost[no].host);
						Param.TargetHost[no].host=NULL;
						fclose(fp);
						return(-1);
					}
				}
				else{
					Syslog(LOG_ERR,"ReadParam:TargetHost%d:no ':'\n",no);
					fclose(fp);
					return(-1);
				}
			}
                }
                else if(strcmp(buf,"Interval")==0){
                        if((ptr=strtok(NULL,"\r\n"))!=NULL){
                                Param.Interval=atoi(ptr);
                        }
                }
                else if(strcmp(buf,"RecvTimeout")==0){
                        if((ptr=strtok(NULL,"\r\n"))!=NULL){
                                Param.RecvTimeout=atoi(ptr);
                        }
                }
                else if(strcmp(buf,"UseDigest")==0){
                        if((ptr=strtok(NULL,"\r\n"))!=NULL){
                                Param.UseDigest=atoi(ptr);
                        }
                }
                else if(strcmp(buf,"SendSize")==0){
                        if((ptr=strtok(NULL,"\r\n"))!=NULL){
                                Param.SendSize=atoi(ptr);
                        }
                }
                else if(strcmp(buf,"LogPath")==0){
                        if((ptr=strtok(NULL,"\r\n"))!=NULL){
                                Param.LogPath=strdup(ptr);
                        }
                }
                else if(strcmp(buf,"LogLevel")==0){
                        if((ptr=strtok(NULL,"\r\n"))!=NULL){
                                Param.LogLevel=GetLogLevelValue(ptr);
                        }
                }
                else if(strcmp(buf,"SlaveRecvTimeout")==0){
                        if((ptr=strtok(NULL,"\r\n"))!=NULL){
                                Param.LogLevel=atoi(ptr);
				if(Param.LogLevel==-1){
					Param.LogLevel=LOG_INFO;
				}
                        }
                }
                else if(strcmp(buf,"StderrOut")==0){
                        if((ptr=strtok(NULL,"\r\n"))!=NULL){
                                Param.StderrOut=atoi(ptr);
                        }
                }
                else if(strcmp(buf,"VersionFilePath")==0){
                        if((ptr=strtok(NULL,"\r\n"))!=NULL){
                                Param.VersionFilePath=strdup(ptr);
                        }
                }
                else if(strcmp(buf,"WorkFilePath")==0){
                        if((ptr=strtok(NULL,"\r\n"))!=NULL){
                                Param.WorkFilePath=strdup(ptr);
                        }
                }
	}

	fclose(fp);

	for(i=0;i<Param.TargetFolderCnt;i++){
		if(Param.TargetFolder[i].name==NULL||Param.TargetFolder[i].path==NULL){
			Syslog(LOG_ERR,"ReadParam:TargetFolder%d:no data\n",no);
			return(-1);
		}
	}

	for(i=0;i<Param.TargetHostCnt;i++){
		if(Param.TargetHost[i].host==NULL||Param.TargetHost[i].port==NULL){
			Syslog(LOG_ERR,"ReadParam:TargetHost%d:no data\n",no);
			return(-1);
		}
	}

	return(0);
}


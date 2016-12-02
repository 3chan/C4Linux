#include    <stdio.h>
#include    <ctype.h>
#include    <unistd.h>
#include    <stdlib.h>
#include    <string.h>
#include    <errno.h>
#include    <signal.h>
#include    <sys/param.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include    <netinet/in.h>
#include    <arpa/inet.h>
#include    <netdb.h>
#include    <poll.h>
#include    <fcntl.h>
#include    <sys/mman.h>
#include    <sys/stat.h>
#include    <dirent.h>
#include    <utime.h>
#include    <pthread.h>
#include    <ipc.h>  // added
#include    "sock.h"
#include    "token.h"
#include    "jconv.h"
#include    "log.h"
#include    "lockf.h"
#define WLSYNCC_MAIN
#include    "param.h"

#define	SOCK_NO	16

PARAM Param={
  0,
  NULL,
  "56000",
  5,
  0,
  "./",
  LOG_DEBUG,
  1,
  "./wlSyncC.ver",
  "./wlSyncC.work"
};

typedef struct {
  int	acc;
  unsigned long	ServerId;
}THREAD_DATA;

int SigEnd=0;

// added
int g_MapSize_recv;

void ending(int sig);
void endingFunc(int sig);
int DeleteVersionFile();
int MakeVersionFile();
int MakeWorkFolder();
int AcceptLoop(int s[],int s_no);
void *RecvLoop(void *arg);
int SendToRemote(int acc,char *data);
int CheckRecvData(THREAD_DATA *td,char *data);
int DoName(THREAD_DATA *td,char *data);
char *SearchCrLf(char *sptr,char *eptr);
int SyncData(int acc,char *workfile);
int SendKeepAlive(int acc);
int CheckEraseData(int acc,int targetNo,char *mptr,char *mapEnd,char *code);
int CheckEraseOneFolder(int acc,char *tpath,char *path,char *mptr,char *tend,char *code);
int DoDelete(char *fullpath);
int SearchOne(char d_f,char *spath,char *mptr,char *tend,char *code);
int SkipOneFolder(char **mptr);
int SyncOneFolder(int acc,int targetNo,char *code,char **mptr,char *mapEnd);
int MakeTargetFolder(int targetNo);
int MakeDirectory(char *path);
int SyncOneFolder_D(int acc,int targetNo,char *path);
int SyncOneFolder_F(int acc,int targetNo,char *code,char *stime,char *ssize,char *sdigest,char *path);
int GetOneFile(int acc,int targetNo,char *code,char *path,unsigned long timestamp);
int CleanFilename(char *name);

// added
void *ipc_listen(void *arg);
void NoopHandler(MSG_INSTANCE ref, void *data, void *dummy);
//void MsgHandler(MSG_INSTANCE ref, void *data, void *dummy);
void MapSizeHandler(MSG_INSTANCE ref, void *data, void *dummy);
//void MapPtrHandler(MSG_INSTANCE ref, void *data, void *dummy);
void MapPtrHandler(MSG_INSTANCE ref, void *data, void *dummy);

void ending(int sig) {
  SigEnd=sig;
}

void endingFunc(int sig) {
  Syslog(LOG_ERR,"endingFunc:sig=%d\n",sig);
  exit(0);
}

int main(int argc,char *argv[]) {
  int	s[SOCK_NO];
  int	s_no,ret,i;

  // added
  char noop[10];
  //char msg[10];
  
  // added
  /* Connect to Central Server */
  IPC_connect("wlSyncC"); 
  
  // added
  /* Define Message */
  //IPC_defineMsg("NOOP", IPC_VARIABLE_LENGTH, "[char: 10]");
  IPC_defineMsg("NOOP", IPC_VARIABLE_LENGTH, "string");
  IPC_defineMsg("MAPSIZE", IPC_VARIABLE_LENGTH, "int");
  //IPC_defineMsg("MAPPTR", IPC_VARIABLE_LENGTH, "string");
  IPC_defineMsg("msg0", IPC_VARIABLE_LENGTH, "string");

  // added
  /* Receive Message */
  IPC_subscribeData("MAPSIZE", MapSizeHandler, NULL);
  //IPC_subscribeData("MAPPTR", MapPtrHandler, NULL);
  IPC_subscribeData("msg0", MapPtrHandler, NULL);
  
  // added
  /* Create a thread for IPC_listen */
  pthread_t thread_listen;
  char *message = "Thread for IPC_listen";
  pthread_create(&thread_listen, NULL, ipc_listen, (void *)message);  // 第4引数は必須 or not??
  sleep(1);

  if(argc>1&&strcmp(argv[1],"-d")==0){
    daemon(1,0);
  }
  
  //added

  DeleteVersionFile();
  
  /* パラメータ読み込み */
  ret=ReadParam(PARAM_PATH);
  if(ret==-1){
    Syslog(LOG_ERR,"main:Cannot read parameter\n");
    return(-1);
  }
  SetLogStderrOut(Param.StderrOut);
  SetLogPath(Param.LogPath);
  
  MakeDirectory(Param.LogPath);
  
  SetLogName(APP_NAME);
  SetLogLevel(-1,Param.LogLevel);
  InitSyslog(LOG_LOCAL7);
  
  ParamLogOut();
  
  SetRecvTimeoutSec(Param.RecvTimeout*60);
  
  MakeWorkFolder();
  
  if((s_no=ServerSocketNew(Param.Port,s,SOCK_NO))==-1){
    Syslog(LOG_ERR,"main:ServerSocketNew(%s):error\n",Param.Port);
    return(-1);
  }
  
  signal(SIGTERM,ending);
  signal(SIGQUIT,ending);
  signal(SIGTERM,ending);
  signal(SIGINT,ending);
  
  MakeVersionFile();
  
  Syslog(LOG_INFO,"main:wlSyncC:ready\n");
  
  AcceptLoop(s,s_no);
  
  for(i=0;i<s_no;i++){
    close(s[i]);
  }
  
  // added
  IPC_disconnect();
  pthread_join(thread_listen, NULL);
  
  return(0);
}

int DeleteVersionFile() {
  Syslog(LOG_DEBUG,"DeleteVersionFile:%s\n",Param.VersionFilePath);
  
  unlink(Param.VersionFilePath);
  
  return(0);
}

int MakeVersionFile() {
  FILE	*fp;
  
  Syslog(LOG_DEBUG,"MakeVersionFile:%s\n",Param.VersionFilePath);
  
  if((fp=fopen(Param.VersionFilePath,"w"))==NULL){
    Syslog(LOG_ERR,"MakeVersionFile:Cannot write version file(%s)\n",Param.VersionFilePath);
    return(-1);
  }
  
  fprintf(fp,"%s\n",VERSION_STRING);
  
  fclose(fp);
  return(0);
}

int MakeWorkFolder() {
  MakeDirectory(Param.WorkFilePath);
  return(0);
}

int AcceptLoop(int s[],int s_no) {
  int acc;
  socklen_t fromlen;
  struct sockaddr_storage from;
  pthread_t thread_id;
  struct pollfd	targets[SOCK_NO];
  char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
  int i, nready;

  Syslog(LOG_DEBUG,"AcceptLoop:s_no=%d\n",s_no);
  
  for(i=0;i<s_no;i++){
    targets[i].fd=s[i];
    targets[i].events=POLLIN|POLLERR;
  }
  
  while(1){
    // added
    /* Send Noop */
    char noop[10];
    sprintf(noop, "#Noop");
    IPC_publishData("NOOP", &noop);
    Syslog(LOG_DEBUG, "NoopHandler:%s", noop);

    if(SigEnd){
      endingFunc(SigEnd);
      SigEnd=0;
    }
    switch((nready=poll(targets,s_no,1000))){
    case	-1:
      if(errno!=EINTR){
	SyslogPerror(LOG_ERR,"poll");
	return(-1);
      }
      break;
    case	0:
      break;
    default:
      for(i=0;i<s_no;i++){
	if(targets[i].revents&(POLLIN|POLLERR)){
	  fromlen=sizeof(from);
	  acc=accept(s[i],(struct sockaddr *)&from,&fromlen);
	  if(acc<0){
	    if(errno!=EINTR){
	      SyslogPerror(LOG_ERR,"AcceptLoop:accept");
	    }
	  }
	  else{
	    if(getnameinfo((struct sockaddr *)&from,fromlen,hbuf,sizeof(hbuf),sbuf,sizeof(sbuf),NI_NUMERICHOST|NI_NUMERICSERV)){
	      Syslog(LOG_ERR,"getnameinfo:error\n");
	    }
	    else{
	      Syslog(LOG_INFO,"AcceptLoop:accept(%d):%s:%s\n",i,hbuf,sbuf);
	    }
	    if(pthread_create(&thread_id,NULL,RecvLoop,(void *)acc)==0){
	      Syslog(LOG_INFO,"AcceptLoop:pthread_create:thread_id=%u\n",thread_id);
	    }
	    else{
	      SyslogPerror(LOG_ERR,"AcceptLoop:pthread_create");
	    }
	  }
	}
      }
      break;
    }
  }
  return(0);
}

void *RecvLoop(void *arg) {
  THREAD_DATA td;
  char *buf;
  
  pthread_detach(pthread_self());
  
  td.acc=(int)arg;
  td.ServerId=pthread_self();
  
  Syslog(LOG_INFO,"RecvLoop:start:acc=%d\n",td.acc);
  
  while(1){
    if(RecvOneLine_2(td.acc,&buf,0)<=0){
      Syslog(LOG_ERR,"RecvLoop:RecvOneLine_2:error or closed\n");
      break;
    }
    CutCrLf(buf);
    Syslog(LOG_DEBUG,"RecvLoop:RecvOneLine_2:%s\n",buf);
    if(CheckRecvData(&td,buf)!=-1){
      free(buf);
      break;
    }
    free(buf);
  }

  close(td.acc);
  
  Syslog(LOG_INFO,"RecvLoop:end\n");
  return(NULL);
}

int SendToRemote(int acc,char *data) {
  int ret;
  
  Syslog(LOG_DEBUG,"SendToRemote:acc=%d,data=%s\n",acc,data);
  
  ret=SendSize(acc,data,strlen(data));
  
  return(ret);
}

int CheckRecvData(THREAD_DATA *td,char *data) {
  int ret;
 
  Syslog(LOG_INFO,"CheckRecvData:td.acc=%d,data=%s\n",td->acc,data);
  
  if(StrNCmp(data,"#Name=",strlen("#Name="))==0){
    ret=DoName(td,data);
    return(ret);
  }
  else{
    Syslog(LOG_ERR,"CheckRecvData:error:data==%s\n",data);
    return(-1);
  }
}

int DoName(THREAD_DATA *td,char *data) {
  int fd;
  char *buf, *workfile;

  workfile=(char *)malloc(strlen(Param.WorkFilePath)+16+1+1);
  sprintf(workfile,"%s/%016lu",Param.WorkFilePath,td->ServerId);
  Syslog(LOG_INFO,"DoName:workfile=%s",workfile);
  
  if((fd=open(workfile,O_WRONLY|O_CREAT,0666))==-1){
    Syslog(LOG_ERR,"DoName:open:error\n");
    return(-1);
  }
  // ac: 受信データを1行ずつ workfile へ書き込む
  write(fd,data,strlen(data));
  write(fd,"\n",1);
  while(1){
    if(RecvOneLine_2(td->acc,&buf,0)<=0){
      Syslog(LOG_ERR,"DoName:RecvOneLine_2:error or closed\n");
      close(fd);
      return(-1);
    }
    CutCrLf(buf);
    if(buf[0]=='#'){
      Syslog(LOG_INFO,"DoName:RecvOneLine_2:%s\n",buf);
    }
    else{
      Syslog(LOG_DEBUG,"DoName:RecvOneLine_2:%s\n",buf);
    }
    write(fd,buf,strlen(buf));
    write(fd,"\n",1);
    if(StrCmp(buf,"#End")==0){
      free(buf);
      break;
    }
    free(buf);
  }
  
  close(fd);
  
  SendToRemote(td->acc,"#OK\r\n");

  // ac: ディレクトリリストを同期
  SyncData(td->acc,workfile);

  unlink(workfile);
  free(workfile);
  
  return(1);
}

// added
/* IPCライブラリでMapSizeを同期 */
void MapSizeHandler(MSG_INSTANCE ref, void *data, void *dummy) {
  //int MapSize_recv;
  g_MapSize_recv = *(int*)data;
  printf("received MapSize = %d\n", g_MapSize_recv);
}

// added
// ここから
//void MapPtrHandler(MSG_INSTANCE ref, void *data, void *dummy) {
void MapPtrHandler(MSG_INSTANCE ref, void *data, void *dummy) {
  char MapPtr_recv[1024];
  printf("coming MapPtrHandler\n");

  /*
  char **strPtr;
  printf("ok01\n");
  IPC_RETURN_TYPE ret;

  if (IPC_unmarshall(IPC_msgInstanceFormatter(ref), data,
                 (void **)(void *)&strPtr) == IPC_Illegal_Formatter) {
    printf("IPC_Illegal_Formatter\n");
    //  } else if (ret == IPC_Not_Initialized) {
    //    printf("IPC_Not_Initialized\n");
  } else {
    printf("IPC_OK\n");
  }

  printf("ok02\n");
  
  printf("Handling %s: Receiving %s\n", IPC_msgInstanceName(ref), *strPtr);
  printf("ok03\n");
  free(data);
  printf("ok04\n");
  free(*strPtr); free(strPtr);
  printf("ok05\n");
  */

  /* 受信したMapPtrの内容を確認する */
  int i;
  for (i = 0; i < 10; i++) {
    //printf("MapPtr_recv[%d] = %d\n", i, ((char *)data)[i]);
    printf("MapPtr_recv[%d] = %d\n", i, *((char *)data+i));
  }
  
  //sprintf(MapPtr_recv, "%s", (char *)data);

  printf("g_MapSize_recv = %d\n", g_MapSize_recv);
  //printf("received MapPtr = %s"), MapPtr_recv;
  //int i;
  //for (i = 0; i < 10; i++) {
  //  printf("%c", (char*)data[i]);
  //}
  //printf("\n");
}


char *SearchCrLf(char *sptr,char *eptr) {
  char	*ptr;

  /*Syslog(LOG_DEBUG,"SearchCrLf:sptr=%x,eptr=%x\n",sptr,eptr);*/

  for(ptr=sptr;ptr<eptr;ptr++){
    if(*ptr=='\r'||*ptr=='\n'){
      return(ptr);
    }
  }
  
  return(NULL);
}

int SyncData(int acc,char *workfile) {
  int	fd;
  int	mapSize;
  char	*mptr, *mapEnd;
  struct stat	st;
  char	*buf, *ptr, *p;
  int	targetNo, i, len;
  TOKEN	token;

  Syslog(LOG_DEBUG,"SyncData:acc=%d",acc);
  
  if((fd=open(workfile,O_RDONLY))==-1){
    SyslogPerror(LOG_ERR,"SyncData:open");
    return(-1);
  }
  stat(workfile,&st);
  mapSize=st.st_size;
  if((mptr=(char *)mmap(0,mapSize,PROT_READ,MAP_SHARED,fd,0))==(char *)-1){
    SyslogPerror(LOG_ERR,"SyncData:mmap");
    close(fd);
    return(-1);
  }
  mapEnd=mptr+mapSize;
  
  for(ptr=mptr;ptr<mapEnd;ptr++){
    if(*ptr=='\r'||*ptr=='\n'){
      continue;
    }
    if((p=SearchCrLf(ptr,mapEnd))==NULL){
      break;
		}
    len=p-ptr;
    buf=(char *)malloc((len+1)*sizeof(char));
    memcpy(buf,ptr,len);
    buf[len]='\0';
    ptr=p;
    if(StrCmp(buf,"#End")==0){
      free(buf);
      break;
    }
    if(StrNCmp(buf,"#Name=",strlen("#Name="))==0){
      GetToken(buf,strlen(buf),&token,"\r\n","=,");
      if(token.no!=5){
	Syslog(LOG_ERR,"SyncData:GetToken:token.no(%d)!=5\n",token.no);
	FreeToken(&token);
	free(buf);
	continue;
      }
      else if(strcmp(token.token[1],"=")!=0){
	Syslog(LOG_ERR,"SyncData:token.token[1](%s)!=\"=\"\n",token.token[1]);
	FreeToken(&token);
	free(buf);
	continue;
      }
      else if(strcmp(token.token[3],",")!=0){
	Syslog(LOG_ERR,"SyncData:token.token[3](%s)!=\",\"\n",token.token[3]);
	FreeToken(&token);
	free(buf);
	continue;
      }
      
      targetNo=-1;
      for(i=0;i<Param.TargetFolderCnt;i++){
	if(strcmp(token.token[2],Param.TargetFolder[i].name)==0){
	  targetNo=i;
	  break;
	}
      }
      if(targetNo==-1){
	if(SkipOneFolder(&ptr)==-1){
	  Syslog(LOG_ERR,"SyncData:SkipOneFolder:error\n");
	  FreeToken(&token);
	  free(buf);
	  break;
	}
      }
      else{
	if(Param.DeleteFlag==1){
	  if(CheckEraseData(acc,targetNo,ptr,mapEnd,token.token[4])==-1){
	    Syslog(LOG_ERR,"SyncData:CheckEraseData:error\n");
	    FreeToken(&token);
	    free(buf);
	    break;
	  }
	}
	if(SyncOneFolder(acc,targetNo,token.token[4],&ptr,mapEnd)==-1){
	  Syslog(LOG_ERR,"SyncData:SyncOneFolder:error\n");
	  FreeToken(&token);
	  free(buf);
	  break;
	}
      }
      
      FreeToken(&token);
    }
    free(buf);
  }
  
  munmap(mptr,mapSize);
  close(fd);
  
  return(0);
}

int SendKeepAlive(int acc) {
  char	*buf;
  
  Syslog(LOG_INFO,"SendKeepAlive:Send \"#Noop\"\n");
  
  SendToRemote(acc,"#Noop\r\n");
  
  // added
  // Send Noop
  //char noop[10];
  //sprintf(noop, "#Noop");
  //IPC_publishData("NOOP", noop);
  
  if(RecvOneLine_2(acc,&buf,0)<=0){
    Syslog(LOG_ERR,"SendKeepAlive:RecvOneLine_2:error or closed\n");
    return(-1);
  }
  CutCrLf(buf);
  Syslog(LOG_DEBUG,"SendKeepAlive:RecvOneLine_2:%s\n",buf);
  if(StrCmp(buf,"#OK")!=0){
    Syslog(LOG_ERR,"SendKeepAlive:buf(%s)!=\"#OK\"\n",buf);
    free(buf);
    return(-1);
  }
  else{
    Syslog(LOG_INFO,"SendKeepAlive:OK\n");
    free(buf);
    return(0);
  }
}


// added
void NoopHandler(MSG_INSTANCE ref, void *data, void *dummy) {
  //char *noop_recv[10];
  char noop_recv[10];
  sprintf(noop_recv, "%s", (char *)data);
}

int CheckEraseData(int acc,int targetNo,char *mptr,char *mapEnd,char *code) {
  char	*p,*tend;
 
  Syslog(LOG_DEBUG,"CheckEraseData:targetNo=%d,mptr=%x,mapEnd=%x,code=%s",targetNo,mptr,mapEnd,code);
  
  if((p=strstr(mptr,"\n#"))==NULL){
    tend=mapEnd;
  }
  else{
    tend=p+1;
  }

  if(CheckEraseOneFolder(acc,Param.TargetFolder[targetNo].path,"",mptr,tend,code)==-1){
    Syslog(LOG_ERR,"CheckEraseData:CheckEraseOneFolder:error\n");
    return(-1);
  }
  else{
    return(0);
  }
}

int CheckEraseOneFolder(int acc,char *tpath,char *path,char *mptr,char *tend,char *code) {
  char	*fullpath,*spath;
  DIR	*dir;
  struct dirent	*dp;
  struct stat	st;
  int	count;
  
  Syslog(LOG_INFO,"CheckEraseOneFolder:tpath=%s,path=%s,mptr=%x,tend=%x,code=%s\n",tpath,path,mptr,tend,code);
  
  fullpath=(char *)malloc(strlen(tpath)+strlen(path)+2);
  if(strlen(path)>0){
    sprintf(fullpath,"%s/%s",tpath,path);
  }
  else{
    strcpy(fullpath,tpath);
  }
  
  if((dir=opendir(fullpath))==NULL){
    free(fullpath);
    return(0);
  }
  free(fullpath);
  
  count=0;
  for(dp=readdir(dir);dp!=NULL;dp=readdir(dir)){
    count++;
    if(count>10){
      count=0;
      if(SendKeepAlive(acc)==-1){
	Syslog(LOG_ERR,"CheckEraseOneFolder:SendKeepAlive:error\n");
	closedir(dir);
	return(-1);
      }
    }
    
    if(strcmp(dp->d_name,".")==0||strcmp(dp->d_name,"..")==0){
      continue;
    }
    fullpath=(char *)malloc(strlen(tpath)+strlen(path)+strlen(dp->d_name)+3);
    sprintf(fullpath,"%s/%s/%s",tpath,path,dp->d_name);
    CleanFilename(fullpath);
    Syslog(LOG_INFO,"CheckEraseOneFolder:fullpath=%s\n",fullpath);
    if(stat(fullpath,&st)!=-1){
      if(S_ISDIR(st.st_mode)){
	if(strlen(path)>0){
	  spath=(char *)malloc(strlen(path)+strlen(dp->d_name)+1+1);
	  sprintf(spath,"%s/%s",path,dp->d_name);
	  CleanFilename(spath);
	}
	else{
	  spath=strdup(dp->d_name);
	}
	if(SearchOne('D',spath,mptr,tend,code)==1){
	  CheckEraseOneFolder(acc,tpath,spath,mptr,tend,code);
	}
	else{
	  DoDelete(fullpath);
	}
	free(spath);
      }
      else{
	if(strlen(path)>0){
	  spath=(char *)malloc(strlen(path)+strlen(dp->d_name)+1+1);
	  sprintf(spath,"%s/%s",path,dp->d_name);
	  CleanFilename(spath);
	}
	else{
	  spath=strdup(dp->d_name);
	}
	if(SearchOne('F',spath,mptr,tend,code)==0){
	  DoDelete(fullpath);
	}
	free(spath);
      }
    }
    free(fullpath);
  }
  
  closedir(dir);
  
  return(1);
}

int DoDelete(char *fullpath) {
  char	*buf;
  
  Syslog(LOG_DEBUG,"DoDelete:fullpath=%s\n",fullpath);
  
  buf=(char *)malloc(strlen("rm -rf ")+strlen(fullpath)+2+1);
  sprintf(buf,"rm -rf '%s'",fullpath);
  Syslog(LOG_INFO,"DoDelete:%s\n",buf);
  system(buf);
  free(buf);
  
  return(0);
}

int SearchOne(char d_f,char *spath,char *mptr,char *tend,char *code) {
  char	*ptr,*p,*buf,*buf2,*startp;
  int	find,len;
  int	sjisFlag;
  
  Syslog(LOG_DEBUG,"SearchOne:d_f=%c,spath=%s,mptr=%x,tend=%x,code=%s\n",d_f,spath,mptr,tend,code);
  
  if(StrCmp(code,"SJIS")==0){
    sjisFlag=1;
  }
  else{
    sjisFlag=0;
  }
  
  Syslog(LOG_DEBUG,"SearchOne:spath=%s\n",spath);
  
  find=0;
  for(ptr=mptr;ptr<tend;ptr++){
    if(*ptr=='\r'||*ptr=='\n'){
      continue;
    }
    else if((p=SearchCrLf(ptr,tend))==NULL){
      break;
    }
    else if(d_f!=*ptr){
      ptr=p;
      continue;
    }
    len=p-ptr;
    buf=(char *)malloc((len+1)*sizeof(char));
    memcpy(buf,ptr,len);
    buf[len]='\0';
    if(sjisFlag){
      buf2=(char *)malloc((len*2+1)*sizeof(char));
      sjtouj(buf2,len*2+1,buf);
      free(buf);
      buf=buf2;
    }
    ptr=p;
    if(d_f=='D'){
      startp=strchr(buf,':');
    }
    else{
      startp=strrchr(buf,',');
    }
    if(startp==NULL){
      free(buf);
      continue;
    }
    startp++;
    CleanFilename(startp);
    Syslog(LOG_DEBUG,"SearchOne:%s:%s\n",startp,spath);
    if(strcmp(startp,spath)==0){
      find=1;
      free(buf);
      break;
    }
    free(buf);
  }
  
  Syslog(LOG_DEBUG,"SearchOne:%c,%s:%d\n",d_f,spath,find);
  
  return(find);
}

int SkipOneFolder(char **mptr) {
  char	*p;
  
  Syslog(LOG_DEBUG,"SkipOneFolder:*mptr=%x\n",*mptr);
  
  if((p=strstr(*mptr,"\n#"))==NULL){
    return(-1);
  }
  *mptr=p+1;
  
  return(0);
}

int SyncOneFolder(int acc,int targetNo,char *code,char **mptr,char *mapEnd) {
  char	*ptr,*buf,*buf2,*p;
  int	len;
  TOKEN	token;
  int	sjisFlag,count;
  
  Syslog(LOG_ERR,"SyncOneFolder:acc=%d,targetNo=%d,code=%s,*mptr=%x,mapEnd=%x\n",acc,targetNo,code,*mptr,mapEnd);
  
  if(StrCmp(code,"SJIS")==0){
    sjisFlag=1;
  }
  else{
    sjisFlag=0;
  }
  
  MakeTargetFolder(targetNo);
  
  count=0;
  for(ptr=*mptr;ptr<mapEnd;ptr++){
    count++;
    if(count>10){
      count=0;
      if(SendKeepAlive(acc)==-1){
	Syslog(LOG_ERR,"SyncOneFolder:SendKeepAlive:error\n");
	return(-1);
      }
    }
    
    if(*ptr=='\r'||*ptr=='\n'){
      continue;
    }
    else if(*ptr=='#'){
      *mptr=ptr-1;
      break;
    }
    else if((p=SearchCrLf(ptr,mapEnd))==NULL){
      Syslog(LOG_ERR,"SyncOneFolder:SearchCrLf:NULL\n");
      return(-1);
    }
    len=p-ptr;
    buf=(char *)malloc((len+1)*sizeof(char));
    memcpy(buf,ptr,len);
    buf[len]='\0';
    ptr=p;
    
    if(sjisFlag){
      buf2=(char *)malloc((len*2+1)*sizeof(char));
      sjtouj(buf2,len*2+1,buf);
      free(buf);
      buf=buf2;
    }
    
    Syslog(LOG_DEBUG,"SyncOneFolder:%s\n",buf);
    
    if(*buf=='D'){
      GetToken(buf,strlen(buf),&token,"\r\n",":");
      if(token.no!=3){
	Syslog(LOG_ERR,"SyncOneFolder:token.no!=3\n");
	FreeToken(&token);
	free(buf);
	return(-1);
      }
      else if(strcmp(token.token[1],":")!=0){
	Syslog(LOG_ERR,"SyncOneFolder:bad format\n");
	FreeToken(&token);
	free(buf);
	return(-1);
      }
      else if(SyncOneFolder_D(acc,targetNo,token.token[2])==-1){
	Syslog(LOG_ERR,"SyncOneFolder:SyncOneFolder_D:error\n");
	FreeToken(&token);
	free(buf);
	return(-1);
      }
      FreeToken(&token);
    }
    else if(*buf=='F'){
      GetToken(buf,strlen(buf),&token,"\r\n",":,");
      if(token.no!=9){
	Syslog(LOG_ERR,"SyncOneFolder:token.no!=9\n");
	FreeToken(&token);
	free(buf);
	return(-1);
      }
      else if(strcmp(token.token[1],":")!=0||strcmp(token.token[3],",")!=0||strcmp(token.token[5],",")!=0||strcmp(token.token[7],",")!=0){
	Syslog(LOG_ERR,"SyncOneFolder:bad format\n");
	FreeToken(&token);
	free(buf);
	return(-1);
      }
      else if(SyncOneFolder_F(acc,targetNo,code,token.token[2],token.token[4],token.token[6],token.token[8])==-1){
	Syslog(LOG_ERR,"SyncOneFolder:SyncOneFolder_F:error\n");
	FreeToken(&token);
	free(buf);
	return(-1);
      }
      FreeToken(&token);
    }
    free(buf);
  }
  
  Syslog(LOG_ERR,"SyncOneFolder:normal end\n");
  
  return(0);
}

int MakeTargetFolder(int targetNo) {
  struct stat	st;
  
  Syslog(LOG_DEBUG,"MakeTargetFolder:targetNo=%d\n",targetNo);
  
  if(stat(Param.TargetFolder[targetNo].path,&st)==-1){
    MakeDirectory(Param.TargetFolder[targetNo].path);
  }
  
  return(0);
}

int MakeDirectory(char *path) {
  char	*ptr, *p;
  
  ptr=path;
  while(1){
    if((p=strchr(ptr,'/'))==NULL){
      break;
    }
    *p='\0';
    mkdir(path,0777);
    *p='/';
    ptr=p+1;
  }
  mkdir(path,0777);
  
  return(0);
}

int SyncOneFolder_D(int acc,int targetNo,char *path) {
  char	*fullpath;
  struct stat	st;
  
  Syslog(LOG_DEBUG,"SyncOneFolder_D:acc=%d,targetNo=%d,path=%s\n",acc,targetNo,path);
  
  fullpath=(char *)malloc(strlen(Param.TargetFolder[targetNo].path)+strlen(path)+1+2+1);
  sprintf(fullpath,"%s/%s",Param.TargetFolder[targetNo].path,path);
  if(stat(fullpath,&st)==-1){
    MakeDirectory(fullpath);
  }
  free(fullpath);
  
  return(0);
}

int SyncOneFolder_F(int acc,int targetNo,char *code,char *stime,char *ssize,char *sdigest,char *path) {
  char	*fullpath;
  struct stat	st;
  int	ret;
  
  Syslog(LOG_DEBUG,"SyncOneFolder_F:acc=%d,targetNo=%d,code=%s,stime=%s,ssize=%s,sdigest=%s,path=%s\n",acc,targetNo,code,stime,ssize,sdigest,path);
  
  fullpath=(char *)malloc(strlen(Param.TargetFolder[targetNo].path)+strlen(path)+2);
  sprintf(fullpath,"%s/%s",Param.TargetFolder[targetNo].path,path);
  Syslog(LOG_DEBUG,"SyncOneFolder_F:%s\n",fullpath);
  
  if(stat(fullpath,&st)==-1){
    Syslog(LOG_INFO,"SyncOneFolder_F:%s:stat=-1:Get\n",fullpath);
    ret=GetOneFile(acc,targetNo,code,path,atol(stime));
  }
  else if(st.st_size!=atoll(ssize)){
    Syslog(LOG_INFO,"SyncOneFolder_F:%s:st.st_size(%d)!=size(%lld):Get\n",fullpath,st.st_size,atoll(ssize));
    ret=GetOneFile(acc,targetNo,code,path,atol(stime));
	}
  else if(st.st_mtime<atol(stime)){
    Syslog(LOG_INFO,"SyncOneFolder_F:%s:st.st_mtime(%d)<time(%ld):Get\n",fullpath,st.st_mtime,atol(stime));
    ret=GetOneFile(acc,targetNo,code,path,atol(stime));
  }
  else{
    Syslog(LOG_INFO,"SyncOneFolder_F:%s:exists\n",fullpath);
    ret=0;
  }
  
  free(fullpath);
  
  return(ret);
}

int GetOneFile(int acc,int targetNo,char *code,char *path,unsigned long timestamp) {
  char	*buf,*buf2,*data,*ptr;
  TOKEN	token;
  long long	size,lestSize,recvSize,len,dataSize;
  char	*fullpath;
  int	fd;
  int	lf;
  struct utimbuf	ut;
  
  Syslog(LOG_INFO,"GetOneFile:acc=%d,targetNo=%d,code=%s,path=%s\n",acc,targetNo,code,path);
  
  buf=(char *)malloc(strlen("#Get=")+3+strlen(Param.TargetFolder[targetNo].name)+strlen(path)+1);
  sprintf(buf,"#Get=%s,%s\r\n",Param.TargetFolder[targetNo].name,path);
  Syslog(LOG_DEBUG,"GetOneFile:buf=%s\n",buf);
  if(StrCmp(code,"SJIS")==0){
    Syslog(LOG_DEBUG,"GetOneFile:SJIS\n");
    buf2=(char *)malloc(strlen(buf)*2+1);
    ujtosj(buf2,strlen(buf)*2+1,buf);
    free(buf);
    buf=buf2;
  }
  SendToRemote(acc,buf);
  free(buf);
  
  while(1){
    if(RecvOneLine_2(acc,&buf,0)<=0){
      Syslog(LOG_ERR,"GetOneFile:RecvOneLine_2:error or closed\n");
      return(-1);
    }
    else if((ptr=strchr(buf,'#'))!=NULL){
      break;
    }
    free(buf);
  }
  CutCrLf(ptr);
  Syslog(LOG_DEBUG,"GetOneFile:RecvOneLine_2:%s\n",ptr);
  if(StrNCmp(ptr,"#Error=",strlen("#Error="))==0){
    Syslog(LOG_ERR,"GetOneFile:%s",ptr);
    free(buf);
    return(0);
  }
  else{
    fullpath=(char *)malloc(strlen(Param.TargetFolder[targetNo].path)+strlen(path)+2);
    sprintf(fullpath,"%s/%s",Param.TargetFolder[targetNo].path,path);
    Syslog(LOG_DEBUG,"GetOneFile:open:%s\n",fullpath);

    lf=LockFile(fullpath);
    if((fd=open(fullpath,O_WRONLY|O_CREAT,0666))==-1){
      SyslogPerror(LOG_ERR,"GetOneFile:open\n");
      free(fullpath);
      free(buf);
      UnlockFile(lf);
      return(-1);
    }
    
    do{
      GetToken(ptr,strlen(ptr),&token,"\r\n","=,");
      if(token.no!=5){
	Syslog(LOG_ERR,"GetOneFile:token.no!=5:unlink\n");
	unlink(fullpath);
	free(fullpath);
	FreeToken(&token);
	free(buf);
	close(fd);
	UnlockFile(lf);
	return(-1);
      }
      else if(strcmp(token.token[1],"=")!=0){
	Syslog(LOG_ERR,"GetOneFile:token.token[1]!=\"=\":unlink\n");
	unlink(fullpath);
	free(fullpath);
	FreeToken(&token);
	free(buf);
	close(fd);
	UnlockFile(lf);
	return(-1);
      }
      else if(strcmp(token.token[3],",")!=0){
	Syslog(LOG_ERR,"GetOneFile:token.token[1]!=\",\":unlink\n");
	unlink(fullpath);
	free(fullpath);
	FreeToken(&token);
	free(buf);
	close(fd);
	UnlockFile(lf);
	return(-1);
      }
      size=atoll(token.token[2]);
      Syslog(LOG_DEBUG,"GetOneFile:size=%lld\n",size);
      
      if(size<0){
	Syslog(LOG_ERR,"GetOneFile:size<0:unlink\n");
	unlink(fullpath);
	free(fullpath);
	FreeToken(&token);
	free(buf);
	close(fd);
	UnlockFile(lf);
	return(-1);
      }
      else if(size==0){
	Syslog(LOG_DEBUG,"GetOneFile:size=0:end\n");
	FreeToken(&token);
	free(buf);
	break;
      }
      else{
	data=malloc(size);
	dataSize=size;
	
	FreeToken(&token);
	free(buf);
	
	lestSize=size;
	do{
	  if(lestSize<dataSize){
	    recvSize=lestSize;
	  }
	  else{
	    recvSize=dataSize;
	  }
	  len=RecvTimeoutPoll(acc,data,recvSize,0);
	  Syslog(LOG_DEBUG,"GetOneFile:RecvTimeoutPoll=%d\n",len);
	  if(len<=0){
	    Syslog(LOG_ERR,"GetOneFile:RecvTimeoutPoll<=0:unlink\n");
	    unlink(fullpath);
	    free(fullpath);
	    close(fd);
	    UnlockFile(lf);
	    return(-1);
	  }
	  write(fd,data,len);
	  lestSize-=len;
	  /*Syslog(LOG_DEBUG,"GetOneFile:lestSize=%d\n",lestSize);*/
	}while(lestSize>0);
	
	free(data);
      }
      
      while(1){
	if(RecvOneLine_2(acc,&buf,0)<=0){
	  Syslog(LOG_ERR,"GetOneFile:RecvOneLine_2:error or closed:unlink\n");
	  unlink(fullpath);
	  free(fullpath);
	  close(fd);
	  UnlockFile(lf);
	  return(-1);
	}
	else if((ptr=strchr(buf,'#'))!=NULL){
	  break;
	}
	free(buf);
      }
      CutCrLf(ptr);
      Syslog(LOG_DEBUG,"GetOneFile:RecvOneLine_2:%s\n",ptr);
    }while(1);
    
    close(fd);
    UnlockFile(lf);
    
    ut.actime=timestamp;
    ut.modtime=timestamp;
    utime(fullpath,&ut);
    
    free(fullpath);
    
    Syslog(LOG_DEBUG,"GetOneFile:Success\n");
    
    return(1);
  }
}

int CleanFilename(char *name) {
  char	*tmp;
  int	len,i,c;
  
  /*Syslog(LOG_DEBUG,"CleanFilename:name=%s\n",name);*/
  
  len=strlen(name);
  tmp=(char *)calloc(len+1,sizeof(char));
  
  c=0;
  for(i=0;i<len;i++){
    if(i>0&&name[i]=='/'&&name[i-1]=='/'){
      continue;
    }
    tmp[c]=name[i];
    c++;
  }
  tmp[c]='\0';
  
  strcpy(name,tmp);
  free(tmp);
  
  len=strlen(name);
  if(name[len]=='/'){
    name[len]='\0';
  }
  
  return(0);
}

// added
void *ipc_listen(void *arg) {
  while(1) {
    //Syslog(LOG_DEBUG,"IPC_LISTEN\n");
    IPC_listenWait(10);
    //sleep(1);
  }
}

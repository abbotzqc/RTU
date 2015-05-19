#include<stdio.h>
#include<string.h>
#include<sys/ipc.h>
#include<sys/msg.h>
#include<sys/sem.h>
#include<sys/shm.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<sys/types.h>
#include<unistd.h>
#include<fcntl.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>
#include<signal.h>
#include<pthread.h>
#include<regex.h>
#include<dirent.h>
#include"sem.h"
#include"data_global.h"
#include<termios.h>
#include<netinet/tcp.h>

//#define N 64
#define N sizeof(shm_s)
#define M 512 
#define IP "192.168.1.10"
#define PORT 8899

#define LEN_WANH 44

int fd_parm;

/******************************************************************************
 *	time stamp 
 ******************************************************************************/
#define TIME 		do{\
						time_t t;\
						struct tm *tim;\
						time(&t);\
						tim=localtime(&t);\
						printf("%d.%02d.%02d-%02d:%02d:%02d : ",tim->tm_year+1900,tim->tm_mon+1,tim->tm_mday,tim->tm_hour,tim->tm_min,tim->tm_sec);\
					}while(0)

#define TIME2 		do{\
						time_t t;\
						struct tm *tim;\
						unsigned char String[256]={};\
						time(&t);\
						tim=localtime(&t);\
						sprintf(String,"%d.%02d.%02d-%02d:%02d:%02d : ",tim->tm_year+1900,tim->tm_mon+1,tim->tm_mday,tim->tm_hour,tim->tm_min,tim->tm_sec);\
						strncat(String,str,32);\
						perror(String);\
						fflush(stderr);\
					}while(0)


void self_printf(unsigned char *str)
{
	TIME;
	printf(str);
	fflush(stdout);
	return;
}

void self_perror(unsigned char *str)
{
	TIME2;
	return;	
}

/******************************************************************************
 *	global paragrams
 *******************************************************************************/

unsigned char wav_head[]={0x52,0x49,0x46,0x46,0x24,0x80,0x00,0x00,0x57,0x41,0x56,0x45,0x66,0x6d,0x74,0x20,
		  0x10,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x80,0xbb,0x00,0x00,0x80,0xbb,0x00,0x00,
          0x20,0x00,0x10,0x00,0x64,0x61,0x74,0x61,0x00,0x80,0x00,0x00,0x3e};


//int fd_errlog; 						    //error.log file
//int fd_data;								//data cache file
int sockfd;
//int errornum;				         		//record error number for the moment of errno set
//unsigned char afn_bak;         			//record last frame afn
/*unsigned char bufframe[N_READ]={};*/          //search frame buf zone
unsigned char *bufframe;
unsigned char bufr[48]={};				//receive buf zone
unsigned char buft[1048]={};				//send buf zone
unsigned char bufdata[680]={};			//send frame buf zone
//char wrongstr[256]={};						//print string writing to error.log
//int datazonelen;          				    //data zone length
shm_s  shmbuf;				   				//buf shm used;
//unsigned char buf32k[32][1025]={}; 
unsigned char buf32k[32][1048]={}; 

//unsigned char flag_msgrcv=0;              //if msgrcv dumped --0  else --1
unsigned char flag_reason;                  //wrong reason
unsigned char flag_passby;                  //PASSBY frame flag
unsigned char flag_loop=0;           		//32kb flag
unsigned char flag_led;            // 1-loop

pthread_t 	id_led,
			id_sdsize,
		//	id_wrong,
			id_vol,
			id_parm,
		    id_UPclient,
		    id_framere,
		    id_shm;

unsigned short rtuaddr=RTUADDR;             //record bak
char rtutype=RTUTYPE;                       //record bak
unsigned short rtuvolt;
unsigned short rtucurr;
unsigned char  seql=0xff;
//unsigned char afn;
unsigned char switch_timeint;               //record bak
int timeint[3];                             //record bak

unsigned short cap_sd=0xffff;               //sd card capacity  
unsigned short surcap_sd=0xffff;            //

unsigned char hostIP[16];
unsigned char hostPORT[6];

unsigned char overtime_server;
unsigned char overtime_client;
unsigned char time_repeat;                  //how many time server send testframe once 
unsigned int duration;

int size_buft;
/*******************************************************************************
 *	exchange function
 *******************************************************************************/
//unsigned char xtoa(unsigned char x)         //0x0a to a 
//{
//	unsigned char a;
//	if((x>=0x00)&&(x<=0x09))
//		a='0'+(x-0x00);
//	if((x>=0x0a)&&(x<=0x0f))
//		a='a'+(x-0x0a);
//	return a;
//}
//
//unsigned char atox(unsigned char a)            //a to 0x0a
//{
//	unsigned char x;
//	if(a>='0'&& a<='9')
//		x+=a-'0';
//	else if(a>='a'&& a<='f')
//		x+=a-'a'+0x0a;
//	else if(a>='A'&& a<='F')
//		x+=a-'A'+0x0a;
//	return x;
//}

/******************************************************************************
 * pthread for record printf message
 *******************************************************************************/
/*
void *pthread_wrong(void *argc)
{
	int fd;
	printf("*********************pthread_wrong in**********************\n");
	if((fd=open("../client_wrong.log",O_RDWR|O_CREAT,0666))<0)
	{
		printf("client_uphost pthread_wrong : open wrong.log faild\n");
		return -1;
	}
	pthread_mutex_lock(&mutex_errno);
	write(fd,wrongstr,4);
	bzero(wrongstr,sizeof(wrongstr));
	pthread_mutex_unlock(&mutex_errno);
	return 1;
}
*/

/******************************************************************************
 *	pthread for own volate and circuit
 *
 *	typedef struct vol
 {
 	unsigned char head;             //'s'
	unsigned char v;                //'v'
	short vol;                        
	unsigned char c;                //'c'
	short cur;
	unsigned char tail;             //'e' 
 } vol_s;

 ******************************************************************************/
void serial_init(int fd)
{
	struct termios options;//termios函数族提供一个常规的终端接口，用于控制非同步通信端口。

	tcgetattr(fd, &options);//取得当前串口的属性，并付给collect_fd这个设备
	options.c_cflag |= (CLOCAL | CREAD);//clocal表示忽略modem控制线，cread表示打开接收者
	options.c_cflag &= ~CSIZE;//清空原有字符长度（csize表示原有字符长度掩码）
//	options.c_cflag &= ~CRTSCTS;//启用RTS/CTS（硬件）流控制
	options.c_cflag |= CS8;//设置字符长度掩码
	options.c_cflag &= ~CSTOPB;//设置停止位为1个（cstopb表示设置两个停止位）
	options.c_iflag |= IGNPAR;//忽略帧错误和奇偶校验错

	options.c_oflag = 0;//设置输出模式标志
	options.c_lflag = 0;//设置本地模式标志

	cfsetispeed(&options, B115200);//设置输入波特率
	cfsetospeed(&options, B115200);//设置输出波特率
	
	tcsetattr(fd, TCSANOW, &options);//把上面设置号的属性赋值给collect_fd这个设备，tcsanow表示马上生效

	return ;
}


void *pthread_vol(void *arg)
{
#ifdef DEBUG
	printf("*********************pthread_vol     in***********************\n");
#endif
	int fd,fd_tty;
	off_t length=32*60*60*24*2;              //overwrite per day
	unsigned char datav[2];
	unsigned char datac[2];
	unsigned char check;
	unsigned char s[32];
	unsigned char str[32];
	int len;
	for(;;)
	{
		if((fd=open(PATH_VAL,O_RDWR|O_CREAT,0666))<0)
		{
			//sprintf(str,"open vol.log faild ");
			//TIME2;
			self_perror("open vol.log faild ");
			close(fd);
			sleep(1);
			continue;
		}
		if((fd_tty=open(PATH_SERIAL,O_RDWR,0666))<0)
		{
			self_perror("open com faild ");
			close(fd_tty);
			continue;
			sleep(1);
		}
		break;
	}
	self_printf("Thread volt start working\n");
	while(1)
	{
	//	close(fd);
	//	close(fd_tty);
		lseek(fd,0,SEEK_SET);
		lseek(fd_tty,0,SEEK_SET);
		if(!(fd>0))
		{
		if((fd=open(PATH_VAL,O_RDWR|O_CREAT,0666))<0)
		{
			//sprintf(str,"open vol.log faild ");
			//TIME2;
			self_perror("open vol.log faild ");
			close(fd);
			continue;
		}
		}
		while(1)
		{
			if(!(fd_tty>0))
			{
			if((fd_tty=open(PATH_SERIAL,O_RDWR,0666))<0)
			{
				self_perror("open com faild ");
				close(fd_tty);
				break;
				//continue;
			}
			}
			else
			{
				serial_init(fd_tty);
				bzero(datav,sizeof(datav));
				bzero(datac,sizeof(datac));
				read(fd_tty,&check,1);
				if(check=='s')
				{
					check=0;
					read(fd_tty,&check,1);
					if(check=='v')
					{
						check=0;
						if((len=read(fd_tty,datav,sizeof(datav)))!=sizeof(datav))
						{
							len--;
							for(len;len<sizeof(datav)+1;len++)
							{
								read(fd_tty,datav+len,1);
							}
						}

						read(fd_tty,&check,1);
						if(check=='c')
						{
							check=0;
							if((len=read(fd_tty,datac,sizeof(datac)))!=sizeof(datac))
							{
								len--;
								for(len;len<sizeof(datac);len++)
								{
									read(fd_tty,datac+len,1);
								}
							}
							memcpy(&rtucurr,datac,sizeof(datac));
							memcpy(&rtuvolt,datav,sizeof(datav));
							sprintf(s,"rtuvolt=%06d   rtucurr=%06d",rtuvolt,rtucurr);                     //len:31
							s[31]='\n';
//#ifdef DEBUG
//							printf("%s",s);
//							printf("v %d c %d\n",rtuvolt,rtucurr);
//							printf("%02x %02x\n",datav[0],datac[1]);
//#endif
							write(fd,str,sizeof(str));
							lseek(fd,0,SEEK_SET);
							pthread_mutex_lock(&mutex_shmbuf);
							memcpy(&shmbuf.volt,datav,2);
							memcpy(&shmbuf.curr,datac,2);
							pthread_mutex_unlock(&mutex_shmbuf);
							pthread_cond_signal(&cond_shm);
#ifdef DEBUG
							printf("UPclient pthread_vol cond_shm\n");
#endif

						}
					}
				}
			}
		//	close(fd_tty);
		}
	//	close(fd);
	}
	return NULL;
}

/*******************************************************************************
 *	pthread for read/write global paragram
 *******************************************************************************/
void *pthread_parm(void *arg)
{
#ifdef DEBUG
	printf("**********************pthread_parm   in*********************\n");
#endif
	pthread_detach(pthread_self());
	unsigned char p;
	if(arg!=NULL)
	{
		p=*(unsigned char*)arg;
		free(arg);
		arg=NULL;
	}
	else
		p=READ_PARM;
	int cnt=0;
	unsigned char buf[256];
	unsigned char bak[8]={};
	//unsigned char str[32]={};
	
	self_printf("Thread parm start working\n");

//	close(fd_parm);
	if(!(fd_parm>0))
	{
	if((fd_parm=open(PATH_PARM,O_RDWR|O_CREAT,0x666))<0)
	{
		self_perror("open parm.log faild ");
		close(fd_parm);
		return;
	}
	}
	cnt=read(fd_parm,buf,sizeof(buf));
	if(p==READ_PARM)                  //1
	{
		memcpy(bak,buf+8,2);
		rtuaddr=strtol(bak,NULL,10);
		bzero(bak,sizeof(bak));

		memcpy(bak,buf+19,2);
		rtutype=strtol(bak,NULL,16);
		bzero(bak,sizeof(bak));

		memcpy(bak,buf+27,2);
		seql=strtol(bak,NULL,16);
		bzero(bak,sizeof(bak));

		memcpy(bak,buf+45,2);
		switch_timeint=strtol(bak,NULL,16);
		bzero(bak,sizeof(bak));

		memcpy(bak,buf+59,2);
		timeint[0]=strtol(bak,NULL,10);
		bzero(bak,sizeof(bak));

		memcpy(bak,buf+73,2);
		timeint[1]=strtol(bak,NULL,10);
		bzero(bak,sizeof(bak));

		memcpy(bak,buf+87,2);
		timeint[2]=strtol(bak,NULL,10);
		bzero(bak,sizeof(bak));

//		memcpy(bak,buf+97,2);
//		n_node=strtol(bak,NULL,10);
//		bzero(bak,sizeof(bak));
//
//		memcpy(bak,buf+106,2);
//		n_sen=strtol(bak,NULL,10);
//		bzero(bak,sizeof(bak));
//
//		memcpy(bak,buf+118,2);
//		duration=strtol(bak,NULL,10);
//		bzero(bak,sizeof(bak));
//
		memcpy(hostIP,buf+170,15);
		memcpy(hostPORT,buf+195,5);

		memcpy(bak,buf+217,2);
		overtime_server=strtol(bak,NULL,16);
		bzero(bak,sizeof(bak));
		memcpy(bak,buf+236,2);
		overtime_client=strtol(bak,NULL,16);
		bzero(bak,sizeof(bak));
		memcpy(bak,buf+251,2);
		time_repeat=strtol(bak,NULL,16);
		bzero(bak,sizeof(bak));


#ifdef DEBUG
		printf("rtuaddr	 rtutype  seql  switch_timeint  timeint[0]  timeint[1]  timeint[2]\n");
		printf("%x        %x        %x     %x               %d           %d           %d\n",rtuaddr,rtutype,seql,switch_timeint,timeint[0],timeint[1],timeint[2]);
		printf("overtime_client %d\n",overtime_client);
#endif
	}
	else
	{
		switch(p)
		{
		case REC_RTUADDR:
			sprintf(bak,"%02d",rtuaddr);
			lseek(fd_parm,8,SEEK_SET);
		//	write(fd_parm,bak,strlen(bak));
			break;
		case REC_RTUTYPE:
			sprintf(bak,"%02x",rtutype);
			lseek(fd_parm,19,SEEK_SET);
		//	write(fd_parm,bak,strlen(bak));
			break;
		case REC_SEQL:
			sprintf(bak,"%02x",seql);
			lseek(fd_parm,27,SEEK_SET);
			break;
		case REC_TIMEINT:
			sprintf(bak,"%02x",switch_timeint);
			lseek(fd_parm,45,SEEK_SET);
			write(fd_parm,bak,strlen(bak));
			bzero(bak,sizeof(bak));

			sprintf(bak,"%02d",timeint[0]);
			lseek(fd_parm,59,SEEK_SET);
			write(fd_parm,bak,strlen(bak));
			bzero(bak,sizeof(bak));
		
			sprintf(bak,"%02d",timeint[1]);
			lseek(fd_parm,73,SEEK_SET);
			write(fd_parm,bak,strlen(bak));
			bzero(bak,sizeof(bak));

			sprintf(bak,"%02d",timeint[2]);
			lseek(fd_parm,87,SEEK_SET);
			system("pkill -SIGINT DWserver");
			break;
		case REC_DUR:
			sprintf(bak,"%02d",duration);
			lseek(fd_parm,118,SEEK_SET);
			system("pkill -SIGINT DWserver");
			break;
		case REC_OVERTS:
			sprintf(bak,"%02x",overtime_server);
			lseek(fd_parm,217,SEEK_SET);
			system("pkill -SIGINT DWserver");
			break;
		case REC_OVERTC:
			sprintf(bak,"%02x",overtime_client);
			lseek(fd_parm,236,SEEK_SET);
			break;
		case REC_REP:
			sprintf(bak,"%02x",time_repeat);
			lseek(fd_parm,251,SEEK_SET);
			system("pkill -SIGINT DWserver");
			break;
		}
			write(fd_parm,bak,strlen(bak));
			bzero(bak,sizeof(bak));
	}
//	close(fd_parm);
	lseek(fd_parm,0,SEEK_SET);
	return;
}


/*******************************************************************************
 *	signal handler used for pthread_framere
 *******************************************************************************/
/*
void handler_framere(int signo)
{
#ifdef DEBUG
	printf("alarm\n");
#endif
	return NULL;
}
*/

/******************************************************************************
 *	pthread: read data from msgqueue to construct frame sending to up server
 ******************************************************************************/
void *pthread_framere(void *arg)
{
#ifdef DEBUG
	printf("*********************pthread_framere in********************\n");
#endif
	key_t key;
//	msg_s msgbuf;
//	int flag_reason;     					 //reason of frame fault
/*	int	flag_msg;         					 //stat of msgrev return  */

/*	char flag_dataat;*/
	frameloc_s frameloc;
	frameinfo_s frameinfo;
//	char *pattern;
	unsigned char buf[N_BUF];//frame[N_FRAME];
	unsigned char str[64];

	int repeat_pass,i;     

	unsigned char path[64];

	int fd;

	int flag_case=0; 


	self_printf("Thread framere start working\n");

	while((key = ftok (PATH_MSG, 'g')) < 0)
	{
		self_perror("create msgqueue key faild ");
		sleep(2);
	}
	while((msgid = msgget (key, IPC_CREAT | IPC_EXCL | 0666)) < 0)
	{
		if(errno == EEXIST)
		{
			msgid = msgget(key, 0666);
			break;
		}
		else
		{
			self_perror("get message id faild");
		}
	}

//	if((fd_data=open("../data.log",O_RDWR|O_CREAT,0666))<0)
//	{
//		printf("client_uphost pthread_framere:open data.log faild\n");
//		exit(-1);
//	}
	
	pthread_mutex_lock(&mutex_bufr);
//	while(1)
	for(;;)
	{

		pthread_cond_wait(&cond_framere,&mutex_bufr);

//		if(sizeof(buf)==N_BUF)
//			strcpy(buf,buf+256);
/*		int i;*/
		bzero(bufdata,sizeof(bufdata));
		bzero(buf,sizeof(buf));
		for(i=0;i<sizeof(bufr);i++)                     //hex to char
		{
			sprintf(buf+(i<<1),"%02x",bufr[i]);
			//buf[i*2]=xtoa((bufr[i]>>4));
			//buf[i*2+1]=xtoa(bufr[i]&0x0f);
		}
	//	strcat(buf,bufr);                  
		pthread_mutex_unlock(&mutex_bufr);
#ifdef DEBUG
		printf("frame receive: %s\n",buf);
#endif
//		self_printf("socket read ");
//		printf("%s\n",buf);
//		fflush(stdout);

	/*	char pattern=PATTERN;
		if(search(pattern,buf,&frameloc,bufr)==0)*/
		if(search(PATTERN,buf,&frameloc,bufr)==0)
		{
			if(flag_passby==1)
			{
				msgtoser_s msgbuf;
				msgtocli_s msgbuff;
				msgbuf.msgtype=MSG_TOSER;
				while(msgrcv(msgid,&msgbuf,sizeof(msgbuf)-sizeof(long),MSG_TOSER,IPC_NOWAIT)>0);
				pthread_mutex_lock(&mutex_bufr);
				memcpy(msgbuf.addr,bufr+1,4);
				msgbuf.seq=bufr[7];
				msgbuf.afn=bufr[14];
				memcpy(msgbuf.buf,bufr,sizeof(msgbuf.buf));
				pthread_mutex_unlock(&mutex_bufr);
				while(msgrcv(msgid,&msgbuff,sizeof(msgbuff)-sizeof(long),MSG_TOCLIENT,IPC_NOWAIT)>0);
				if(msgsnd(msgid,&msgbuf,sizeof(msgbuf)-sizeof(long),0)<0)
				{

					self_perror("send message faild ");

					flag_reason=ERR_EXEC;
					funafnpass(bufdata,flag_reason,msgbuf);

					pthread_mutex_lock(&mutex_buft);
					memcpy(buft,bufdata,sizeof(bufdata));
					pthread_mutex_unlock(&mutex_buft);

					flag_reason=0;
				}
				else
				{
					bzero(str,sizeof(str));
					self_perror("send message to server");
					/*
					sprintf(str,"sended to server : %s",msgbuf.buf);
					strcat(str,"\n");
					self_printf(str);
					*/

				//  flag_msgrcv=0;
				//	struct sigaction alm;
				//	alm.sa_handler=handler_framere;
				//	alm.sa_flags=(SA_INTERRUPT|SA_RESETHAND);
				//	sigaction(SIGALRM,&alm,NULL);
					//signal(SIGALRM,handler_framere);
					int sleeptime,flag_msgrcv,wh=1;
					if((msgbuf.afn!=0xf0)&&(msgbuf.afn!=0xf1))
					{
						(overtime_client==0)?(sleeptime=1500000):(sleeptime=overtime_client*1500000);
						switch(msgbuf.afn)
							{
							case 0x01:
								size_buft=40;
								break;
							case 0x04:
								size_buft=19;
								break;
							case 0xf2:
								size_buft=19;
								break;
							}
						repeat_pass=1;
					}
					else
					{
						//sleep(4);
					//	alarm(4);
						(overtime_client==0)?(sleeptime=6000000):(sleeptime=overtime_client*6000000);
						if(msgbuf.afn==0xf0)
						{
							repeat_pass=32;
							flag_loop=32;
							flag_reason=0xff;
						}
						else
						{
							repeat_pass=1;
						}
						size_buft=sizeof(buft);
					}
				
					bzero(buf32k,sizeof(buf32k));
					sleep(2);
					while(repeat_pass--)
					{
						while((flag_msgrcv=msgrcv(msgid,&msgbuff,sizeof(msgbuff)-sizeof(long),MSG_TOCLIENT,IPC_NOWAIT))<0)
						{
							if((sleeptime--)==0)
							{
								//goto out;
#ifdef DEBUG
								printf("flag_msgrcv %d\n",flag_msgrcv);
#endif
								break;
							}
						}
#ifdef DEBUG
						printf("691\n");
#endif
						if(!(flag_msgrcv<0))
						{
							//flag_msgrcv=1;                                //msgrcv receive over	
							pthread_mutex_lock(&mutex_buft);
#ifdef DEBUG
							printf("698\n");
#endif
							if(msgbuf.afn==0xf0||msgbuf.afn==0xf1)
							{
								memcpy(buft,msgbuff.buf,sizeof(msgbuff.buf));
							}	
							else
							/*	memcpy(buft,msgbuff.buf,64);*/
								memcpy(buft,msgbuff.buf,46);
							if(msgbuff.err_flag==1)
							{
							if((wh=1)&&(msgbuff.buf[15]==0xe3||msgbuff.buf[15]==0xe5||msgbuff.buf[15]==ERR_NODEBUSY||msgbuff.buf[15]==ERR_PASS))
							{
								flag_loop=0;
								flag_reason=0;
								wh=0;
								while(msgrcv(msgid,&msgbuff,sizeof(msgbuff)-sizeof(long),MSG_TOCLIENT,IPC_NOWAIT)>0);
								size_buft=19;
								pthread_mutex_unlock(&mutex_buft);
								goto out;
							}
							}
						//	pthread_mutex_unlock(&mutex_buft);
						}
						else
						/* if(flag_msgrcv<0)   */
						{
							//flag_msgrcv=1;                                //msgrcv receive over	
							flag_reason=ERR_OVERTIMEC;
							//if(flag_reason==ERR_OVERTIME)
							//funafnpass(bufdata,flag_reason,msgbuf);
							pthread_mutex_lock(&mutex_buft);
							funafnpass(buft,flag_reason,&msgbuf);
						//	pthread_mutex_unlock(&mutex_buft);
						}

						memcpy(buf32k[repeat_pass],buft,sizeof(buft));
						pthread_mutex_unlock(&mutex_buft);
					}
				}

				while(msgrcv(msgid,&msgbuff,sizeof(msgbuff)-sizeof(long),MSG_TOCLIENT,IPC_NOWAIT)>0);
			}
			else
			{
#ifdef DEBUG
				printf("frame search : no data\n");
#endif
			}
			//	strcpy(buf,buf+256);
			//return 0;
out:    	pthread_cond_signal(&cond_write);
			flag_passby=0;

#ifdef DEBUG
			printf("search : cond_write\n");
#endif
			continue;
		}
		//strncpy(bufframe,frameloc.phead,frameloc.len);
		//strcpy(buf,frameloc.phead+frameloc.len);
		if(analysis(bufframe,&frameinfo)<=0) 
		{
#ifdef DEBUG
			printf("framere analysis : analysis wrong\n");
#endif
			pthread_cond_signal(&cond_write);
			continue;
		}
	
//		int fd;                             //read data file
//		afn05_s afn05[128];					
//		regex_t reg;
//		char errbuf[1024];
//		int err;
//		int cflag=REG_EXTENDED;
//		int nmatch=1;
//		unsigned char bufsource[48];
//		regmatch_t pmatch[nmatch];
/*		bzero(bufdata,sizeof(bufdata));*/
		flag_case=0;
		switch(frameinfo.afn)
		{
		case 0x01:	
			{
#ifdef DEBUG
				printf("client_uphost pthread_framere : case 0x01 in\n");
#endif
				/*	msgnode_s msgnodebuf;*/
					pthread_cond_signal(&cond_sdsize);	
				/*	afn01t_s afn01t;
					afn01t.rtutype=rtutype;
					afn01t.rtuvolt=rtuvolt;
					afn01t.rtucurr=rtucurr;
					afn01t.rtuversion=itoc_bcd(RTUVERSION_M);
					afn01t.rtuversion<<=8;
					afn01t.rtuversion+=itoc_bcd(RTUVERSION_S);*/
				/*	if(frameinfo.addr==rtuaddr)
					{
						funafn01t(bufdata)//,&afn01t);
					}
					else */
					if(frameinfo.addr==BROADCASTADDR||frameinfo.addr==rtuaddr)
					{
						//datazonelen=-1;
						pthread_cond_signal(&cond_shm);             //wakeup shm pthread
#ifdef DEBUG
						printf("client_uphost pthread_framere : case 0x01 send cond_shm\n");
#endif
						afnbroadt_s afnbroadt;
						//	afnbroadt.afn01t=afn01t;
						//read from msgqueue 
						//store in linklist or big arrey?  use arrey easyly to search 
				/*		int i;*/
				/*		unsigned char path[64];

						int fd;*/
						int j;
						unsigned char buflog2[32]={};
						unsigned char buflog[64]={};
						for(i=1;i<5;i++)
						{
							bzero(path,sizeof(path));
							strcpy(path,PATH_LINK);
							sprintf(path+strlen(PATH_LINK),"node%d/testframe.log",i);
							//sprintf(path,"../Data/linkdata/node%d/testframe.log",i);
							if((fd=open(path,O_RDWR,0666))<0)
							{
								afnbroadt.nodemsg[i-1].year_rec=0;
							}
							else
							{
								bzero(buflog,sizeof(buflog));
								bzero(buflog2,sizeof(buflog2));
								if(lseek(fd,-57,SEEK_END)<0)
								{
									self_printf("case 0x01 : lseek faild\n");
									afnbroadt.nodemsg[i-1].year_rec=0;
									close(fd);
									continue;
								}
								if(read(fd,buflog,57)<0)
								{
									self_printf("case 0x01 : read testframe faild\n");
								}
								//printf("buflog:%s\n",buflog);
							
								for(j=0;j<strlen(buflog);j++)
								{
									//	buflog2[j>>1]=strtol(buflog+j,NULL,16);
									//	//putchar(buf[i]);
									if(buflog[j]>='0'&&buflog[j]<='9')
										buflog2[j/2]+=buflog[j]-'0';
									else if(buflog[j]>='a'&&buflog[j]<='f')
										buflog2[j/2]+=buflog[j]-'a'+0x0a;
									else if(buflog[j]>='A'&&buflog[j]<='F')
										buflog2[j/2]+=buflog[j]-'A'+0x0a;
									if(j%2==0)
										buflog2[j/2]<<=4;
								}
								memcpy(&afnbroadt.nodemsg[i-1],buflog2,strlen(buflog)/2);
							}
							close(fd);
						}
						funafnbroadcastt(bufdata,&afnbroadt);
						size_buft=151;
	//					printf("client_uphost pthread_framere : case 0x01 out     %d\n",counter++);
					}
					break;
			}
		case 0x02:
			{
#ifdef DEBUG
				printf("client_uphost pthread_framere : case 0x02 in\n");
#endif
				afn02r_s afn02r;
				flag_reason=funafn02r(&afn02r,frameinfo.pdata);
				rtuaddr=afn02r.addrall;
#ifdef DEBUG
				printf("case 0x02 : rtuaddr=%d\n",rtuaddr);
#endif
				unsigned char *p=malloc(sizeof(unsigned char));
				*p=REC_RTUADDR;
				pthread_create(&id_parm,0,&pthread_parm,p);
				funafn02t(bufdata,flag_reason);
				size_buft=19;
				break;
			}
		case 0x04: 
			{
				unsigned char string[64];
#ifdef DEBUG
				printf("client_uphost pthread_framere : case 0x04 in\n");
#endif
				afn04r_s afn04r;
				flag_reason=funafn04r(&afn04r,frameinfo.pdata);
				//printf("case 0x04 : year %d\n",afn04r.year);
				sprintf(string,"date -s %d.%d.%d-%02d:%02d:%02d\n",afn04r.year,afn04r.month,afn04r.day,afn04r.hour,afn04r.min,afn04r.sec);
				//printf("client_uphost pthread_framere : case 0x04 : %s",str);
				if(system(string)<0)
				{
					self_perror("set date faild ");
					flag_reason=FAILD;
				}
#ifdef DEBUG
				perror("system");
#endif
				funafn04t(bufdata,flag_reason);
				size_buft=19;
				break;
			}
		case 0x05:  
			{
#ifdef DEBUG
				printf("client_uphost pthread_framere : case 0x05 in\n");
#endif
			//	int fd;           //read data file
				afn05_s afn05[128]={};					
				regex_t reg;
				char errbuf[1024];
				int err;
				int cflag=(REG_EXTENDED|REG_ICASE);
				int nmatch=1;
				regmatch_t pmatch[nmatch];
				afn05r_s afn05r;
				unsigned char bufsource[37];
				afn05t_s afn05t;
			/*	unsigned char path[64];*/
				unsigned char string[84];//[64];
				funafn05r(&afn05r,frameinfo.pdata);
				//flag_dataat=MSG;
				//
				bzero(path,sizeof(path));
				strcpy(path,PATH_MEDIA);
				sprintf(path+strlen(PATH_MEDIA),"%d-%02d-%02d/index.log",afn05r.year,afn05r.month,afn05r.day);
				//sprintf(path,"..media/mmcblkp0p1/Data/%d-%2d-%2d/index.log",afn05r.year,afn05r.month,afn05r.day);
		
				bzero(string,sizeof(string));
				if(afn05r.nodeno==0)
				{
					if(afn05r.sensorno==0)
						sprintf(string,"/timeint_%d/node[1-4]/sensor[1-6]/[0-2][0-9]:[0-6][0-9]:[0-6][0-9].wav",afn05r.hour);
					else
						sprintf(string,"/timeint_%d/node[1-4]/sensor%d/[0-2][0-9]:[0-6][0-9]:[0-6][0-9].wav",afn05r.hour,afn05r.sensorno);
				}
				else
				{
					if(afn05r.sensorno==0)
						sprintf(string,"/timeint_%d/node%d/sensor[1-6]/[0-2][0-9]:[0-6][0-9]:[0-6][0-9].wav",afn05r.hour,afn05r.nodeno);
					else
						sprintf(string,"/timeint_%d/node%d/sensor%d/[0-2][0-9]:[0-6][0-9]:[0-6][0-9].wav",afn05r.hour,afn05r.nodeno,afn05r.sensorno);
				}
				//printf("xxxxxxxxxxxxxxx%s\n",string);
				afn05t.recordsum=0;

				bzero(&afn05t,sizeof(afn05t));

				if((fd=open(path,O_RDWR|O_CREAT,0666))<0)
				{
					self_perror("open index.log faild ");
#ifdef DEBUG
					printf("case 0x05 : open index file faild\n");
					perror("error ");
#endif
					afn05t.year=afn05r.year;
					afn05t.month=afn05r.month;
					afn05t.day=afn05r.day;
					afn05t.pafn05=afn05;
					funafn05t(bufdata,&afn05t);
				//	close(fd);
					break;
					//goto next;
				}
				while(read(fd,bufsource,sizeof(bufsource))>0)
				{
#ifdef DEBUG
					printf("xxxxxxxxxxxxxxxxbufsource%s\n",bufsource);
					printf("xxxxxxxxxxxxxxxxstring%s\n",string);
#endif
					if(regcomp(&reg,string,cflag)<0)
					{
						regerror(err,&reg,errbuf,sizeof(errbuf));
						self_perror("case 0x05 regcomp faild ");
#ifdef DEBUG
						printf("case 0x05 regcomp : err : %s\n",errbuf);
						perror("regcomp");
#endif
					}
					err=regexec(&reg,bufsource,nmatch,pmatch,0);
					if(err==REG_NOMATCH)
					{
#ifdef DEBUG
						printf("case 0x05 regexec : no match\n");
#endif
						lseek(fd,1,SEEK_CUR);             //1
						regfree(&reg);
						continue;
					}
					else if(err)
					{
						regerror(err,&reg,errbuf,sizeof(errbuf));
#ifdef DEBUG
						printf("case 0x05 regerror : err : %s\n",errbuf);
#endif
						lseek(fd,1,SEEK_CUR);             //1
						regfree(&reg);
						continue;	
					}
					afn05[afn05t.recordsum].nodeno=*(bufsource+pmatch[0].rm_so+15)-'0';
					afn05[afn05t.recordsum].sensorno=*(bufsource+pmatch[0].rm_so+23)-'0';
					afn05[afn05t.recordsum].hour=(*(bufsource+pmatch[0].rm_so+25)-'0')<<4;
					afn05[afn05t.recordsum].hour+=*(bufsource+pmatch[0].rm_so+26)-'0';
					afn05[afn05t.recordsum].min=(*(bufsource+pmatch[0].rm_so+28)-'0')<<4;
					afn05[afn05t.recordsum].min+=*(bufsource+pmatch[0].rm_so+29)-'0';
					afn05[afn05t.recordsum].sec=(*(bufsource+pmatch[0].rm_so+31)-'0')<<4;
					afn05[afn05t.recordsum].sec+=*(bufsource+pmatch[0].rm_so+32)-'0';
					afn05t.recordsum++;
					bzero(bufsource,sizeof(bufsource));
					lseek(fd,1,SEEK_CUR);             
				}
				regfree(&reg);
				afn05t.year=afn05r.year;
				afn05t.month=afn05r.month;
				afn05t.day=afn05r.day;
				//		afn05t.hour=afn05r.hour;
				afn05t.pafn05=afn05;
				funafn05t(bufdata,&afn05t);
				size_buft=sizeof(bufdata);
				close(fd);
				//	pthread_cond_signal(&cond_readlog);
				break;
			}
		case 0x06:
			{
#ifdef DEBUG
				printf("client_uphost pthread_framere : case 0x06 in\n");
#endif
			/*	int fd;           //read data file
				unsigned char path[64]; */
				regex_t reg;
				char errbuf[1024];
				int err;
				int cflag=REG_EXTENDED;
				int nmatch=1;
				unsigned char bufsource[48];
				regmatch_t pmatch[nmatch];

				afn06r_s afn06r;
				afn06t_s afn06t;
				funafn06r(&afn06r,frameinfo.pdata);
				bzero(path,sizeof(path));
				strcpy(path,PATH_MEDIA);
				sprintf(path+strlen(PATH_MEDIA),"%d-%02d-%02d/node%d/sensor%d/%02d.%02d.%02d.wav",afn06r.year,afn06r.month,afn06r.day,afn06r.nodeno,afn06r.sensorno,afn06r.hour,afn06r.min,afn06r.sec);
				//sprintf(path,"../media/mmcblkp0p1/Data/%d-%2d-%2d/node%d/sensor%d/%02d.%02d.%02d.wav",afn06r.year,afn06r.month,afn06r.day,afn06r.nodeno,afn06r.sensorno,afn06r.hour,afn06r.min,afn06r.sec);
				if((fd=open(path,O_RDWR,0666))<0)
				{
					flag_reason=1;               //NONE
					flag_loop=1;
#ifdef DEBUG
					printf("NONE direct file\n");
#endif
				}
				else
				{
					unsigned char a='0';
					//unsigned char b[2];
					read(fd,&a,1);
					if(a=='N')
					{
						read(fd,buf32k[0],2);
						flag_reason=(unsigned char)strtol(buf32k[0],NULL,16);
						flag_loop=1;
					}
					else
					{
						lseek(fd,LEN_WANH,SEEK_SET);
						for(i=0;i<32;i++)
						{
						//	lseek(fd,1,SEEK_CUR);
							flag_case=read(fd,buf32k[i],1024);
							//		if(!(flag_case>0));
							//		{
							//			printf("flag_case %d   case 0x06 [%d]:read 32kb faild\n",flag_case,i);
							//			printf("%s\n",buf32k[i]);
							//			perror("error");
							//		}
						}
						flag_loop=32;
					}
				}
				//	sleep(1);
				//	printf("0x06 mutex_buft own		%d\n",counter++);
				pthread_mutex_lock(&mutex_buft);
				//	printf("case 0x06 mutex_buft	%d\n",counter++);
				sprintf(buft,"success",10);
				pthread_mutex_unlock(&mutex_buft);
				//	printf("cond_write   	 %d\n",counter++);
				flag_case=1;         //jump out big while
				pthread_mutex_lock(&mutex_bufr);
				pthread_cond_signal(&cond_write);
				close(fd);
#ifdef DEBUG
				printf("client_uphost pthread_framere : case 0x06 out\n");
#endif
				break;
			}
/*		case 0xf1: 
			{
#ifdef DEBUG
				printf("client_uphost pthread_framere : case 0xf1 in\n");
#endif
				afnf1r_s afnf1r;
				afnf1t_s afnf1t;
				funafnf1r(&afnf1r,frameinfo.pdata);
				afnf1t.frameno=afnf1r.frameno;
				memcpy(afnf1t.data,buf32k[afnf1r.frameno],1024);
				funafnf1t(bufdata,&afnf1t);
				break;
			}
*/
		case 0xf3:	
			{
#ifdef DEBUG
				printf("client_uphost pthread_framere : case 0xf3 in\n");
#endif
				flag_reason=funafnf3r(frameinfo.pdata);
				unsigned char *p=malloc(sizeof(unsigned char));
				*p=REC_TIMEINT;
				pthread_create(&id_parm,0,&pthread_parm,p);
				funafnf3t(bufdata,flag_reason);
				size_buft=19;
				break;
			}
		}
			
		if(flag_case==1)
		{
			continue;
		}
#ifdef DEBUG
		printf("client_uphost pthread_framere : switch out\n");
#endif
		usleep(100);
		pthread_mutex_lock(&mutex_buft);
#ifdef DEBUG
		printf("client_uphost pthread_framere : own mutex_buft\n");
#endif
		memcpy(buft,bufdata,sizeof(bufdata));
		//	int k;
		//	printf("XXXXXXbufdata:\n");
		//		for(k=0;k<N_READ;k++)
		//		{
		//			printf("%x",bufdata[k]);
		//		printf("client_uphost pthread_framere:bufdata[%d] %x",k,bufdata[k]);
		//		printf("client_uphost pthread_framere:buft[%d] %x\n",k,buft[k]);
		//		}
		//		printf("\n");
		pthread_mutex_unlock(&mutex_buft);
		
		pthread_mutex_lock(&mutex_bufr);

		pthread_cond_signal(&cond_write);

	//	unsigned char *p=malloc(sizeof(unsigned char));
	//	*p=REC_SEQL;
	//	pthread_create(&id_parm,0,&pthread_parm,p);
	
	}
	return NULL;
}

/*******************************************************************************
 *	pthread of writing shm to send frame to server_down
 *******************************************************************************/
void *pthread_shm(void *arg)
{
	shm_s *pshmbuf;
	key_t key_info;
	unsigned char str[32];
#ifdef DEBUG
	printf("*********************pthread_shm     in************************\n");
#endif
	self_printf("Thread shm start working\n");

	while ((key_info = ftok (PATH_SHM, 'i')) < 0)
	{
		self_perror("own shm key faild ");
		//exit (-1);
	}
#ifdef DEBUG
	printf ("client_uphost pthread_shm : key = %d\n", key_info);
#endif

//	if((semid = semget (key_info, 1, IPC_CREAT | IPC_EXCL |0666)) < 0)
//	{
//		if (errno == EEXIST)
//		{
//			semid = semget (key_info, 1, 0666);
//		}
//		else
//		{
//			self_perror("get sem id faild ");
//			//perror ("client_uphost pthread_shm : semget\n");
//			//exit (-1);
//		}
//	}
//	else
//	{
//		init_sem (semid, 0, 2);
//	}
#ifdef DEBUG
	printf("client_uphost pthread_shm : shm bellow\n");
#endif
	if ((shmid = shmget (key_info, N, IPC_CREAT | IPC_EXCL | 0666)) < 0)
	{
		if (errno == EEXIST)
		{
			shmid = shmget (key_info, N, 0666);
			pshmbuf = (shm_s*)shmat (shmid, NULL, 0);
			bzero (pshmbuf, sizeof(shm_s));
		}
		else
		{
			self_perror("get shm id faild ");
			//perror ("client_uphost pthread_shm : shmget\n");
			//exit (-1);
		}
	}
	else
	{
		if ((pshmbuf = (shm_s*)shmat(shmid, NULL, 0)) == (void *)-1)
		{
			self_perror("map shm faild ");
			//perror ("client_uphost pthread_shm : shmat\n");
			//exit (-1);
		}
	}


	pthread_mutex_lock(&mutex_shmbuf);
	while(1)
	{
#ifdef DEBUG
		printf("client_uphost pthread_shm : wait cond_shm\n");
#endif
		shmbuf.switch_timeint=switch_timeint;
		int n;
		for(n=0;n<3;n++)
			shmbuf.timeint[n]=timeint[n];

		//sem_p(semid,0);              //locate here ok?
	//	pthread_cond_wait(&cond_shm,&mutex_shmbuf);
#ifdef DEBUG
		printf("client_uphost pthread_shm : wait cond_shm awake\n");
#endif
		
		pshmbuf->volt=shmbuf.volt;
		pshmbuf->curr=shmbuf.curr;
		pshmbuf->switch_timeint=shmbuf.switch_timeint;
		memcpy(pshmbuf->timeint,shmbuf.timeint,sizeof(timeint));
		pshmbuf->overtime_server=overtime_server;
		pshmbuf->time_repeat=time_repeat;

		system("pkill -SIGINT DWserver");
	//	memcpy(pshmbuf,&shmbuf,sizeof(shm_s));
	//	if(datazonelen==-1)
	//	{
	//		pshmbuf->flag=LATESTTEST;
	//		strncpy(pshmbuf->buftdown,bufframe,18);                  //use bufframe ok?
	//	}
	//	else
	//	{
	//		pshmbuf->flag=PASSBY;
	//		strncpy(pshmbuf->buftdown,bufframe+15,datazonelen);
	//	}
	//
		pthread_mutex_unlock(&mutex_shmbuf);
		//sem_v(semid,0);
		pthread_mutex_lock(&mutex_shmbuf);
		pthread_cond_wait(&cond_shm,&mutex_shmbuf);
	}
	return;
}

/******************************************************************************
 *	pthread for led ON/OFF
 *	function: when client_uphost begin deal frame led loop bright
 ******************************************************************************/
void handler_led(int signo)
{
	signed char speed;
	int booling=4;
	
	pthread_mutex_lock(&mutex_led);
	speed=flag_led;
	pthread_mutex_unlock(&mutex_led);

	while(booling--)
	{
		if(speed==-1)
		{
			led_ctrl("D6",OFF);
			break;
		}
		else
		{
			usleep(500000/(speed+1));
			led_ctrl("D6",OFF);
			usleep(500000/(speed+1));
			led_ctrl("D6",ON);
		}
	}
	return;
}

void *pthread_led(void *arg)
{
	signal(SIGUSR2,handler_led);
	led_ctrl("D6",ON);
	while(1)
	{
		usleep(1000000);
		pthread_testcancel();
		led_ctrl("D6",OFF);
		pthread_testcancel();
		usleep(1000000);
		led_ctrl("D6",ON);
	}
	led_ctrl("D6",OFF);
	return NULL;
}

//void *pthread_led(void *arg)
//{
//	led_ctrl("D6",ON);
//	while(1)
//	{
//		pthread_mutex_lock(&mutex_led);
//		pthread_cond_wait(&cond_led,&mutex_led);
//		while(flag_led)
//		{
//			pthread_mutex_unlock(&mutex_led);
//			usleep(50000);
//			led_ctrl("D6",OFF);
//			usleep(50000);
//			led_ctrl("D6",ON);
//		}
//	}
//	return NULL;
//}

/******************************************************************************
 * pthread for to earn the data of sdcard size
 ******************************************************************************/
void *pthread_sdsize(void *arg)
{
	char errbuf[1024];
	int err,nmatch=6;
	regex_t reg;
	int cflags=(REG_EXTENDED|REG_ICASE);
	regmatch_t pmatch[nmatch];
	int fd;
	unsigned char buf[128];
	unsigned char *pattern="( ([1-9]{1}[0-9]{1,9}) {1,}([1-9]{1}[0-9]{1,9}) {1,}([1-9]{1}[0-9]{1,9}))";
	//unsigned char str[32];
#ifdef DEBUG
	printf("*********************pthread_sdsize  in*********************\n");
#endif

	DIR *dir;
	self_printf("Thread sdsize start working\n");

	for(;;)
	{
		dir=opendir(PATH_MEDIA);
		if(dir==NULL&&errno==ENOENT)
		{
			free(dir);
			dir=NULL;
			sleep(1);
			continue;
		}
		if((fd=open(PATH_SD,O_RDWR|O_CREAT,0666))<0)
		{
			self_perror("open sdsize.log faild ");
			pthread_mutex_unlock(&mutex_sdsize);
			close(fd);
			free(dir);
			dir=NULL;
			sleep(1);
			continue;
		}
		break;
	}

	while(1)
	{
		pthread_mutex_lock(&mutex_sdsize);
		pthread_cond_wait(&cond_sdsize,&mutex_sdsize);
#ifdef DEBUG
		printf("client_uphost pthread_sdsize : cond_sdsize awake\n");
#endif
		//dir=opendir("../media/mmcblkp0p1");
	//	free(dir);
	//	close(fd);
		if(!(fd>0))
		{
			dir=opendir(PATH_MEDIA);
			if(dir==NULL&&errno==ENOENT)
			{
				free(dir);
				dir=NULL;
				pthread_mutex_unlock(&mutex_sdsize);
				continue;			
			}
			if((fd=open(PATH_SD,O_RDWR|O_CREAT,0666))<0)
			{
				self_perror("open sdsize.log faild ");
				pthread_mutex_unlock(&mutex_sdsize);
				close(fd);
				free(dir);
				dir=NULL;
				continue;
			}
		}
		lseek(fd,0,SEEK_SET);
		bzero(buf,sizeof(buf));
		sprintf(buf,"df -k ");
		strcat(buf,PATH_MEDIA);
		sprintf(buf+strlen(buf)," > ");
		strcat(buf,PATH_SD);
		//if(system("df -k ../media/mmcblk0p1 > ../Data/sdsize.log")<0)
		if(system(buf)<0)
		{
			self_perror("sd card not exist ");
			pthread_mutex_unlock(&mutex_sdsize);
		//	close(fd);
			free(dir);
			dir=NULL;
			continue;
		}
		//printf("client_uphost pthread_sdsize:system(df) pass\n");
		//perror("system");
		
		bzero(buf,sizeof(buf));
		lseek(fd,70,SEEK_SET);
		if(read(fd,buf,256)<0)
		{
#ifdef DEBUG
			printf("client_uphost pthread_sdsize : read faild\n");
#endif
			pthread_mutex_unlock(&mutex_sdsize);
		//	close(fd);
			free(dir);
			dir=NULL;
			continue;//?
		}
		if(regcomp(&reg,pattern,cflags)<0)
		{
			regerror(err,&reg,errbuf,sizeof(errbuf));
#ifdef DEBUG
			printf("client_uphost pthread_sdsize : search err : %s\n",errbuf);
#endif
		}
		err=regexec(&reg,buf,nmatch,pmatch,0);
		if(err==REG_NOMATCH)
		{
#ifdef DEBUG
			printf("client_uphost pthread_sdsize : search no match\n");
#endif
			pthread_mutex_unlock(&mutex_sdsize);
		//	close(fd);
			free(dir);
			dir=NULL;
			regfree(&reg);
			continue;
		}
		else if(err)
		{
			regerror(err,&reg,errbuf,sizeof(errbuf));
#ifdef DEBUG
			printf("client_uphost pthread_sdsize : search err : %s\n",errbuf);
#endif
			pthread_mutex_unlock(&mutex_sdsize);
		//	close(fd);
			free(dir);
			dir=NULL;
			regfree(&reg);
			continue;
		}
		cap_sd=(unsigned short)(((float)(strtol(buf+pmatch[1].rm_so,NULL,10)))/1024/1024*1000);
		surcap_sd=(unsigned short)(((float)(strtol(buf+pmatch[4].rm_so,NULL,10)))/1024/1024*1000);

		if(surcap_sd<1000)
		{
			system("/rturun/rmsd.sh");
		}

#ifdef DEBUG
		printf("cap_sd %d  surcap_sd %d\n",cap_sd,surcap_sd);
		printf("       %d            %d\n",strtol(buf+pmatch[1].rm_so,NULL,10),strtol(buf+pmatch[4].rm_so,NULL,10));
		printf("       %d            %d\n",strtol(buf+pmatch[2].rm_so,NULL,10),strtol(buf+pmatch[3].rm_so,NULL,10));
		fflush(stdout);
#endif
		pthread_mutex_unlock(&mutex_sdsize);
#ifdef DEBUG
		printf("client_uphost pthread_sdsize : mutex_sdsize free\n");	
#endif
		regfree(&reg);
		free(dir);
		dir=NULL;
	//	close(fd);
	}
	return 0;
}



/******************************************************************************
 *	pthread for client to up server 
 ******************************************************************************/
void *pthread_UPclient(void *arg)
{
	//	int sockfd;
	int flag,i;
	struct sockaddr_in serv_addr;

	fd_set read_flags,write_flags;//,err_flags;
	struct timeval waitd;
	int stat;
	int stat_sel;

	//char s[128]={};
	//char ss[32]={};
	//char s[32]={};
	unsigned char str[32];

	int keepidle,keepcnt,keepintval,keepalive=1;

	self_printf("Thread UPclient start working\n");
#ifdef DEBUG
	printf("*********************pthread_UPclient in*******************\n");
#endif
	if(arg==NULL)
	{
		serv_addr.sin_family	 =AF_INET;
		//	serv_addr.sin_port 		 =htons(PORT);
		//	serv_addr.sin_addr.s_addr=inet_addr(IP);
		serv_addr.sin_port 		 =htons(atoi(hostPORT));
		serv_addr.sin_addr.s_addr=inet_addr(hostIP);
	}
	else
	{
		serv_addr.sin_family	 =AF_INET;
		serv_addr.sin_port 		 =htons(((ipinfo_s *)arg)->port);
		serv_addr.sin_addr.s_addr=inet_addr(((ipinfo_s *)arg)->ip);
	}

start:
	if((sockfd=socket(AF_INET,SOCK_STREAM,0))<0)
	{
		self_perror("create socket faild ");
		//exit(-1);
		goto start;
	}

	flag=fcntl(sockfd,F_GETFL,0);

	keepidle=120;
	keepcnt=3;
	keepintval=60;
	setsockopt(sockfd,SOL_SOCKET,SO_KEEPALIVE,(void *)&keepalive,sizeof(keepalive));
	setsockopt(sockfd,SOL_TCP,TCP_KEEPIDLE,(void *)&keepidle,sizeof(keepidle));
	setsockopt(sockfd,SOL_TCP,TCP_KEEPINTVL,(void *)&keepintval,sizeof(keepintval));
	setsockopt(sockfd,SOL_TCP,TCP_KEEPCNT,(void *)&keepcnt,sizeof(keepcnt));
	
	int reuseaddr=1;
	setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&reuseaddr,sizeof(reuseaddr));                   //socket implex useing

	//	FD_ZERO(&read_flags);
	//FD_ZERO(&write_flags);
	//FD_SET(sockfd,&write_flags);
	//	FD_SET(sockfd,&read_flags);

	while(1)
	{
		//	printf("client_uphost pthread_UPclient:while in\n");
		if((connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)))<0)
		{
		//	sprintf(ss,"connect faild ");
		//	if(strcmp(s,ss)!=0)
		//	{
		//		strcpy(s,ss);
		//		strcpy(str,s);
		//		self_perror(str);
		//		//perror(s);
		//		//fflush(stderr);
		//	}
			self_perror("connect faild ");
			close(sockfd);
			goto start;
			//continue;
		}


		fcntl(sockfd,F_SETFL,flag|O_NONBLOCK);
#ifdef DEBUG
		perror("client_uphost pthread_UPclient connect ");
		//		printf("client_uphost pthread_UPclient : connect OK!\n");
#endif

		//led booling
		pthread_create(&id_led,0,&pthread_led,NULL);

		while(1)
		{
			//self_printf("<<<<BEGINE ONE LOOP RD & WR>>>>\n");

			stat=read(sockfd,bufr,sizeof(bufr));
			if(stat>17)
				goto read_unnon;

			FD_ZERO(&read_flags);
			//FD_ZERO(&err_flags);
			//FD_SET(sockfd,&err_flags);
			FD_SET(sockfd,&read_flags);
			//waitd.tv_usec=0;
			//waitd.tv_sec=2;
			stat_sel=select(sockfd+1,&read_flags,NULL,NULL,NULL);//&waitd);
		
			if(stat_sel>0)
			{	
#ifdef DEBUG
				perror("client_uphost pthread_UPclient : select pass ");
#endif

#ifdef DEBUG
				printf("client_uphost pthread_UPclient : read in\n");
#endif 
				pthread_mutex_lock(&mutex_bufr);
#ifdef DEBUG
				printf("client_uphost pthread_UPclient : mutex_bufr own\n");
#endif
				memset(bufr,0,sizeof(bufr));
				stat=read(sockfd,bufr,sizeof(bufr));
				//if(stat<0)
				if(!(stat>0))
				{
					self_perror("socket read ");
					if(stat==0)
					{
							pthread_mutex_unlock(&mutex_bufr);
							pthread_cancel(id_led);
							pthread_join(id_led,NULL);
							led_ctrl("D6",ON);
							close(sockfd);
							goto start;
					}
					else
					{
						if(errno==EAGAIN||errno==EINTR||errno==EWOULDBLOCK)
							continue;
						else
							//if(errno==ETIMEDOUT)
						{
							pthread_mutex_unlock(&mutex_bufr);
							pthread_cancel(id_led);
							pthread_join(id_led,NULL);
							led_ctrl("D6",ON);
							close(sockfd);
							goto start;
						}
					}

				}
read_unnon:
				pthread_mutex_lock(&mutex_led);
				flag_led=2;
				pthread_mutex_unlock(&mutex_led);
				pthread_kill(id_led,SIGUSR2);

#ifdef DEBUG
				printf("client_uphost pthread_UPclient : select read : %s\n",bufr);
#endif
				pthread_mutex_unlock(&mutex_bufr);
#ifdef DEBUG
				printf("client_uphost pthread_UPclient : mutex_bufr free\n");
#endif

				pthread_mutex_lock(&mutex_buft);
#ifdef DEBUG
				printf("client_uphost pthread_UPclient : mutex_buft own\n");
#endif
				
				usleep(500);
				pthread_cond_signal(&cond_framere);
				pthread_cond_wait(&cond_write,&mutex_buft);

				//	pthread_create(&id_led,0,&pthread_led,NULL);
			//	pthread_cond_signal(&cond_framere);
				//	pthread_mutex_lock(&mutex_led);
				//	flag_led=1;
				//	pthread_mutex_unlock(&mutex_led);
				//	pthread_cond_signal(&cond_led);
#ifdef DEBUG
				printf("client_uphost pthread_UPclient : send cond_framere\n");
#endif

				//self_printf("socket read : ");
				printf("<<RTU>>\n");
				self_printf("receive from host : ");
				for(i=0;i<stat;i++)
				{
					printf("%02x",bufr[i]);
				}
				printf("\n");
				fflush(stdout);

				
				if(strlen(buft)!=0)
				{
					if(flag_loop==0)
					{
						//stat=write(sockfd,buft,sizeof(bufdata));
						stat=write(sockfd,buft,size_buft);
						if(stat<0)
						{
							self_perror("write socket ");
							if(errno==ENOTCONN)
							{
								break;	
							}
							if(errno==EPIPE)
							{
								pthread_mutex_unlock(&mutex_buft);
								pthread_cancel(id_led);
								pthread_join(id_led,NULL);
								led_ctrl("D6",ON);
								close(sockfd);
								goto start;
							}

						}

						pthread_mutex_lock(&mutex_led);
						flag_led=2;
						pthread_mutex_unlock(&mutex_led);
						pthread_kill(id_led,SIGUSR2);

						//	printf("client_uphost pthread_UPclient:select write:%s\n",buft);
						//					memset(buft,0,sizeof(buft));

					}
					else
					{
					/*	int i;*/
						unsigned char a[1048];

						afn06t_s afn06t;
						if(seql==255)
							seql=0;
						else
							seql+=1;
						//	seql++;
						if(flag_reason!=0)
						{
							/**/
							if(flag_reason==0xff)                //PASSBY 32kb
							{
								for(i=0;i<flag_loop;i++)
								{
									memcpy(buft,buf32k[i],1048);

									FD_ZERO(&write_flags);
									FD_SET(sockfd,&write_flags);
									waitd.tv_sec=1;
									stat_sel=select(sockfd+1,NULL,&write_flags,NULL,&waitd);
									if(!(stat_sel>0))
										continue;
									stat=write(sockfd,buft,sizeof(buft));
									if(stat<0)
									{
										self_perror("write socket ");
										if(errno==EAGAIN)
										{
											i--;
										//	usleep(100);
											continue;
										}
										if(errno==ENOTCONN)
										{
											break;
										}
										if(errno==EPIPE)
										{
											pthread_mutex_unlock(&mutex_buft);
											pthread_cancel(id_led);
											pthread_join(id_led,NULL);
											led_ctrl("D6",ON);
											close(sockfd);
											goto start;
										}
									}
								}
							}
							/**/
							else
							{
								funafn06t(a,&afn06t,flag_reason);
								stat=write(sockfd,a,1043);
								if(stat<0)
								{
									self_perror("write socket ");
									if(errno==ENOTCONN)
									{
										break;
									}
									if(errno==EPIPE)
									{
										pthread_mutex_unlock(&mutex_buft);
										pthread_cancel(id_led);
										pthread_join(id_led,NULL);
										led_ctrl("D6",ON);
										close(sockfd);
										goto start;
									}
								}
							}
						//	pthread_mutex_lock(&mutex_led);
						//	flag_led=2;
						//	pthread_mutex_unlock(&mutex_led);
						//	pthread_kill(id_led,SIGUSR2);
						}
						else
						{
							for(i=0;i<flag_loop;i++)
							{
								afn06t.frameno=i+1;
								memcpy(afn06t.data,buf32k[i],1024);
								funafn06t(a,&afn06t,flag_reason);

								stat=write(sockfd,a,1043);
								if(stat<0)
								{
									self_perror("write socket ");
									if(errno==EAGAIN)
									{
										i--;
									//	usleep(100);
										continue;
									}
									if(errno==ENOTCONN)
									{
										break;
									}
									if(errno==EPIPE)
									{
										pthread_mutex_unlock(&mutex_buft);
										pthread_cancel(id_led);
										pthread_join(id_led,NULL);
										led_ctrl("D6",ON);
										close(sockfd);
										goto start;
									}
								}

							//	pthread_mutex_lock(&mutex_led);
							//	flag_led=2;
							//	pthread_mutex_unlock(&mutex_led);
							//	pthread_kill(id_led,SIGUSR2);
							}
						}

						pthread_mutex_lock(&mutex_led);
						flag_led=2;
						pthread_mutex_unlock(&mutex_led);
						pthread_kill(id_led,SIGUSR2);

						flag_loop=0;
						flag_reason=0;
					}

				/*	int i;    */
				//	self_printf("socket write : ");
					self_printf("send to host : ");
					for(i=0;i<stat;i++)       
					{
						printf("%02x",buft[i]);
					}
					printf("\n");
					fflush(stdout);

					//memset(buft,0,sizeof(buft));
					bzero(buft,sizeof(buft));
					size_buft=sizeof(buft);
				}
				else
					self_printf("transmit buf NULL \n");
				pthread_mutex_unlock(&mutex_buft);

				//	pthread_mutex_lock(&mutex_led);
				//	flag_led=0;
				//	pthread_mutex_unlock(&mutex_led);
#ifdef DEBUG
				printf("client_uphost pthread_UPclient : mutex_buft free\n");
#endif
			}
			else if(stat_sel<0)
			{
				self_perror("select faild ");
				continue;
			}

			else if(stat_sel==0)
			{
				if(getpeername(sockfd,NULL,NULL)<0&&errno==ENOTCONN)
				{	
					pthread_cancel(id_led);
					pthread_join(id_led,NULL);
					led_ctrl("D6",ON);
					close(sockfd);
					goto start;
				}
				self_perror("select over time ");
				continue;
			}
		}

		pthread_mutex_unlock(&mutex_buft);
	
		pthread_cancel(id_led);
		pthread_join(id_led,NULL);
		led_ctrl("D6",ON);

		close(sockfd);
		goto start;

	}
	close(sockfd);
	return NULL;
}



void releaseresouce(int signo)
{
	led_ctrl("D6",OFF);                                        
	//pthread_mutex_destroy(&mutex_socket);
	pthread_mutex_destroy(&mutex_shmbuf);
	pthread_mutex_destroy(&mutex_errno);
	pthread_mutex_destroy(&mutex_sdsize);
	pthread_mutex_destroy(&mutex_led);

	pthread_cond_destroy(&cond_framere);
	pthread_cond_destroy(&cond_shm);
	pthread_cond_destroy(&cond_fddata);
	pthread_cond_destroy(&cond_fderrlog);
	pthread_cond_destroy(&cond_write);
	pthread_cond_destroy(&cond_sdsize);
	//pthread_cond_destroy(&cond_led);

	shmctl(shmid,IPC_RMID,NULL);
	msgctl(msgid,IPC_RMID,NULL);
//	semctl(semid,1,IPC_RMID,NULL);

	pthread_cancel(id_UPclient);
	pthread_cancel(id_parm);
	pthread_cancel(id_vol);
	pthread_cancel(id_framere);
	pthread_cancel(id_shm);
	//pthread_cancel(id_wrong);
	pthread_cancel(id_sdsize);
	pthread_cancel(id_led);

//	close(fd_errlog);
//	close(fd_data);
#ifndef DISPLAY
	fclose(stdout);
	fclose(stderr);
#endif
	close(sockfd);
	led_ctrl("D6",OFF);                                        
#ifdef DEBUG
	printf("********************* releaseresouce ************************\n");
#endif
	close(fd_parm);
	close(fd_ledsix);
	return ;//exit(0);
}

void handler_parm(int signo)
{
	pthread_create(&id_parm,0,&pthread_parm,NULL);
	sleep(2);
	pthread_cond_signal(&cond_shm);
	return;
}



int main(int argc, const char *argv[])                            //client_uphost
{
	signal(SIGTSTP,releaseresouce);
	//signal(SIGINT,releaseresouce);
	signal(SIGINT,handler_parm);
	signal(SIGPIPE,SIG_IGN);

#ifndef DISPLAY
	freopen(PATH_CLIWR,"a+",stdout);
	freopen(PATH_CLIWR,"a+",stderr);
#endif	
	fd_ledsix = open ("/sys/class/leds/d6/brightness", O_WRONLY);
	self_printf("UPclient START WORKING\n");
	
	//	printf("main:IP:%s	PORT:%d  argv:%p\n",argv[1],atoi(argv[2]),argv);
	
	pthread_mutex_init(&mutex_errno,NULL);
	pthread_mutex_init(&mutex_shmbuf,NULL);
	pthread_mutex_init(&mutex_sdsize,NULL);
	pthread_mutex_init(&mutex_led,NULL);
	
	//pthread_cond_init(&cond_led,NULL);
	pthread_cond_init(&cond_framere,NULL);
	pthread_cond_init(&cond_shm,NULL);
	pthread_cond_init(&cond_sdsize,NULL);
	//pthread_cond_init(&cond_readshm,NULL);
	pthread_cond_init(&cond_fddata,NULL);
	pthread_cond_init(&cond_fderrlog,NULL);
	pthread_cond_init(&cond_write,NULL);
	
	fd_parm=open(PATH_PARM,O_RDWR|O_CREAT,0x666);
	sleep(1);

	pthread_create(&id_parm,0,&pthread_parm,NULL);
	sleep(2);
	pthread_create(&id_vol,0,&pthread_vol,NULL);
	pthread_create(&id_framere,0,&pthread_framere,NULL);
	pthread_create(&id_shm,0,&pthread_shm,NULL);

	if(argc==3)
	{
		ipinfo_s ipinfo;
		ipinfo.port=atoi(argv[2]);
		ipinfo.ip=argv[1];
		pthread_create(&id_UPclient,0,&pthread_UPclient,&ipinfo);
	}
	else
	{
		if(argc==5)                          //./UPclient set rtu addr argv[4]
			rtuaddr=strtol(argv[4],NULL,10);
		pthread_create(&id_UPclient,0,&pthread_UPclient,NULL);
	}
	pthread_create(&id_sdsize,0,&pthread_sdsize,NULL);

	led_ctrl("D6",ON);
//	pthread_create(&id_led,0,&pthread_led,NULL);
//	pthread_create(&id_wrong,0,&pthread_wrong,NULL);

//	pthread_join(id_parm,NULL);
	pthread_join(id_UPclient,NULL);
//	pthread_join(id_wrong,NULL);
	pthread_join(id_framere,NULL);
	pthread_join(id_shm,NULL);
	pthread_join(id_sdsize,NULL);
	pthread_join(id_led,NULL);
	return 0;
}


#include<signal.h>
#include<sys/time.h>
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
#include"data_global.h"
#include<pthread.h>
#include"sem.h"
#include"linklist.h"
#include<regex.h>
#include<sys/stat.h>
#include<netinet/tcp.h>

//#define N 64
#define N sizeof(shm_s)
#define M 512 
#define IP "192.168.1.11"
#define PORT 8899

#define F_TRMID 0xf1               //interrupt during transmit data
#define F_CON   0xf2			   //after connect and send,no reply

#define POSITIVE 0x01			   //self report
#define NAGETIVE 0x02              //called measure

#define TEST_F  0x1f               //the type of frame to be send
#define TIME_F  0x2f
#define REQDATA_F 	0x3f
#define REREQDATA_F 	0x4f
//#define NONE   0x0f
#define REPEAT 0x0f

//#define LATESTTEST 0x11             //use for shm     0x10-address is 0xffffffff
//									  //				0x01-data in shm is new 
//#define PASSBY     0x21             //passby frame   
#define SHM_NULL     0x30             //already read
#define N_NODE 4                      //node number

#define REVISE_TIME  1                //used revise node time    per minit

int fd_parm;

/******************************************************************************
 *	time stamp
 *	TIME : standard put out
 *	TIME2 : standard error put out
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
						strncat(String,str,48);\
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

void self_printf_fd(unsigned char *str,int fd)
{
	unsigned char buf[400];
	time_t t;
	struct tm *tim;
	time(&t);
	tim=localtime(&t);
	sprintf(buf,"%d.%02d.%02d-%02d:%02d:%02d : %s",tim->tm_year+1900,tim->tm_mon+1,tim->tm_mday,tim->tm_hour,tim->tm_min,tim->tm_sec,str);
	write(fd,buf,strlen(buf));
	return;

}

void self_perror_fd(unsigned char *str,int fd)
{
	unsigned char buf[400];
	time_t t;
	struct tm *tim;
	time(&t);
	tim=localtime(&t);
	sprintf(buf,"%d.%02d.%02d-%02d:%02d:%02d : %s\n",tim->tm_year+1900,tim->tm_mon+1,tim->tm_mday,tim->tm_hour,tim->tm_min,tim->tm_sec,str);
	write(fd,buf,strlen(buf));
	return;	
}

/**********************************************************************************/

#define LEN_WAVH 44

unsigned char wav_head[]={0x52,0x49,0x46,0x46,0x24,0x80,0x00,0x00,0x57,0x41,0x56,0x45,0x66,0x6d,0x74,0x20,
		  0x10,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x80,0xbb,0x00,0x00,0x80,0xbb,0x00,0x00,
          0x02,0x00,0x10,0x00,0x64,0x61,0x74,0x61,0x00,0x80,0x00,0x00,0x3e};



nodeprivate_s nodeprivate[N_NODE];

store_linklist *store_head,*store_tail;          //linklist contain 32kb data
test_linklist *test_head,*test_tail;             //linklist contain latest communication time and link_test_frame data

unsigned char flag_online=0;                     //used for judging which node is online,order is node : 4 3 2 1  			online--1
char flag_led;                                   // -1--turn off  >0--booling  1--node login  2--node outline 
//unsigned char flag_done[4];                      // record 32kb done compelety or not

unsigned char switch_timeint_loc;                //RTU time interval
int timeint_loc[3];
int timeint_loc_bak[3];

unsigned char seql;

typedef struct alarm
{
	unsigned char nodeno;
	unsigned char switch_timeint;
	int timeint[3];
} alarm_s;

pthread_t id_beep,
		  id_led,
		  id_shm,
		  id_parm,
		  id_msg,
		  id_test,
		  id_restore32kb;

unsigned char rtuaddr;

unsigned char serverIP[16];
unsigned char serverPORT[6];
unsigned char overtime_server;                             //second for unit
unsigned char time_repeat;                                 //minute for unit   -- how many seconds send testframe once

unsigned int duration;

/************************************
 *	pthread of read timeint information and rtuaddr
 *	
 *************************************/
void *pthread_parm(void *arg)
{
	pthread_detach(pthread_self());
#ifdef DEBUG
	printf("**********************pthread_parm  in*********************\n");
#endif
	int i,j,cnt=0;
	unsigned char buf[256]={};
	unsigned char bak[8]={};

	//self_printf("Thread parm start working\n");
	if(!(fd_parm>0))
	{
	if((fd_parm=open(PATH_PARM,O_RDWR|O_CREAT,0x666))<0)
	{
		close(fd_parm);
		self_perror("open parm.log fault ");
		return;
	}
	}
	//while((cnt=read(fd_parm,buf,sizeof(buf)))<217);
	cnt=read(fd_parm,buf,sizeof(buf));

	memcpy(bak,buf+8,2);
	rtuaddr=strtol(bak,NULL,10);
	bzero(bak,sizeof(bak));

	memcpy(bak,buf+45,2);
	switch_timeint_loc=strtol(bak,NULL,16);
	bzero(bak,sizeof(bak));

	memcpy(bak,buf+59,2);
	timeint_loc[0]=strtol(bak,NULL,10);
	bzero(bak,sizeof(bak));

	memcpy(bak,buf+73,2);
	timeint_loc[1]=strtol(bak,NULL,10);
	bzero(bak,sizeof(bak));

	memcpy(bak,buf+87,2);
	timeint_loc[2]=strtol(bak,NULL,10);
	bzero(bak,sizeof(bak));

	memcpy(bak,buf+118,2);
	duration=strtol(bak,NULL,10);
	bzero(bak,sizeof(bak));

	memcpy(serverIP,buf+130,15);
	memcpy(serverPORT,buf+157,5);

	memcpy(bak,buf+217,2);
	overtime_server=strtol(bak,NULL,16);
	bzero(bak,sizeof(bak));

	memcpy(bak,buf+251,2);
	time_repeat=strtol(bak,NULL,16);

#ifdef DEBUG
	printf("time repeat %d\n",time_repeat);
	printf("parm %02x %02x %d %d %d\n",rtuaddr,switch_timeint_loc,timeint_loc[0],timeint_loc[1],timeint_loc[2]);
#endif

//	while(1)
//	{
	//	lseek(fd_parm,SEEK_SET,217);
	//	read(fd_parm,buf,sizeof(buf));
	//	memcpy(bak,buf+217,2);
	//	overtime_server=strtol(bak,NULL,16);
	//	bzero(bak,sizeof(bak));

	//	lseek(fd_parm,SEEK_SET,251);
	//	read(fd_parm,buf,sizeof(buf));
	//	memcpy(bak,buf+251,2);
	//	time_repeat=strtol(bak,NULL,16);
	//	bzero(bak,sizeof(bak));

//		sleep(2);
//	}
	for(i=0;i<3;i++)
	{
		if(timeint_loc[i]!=timeint_loc_bak[i])
		{
			for(j=0;j<N_NODE;j++)
			{
				nodeprivate[j].flag_timeint[i]=0;
			}
			timeint_loc_bak[i]=timeint_loc[i];
		}
	}
//	close(fd_parm);
//	fd_parm=0;
	lseek(fd_parm,0,SEEK_SET);
	return;
}


/*********************************
 * pthread of record the result of node testframe
 * 
 *********************************/
void *pthread_testframe(void *arg)
{
	int fd_t;
	struct tm *tim;
	unsigned char path[64];
	int i;
#ifdef DEBUG
	printf("*****************************pthread_testframe in***********************\n");
#endif
	test_head=test_CreateLinklist();
	test_tail=test_head;

	pthread_mutex_lock(&mutex_testhead);
	
	mkdir(PATH_LINK,S_IRWXU|S_IRWXG|S_IRWXO);

	for(i=0;i<N_NODE;i++)
	{
		bzero(path,sizeof(path));
		strcpy(path,PATH_LINK);
		sprintf(path+strlen(PATH_LINK),"node%d",i+1);
		mkdir(path,S_IRWXU|S_IRWXG|S_IRWXO);
	//	if(mkdir(path,S_IRWXU|S_IRWXG|S_IRWXO)<0)
	//		self_perror(path);
	}
//	mkdir("../Data/linkdata/node1",S_IRWXU);
//	mkdir("../Data/linkdata/node2",S_IRWXU);
//	mkdir("../Data/linkdata/node3",S_IRWXU);
//	mkdir("../Data/linkdata/node4",S_IRWXU);
#ifdef DEBUG
	perror("mkdir ../Data/linkdata/node_ ");
#endif
test:
	while(!test_EmptyLinklist(test_head))
	{
		test_linklist *p;
		p=test_GetLinknode(test_head);
		pthread_mutex_unlock(&mutex_testhead);

		tim=localtime(&(p->t));
		bzero(path,sizeof(path));
	//	strcpy(path,PATH_LINK);
	//	sprintf(path+strlen(PATH),"node%d/testframe.log",p->nodeno);
		sprintf(path,"/media/mmcblk0p1/Data/linkdata/node%d/testframe.log",p->nodeno);          //nodenum wait to deal
		if((fd_t=open(path,O_RDWR|O_CREAT,0666))<0)               //O_APPEND
		{
			continue;
		}
		
		bzero(path,sizeof(path));
		sprintf(path,"%d%02d%02d%02d%02d%02d",tim->tm_year+1900,tim->tm_mon+1,tim->tm_mday,tim->tm_hour,tim->tm_min,tim->tm_sec);
		int i=0;
		for(i=0;i<21;i++)
		{
			sprintf(path+14+(i*2),"%02x",p->buf[i]);
		}
		write(fd_t,path,strlen(path));
		write(fd_t,"\n",1);
#ifdef DEBUG
		perror("141 write linkdata ");
#endif
		close(fd_t);
		free(p);
		p=NULL;
		pthread_mutex_lock(&mutex_testhead);
	}
	pthread_cond_wait(&cond_testhead,&mutex_testhead);
	goto test;
	return;
}


/*********************************
 *	pthread of read shm   
 *	function : share memory get timeint information 
 *	???shm add a flag? mark shm is new or not
 *********************************/
void *pthread_shm(void *arg)
{
	shm_s *pshmbuf;
	key_t key_info;
#ifdef DEBUG	
	printf("*************************server pthread_shm in****************************\n");
#endif
start_shm:
	if ((key_info = ftok (PATH_SHM, 'i')) < 0)
	{
		//perror("server pthread_shm:ftok info\n");
		//exit (-1);
		goto start_shm;
	}
	//printf ("server pthread_shm:key = %d\n", key_info);

//	if ((semid = semget (key_info, 1, IPC_CREAT | IPC_EXCL |0666)) < 0)
//	{
//		if (errno == EEXIST)
//		{
//			semid = semget (key_info, 1, 0666);
//		}
//		else
//		{
//			self_perror ("DWserver pthread_shm:semget\n");
//			//exit (-1);
//			goto start_shm;
//		}
//	}
//	else
//	{
//		init_sem (semid, 0, 2);
//	}
	
//	printf("DWserver pthread_shm :shm bellow\n");
	if ((shmid = shmget (key_info, N, IPC_CREAT | IPC_EXCL | 0666)) < 0)
	{
		if (errno == EEXIST)
		{
			shmid = shmget(key_info, N, 0666);
			pshmbuf = (shm_s*)shmat (shmid, NULL, 0);
			bzero (pshmbuf, sizeof(shm_s));
		}
		else
		{
			self_perror ("share memory get ");
			//exit (-1);
			goto start_shm;
		}
	}
	else
	{
		if ((pshmbuf = (shm_s*)shmat (shmid, NULL, 0)) == (void *)-1)
		{
			self_perror ("share memory map ");
			//exit (-1);
			goto start_shm;
		}
	}

	pthread_mutex_lock(&mutex_shmbuf);                            
	while(1)
	{
#ifdef DEBUG
		printf("DWserver pthread_shm:poll\n");
#endif
	//	sem_p(semid,0); 

	//	switch_timeint_loc=pshmbuf->switch_timeint;
		int n;
	//	for(n=0;n<3;n++)
	//		timeint_loc[n]=pshmbuf->timeint[n];

		//pthread_mutex_lock(pthread_mutex_lock(&mutex_online);mutex_online);

	/*	overtime_server=pshmbuf->overtime_server;
		time_repeat=pshmbuf->time_repeat;*/
		pshmbuf->flag_online=flag_online;
		//pthread_mutex_unlock(pthread_mutex_unlock(&mutex_online);mutex_online);

		for(n=0;n<N_NODE;n++)
		{
			pshmbuf->disply_node[n].node_addr=nodeprivate[n].node_addr;
			memcpy(pshmbuf->disply_node[n].ip,nodeprivate[n].ip,16);
			pshmbuf->disply_node[n].port=nodeprivate[n].port;
			pshmbuf->disply_node[n].t_login=nodeprivate[n].t_login;
			pshmbuf->disply_node[n].t_login_bak=nodeprivate[n].t_login_bak;
			pshmbuf->disply_node[n].t_logoff=nodeprivate[n].t_logoff;
			pshmbuf->disply_node[n].t_logoff_bak=nodeprivate[n].t_logoff_bak;
			pshmbuf->disply_node[n].rx=nodeprivate[n].rx;
			pshmbuf->disply_node[n].tx=nodeprivate[n].tx;
			memcpy(&(pshmbuf->disply_node[n].afnbroadtser),&(nodeprivate[n].afnbroadtser),sizeof(afnbroadtser_s));
		}

	//	sem_v(semid,0);
	//	pthread_mutex_unlock(&mutex_shmbuf);
	//	pthread_mutex_lock(&mutex_shmbuf);                            
		pthread_cond_wait(&cond_shm,&mutex_shmbuf);
	}
	return;
}


/***********************************************
 *	pthread read/write message queue to UP_client
 ***********************************************/

void *pthread_msg(void *arg)
{
	key_t key;
#ifdef DEBUG
	printf("******************************pthread_msg in******************************\n");
#endif
start_msg:	
	if ((key = ftok (PATH_MSG, 'g')) < 0)
	{
		perror ("ftok msgqueue ");
	//	exit (-1);
		goto start_msg;
	}
	if ((msgid = msgget (key, IPC_CREAT | IPC_EXCL | 0666)) < 0)
	{
		if(errno == EEXIST)
		{
			msgid = msgget(key, 0666);
		}
		else
		{
			self_perror ("msgget ");
			//exit (-1);
			goto start_msg;
		}
	}

	msgtoser_s msgbuf;
	msgtocli_s msgbuff;
	unsigned char nodeno;
	unsigned char str[72];
	
	int n;

	while(1)
	{
		msgrcv(msgid,&msgbuf,sizeof(msgbuf)-sizeof(long),MSG_TOSER,0);
#ifdef DEBUG
		printf("331 msgrcv\n");
#endif

		bzero(str,sizeof(str));
		sprintf(str,"receive message : ");
		for(n=0;n<sizeof(str);n++)
		{
			sprintf(str+strlen(str),"%02x",msgbuf.buf[n]);
		}
		strcat(str,"\n");
		self_printf(str);
#ifdef DEBUG
		for(n=0;n<4;n++)
			printf("addr[%d]=%02x\n",n,msgbuf.addr[n]);
		fflush(stdout);
#endif
		nodeno=0;
		for(n=0;n<N_NODE;n++)
		{
			if(nodeprivate[n].node_addr==msgbuf.addr[2])
			{
				nodeno=n+1;
				break;
			}
		}

		//pthread_mutex_lock(pthread_mutex_lock(&mutex_online);mutex_online);
		if((flag_online&(1<<(nodeno-1)))==0)										//0xe3 not online
		{
			//pthread_mutex_unlock(pthread_mutex_unlock(&mutex_online);mutex_online);
			//printf("396\n");
			msgbuff.msgtype=MSG_TOCLIENT;
			msgbuff.err_flag=1;
			funoutline(&msgbuf,&msgbuff);
			msgsnd(msgid,&msgbuff,sizeof(msgbuff)-sizeof(long),0);
			self_printf("sended outline message\n");
	//		printf("%02x\n",flag_online);
	//		fflush(stdout);
			//printf("400 msgsnd\n");
		}
		else
		{
			if(nodeprivate[nodeno-1].busy==1)
			{
				msgbuff.msgtype=MSG_TOCLIENT;
				msgbuff.err_flag=1;
				funbusy(&msgbuf,&msgbuff);
				msgsnd(msgid,&msgbuff,sizeof(msgbuff)-sizeof(long),0);
				self_printf("sended busy message\n");
			}
			else
			{
			pass_InsertLinklist_tail(nodeprivate[nodeno-1].pass_head,&msgbuf);
			int err;
			err=pthread_kill(nodeprivate[nodeno-1].tid,SIGRTMIN+nodeno-1);
			self_perror("pthread_kill ");
#ifdef DEBUG
			printf("tid %ld    err %d\n",nodeprivate[nodeno-1].tid,err);
			fflush(stdout);
#endif
			}
		}
	}
	return NULL;
}

/****************************************
 *	pthread alarm 
 *	function : according to the local time interval , if time is up send signal
 *			   to the process,if the node workmode is self_report ,it ignore the
 *			   SIGALRM
 *			But not know whether every pthread will catch the signal or not
 *****************************************/

/*****************************************
 *	pthread to restore the 32kb data   and  write index.log in each YY-MM-DD floader      
 *	fun:     get data from data_linklist
 *			 record the data into file
 * *****************************************/
void *pthread_restore32kb(void *arg)
{
	unsigned char path[64];
	struct tm *tim;
	int fd;
#ifdef DEBUG
	printf("**********************pthread_restore32kb in ****************\n");
#endif
	store_head=store_CreateLinklist();
	store_tail=store_head;

	pthread_mutex_lock(&mutex_storehead);
store:
	while(!store_EmptyLinklist(store_head))                     //is empty
	{
#ifdef DEBUG
		printf("store while in\n");
#endif
		store_linklist *p;
		p=store_GetLinknode(store_head);
		pthread_mutex_unlock(&mutex_storehead);
		
		tim=localtime(&(p->t));
		
		bzero(path,sizeof(path));
		sprintf(path,"/media/mmcblk0p1/%d-%02d-%02d",tim->tm_year+1900,tim->tm_mon+1,tim->tm_mday);
		mkdir(path,S_IRWXU|S_IRWXG|S_IRWXO);

		sprintf(path,"/media/mmcblk0p1/%d-%02d-%02d/node%d",tim->tm_year+1900,tim->tm_mon+1,tim->tm_mday,p->nodeno);
		mkdir(path,S_IRWXU|S_IRWXG|S_IRWXO);
		sprintf(path,"/media/mmcblk0p1/%d-%02d-%02d/node%d/sensor%d",tim->tm_year+1900,tim->tm_mon+1,tim->tm_mday,p->nodeno,p->sensorno);
		mkdir(path,S_IRWXU|S_IRWXG|S_IRWXO);


		bzero(path,sizeof(path));
		sprintf(path,"/media/mmcblk0p1/%d-%02d-%02d/index.log",tim->tm_year+1900,tim->tm_mon+1,tim->tm_mday);
		fd=open(path,O_RDWR|O_CREAT,0666);

		
		bzero(path,sizeof(path));
		sprintf(path,"/timeint_%d/node%d/sensor%d/%02d:%02d:%02d.wav\n",p->timeintno,p->nodeno,p->sensorno,tim->tm_hour,tim->tm_min,tim->tm_sec);
		lseek(fd,0,SEEK_END);
		write(fd,path,strlen(path));
		close(fd);
		
		bzero(path,sizeof(path));
		sprintf(path,"/media/mmcblk0p1/%d-%02d-%02d/node%d/sensor%d/%02d.%02d.%02d.wav",tim->tm_year+1900,tim->tm_mon+1,tim->tm_mday,p->nodeno,p->sensorno,tim->tm_hour,tim->tm_min,tim->tm_sec);
		fd=open(path,O_RDWR|O_CREAT,0666);

	
		int i;
		if(p->buf[0][0]!='N')
		{
			write(fd,wav_head,LEN_WAVH);
			for(i=0;i<32;i++)
			{
				//if(i==0&&p->buf[0][0]!='N')
				//	write(fd,"Y",1);
				write(fd,p->buf[i],1024);
			//	write(fd,"\n",1);

		//	if(i==0&&p->buf[0][0]=='N')
		//	{
		//		write(fd,p->buf[0],4);
		//		break;
		//	}
			}
		}
		else
		{
			write(fd,p->buf[0],4);
		}

		close(fd);

		free(p);
		p=NULL;
		pthread_mutex_lock(&mutex_storehead);
	}
#ifdef DEBUG
	printf("wait cond_storehead\n");
#endif
	pthread_cond_wait(&cond_storehead,&mutex_storehead);
#ifdef DEBUG
	printf("goto store\n");
#endif
	goto store;
	return;
}

/****************************************
 *	signal handler function
 ****************************************/
void funhandler(int nodeno)
{
	unsigned char buf[1042]={};
	unsigned char string[156]={};
	int stat,size,count,flag,repeat=2;
	fd_set readfds;
	struct timeval wait;
	int confd=nodeprivate[nodeno-1].connectfd;
	pass_linklist *p;
	msgtocli_s msgbuff;

	msgbuff.msgtype=MSG_TOCLIENT;
	msgbuff.err_flag=0;

	p=pass_GetLinknode(nodeprivate[nodeno-1].pass_head);

//	flag=fcntl(confd,F_GETFL,0);
//	fcntl(confd,F_SETFL,flag|O_NONBLOCK);

	switch(p->afn)
	{
		case 0x01:
			size=18;
			break;
		case 0x04:
			size=25;
			break;
		case 0xf2:
			size=26;
			break;
		case 0xf0:
			size=19;
			break;
		case 0xf1:
			size=20;
			break;
		case 0x02:                                    //2014.10.10 改 使支持透传更改地址
			size=23;
			break;
		default:
			size=0;
			break;
	}
	while(repeat--)
	{
	/*	count=write(confd,p->buf,sizeof(p->buf));*/
		count=write(confd,p->buf,size);
	/*	if(count<sizeof(p->buf))*/
		if(count<size)
		{
			bzero(string,sizeof(string));
			sprintf(string,"node %d(%s) wrote message",nodeno,nodeprivate[nodeno-1].ip);
			self_perror_fd(string,nodeprivate[nodeno-1].fd_log);//"wrote message ");
			if(repeat==0)
			{
				msgbuff.err_flag=1;
				funsndfail(&msgbuff,p->addr,p->seq,p->afn);
				msgsnd(msgid,&msgbuff,sizeof(msgbuff)-sizeof(long),0);
				goto snd;
			}
			usleep(500);
			
		}
		else
		{
			bzero(string,sizeof(string));
			sprintf(string,"node %d(%s) wrote message ok",nodeno,nodeprivate[nodeno-1].ip);
			self_perror_fd(string,nodeprivate[nodeno-1].fd_log);
			break;
		}
	}
	if(p->afn==0xf0||p->afn==0xf1)
	{
		count=1042;
		setsockopt(confd,SOL_SOCKET,SO_RCVLOWAT,&count,sizeof(count));                   //socket implex useing
	}
	if(p->afn==0xf0)
		count=32;
	else
		count=1;
	while(count--)
	{
		FD_ZERO(&readfds);
		FD_SET(confd,&readfds);
		if(p->afn==0xf0||p->afn==0xf1)
		{
			(overtime_server==0)?(wait.tv_sec=2):(wait.tv_sec=(overtime_server<<1));
			wait.tv_usec=0;
		}
		else
		{
			(overtime_server==0)?(wait.tv_sec=1):(wait.tv_sec=overtime_server);
			wait.tv_usec=0;
		}
#ifdef DEBUG
		printf("645\n");
#endif
		stat=select(confd+1,&readfds,NULL,NULL,&wait);
		//if(select(confd+1,&readfds,NULL,NULL,&wait)<=0)
#ifdef DEBUG
		perror("651 ");
		printf("%02x   %d\n",p->addr[1],stat);
#endif
		if(!(stat>0))
		{
			if(p->afn==0xf0)
				continue;
			else
			{
				msgbuff.err_flag=1;
				funouttime(&msgbuff,p->addr,p->seq,p->afn);
				msgsnd(msgid,&msgbuff,sizeof(msgbuff)-sizeof(long),0);
				break;
			}
		}
		else
		{
			if(FD_ISSET(confd,&readfds))
			{
				int i;
				i=read(confd,buf,sizeof(buf));
				memcpy(msgbuff.buf,buf,i);
			}
		}
		msgsnd(msgid,&msgbuff,sizeof(msgbuff)-sizeof(long),0);
	}

snd:
	bzero(string,sizeof(string));
	sprintf(string,"node %d(%s) send message to client\n",nodeno,nodeprivate[nodeno-1].ip);
	self_printf_fd(string,nodeprivate[nodeno-1].fd_log);//"send message to client\n");
	while(read(confd,buf,sizeof(buf))>0);
	if(p->afn==0xf0||p->afn==0xf1)
	{
		count=1;
		setsockopt(confd,SOL_SOCKET,SO_RCVLOWAT,&count,sizeof(count));                   //socket implex useing
	}
//	if(p->afn==0xf0)
//	{
//		while(!(select(confd+1,&readfds,NULL,NULL,0)>0))
//		{
//			read(confd,buf,sizeof(buf));
//		}
//	}
	bzero(string,sizeof(string));
	sprintf(string,"receive node %d(%s) sended data over , afn %02x\n",nodeno,nodeprivate[nodeno-1].ip,p->afn);
	self_printf_fd(string,nodeprivate[nodeno-1].fd_log);//"send message to client\n");

	self_printf("send message to client\n");
	free(p);
	p=NULL;
	return;
}

void handler_one(int signo)
{
#ifdef DEBUG
	self_printf("handler_one in\n");
#endif
	funhandler(1);	
	return ;
}

void handler_two(int signo)
{
	funhandler(2);
	return;
}

void handler_three(int signo)
{
	funhandler(3);
	return;
}

void handler_four(int signo)
{
	funhandler(4);
	return;
}



/*****************************************
 *	subfunction used for pthread_dealclient
 *****************************************/
//int send_cmd(unsigned char nodeno,unsigned char frametype,unsigned char *seqs,unsigned short parm,unsigned char timeintno)
int send_cmd(unsigned char nodeno,unsigned char frametype,unsigned char *seqs,int parm,unsigned char timeintno)
{
	unsigned char bufrecord[32][1025]={};
	int repeat=0;
	unsigned int confd=nodeprivate[nodeno-1].connectfd;
	unsigned int flag_data=0;                                              //used for judging if 32kb receive all or not
	//unsigned int flag_data=nodeprivate[nodeno-1].flag_sensor[timeintno-1][parm-1];
	unsigned char *pattern;
	unsigned char buf[1042<<2+1]={};
	unsigned char bufr[1042]={};
	unsigned char buft[24]={};
	unsigned char string[128];
	int len=0;                                                             //pmatch[0].rm_eo-pmatch[0].rm_so

	int nmatch=8,stat,size,count;
	regmatch_t pmatch[nmatch];
	unsigned char addr[3];                                           //judge node addr

	fd_set readfds,writefds;
	struct timeval wait;

	int i,j;
	unsigned char n[3];
	int num;

	int len_repeat=0;                                         //if len_repeat==1 back half of buf recover front half of buf
	time_t t;
//	int flag_sock;
//	flag_sock=fcntl(confd,F_GETFL,0);         
//	fcntl(confd,F_SETFL,flag_sock|O_NONBLOCK);
#ifdef DEBUG
	//printf("SO_RCVLOWAT : %f\n",s);
	printf("node_%d in send_cmd\n",nodeno);
#endif
	
	(flag_data==0)?(frametype=REQDATA_F):(frametype=REREQDATA_F);

//	FD_ZERO(&readfds);
//	FD_SET(confd,&readfds);
//	while(select(confd,&readfds,NULL,NULL,0)>0)
//	{
//		read(confd,buft,sizeof(buft));
//	}
	stat=1042;
	setsockopt(confd,SOL_SOCKET,SO_RCVLOWAT,&stat,sizeof(stat)); 

start:
	do	
	{
		switch(frametype)                                                  //copy transmit frame and define pattern
		{
		case REQDATA_F:
			{
			
#ifdef DEBUG 
				printf("node_%d in case REQDATA_F\n",nodeno);
#endif
				bzero(buft,sizeof(buft));
				usleep(500000);
				funafnf0tser(buft,nodeprivate[nodeno-1].node_addr,*seqs,(unsigned char)parm);           //parm is sensor number
				pattern=PATTERN_3;
				size=19;

				FD_ZERO(&writefds);
				FD_SET(confd,&writefds);
				select(confd+1,NULL,&writefds,NULL,NULL);

				if((count=write(confd,buft,size))<0)//!=sizeof(buft))           //transmit to this node
				{
#ifdef DEBUG
					printf("DWserver pthread_dealclient : write testframe faild\n");
#endif
					continue;
				}
				if(count==0)
					return OUTLINE;
#ifdef DEBUG
				printf("674 send to node_%d : \n",nodeno);
				int d;
				for(d=0;d<sizeof(buft);d++)
					printf("%02x",buft[d]);
				printf("\n");
#endif

				if(*seqs==255)
					(*seqs)=0;
				else
					(*seqs)++;
				
				//printf("send_cmd seqs : %02x\n",*seqs);

				for(j=0;j<32;j++)
				{
					repeat=0;
				//	int i;
					bzero(bufr,sizeof(bufr));

again_one:			FD_ZERO(&readfds);
					FD_SET(confd,&readfds);
					wait.tv_usec=500000;                  //187500
					count=select(confd+1,&readfds,NULL,NULL,&wait);
					if(count==0)
					{
						if(repeat==2)
							continue;
						else
							repeat++;
							goto again_one;
					}
#ifdef DEBUG
					printf("select out loop : %d\n",j);
					perror("select");
					printf("count : %d\n",count);
#endif
					if((count=read(confd,bufr,sizeof(bufr)))<0)                                //receive from this node 
					{

						nodeprivate[nodeno-1].rx+=count;
						pthread_cond_signal(&cond_shm);

#ifdef DEBUG
						perror("pthread_dealclient : read testframe faild ");
#endif
						//	if(repeat==2)
						//		return FAILD;	
						//	repeat++;
						//	frametype=REPEAT;
						continue;
					}
					else
					{
						nodeprivate[nodeno-1].rx+=count;
						pthread_cond_signal(&cond_shm);

						if(count==0)                
							return OUTLINE;
					}
#ifdef DEBUG
					printf("read %d byte\n",count);
#endif

					for(i=0;i<count;i++)          
					//for(i=0;i<sizeof(bufr);i++)          
					{
						//printf("%02x",bufr[i]);
						sprintf(buf+len+i*2,"%02x",bufr[i]);
					}
					//printf("\n");
#ifdef DEBUG
					printf("send_cmd receive : %s\n",buf);
#endif
					bzero(pmatch,sizeof(pmatch));	
					stat=searchser(nodeno,pattern,buf,pmatch,nmatch,*seqs,1);
					if(stat==1)
					{
						memcpy(addr,buf+pmatch[0].rm_so+4,2);
#ifdef DEBUG
						//printf("addr %02x\n",strtol(addr,NULL,16));
#endif
						if(rtuaddr!=strtol(addr,NULL,16))
						{
						//	printf("                           node_%d searchser : rtuaddr wrong\n",nodeno);
							self_printf_fd("searchser : rtuaddr wrong\n",nodeprivate[nodeno-1].fd_log);
							stat=-1;
						}
						else
						{
							memcpy(addr,buf+pmatch[0].rm_so+8,2);
							if(parm!=strtol(addr,NULL,16))
							{
							//	printf("                       node_%d searchser : sensor addr wrong\n",nodeno);
								self_printf_fd("searchser : sensor addr wrong\n",nodeprivate[nodeno-1].fd_log);
								stat=-1;
							}
						}
					}

					if(stat==0)
					{
#ifdef DEBUG
						printf("DWserver pthread_dealclient:search testframe faild\n");
#endif
						//	if(repeat==2)
						//		return FAILD;
						//	repeat++;
						//	frametype=REPEAT;
						//len=sizeof(bufr)*2;
						if(len_repeat==1)
						{
							strncpy(buf,buf+(strlen(buf)>>1),(strlen(buf)>>1));
							bzero(buf+(sizeof(buf)>>1),(sizeof(buf)>>1));
							len_repeat=0;
						}
						else
						{
							len_repeat++;
						}
						len=strlen(buf);

						continue;
					}
				
					if(stat==-1)
					{					
						len=strlen(buf)-pmatch[0].rm_eo;
						strcpy(buf,buf+pmatch[0].rm_eo);
						bzero(buf+strlen(buf),sizeof(buf)-strlen(buf));
						continue;
					}
					//if(j==0 && *(bufr+pmatch[2].rm_so/2-1)==0x89)                                   //is deny frame
					if(j==0 && strncmp(buf+pmatch[2].rm_so-2,"89",2)==0)                                   //is deny frame
					{
					//	sprintf(*bufrecord,"N%02x",*(bufr+pmatch[4].rm_so));
						sprintf(*bufrecord,"N");
						strncat(*bufrecord,buf+pmatch[4].rm_so,2);
						goto over;
					}

					//unsigned int num=*(bufr+(pmatch[3].rm_so>>1));

					memcpy(n,buf+pmatch[3].rm_so,2);
					num=strtol(n,NULL,16);

					if(num>0&&num<33)
					{
#ifdef DEBUG
						printf("871 bufrecord num : %d\n",num);
						//printf("rm_so : %d\n",pmatch[4].rm_so);
#endif
						//memcpy(*(bufrecord+num-1),bufr+(pmatch[4].rm_so>>1),1024);
						for(i=0;i<1025;i++)
						{
							memcpy(n,buf+pmatch[4].rm_so+(i<<1),2);
						/*	int w;*/
							*(*(bufrecord+num-1)+i)=(unsigned char)strtol(n,NULL,16);
							
						}
						//memcpy(*(bufrecord+num-1),buf+pmatch[4].rm_so,1024);
						flag_data|=(1<<(num-1));
					}

					len=strlen(buf)-pmatch[0].rm_eo;
					strcpy(buf,buf+pmatch[0].rm_eo);
					bzero(buf+strlen(buf),sizeof(buf)-strlen(buf));

				}

				while((count=read(confd,bufr,sizeof(bufr)))>0)                                                                 //go on storing data in arrey buf     
				{

					nodeprivate[nodeno-1].rx+=count;
					pthread_cond_signal(&cond_shm);

#ifdef DEBUG
					printf("read %d byte\n",count);
#endif

					for(i=0;i<count;i++)          
					{
						sprintf(buf+len+i*2,"%02x",bufr[i]);
					}
#ifdef DEBUG
					printf("send_cmd receive : %s\n",buf);
#endif


					bzero(pmatch,sizeof(pmatch));
					stat=searchser(nodeno,pattern,buf,pmatch,nmatch,*seqs,1);
					if(stat==1)
					{
						memcpy(addr,buf+pmatch[0].rm_so+4,2);
#ifdef DEBUG
						//printf("addr %02x\n",strtol(addr,NULL,16));
#endif
						if(rtuaddr!=strtol(addr,NULL,16))
						{
						//	printf("                           node_%d searchser : rtuaddr wrong\n",nodeno);
							self_printf_fd("searchser : rtuaddr wrong\n",nodeprivate[nodeno-1].fd_log);
							stat=-1;
						}
						else
						{
							memcpy(addr,buf+pmatch[0].rm_so+8,2);
							if(parm!=strtol(addr,NULL,16))
							{
								//printf("                       node_%d searchser : sensor addr wrong\n",nodeno);
								printf("searchser : sensor addr wrong\n",nodeprivate[nodeno-1].fd_log);
								stat=-1;
							}
						}
					}
					if(stat==0)
					{
						if(len_repeat==1)
						{
							strncpy(buf,buf+(strlen(buf)>>1),(strlen(buf)>>1));
							bzero(buf+(sizeof(buf)>>1),(sizeof(buf)>>1));
							len_repeat=0;
						}
						else
						{
							len_repeat++;
						}
						len=strlen(buf);

						continue;
					//	break;
					}
					
					
					if(stat==-1)
					{	len=strlen(buf)-pmatch[0].rm_eo;
						strcpy(buf,buf+pmatch[0].rm_eo);
						bzero(buf+strlen(buf),sizeof(buf)-strlen(buf));

						continue;
					}
				
					memcpy(n,buf+pmatch[3].rm_so,2);
					num=strtol(n,NULL,16);

					if(num>0&&num<33)
					{
						for(i=0;i<1025;i++)
						{
							memcpy(n,buf+pmatch[4].rm_so+(i<<1),2);
							*(*(bufrecord+num-1)+i)=(unsigned char)strtol(n,NULL,16);
						}

						//	unsigned int num=*(bufr+(pmatch[3].rm_so>>1));
						//	memcpy(*(bufrecord+num-1),bufr+(pmatch[4].rm_so>>1),1024);
						flag_data|=(1<<(num-1));
					}

					len=strlen(buf)-pmatch[0].rm_eo;
					strcpy(buf,buf+pmatch[0].rm_eo);
					bzero(buf+strlen(buf),sizeof(buf)-strlen(buf));

				}
				
			/*	self_printf("\0");
				printf("node_%d sensor_%d interval_%d REQDATA %x\n",nodeno,parm,timeintno,flag_data);
				fflush(stdout);*/

				sprintf(string,"node_%d sensor_%d interval_%d REQDATA %x\n",nodeno,parm,timeintno,flag_data);
				self_printf_fd(string,nodeprivate[nodeno-1].fd_log);
			
				//flag_data = 0;                          //高频率测试节点用
				if(flag_data!=0xffffffff)
					frametype=REREQDATA_F;
				else
					goto over;
				
				break;
			}
		case REREQDATA_F:
			{
				len=0;
				len_repeat=0;
				pattern=PATTERN_4;
				size=20;
				bzero(buf,sizeof(buf));
				for(j=0;j<32;j++)
				{
					repeat=0;
					if((flag_data&(1<<j))!=0)
						continue;
					else
					{ 
						if(*seqs==0)
							*seqs=255;
						else
							(*seqs)--;

						bzero(buft,sizeof(buft));
						funafnf1tser(buft,nodeprivate[nodeno-1].node_addr,*seqs,j+1);//,(unsigned char)parm);               //parm is sensorno          //XX //parm is which sub frame
						
						FD_ZERO(&writefds);
						FD_SET(confd,&writefds);
						select(confd+1,NULL,&writefds,NULL,NULL);

						if((count=write(confd,buft,size))<0)//!=sizeof(buft))           //transmit to this node
						{
							nodeprivate[nodeno-1].tx+=count; 
							pthread_cond_signal(&cond_shm);
							j--;
							if(*seqs==255)
								*seqs=0;
							else
								(*seqs)++;
							continue;
						}
						else
						{
							nodeprivate[nodeno-1].tx=count; 
							pthread_cond_signal(&cond_shm);

							if(count==0)
								return OUTLINE;
						}		
					
						if(*seqs==255)
							(*seqs)=0;
						else
							(*seqs)++;

						bzero(bufr,sizeof(bufr));

again_two:			//	if(repeat==5)
					//		return FAILD;
						
						FD_ZERO(&readfds);
						FD_SET(confd,&readfds);
						wait.tv_usec=500000;                  //187500
						count=select(confd+1,&readfds,NULL,NULL,&wait);
#ifdef DEBUG 
						printf("again_two again_two %d\n",count);
#endif
						if(count==0)
						{
							if(repeat==2)
								continue;
							else
							{
								repeat++;
								goto again_two;
							}
						}
						if((count=read(confd,bufr,sizeof(bufr)))<0)                                //receive from this node 
						{
							
							nodeprivate[nodeno-1].rx+=count;
							pthread_cond_signal(&cond_shm);

#ifdef DEBUG
							perror("845 ");
#endif
							//	if(repeat==2)
							//		return FAILD;	
							//	repeat++;
							//	frametype=REPEAT;
							continue;
						}
						else
						{
							nodeprivate[nodeno-1].rx+=count;
							pthread_cond_signal(&cond_shm);

							if(count==0)
								return OUTLINE;
						}
#ifdef DEBUG
						printf("1065 read %d byte\n",count);
#endif
						//for(i=0;i<sizeof(bufr);i++)          
						for(i=0;i<count;i++)          
						{
							sprintf(buf+len+i*2,"%02x",bufr[i]);
						}
#ifdef DEBUG
						printf("1111 REREQDATA_F receive : %s\n",buf);
#endif
						
						bzero(pmatch,sizeof(pmatch));
						stat=searchser(nodeno,pattern,buf,pmatch,nmatch,*seqs,1);
						if(stat==1)
						{
							memcpy(addr,buf+pmatch[0].rm_so+4,2);
#ifdef DEBUG
							//printf("addr %02x\n",strtol(addr,NULL,16));
#endif
							if(rtuaddr!=strtol(addr,NULL,16))
							{
							//	printf("                           node_%d searchser : rtuaddr wrong\n",nodeno);
								self_printf_fd("searchser : rtuaddr wrong\n",nodeprivate[nodeno-1].fd_log);
								stat=-1;
							}
							else
							{
//								memcpy(addr,buf+pmatch[0].rm_so+6,2);
//								if(nodeprivate[nodeno-1].node_addr!=strtol(addr,NULL,16))
//								{
//#ifdef DEBUG
//									printf("node_addr wrong\n");
//#endif
//									stat=-1;
//								}
								memcpy(addr,buf+pmatch[0].rm_so+8,2);
								if(parm!=strtol(addr,NULL,16))
								{
									//printf("                       node_%d searchser : sensor addr wrong\n",nodeno);
									self_printf_fd("searchser : sensor addr wrong\n",nodeprivate[nodeno-1].fd_log);
									stat=-1;

								}
							}
						}
						if(stat==0)
						{
#ifdef DEBUG
							printf("894 : search testframe faild\n");
#endif		
							if(len_repeat==1)
							{
								strncpy(buf,buf+(strlen(buf)>>1),(strlen(buf)>>1));
								bzero(buf+(sizeof(buf)>>1),(sizeof(buf)>>1));
								len_repeat=0;
							}
							else
							{
								len_repeat++;
							}
							len=strlen(buf);
							continue;
						}
					//	if(stat==-1)
					//	{
					//		printf("905 : search testframe faild\n");
					//		//	if(repeat==2)
					//		//		return FAILD;
					//		//	repeat++;
					//		//	frametype=REPEAT;
					//		bzero(buf,sizeof(buf));
					//		continue;
					//	}
	
					//	len=strlen(buf)-pmatch[0].rm_eo;
					//	strcpy(buf,buf+pmatch[0].rm_eo);
					//	bzero(buf+strlen(buf),sizeof(buf)-strlen(buf));

						if(stat==-1)
						{
							len=strlen(buf)-pmatch[0].rm_eo;
							strcpy(buf,buf+pmatch[0].rm_eo);
							bzero(buf+strlen(buf),sizeof(buf)-strlen(buf));
							continue;
						}

					/*	if(j==0 && strncmp(buf+pmatch[2].rm_so-2,"89",2)==0)                                   //is deny frame
						{
							sprintf(*bufrecord,"N");
							strncat(*bufrecord,buf+pmatch[4].rm_so,2);
							goto over;
						}
					*/
						memcpy(n,buf+pmatch[3].rm_so,2);
						num=strtol(n,NULL,16);
						
						if(num>0&&num<33)
						{
							for(i=0;i<1025;i++)
							{
								memcpy(n,buf+pmatch[4].rm_so+(i<<1),2);
								*(*(bufrecord+num-1)+i)=(unsigned char )strtol(n,NULL,16);
							}

							flag_data|=(1<<(num-1));
						}

						len=strlen(buf)-pmatch[0].rm_eo;
						strcpy(buf,buf+pmatch[0].rm_eo);
						bzero(buf+strlen(buf),sizeof(buf)-strlen(buf));
					}
				
				}

				while((count=read(confd,bufr,sizeof(bufr)))>0)                                                                 //go on storing data in arrey buf     
				{

					nodeprivate[nodeno-1].rx+=count;
					pthread_cond_signal(&cond_shm);

#ifdef DEBUG
					printf("read %d byte\n",count);
#endif

					for(i=0;i<count;i++)          
					{
						sprintf(buf+len+i*2,"%02x",bufr[i]);
					}
#ifdef DEBUG
					printf("send_cmd receive : %s\n",buf);
#endif

							
					bzero(pmatch,sizeof(pmatch));
					stat=searchser(nodeno,pattern,buf,pmatch,nmatch,*seqs,1);

					if(stat==1)
					{
						memcpy(addr,buf+pmatch[0].rm_so+4,2);
#ifdef DEBUG
						//printf("addr %02x\n",strtol(addr,NULL,16));
#endif
						if(rtuaddr!=strtol(addr,NULL,16))
						{
						//	printf("                           node_%d searchser : rtuaddr wrong\n",nodeno);
						    self_printf_fd("searchser : rtuaddr wrong\n",nodeprivate[nodeno-1].fd_log);
							stat=-1;
						}
						else
						{
							memcpy(addr,buf+pmatch[0].rm_so+8,2);
							if(parm!=strtol(addr,NULL,16))
							{
								//printf("                       node_%d searchser : sensor addr wrong\n",nodeno);
								self_printf_fd("searchser : sensor addr wrong\n",nodeprivate[nodeno-1].fd_log);
								stat=-1;
							}
						}
					}

					if(stat==0)
					{
						if(len_repeat==1)
						{
							strncpy(buf,buf+(strlen(buf)>>1),(strlen(buf)>>1));
							bzero(buf+(sizeof(buf)>>1),(sizeof(buf)>>1));
							len_repeat=0;
						}
						else
						{
							len_repeat++;
						}
						len=strlen(buf);

						continue;
					//	break;
					}
					
					if(stat==-1)
					{
						len=strlen(buf)-pmatch[0].rm_eo;
						strcpy(buf,buf+pmatch[0].rm_eo);
						bzero(buf+strlen(buf),sizeof(buf)-strlen(buf));

						continue;
					}

					memcpy(n,buf+pmatch[3].rm_so,2);
					num=strtol(n,NULL,16);

					if(num>0&&num<33)
					{
						for(i=0;i<1025;i++)
						{
							memcpy(n,buf+pmatch[4].rm_so+(i<<1),2);
							*(*(bufrecord+num-1)+i)=(unsigned char)strtol(n,NULL,16);

						}

						//	unsigned int num=*(bufr+(pmatch[3].rm_so>>1));
						//	memcpy(*(bufrecord+num-1),bufr+(pmatch[4].rm_so>>1),1024);
						flag_data|=(1<<(num-1));
					}

					len=strlen(buf)-pmatch[0].rm_eo;
					strcpy(buf,buf+pmatch[0].rm_eo);
					bzero(buf+strlen(buf),sizeof(buf)-strlen(buf));
				}

				bzero(string,sizeof(string));
				sprintf(string,"node_%d sensor_%d interval_%d REREQDATA %x\n",nodeno,parm,timeintno,flag_data);
				self_printf_fd(string,nodeprivate[nodeno-1].fd_log);
			
				//flag_data=0;                                             //高频率测试节点用
				if(flag_data!=0xffffffff)
				{
					//nodeprivate[nodeno-1].flag_sensor[timeintno][parm]=flag_data;
					return FAILD;
				}
				else
					goto over;
			}
		}
	}while(1);
over:
	//nodeprivate[nodeno-1].flag_sensor[timeintno][parm]=flag_data;
//	pthread_mutex_lock(&mutex_storetail);                            //can make it used a pthread
//	time_t t;
//	time(&t);
//	store_InsertLinklist_tail(store_tail,nodeno,(unsigned char)parm,timeintno,t,bufrecord);
//	pthread_mutex_unlock(&mutex_storetail);
	pthread_mutex_lock(&mutex_storehead);                            //can make it used a pthread
	time(&t);
	store_InsertLinklist_tail(store_head,nodeno,(unsigned char)parm,timeintno,t,bufrecord);
	pthread_mutex_unlock(&mutex_storehead);
	pthread_cond_signal(&cond_storehead);
#ifdef DEBUG
	printf("send cond_signal to store\n");
#endif
	return SUCCESS;
}

/*****************************************************************************
 *	pthread beep 
 *
 ****************************************************************************/
void *pthread_beep(void *arg)
{
	pthread_detach(pthread_self());
	if(arg!=NULL)
	{
		beep_ctrl("beep",ON);
		usleep(50000);
		beep_ctrl("beep",OFF);
		usleep(5000);
		beep_ctrl("beep",ON);
		usleep(50000);
		beep_ctrl("beep",OFF);
	}
	else
	{
		beep_ctrl("beep",ON);
		usleep(100000);
		beep_ctrl("beep",OFF);
	}

	return;
}


/******************************************
*	pthread deal linked client 
*******************************************/
typedef struct whichnode
{
	pthread_t tid;
	unsigned char connectfd;
	unsigned char node_addr;
	unsigned char ip[16];
	unsigned int port;
} whichnode_s;



void *pthread_dealclient(void *arg)
{
	pthread_detach(pthread_self());
	//unsigned char bufframe[256];
	unsigned char buft[32]={};
	//unsigned char bufr[1042];
	unsigned char bufr[64]={};											//太小导致不能将wifi模块套接子中缓冲区到数据全读出清空，一直在连说明wifi模块并未断开链接？为啥不是过3分钟后在重新连？
	unsigned char buf[128]={};
	unsigned char nodeno=0;           /**/
	/*unsigned char nodeno=*(unsigned char*)arg; */                    //get to know which node thread it is
	int fd_log;

	whichnode_s whichnode=*(whichnode_s*)arg;   /**/
	
	whichnode.tid=pthread_self();
	free(arg);
	arg=NULL;
	unsigned char string[360];
    /* 
	sprintf(string,"node %d login\n                      IP : %s\n                      PORT : %d\n",nodeno,nodeprivate[nodeno-1].ip,nodeprivate[nodeno-1].port);
	self_printf(string);
    */	
	/*time(&(nodeprivate[nodeno-1].t_login));   */

//	freopen(whichnode.ip,"a+",stdout);
//	freopen(whichnode.ip,"a+",stderr);

	bzero(string,sizeof(string));
	sprintf(string,"/rturun/%s",whichnode.ip);
	fd_log=open(string,O_RDWR|O_CREAT|O_APPEND,0666);
	if(fd_log<0)
	{
		if(fd_log=open(string,O_RDWR|O_CREAT|O_APPEND,0666)<0)
			self_perror(string);
	}

	bzero(string,sizeof(string));
	sprintf(string,"One node login(%s)\n",whichnode.ip);
	self_printf_fd(string,fd_log);            //"One node login\n");

	time_t t_login,t,t_node,t_init;                 /**/
	time(&t_login);

//	struct timeval tv;
//	tv.tv_sec=10;
//	tv.tv_usec=0;
//	setsockopt(nodeprivate[nodeno].connectfd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));

	/* unsigned int confd=nodeprivate[nodeno-1].connectfd;  */

    /**/
	unsigned int confd=whichnode.connectfd;    
	long rx;
	long tx;
	/**/

	unsigned char *pattern;

	int count,i;
	int repeat=0;
	unsigned char workmode;
	unsigned short keepruntime;               //length of run time 
	unsigned short keepsleeptime;             //length of sleep time
	unsigned char frametype;
	unsigned char frametype_bak;
	unsigned char data[4];                    //store data from linklist
	
	unsigned char switch_timeint=0;           //time interval information of node   order 3 2 1
	int timeint[3]={};
	
	unsigned char seqs=0;                     //seq   
	unsigned char flag_log=0;                   //symbol whether the node is log in first time or not

	unsigned char flag_sensor;                //record the sensor which push data faild   0--faild
	int flag_sock;                            //used for getsockopt
	int keepalive=1;
	int keepintval;
	int keepcnt;
	int keepidle;

	flag_sock=fcntl(confd,F_GETFL,0);         
	fcntl(confd,F_SETFL,flag_sock|O_NONBLOCK);

	struct timeval wait;                      //used for socket delay time
	wait.tv_usec=0;
	fd_set readfds,writefds;
//	FD_ZERO(&readfds);
//	FD_ZERO(&writefds);
//	FD_SET(confd,&readfds);
//	FD_SET(confd,&writefds);

	int curtime;
	int sectime;

	int nmatch=8,stat,size;
	regmatch_t pmatch[nmatch];

	//unsigned char testbuf[64];
	unsigned char testframe[]={0x68,0xff,0xff,0xff,0xff,0x68,0x42,0,0x01,0x00,0x01,0x00,0x00,0x00,0x01,0x06,0xe4,0xba};
start:
	frametype=TEST_F;
	frametype_bak==TEST_F;
	seqs=0;
	//sleep(1);
	do	
	{
		switch(frametype)                                           //copy transmit frame and define pattern
		{
			case TEST_F:
				bzero(buft,sizeof(buft));
				memcpy(buft,testframe,sizeof(testframe));
				pattern=PATTERN_1;
				size=18;
				break;
			case TIME_F:
				bzero(buft,sizeof(buft));
				funafn04tser(buft,nodeno,seqs);
				pattern=PATTERN_2;
				size=25;
				break;
//			case REQDATA_F:
//				bzero(buft,sizeof(buft));
//				funafnf0tser(buft,nodeno,seqs,sensor);
//				pattern=PATTERN_3;
//				break;
//			case REREQDATA_F:
//				bzero(buft,sizeof(buft));
//				funafnf1tser(buft,nodeno,seqs,whichframe);
//				pattern=PATTERN_4;
//				break;
			case REPEAT:
				if(seqs==0)
					seqs=255;
				else
					seqs--;
				break;
//			case RECV:break;
		}
		
		FD_ZERO(&writefds);
		FD_SET(confd,&writefds);
		wait.tv_sec=1;
		count=select(confd+1,NULL,&writefds,NULL,&wait);
		if(!(count>0))
			continue;
		//if((count=write(confd,buft,sizeof(buft)))!=sizeof(buft))           //transmit to this node
		if((count=write(confd,buft,size))!=size)            //transmit to this node
		{
			if(count=0)
#ifdef DEBUG
				printf("1038 connection down\n");
#endif
			goto con_dw; 

			if(flag_log!=0)             /**/
			{
			nodeprivate[nodeno-1].tx+=count;
			pthread_cond_signal(&cond_shm);
			}
			else
				tx+=count;
#ifdef DEBUG
			printf("1017 pthread_dealclient : write testframe faild\n");
#endif
			continue;
		}
	
		if(flag_log!=0)             /**/
		{
		nodeprivate[nodeno-1].tx+=count;
		pthread_cond_signal(&cond_shm);
		}
		else
			tx+=count;


		if(seqs==255)
			seqs=0;
		else
			seqs++;

		
	//	self_printf("\0");
	//	printf("send to node %d(%s) : ",nodeno,whichnode.ip);
		bzero(string,sizeof(string));
		sprintf(string,"send to node %d(%s) : ",nodeno,whichnode.ip);
		for(i=0;i<count;i++)
			sprintf(string+strlen(string),"%02x",buft[i]);
		sprintf(string+strlen(string),"\n");
		self_printf_fd(string,fd_log);
	//	fflush(stdout);


		FD_ZERO(&readfds);
		FD_SET(confd,&readfds);
		wait.tv_sec=4;    //tv
		stat=select(confd+1,&readfds,NULL,NULL,&wait);//NULL);   //tv
#ifdef DEBUG
		printf("node_%d select out\n",nodeno);
#endif
		if(stat==0)                                  //receive from this node 
		{
#ifdef DEBUG
			perror("1039 pthread_dealclient : select testframe faild ");
#endif
			if(repeat==2)
			{
				bzero(string,sizeof(string));
				if(frametype_bak==TEST_F)
					sprintf(string,"node %d(%s) overtime(1) of link ",nodeno,whichnode.ip);
				else
					sprintf(string,"node %d(%s) overtime(1) of revise ",nodeno,whichnode.ip);
				self_perror_fd(string,fd_log);
				break;//goto outline;
			}
			repeat++;
			frametype_bak=frametype;
			frametype=REPEAT;
			continue;
		}
		if(stat<0)                                  //receive from this node 
		{
			if(repeat==2)
			{
				bzero(string,sizeof(string));
				if(frametype_bak==TEST_F)
					sprintf(string,"node %d(%s) overtime(2) of link ",nodeno,whichnode.ip);
				else
					sprintf(string,"node %d(%s) overtime(2) of revise ",nodeno,whichnode.ip);
				self_perror_fd(string,fd_log);
				break;//goto outline;
			}
			repeat++;
			frametype_bak=frametype;
			frametype=REPEAT;
			continue;
		}	
		
		stat=read(confd,bufr,sizeof(bufr));

#ifdef DEBUG
		printf("node_%d receive byte %d\n",nodeno,stat);
#endif
		if(stat==0)
		{
			bzero(string,sizeof(string));
			sprintf(string,"node %d(%s) out of line(1)",nodeno,whichnode.ip);
			self_perror_fd(string,fd_log);
			break; 
		}
		if(stat<0)                                  //receive from this node 
		{
#ifdef DEBUG
			perror("1059 pthread_dealclient : read testframe faild ");
#endif
			if(repeat==2)
			{
				bzero(string,sizeof(string));
				if(frametype_bak==TEST_F)
					sprintf(string,"node %d(%s) overtime(3) of link ",nodeno,whichnode.ip);
				else
					sprintf(string,"node %d(%s) overtime(3) of revise ",nodeno,whichnode.ip);
				self_perror_fd(string,fd_log);
				break;//goto outline;
			}
			repeat++;
			frametype_bak=frametype;
			frametype=REPEAT;
			continue;
		}

		if(flag_log!=0)            /**/
		{
		nodeprivate[nodeno-1].rx+=stat;
		pthread_cond_signal(&cond_shm);
		}
		else
			rx+=stat;

		bzero(buf,sizeof(buf));
		//for(i=0;i<sizeof(bufr);i++)             
		for(i=0;i<stat;i++)             
		{
			//printf("%02x",bufr[i]);
			sprintf(buf+i*2,"%02x",bufr[i]);
		}
		//printf("\n");
#ifdef DEBUG
		printf("receive from node_%d : %s\n",nodeno,buf);
		//printf("seqs %02x\n",seqs);
#endif		
		
		//printf("<<NODE>>\n");
		//self_printf("\0");
		bzero(string,sizeof(string));
		sprintf(string,"receive from node %d(%s) : %s\n",nodeno,whichnode.ip,buf);
		self_printf_fd(string,fd_log);

		stat=searchser(nodeno,pattern,buf,pmatch,nmatch,seqs,flag_log);
		if(stat==0)
		{
#ifdef DEBUG
			printf("1083 pthread_dealclient : search testframe faild\n");
#endif
			if(repeat==2)
			{
				bzero(string,sizeof(string));
				if(frametype_bak==TEST_F)
					sprintf(string,"node %d(%s) search link \n ",nodeno,whichnode.ip);
				else
					sprintf(string,"node %d(%s) search revise \n ",nodeno,whichnode.ip);
				self_printf(string);
				break;///goto next;
			}
			repeat++;
			frametype_bak=frametype;
			frametype=REPEAT;
			continue;
		}
		if(stat==-1)
		{
#ifdef DEBUG
			printf("1094 pthread_dealclient : search testframe faild\n");
#endif
			if(repeat==2)
			{
				bzero(string,sizeof(string));
				if(frametype_bak==TEST_F)
					sprintf(string,"node %d(%s) reaply of link Wrong\n",nodeno,whichnode.ip);
				else
					sprintf(string,"node %d(%s) reaply of revise Wrong\n",nodeno,whichnode.ip);
				self_printf_fd(string,fd_log);
				break;///goto next;
			}
			repeat++;
			frametype_bak=frametype;
			frametype=REPEAT;
			bzero(buf,sizeof(buf));
			continue;
		}
		if(frametype==REPEAT)
			frametype=frametype_bak;
		switch(frametype)                             //frame analysis
		{
		case TEST_F:
			{
				afnbroadtser_s afnbroadtser;
			/**/
				if(flag_log==0)
				{
					nodeno=*(bufr+(pmatch[1].rm_so>>1)+2);
					usleep(500);
					if((flag_online&(1<<(nodeno-1)))!=0)
					{
						self_printf("node number exist already\n");
						goto con_dw;
					}
					flag_online|=(1<<(nodeno-1));
					memcpy(nodeprivate[nodeno-1].ip,whichnode.ip,sizeof(whichnode.ip));
					nodeprivate[nodeno-1].port=whichnode.port;
					nodeprivate[nodeno-1].connectfd=whichnode.connectfd;
					nodeprivate[nodeno-1].tid=whichnode.tid;

					bzero(string,sizeof(string));
					sprintf(string,"node %d login\n                      IP : %s\n                      PORT : %d\n",nodeno,nodeprivate[nodeno-1].ip,nodeprivate[nodeno-1].port);
					self_printf_fd(string,fd_log);

					flag_log=1;
					nodeprivate[nodeno-1].node_addr=*(bufr+pmatch[1].rm_so/2+2);
					nodeprivate[nodeno-1].t_login=t_login;
					nodeprivate[nodeno-1].rx=rx;
					nodeprivate[nodeno-1].tx=tx;
					nodeprivate[nodeno-1].fd_log=fd_log;
					pthread_cond_signal(&cond_shm);
					t_init=t_login;

					switch(nodeno)
					{
					case 1:
						signal(SIGRTMIN,handler_one);
						break;
					case 2:
						signal(SIGRTMIN+1,handler_two);
						break;
					case 3:
						signal(SIGRTMIN+2,handler_three);
						break;
					case 4:
						signal(SIGRTMIN+3,handler_four);
						break;
					}

				}
			/**/

				nodeprivate[nodeno-1].node_addr=*(bufr+pmatch[1].rm_so/2+2);
#ifdef DEBUG
				//printf("node_%d node_addr : %02x\n",nodeno,nodeprivate[nodeno-1].node_addr);
#endif
				funafnbroadcastser(&afnbroadtser,bufr+pmatch[3].rm_so/2);              //do with link test frame

				memcpy(&(nodeprivate[nodeno-1].afnbroadtser),&afnbroadtser,sizeof(afnbroadtser));

				pthread_cond_signal(&cond_shm);
			
				workmode=afnbroadtser.workmode;
				memcpy(&keepruntime,afnbroadtser.runtime,2);
				memcpy(&keepsleeptime,afnbroadtser.sleeptime,2);
				//printf("afnbroadtser.workmode : %02x\n",afnbroadtser.workmode);

				struct tm tim;
				time(&t);
		
			//	pthread_mutex_lock(&mutex_testtail);
			//	test_InsertLinklist_tail(test_tail,nodeno,t,&afnbroadtser);
			//	pthread_mutex_unlock(&mutex_testtail);
				pthread_mutex_lock(&mutex_testhead);
				test_InsertLinklist_tail(test_head,nodeno,t,&afnbroadtser);
				pthread_mutex_unlock(&mutex_testhead);

				pthread_cond_signal(&cond_testhead);

				tim.tm_year=ctoi_bcd(afnbroadtser.nodetime[0])*100+ctoi_bcd(afnbroadtser.nodetime[1])-1900;
				tim.tm_mon=ctoi_bcd(afnbroadtser.nodetime[2])-1;
				tim.tm_mday=ctoi_bcd(afnbroadtser.nodetime[3]);
				tim.tm_hour=ctoi_bcd(afnbroadtser.nodetime[4]);
				tim.tm_min=ctoi_bcd(afnbroadtser.nodetime[5]);
				tim.tm_sec=ctoi_bcd(afnbroadtser.nodetime[6]);
				/*time_t t_node;*/
				t_node=mktime(&tim);
				time(&t);
				//if(difftime(t_node,t)>=REVISE_TIME*60)                        //change node time correct
				(duration==0)?(duration=REVISE_TIME):(1);
				//printf("                      time difference :%lf\n",difftime(t_node,t));
				if((!(difftime(t_node,t) < duration*60))||(!(difftime(t,t_node) < duration*60)))                        //change node time correct
				{
					frametype=TIME_F;
					frametype_bak=TIME_F;
					self_printf_fd("goto change time ok\n",fd_log);
					continue;
					//goto start;
				}

				for(i=0;i<3;i++)
				{
					switch_timeint|=((afnbroadtser.timeset[i]>>7)<<i);
					timeint[i]=ctoi_bcd(afnbroadtser.timeset[i]&0x7f);
				}
				break;
			}
		case TIME_F:
			{
			
				if((*(buf+pmatch[4].rm_so)!='0') || (*(buf+pmatch[4].rm_so+1)!='0'))
				{
					if(repeat==2)
					{
						break;
						//return;
					}
					repeat++;
					//frametype_bak=frametype;
					frametype_bak=TIME_F;
					frametype=REPEAT;
					continue;
				}

				self_printf_fd("change time ok\n",fd_log);
				break;                 //surpose to do what
			}
		}


next:	if(workmode==POSITIVE)                                         //self_report
		{
#ifdef DEBUG
			printf("node_%d in POSITIVE\n",nodeno);
			printf("%02x  %d   %d   %d\n",switch_timeint,timeint[0],timeint[1],timeint[2]);
#endif
			for(i=0;i<3;i++)
			{
				time(&t);
				if((labs(timeint[i]-(t%(24*60*60))/3600-8)<1)&&((switch_timeint&(1<<i))!=0))
				{
					if(nodeprivate[nodeno-1].flag_timeint[i]==0x3f)
						break;

					frametype=REQDATA_F;
					int j;
					unsigned char flag_repeat=0;                        //record how many times that ask for each sensor that push data faild,max is 3 times
				
					nodeprivate[nodeno-1].busy=1;/**/
				//	stat=1042;
				//	setsockopt(confd,SOL_SOCKET,SO_RCVLOWAT,&s,sizeof(s)); 
					
					flag_sensor=nodeprivate[nodeno-1].flag_timeint[i];
					while(flag_sensor!=0x3f&&flag_repeat<3)
					{
						for(j=0;j<6;j++)
						{
							if((flag_sensor&(1<<j))!=0)
								continue;
							if(send_cmd(nodeno,frametype,&seqs,j+1,i+1)==SUCCESS)              //i--timeint 
							{
								flag_sensor|=(1<<j);
							}
						}
						flag_repeat++;
					}
					nodeprivate[nodeno-1].flag_timeint[i]=flag_sensor;
					nodeprivate[nodeno-1].busy=0;/**/
					flag_sensor=0;

					stat=1;
					setsockopt(confd,SOL_SOCKET,SO_RCVLOWAT,&stat,sizeof(stat)); 
					break;
				}	
			}
			
			curtime=(t%(24*60*60))/3600+8;
			sectime=(t%(24*60*60))%3600;                      //current second time     

			if(curtime<timeint[0])
			{
				nodeprivate[nodeno-1].flag_timeint[0]=0;
			}
			else if(curtime<timeint[1])
			{
				nodeprivate[nodeno-1].flag_timeint[1]=0;
			}
			else if(curtime<timeint[2])
			{
				nodeprivate[nodeno-1].flag_timeint[2]=0;
			}
			else //if(curtime>timeint[2])
			{
				nodeprivate[nodeno-1].flag_timeint[0]=0;
			}

			//wait.tv_sec=keepruntime*60;
#ifdef DEBUG
			printf("node%d sleep\n",nodeno);
#endif
kpal_P:	
			//time(&t);
			keepidle=60;//wait.tv_sec-(t-nodeprivate[nodeno-1].t_login);
			keepcnt=3;
			keepintval=5;
			setsockopt(confd,SOL_SOCKET,SO_KEEPALIVE,(void *)&keepalive,sizeof(keepalive));
			setsockopt(confd,SOL_TCP,TCP_KEEPIDLE,(void *)&keepidle,sizeof(keepidle));
			setsockopt(confd,SOL_TCP,TCP_KEEPINTVL,(void *)&keepintval,sizeof(keepintval));
			setsockopt(confd,SOL_TCP,TCP_KEEPCNT,(void *)&keepcnt,sizeof(keepcnt));

			//select(confd+1,NULL,NULL,NULL,&wait);
			FD_ZERO(&readfds);
			FD_SET(confd,&readfds);
			stat=select(confd+1,&readfds,NULL,NULL,NULL);
#ifdef DEBUG
			self_perror("1757 select ");
#endif
			if(stat==-1)
			/*if(stat==-1&&errno==EINTR)*/
			{
				if(errno==EINTR)
				{
				sleep(1);
				goto kpal_P;
				}

			}
			else
			{
			if((getpeername(confd,NULL,NULL)<0&&errno==ENOTCONN)||(read(confd,string,sizeof(string))==-1&&errno==ETIMEDOUT))
			{
				bzero(string,sizeof(string));
				sprintf(string,"node %d(%s) out of line(2)\n",nodeno,whichnode.ip);
				self_printf_fd(string,fd_log);
				goto con_dw;
			}
			else
				goto kpal_P;
			}
		}
		else //if(workmode==NAGETIVE)                                                      //called measure workmode;get frame from linklist
		{
		//	int curtime;
		//	int sectime;
			curtime=0;
			sectime=0;

			//while(1)
			for(;;)
			{
				repeat=0;
				switch_timeint=switch_timeint_loc;
				memcpy(timeint,timeint_loc,sizeof(timeint));

				for(i=0;i<3;i++)
				{
					time(&t);
					curtime=(t%(24*60*60))/3600+8;                          //current hour time

					if((labs(timeint[i]-curtime)<1)&&((switch_timeint&(1<<i))!=0))
					{
						//if(flag_done[nodeno-1]==1)
							//break;
						if(nodeprivate[nodeno-1].flag_timeint[i]==0x3f)
							break;
						
						frametype=REQDATA_F;
						int j;
						unsigned char flag_repeat=0;                        //record how many times that ask for each sensor that push data faild,max is 3 times 
						flag_sensor=nodeprivate[nodeno-1].flag_timeint[i];
						
						nodeprivate[nodeno-1].busy=1;/**/

					//	stat=1042;
					//	setsockopt(confd,SOL_SOCKET,SO_RCVLOWAT,&s,sizeof(s));                   //socket implex useing


						while((flag_sensor!=0x3f)&&(flag_repeat<3))
						{
							for(j=0;j<6;j++)
							{
#ifdef DEBUG
								printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<sensor loop in %d>>>>>>>\n",j);
#endif
								if((flag_sensor&(1<<j))!=0)
								{
#ifdef DEBUG
									printf("have sensor %d flag_sensor=%02x\n",j+1,flag_sensor);
#endif
									continue;
								}
								if((stat=send_cmd(nodeno,frametype,&seqs,j+1,i+1))==SUCCESS)
								{
									flag_sensor|=(1<<j);
#ifdef DEBUG
									printf("sensor : %d\n",j+1);
									printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<flag_sensor : %02x\n",flag_sensor);
#endif
								}
								else
								{
									if(stat==OUTLINE)
										goto outline;
								}
							}
							flag_repeat++;
						}
						//if(flag_sensor==0x3f)
							//flag_done[nodeno-1]==1;
							nodeprivate[nodeno-1].flag_timeint[i]=0x3f;
							nodeprivate[nodeno-1].busy=0;/**/
							flag_sensor=0;

							stat=1;
							setsockopt(confd,SOL_SOCKET,SO_RCVLOWAT,&stat,sizeof(stat));                   //socket implex useing

							break;                                    //then current's timeint[i] is now time interval 
					}	
				}
				
				time(&t);
				curtime=(t%(24*60*60))/3600+8;
				sectime=(t%(24*60*60))%3600;                      //current second time     

				if(curtime<timeint[0])
				{
					nodeprivate[nodeno-1].flag_timeint[0]=0;
					i=0;
				}
				else if(curtime<timeint[1])
				{
					nodeprivate[nodeno-1].flag_timeint[1]=0;
					i=1;
				}
				else if(curtime<timeint[2])
				{
					nodeprivate[nodeno-1].flag_timeint[2]=0;
					i=2;
				}
				else //if(curtime>timeint[2])
				{
					nodeprivate[nodeno-1].flag_timeint[0]=0;
					i=3;
				}
	/*			
				if(i==3)
				{
					wait.tv_sec=(24-curtime+timeint[0])*60*60-sectime;
#ifdef DEBUG
					printf("i==%d %d\n",i,(24-curtime+timeint[0])*60*59-sectime);
#endif
				}
				else
				{
					wait.tv_sec=(timeint[i]-curtime)*60*60-sectime;
#ifdef DEBUG
					printf("i=%d timeint[1]=%d curtime=%d %d\n",i,timeint[1],curtime,(timeint[i]-curtime)*60*59-sectime);
#endif
				}
#ifdef DEBUG
				printf("wait.tv_sec=%ld\n",wait.tv_sec);
#endif
		*/	
				struct timeval wait_bak,leftruntime;
				struct timeval wait_time;
				unsigned char flag_wait=0;                                   // 1--go to send testframe
				wait_time.tv_usec=0;
				wait_bak.tv_usec=0;

reselect:
				FD_ZERO(&readfds);
				FD_SET(confd,&readfds);
				//FD_ZERO(&writefds);
				//FD_SET(confd,&writefds);
				//fcntl(confd,F_SETFL,flag_sock|O_NONBLOCK);
				
							
				keepidle=60;;//leftruntime.tv_sec;
				keepcnt=3;
				keepintval=5;
				setsockopt(confd,SOL_SOCKET,SO_KEEPALIVE,(void *)&keepalive,sizeof(keepalive));
				setsockopt(confd,SOL_TCP,TCP_KEEPIDLE,(void *)&keepidle,sizeof(keepidle));
				setsockopt(confd,SOL_TCP,TCP_KEEPINTVL,(void *)&keepintval,sizeof(keepintval));
				setsockopt(confd,SOL_TCP,TCP_KEEPCNT,(void *)&keepcnt,sizeof(keepcnt));
			
				time(&t);
				curtime=(t%(24*60*60))/3600+8;/**/
				sectime=(t%(24*60*60))%3600;  /**/                  //current second time    
				
				//leftruntime.tv_sec=keepruntime*60-(t-nodeprivate[nodeno-1].t_login);
				leftruntime.tv_sec=keepruntime*60-(t-t_init);
				wait_bak.tv_sec=leftruntime.tv_sec+keepsleeptime*60;                        //the seconds to next keepruntime
		
				if(i==3)
				{
					wait.tv_sec=(24-curtime+timeint[0])*60*60-sectime;                      //the seconds to next time interval
				}
				else
				{
					wait.tv_sec=(timeint[i]-curtime)*60*60-sectime;
				}
		
				(time_repeat==0)?time_repeat=2:(1);
				if(leftruntime.tv_sec>time_repeat*60)
				{
					wait_time.tv_sec=time_repeat*60;
					flag_wait=1;                                                            // 1 -- goto start , send testframe
//					printf(" time REPEAT %d\n",time_repeat);
//					fflush(stdout);
				}
				else
				{
					if(leftruntime.tv_sec<0)     /**/
						wait_time.tv_sec=keepsleeptime*60;
					else                 /**/         
						wait_time.tv_sec=wait_bak.tv_sec;
				/*	{
					if(wait.tv_sec>wait_bak.tv_sec)                                                       //wait -- time before next interval  wait_bak -- time before node awake
					{
						wait_time.tv_sec=wait_bak.tv_sec;
						flag_wait=1;
					}
					else
						wait_time.tv_sec=wait.tv_sec;
					}*/
				}
				if(wait_time.tv_sec>wait.tv_sec)
					wait_time.tv_sec=wait.tv_sec;
				
				wait_time.tv_usec=0;
				
				//printf(string,"    		      wait time : %lds\n",wait_time.tv_sec);
				bzero(string,sizeof(string));
				sprintf(string,"wait time : %lds\n",wait_time.tv_sec);
				self_printf_fd(string,fd_log);
			
				//if(select(confd+1,&readfds,NULL,NULL,&wait)!=0)
				stat=select(confd+1,&readfds,NULL,NULL,&wait_time);
				if(stat==-1)
				{
					/*if((stat==-1)&&(errno==EINTR))*/
					if(errno==EINTR)
					{
						sleep(1);
						goto reselect;
					}
					//if((stat==-1)&&(errno==EINVAL))  /**/
					else
					{
						goto start;
					}
				}
				
				else
				{
				bzero(string,sizeof(string));
				sprintf(string,"node %d(%s) select out : %s",nodeno,whichnode.ip,strerror(errno));
				self_perror_fd(string,fd_log);
							
			//	if(FD_ISSET(confd,&readfds));
			//	{
					//if(stat!=0)
					if(stat>0)
					{
						if(getpeername(confd,NULL,NULL)<0&&errno==ENOTCONN)
						{
outline:				bzero(string,sizeof(string));
						sprintf(string,"node %d(%s) out of line(3)\n",nodeno,whichnode.ip);
						self_printf_fd(string,fd_log);
						goto con_dw;
						//break;

						}
						else
						{
							unsigned char what[1048];
							stat=read(confd,what,sizeof(what));
							if(stat==0)
							{
								nodeprivate[nodeno-1].rx+=stat;
								pthread_cond_signal(&cond_shm);

								//outline:			
								bzero(string,sizeof(string));
								sprintf(string,"node %d(%s) out of line(4)\n",nodeno,whichnode.ip);
								self_printf_fd(string,fd_log);
								goto con_dw;
								//break;
							}
							else
							{
								if(stat==-1&&errno==ETIMEDOUT)
									goto con_dw;
								if(stat>0)
								{
								nodeprivate[nodeno-1].rx+=stat;
								pthread_cond_signal(&cond_shm);
								int w;
								bzero(string,sizeof(string));
								sprintf(string,"receive from node %d(%s) : ",nodeno,whichnode.ip);
								//for(w=0;w<stat;w++)
								//for(w=0;w<(sizeof(string)-strlen(string));w++)
								for(w=0;w<32;w++)
								{
									sprintf(string+22+2*w,"%02x",what[w]);
								}
								sprintf(string+stat*2+22,"\n");
								self_printf_fd(string,fd_log);
								}
							}
						}
					}
					else                                  //select return 0
					{
						if(flag_wait!=1)
						{
						time(&t);
						t_init=t;
						}
						goto start;
						/*
						if(flag_wait==1)
						{
							goto start;
						}
						else
						{
						//	flag_done[nodeno-1]=0;
							
							write(confd,"h",1);
							bzero(string,sizeof(string));
							sprintf(string,"wakeup node %d(%s)\n",nodeno,whichnode.ip);
							self_printf(string);

							pthread_mutex_lock(&mutex_led);
							flag_led=3;
							pthread_mutex_unlock(&mutex_led);
							pthread_kill(id_led,SIGUSR2);
							pthread_create(&id_beep,0,&pthread_beep,string);
						}*/
					}
			//	}
				}
			}
		}
		/*break;*/
	}while(1);

con_dw:	
	
	time(&(nodeprivate[nodeno-1].t_logoff_bak));
	nodeprivate[nodeno-1].t_login_bak=nodeprivate[nodeno-1].t_login;
	nodeprivate[nodeno-1].t_login=0;
	nodeprivate[nodeno-1].t_logoff=0;
	nodeprivate[nodeno-1].rx=0;
	nodeprivate[nodeno-1].tx=0;

	//pthread_mutex_lock(pthread_mutex_lock(&mutex_online);mutex_online);
//	flag_online&=(~(1<<(nodeno-1)));
	//pthread_mutex_unlock(pthread_mutex_unlock(&mutex_online);mutex_online);

	//bzero(&nodeprivate[nodeno-1].afnbroadtser,sizeof(afnbroadtser_s));
	pthread_cond_signal(&cond_shm);


	pthread_mutex_lock(&mutex_led);
	flag_led=2;
	pthread_mutex_unlock(&mutex_led);
	//pthread_cond_signal(&cond_led);
	pthread_kill(id_led,SIGUSR2);
	pthread_create(&id_beep,0,&pthread_beep,NULL);
#ifdef DEBUG
	printf("1378 pthread_dealclient : exit\n");
#endif
	bzero(string,sizeof(string));
	sprintf(string,"node %d(%s) logoff\n",nodeno,whichnode.ip);
	self_printf_fd(string,fd_log);
	close(fd_log);
	close(nodeprivate[nodeno-1].fd_log);
	close(confd);
	flag_online&=(~(1<<(nodeno-1)));
	return ;
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
		if(speed<0)//==(-1))
		{
			led_ctrl("D9",OFF);
			break;
		}
		else
		{
			usleep(500000/(speed+1));
			led_ctrl("D9",OFF);
			usleep(500000/(speed+1));
			led_ctrl("D9",ON);
		}
	}
	return;
}

void *pthread_led(void *arg)
{
	signal(SIGUSR2,handler_led);
	led_ctrl("D9",ON);
	while(1)
	{
		usleep(1000000);
		led_ctrl("D9",OFF);
		usleep(1000000);
		led_ctrl("D9",ON);
	}
	led_ctrl("D9",OFF);
	return NULL;
}


/************************************
*  main 
************************************/
void releaseresouce(int signo)
{
#ifdef DEBUG
	printf("releaseresouce in\n");
#endif
	led_ctrl("D9",OFF);
	pthread_mutex_destroy(&mutex_storehead);
	pthread_mutex_destroy(&mutex_testhead);
	pthread_mutex_destroy(&mutex_storetail);
	pthread_mutex_destroy(&mutex_testtail);
	pthread_mutex_destroy(&mutex_shmbuf);
	pthread_mutex_destroy(&mutex_online);
	
	pthread_cond_destroy(&cond_testhead);
	pthread_cond_destroy(&cond_testtail);
	pthread_cond_destroy(&cond_led);
	pthread_cond_destroy(&cond_shm);
	pthread_cond_destroy(&cond_storehead);

	shmctl(shmid,IPC_RMID,NULL);
	msgctl(msgid,IPC_RMID,NULL);
//	semctl(semid,1,IPC_RMID,NULL);

	pthread_cancel(id_shm);
	pthread_cancel(id_parm);
	pthread_cancel(id_msg);
	pthread_cancel(id_restore32kb);
	pthread_cancel(id_led);
	//pthread_cancel(id_beep);
	pthread_cancel(id_test);
	
	led_ctrl("D9",OFF);
	close(fd_parm);
	close(fd_lednine);
	close(fd_beep);
#ifdef DEBUG
	printf("*******************************releaseresouce:all quit**********************************\n");
#endif

	return;
}

void handler_parm(int signo)
{
	pthread_create(&id_parm,0,&pthread_parm,NULL);
	sleep(2);
	pthread_cond_signal(&cond_shm);
	return;
}

int main(int argc, const char *argv[])
{
	signal(SIGTSTP,releaseresouce);
	//signal(SIGINT,releaseresouce);
	signal(SIGPIPE,SIG_IGN);
	signal(SIGINT,handler_parm);

#ifndef DISPLAY
	freopen(PATH_SERWR,"a+",stdout);
	freopen(PATH_SERWR,"a+",stderr);
#endif

	fd_lednine = open ("/sys/class/leds/d9/brightness", O_WRONLY);
	fd_beep = open ("/sys/class/leds/beep", O_WRONLY);

	int n=0;
	for(n=0;n<N_NODE;n++)
	{
		nodeprivate[n].cmd_head=cmd_CreateLinklist();
		nodeprivate[n].pass_head=pass_CreateLinklist();
        bzero(nodeprivate[n].flag_timeint,3);
	//	bzero(nodeprivate[n].flag_sensor,sizeof(nodeprivate[n].flag_sensor));
	}

	pthread_mutex_init(&mutex_storehead,NULL);
	pthread_mutex_init(&mutex_testhead,NULL);
	pthread_mutex_init(&mutex_storetail,NULL);
	pthread_mutex_init(&mutex_testtail,NULL);
	pthread_mutex_init(&mutex_led,NULL);
	pthread_mutex_init(&mutex_shmbuf,NULL);
	pthread_mutex_init(&mutex_online,NULL);
	
	pthread_cond_init(&cond_testhead,NULL);
	pthread_cond_init(&cond_storehead,NULL);
	pthread_cond_init(&cond_testtail,NULL);
	pthread_cond_init(&cond_led,NULL);
	pthread_cond_init(&cond_shm,NULL);

	fd_parm=open(PATH_PARM,O_RDWR|O_CREAT,0x666);
	sleep(1);
	pthread_create(&id_parm,0,&pthread_parm,NULL);
	sleep(2);
#ifdef DEBUG
	printf("*****************************Server  in****************************\n");
#endif

	self_printf("DWserver START WORKING\n");

	//pthread_create(&id_node,0,&pthread_node,NULL);
	pthread_create(&id_shm,0,&pthread_shm,NULL);
	pthread_create(&id_led,0,&pthread_led,NULL);
	pthread_create(&id_restore32kb,0,&pthread_restore32kb,NULL);
	pthread_create(&id_test,0,&pthread_testframe,NULL);
	pthread_create(&id_msg,0,&pthread_msg,NULL);

	atexit(releaseresouce);

	int sockfd,connectfd;
	struct sockaddr_in server_addr,client_addr;

	while((sockfd=socket(AF_INET,SOCK_STREAM,0))<0)
	{
		self_perror("create server socket ");
	}
	memset(&server_addr,0,sizeof(server_addr));
	memset(&client_addr,0,sizeof(client_addr));
	server_addr.sin_family=AF_INET;
	if(argc==3)                                              //./DWserver IP PORT
	{
		server_addr.sin_port=htons(atoi(argv[2]));
		server_addr.sin_addr.s_addr=inet_addr(argv[1]);
	}
	else
	{
		server_addr.sin_port=htons(atoi(serverPORT));
		server_addr.sin_addr.s_addr=inet_addr(serverIP);

		//server_addr.sin_port=htons(PORT);
		//server_addr.sin_addr.s_addr=inet_addr(IP);
	}
	
	int s=1;
	setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&s,sizeof(s));                   //socket implex useing
	
	while(bind(sockfd,(struct sockaddr *)&server_addr,sizeof(server_addr))<0)
	{
		self_perror("bind server socket ");
	}
	listen(sockfd,N_NODE);
	socklen_t addrlen=sizeof(client_addr);
	
	int i;
	while(1)
	{
		if((connectfd=accept(sockfd,(struct sockaddr*)&client_addr,&addrlen))<0)
		{
			self_perror("accept client socket ");
			continue;
		}
		unsigned char ip[16];
		unsigned int port=ntohs(client_addr.sin_port);
		sprintf(ip,"%s",inet_ntoa(client_addr.sin_addr));
//		for(i=0;i<N_NODE;i++)                             //judge whether regist already or not
//		{
//			if(strcmp(nodeprivate[i].ip,ip)==0)
//			{	
//				//memcpy(nodeprivate[i].ip,ip,sizeof(ip));
//				nodeprivate[i].port=port;
//				nodeprivate[i].connectfd=connectfd;
//
//				//pthread_mutex_lock(pthread_mutex_lock(&mutex_online);mutex_online);
//				flag_online|=(1<<i);
//				//pthread_mutex_unlock(pthread_mutex_unlock(&mutex_online);mutex_online);
//#ifdef DEBUG
//				printf("1589 online_node : %02x\n",flag_online);
//#endif
//				break;
//			}
//		}
//		if(i==N_NODE)
//		{
//			int j;
//			for(j=0;j<N_NODE;j++)
//			{
//				//pthread_mutex_lock(pthread_mutex_lock(&mutex_online);mutex_online);
//				if(!(flag_online&(1<<j)))                 //if is 1 , symbol is online        if outline , then inside if
//				{
//					//pthread_mutex_unlock(pthread_mutex_unlock(&mutex_online);mutex_online);
//
//					memcpy(nodeprivate[j].ip,ip,sizeof(ip));
//					nodeprivate[j].port=port;
//					nodeprivate[j].connectfd=connectfd;
//
//					//pthread_mutex_lock(pthread_mutex_lock(&mutex_online);mutex_online);
//					flag_online|=(1<<j);
//					//pthread_mutex_unlock(pthread_mutex_unlock(&mutex_online);mutex_online);
//
//					i=j;                                  //used for 1613
//					break;
//#ifdef DEBUG
//					printf("1609 online_node : %02x\n",flag_online);
//#endif
//				}	
//			}
//		}
		pthread_t tid;
		/* nodeprivate[i].tid=tid; */
#ifdef DEBUG
		printf("=====================ip=%s port=%d==========================\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
#endif
		/*
        unsigned char *p=(unsigned char*)malloc(sizeof(unsigned char));
        *p=i+1;
		*/
	
		/**/
		whichnode_s *p=(whichnode_s*)malloc(sizeof(whichnode_s));
		p->tid=tid;
		p->port=port;
		memcpy(p->ip,ip,sizeof(ip));
		p->connectfd=connectfd;
		/**/


		pthread_create(&tid,NULL,&pthread_dealclient,p);
#ifdef DEBUG
		printf("2347 %ld\n",tid);
		fflush(stdout);
#endif
		pthread_create(&id_beep,0,&pthread_beep,&id_beep);
		pthread_mutex_lock(&mutex_led);
		flag_led=1;
		pthread_mutex_unlock(&mutex_led);
		//pthread_cond_signal(&cond_led);
		pthread_kill(id_led,SIGUSR2);

		/* pthread_cond_signal(&cond_shm); */
	}

	pthread_join(id_led,NULL);
	//pthread_join(id_beep,NULL);
	pthread_join(id_shm,NULL);
	//pthread_join(id_parm,NULL);
	pthread_join(id_restore32kb,NULL);
	pthread_join(id_test,NULL);
	pthread_join(id_msg,NULL);

	close(sockfd);
	return 0;
}


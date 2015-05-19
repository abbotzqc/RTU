#include<signal.h>
#include<sys/time.h>
#include<stdio.h>
#include<string.h>
#include<sys/ipc.h>
#include<sys/msg.h>
#include<sys/sem.h>
#include<sys/shm.h>
#include<stdlib.h>
#include<sys/types.h>
#include<unistd.h>
#include<fcntl.h>
#include<errno.h>
#include<signal.h>
#include<pthread.h>
#include"sem.h"
#include<regex.h>
#include<sys/stat.h>


#define DISPLAY 1

#define N sizeof(shm_s)
#define N_NODE 4
#define N_OPT 13
#define PATH_PARM "/rturun/parm.log"
#define PATH_SHM  "/rturun/app"

#define PRINTALL              21
#define PRINT_RTU             22
#define PRINT_NODE_ONE        23
#define PRINT_NODE_TWO        24
#define PRINT_NODE_THR        25
#define PRINT_NODE_FOU        26

#define REC_RTUADDR            11
#define REC_RTUTYPE            12
#define REC_SEQL               13
#define REC_SWITCH             14
#define REC_TIMEINT1           15
#define REC_TIMEINT2           16
#define REC_TIMEINT3           17
#define REC_NODE               18
#define REC_SEN                19
#define REC_DUR                21
#define REC_SERIP              22
#define REC_SERPORT            23
#define REC_CLIIP              24
#define REC_CLIPORT            25
#define REC_OTS                26
#define REC_OTC                27
#define REC_REP				   28

unsigned short rtuaddr;
char rtutype;
unsigned char seq;
unsigned char switch_timeint_loc;
//unsigned char switch_timeint;                      //switch_timeint in parm.log 
int timeint_loc[3];
unsigned int n_node;
unsigned int n_sen;
unsigned int duration;
unsigned char serverIP[16];
unsigned char serverPORT[6];
unsigned char hostIP[16];
unsigned char hostPORT[6];
int overtime_server;
int overtime_client;
int time_repeat;

char optionname[N_OPT][96]={"(interval)(([1,2,3][0,1][0,1,2][0-9]){1,3})",                // number --1
					      "(serverIP)([0-2][0-9][0-9].[0-9]{1,3}.[0-9]{1,3}.[0-9]{1,3})",                // number --2
					      "(serverPORT)([0-9]{4,5})",              //3
					 	  "(hostIP)([0-2][0-9][0-9].[0-9]{1,3}.[0-9]{1,3}.[0-9]{1,3})",                //4
					      "(hostPORT)([0-9]{4,5})",              //5 
					      "(address)([0-9]{8})",                   //6
						  "(nodenumber)([0-9]{1,2})",         //7
						  "(sensornuber)([0-9]{1,2})",        //8
						  "(duration)([0-9]{1,2})",                 //9
						  "(rtutype)([0-9]{1,2})",                 //10
						  "(overtime_server)([0-9]{1,2})",          //11 
						  "(overtime_client)([0-9]{1,2})",           //12
						  "(time_repeat)([0-9]{1,2})"               //13
				         };

pthread_t id_shm,id_parm,id_key;
//int msgid;
//int semid;
int shmid;

pthread_mutex_t mutex_shmbuf,mutex_parm_loc,mutex_tab;
pthread_cond_t cond_tab;

typedef struct afnbroadtser		
{
	//unsigned char nodetype;
	unsigned char nodevolt[2];
	unsigned char nodecir[2];
	unsigned char nodetime[7];
	unsigned char workmode;
	unsigned char runtime[2];
	unsigned char sleeptime[2];
	unsigned char timeset[3];
	//unsigned char timeset_two;
	//unsigned char timeset_three;
	unsigned char nodeversion[2]; 
} afnbroadtser_s;

typedef struct disply_node
{
	unsigned char node_addr;
	unsigned char ip[16];
	unsigned int port;
	
	//unsigned char switch_timeint;
	//int timeint[3];

	time_t t_login;
	time_t t_logoff;
	time_t t_login_bak;
	time_t t_logoff_bak;

	unsigned long rx;
	unsigned long tx;

	afnbroadtser_s afnbroadtser;
} disply_node_s;

typedef struct shm
{
	//RTU
	unsigned short volt;
	unsigned short curr;
	int timeint[3];
	unsigned char switch_timeint;
	unsigned char overtime_server;
	unsigned char overtime_client;
	
	//NODE
	unsigned char flag_online;
	disply_node_s disply_node[4];
} shm_s;

shm_s disply_shm;

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

	while ((key_info = ftok (PATH_SHM, 'i')) < 0);
	
	//	exit (-1);
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
//			perror ("client_uphost pthread_shm : semget\n");
//		//	exit (-1);
//		}
//	}
//	else
//	{
//		init_sem (semid, 0, 2 );
//	}
#ifdef DEBUG
	printf("client_uphost pthread_shm : shm bellow\n");
#endif
	if ((shmid = shmget (key_info, sizeof(shm_s), IPC_CREAT | IPC_EXCL | 0666)) < 0)
	{
		if (errno == EEXIST)
		{
			shmid = shmget (key_info, N, 0666);
			pshmbuf = (shm_s*)shmat (shmid, NULL, 0);
			bzero (pshmbuf, sizeof(shm_s));
		}
		else
		{
			perror ("client_uphost pthread_shm : shmget\n");
		//	exit (-1);
		}
	}
	else
	{
		if ((pshmbuf = (shm_s*)shmat(shmid, NULL, 0)) == (void *)-1)
		{
			perror ("client_uphost pthread_shm : shmat\n");
		//	exit (-1);
		}
	}
	while(1)
	{
//#ifdef DEBUG
//		printf("client_uphost pthread_shm : wait cond_shm\n");
//#endif
	
		pthread_mutex_lock(&mutex_shmbuf);
		//sem_p(semid,0);                                       //locate here ok?
	
		//pthread_cond_wait(&cond_shm,&mutex_shmbuf);
//#ifdef DEBUG
//		printf("client_uphost pthread_shm : wait cond_shm awake\n");
//#endif
		
		memcpy(&disply_shm,pshmbuf,sizeof(shm_s));

#ifdef DEBUG
		printf("!!!!!!!!flag_online %d\n",pshmbuf->flag_online);
		printf("!!!!!!!!workmode %02x\n",pshmbuf->disply_node[0].afnbroadtser.workmode);
#endif

		//sem_v(semid,0);
		pthread_mutex_unlock(&mutex_shmbuf);
		//usleep(10000);
	}
	return;
}

/**************************************
 *
 *************************************/
void *pthread_key(void *arg)
{
	pthread_detach(pthread_self());
	int *flag_tab=(int*)arg; 
	unsigned char a;
	while(1)
	{
		a=getchar();	
		pthread_mutex_lock(&mutex_tab);
		if(*flag_tab==5)
			(*flag_tab)=1;
		else
			(*flag_tab)++;
		pthread_mutex_unlock(&mutex_tab);
		pthread_cond_signal(&cond_tab);

	}
	return;
}

/*****************************************
 *	thread read parmgrams from parm.log
 ****************************************/
void *pthread_parm(void *arg)
{
	pthread_detach(pthread_self());
	unsigned char flag=0;

	if(arg!=NULL)
	{
		free(arg);
		flag=1;
	}

	int fd,cnt;
	unsigned char buf[256];

	unsigned char bak[8]={};
//	while((fd=open(PATH_PARM,O_RDWR|O_CREAT,0x666))<0);
	if((fd=open(PATH_PARM,O_RDWR|O_CREAT,0x666))<0)
		if((fd=open(PATH_PARM,O_RDWR|O_CREAT,0x666))<0)
			return;
	cnt=read(fd,buf,sizeof(buf));

	pthread_mutex_lock(&mutex_parm_loc);

	bzero(bak,sizeof(bak));
	memcpy(bak,buf+45,2);
	switch_timeint_loc=strtol(bak,NULL,16);
	if(flag==1)
		return ;
	bzero(bak,sizeof(bak));

	memcpy(bak,buf+8,2);
	rtuaddr=strtol(bak,NULL,10);
	bzero(bak,sizeof(bak));

	memcpy(bak,buf+19,2);
	rtutype=strtol(bak,NULL,16);
	bzero(bak,sizeof(bak));

//	memcpy(bak,buf+27,2);
//	seql=strtol(bak,NULL,16);
//	bzero(bak,sizeof(bak));

	memcpy(bak,buf+59,2);
	timeint_loc[0]=strtol(bak,NULL,10);
	bzero(bak,sizeof(bak));

	memcpy(bak,buf+73,2);
	timeint_loc[1]=strtol(bak,NULL,10);
	bzero(bak,sizeof(bak));

	memcpy(bak,buf+87,2);
	timeint_loc[2]=strtol(bak,NULL,10);
	bzero(bak,sizeof(bak));

	memcpy(bak,buf+97,2);
	n_node=strtol(bak,NULL,10);
	bzero(bak,sizeof(bak));

	memcpy(bak,buf+106,2);
	n_sen=strtol(bak,NULL,10);
	bzero(bak,sizeof(bak));

	memcpy(bak,buf+118,2);
	duration=strtol(bak,NULL,10);
	bzero(bak,sizeof(bak));

	memcpy(serverIP,buf+130,15);
	memcpy(serverPORT,buf+157,5);
	memcpy(hostIP,buf+170,15);
	memcpy(hostPORT,buf+195,5);

	memcpy(bak,buf+217,2);
	overtime_server=strtol(bak,NULL,10);
	bzero(bak,sizeof(bak));
	memcpy(bak,buf+236,2);
	overtime_client=strtol(bak,NULL,10);
	bzero(bak,sizeof(bak));
	memcpy(bak,buf+251,2);
	time_repeat=strtol(bak,NULL,10);
	bzero(bak,sizeof(bak));


	pthread_mutex_unlock(&mutex_parm_loc);

	close(fd);
	return;
}

/*****************************
 * record parmgrams into parm.log
 * */
int rec_parm(int cmd)
{
	int fd;
	unsigned char bak[32]={};
//	while((fd=open(PATH_PARM,O_RDWR|O_CREAT,0x666))<0);
	if((fd=open(PATH_PARM,O_RDWR|O_CREAT,0x666))<0)
		if((fd=open(PATH_PARM,O_RDWR|O_CREAT,0x666))<0)
			return 0;

	switch(cmd)
	{
	case REC_RTUADDR:
		sprintf(bak,"%02d",rtuaddr);
		lseek(fd,8,SEEK_SET);
		break;
	case REC_RTUTYPE:
		sprintf(bak,"%02x",rtutype);
		lseek(fd,19,SEEK_SET);
		break;
//	case REC_SEQL:
//		sprintf(bak,"%02x",seql);
//		lseek(fd,27,SEEK_SET);
//		break;
	case REC_SWITCH:
		sprintf(bak,"%02x",switch_timeint_loc);
		lseek(fd,45,SEEK_SET);
		break;
	case REC_TIMEINT1:
		sprintf(bak,"%02d",timeint_loc[0]);
		lseek(fd,59,SEEK_SET);
		break;
	case REC_TIMEINT2:
		sprintf(bak,"%02d",timeint_loc[1]);
		lseek(fd,73,SEEK_SET);
		break;
	case REC_TIMEINT3:
		sprintf(bak,"%02d",timeint_loc[2]);
		lseek(fd,87,SEEK_SET);
		break;
	case REC_NODE:
		sprintf(bak,"%03d",n_node);
		lseek(fd,97,SEEK_SET);
		break;
	case REC_SEN:
		sprintf(bak,"%03d",n_sen);
		lseek(fd,106,SEEK_SET);
		break;
	case REC_DUR:
		sprintf(bak,"%04d",duration);
		lseek(fd,118,SEEK_SET);
		break;
	case REC_SERIP:
		sprintf(bak,"%-15s\n",serverIP);
		lseek(fd,130,SEEK_SET);
		break;
	case REC_SERPORT:
		sprintf(bak,"%-6s\n",serverPORT);
		lseek(fd,157,SEEK_SET);
		break;
	case REC_CLIIP:
		sprintf(bak,"%-15s\n",hostIP);
		lseek(fd,170,SEEK_SET);
		break;
	case REC_CLIPORT:
		sprintf(bak,"%-6s\n",hostPORT);
		lseek(fd,195,SEEK_SET);
		break;
	case REC_OTS:
		sprintf(bak,"%02d",overtime_server);
		lseek(fd,217,SEEK_SET);
		break;
	case REC_OTC:
		sprintf(bak,"%02d",overtime_client);
		lseek(fd,236,SEEK_SET);
		break;
	case REC_REP:
		sprintf(bak,"%02d",time_repeat);
		lseek(fd,251,SEEK_SET);
		break;

	}

	if(write(fd,bak,strlen(bak))==strlen(bak))
	{
		return 1;                                      //success
	}
	else
	{
		return 0;
	}
}

/**************************
 *	record the parmgrams according to the optionname
 *************************/

void analysis(int optn_no,char *cmd,regmatch_t *pmatch)
{
	int error=0;                         // 1--FAILD;
//	int fd;
//	if((fd=open(PATH,O_RDWR|O_CREAT,0x666))<0)
//		printf("open %s wrong",PATH);

	//printf("optn_no : %d\n",optn_no);

	switch(optn_no)
	{
	case 1:
		{
			int i=0;
			char parm[5];
			for(i=0;i<pmatch[2].rm_eo-pmatch[2].rm_so;i+=4)
			{
				bzero(parm,sizeof(parm));
				strncpy(parm,cmd+pmatch[2].rm_so+i,4);
				switch(parm[0])
				{
				case '1':
					timeint_loc[0]=strtol(parm+2,NULL,10);
					if(rec_parm(REC_TIMEINT1)==0)             //faild
						error=1;
					break;
				case '2':	
					timeint_loc[1]=strtol(parm+2,NULL,10);
					//printf("1111   %d\n",timeint_loc[1]);
					if(!rec_parm(REC_TIMEINT2))
						error=1;
					break;
				case '3':
					timeint_loc[2]=strtol(parm+2,NULL,10);
					if(!rec_parm(REC_TIMEINT3))
						error=1;
					break;
				}
				switch_timeint_loc|=((parm[1]-'0')<<(parm[0]-'0'-1));
				if(!rec_parm(REC_SWITCH))
					error=1;
			}	
			if(error)
				printf("Set RTU interval FAILD\n");
			else
				printf("Set RTU interval OK\n");
			break;
		}
	case 2:
		{
			bzero(serverIP,sizeof(serverIP));
			memcpy(serverIP,cmd+pmatch[2].rm_so,pmatch[2].rm_eo-pmatch[2].rm_so);
			if(!rec_parm(REC_SERIP))
			{
				error=1;
			}
			if(error)
				printf("Set RTU serverIP FAILD\n");
			else
				printf("Set RTU serverIP OK\n");
			break;
		}
	case 3:
		{
			bzero(serverPORT,sizeof(serverPORT));
			memcpy(serverPORT,cmd+pmatch[2].rm_so,pmatch[2].rm_eo-pmatch[2].rm_so);
			if(!rec_parm(REC_SERPORT))
			{
				error=1;
			}
			if(error)
				printf("Set RTU serverPORT FAILD\n");
			else
				printf("Set RTU serverPORT OK\n");
			break;
		}
	case 4:
		{
			bzero(hostIP,sizeof(hostIP));
			memcpy(hostIP,cmd+pmatch[2].rm_so,pmatch[2].rm_eo-pmatch[2].rm_so);
			if(!rec_parm(REC_CLIIP))
			{
				error=1;
			}
			if(error)
				printf("Set RTU hostIP FAILD\n");
			else
				printf("Set RTU hostIP OK\n");
			break;
		}
	case 5:
		{
			bzero(hostPORT,sizeof(hostPORT));
			memcpy(hostPORT,cmd+pmatch[2].rm_so,pmatch[2].rm_eo-pmatch[2].rm_so);
			if(!rec_parm(REC_CLIPORT))
			{
				error=1;
			}
			if(error)
				printf("Set RTU hostPORT FAILD\n");
			else
				printf("Set RTU hostPORT OK\n");
			break;
		}
	case 6:
		{
			unsigned char buf[3];
			strncmp(buf,cmd+pmatch[2].rm_so+3,2);
			rtuaddr=strtol(buf,NULL,10);
			if(!rec_parm(REC_RTUADDR))
				error=1;
			if(error)
				printf("Set RTU address FAILD\n");
			else
				printf("Set RTU address OK\n");
			break;
		}
	case 7:
		{
			unsigned char parm[4]={};
			memcpy(parm,cmd+pmatch[2].rm_so,pmatch[2].rm_eo-pmatch[2].rm_so);
			n_node=strtol(parm,NULL,10);
			if(!rec_parm(REC_NODE))
				error=1;
			if(error)
				printf("Set NODE number FAILD\n");
			else
				printf("Set NODE number OK\n");
			break;
		}
	case 8:
		{
			unsigned char parm[4]={};
			memcpy(parm,cmd+pmatch[2].rm_so,pmatch[2].rm_eo-pmatch[2].rm_so);
			n_sen=strtol(parm,NULL,10);
			if(!rec_parm(REC_SEN))
				error=1;
			if(error)
				printf("Set NODE number FAILD\n");
			else
				printf("Set NODE number OK\n");
			break;
		}
	case 9:
		{
			unsigned char parm[4]={};
			memcpy(parm,cmd+pmatch[2].rm_so,pmatch[2].rm_eo-pmatch[2].rm_so);
			duration=strtol(parm,NULL,10);
			if(!rec_parm(REC_DUR))
				error=1;
			if(error)
				printf("Set RTU duration FAILD\n");
			else
				printf("Set RTU duration OK\n");
			break;
		}
	case 10:
		{
			unsigned char parm[4]={};
			memcpy(parm,cmd+pmatch[2].rm_so,pmatch[2].rm_eo-pmatch[2].rm_so);
			rtutype=strtol(parm,NULL,16);
			if(!rec_parm(REC_RTUTYPE))
				error=1;
			if(error)
				printf("Set RTU device type FAILD\n");
			else
				printf("Set RTU device type number OK\n");
			break;
		}
	case 11:
		{
			unsigned char parm[4]={};
			memcpy(parm,cmd+pmatch[2].rm_so,pmatch[2].rm_eo-pmatch[2].rm_so);
			overtime_server=strtol(parm,NULL,10);
			if(!rec_parm(REC_OTS))
				error=1;
			if(error)
				printf("Set RTU overtime_server FAILD\n");
			else
				printf("Set RTU overtime_server OK\n");
			break;
		}
	case 12:
		{
			unsigned char parm[4]={};
			memcpy(parm,cmd+pmatch[2].rm_so,pmatch[2].rm_eo-pmatch[2].rm_so);
			overtime_client=strtol(parm,NULL,10);
			if(!rec_parm(REC_OTC))
				error=1;
			if(error)
				printf("Set RTU overtime_client FAILD\n");
			else
				printf("Set RTU overtime_client OK\n");
			break;
		}
	case 13:
		{
			unsigned char parm[4]={};
			memcpy(parm,cmd+pmatch[2].rm_so,pmatch[2].rm_eo-pmatch[2].rm_so);
			time_repeat=strtol(parm,NULL,10);
			if(!rec_parm(REC_REP))
				error=1;
			if(error)
				printf("Set RTU time_repeat FAILD\n");
			else
				printf("Set RTU time_repeat OK\n");
			break;

			
		}
	}
	return ;
}

/******************************************
 *	earn YYYY-MM-DD HH:MM:SS
 * * */
void earntime(time_t t,unsigned char *string)
{
	if(t==0)
	{
		sprintf(string,"0000-00-00 00:00:00");
		return;
	}
	struct tm *tim;
	//time(&t);
	tim=localtime(&t);
	sprintf(string,"%d-%02d-%02d %02d:%02d:%02d",tim->tm_year+1900,tim->tm_mon+1,tim->tm_mday,tim->tm_hour,tim->tm_min,tim->tm_sec);
	return;
}


/********************************************
 * print information of -l
 *******************************************/
void print(int which)
{
#ifdef DEBUG
	printf("print in\n");
#endif
	unsigned char *flag_switch[3];
	unsigned char flag_onlinenode[4];
	unsigned char rtutime[20];

//	unsigned char switch_timeint;
//	int timeint[3];
//	unsigned char str_workmode[2][]={"Self-reported","Called-measurement"};
//
//	unsigned char year[5];
//	unsigned char mon[3];
//	unsigned char day[3];
//	unsigned char hh[3];
//	unsigned char mm[3];
//	unsigned char ss[3];
//	unsigned short runtime;
//	unsigned short sleeptime;

	if(which==PRINT_RTU)
	{
		time_t t_loc;
		time(&t_loc);
		earntime(t_loc,rtutime);
		float valt;	
		float curr;

		pthread_mutex_lock(&mutex_shmbuf);
		valt=((float)disply_shm.volt)/100;
		curr=((float)disply_shm.curr)/100;
		pthread_mutex_unlock(&mutex_shmbuf);

		pthread_mutex_lock(&mutex_parm_loc);
		int i;
		for(i=0;i<3;i++)
		{
			if((switch_timeint_loc&(1<<i))!=0)
				flag_switch[i]="ON";
			else 
				flag_switch[i]="OFF";
		}
		for(i=0;i<N_NODE;i++)
		{
			if((disply_shm.flag_online&(1<<i))!=0)
			{
				flag_onlinenode[i]=0x76;
			}
			else
			{
				flag_onlinenode[i]=0x20;
			}
		}
		printf("=======================================================================\n");
		printf("=   ||||RTU||||   NODE 1   |   NODE 2   |   NODE 3   |   NODE 4   |   =\n");
		printf("=                                                                     =\n");
		printf("=   RTU address : 00%02d0000                                            =\n",rtuaddr);
		printf("=                                                                     =\n");
		printf("=   RTU type : %02x                                                     =\n",rtutype);
		printf("=                                                                     =\n");
		printf("=   time interval   |   switch       hour                             =\n");
		printf("=         1         |   %-3s          %-2d                               =\n",flag_switch[0],timeint_loc[0]);
		printf("=         2         |   %-3s          %-2d                               =\n",flag_switch[1],timeint_loc[1]);
		printf("=         3         |   %-3s          %-2d                               =\n",flag_switch[2],timeint_loc[2]);
		printf("=                                                                     =\n");
		printf("=   server IP : %-16s                                      =\n",serverIP);
		printf("=   server PORT : %-5s                                               =\n",serverPORT);
		printf("=                                                                     =\n");
		printf("=   host   IP : %-16s                                      =\n",hostIP);
		printf("=   host   port : %-5s                                               =\n",hostPORT);
		printf("=                                                                     =\n");
		printf("=   voltage : %-4.2f                                                    =\n",valt);
		printf("=   circuit : %-4.2f                                                    =\n",curr);
		printf("=                                                                     =\n");
		printf("=                |   NODE 1    NODE 2    NODE 3    NODE 4             =\n");
		printf("=   online node  |   %1c         %1c         %1c         %1c                  =\n",flag_onlinenode[0],flag_onlinenode[1],flag_onlinenode[2],flag_onlinenode[3]);
		printf("=                                                                     =\n");
		printf("=   local time : %-20s                                 =\n",rtutime);
		printf("=                                                                     =\n");
		printf("=                                                                     =\n");
		printf("=                                                                     =\n");
		printf("=                                                                     =\n");
		printf("=                                                                     =\n");
		printf("=                                                                     =\n");
		printf("=======================================================================\n");

		pthread_mutex_unlock(&mutex_parm_loc);
	
	}
	else
	{
		int i;
		i=which-23;
		
		shm_s buf;
		pthread_mutex_lock(&mutex_shmbuf);
		memcpy(&buf,&disply_shm,sizeof(buf));
		pthread_mutex_unlock(&mutex_shmbuf);

		unsigned char time_login[20];
		unsigned char time_logoff[20];
		unsigned char time_login_bak[20];
		unsigned char time_logoff_bak[20];


		earntime(buf.disply_node[i].t_login,time_login);
		earntime(buf.disply_node[i].t_logoff,time_logoff);
		earntime(buf.disply_node[i].t_login_bak,time_login_bak);
		earntime(buf.disply_node[i].t_logoff_bak,time_logoff_bak);

		unsigned char switch_timeint;
		int timeint[3];


		int l;
		for(l=0;l<3;l++)
		{
			unsigned char a[3];
			sprintf(a,"%02x",buf.disply_node[i].afnbroadtser.timeset[l]&0x7f);
			switch_timeint|=((buf.disply_node[i].afnbroadtser.timeset[l]>>7)<<l);
			timeint[l]=strtol(a,NULL,10);
		}

		for(l=0;l<3;l++)
		{
			if((switch_timeint&(1<<l))!=0)
				flag_switch[l]="ON";
			else 
				flag_switch[l]="OFF";
		}

		unsigned char str_workmode[2][24]={"Self-reported","Called-measurement"};

		unsigned char year[5];
		unsigned char mon[3];
		unsigned char day[3];
		unsigned char hh[3];
		unsigned char mm[3];
		unsigned char ss[3];

		sprintf(year,"%02x",buf.disply_node[i].afnbroadtser.nodetime[0]);
		sprintf(year+2,"%02x",buf.disply_node[i].afnbroadtser.nodetime[1]);
		sprintf(mon,"%02x",buf.disply_node[i].afnbroadtser.nodetime[2]);
		sprintf(day,"%02x",buf.disply_node[i].afnbroadtser.nodetime[3]);
		sprintf(hh,"%02x",buf.disply_node[i].afnbroadtser.nodetime[4]);
		sprintf(mm,"%02x",buf.disply_node[i].afnbroadtser.nodetime[5]);
		sprintf(ss,"%02x",buf.disply_node[i].afnbroadtser.nodetime[6]);

		unsigned short runtime;
		memcpy(&runtime,buf.disply_node[i].afnbroadtser.runtime,2);
		unsigned short sleeptime;
		memcpy(&sleeptime,buf.disply_node[i].afnbroadtser.sleeptime,2);

		//printf("=======================================================================\n");
		//printf("=   |   RTU   ||||NODE 1||||   NODE 2   |   NODE 3   |   NODE 4   |   =\n");
		//printf("=                                                                     =\n"); 
	
		printf("=======================================================================\n");
		switch(which)
		{
		case PRINT_NODE_ONE:
			printf("=   |   RTU   ||||NODE 1||||   NODE 2   |   NODE 3   |   NODE 4   |   =\n");			
			printf("=                                                                     =\n"); 
			printf("=               %-3s Line                                              =\n",(((buf.flag_online&(1<<i))==(1<<i))?"ON":"OUT"));

			break;
		case PRINT_NODE_TWO:
			printf("=   |   RTU   |   NODE 1   ||||NODE 2||||   NODE 3   |   NODE 4   |   =\n");			
			printf("=                                                                     =\n"); 
			printf("=                           %-3s Line                                  =\n",(((buf.flag_online&(1<<i))==(1<<i))?"ON":"OUT"));

			break;
		case PRINT_NODE_THR:
			printf("=   |   RTU   |   NODE 1   |   NODE 2   ||||NODE 3||||   NODE 4   |   =\n");			
			printf("=                                                                     =\n"); 
			printf("=                                        %-3s Line                     =\n",(((buf.flag_online&(1<<i))==(1<<i))?"ON":"OUT"));

			break;
		case PRINT_NODE_FOU:
			printf("=   |   RTU   |   NODE 1   |   NODE 2   |   NODE 3   ||||NODE 4||||   =\n");			
			printf("=                                                                     =\n"); 
			printf("=                                                     %-3s Line        =\n",(((buf.flag_online&(1<<i))==(1<<i))?"ON":"OUT"));

			break;
		}
		
		printf("=                                                                     =\n");
		printf("=   Address : 00%02d%02x00                                                =\n",rtuaddr,buf.disply_node[i].node_addr);
		printf("=                                                                     =\n");
		printf("=   Vertion : %02x%02x                                                    =\n",buf.disply_node[i].afnbroadtser.nodeversion[0],buf.disply_node[i].afnbroadtser.nodeversion[1]);
		printf("=   Work Mode : %-20s                                  =\n",str_workmode[buf.disply_node[i].afnbroadtser.workmode-1]);
		printf("=   Login Time (NODE) : %s-%s-%s %s:%s:%s                           =\n",year,mon,day,hh,mm,ss);
		printf("=   Running Time (minut): %2d                                          =\n",runtime);
		printf("=   Sleeping Time (minut): %2d                                         =\n",sleeptime);
		printf("=                                                                     =\n");
		printf("=   IP : %-16s                                             =\n",buf.disply_node[i].ip);
		printf("=   PORT : %-6d                                                     =\n",buf.disply_node[i].port);
		printf("=                                                                     =\n");
		printf("=   Login Voltage : %-4.2f                                              =\n",((float)strtol(buf.disply_node[i].afnbroadtser.nodevolt,NULL,10))/100);
		printf("=   Login Cirruit : %-4.2f                                              =\n",((float)strtol(buf.disply_node[i].afnbroadtser.nodecir,NULL,10))/100);
		printf("=                                                                     =\n");
		printf("=   time interval   |   switch       hour                             =\n");
		printf("=         1         |     %-3s        %02d                               =\n",flag_switch[0],timeint[0]);
		printf("=         2         |     %-3s        %02d                               =\n",flag_switch[1],timeint[1]);
		printf("=         3         |     %-3s        %02d                               =\n",flag_switch[2],timeint[2]);
		printf("=                                                                     =\n");
		printf("=   Login  time : %-20s                                =\n",time_login);
		printf("=   Logoff time : %-20s                                =\n",time_logoff);
		printf("=   Last Login  time : %-20s                           =\n",time_login_bak);
		printf("=   Last Logoff time : %-20s                           =\n",time_logoff_bak);
		printf("=                                                                     =\n");
		printf("=   RX byte : %-30ld                          =\n",buf.disply_node[i].rx);
		printf("=   TX byte : %-30ld                          =\n",buf.disply_node[i].tx);
		printf("=                                                                     =\n");
		printf("=======================================================================\n");






	}
	return;
}




/******************************
 * release resouce
 * */
void releaseresouce(void)
{
	pthread_mutex_destroy(&mutex_parm_loc);
	pthread_mutex_destroy(&mutex_shmbuf);
	pthread_mutex_destroy(&mutex_tab);

	pthread_cond_destroy(&cond_tab);

	shmctl(shmid,IPC_RMID,NULL);
//	msgctl(msgid,IPC_RMID,NULL);
//	semctl(semid,1,IPC_RMID,NULL);

	pthread_cancel(id_shm);
	return;
}

void handler_alarm(int signo)
{
	pthread_cond_signal(&cond_tab);
	return;
}

/*********************************************
 *	main function
 *
 *********************************************/

int main(int argc, const char *argv[])
{
	int nmatch=8;
	int err;
	char errbuf[1024];
	regex_t reg;
	regmatch_t pmatch[nmatch];
	int cflags=(REG_EXTENDED|REG_ICASE);

	char cmd[256];

	pthread_mutex_init(&mutex_shmbuf,NULL);
	pthread_mutex_init(&mutex_parm_loc,NULL);
	pthread_mutex_init(&mutex_tab,NULL);

	pthread_cond_init(&cond_tab,NULL);

	pthread_create(&id_shm,0,&pthread_shm,NULL);
	pthread_create(&id_parm,0,&pthread_parm,NULL);
	printf("Loading Data ...\n");
	sleep(10);


	if(argc<3)
	{
		if(argc==2 && (strcmp(argv[1],"--help")==0));
		else
		{
error:	printf("debug : invalid format of debug\n");
		printf("        Try 'debug --help' for more information\n");
		return 0;
		}
	}
	if(!strcmp(argv[1],"--help"))
	{
		printf("Usage : debug [option] RTU/NODE [+suboption] [paragrams] ...\n");
		printf("        or 'debug -l all'\n");
		printf("Such as : 'debug -l NODE 1' or 'debug -s RTU duration 60'\n");
		printf("Use for : look the current running state or set the paragrams of RTU or NODE\n");
		printf("option :\n");
		printf("  -l ~~~ look the current running stat of RTU or NODE or both\n");
		printf("  -s ~~~ set the paragrams of RTU or NODE\n");
		printf("suboption :\n");
		printf("  interval ~~~ time interval , for example : \n");
		printf("              'interval 004 123' symbol first time interval is OFF time interval hour is 04\n");
		printf("  serverIP ~~~ the IP of DWserver \n");
		printf("  serverPORT ~~~ the PORT of DWserver \n");
		printf("  hostIP ~~~ the IP of UPclient \n");
		printf("  hostPORT ~~~ the PORT of UPclient \n");
		printf("  address ~~~ two bits number of RTU , four bits number of NODE\n");
		printf("  nodenumber ~~~ the number of node\n");
		printf("  sensornuber ~~~ the number of sensor\n");
		printf("  duration ~~~ revise the duration\n");
		printf("  overtime_server ~~~ passby frame wait limit time\n");
		printf("  overtime_client ~~~ passby frame wait limit time\n");
		printf("  time_repeat ~~~ the interval of doing test frame with NODE\n");
	}

	if((strcmp(argv[1],"-l")!=0) && (strcmp(argv[1],"-s")!=0))
	{
		printf("debug : invalid option of debug\n");
		printf("        Try 'debug --help' for more information\n");
		return 0;
	}
	else
	{
		if(strcmp(argv[1],"-l")==0)                   //debug -l 
		{
			int j=5;
			if(!strcmp(argv[2],"NODE"))
			{
				int i;
				for(i=1;i<=N_NODE;i++)
				{
				//	unsigned char num;
				//	memcpy(&num,argv[3],1);
					if((*argv[3]-'0')==i)
					{
#ifndef DISPLAY 
					system("clear");
#endif				
						while(j--)
						{
							print(i+22);
							sleep(1);
#ifndef DISPLAY 
					system("clear");
#endif				
						}
						break;
					}
				}
				if(i==N_NODE)
					goto error;
			}
			else if(!strcmp(argv[2],"RTU"))
			{
#ifdef DEBUG
				printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!-l RTU in\n");
#endif
				pthread_create(&id_parm,0,&pthread_parm,NULL);

				sleep(1);
#ifndef DISPLAY 
				system("clear");
#endif
				while(j--)
				{
					print(PRINT_RTU);
					pthread_create(&id_parm,0,&pthread_parm,NULL);
					sleep(1);
#ifndef DISPLAY 
					system("clear");
#endif				
				}
			}
			else if(!strcmp(argv[2],"all"))
			{
				int *flag_tab,l;
				signal(SIGALRM,handler_alarm);
				flag_tab=malloc(sizeof(int));
				*flag_tab=1;
				pthread_create(&id_key,0,&pthread_key,flag_tab);
				pthread_mutex_lock(&mutex_tab);
				while(1)
				{
					alarm(5);
					switch(*flag_tab)
					{
					case 1:
						print(PRINT_RTU);
						break;
					case 2:
						print(PRINT_NODE_ONE);
						break;
					case 3:
						print(PRINT_NODE_TWO);
						break;
					case 4:
						print(PRINT_NODE_THR);
						break;
					case 5:
						print(PRINT_NODE_FOU);
						break;
					}
					pthread_mutex_unlock(&mutex_tab);
					pthread_mutex_lock(&mutex_tab);
					pthread_cond_wait(&cond_tab,&mutex_tab);
				}
			}
			else
			{
				goto error;
			}
		}
		else if(strcmp(argv[1],"-s")==0)                  //debug -s 
		{
			if(argc<4)
				goto error;
			if(!strcmp(argv[2],"NODE"))                   // not support -s NODE temperare
			{
				//if(argc<6)
				goto error;
			}
			else                      //RTU
			{
				int j;
				for(j=0;j<argc;j++)
				{
					sprintf(cmd+strlen(cmd),"%s",argv[j]);
				}

				if(!strcmp(argv[2],"RTU"))
				{
					int i,c;
					for(i=1;i<=N_OPT;i++)
					{
						bzero(pmatch,sizeof(pmatch));
						if(regcomp(&reg,optionname[i-1],cflags)<0)
						{
							regerror(err,&reg,errbuf,sizeof(errbuf));
							continue;
						}
						err=regexec(&reg,cmd,nmatch,pmatch,0);
						if(err==REG_NOMATCH)
						{
							continue;
						}
						else if(err)
						{
							regerror(err,&reg,errbuf,sizeof(errbuf));
							continue;
						}
						if(i==1)
						{
							char *p;
							p=(char *)malloc(sizeof(char));
							pthread_create(&id_parm,0,&pthread_parm,p);
						}
						analysis(i,cmd,pmatch);
						c=i;
						regfree(&reg);

					}


					if(c==2||c==3)
					{
						system("pkill -SIGINT display");
						sleep(1);
						system("killall DWserver");
						sleep(1);
						//system("./rturun/DWserver &");
					}
					else
					{
						if(c==4||c==5)
						{
							system("pkill -SIGINT display");
							sleep(1);
							system("killall UPclient");
							sleep(1);
							//system("./rturun/UPclcent &");
						}
						else
						{
							system("pkill -SIGINT DWserver");
							system("pkill -SIGINT UPclcent");
						}
					}

				}
				else
				{
					goto error;
				}
			}
		}
	}

	pthread_mutex_destroy(&mutex_parm_loc);
	pthread_mutex_destroy(&mutex_shmbuf);
	
	atexit(&releaseresouce);

	return 0;
}

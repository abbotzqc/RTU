#include<stdio.h>
#include<pthread.h>

//#define UPCLIENT 1               //编译UPclient时用，区分frame.c中到两部分  

//#define DISPLAY 1                //used for DEBUG print on console
//#define DEBUG   1                //used for DEBUG  


/******************************************
 *	file path macro 
 * **************************************/
#define PATH_VAL      "/media/mmcblk0p1/Data/vol.log"
#define PATH_SERIAL   "/dev/ttySAC3"
#define PATH_PARM     "/rturun/parm.log"
#define PATH_SHM      "/rturun/app"
#define PATH_MSG      "/rturun/appm"
#define PATH_LINK     "/media/mmcblk0p1/Data/linkdata/"
#define PATH_MEDIA    "/media/mmcblk0p1/"
#define PATH_SD       "/media/mmcblk0p1/Data/sdsize.log"
#define PATH_CLIWR    "/rturun/rtu.log"//client.log"
#define PATH_SERWR    "/rturun/rtu.log"//server.log"
/******************************************************************************
 *	led used
 * */
#include"ledlib.h"
#define ON  1
#define OFF 0

//	beep used
#include"beeplib.h"
/******************************************************************************/

/******************************
 * struct afn server used
 ********************************/
#ifndef AFNBROADCASTSER_H
#define AFNBROADCASTSER_H
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
#endif


/******************************************************************************
 * 	UPclient used
 ******************************************************************************/
#define PATTERN "68(0{2}[0-9]{1}[1-9]{1}[0-9]{4}|f{8})68(41|42|43)[0-f]{14}(01|02|04|05|06|f1|f3|f2|f0).*06[0-f]{4}"
#define N_FRAME 1024
#define N_BUF 516//1024
#define N_NODE 4
#define N_READ 256
#define DIR_R 0     //frame direction
#define DIR_T 1
#define FRM_REGLEN (18-2) 
#define DATAZONEOFFSET 12  
#define RTUADDR 1
#define BROADCASTADDR 165
#define LOCALADDR RTUADDR        //address of which machine communication protocol used in
#define MSG 1
#define FD 0
#define MSG_MSGTYPE 1
#define MSGNODE_MSGTYPE 2
#define N_DOWN 1056
#define RTUTYPE 1
#define RTUVERSION_M 1
#define RTUVERSION_S 2

#define LATESTTEST 0x11             //use for shm     0x10-address is 0xfffffff
									//				  0x01-data in shm is new 
#define PASSBY     0x21             //passby frame   


#define FAILD 5                //deny frame wrong reason code
#define SUCCESS 1
#define OUTLINE 2

/*****************************
 *	macro used for deny frame
 *****************************/
#define ERR_OVERTIMEC    0xe6
#define ERR_OVERTIME     0xe5
#define ERR_EXEC 	 	 0xe1
//#define ERR_DATAFRM  	 0xe2
#define ERR_ONLINE 		 0xe3
#define ERR_PASS   		 0xe2
#define ERR_NODEBUSY     0xe0
#define NONE   				1


/*****************************
 * macro used for pthread_parm
 ******************************/
#define READ_PARM	 1                             //read paragram from bak file
#define REC_RTUADDR	 0xa1
#define REC_RTUTYPE	 0xa2
#define REC_SEQL	 0xa3
#define REC_TIMEINT  0xa4
#define REC_OVERTS   0xa5
#define REC_OVERTC   0xa6
#define REC_REP      0xa7
#define REC_DUR      0xa8


typedef struct ipinfo
{
	int port;
	char *ip;
} ipinfo_s;

int shmid;
int msgid;
//int semid;

/******************************
 *	macro used for msgtype
 *****************************/
#define MSG_TOCLIENT 0xd1         //d--deliver
#define MSG_TOSER    0xd2

#ifndef MSGTOSER_H
#define MSGTOSER_H
typedef struct msgtoser
{
	long msgtype;               
	unsigned char addr[4];
	unsigned char seq;
	unsigned char afn;
	//char bufrdown[N_DOWN];	
	unsigned char buf[64];
} msgtoser_s;
#endif

typedef struct msgtocli
{
	long msgtype;
	unsigned char err_flag;
	unsigned char buf[1048];
} msgtocli_s;

/*****************************
 *	struct PASSBY frame used
 *****************************/
typedef struct passby
{
	unsigned char seq;
	unsigned char afn;
} passby_s;

/*
typedef struct nodemsg
{
	int year_rec;
	int month_rec;
	int day_rec;
	int hour_rec;
	int min_rec;
	int sec_rec;
	unsigned char nodevolt[2];
	unsigned char nodecir[2];
	unsigned char nodetime[7];
	unsigned char nodemode;
	unsigned char runtime[2];
	unsigned char sleeptime[2];
	unsigned char timeset_one;
	unsigned char timeset_two;
	unsigned char timeset_three;
	unsigned char nodeversion[2];
} nodemsg_s;
*/

typedef struct nodemsg
{
	unsigned char year_rec;
	unsigned char month_rec;
	unsigned char day_rec;
	unsigned char hour_rec;
	unsigned char min_rec;
	unsigned char sec_rec;
	unsigned char nodevolt[2];
	unsigned char nodecir[2];
	unsigned char nodetime[7];
	unsigned char nodemode;
	unsigned char runtime[2];
	unsigned char sleeptime[2];
	unsigned char timeset_one;
	unsigned char timeset_two;
	unsigned char timeset_three;
	unsigned char nodeversion[2];
} nodemsg_s;

typedef struct msgnode
{
	long msgtype;                 //2  MSGNODE_MSGTYPE
	int nodenum;
	nodemsg_s nodemsg;
} msgnode_s;

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
	unsigned short volt;
	unsigned short curr;
	int timeint[3];
	unsigned char switch_timeint;
	unsigned char overtime_server;
	unsigned char time_repeat;

	unsigned char flag_online;
	disply_node_s disply_node[4];
} shm_s;

pthread_mutex_t mutex_errno;
pthread_mutex_t mutex_shmbuf;
pthread_mutex_t mutex_online;
pthread_mutex_t mutex_buft;
pthread_mutex_t mutex_bufr;
pthread_mutex_t mutex_sdsize;
pthread_mutex_t mutex_led;
//pthread_mutex_t mutex_socket;


pthread_cond_t cond_led;
pthread_cond_t cond_sdsize;
pthread_cond_t cond_write;
pthread_cond_t cond_framere;
pthread_cond_t cond_shm;
pthread_cond_t cond_fddata;
pthread_cond_t cond_fderrlog;
pthread_cond_t cond_readlog;
/****************************
 *	struct used for communication protocol
 ****************************/

typedef struct frameinfo
{
	unsigned short addr;
	unsigned char seq;
	unsigned short sum;
	unsigned short num;
	unsigned short datalen;
	unsigned char afn;
	unsigned char *pdata;
} frameinfo_s;

typedef struct frameloc
{
	char  *phead;
	int  len;
} frameloc_s;
/*
typedef struct afn01r
{
	
} afn01r_s;
*/
/*
typedef struct afn01t
{
	char rtutype;
	short rtuvolt;
	short rtucurr;
	short rtuversion;
} afn01t_s;
*/
/*
typedef struct nodeinfo
{
	char year;
	char month;
	char day;
	char hour;
	char min;
	char sec;
	char year_rec;
	char month_rec;
	char day_rec;
	char hour_rec;
	char min_rec;
	char sec_rec;
	short nodevolt;
	short nodeversion;
	char nodemode;
	char nodestart;
	char nodesleep;
	char timeset_one;
	char timeset_two;
	char timeset_three;
} nodeinfo_s;
*/
typedef struct afnbroadt
{
//	afn01t_s afn01t;
//	nodeinfo_s nodeinfo[N_NODE];
	nodemsg_s nodemsg[N_NODE];
} afnbroadt_s;

typedef struct afn02r
{
	unsigned int addrall;	
} afn02r_s;

typedef struct afn04r
{
	int year;
	int month;
	int day;
	int hour;
	int min;
	int sec;
} afn04r_s;

typedef struct afn05r
{
	unsigned char nodeno;
	unsigned char sensorno;
	unsigned int year;
	unsigned int month;
	unsigned int day;
	unsigned int hour;//timeint
	unsigned int timelong;//00
} afn05r_s;

typedef struct afn05
{
	unsigned char nodeno;
	unsigned char sensorno;
	unsigned char hour;
	unsigned char min;
	unsigned char sec;
} afn05_s;

typedef struct afn05t
{
	unsigned char recordsum;
	unsigned int year;
	unsigned int month;
	unsigned int day;
//	unsigned int hour;//timeint
	afn05_s *pafn05;
} afn05t_s;

typedef struct afn06r
{
	unsigned char nodeno;
	unsigned char sensorno;
	int year;
	int month;
	int day;
	int hour;
	int min;
	int sec;
} afn06r_s;

typedef struct afn06t
{
	int frameno;
	unsigned char data[1024];
} afn06t_s;

typedef struct afnf1r
{
	int frameno;
} afnf1r_s;

typedef struct afnf1t
{
	int frameno;
	char data[1024];
} afnf1t_s;
/*
typedef struct afnf3r
{
	unsigned char *switch_timeint;
	int *timeint_one;
	int *timeint_two;
	int *timeint_three;
} afnf3r_s;
*/



/******************************************************************************
 * Server used
 *
 *******************************************************************************/
pthread_cond_t	cond_testhead,cond_testtail,cond_storehead;

pthread_mutex_t mutex_storehead,
				mutex_storetail,
				mutex_testhead,
				mutex_testtail;

/*********************************
 *	macro used by server
 *********************************/
#define PATTERN_1 "68(00[0-9][1-9]0[1-4]00)6888([0-9A-Fa-f]{2})0100010016000102([0-9]{22}(02|01)[0-9A-Fa-f]{8}[0-9A-Fa-f]{10})06[0-9A-Fa-f]{4}"
#define PATTERN_2 "68(00[0-9][1-9]0[1-4]00)6880([0-9A-Fa-f]{2})010001000100(02|04|f2|F2)(00)06[0-9A-Fa-f]{4}|68(00[0-9][1-9]0[1-4]00)6881([0-9A-Fa-f]{2})010001000100(02|04|f2|F2)(e[0-5])06[0-9A-Fa-f]{4}"
#define PATTERN_3 "68(00[0-9][1-9]0[1-4]0[1-6])6888([0-9A-Fa-f]{2})2000([0-2][0-9a-fA-F]00)0004[fF]0([0-9a-fA-F]{2048})06[0-9a-fA-F]{4}|68(00[0-9]{6})6889([0-9A-Fa-f]{2})0100(0100)0100[fF]0(e[1-5])06[0-9a-fA-F]{4}"
#define PATTERN_4 "68(00[0-9][1-9]0[1-4]0[1-6])6888([0-9A-Fa-f]{2})0100([0-2][0-9a-fA-F]00)0004[fF]1([0-9a-fA-F]{2048})06[0-9a-fA-F]{4}|68(00[0-9]{6})6889([0-9A-Fa-f]{2})0100(0100)0100[fF]1(e[1-5])06[0-9a-fA-F]{4}|68(00[0-9][1-9]0[1-4]0[1-6])6888([0-9A-Fa-f]{2})2000([0-2][0-9a-fA-F]00)0004[fF]0([0-9a-fA-F]{2048})06[0-9a-fA-F]{4}"

/*********************************
 *	global paragram used by server
 *********************************/
pthread_t id_alarm1;
pthread_t id_alarm2;
pthread_t id_alarm3;
pthread_t id_alarm4;
pthread_t id_shmser;
pthread_t id_msgser;





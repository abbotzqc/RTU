
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

typedef struct pass_node
{
	unsigned char addr[4];
	unsigned char seq;
	unsigned char afn;
	unsigned char buf[64];
	struct pass_node *next;
} pass_linklist;

typedef unsigned char frametype;
typedef struct cmd_node
{
	frametype frame;				//macro 
	unsigned short framedata;                  //used for 0xf0 & 0xf1   
	unsigned char timeintno;
	struct cmd_node *next;
} cmd_linklist;

typedef struct store_node
{
	unsigned char nodeno;
	unsigned char sensorno;
	unsigned char timeintno;
	time_t t;
	unsigned char buf[32][1025];
	struct store_node *next;
} store_linklist;

typedef struct test_node
{
	unsigned char nodeno;
	time_t t;
	unsigned char buf[24];
	struct test_node *next;
} test_linklist;

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

typedef struct nodeprivate
{
	pthread_t tid;                               //pthread id
	unsigned int connectfd;					     //connectfd
	cmd_linklist *cmd_head;                      //self_report node get frame from it
	pass_linklist *pass_head;                    //get passby frame from it

	unsigned char node_addr;                      //node addr
	unsigned char ip[16];
	unsigned int port;

	time_t t_login;
	time_t t_logoff;
	time_t t_login_bak;
	time_t t_logoff_bak;

	long rx;
	long tx;
	
	afnbroadtser_s afnbroadtser;
		
	unsigned char flag_timeint[3];				  //flag_timeint[n]==0x3f symbol in n-1 interval time the data of each six sensors asked compeletly
//	unsigned int flag_sensor[3][6];              //flag_sensor[n][m]=0xffffffff symbol in n-1 interval time the frame of the m sensor asked compeletly
	
	unsigned char busy;
	int fd_log;
} nodeprivate_s;

cmd_linklist* cmd_CreateLinklist();
int cmd_EmptyLinklist(cmd_linklist* h);
cmd_linklist* cmd_GetLinknode(cmd_linklist *h);
void cmd_InsertLinklist_tail(cmd_linklist* h,frametype x,unsigned short data,unsigned char timeintno);
int cmd_LengthLinklist(cmd_linklist* h);
pass_linklist* pass_CreateLinklist();
int pass_EmptyLinklist(pass_linklist* h);
pass_linklist* pass_GetLinknode(pass_linklist* h);
void pass_InsertLinklist_tail(pass_linklist* h,msgtoser_s *pmsgbuf);
int pass_LengthLinklist(pass_linklist* h);
test_linklist* test_CreateLinklist();
int test_EmptyLinklist(test_linklist* h);
test_linklist* test_GetLinknode(test_linklist* h);
void test_InsertLinklist_tail(test_linklist* t,unsigned char nodeno,time_t time,unsigned char *buf);
int test_LengthLinklist(test_linklist* h);
store_linklist* store_CreateLinklist();
int store_EmptyLinklist(store_linklist* h);
store_linklist* store_GetLinknode(store_linklist* h);
void store_InsertLinklist_tail(store_linklist* t,unsigned char nodeno,unsigned char sensorno,unsigned char timeintno,time_t time,unsigned char (*buf)[1025]);
int store_LengthLinklist(store_linklist* h);

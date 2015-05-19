#include <stdio.h>
#include <stdlib.h>
#include<string.h>
#include"linklist.h"
#include"data_global.h"
//typedef struct pass_node
//{
//	unsigned char addr[4];
//	unsigned char seq;
//	unsigned char afn;
//	unsigned char buf[64];
//	struct pass_node *next;
//} pass_linklist;
//
//typedef unsigned char frametype;
//typedef struct cmd_node
//{
//	frametype frame;				//macro 
//	unsigned short framedata;                  //used for 0xf0 & 0xf1   
//	unsigned char timeintno;
//	struct cmd_node *next;
//} cmd_linklist;
//
//typedef struct store_node
//{
//	unsigned char nodeno;
//	unsigned char sensorno;
//	unsigned char timeintno;
//	time_t t;
//	unsigned char buf[32][1024];
//	struct store_node *next;
//} store_linklist;
//
//typedef struct test_node
//{
//	unsigned char nodeno;
//	time_t t;
//	unsigned char buf[64];
//	struct test_node *next;
//} test_linklist;
//
//typedef struct nodeprivate
//{
//	unsigned char node_addr;                      //node addr
//	unsigned char ip[16];
//	unsigned int port;
//	pthread_t tid;                               //pthread id
//	unsigned int connectfd;					     //connectfd
//	cmd_linklist *cmd_head;                      //self_report node get frame from it
//	pass_linklist *pass_head;                    //get passby frame from it
//} nodeprivate_s;
//

/**************************************
 * cmd_linklist operation
 */

cmd_linklist* cmd_CreateLinklist()
{
	cmd_linklist* h;
	h = (cmd_linklist*)malloc(sizeof(cmd_linklist));
	h->next = NULL;
	return h;
}

int cmd_EmptyLinklist(cmd_linklist* h)
{
	return (NULL == h->next);
}

cmd_linklist* cmd_GetLinknode(cmd_linklist *h)
{
	if (1 ==cmd_EmptyLinklist (h))	
	{
		return NULL;
	}
	cmd_linklist *p = h->next;
	h->next = p->next;
//	if (p->next == NULL)
//		linkTail = h;
	return p;
}

void cmd_InsertLinklist_tail(cmd_linklist* h,frametype x,unsigned short data,unsigned char timeintno)
{
	cmd_linklist* p,*head;

	while(head->next != NULL) 
		head = head->next;
	p = (cmd_linklist*)malloc(sizeof(cmd_linklist));
	p->frame=x;
	p->framedata=data;
	p->timeintno=timeintno;
	p->next = NULL;
	head->next = p;
	return;
}
/*
void cmd_InsertLinklist_head(cmd_linklist* h, frametype x,framedata data)
{
	cmd_linklist* p;
	p = (cmd_linklist*)malloc(sizeof(cmd_linklist));
	p->frametype=x;
	p->framedata=data;
	p->next=h->next;
	h=p;
	return;
}
*/
int cmd_LengthLinklist(cmd_linklist* h)
{
	int len = 0;
	h = h->next;
	while (h != NULL)
	{
		len++;
		h = h->next;
	}
	return len;
}


/**************************************
 * pass_linklist operation
 */

pass_linklist* pass_CreateLinklist()
{
	pass_linklist* h;
	h = (pass_linklist*)malloc(sizeof(pass_linklist));
	h->next = NULL;
	return h;
}

int pass_EmptyLinklist(pass_linklist* h)
{
	return (NULL == h->next);
}

pass_linklist* pass_GetLinknode(pass_linklist* h)
{
	if (1 ==pass_EmptyLinklist (h))	
	{
		return NULL;
	}
	pass_linklist* p = h->next;
	h->next = p->next;
//	if (p->next == NULL)
//		linkTail = h;
	return p;
}

void pass_InsertLinklist_tail(pass_linklist *h,msgtoser_s *pmsgbuf)//unsigned char afn,unsigned char seq,unsigned char* buf)
{
	pass_linklist *p,*head;
	head=h;

	while(head->next!=NULL)
		head=head->next;
	
	p = (pass_linklist*)malloc(sizeof(pass_linklist));
	memcpy(p->addr,pmsgbuf->addr,sizeof(p->addr));
	p->afn=pmsgbuf->afn;
	p->seq=pmsgbuf->seq;
	memcpy(p->buf,pmsgbuf->buf,sizeof(p->buf));
//	memcpy(p,pmsgbuf+sizeof(long),sizeof(pass_linklist));
	p->next = NULL;
	head->next = p;
	return;
}
/*
void pass_InsertLinklist_head(pass_linklist* h, frametype x,framedata data)
{
	pass_linklist* p;
	p = (pass_linklist*)malloc(sizeof(pass_linklist));
	p->frametype=x;
	p->framedata=data;
	p->next=h->next;
	h=p;
	return;
}
*/
int pass_LengthLinklist(pass_linklist* h)
{
	int len = 0;
	h = h->next;
	while (h != NULL)
	{
		len++;
		h = h->next;
	}
	return len;
}



/**************************************
 *	test_linklist operation 
 */

test_linklist* test_CreateLinklist()
{
	test_linklist* h;
	h = (test_linklist*)malloc(sizeof(test_linklist));
	h->next = NULL;
	return h;
}

int test_EmptyLinklist(test_linklist* h)
{
	return (NULL == h->next);
}

test_linklist* test_GetLinknode(test_linklist* h)
{
	if (1 ==test_EmptyLinklist (h))	
	{
		return NULL;
	}
	test_linklist* p; 
	p=h->next;
	h->next = p->next;
//	if (p->next == NULL)
//		linkTail = h;
	return p;
}

void test_InsertLinklist_tail(test_linklist* t,unsigned char nodeno,time_t time,unsigned char *buf)
{
	test_linklist* p,*h;
	h=t;
	while(h->next != NULL) 
		h = h->next;
	p = (test_linklist*)malloc(sizeof(test_linklist));
	p->nodeno=nodeno;
	p->t=time;
	memcpy(p->buf,buf,21);
	p->next = NULL;
	h->next = p;
	return;
}

int test_LengthLinklist(test_linklist* h)
{
	int len = 0;
	h = h->next;
	while (h != NULL)
	{
		len++;
		h = h->next;
	}
	return len;
}


/**************************************
 *	store_linklist operation 
 */

store_linklist* store_CreateLinklist()
{
	store_linklist* h;
	h = (store_linklist*)malloc(sizeof(store_linklist));
	h->next = NULL;
	return h;
}

int store_EmptyLinklist(store_linklist* h)
{
	return (NULL == h->next);
}

store_linklist* store_GetLinknode(store_linklist* h)
{
	if (1 ==store_EmptyLinklist (h))	
	{
		return NULL;
	}
	store_linklist* p = h->next;
	h->next = p->next;
//	if (p->next == NULL)
//		linkTail = h;
	return p;
}

void store_InsertLinklist_tail(store_linklist* t,unsigned char nodeno,unsigned char sensorno,unsigned char timeintno,time_t time,unsigned char (*buf)[1025])
{
	store_linklist* p,*h;
	h=t;
	while(h->next != NULL) 
		h = h->next;
	
	p = (store_linklist*)malloc(sizeof(store_linklist));
	p->nodeno=nodeno;
	p->sensorno=sensorno;
	p->timeintno=timeintno;
	p->t=time;
	memcpy(p->buf,buf,sizeof(p->buf));
	p->next = NULL;
	h->next = p;
//	t->next=p;
//	t=t->next;
	return;
}

int store_LengthLinklist(store_linklist* h)
{
	int len = 0;
	h = h->next;
	while (h != NULL)
	{
		len++;
		h = h->next;
	}
	return len;
}



/*
void VisitLinklist(linklist h)
{
	h = h->next;
	while (h != NULL)
	{
		printf("%d ", h->data);
		h = h->next;
	}
	printf("\n");
}

linklist SearchLinklist(linklist h, datatype x)
{
	h = h->next;
	while (h != NULL)
	{
		if (h->data == x) break;
		h = h->next;
	}
	return h;
}
*/

/*
void InsertLinklist_sort(linklist h, datatype x)                //insert according to order
{
	linklist p;

	while ((h->next != NULL) && (h->next->data < x)) h = h->next;
	p = (linklist)malloc(sizeof(linknode));
	p->data = x;
	p->next = h->next;
	h->next = p;
}

void DeleteLinklist(linklist h, datatype x)
{
	linklist p = h->next;

	while (p != NULL)
	{
		if (p->data == x)
		{
			h->next = p->next;
			free(p);
		}
		else
		{
			h = h->next;
		}
		p = h->next;
	}
}

#if 1
void ClearLinklist(linklist h)
{
	linklist p;

	while (h->next != NULL)
	{
		p = h->next;
		h->next = p->next;
		free(p);
	}
}
#else
void ClearLinklist(linklist h)
{
	linklist p, q;

	p = h->next;
	h->next = NULL;
	while (p != NULL)
	{
		q = p;
		p = p->next;
		free(q);
	}

}
#endif
/*
void ReverseLinklist(linklist h)                //reverse linklist
{
	linklist p, q;
	
	p = h->next;
	h->next = NULL;
	while (p != NULL)
	{
		q = p;
		p = p->next;
		q->next = h->next;
		h->next = q;
	}
}
*/

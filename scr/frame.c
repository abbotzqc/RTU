#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<string.h>
//#include<frame.h>
#include"data_global.h"
#include<sys/types.h>
#include<regex.h>

//#define UPCLIENT 1


extern int counter;

#ifdef UPCLIENT
extern unsigned short rtuaddr;
#else
extern unsigned char rtuaddr;
#endif
extern char rtutype;
extern short rtuvolt;
extern short rtucurr;
extern unsigned char seql;
//extern unsigned char afn;    
/* extern unsigned char bufframe[N_READ]; */
extern unsigned char *bufframe;
extern unsigned char switch_timeint;
extern int timeint[3];
extern unsigned short cap_sd;
extern unsigned short surcap_sd;

extern unsigned char flag_passby;             //PASSBY frame flag

/*****************************
 *	functions used for call
 *****************************/

int ctoi_bcd(unsigned char c)
{
	int i,k,j;
	k=c&0xf0;
	k>>=4;
	j=c&0x0f;
	i=10*k+j;
	return i;
}

unsigned char itoc_bcd(int i)
{
	int j,k,c;
	k=i/10;
	j=i%10;
	c=k;
	c<<=4;
	c+=(unsigned char)j;
	return c;
}

void change_s(unsigned short *ptr)
{
	unsigned char *l,*h;
	unsigned char tmp;
	l=(unsigned char*)ptr;
	h=(unsigned char*)ptr+1;
	tmp=*l;
	*l=*h;
	*h=tmp;
	return;
}
void change_i(unsigned int *ptr)
{
	unsigned char *l,*h;
	unsigned char tmp;
	l=(unsigned char*)ptr;
//	h=
	return;
}

//CRC checking
unsigned short crc(unsigned char *ptr,int bufflen)
{
	unsigned short crc=0xffff;
	int i,j;
	for(i=0;i<bufflen;i++,ptr++)
	{
		crc^=(unsigned short)*ptr;
		for(j=0;j<8;j++)
		{
			if((crc&0x0001)>0)
			{
				crc>>=1;
				crc^=0xa001;
			}
			else crc>>=1;
		}
	}

//	printf("crc : crc result : %x\n",crc);
	return crc;
}


#ifdef UPCLIENT
/*==========================================================================================*/
/*                                    UPclient                                              */
/*===========================================================================================*/

/*********************************
 *	process DOWN frame functions
 *********************************/

char *addhead(char *buf)//,unsigned char afn)
{
	*buf++=0x68;
	*buf++=0x00;
	*buf++=itoc_bcd(rtuaddr);
	*buf++=0x00;
	*buf++=0x00;
	*buf++=0x68;
	return buf;
}

/********************************
 *	process UP frame functions
 *******************************/


int search(char *pattern,unsigned char *buf,frameloc_s *pframeloc,unsigned char *bufr)                  //judge address and crc
{
	int stat;
	char errbuf[1024];
//	char match[100];
	int err,nm=8;
	regex_t reg;
	int cflags=(REG_EXTENDED|REG_ICASE);
	regmatch_t pmatch[nm];
	unsigned short a;

//	printf("search: buf %s\n",buf);
//	printf("search: pattern %s\n",pattern);
	stat=regcomp(&reg,pattern,cflags);                /**/
	/*if(regcomp(&reg,pattern,cflags)<0)*/
	if(stat==0)
	{
		err=regexec(&reg,buf,nm,pmatch,0);
		if(err==0)
		{
			unsigned char addrstr[3]={};
			int addri,i;
			strncpy(addrstr,buf+pmatch[0].rm_so+4,2);
			addri=ctoi_bcd(strtol(addrstr,NULL,16));

			/*
			for(i=0;i<2;i++)                                 //char to hex
			{
				addri*=10*i;                   //XXXXX ought to *=16*i;but address is BCD code              
				if(addrstr[i]>='a'&&addrstr[i]<='f')
					addri+=(addrstr[i]-'a'+10);
				if(addrstr[i]>='A'&&addrstr[i]<='F')
					addri+=(addrstr[i]-'A'+10);
				if(addrstr[i]>='0'&&addrstr[i]<='9')
					addri+=addrstr[i]-'0';
			}
			*/
			//	printf("search:addrstr %s addri %d\n",addrstr,addri);
			if(addri!=rtuaddr&&addri!=BROADCASTADDR)
			{
				printf("                      search : addr wrong\n");
				fflush(stdout);
				regfree(&reg);
				//		bzero(buf,sizeof(buf));		
				return 0;
			}

			if(strncmp(buf+pmatch[0].rm_so+6,"0000",4)!=0&&strncmp(buf+pmatch[0].rm_so+6,"ffff",4)!=0)
			{
				flag_passby=1;
				printf("                      search : PASSBY frame\n");
				fflush(stdout);
				regfree(&reg);
				return 0;
			}

			//	printf("match:\n");
			
			/*
			bzero(bufframe,sizeof(bufframe));
			for(i=pmatch[0].rm_so;i<pmatch[0].rm_eo;++i)                 //char to hex
			{
				int j;
				j=i-pmatch[0].rm_so;
				//		putchar(buf[i]);
				if(buf[i]>='0'&&buf[i]<='9')
					bufframe[j/2]+=buf[i]-'0';
				else if(buf[i]>='a'&&buf[i]<='f')
					bufframe[j/2]+=buf[i]-'a'+0x0a;
				else if(buf[i]>='A'&&buf[i]<='F')
					bufframe[j/2]+=buf[i]-'A'+0x0a;
				if(j%2==0)
					bufframe[j/2]<<=4;
				//		printf("	bufframe[%d]=%x\n",j/2,bufframe[j/2]);
			}
			*/

			bufframe=bufr+(pmatch[0].rm_so>>1);                                    /**/

#ifdef DEBUG
			printf("search : bufframe \n");	
			for(i=0;i<(pmatch[0].rm_eo-pmatch[0].rm_so)/2;i++)
				printf("%x",bufframe[i]);
			printf("\n");
#endif
			pframeloc->phead=buf+pmatch[0].rm_so;
			pframeloc->len=pmatch[0].rm_eo-pmatch[0].rm_so;
		/*	regfree(&reg);*/
			/*
			   change_s((unsigned short*)(bufframe+pframeloc->len/2-2));
			   printf("search :after change_s bufframe \n");	
			   for(i=0;i<(pmatch[0].rm_eo-pmatch[0].rm_so)/2;i++)
			   printf("%x",bufframe[i]);
			   printf("\n");
			   */
			memcpy(&a,bufframe+pframeloc->len/2-2,2);
			//	printf("search:a %x\n",a);
			//	printf("search:crc %x\n",(*(unsigned short*)(bufframe+pframeloc->len/2-2)));

			if(crc(bufframe,(pframeloc->len)/2-2)==a);//(*(unsigned short *)(bufframe+pframeloc->len/2-2)))
			//	printf("search : crc ok!\n");
			else 
			{
				printf("                      search : crc wrong!\n");
				fflush(stdout);
				regfree(&reg);
				return 0;
			}
			regfree(&reg);
			return 1;
		}
		if(err==REG_NOMATCH)
		{
			perror("                      search : no match ");
			regfree(&reg);
			return 0;
		}
		else if(err)
		{
			regerror(err,&reg,errbuf,sizeof(errbuf));
			printf("                      search : err : %s\n",errbuf);
			fflush(stdout);
			regfree(&reg);
			return 0;
		}
	}
	if(stat<0)
	{
		regerror(err,&reg,errbuf,sizeof(errbuf));
		printf("                      search : err:%s\n",errbuf);
		fflush(stdout);
	}

	regfree(&reg);
	return 1;
}

int analysis(char *bufframe,frameinfo_s *pframeinfo)                           //pick up data
{
	char *ptr;
	int seqr;
//	pframeinfo->addr=(short)ctoi_bcd(*(bufframe+1));    //wrong
//	if((*(int*)(bufframe+1)!=LOCALADDR)&&(*(int*)(bufframe+1)!=BROADCASTADDR))   //judge address right or not 

	pframeinfo->addr=ctoi_bcd(*(bufframe+2));
	ptr=bufframe+7;
	seqr=*ptr;
/*	
	if(seqr==seql)				
	{
		printf("seqr repeat(fun analysis)\n");
		return -1;
	}
	else
	{*/
		pframeinfo->seq=seqr;
		ptr++;
		change_s((unsigned short*)ptr);
		pframeinfo->sum=*(unsigned short*)ptr;
		ptr+=2;
		change_s((unsigned short*)ptr);
		pframeinfo->num=*((unsigned short*)ptr);
		ptr+=2;
		change_s((unsigned short*)ptr);
		pframeinfo->datalen=*((unsigned short*)ptr);
		ptr+=2;
		pframeinfo->afn=*ptr++;
		pframeinfo->pdata=ptr;
	//	if(seqr==seql)
	//	{
	//		if(pframeinfo->afn!=0xf1)
	//		{
	//			printf("analysis: seql:%x  seqr:%x  pframeinfo->afn:%x\n",seql,seqr,pframeinfo->afn);
	//			return 0;
	//		}
	//	}
	//	else seql=seqr;
		seql=seqr;
//	}
	return 1;
}
/*
int funafn01t(char *bufdata)//,afn01t_s *pafn)
{
	bufdata=(char *)addhead(bufdata);
	*bufdata++=0x88;
	*bufdata++=seql;
	for(int i=0;i<2;i++)
	{
		*bufdata++=0x01;
		*bufdata++=0x00;
	}
	*bufdata++=0x60;
	*bufdata++=0x00;
	*bufdata++=0x01;				//afn 
	*bufdata++=rtutype;//pafn->rtutype;
	*(short*)bufdata=rtuvolt;//(pafn->rtuvolt);
	change_s((short*)bufdata);
	bufdata+=2;
	*(short*)bufdata=rtucurr;//pafn->rtucurr;
	change_s((short*)bufdata);
	bufdata+=2;
*/	/*
	*(short*)bufdata=pafn->rtuversion;
	change_s((short*)bufdata);
	bufdata+=2;*/
/*	*bufdata++=itoc_bcd(RTUVERSION_M);
	*bufdata++=itoc_bcd(RTUVERSION_S);
	*(unsigned short*)bufdata=crc(ptr,bufdata-ptr);
	change_s((unsigned short*)bufdata);
	return 1;
}
*/

int funafnpass(unsigned char *bufdata,unsigned char flag_reason,msgtoser_s *pmsgbuf)
{
	unsigned char *ptr=bufdata;
	*bufdata++=0x68;
	memcpy(bufdata,pmsgbuf->addr,4);
	bufdata+=4;
	*bufdata++=0x68;
	unsigned char *c=bufdata++;
	if(pmsgbuf->seq==0xff)
		*bufdata++=0;
	else
		*bufdata++=pmsgbuf->seq+1;
	int i;
	for(i=0;i<3;i++)
	{
		*bufdata++=0x01;
		*bufdata++=0x00;
	}
	*bufdata++=pmsgbuf->afn;
	if(pmsgbuf->afn==0xf0||pmsgbuf->afn==0x01)
	{
		*c=0x89;
	}
	else
	{
		*c=0x81;
	}
	*bufdata++=flag_reason;
	*bufdata++=0x06;
	unsigned short b=crc(ptr,bufdata-ptr);
	memcpy(bufdata,&b,2);
	return 1;
}



int funafnbroadcastt(unsigned char *bufdata,afnbroadt_s *pafn)
{
//	printf("client_uphost funafnbroadcastt in\n");
	int i;
    unsigned char *datalen;
	unsigned char *ptr;
	ptr=bufdata;
	bufdata=addhead(bufdata);
	*bufdata++=0x88;
	if(seql==255)
		*bufdata++=0;
	else
		*bufdata++=seql+1;
	for(i=0;i<2;i++)
	{
		*bufdata++=0x01;
		*bufdata++=0x00;
	}
	datalen=bufdata++;
	*bufdata++=0x00;
	*bufdata++=0x01;				//afn 
	*bufdata=rtutype;//pafn->rtutype;
	bufdata++;
	memcpy(bufdata,&rtuvolt,2);
	//*(short*)bufdata=rtuvolt;//(pafn->rtuvolt);
	//change_s((unsigned short*)bufdata);
	bufdata+=2;
	memcpy(bufdata,&rtucurr,2);
	//*(short*)bufdata=rtucurr;//pafn->rtucurr;
	//change_s((unsigned short*)bufdata);
	bufdata+=2;
	
	/*****************
	 * get time
	*/
	time_t t;
	struct tm *tim;
	time(&t);
	tim=localtime(&t);
	*bufdata++=itoc_bcd((tim->tm_year+1900)/100);
	*bufdata++=itoc_bcd((tim->tm_year+1900)%100);
	*bufdata++=itoc_bcd(tim->tm_mon+1);
	*bufdata++=itoc_bcd(tim->tm_mday);
	*bufdata++=itoc_bcd(tim->tm_hour);
	*bufdata++=itoc_bcd(tim->tm_min);
	*bufdata++=itoc_bcd(tim->tm_sec);
	/***************/

	for(i=0;i<3;i++)
	{
		*bufdata=switch_timeint<<(7-i);                              //1 2 3
		*bufdata&=0x80;
		*(bufdata++)+=itoc_bcd(timeint[i]);
	}

//	*bufdata++=itoc_bcd(RTUVERSION_M);
//	*bufdata++=itoc_bcd(RTUVERSION_S);

	*bufdata=RTUVERSION_M;
	bufdata++;
	*bufdata=RTUVERSION_S;
	bufdata++;

//	printf("mutex_sdsize own			%d\n",counter++);
	pthread_mutex_lock(&mutex_sdsize);
	memcpy(bufdata,&cap_sd,2);
//	*(unsigned short*)bufdata=cap_sd;
	bufdata+=2;
	memcpy(bufdata,&surcap_sd,2);
//	*(unsigned short*)bufdata=surcap_sd;
	bufdata+=2;
	pthread_mutex_unlock(&mutex_sdsize);
//	printf("client_uphost funafnbroadcastt:mutex_sdsize free		%d\n",counter++);
	for(i=0;i<N_NODE;i++)
	{
		if(pafn->nodemsg[i].year_rec==0)
		{ 
			int j;
			for(j=0;j<7;j++)
			{
				*bufdata++=0x00;
			}
		}
		else
		{
			memcpy(bufdata,&pafn->nodemsg[i],sizeof(pafn->nodemsg[i]));
			bufdata+=(sizeof(pafn->nodemsg[i])+1);
//			printf("funafnbroadcastt:sizeof(pafn->nodemsg[i])=%d\n",sizeof(pafn->nodemsg[i]));
		}
	}
	*datalen=bufdata-datalen-3;
//	printf("funafnbroadcastt:datalen:%x\n",*datalen);
/*	int k;
	for(k=0;k<N_READ;k++)
	printf("funafnbroadcastt:bufdata[%d]:%x\n",k,bufdata+k);
*/
	*bufdata++=0x06;
	unsigned short b=crc(ptr,bufdata-ptr);
	memcpy(bufdata,&b,2);

//	*bufdata++=0x06;
//	*(unsigned short*)bufdata=crc(ptr,bufdata-ptr);
//	change_s((unsigned short*)bufdata);

	return 1;
}

int funafn02r(afn02r_s *pafn,char *pdata)
{
	pafn->addrall=ctoi_bcd(*pdata++)*10;
	pafn->addrall+=ctoi_bcd(*pdata);
	return 1;
}

int funafn02t(char *bufdata,int flag_reason)
{
	char *ptr;
	ptr=bufdata;
	bufdata=(char *)addhead(bufdata);
	switch(flag_reason)
	{
	case 1:*bufdata++=0x80;
		   if(seql==255)
			   *bufdata++=0;
		   else
			   *bufdata++=seql+1;
		   int i;
		   for(i=0;i<3;i++)
		   {
			   *bufdata++=0x01;
			   *bufdata++=0x00;
		   }
		   *bufdata++=0x02;
		   *bufdata++=0x00;
		   break;
	case -1:break;
	case -2:break;
	}
	*bufdata++=0x06;
	unsigned short b=crc(ptr,bufdata-ptr);
	memcpy(bufdata,&b,2);

//	*bufdata++=0x06;
//	*(unsigned short*)bufdata=crc(ptr,bufdata-ptr);
//	change_s((unsigned short*)bufdata);
	return 1;
}

int funafn04r(afn04r_s *pafn,unsigned char *pdata)
{
	short limit[]={99,99,12,31,59,59,59};        //have bugs
	int *ptr=&pafn->year;
	*ptr=ctoi_bcd(*pdata++)*100;
	*(ptr++)+=ctoi_bcd(*pdata++);
	int i;
	for(i=3;i<7;i++)
	{
		if(ctoi_bcd(*pdata)<=limit[i-1])
		{
			*ptr++=ctoi_bcd(*pdata++);
		}
		else return (-i);
	}
	return 1;
}

int funafn04t(char *bufdata,int flag_reason)
{
	char *ptr;
	ptr=bufdata;
	bufdata=(char *)addhead(bufdata);
	switch(flag_reason)
	{
		case 1:
			*bufdata++=0x80;
			if(seql==255)
				*bufdata++=0;
			else
				*bufdata++=seql+1;
			int i;
			for(i=0;i<3;i++)
			{
				*bufdata++=0x01;
				*bufdata++=0x00;
			}
			*bufdata++=0x04;
			*bufdata++=0x00;
			break;
		case -1:
			break;
		case -2:
			break;
	}
	
	*bufdata++=0x06;
	unsigned short b=crc(ptr,bufdata-ptr);
	memcpy(bufdata,&b,2);

//	*bufdata++=0x06;
//	*(unsigned short*)bufdata=crc(ptr,bufdata-ptr);
//	change_s((unsigned short *)bufdata);
	return 1;
}

int funafn05r(afn05r_s *pafn,char *pdata)
{
	pafn->nodeno=*pdata++;
	pafn->sensorno=*pdata++;
	pafn->year=ctoi_bcd(*pdata++)*100;
	pafn->year+=ctoi_bcd(*pdata++);
	pafn->month=ctoi_bcd(*pdata++);
	pafn->day=ctoi_bcd(*pdata++);
	pafn->hour=ctoi_bcd(*pdata++);
//	pafn->timelong=(*pdata);
	return 1;
}

int funafn05t(unsigned char *bufdata,afn05t_s *pafn)
{
	unsigned char *ptr;
	int i;
	ptr=bufdata;
	bufdata=(unsigned char *)addhead(bufdata);
	*bufdata++=0x88;
	if(seql==255)
		*bufdata++=0;
	else
		*bufdata++=seql+1;
	for(i=0;i<2;i++)
	{
		*bufdata++=0x01;
		*bufdata++=0x00;
	}
	unsigned short b;
	b=pafn->recordsum+((pafn->recordsum)<<3)+1;
	memcpy(bufdata,&b,2);
//	*((unsigned short*)bufdata)=pafn->recordsum*9+1;
	bufdata+=2;
	*bufdata++=0x05;
	*bufdata++=pafn->recordsum;
	for(i=0;i<pafn->recordsum;i++)
	{
		*bufdata++=(pafn->pafn05+i)->nodeno;
		*bufdata++=(pafn->pafn05+i)->sensorno;
		*bufdata++=itoc_bcd(pafn->year/100);
		*bufdata++=itoc_bcd(pafn->year%100);
		*bufdata++=itoc_bcd(pafn->month);
		*bufdata++=itoc_bcd(pafn->day);
//		*bufdata++=itoc_bcd(pafn->hour);
		*bufdata++=(pafn->pafn05+i)->hour;
		*bufdata++=(pafn->pafn05+i)->min;
		*bufdata++=(pafn->pafn05+i)->sec;
	}
	*bufdata++=0x06;

	b=crc(ptr,bufdata-ptr);
	memcpy(bufdata,&b,2);
	
//	*((unsigned short*)bufdata)=b;//crc(ptr,bufdata-ptr);
//	printf("xx%x\n",b);
	return 1;
}

int funafn06r(afn06r_s *pafn,char *pdata)
{
	pafn->nodeno=*pdata++;
	pafn->sensorno=*pdata++;
	pafn->year=ctoi_bcd(*pdata++)*100;
	pafn->year+=ctoi_bcd(*pdata++);
	pafn->month=ctoi_bcd(*pdata++);
	pafn->day=ctoi_bcd(*pdata++);
	pafn->hour=ctoi_bcd(*pdata++);
	pafn->min=ctoi_bcd(*pdata++);
	pafn->sec=ctoi_bcd(*pdata++);
	return 1;
}

int funafn06t(unsigned char *bufdata,afn06t_s *pafn,unsigned char flag_reason)
{
	unsigned char *ptr;
	int i;
	ptr=bufdata;
	bufdata=(unsigned char *)addhead(bufdata);
	if(flag_reason==0)
	{
		*bufdata++=0x88;
		*bufdata++=seql;
		*bufdata++=0x20;
		*bufdata++=0x00;
		*(unsigned short*)bufdata=pafn->frameno;
		bufdata+=2;
		*bufdata++=0x00;
		*bufdata++=0x04;
		*bufdata++=0x06;
		memcpy(bufdata,pafn->data,1024);
		bufdata+=1024;
	}
	else
	{
		*bufdata++=0x89;
		*bufdata++=seql;
		for(i=0;i<3;i++)
		{
			*bufdata++=0x01;
			*bufdata++=0x00;
		}
		*bufdata++=0x06;
		*bufdata++=flag_reason;
	}
		*bufdata++=0x06;
	unsigned short b=crc(ptr,bufdata-ptr);
	memcpy(bufdata,&b,2);
//	*bufdata++=0x06;
//		*(unsigned short*)bufdata=crc(ptr,bufdata-ptr);
		//	change_s((unsigned short *)bufdata);
	return 1;
}
	
int funafnf1r(afnf1r_s *pafn,char *pdata)
{
	pafn->frameno=*pdata;
	return 1;
}

int funafnf1t(char *bufdata,afnf1t_s *pafn)
{
	char *ptr;
	int i;
	ptr=bufdata;
	bufdata=(char *)addhead(bufdata);
	*bufdata++=0x88;
	if(seql==255)
		*bufdata++=0;
	else
		*bufdata++=seql+1;
	*bufdata++=0x20;
	*bufdata++=0x00;
	*(unsigned short*)bufdata=pafn->frameno;
	bufdata+=2;
	*bufdata++=0x00;
	*bufdata++=0x40;
	*bufdata++=0x06;
	strcpy(bufdata,pafn->data);
	bufdata+=1024;
	*bufdata++=0x06;
	unsigned short b=crc(ptr,bufdata-ptr);
	memcpy(bufdata,&b,2);
//*bufdata++=0x06;
//	*(unsigned short*)bufdata=crc(ptr,bufdata-ptr);
//	change_s((unsigned short *)bufdata);
return 1;
}

int funafnf3r(unsigned char *pdata)
{
	int i;
	switch_timeint=0;
	for(i=0;i<3;i++)
	{
		switch_timeint+=(((*pdata)>>7)<<i);
		timeint[i]=ctoi_bcd((*pdata++)&0x7f);
	}
	return 1;
}

int funafnf3t(char *bufdata,int flag_reason)
{
	char *ptr;
	ptr=bufdata;
	bufdata=(char *)addhead(bufdata);
	switch(flag_reason)
	{
	case 1:*bufdata++=0x80;
		   if(seql==255)
			   *bufdata++=0;
		   else
			   *bufdata++=seql+1;
		   int i;
		   for(i=0;i<3;i++)
		   {
			   *bufdata++=0x01;
			   *bufdata++=0x00;
		   }
		   *bufdata++=0xf3;
		   *bufdata++=0x00;
		   break;
	case -1:break;
	case -2:break;
	}
	*bufdata++=0x06;
	unsigned short b=crc(ptr,bufdata-ptr);
	memcpy(bufdata,&b,2);

//	*bufdata++=0x06;
//	*(unsigned short*)bufdata=crc(ptr,bufdata-ptr);
//	change_s((unsigned short*)bufdata);
	return 1;
}

/*********************************
 *	function of call communication protocol
 *********************************/
/*
int frame(int fd_data)
{
	msg_s msgbuf;
	char flag_dataat;
	frameloc_s frameloc;
	frameinfo_s frameinfo;
	char *pattern="PATTERN";
	char bufframe[N_FRAME];
	strcat(buf,bufr);
	if(search(pattern,buf,&frameloc)==0)
	{
		printf("frame_re:no data\n");
		return 0;
	}
	strncpy(bufframe,frameloc.phead,frameloc.len);
	strcpy(buf,frameloc.phead);
	if(analysis(bufframe,&frameinfo)<=0) 
	{
		printf("frame_re:analysis wrong\n");
	}
	switch(frameinfo.afn)
	{
	case 0x10:	afn01r_s afn01r;
				funafn01r(afn01r);

				break;
	case 0x02:	afn02r_s afn02r;
				funafn02r(afn02r);
				
				break;
	case 0x04:  afn04r_s afn04r;

				break;
	case 0xf4: 	afnf4r_s afnf4r;
				afnf4t_s afnf4t;
				strcpy(msgbuf.buftdown,pdata);
				msgbuf.msgtype=MSGTYPE;
				msgsnd(msgid,&msgbuf,sizeof(msgbuf)-sizeof(long),0);
				flag_dataat=MSG;
				addhead(*bufdata);
				break;
	}
*/

#else
/*======================================================================================================*/
/*                                            Server                                                    */
/*======================================================================================================*/
#include "linklist.h"

extern nodeprivate_s nodeprivate[3];

int searchser(unsigned char nodeno,unsigned char *pattern,unsigned char *buf,regmatch_t *pmatch,int nmatch,unsigned char seqs,unsigned char flag_log)                           //judge address and crc and seq
{
	int i;
	char errbuf[1024]={};
	unsigned char seq[3]={};
	int err;
	regex_t reg;
	int cflags=(REG_EXTENDED|REG_ICASE);
//	printf("search: buf %s\n",buf);
//	printf("search: pattern %s\n",pattern);
//	printf("searchser start\n");
//	fflush(stdout);
	

	if((err=regcomp(&reg,pattern,cflags))<0)
	{
		regerror(err,&reg,errbuf,sizeof(errbuf));
	//	printf("                      node %d searchser : err--%s\n",nodeno,errbuf);
	//	fflush(stdout);
		regfree(&reg);
		return 0;
	}
	err=regexec(&reg,buf,nmatch,pmatch,0);

	if(err==REG_NOMATCH)
	{
	//	printf("                      node %d searchser : no match\n",nodeno);
	//	fflush(stdout);
		regfree(&reg);
		return 0;
	}
	else if(err)
	{
		regerror(err,&reg,errbuf,sizeof(errbuf));
	//	printf("                      node %d searchser : err : %s\n",nodeno,errbuf);
	//	fflush(stdout);
		regfree(&reg);
		return 0;
	}
	if(flag_log!=0)              /**/
	{
		unsigned char addr[3]={};
		memcpy(addr,buf+pmatch[0].rm_so+6,2);
		if(nodeno!=strtol(addr,NULL,16))
		{
		//	printf("                      node %d searchser : node addr wrong\n",nodeno);
		//	fflush(stdout);
			regfree(&reg);
			return -1;
		}
	}
	
	memcpy(seq,buf+pmatch[0].rm_so+14,2);
	if(seqs!=strtol(seq,NULL,16))
	{
//		printf("          seqs %02x\n",seqs);
	//	printf("                      node %d searchser : seqs wrong\n",nodeno);
	//	fflush(stdout);
		regfree(&reg);
		return -1;
	}
	unsigned char bufframe[1043];
//	printf("match:\n");
	bzero(bufframe,sizeof(bufframe));
	for(i=pmatch[0].rm_so;i<pmatch[0].rm_eo;++i)                 //char to hex
	{
		int j;
		j=i-pmatch[0].rm_so;
	//	bufframe[j>>1]=strtol(buf+i,NULL,16);
//		putchar(buf[i]);
		if(buf[i]>='0'&&buf[i]<='9')
			bufframe[j/2]+=buf[i]-'0';
		else if(buf[i]>='a'&&buf[i]<='f')
			bufframe[j/2]+=buf[i]-'a'+0x0a;
		else if(buf[i]>='A'&&buf[i]<='F')
			bufframe[j/2]+=buf[i]-'A'+0x0a;
		if(j%2==0)
			bufframe[j/2]<<=4;
//		printf("	bufframe[%d]=%x\n",j/2,bufframe[j/2]);
	}

	regfree(&reg);
/*
	change_s((unsigned short*)(bufframe+pframeloc->len/2-2));
	printf("search :after change_s bufframe \n");	
	for(i=0;i<(pmatch[0].rm_eo-pmatch[0].rm_so)/2;i++)
		printf("%x",bufframe[i]);
	printf("\n");
*/
	unsigned short a;
	memcpy(&a,bufframe+(pmatch[0].rm_eo-pmatch[0].rm_so)/2-2,2);
//	printf("%02x\n",a);
//	printf("search:a %x\n",a);
//	printf("search:crc %x\n",(*(unsigned short*)(bufframe+pframeloc->len/2-2)));

	if(crc(bufframe,(pmatch[0].rm_eo-pmatch[0].rm_so)/2-2)==a)//(*(unsigned short *)(bufframe+pframeloc->len/2-2)))
	{
#ifdef DEBUG
		printf("                       node_%d searchser : crc ok!\n",nodeno);
#endif
	}
	else 
	{
	//	printf("                      node %d searchser : crc wrong!\n",nodeno);
	//	fflush(stdout);
		return -1;
	}
	return 1;
}


/*************************************
 * funafn used for server 
 ************************************/
int funafnbroadcastser(afnbroadtser_s *pafn,unsigned char *pdata)              //receive
{
	memcpy(pafn,pdata,sizeof(afnbroadtser_s));
	return 1;
}

int funafn04tser(unsigned char *buft,unsigned char nodeno,unsigned char seqs)
{
	unsigned char *ptr;
	ptr=buft;
	struct tm *tim;
	time_t t;

	*buft++=0x68;
	*buft++=0x00;
	*buft++=rtuaddr;
	*buft++=nodeprivate[nodeno-1].node_addr;
	*buft++=0x00;
	*buft++=0x68;
	*buft++=0x41;
	*buft++=seqs;             //one node one seql
	*buft++=0x01;
	*buft++=0x00;
	*buft++=0x01;
	*buft++=0x00;
	*buft++=0x07;
	*buft++=0x00;
	*buft++=0x04;
	time(&t);
	tim=localtime(&t);
	*buft++=itoc_bcd((tim->tm_year+1900)/100);
	*buft++=itoc_bcd((tim->tm_year+1900)%100);
	*buft++=itoc_bcd(tim->tm_mon+1);
	*buft++=itoc_bcd(tim->tm_mday);
	*buft++=itoc_bcd(tim->tm_hour);
	*buft++=itoc_bcd(tim->tm_min);
	*buft++=itoc_bcd(tim->tm_sec);
	*buft++=0x06;
	unsigned short b=crc(ptr,buft-ptr);
	memcpy(buft,&b,2);
//	*buft++=0x06;
//	*(unsigned short*)buft=crc(ptr,buft-ptr);
	return 1;
}

int funafnf0tser(unsigned char *buft,unsigned char nodeaddr,unsigned char seqs,unsigned char sensorno)
{
	unsigned char *ptr;
	ptr=buft;
	*buft++=0x68;
	*buft++=0x00;
	*buft++=rtuaddr;
	*buft++=nodeaddr;//nodeprivate[nodeno-1].node_addr;            
	*buft++=0x00;
	*buft++=0x68;
	*buft++=0x43;
	*buft++=seqs;             //one node one seql
	*buft++=0x01;
	*buft++=0x00;
	*buft++=0x01;
	*buft++=0x00;
	*buft++=0x01;
	*buft++=0x00;
	*buft++=0xf0;
	*buft++=sensorno;
	*buft++=0x06;
	unsigned short b=crc(ptr,buft-ptr);
	memcpy(buft,&b,2);
//*buft++=0x06;
//	*(unsigned short*)buft=crc(ptr,buft-ptr);
	return 1;
}

int funafnf1tser(unsigned char *buft,unsigned char nodeaddr,unsigned char seqs,unsigned short which)//,unsigned char sensorno)
{
	unsigned char *ptr;
	ptr=buft;
	*buft++=0x68;
	*buft++=0x00;
	*buft++=rtuaddr;
	*buft++=nodeaddr;//nodeprivate[nodeno-1].node_addr;
	*buft++=0;//sensorno;
	*buft++=0x68;
	*buft++=0x42;
	*buft++=seqs;             //one node one seql
	*buft++=0x01;
	*buft++=0x00;
	*buft++=0x01;
	*buft++=0x00;
	*buft++=0x02;
	*buft++=0x00;
	*buft++=0xf1;
	memcpy(buft,&which,2);
	buft+=2;
	*buft++=0x06;
	unsigned short b=crc(ptr,buft-ptr);
	memcpy(buft,&b,2);
//	*buft++=0x06;
//	*(unsigned short*)buft=crc(ptr,buft-ptr);
	return 1;
}

int funbusy(msgtoser_s *msgbuf,msgtocli_s *msgbuff)
{
	unsigned char *buf=msgbuff->buf;
	int i;
	*buf++=0x68;
	memcpy(buf,msgbuf->addr,4);
	buf+=4;
	*buf++=0x68;
	if(msgbuf->afn==0xf0||msgbuf->afn==0x01)
		*buf++=0x89;
	else
		*buf++=0x81;
	if(msgbuf->seq==0xff)
		*buf++=0;
	else
		*buf++=msgbuf->seq+1;
	for(i=0;i<3;i++)
	{
		*buf++=0x01;
		*buf++=0;
	}
	*buf++=msgbuf->afn;
	*buf++=ERR_NODEBUSY;
	*buf++=0x06;
	unsigned short b=crc(msgbuff->buf,buf-msgbuff->buf);
	memcpy(buf,&b,2);
	return 1;
}

int funoutline(msgtoser_s *msgbuf,msgtocli_s *msgbuff)
{
	unsigned char *buf=msgbuff->buf;
	int i;
	*buf++=0x68;
	memcpy(buf,msgbuf->addr,4);
	buf+=4;
	*buf++=0x68;
	if(msgbuf->afn==0xf0||msgbuf->afn==0x01)
		*buf++=0x89;
	else
		*buf++=0x81;
	if(msgbuf->seq==0xff)
		*buf++=0;
	else
		*buf++=msgbuf->seq+1;
	for(i=0;i<3;i++)
	{
		*buf++=0x01;
		*buf++=0;
	}
	*buf++=msgbuf->afn;
	*buf++=0xe3;
	*buf++=0x06;
	unsigned short b=crc(msgbuff->buf,buf-msgbuff->buf);
	memcpy(buf,&b,2);
	return 1;
}

int funouttime(msgtocli_s *msgbuf,unsigned char *addr,unsigned char seq,unsigned char afn)
{
	unsigned char *buf=msgbuf->buf;
	*buf++=0x68;
#ifdef DEBUG
	printf("1077 %02x\n",addr[0]);
#endif
	int i;
	for(i=0;i<4;i++)
		*buf++=addr[i];
	//memcpy(buf,addr,4);
	//buf+=4;
	*buf++=0x68;
	if(afn==0xf0||afn==0x01)
		*buf++=0x89;
	else
		*buf++=0x81;
	if(seq==255)
		*buf++=0;
	else
		*buf++=seq+1;
	for(i=0;i<3;i++)
	{
		*buf++=0x01;
		*buf++=0;
	}
	*buf++=afn;
	*buf++=0xe5;
	*buf++=0x06;
	unsigned short b=crc(msgbuf->buf,buf-msgbuf->buf);
	memcpy(buf,&b,2);
	return 1;
}

int funsndfail(msgtocli_s *msgbuf,unsigned char *addr,unsigned char seq,unsigned char afn)
{
	unsigned char *buf=msgbuf->buf;
	*buf++=0x68;
#ifdef DEBUG
	printf("1077 %02x\n",addr[0]);
#endif
	int i;
	for(i=0;i<4;i++)
		*buf++=addr[i];
	//memcpy(buf,addr,4);
	//buf+=4;
	*buf++=0x68;
	if(afn==0xf0||afn==0x01)
		*buf++=0x89;
	else
		*buf++=0x81;
	if(seq==255)
		*buf++=0;
	else
		*buf++=seq+1;
	for(i=0;i<3;i++)
	{
		*buf++=0x01;
		*buf++=0;
	}
	*buf++=afn;
	*buf++=ERR_PASS;
	*buf++=0x06;
	unsigned short b=crc(msgbuf->buf,buf-msgbuf->buf);
	memcpy(buf,&b,2);
	return 1;
}


#endif

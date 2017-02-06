//DNS Query Program on Linux

//Header Files
#include<stdio.h> //printf
#include<string.h>    //strlen
#include<stdlib.h>    //malloc
#include<sys/socket.h>    //you know what this is for
#include<arpa/inet.h> //inet_addr , inet_ntoa , ntohs etc
#include<netinet/in.h>
#include<unistd.h>    //getpid
#include <ctime>
#include <stdlib.h>
#include <math.h>
#include <mysql.h>

//----------------------------TRAFIC ENGINEERING LAB @ UNIVERSITY OF TEXAS AT DALLAS-------------------------------------
const unsigned char GREETING[]=
        "████████╗██████╗  █████╗ ███████╗███████╗██╗ ██████╗\n"
        "╚══██╔══╝██╔══██╗██╔══██╗██╔════╝██╔════╝██║██╔════╝\n"
        "   ██║   ██████╔╝███████║█████╗  █████╗  ██║██║     \n"
        "   ██║   ██╔══██╗██╔══██║██╔══╝  ██╔══╝  ██║██║     \n"
        "   ██║   ██║  ██║██║  ██║██║     ██║     ██║╚██████╗\n"
        "   ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝     ╚═╝     ╚═╝ ╚═════╝\n"
        "                                                     \n"
        "███████╗███╗   ██╗ ██████╗        ██╗      █████╗ ██████╗\n"
        "██╔════╝████╗  ██║██╔════╝        ██║     ██╔══██╗██╔══██╗\n"
        "█████╗  ██╔██╗ ██║██║  ███╗       ██║     ███████║██████╔╝\n"
        "██╔══╝  ██║╚██╗██║██║   ██║       ██║     ██╔══██║██╔══██╗\n"
        "███████╗██║ ╚████║╚██████╔╝██╗    ███████╗██║  ██║██████╔╝\n"
        "╚══════╝╚═╝  ╚═══╝ ╚═════╝ ╚═╝    ╚══════╝╚═╝  ╚═╝╚═════╝\n" ;

//-------------------------------------------------------------------------------------------------------------------------

//List of DNS Servers registered on the system
char dns_servers[10][100];
int dns_server_count = 0;
//Types of DNS resource records :)

#define T_A 1 //Ipv4 address
#define T_NS 2 //Nameserver
#define T_CNAME 5 // canonical name
#define T_SOA 6 /* start of authority zone */
#define T_PTR 12 /* domain name pointer */
#define T_MX 15 //Mail server

//Function Prototypes
void ngethostbyname (unsigned char* , int);
void ChangetoDnsNameFormat (unsigned char*,unsigned char*);
unsigned char* ReadName (unsigned char*,unsigned char*,int*);
double calcVar(double* , int , double );
double calcMean(double* ,int );
void get_dns_servers();
int dns_Table_Exist(MYSQL* );
int dns_Table_Create(MYSQL* );
int dns_Table_Insert(MYSQL* ,int site_, float , float , float , float );


//DNS header structure
struct DNS_HEADER
{
    unsigned short id; // identification number

    unsigned char rd :1; // recursion desired
    unsigned char tc :1; // truncated message
    unsigned char aa :1; // authoritive answer
    unsigned char opcode :4; // purpose of message
    unsigned char qr :1; // query/response flag

    unsigned char rcode :4; // response code
    unsigned char cd :1; // checking disabled
    unsigned char ad :1; // authenticated data
    unsigned char z :1; // its z! reserved
    unsigned char ra :1; // recursion available

    unsigned short q_count; // number of question entries
    unsigned short ans_count; // number of answer entries
    unsigned short auth_count; // number of authority entries
    unsigned short add_count; // number of resource entries
};

//Constant sized fields of query structure
struct QUESTION
{
    unsigned short qtype;
    unsigned short qclass;
};

//Constant sized fields of the resource record structure
#pragma pack(push, 1)
struct R_DATA
{
    unsigned short type;
    unsigned short _class;
    unsigned int ttl;
    unsigned short data_len;
};
#pragma pack(pop)

//Pointers to resource record contents
struct RES_RECORD
{
    unsigned char *name;
    struct R_DATA *resource;
    unsigned char *rdata;
};

//Structure of a Query
typedef struct
{
    unsigned char *name;
    struct QUESTION *ques;
} QUERY;

int main( int argc , char *argv[])
{
	///////////////////CONSTANTS
    const int HOST_COUNT = 10;
    const int HOST_NAME_LEN = 20;
    const int MAX_FREQ = 100;
	///////////////////DNS_VARS
    int Frequency ;
    int i,j;
    clock_t t1, t2,start_time, end_time;
    double elapsedTime[MAX_FREQ], mean, var;
    double PER_MIL = ((double)CLOCKS_PER_SEC)/1000.0;
    unsigned char hostnames[HOST_COUNT][HOST_NAME_LEN]={"www.google.com\0","www.facebook.com\0","www.youtube.com\0"
                              ,"www.yahoo.com\0","www.live.com\0","www.wikipedia.com\0",
                              "www.baidu.com\0","www.bloger.com\0","www.msn.com\0","www.gg.com\0"};
	///////////////////MYSQL_VARS
	MYSQL *conn;
	MYSQL_RES *res;
	MYSQL_ROW row;
	char server[] = "localhost";
	char user[] = "root";
	char password[] = "dallas7573"; /* set me first */
	char database[] = "mysql";
	conn = mysql_init(NULL);
 	//GREETINGS
    printf("%s",GREETING);
    usleep(1E6);
    if(argc>1)
    {
        Frequency = atoi(argv[2]);
        if(Frequency>MAX_FREQ)
            Frequency= MAX_FREQ;
    }
    else
        Frequency = 20; //By default if it is not specified


	/* Connect to database */
	printf("connecting to database........");	
	if (!mysql_real_connect(conn, server,
		 user, password, database, 0, NULL, 0)) {
	  fprintf(stderr, "\n%s\n", mysql_error(conn));
	  return 1;
	}
	printf("done\n");
	//Check database tables
	printf("Checking database tables.....");
	if(dns_Table_Exist(conn))
		printf("done\n");
	else
	{
		dns_Table_Create(conn);
		printf("Created\n");
	}
	
    //Get the DNS servers from the resolv.conf file
	printf("getting list of dns servers.....");    
	get_dns_servers();
	printf("done\n");
	
    usleep(1E6);

    for(i=0;i<HOST_COUNT;i++)
    {
        printf("%s\n",hostnames[i]);
        start_time = clock();
        printf("Start @ : %f ms\n",(double)start_time/(PER_MIL));
        for(j=0;j<Frequency;j++)
        {
            t1=clock();
            ngethostbyname(hostnames[i],T_A);
            t2=clock();
            elapsedTime[j] = (double)(t2 - t1);
        }
        end_time = clock();
        printf("End @ : %f ms\n",(double)end_time/PER_MIL);
        mean= calcMean(elapsedTime,Frequency);
        var = calcVar(elapsedTime,Frequency,mean);
		dns_Table_Insert(conn ,i, mean , var , start_time , end_time );
        printf("Average Delay: %.4f ms and Variance is: %.4f ms \n",mean/PER_MIL,sqrt(var)/PER_MIL);
        printf("--------------------------------------------\n");
    }

	///FREE MYSQL
	mysql_free_result(res);
	mysql_close(conn);
    return 0;
}

double calcMean(double* Array,int Length )
{
     double sum = 0;
     for (int i=0;i<Length;i++)
     {
        sum+=Array[i];
     }
     return sum / (double)Length;

}

double calcVar(double* Array, int Length, double Mean)
{
    double sum = 0;
    for (int i=0;i<Length;i++)
    {
        sum+=(Array[i]-Mean)*(Array[i]-Mean);
    }
    return sum/((double)(Length-1));
}

/*
 * Perform a DNS query by sending a packet
 * */
void ngethostbyname(unsigned char *host , int query_type)
{
    unsigned char buf[65536],*qname,*reader;
    int i , j , stop , s;

    struct sockaddr_in a;

    struct RES_RECORD answers[20],auth[20],addit[20]; //the replies from the DNS server
    struct sockaddr_in dest;

    struct DNS_HEADER *dns = NULL;
    struct QUESTION *qinfo = NULL;
#ifdef VEROBOSE
    printf("Resolving %s" , host);
#endif
    s = socket(AF_INET , SOCK_DGRAM , IPPROTO_UDP); //UDP packet for DNS queries

    dest.sin_family = AF_INET;
    dest.sin_port = htons(53);
    dest.sin_addr.s_addr = inet_addr(dns_servers[0]); //dns servers

    //Set the DNS structure to standard queries
    dns = (struct DNS_HEADER *)&buf;

    dns->id = (unsigned short) htons(getpid());
    dns->qr = 0; //This is a query
    dns->opcode = 0; //This is a standard query
    dns->aa = 0; //Not Authoritative
    dns->tc = 0; //This message is not truncated
    dns->rd = 1; //Recursion Desired
    dns->ra = 0; //Recursion not available! hey we dont have it (lol)
    dns->z = 0;
    dns->ad = 0;
    dns->cd = 0;
    dns->rcode = 0;
    dns->q_count = htons(1); //we have only 1 question
    dns->ans_count = 0;
    dns->auth_count = 0;
    dns->add_count = 0;

    //point to the query portion
    qname =(unsigned char*)&buf[sizeof(struct DNS_HEADER)];

    ChangetoDnsNameFormat(qname , host);
    qinfo =(struct QUESTION*)&buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1)]; //fill it

    qinfo->qtype = htons( query_type ); //type of the query , A , MX , CNAME , NS etc
    qinfo->qclass = htons(1); //its internet (lol)
#ifdef VEROBOSE
    printf("\nSending Packet...");
#endif

    if( sendto(s,(char*)buf,sizeof(struct DNS_HEADER) + (strlen((const char*)qname)+1) + sizeof(struct QUESTION),0,(struct sockaddr*)&dest,sizeof(dest)) < 0)
    {
        perror("sendto failed");
    }
    #ifdef VEROBOSE
    printf("Done");
#endif

    //Receive the answer
    i = sizeof dest;
#ifdef VEROBOSE
    printf("\nReceiving answer...");
#endif
    if(recvfrom (s,(char*)buf , 65536 , 0 , (struct sockaddr*)&dest , (socklen_t*)&i ) < 0)
    {
        perror("recvfrom failed");
    }
#ifdef VEROBOSE
    printf("Done");
#endif

    dns = (struct DNS_HEADER*) buf;

    //move ahead of the dns header and the query field
    reader = &buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname)+1) + sizeof(struct QUESTION)];
#ifdef VEROBOSE
    printf("\nThe response contains : ");
    printf("\n %d Questions.",ntohs(dns->q_count));
    printf("\n %d Answers.",ntohs(dns->ans_count));
    printf("\n %d Authoritative Servers.",ntohs(dns->auth_count));
    printf("\n %d Additional records.\n\n",ntohs(dns->add_count));
#endif
    //Start reading answers
    stop=0;

    for(i=0;i<ntohs(dns->ans_count);i++)
    {
        answers[i].name=ReadName(reader,buf,&stop);
        reader = reader + stop;

        answers[i].resource = (struct R_DATA*)(reader);
        reader = reader + sizeof(struct R_DATA);

        if(ntohs(answers[i].resource->type) == 1) //if its an ipv4 address
        {
            answers[i].rdata = (unsigned char*)malloc(ntohs(answers[i].resource->data_len));

            for(j=0 ; j<ntohs(answers[i].resource->data_len) ; j++)
            {
                answers[i].rdata[j]=reader[j];
            }

            answers[i].rdata[ntohs(answers[i].resource->data_len)] = '\0';

            reader = reader + ntohs(answers[i].resource->data_len);
        }
        else
        {
            answers[i].rdata = ReadName(reader,buf,&stop);
            reader = reader + stop;
        }
    }

    //read authorities
    for(i=0;i<ntohs(dns->auth_count);i++)
    {
        auth[i].name=ReadName(reader,buf,&stop);
        reader+=stop;

        auth[i].resource=(struct R_DATA*)(reader);
        reader+=sizeof(struct R_DATA);

        auth[i].rdata=ReadName(reader,buf,&stop);
        reader+=stop;
    }

    //read additional
    for(i=0;i<ntohs(dns->add_count);i++)
    {
        addit[i].name=ReadName(reader,buf,&stop);
        reader+=stop;

        addit[i].resource=(struct R_DATA*)(reader);
        reader+=sizeof(struct R_DATA);

        if(ntohs(addit[i].resource->type)==1)
        {
            addit[i].rdata = (unsigned char*)malloc(ntohs(addit[i].resource->data_len));
            for(j=0;j<ntohs(addit[i].resource->data_len);j++)
            addit[i].rdata[j]=reader[j];

            addit[i].rdata[ntohs(addit[i].resource->data_len)]='\0';
            reader+=ntohs(addit[i].resource->data_len);
        }
        else
        {
            addit[i].rdata=ReadName(reader,buf,&stop);
            reader+=stop;
        }
    }

    //print answers
 #ifdef VEROBOSE
    printf("\nAnswer Records : %d \n" , ntohs(dns->ans_count) );
#endif
    for(i=0 ; i < ntohs(dns->ans_count) ; i++)
    {
#ifdef VEROBOSE
        printf("Name : %s ",answers[i].name);
#endif
        if( ntohs(answers[i].resource->type) == T_A) //IPv4 address
        {
            long *p;
            p=(long*)answers[i].rdata;
            a.sin_addr.s_addr=(*p); //working without ntohl
 #ifdef VEROBOSE
            printf("has IPv4 address : %s",inet_ntoa(a.sin_addr));
#endif
        }

        if(ntohs(answers[i].resource->type)==5)
        {
            //Canonical name for an alias
#ifdef VEROBOSE
            printf("has alias name : %s",answers[i].rdata);
#endif
        }
#ifdef VEROBOSE
        printf("\n");
#endif
    }

    //print authorities
#ifdef VEROBOSE
    printf("\nAuthoritive Records : %d \n" , ntohs(dns->auth_count) );
#endif
    for( i=0 ; i < ntohs(dns->auth_count) ; i++)
    {
        #ifdef VEROBOSE
        printf("Name : %s ",auth[i].name);
        #endif
        if(ntohs(auth[i].resource->type)==2)
        {
            #ifdef VEROBOSE
            printf("has nameserver : %s",auth[i].rdata);
            #endif
        }
#ifdef VEROBOSE
        printf("\n");
#endif
    }

    //print additional resource records
  #ifdef VEROBOSE
    printf("\nAdditional Records : %d \n" , ntohs(dns->add_count) );
#endif
    for(i=0; i < ntohs(dns->add_count) ; i++)
    {
        #ifdef VEROBOSE
        printf("Name : %s ",addit[i].name);
        #endif
        if(ntohs(addit[i].resource->type)==1)
        {
            long *p;
            p=(long*)addit[i].rdata;
            a.sin_addr.s_addr=(*p);
            #ifdef VEROBOSE
            printf("has IPv4 address : %s",inet_ntoa(a.sin_addr));
            #endif
        }
        #ifdef VEROBOSE
        printf("\n");
        #endif
    }
    return;
}

/*
 *
 * */
u_char* ReadName(unsigned char* reader,unsigned char* buffer,int* count)
{
    unsigned char *name;
    unsigned int p=0,jumped=0,offset;
    int i , j;

    *count = 1;
    name = (unsigned char*)malloc(256);

    name[0]='\0';

    //read the names in 3www6google3com format
    while(*reader!=0)
    {
        if(*reader>=192)
        {
            offset = (*reader)*256 + *(reader+1) - 49152; //49152 = 11000000 00000000 ;)
            reader = buffer + offset - 1;
            jumped = 1; //we have jumped to another location so counting wont go up!
        }
        else
        {
            name[p++]=*reader;
        }

        reader = reader+1;

        if(jumped==0)
        {
            *count = *count + 1; //if we havent jumped to another location then we can count up
        }
    }

    name[p]='\0'; //string complete
    if(jumped==1)
    {
        *count = *count + 1; //number of steps we actually moved forward in the packet
    }

    //now convert 3www6google3com0 to www.google.com
    for(i=0;i<(int)strlen((const char*)name);i++)
    {
        p=name[i];
        for(j=0;j<(int)p;j++)
        {
            name[i]=name[i+1];
            i=i+1;
        }
        name[i]='.';
    }
    name[i-1]='\0'; //remove the last dot
    return name;
}

/*
 * Get the DNS servers from /etc/resolv.conf file on Linux
 * */
void get_dns_servers()
{
    FILE *fp;
    char line[200] , *p;
    if((fp = fopen("/etc/resolv.conf" , "r")) == NULL)
    {
        printf("Failed opening /etc/resolv.conf file \n");
    }

    while(fgets(line , 200 , fp))
    {
        if(line[0] == '#')
        {
            continue;
        }
        if(strncmp(line , "nameserver" , 10) == 0)
        {
            p = strtok(line , " ");
            p = strtok(NULL , " ");

            //p now is the dns ip :)
            //????
        }
    }

    strcpy(dns_servers[0] , "208.67.222.222");
    strcpy(dns_servers[1] , "208.67.220.220");
}

/*
 * This will convert www.google.com to 3www6google3com
 * got it :)
 * */
void ChangetoDnsNameFormat(unsigned char* dns,unsigned char* host)
{
    int lock = 0 , i;
    strcat((char*)host,".");

    for(i = 0 ; i < strlen((char*)host) ; i++)
    {
        if(host[i]=='.')
        {
            *dns++ = i-lock;
            for(;lock<i;lock++)
            {
                *dns++=host[lock];
            }
            lock++; //or lock=i+1;
        }
    }
    *dns++='\0';
}

/*************************************************************
******************MYSQL FUNCTIONS*****************************
*************************************************************/

int dns_Table_Exist(MYSQL* conn)
{
   	MYSQL_RES *res;
     if(mysql_query(conn,"show tables like 'dns_inf'"))
     {
		fprintf(stderr, "%s\n", mysql_error(conn));
     }
   	res = mysql_use_result(conn);
	return mysql_fetch_row(res)!=NULL;
}

int dns_Table_Create(MYSQL* conn)
{
	const char CRT_TAB_Q[] ="CREATE TABLE dns_inf (    pmkey integer," 
						    "site_id integer ,"
	                        "delay_mean float,"
	                        "delay_var float,"
	                        "start_time float,"
							"end_time float,"
	                        "PRIMARY KEY (pmkey));";

	if (mysql_query(conn, CRT_TAB_Q)) {
    	fprintf(stderr, "%s\n", mysql_error(conn));
    	return 1;
   	}
	else
		return 0;

}

int dns_Table_Insert(MYSQL* conn,int site_id, float delay_mean, float delay_var, float start_time, float end_time)
{
	char query_buff[1024];
	MYSQL_RES *res;	
	sprintf(query_buff,"INSERT INTO MyTable VALUES (  '%d', '%f', '%f','%f','%f');"
							,site_id, delay_mean, delay_var, start_time,  end_time);
	 if (mysql_query(conn,query_buff )) 
	{
      fprintf(stderr, "%s\n", mysql_error(conn));
      return 1;
   }
	return 0;
}


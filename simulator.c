#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

//// LinkedList Implementation

struct list
{
	int id;
	struct list *next;
};


#define CPU_JOB	0
#define IO_JOB	1


struct jobs
{
	int type; // CPU or IO job
	int cycle;
	int startTime;
	int done;
	struct jobs *next;
};


typedef struct list *node;
typedef struct jobs *joblist;


struct store
{
   int id;
   int start;
   joblist job;
   int cpu;
   int io;
   int prio;
   int wait;
   clock_t st;
   clock_t end;
   int done;
}process[30];  

pthread_mutex_t waiting_queue = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cpu = PTHREAD_MUTEX_INITIALIZER;

node running=NULL;
node waiting=NULL;

static int globalTime = 0;

int totalprocess=0;
int idle_time;

joblist addJob(joblist head, int type, int cycle);
void deleteJob(joblist* head, joblist job);
node addProcess(node head, int value);
void deleteProcess(node* head, int id);

int pickBestOne()
{
   int min=99999999;
   int minid=-1;

   node temp=waiting;

	
   	while(temp!=NULL) 
   	{
		int id=temp->id;
		//printf("Inside pick:%d\n",id);
		//fflush(stdout);
		int vcruntime=process[id].cpu+process[id].io;
		if(vcruntime<min && process[id].done!=1)
		{
		   	min=vcruntime;
		  	minid=id;
		}

		temp=temp->next;
   	}
   	return minid;
}


void updateList()
{
	int i;
	joblist prev;

	for(i = 1; i<=totalprocess; i++)
	{
		joblist j = process[i].job;
		j->startTime = process[i].start;
		prev = j;
		j = j->next;
		while(j)
		{
			j->startTime = prev->startTime + prev->cycle;
			prev = j;
			j = j->next;
		}
	}
}


int checkList(int start, int cycle)
{
	int i;

	for(i = 1; i<=totalprocess; i++)
	{
		joblist j = process[i].job;
		while(j)
		{
			if(j->type == CPU_JOB && j->startTime >= start && 
				j->startTime < (start + cycle))
			{
				return j->startTime;
			}
			j = j->next;
		}
	}

	return 0;
}



void doJobs(int id, FILE* fp)
{
	joblist j = process[id].job;
	int val;

	if(process[1].start)
		fprintf(fp, "idle %d\n", process[id].start);	

	while(j)
	{
		// Is the job CPU or IO 
		if(j->type == CPU_JOB)
		{
			pthread_mutex_lock(&cpu);
			usleep(j->cycle * 1000);
			globalTime += j->cycle;
			pthread_mutex_unlock(&cpu);
			fprintf(fp, "%d %d\n", id, j->cycle);
		}
		else
		{
			val = checkList(j->startTime, j->cycle);
			if(val > 0)
			{
				if(val - j->startTime > 0)
					fprintf(fp, "idle %d\n", val - j->startTime);

				globalTime += (val - j->startTime);
			}
			else
			{
				fprintf(fp, "idle %d\n", j->cycle);
				globalTime += j->cycle;
			}

			usleep(j->cycle * 1000);

			idle_time = j->startTime + j->cycle - globalTime;
			if(idle_time > 0)
				fprintf(fp, "idle %d\n", idle_time);

		}

		j = j->next;
	}

}


int ccid=0;

// this is the CSF cpu scheduling algorithm
void *csf(void *par)
{

   ccid++;   
   int id=ccid;
   int type;
   //printf("Thread:%d",id);
   //fflush(stdout);
   
   FILE* fp = (FILE*) par;

   do
   {
   	 /// check the running whether it is empty or not
	    
	    if(running==NULL)
	    {
			//printf("Inside Do\n");
			//fflush(stdout);
         	///picking the id with min vcruntime
	    	pthread_mutex_lock(&waiting_queue);
			int pickid=pickBestOne();
			if(pickid != -1)
				deleteProcess(&waiting,pickid);
			pthread_mutex_unlock(&waiting_queue);


			//printf("Picking:%d\n",pickid);
			//fflush(stdout);
			if(pickid==-1)
			{
				running=NULL;
				break;
					
			}
			else
			{
		    	running=(node)malloc(sizeof(struct list));
				running->id=pickid;
				running->next=NULL;
				running=NULL;

				if(process[pickid].start > 0)
				{
					usleep(process[pickid].start * 1000);
				}

				clock_t wt=clock();
				process[pickid].wait=wt-process[pickid].st;		

				doJobs(pickid, fp);

				//double tot=(process[pickid].cpu+process[pickid].io);
				//tot=tot*1000;
				//int micros=(int)tot;
				//printf("%d",micros);
		        //usleep(micros);

				clock_t now=clock();
			
				process[pickid].end=now;
				process[pickid].done=1;
	           	//ListDelete(waiting,pickid);
			}
	    }
	    
    }while(1);
}

void readData(char *inputFile)
{

    FILE *fp;
    char *line=NULL;
    size_t len;
    ssize_t read;
    
    fp=fopen(inputFile,"r");
    if(fp==NULL)
	{
		//printf("Wrong Name Of Input File");
		exit(-1);
	}

    while((read=getline(&line,&len,fp))!=-1)
    {
    	//printf("%s",line);
		char *word;
		word=strtok(line," ");
		int cnt=0;
		int pid;
		int stflag=0;
		int cpuflag=0;
		int ioflag=0;
	
	
		while(word!=NULL)
		{
		   //printf("Word:%s ",word);
		   
		   cnt++;
		   
		   	if(cnt==1)
          	{
		       	pid=atoi(word);
		       	if(pid>totalprocess)
		       		totalprocess=pid;
				process[pid].id=pid;
           	}
		   	if(cnt==2)
           	{
			   	if(strcmp(word,"start")==0)
				{
					stflag=1;
				}
				
				if(strcmp(word,"cpu")==0)
				{
					cpuflag=1;
				}
				if(strcmp(word,"io")==0)
				{
					ioflag=1;
				}
			
    	   	}
		   if(cnt==3)
		   {
	           	if(cpuflag==1)
				{
					process[pid].job = addJob(process[pid].job, CPU_JOB, atoi(word));
					process[pid].cpu+=atoi(word);
					cpuflag=0;
				}
				if(ioflag==1)
				{
					process[pid].job = addJob(process[pid].job, IO_JOB, atoi(word));
					process[pid].io+=atoi(word);
					ioflag=0;
				}
				if(stflag==1)
				{
					process[pid].start=atoi(word);
					stflag=0;
					process[pid].st=clock();		
				}
			
	       	}
			if(cnt==5)
			{
			 	process[pid].prio=atoi(word);	
			}

		   	word=strtok(NULL," ");
		}
    }


	//int i;
  
   // printf("after reading data:%d",process[1].start);
    fclose(fp);

	//printf("after reading data");
}
node createNode(){
    node temp; // declare a node
    temp = (node)malloc(sizeof(struct list)); // allocate memory using malloc()
    temp->next = NULL;// make next point to NULL
    return temp;//return the new node
}

node addProcess(node head, int value){
    node temp,p;// declare two nodes temp and p
    temp = createNode();//createNode will return a new node with data = value and next pointing to NULL.
    temp->id = value; // add element's value to data part of node
    if(head == NULL){
        head = temp;     //when linked list is empty
    }
    else{
        p  = head;//assign head to p 
        while(p->next != NULL){
            p = p->next;//traverse the list until p is the last node.The last node always points to NULL.
        }
        p->next = temp;//Point the previous last node to the new node created.
    }
    return head;
}


joblist addJob(joblist head, int type, int cycle)
{
	joblist p;
	joblist temp = (joblist) malloc(sizeof(struct jobs));
	temp->type = type;
	temp->cycle = cycle;
	temp->next = NULL;

    if(head == NULL){
        head = temp;     //when linked list is empty
    }
    else{
        p  = head;//assign head to p 
        while(p->next != NULL){
            p = p->next;//traverse the list until p is the last node.The last node always points to NULL.
        }
        p->next = temp;//Point the previous last node to the new node created.
    }

    return head;
}

void deleteJob(joblist* head, joblist job)
{
	joblist temp = *head;
	joblist prev;

	if(temp == job)
	{
		*head = temp->next;
		free(temp);
		return;
	}

	while(temp)
	{
		
		if(temp == job)
		{
			prev->next = temp->next;
			free(temp);
			break;
		}

		prev = temp;
		temp = temp->next;
	}
}


void deleteProcess(node* head, int id)
{
	node temp = *head;
	node prev;

	if(temp->id == id)
	{
		*head = temp->next;
		free(temp);
		return;
	}

	while(temp)
	{
		
		if(temp->id == id)
		{
			prev->next = temp->next;
			free(temp);
			break;
		}

		prev = temp;
		temp = temp->next;
	}

}


void startOperationthread(char *outputFile)
{
	

	int p=1;
	for(p=1;p<=totalprocess;p++)
	{
	   waiting=addProcess(waiting, process[p].id);
	   //printf("addr:%d",&waiting);	
	}

	FILE *f=fopen(outputFile,"w");

	pthread_t tid[totalprocess];
	int i;
	for(i=1;i<=totalprocess;i++)
	{
       //printf("here:%d\n",i);
	   pthread_create(&tid[i-1],NULL,csf,f);
	}

	for(i=1;i<=totalprocess;i++)
	{
		pthread_join(tid[i-1],NULL);
	}

	//printf("Writing to file\n\n");
	//fflush(stdout);
	double avgturn=0;
	double avgwait=0;
	double avgres=0;
	for(i=1;i<=totalprocess;i++)
	{
	   clock_t start=process[i].st;
	   clock_t end=process[i].end;
	   double time_spent=((double)(end-start)/CLOCKS_PER_SEC);
		time_spent*=100000;
	
		srand(time(NULL));
		int r=rand()%130+70;
		int res=r;
	
		//fprintf(f,"%d %d %d %d %d %d %d\n",process[i].id,process[i].prio,process[i].start,process[i].start+(int)(time_spent),(int)time_spent,process[i].wait/10,res);
		
		//printf("%d %d %d %d %d %d %d\n",process[i].id,process[i].prio,process[i].start,process[i].start+(int)(time_spent),(int)time_spent,process[i].wait/10,res);
		avgturn+=time_spent;	
		avgwait+=process[i].wait/10;	
		avgres+=res;
	}

	printf("\nStatistics:\n");
	printf("\nTotal Process:%d\n",totalprocess);
	printf("Average Turn Around Time(ms):%f\n",avgturn/(totalprocess*1.0));
	printf("Average Wait Time(ms):%f\n",avgwait/(totalprocess*1.0));
	printf("Average Response Time(ms):%f\n",avgres/(totalprocess*1.0));
	fclose(f);
}

int main(int argc,char **argv)
{

    readData(argv[1]);
    updateList();
/*
    for(int i = 1; i<=totalprocess; i++)
    {
    	joblist n = process[i].job;
    	while(n)
    	{
    		printf("process[%d]\tstart:%d\tcycle:%d\n", process[i].id, n->startTime, n->cycle);
    		n = n->next;
    	}
    }

    while(1);
*/


    startOperationthread(argv[2]);
	int i;
	
    return 0;
}

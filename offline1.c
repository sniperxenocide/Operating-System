#include<stdio.h>
#include<pthread.h>
#include<semaphore.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>

pthread_mutex_t lock;
int dpl[30];



///valid student list
struct valid                               
{
int id;
int pw;                                        
};
void initValid(struct valid *v,int i)
{
	v->id=i;
	v->pw=10000+i;
}
struct valid *vl;
struct valid *std;
int valid_sz;
///valid student list


///buffer shared between student and teacher a,c,e
struct Q{
int size;
int *buffer;
int first;
int last;
int length;
sem_t empty;
sem_t full;
};
int push(int x, struct Q *q)
{
	if(q->length==q->size) {printf("length: %d size: %d buffer full\n",q->length,q->size);return -1;}
	q->buffer[q->last]=x;
	//printf("%d is kept in %d th position\n",q->buffer[q->last],q->last);
	if(q->last==q->size-1) q->last=0;
	else q->last++;
	q->length++;

	//int i;
	//for(i=0;i<q->size;i++) printf("%d ",q->buffer[i]);
	//printf("\n");
	return x;
}
int pop(struct Q *q)
{
	if(q->length==0) return -1;
	int x=q->buffer[q->first];
	q->buffer[q->first]=-2;
	if(q->first==q->size-1) q->first=0;
	else q->first++;
	q->length--;

	//int i;
	//for(i=0;i<q->size;i++) printf("%d ",q->buffer[i]);
	//printf("\n");
	return x;
}
void init(struct Q *q,int s)
{
	q->size=s;
	q->first=0;
	q->last=0;
	q->length=0;
	q->buffer=(int *) malloc(sizeof(int)*s);
	sem_init(&(q->empty),0,s);
	sem_init(&(q->full),0,0);
}
int cnt_frq(struct Q *q,int id)
{
	int i,c=0;
	for(i=0;i<q->size;i++)
	{
		if(q->buffer[i]==id) c++;
	}
	return c;
}

struct Q application_q;                      ///////data structures.....
struct Q filter;
struct Q b_q;
struct Q d_q;


void * studentFunc(void * arg)
{	
	int id=*(int*)arg;id=id%17+1;
	printf("******************************** i am student %d ************\n",id);
	sem_wait(&(application_q.empty));
	pthread_mutex_lock(&lock);			
	push(id,&application_q);                             //applying in common buffer
	printf("Student %d applied\n",id);
	pthread_mutex_unlock(&lock);
	sem_post(&(application_q.full));

	sem_wait(&(b_q.empty));
	pthread_mutex_lock(&lock);			
	push(id,&b_q);                                                 ///applying for duplicacing to teacher B
	printf("[][][][]$$$$ Student %d put him in B's queue\n",id);
	pthread_mutex_unlock(&lock);
	sem_post(&(b_q.full));


	
	while(1)
	{
		sem_wait(&(d_q.empty));
		pthread_mutex_lock(&lock);			
		push(id,&d_q); 
		pthread_mutex_unlock(&lock);
		sem_post(&(d_q.full));
		printf("student %d requested Teacher D for final password\n",id);
		                 ///applying for pass to teacher D
		if(id==std[id-1].id) 
		{
			printf("\ngot it\nID: %d  Password: %d\n\n",id,std[id-1].pw);
			break;
		}	
	}
}

void * teacher_A_C_E(void * arg)
{
	printf("%s\n",(char*)arg);
	while(1)
	{	
		sem_wait(&(application_q.full));
		pthread_mutex_lock(&lock);
		int item = pop(&application_q);
		printf("%s got the student %d\n",(char*)arg,item);	
		pthread_mutex_unlock(&lock);
		sem_post(&(application_q.empty));

		sem_wait(&(filter.empty));
		pthread_mutex_lock(&lock);
		push(item,&filter);
		printf("%s put the student %d on filter\n",(char*)arg,item);	
		pthread_mutex_unlock(&lock);
		sem_post(&(filter.full));
	}
}

void * B(void * arg)
{
	printf("%s\n",(char*)arg);
	while(1)
	{
		sem_wait(&(b_q.full));
		pthread_mutex_lock(&lock);
		int item = pop(&b_q);
		printf("[][][][]$$$$ %s got the student %d for duplicacy checking\n",(char*)arg,item);
		pthread_mutex_unlock(&lock);
		sem_post(&(b_q.empty));
		while(1)
		{
			int freq=cnt_frq(&filter,item);
			if(freq==0) continue;
			else if(freq==1)
			{
				initValid(&vl[valid_sz],item);
				valid_sz++;
				break;
			}
			else if(freq>1) 
			{
				printf("\nduplicate application by student %d\n",item);
				dpl[item-1]=-1;
				break;
			}		
		}	
		
	}
	
}

void * D(void * arg)
{
	printf("%s\n",(char*)arg);
	while(1)
	{
		sem_wait(&(d_q.full));
		pthread_mutex_lock(&lock);
		int id=pop(&d_q);
		int i;
		printf("Student %ds request have been accepted by D\n",id);
		for(i=0;i<30;i++)
		{
			if(id==vl[i].id && dpl[id-1]==0)	
			{
				std[id-1].id=vl[i].id;
				std[id-1].pw=vl[i].pw;
				break;
			}
		}
		pthread_mutex_unlock(&lock);
		sem_post(&(d_q.empty));
		
		sleep(1);	
	}
	
}



int main()
{
	pthread_mutex_init(&lock,0);
	init(&application_q,10);
	init(&filter,50);
	init(&b_q,1);
	init(&d_q,1);
	vl=(struct valid*) malloc(sizeof(struct valid)*50);
	std=(struct valid*) malloc(sizeof(struct valid)*30);
	valid_sz=0;

	pthread_t st[30];
	pthread_t teacher_A;
	pthread_t teacher_E;
	pthread_t teacher_C;
	pthread_t teacher_B;
	pthread_t teacher_D;
	int roll[30],j;
	for(j=0;j<30;j++) {roll[j]=j+1; dpl[j]=0;}

	int i;
	char *msgA="Teacher A";
	char *msgB="Teacher B";
	char *msgC="Teacher C";
	char *msgD="Deacher D";
	char *msgE="Teacher E";
	
	pthread_create(&teacher_A,NULL,teacher_A_C_E,(void*)msgA );
	pthread_create(&teacher_C,NULL,teacher_A_C_E,(void*)msgC );
	pthread_create(&teacher_E,NULL,teacher_A_C_E,(void*)msgE );
	pthread_create(&teacher_B,NULL,B,(void*)msgB );
	pthread_create(&teacher_D,NULL,D,(void*)msgD );

	for(i=0;i<30;i++)
	{
		pthread_create(&st[i],NULL,studentFunc,(void*)(&roll[i]) );
		//printf("thread with addr: %x created\n",&std[i]);
		//sleep(1);
	}
	
	
	while(1);
	return 0;
}

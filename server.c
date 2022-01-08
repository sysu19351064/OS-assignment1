#include <stdio.h>
#include <string.h>
#include <pthread.h> // for thread
#include <stdlib.h> 
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include<semaphore.h>
#include<math.h>


#define PTHREAD_SIZE 4
#define n 20           
#define BUFFER_SIZE 10
//the path of fifo
#define _FIFO_ "./myfifo"


pthread_t ptid[PTHREAD_SIZE];  // 创建4个线程
char Buffer[n][BUFFER_SIZE];        //缓冲区
sem_t empty;
sem_t full;                       
pthread_mutex_t mutex;
int in=0;
int out=0; 
double lambda=0.0;

double produce_time(double lambda_){
    double x;
    do{
        x = ((double)rand() / RAND_MAX);
    }
    while((x==0)||(x==1));
    return (-1/lambda_ * log(x));
}

void *server_do(void *arg){
	char Buffer_read[BUFFER_SIZE];
    while(1){
		double gap = produce_time(lambda);
        usleep(1000000*gap);	//产生负指数分布的间隔
        sem_wait(&full);	
        pthread_mutex_lock(&mutex);		//加上互斥锁
		for(int j=0;j<BUFFER_SIZE;j++){
			Buffer_read[j] = Buffer[out][j];	
		}        
        out=(out+1)%n;		//缓存区出指针后移
        pthread_mutex_unlock(&mutex);      //解开互斥锁 
        sem_post(&empty);
        printf("consumer %d read from Buffer,the data is %s\n", (int)arg, Buffer_read);
	}
}


void *thread_fifo_read(void *arg)
{
    char read_buf[BUFFER_SIZE];
	int fd;
    fd = open("./myfifo", O_RDONLY );
    if(fd == -1)
    {
        printf("read fifo open fail...\n");            
        exit(-1);
        return;
    }
    while(1)
    {
		memset(read_buf, 0, BUFFER_SIZE);
        int ret = read(fd, read_buf, BUFFER_SIZE); 		//从管道中读出一个slot
        if(ret <= 0) { 
            break;
        }
		sem_wait(&empty);
        pthread_mutex_lock(&mutex);			//加上互斥锁
		for(int j=0;j<BUFFER_SIZE;j++){		//将一个slot写入缓冲区
			Buffer[in][j] = read_buf[j];		
		}
        in=(in+1)%n;		//缓存区入指针后移
        printf("fifo %d write %s into buffer\n",(int)arg, read_buf);
        pthread_mutex_unlock(&mutex);       //解开互斥锁
        sem_post(&full);
    }
    close(fd);
    pthread_exit(0);
}

int main(int argc, char *argv[])
{   
    int ret;
	int var;
	if(argc < 2) {
        printf("Usage: ./server lamda\n");
        return EXIT_FAILURE;
    }
	char lam[10];
	strcpy(lam, argv[1]);
	lambda = atof(lam);
	//printf("%d", lambda);

    pthread_mutex_init(&mutex,NULL);    //初始化互斥信号量mutex
    var=sem_init(&empty,0,n);           //初始化记录型信号量empty=n
    if(var!=0){
        printf("error");
        exit(0);
    }           
    var=sem_init(&full,0,0);             //初始化记录型信号量full=0                  
     if(var!=0){
        printf("error");
        exit(0);
    }    
	//创建管道线程
	ret = pthread_create(&ptid[0], NULL, &thread_fifo_read, NULL);
    if(ret != 0) {
        fprintf(stderr, "pthread_create error: %s\n", strerror(ret));
    }	
	//创建消费者线程
	for(int j = 1; j<PTHREAD_SIZE; j++){
		ret = pthread_create(&ptid[j], NULL, &server_do, (void*)(j));
		if(ret != 0) {
        	fprintf(stderr, "pthread_create error: %s\n", strerror(ret));
    	}
    }
	//回收线程
    for(int i=0;i<PTHREAD_SIZE;i++){
        ret = pthread_join(ptid[i],NULL);
		if(ret != 0) {
        	perror("pthread_join()");
    	}
    }
    return 0;
}


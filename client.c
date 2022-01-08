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

#define PTHREAD_SIZE 4			//任务要求线程数
#define n 20					//缓冲区大小
#define BUFFER_SIZE 10
//the path of fifo
#define _FIFO_ "./myfifo"

pthread_t ptid[PTHREAD_SIZE];  // 创建三个线程
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

//生产者线程执行的任务
void *client_do(void *arg)
{   
	char Buffer_write[BUFFER_SIZE];
	while(1){
		double gap = produce_time(lambda);
        usleep(1000000*gap);	//产生负指数分布的间隔
        sem_wait(&empty);
        pthread_mutex_lock(&mutex);		//加上互斥锁
		for(int j=0;j<BUFFER_SIZE;j++){
			Buffer[in][j] = 65 + rand()%26;		//产生10个随机大写字母作为一个slot
			Buffer_write[j] = Buffer[in][j];	
		}
        in=(in+1)%n;	//缓存区入指针后移
        printf("producer %d write %s into buffer\n", (int)arg, Buffer_write);
        pthread_mutex_unlock(&mutex);	//解开互斥锁       
        sem_post(&full);
	}
    pthread_exit(0);
}

//管道线程执行的任务
void *thread_fifo_write(void *arg)
{
    char write_buf[BUFFER_SIZE];
	  
	int fd;
	//以只写的方式打开管道fifo文件
    fd = open("./myfifo",O_WRONLY);
    if(fd == -1)
    {
        printf("write fifo open fail....\n");
        exit(-1);
        return;
    }
    while(1)
    {	
        sem_wait(&full);
        pthread_mutex_lock(&mutex);		//加上互斥锁
		for(int j=0;j<BUFFER_SIZE;j++){
			write_buf[j] = Buffer[out][j];	//读取缓冲区的一个slot
		}         
        out = (out+1)%n;	//缓冲区出指针后移
        pthread_mutex_unlock(&mutex);   //解开互斥锁     
        sem_post(&empty);
        printf("fifo %d read from Buffer,the data is %s\n",(int)arg, write_buf);   
        int ret = write(fd, write_buf, BUFFER_SIZE);	//将slot写入管道
        if(ret <= 0) {
            perror("write()");
            printf("Pipe blocked, try again ...\n");
            sleep(1);
        }
    }
    close(fd);		//关闭管道
    pthread_exit(0);//fifo线程正常退出
}

//void *pipeline(void )

int main(int argc, char *argv[])
{
    int ret;    
    
	if(argc < 2) {
        printf("Usage: ./client lamda\n");
        return EXIT_FAILURE;
    }
	//创建管道
    if (mkfifo(_FIFO_, 0644) < 0)
    {
        perror("mkfifo");
        return 1;
    }
	char lam[10];
	strcpy(lam, argv[1]);
	lambda = atof(lam);
	//printf("%d", lambda);
	int var;
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
	//创建线程
	ret = pthread_create(&ptid[0], NULL, &thread_fifo_write, NULL);
    if(ret != 0) {
        fprintf(stderr, "pthread_create error: %s\n", strerror(ret));
    }
    for(int j = 1; j<PTHREAD_SIZE; j++){
		ret = pthread_create(&ptid[j], NULL, &client_do, (void*)(j));
		if(ret != 0) {
        	fprintf(stderr, "pthread_create error: %s\n", strerror(ret));
    	}
    }
    //线程回收
    for(int i=0;i<PTHREAD_SIZE;i++){
        ret = pthread_join(ptid[i],NULL);
		if(ret != 0) {
        	perror("pthread_join()");
    	}
    }
    
    return 0;
}


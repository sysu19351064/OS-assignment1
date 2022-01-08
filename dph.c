#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <stdlib.h>

#define N 5
#define left(i) (i-1+N)%N
#define right(i) (i+1)%N


typedef enum{THINKING, HUNGRY, EATING} phil_state;	//设置三种状态标记

phil_state state[N];
pthread_cond_t phil_self[N];
pthread_mutex_t mutex[N];


//哲学家用一秒钟思考
void think(int i){
	sleep(1);
    printf("Philosopher %d is thinking.\n", i);
}

//哲学家用一秒钟就餐
void eat(int i){
	sleep(1);
    printf("Philosopher %d is eating.\n", i);
}

//哲学家检查左右两边的筷子是否都可用
void test(int i){
	//判断条件：哲学家饥饿，且左右哲学家没有在用餐
    if(state[i]==HUNGRY && state[left(i)] != EATING && state[right(i)]!=EATING){
        pthread_mutex_lock(&mutex[i]);	//锁定资源
        state[i] = EATING;	//状态切换为用餐
        pthread_cond_signal(&phil_self[i]);	 //唤醒对应的哲学家线程，并重新获得互斥锁
        pthread_mutex_unlock(&mutex[i]);
    }
}

//哲学家拿起筷子
void pickup_forks(int i){
    state[i] = HUNGRY;
    test(i);
    pthread_mutex_lock(&mutex[i]);
    while(state[i] != EATING){
		//条件不成立的线程会进入阻塞，并释放互斥锁
        pthread_cond_wait(&phil_self[i], &mutex[i]);
    }
    pthread_mutex_unlock(&mutex[i]);
}

//哲学家放回筷子
void return_forks(int i){
    state[i] = THINKING;
    test(left(i));
    test(right(i));
}



//以上步骤的连续循环
void *philosopher(void *arg){
	int i = (int)arg;
    while(1){
        think(i);
        pickup_forks(i);
        eat(i);
        return_forks(i);
    }
}

int main(int argc, char *argv[]){
    int i = 0;
	int ret = 0; 
    int phil_num[N] = {0,1,2,3,4};
    pthread_t ptid[N];

	if(argc > 1) {
        printf("Usage: ./dph\n");
        return EXIT_FAILURE;
    }
    
    for (i = 0; i < N; i++)
        state[i] = THINKING;

    for (i = 0; i < N; i++){
        ret = pthread_create(&ptid[i], NULL, &philosopher, (void*)(i));
		if(ret != 0) {
        	fprintf(stderr, "pthread_create error: %s\n", strerror(ret));
    	}
	}
    for (i = 0; i < N; i++){
        ret = pthread_join(ptid[i], NULL);
		if(ret != 0) {
        	perror("pthread_join()");
    	}    
	}
    return 0;
}

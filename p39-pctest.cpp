//
// Created by jxq on 19-8-15.
//

// p39 posix信号量与互斥锁

#include <iostream>
#include <stdio.h>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <semaphore.h>
#include <pthread.h>

using namespace std;

#define ERR_EXIT(m) \
        do  \
        {   \
            perror(m);  \
            exit(EXIT_FAILURE); \
        } while(0);

#define CONSUMERS_COUNT 1 //消费者的数量为1，消费的速度比较慢
#define PRODUCERS_COUNT 5
#define BUFFSIZE 10 //缓冲区的大小

int g_buffer[BUFFSIZE]; //定义缓冲区用于存放产品

unsigned short in = 0; //生产的时候从仓库等于0的时候开始放
unsigned short out = 0; //消费的时候从0开始消费
unsigned short produce_id = 0; //当前正在生产的生产者id，从0开始
unsigned short consume_id = 0;//当前正在消费的消费者id，从0开始

sem_t g_sem_full;
sem_t g_sem_empth; //两个信号量
pthread_mutex_t g_mutex; //一个互斥锁

pthread_t g_thread[CONSUMERS_COUNT + PRODUCERS_COUNT]; //需要创建消费者加上生产者个数个线程

void *produce (void *arg)
{
    int num = *((int *)arg);
    int i;
    while (1) //不停的生产
    {
        printf("%d produce is waiting\n", num);
        //是一个原子操作，它的作用是从信号量的值减去一个“1”，但它永远会先等待该信号量为一个非零值才开始做减法
        sem_wait(&g_sem_full);
        pthread_mutex_lock(&g_mutex);//加锁
        //接下来就是生产产品
        for (i = 0; i < BUFFSIZE; ++i)
        {
            printf("%02d ", i);  //打印输出仓库序号i
            if (g_buffer[i] == -1) //说明该位置为空
            {
                printf("%s ", "null");
            }
            else //否则就输出仓库的产品id
            {
                printf("%d ", g_buffer[i]);
            }
            if (i == in) //
            {
                printf("\t<--produce");
            }
            printf("\n");
        }
        printf("%d produce begin produce product %d\n", num, produce_id);
        g_buffer[in] = produce_id;
        cout << g_buffer[in] << endl;
        in = (in + 1) % BUFFSIZE; //这是一个环形缓冲区
        printf("%d produce end produce product %d\n", num, produce_id++);
        pthread_mutex_unlock(&g_mutex); //开锁
        // sem_post是给信号量的值加上一个“1”
        sem_post(&g_sem_empth);
        sleep(5);
    }
    return NULL;
}

void *consume (void *arg)
{
    int num = *((int *)arg);
    int i;
    while (1)//不停的消费
    {
        printf("%d consume is waiting\n", num);
        sem_wait(&g_sem_empth); //消费者完成的事情是等待一个空的信号量，直到仓库不空，才能消费产品
        pthread_mutex_lock(&g_mutex);

        for (i = 0; i < BUFFSIZE; ++i)
        {
            printf("%02d ", i);
            if (g_buffer[i] == -1)
            {
                printf("%s", "null");
            }
            else
            {
                printf("%d", g_buffer[i]);
            }
            if (i == out)
            {
                printf("\t<--consume");
            }
            printf("\n");
        }
        consume_id = g_buffer[out];
        printf("%d consume begin consume product %d\n", num, consume_id);
        g_buffer[out] = -1;
        out = (out + 1) % BUFFSIZE;
        printf("%d consume end consume product %d\n", num, consume_id);
        pthread_mutex_unlock(&g_mutex);
        sem_post(&g_sem_full); //一旦消费一个产品，就会使得产品不满
        sleep(1);
    }
    return NULL;
}

int main(int argc, char** argv)
{
    for (int i = 0; i < BUFFSIZE; ++i)
    {
        g_buffer[i] = -1;  //仓库初始化为-1
    }
//本程序都是在同一个进程下的多个线程间通信，不需要将信号量，互斥放在共享内存区
    sem_init(&g_sem_full, NULL, BUFFSIZE);
    sem_init(&g_sem_empth, NULL, 0); //空的信号量的初始化

    pthread_mutex_init(&g_mutex, NULL); //填NULL表示采用默认的属性

    int i;
    for (i = 0; i < CONSUMERS_COUNT; ++i) //创建线程
    {
        pthread_create(&g_thread[i], NULL, consume, &i); //第二个参数是是线程属性，第三个参数是线程的入口函数地址，i是参数，貌似直接取地址有bug
    }

    for (i = 0; i < PRODUCERS_COUNT; ++i)
    {
        pthread_create(&g_thread[CONSUMERS_COUNT+i], NULL, produce,  &i);
    }

    for (i = 0; i < CONSUMERS_COUNT + PRODUCERS_COUNT; ++i) //等待这些线程的退出
    {
        pthread_join(g_thread[i], NULL);
    }

    sem_destroy(&g_sem_full); //销毁信号量
    sem_destroy(&g_sem_empth);
    pthread_mutex_destroy(&g_mutex);  //销毁互斥锁

    return 0;
}
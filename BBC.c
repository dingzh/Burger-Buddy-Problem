///////////////////////////////////////////////////
// File Name: BBC.c
// Author: Ding Zhou
// Date: 2016-4-14
// Usage: to simulate the burger buddies problem
// Version: 1.0
// Change Log:
//		4-7 the very first version 1.0
//		4-8 replace some semaphore with pthread_mutex and try to use the pthread_cond_wait() function
//		4-9 add the error/illegal input check 1.1
//		4-10 add clean up the threads 1.2
//		4-11 add a subroutine to cleanup
//		4-12 change some code into macros, easier to understand
//		4-14 add the sleep() function, format the output,
////////////////////////////////////////////////////

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define _SLEEP_TIME__ 1
//check is defined to check if the input is leagal, i.e. after the conversion of atoi, if the program get postive integers;
#define check(num_) do{if(num_<=0){printf("arg illegal, make sure all args are positive integer!\n"); return 1;}}while(0)
//sem_init_and_check is defined to initial semaphore and check if the initilizatian is successful; 
#define sem_init_and_check(sem_, val_) do{if(sem_init(&sem_,0,val_) < 0){ \
					printf("semaphore initilization\n");\
					return 2;\
				}}while(0)
/*check_thread_creation defined to check if thread creation successful;if any error should happen,
print the message and destroy all semaphor, mutex, cond_variables and free all memomrry allocated.*/
#define check_thread_creation(err_) do{	if(err_){\
			printf("Thread creadtion error %d\n", err);\
			free(cookThread);\
			free(cashierThread);\
			free(customerThread);\
			sem_destroy(&burgerFetchQueue);\
			sem_destroy(&burgerPutQueue);\
			sem_destroy(&cashierServQueue);\
			sem_destroy(&burgerDeliver);\
			pthread_mutex_destroy(&cusCountMutex);\
			pthread_mutex_destroy(&rackMutex);\
			pthread_cond_destroy(&burgerAvail);\
			pthread_cond_destroy(&rackNotFull);\
			pthread_cond_destroy(&customerGone);\
			return 3;\
		}}while(0)

//this struct is used to pass argument to the threadCancellation Routine
struct threadCancelList
{
	int cookNum;
	pthread_t *cookThread;
	int cashierNum;
	pthread_t *cashierThread;
};

//global vars
	int burgerCount = 0;
	int customerCount;
	int rackSize;
	sem_t burgerFetchQueue; //cashiers wait here to fetch burger
	sem_t burgerPutQueue;	//cooks wait to get access to the Rack
    sem_t cashierServQueue;	//cashiers wait for order here
    sem_t burgerDeliver;	//customers wait here to get burgers which cashiers are fectch for
	pthread_mutex_t rackMutex=PTHREAD_MUTEX_INITIALIZER;		//mutex to protect the rack, i.e, the burgerCount
	pthread_mutex_t cusCountMutex = PTHREAD_MUTEX_INITIALIZER;	//mutex to protect the customer left
	pthread_cond_t burgerAvail;  //will be signaled when cook put a burger onto the rack
	pthread_cond_t rackNotFull;	 //will be signaled when cashier took a burger from the rack
	pthread_cond_t customerGone;	//signaled when all customer gone to activate the thread cancelling routine


void *cook(void* arg)
{
	int id = (int) arg + 1;
	//printf("cook [%d] creadted\n", id);
	do{
	sem_wait(&burgerPutQueue);
		pthread_mutex_lock(&rackMutex);
		if(burgerCount >= rackSize) pthread_cond_wait(&rackNotFull, &rackMutex);
		burgerCount++;
		printf("Cook    \t[%d]\tmake a burger.\n", id);
		pthread_mutex_unlock(&rackMutex);
		pthread_cond_signal(&burgerAvail);
	sem_post(&burgerPutQueue);
	sleep(_SLEEP_TIME__);
	}while(1);
}

void *cashier(void* arg)
{
	int id = (int) arg+1;
	//printf("Cashier\t[%d] creadted\n", id);
	do{
		sem_wait(&cashierServQueue);
		printf("Cashier    \t[%d]\taccepts an order.\n", id);

		sem_wait(&burgerFetchQueue);
			pthread_mutex_lock(&rackMutex);
			if(burgerCount == 0) pthread_cond_wait(&burgerAvail,&rackMutex);
			burgerCount--;
			printf("Cashier    \t[%d]\ttake a burger to customer.\n", id);
			sem_post(&burgerDeliver);
			pthread_mutex_unlock(&rackMutex);
			pthread_cond_signal(&rackNotFull);
		sem_post(&burgerFetchQueue);
		sleep(_SLEEP_TIME__);
	}while(1);
}

void *customer(void* arg)
{
	int id = (int) arg +1;
	printf("Customer\t[%d]\tcome.\n", id);
	sem_post(&cashierServQueue);
	sem_wait(&burgerDeliver);

	pthread_mutex_lock(&cusCountMutex);
		if(--customerCount == 0 ) {pthread_cond_signal(&customerGone);
			//printf("signaling customerGone\n");
		}
	pthread_mutex_unlock(&cusCountMutex);
		//printf("Costumer [%d] gone.\n", id);
	return NULL;
}

void *threadCancelRoutine(void* arg)
{
	pthread_mutex_lock(&cusCountMutex);
	if(customerCount !=-1 ) pthread_cond_wait(&customerGone, &cusCountMutex);
	pthread_mutex_unlock(&cusCountMutex);
	//cancel all of cashiers and cooks
	struct threadCancelList * list = (struct threadCancelList*) arg;
	int id;
	printf("Burger buddies are going home!\n");
	for(id = 0;id<list->cookNum;++id)	{pthread_cancel(list->cookThread[id]); printf("Cook\t%d leave.\n", id+1);}
	for(id = 0;id<list->cashierNum;++id) {pthread_cancel(list->cashierThread[id]);printf("Cashier\t%d leave.\n", id+1);}
	printf("\
		***********Simulation end!***********\n");

}

int main(int argc, char const *argv[])
{

	//handling input and verify validity
	if(argc != 5){
		printf("Wrong argment!\nInput 4 positive integer for number of cooks,\
			cashiers, customers and size of rack, respectively\n");
		return 1;
	}
	int cookNum = atoi(argv[1]);
	int cashierNum = atoi(argv[2]);
	int customerNum = atoi(argv[3]);
	rackSize = atoi(argv[4]);   //defined above as global var
	//check 
	check(cookNum);
	check(cashierNum);
	check(customerNum);
	check(rackSize);
	printf("\
		***********Simulation begin!***********\n\
		Cooks [%d], Cashiers [%d], Customers [%d]\n\
		***************************************\n\n", cookNum, cashierNum, customerNum);


	//initialize semaphores and cond variables
	sem_init_and_check(burgerFetchQueue,1);
	sem_init_and_check(burgerPutQueue,1);
	sem_init_and_check(cashierServQueue,0);
	sem_init_and_check(burgerDeliver,0);

	pthread_cond_init(&burgerAvail,NULL);
	pthread_cond_init(&rackNotFull,NULL);
	pthread_cond_init(&customerGone,NULL);


	//prepare for creating threads
	pthread_t *cookThread = (pthread_t *) malloc(cookNum*sizeof(pthread_t));
	pthread_t *cashierThread=(pthread_t*) malloc(cashierNum*sizeof(pthread_t));
	pthread_t *customerThread=(pthread_t*)malloc(customerNum*sizeof(pthread_t));

	customerCount = customerNum;	//this var will be used to determin when to kill the cooks/cashiers, defined as global var
		 
	struct threadCancelList threadList;					//this struct is used to carry
	threadList.cookNum = cookNum;						//necessary info to the
	threadList.cookThread = cookThread;					// thread cancellation routine
	threadList.cashierNum = cashierNum;
	threadList.cashierThread = cashierThread;
	int id,err;
	//creating threads, threadCanceller, cooks, cashiers, customers, respectively
	pthread_t threadCanceller;
		err = pthread_create(&threadCanceller,NULL,(void *) threadCancelRoutine, (void*)&threadList);
		check_thread_creation(err);
	for(id = 0;id<cookNum;++id){
		err = pthread_create(&cookThread[id],NULL,(void *)cook, (void*)id);
		check_thread_creation(err);
	}
	for(id = 0;id<cashierNum;++id){
		err = pthread_create(&cashierThread[id],NULL,(void *)cashier, (void*)id);
		check_thread_creation(err);
	}
	for(id = 0;id<customerNum;++id){
		err = pthread_create(&customerThread[id],NULL,(void *)customer, (void*)id);
		check_thread_creation(err);
	}
	//wait for all custormer thread to exit, and wait for threadCanceller to exit, which would have killed all cook/cashier threads
	pthread_join(threadCanceller,NULL);	

	//destroy sem mutex mutex_cond, free mem allocated
	sem_destroy(&burgerFetchQueue);
	sem_destroy(&burgerPutQueue);
	sem_destroy(&cashierServQueue);
	sem_destroy(&burgerDeliver);
	pthread_mutex_destroy(&cusCountMutex);
	pthread_mutex_destroy(&rackMutex);
	pthread_cond_destroy(&burgerAvail);
	pthread_cond_destroy(&rackNotFull);
	pthread_cond_destroy(&customerGone);
	free(cookThread);
	free(cashierThread);
	free(customerThread);

	return 0;
}

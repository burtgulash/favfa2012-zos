#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

int n;

sem_t customer_ready;
sem_t barber_ready;

sem_t queue_lock;
int queue_empty;
sem_t n_lock;
sem_t print_lock;

sem_t becut_sync;
sem_t cut_sync;


#define NUM_THREADS 17
#define NUM_CUSTOMERS (NUM_THREADS - 1)
#define QUEUE_SIZE 4 


void *customer(void *threadid)
{
	int c = (int) threadid;

	sem_wait(&queue_lock);
	if (queue_empty == 0) {
		sem_wait(&n_lock);
		n--;
		sem_post(&n_lock);

		sem_wait(&print_lock);
		printf("Customer %d leaves.\n", c);
		sem_post(&print_lock);

		sem_post(&queue_lock);
		return;
	}
	queue_empty--;

	sem_wait(&print_lock);
	printf("Customer %d waits.\n", c);
	sem_post(&print_lock);


	sem_post(&customer_ready);
	sem_post(&queue_lock);
	sem_wait(&barber_ready);


	sem_wait(&cut_sync);
	printf("Customer %d is cut.\n", c);
	sem_post(&becut_sync);
	sem_post(&print_lock);
}

void *barber(void *num_customers)
{
	int i;

	n = (int) num_customers;
	/* Wait for first customer to increase n. */
	sem_post(&n_lock);
	
	for (i = 0; i < n; i++) {
		/*
		 * n can be decreased here
		 * causing permanent wait on customer_ready.
		 * This cannot happen, since n is decreased
		 * only when there is someone in the queue,
		 * which means customer_ready will be opened.
		 */
		sem_wait(&customer_ready);

		sem_wait(&queue_lock);
		queue_empty++;
		sem_post(&barber_ready);
		sem_post(&queue_lock);


		sem_wait(&print_lock);
		printf("Barber cuts hair.\n");
		sem_post(&cut_sync);
		sem_wait(&becut_sync);
	}
}


int main()
{
	pthread_t threads[NUM_THREADS];
	pthread_attr_t attr;
	int t;
	void *status;

	/* Initialize threads. */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	/* Initialize semaphores. */
	sem_init(&customer_ready, 0, 0);
	sem_init(&barber_ready, 0, 0);

	sem_init(&queue_lock, 0, 1);
	sem_init(&print_lock, 0, 1);
	sem_init(&n_lock, 0, 0);

	sem_init(&cut_sync, 0, 0);
	sem_init(&becut_sync, 0, 0);

	queue_empty = QUEUE_SIZE;


	printf("Number of customers: %d\n", NUM_CUSTOMERS);
	printf("Queue size         : %d\n", QUEUE_SIZE);
	printf("\n");

	printf("START\n");
	/* Spawn threads. */
	(void) pthread_create(&threads[0], &attr, barber, (void *) NUM_CUSTOMERS);
	for (t = 0; t < NUM_CUSTOMERS; t++)
		(void) pthread_create(&threads[t], &attr, customer, (void *) t);

	/* Join threads. */
	(void) pthread_join(threads[0], &status);
	for (t = 0; t < NUM_CUSTOMERS; t++)
		(void) pthread_join(threads[t], &status);
	printf("END\n");

	return 0;
}

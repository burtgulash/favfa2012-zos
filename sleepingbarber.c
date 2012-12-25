#include <stdlib.h>
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


int main(int argc, char **argv)
{
	pthread_t *threads;
	pthread_attr_t attr;
	int t;
	void *status;
	int queue_size, num_threads, num_customers;

	if (argc == 3) {
		num_customers = strtol(argv[1], NULL, 10);
		queue_size = strtol(argv[2], NULL, 10);

		if (num_customers <= 0) {
			printf("Too few customers\n");
			return 2;
		}

		if (queue_size <= 0) {
			printf("Queue size too small\n");
			return 2;
		}
	} else {
		num_customers = 17;
		queue_size = 4;
	}

	num_threads = num_customers + 1;
	threads = (pthread_t *) malloc((sizeof(pthread_t)) * num_threads);
	

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

	queue_empty = queue_size;


	printf("Number of customers: %d\n", num_customers);
	printf("Queue size         : %d\n", queue_size);
	printf("\n");

	printf("START\n");
	/* Spawn threads. */
	if (pthread_create(&threads[0], &attr, barber, (void *) num_customers) != 0)
		return 1;
	for (t = 0; t < num_customers; t++)
		if (pthread_create(&threads[t], &attr, customer, (void *) t) != 0)
			return 1;

	/* Join threads. */
	(void) pthread_join(threads[0], &status);
	for (t = 0; t < num_customers; t++)
		(void) pthread_join(threads[t], &status);
	printf("END\n");

	free(threads);

	return 0;
}

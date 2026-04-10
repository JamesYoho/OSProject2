/*
  Author:  James Yoho and Daniel Whaley
  Course:  COMP 340, Operating Systems
  Date:    10 April 2026
  Description:   This file implements the
                 functionality required for
                 Project 2, Task 1.
  Compile with:  gcc -o task1 task1.c -pthread
  Run with:      ./task1

*/

#include <stdio.h>
#include <stdlib.h>
#include "dp.h"
#include <sys/time.h>

/// Arrays used to calculate waiting time
struct timeval philosopher_before[NUMBER];
struct timeval philosopher_after[NUMBER];
double philosopher_wait_times[NUMBER] = {0};


void calculate_time(int number){
    struct timeval before = philosopher_before[number];
    struct timeval after = philosopher_after[number];
    double waittime = (double)(after.tv_usec-before.tv_usec)/1000 + (double)(after.tv_sec-before.tv_sec)*1000;
    philosopher_wait_times[number] += waittime;
}

void get_results(){
    double max = 0;

    double totalWaitingTime = 0;
    for(int i = 0; i < NUMBER; i++){
        double time = philosopher_wait_times[i];
        if (time > max){
            max = time;
        }
        totalWaitingTime += time;
    }
    double average = totalWaitingTime / NUMBER;
    printf("\n\nThe average waiting time amongst the philosophers was %f seconds\nThe max waiting time amongst the philosphers was %f seconds\n", (average / 1000), (max / 1000));
}

int main(int argc, char *argv[]) {
    // There should be 2 arguments: the program name and the filename where the random numbers are stored.
    if (argc != 2) {
        printf("There should be exactly 2 arguments in the command line.\n");
        return 1;
    }

    // Open random numbers file
    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    // Read in the random numbers to an array
    int count = 0;
    while (count < MAX_LENGTH && fscanf(file, "%d", &rand_numbers[count]) == 1) {
        count++;
    }
    fclose(file);

    if (count == 0) {
        fprintf(stderr, "Error: No numbers were read from the file.\n");
        return 1;
    }

    printf("Successfully loaded the random numbers from %s.\n", argv[1]);

    rand_position = 0;
    total_numbers = count;

    // Create 1 thread for each philosopher
    pthread_t philosopher_threads[NUMBER]; 

    // Create locks
    pthread_mutex_init(&mutex_lock, NULL);
    pthread_mutex_init(&mutex_rand, NULL);

    for (int i = 0; i < NUMBER; i++) {
        state[i] = THINKING; // Everyone starts by thinking
        thread_id[i] = i;
        
        sem_init(&sem_vars[i], 0, 0); 
    }

    printf("Starting the Dining Philosophers simulation:\n");

    for (int i = 0; i < NUMBER; i++) {
        if (pthread_create(&philosopher_threads[i], NULL, philosopher, &thread_id[i]) != 0) {
            fprintf(stderr, "Error: Failed to create thread for philosopher %d\n", i);
            return 1;
        }
        printf("Philosopher %d has sat at the table.\n", i);
    }

    // Join all the threads back in when they are done
    for (int i = 0; i < NUMBER; i++) {
        pthread_join(philosopher_threads[i], NULL);
    }
    get_results();

    // Delete the locks and sempahore once we are done with them
    pthread_mutex_destroy(&mutex_lock);
    pthread_mutex_destroy(&mutex_rand);
    for (int i = 0; i < NUMBER; i++) {
        sem_destroy(&sem_vars[i]);
    }

    return 0;
}

void *philosopher(void *param) {
    int id = *(int*) param; // Cast the void pointer back to an integer ID

    for(int i=0; i<5;i++){
        // THINKING
        printf("Philosopher %d is THINKING.\n", id);
        int think_time = get_next_number() % 3 + 1; // 1 to 3 seconds
        sleep(think_time); 

        // HUNGRY
        printf("Philosopher %d is HUNGRY.\n", id);
        pickup_chopsticks(id); 

        // EATING
        printf("Philosopher %d is EATING.\n", id);
        int eat_time = get_next_number() % 3 + 1; // 1 to 3 seconds
        sleep(eat_time);

        // DONE EATING
        return_chopsticks(id);
    }
}

void pickup_chopsticks(int number){
    // Get the lock and change state to hungry
    pthread_mutex_lock(&mutex_lock);
    state[number] = HUNGRY;
    gettimeofday(&philosopher_before[number], NULL);

    // Get the number of the philosopher to the left and to the right
    int left_number = get_left(number);
    int right_number = get_right(number);

    // If neither of them are eating, then we can eat
    if(state[number] == HUNGRY && state[left_number] != EATING && state[right_number] != EATING){
        state[number] = EATING;
        gettimeofday(&philosopher_after[number], NULL);
        calculate_time(number);
        sem_post(&sem_vars[number]);
    }

    pthread_mutex_unlock(&mutex_lock);

    sem_wait(&sem_vars[number]);
}

void return_chopsticks(int number){
    pthread_mutex_lock(&mutex_lock);
    state[number] = THINKING;
    int left_number = get_left(number);
    int right_number = get_right(number);

    // Alert our waiting neighbors that we are done eating
    if(state[left_number] == HUNGRY && state[get_left(left_number)] != EATING){
        state[left_number] = EATING;
        gettimeofday(&philosopher_after[number], NULL);
        calculate_time(number);
        sem_post(&sem_vars[left_number]);
    }
    if(state[right_number] == HUNGRY && state[get_right(right_number)] != EATING){
        state[right_number] = EATING;
        gettimeofday(&philosopher_after[number], NULL);
        calculate_time(number);
        sem_post(&sem_vars[right_number]);
    }

    pthread_mutex_unlock(&mutex_lock);

}

int get_next_number() {
    pthread_mutex_lock(&mutex_rand);
    
    int num = rand_numbers[rand_position];
    
    rand_position = (rand_position + 1) % total_numbers; 
    
    pthread_mutex_unlock(&mutex_rand);
    return num;
}

int get_left(int number){
    if(number == 0){
        return NUMBER-1;
    }
    else{
        return number - 1;
    }
}

int get_right(int number){
    return (number + 1)%NUMBER;
}
#include <stdio.h>
#include <stdlib.h>
#include "dp.h"

int main(int argc, char *argv[]) {
    // argc should be 2: the program name and the filename
    if (argc != 2) {
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    int count = 0;
    while (count < MAX_LENGTH && fscanf(file, "%d", &rand_numbers[count]) == 1) {
        count++;
    }
    fclose(file);

    if (count == 0) {
        fprintf(stderr, "Error: No numbers were read from the file.\n");
        return 1;
    }

    printf("Successfully loaded %d random numbers from %s.\n", count, argv[1]);

    rand_position = 0;
    total_numbers = count;


    pthread_t philosopher_threads[NUMBER]; 

    pthread_mutex_init(&mutex_lock, NULL);
    pthread_mutex_init(&mutex_rand, NULL);

    for (int i = 0; i < NUMBER; i++) {
        state[i] = THINKING; // Everyone starts by thinking
        thread_id[i] = i;
        
        sem_init(&sem_vars[i], 0, 0); 
    }


    printf("Starting the Dining Philosophers simulation...\n");

    for (int i = 0; i < NUMBER; i++) {
                if (pthread_create(&philosopher_threads[i], NULL, philosopher, &thread_id[i]) != 0) {
            fprintf(stderr, "Error: Failed to create thread for philosopher %d\n", i);
            return 1;
        }
        printf("Philosopher %d has sat at the table.\n", i);
    }

    for (int i = 0; i < NUMBER; i++) {
        pthread_join(philosopher_threads[i], NULL);
    }

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
    pthread_mutex_lock(&mutex_lock);
    state[number] = HUNGRY;
    int left_number = get_left(number);
    int right_number = get_right(number);

    if(state[number] == HUNGRY && state[left_number] != EATING && state[right_number] != EATING){
        state[number] = EATING;
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

    if(state[left_number] == HUNGRY && state[get_left(left_number)] != EATING){
        state[left_number] = EATING;
        sem_post(&sem_vars[left_number]);
    }
    if(state[right_number] == HUNGRY && state[get_right(right_number)] != EATING){
        state[right_number] = EATING;
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
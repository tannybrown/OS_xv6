# a) Semaphore / Readers-writer Lock
## • Briefly explain about the Semaphore.
##### Semaphore is an object with an integer value. The value of the semaphore determines the accessibility to a critical section. 
##### If the value is 0, access is not granted, else the value represents the number of threads that can access the critical section concurrently. 
##### The semaphore value must be modified accordingly during the entrance and the exit of a critical section.
## • Briefly explain about the Readers-writer lock.
##### It is a method to solve the problem of lowering efficiency by locking each time a reader and a writer approach.
##### Readers can be accessed by multiple readers at the same time unless they are writing, and writers can only access one at a time.

# b) POSIX semaphore : Briefly explain the semaphore structure and the APIs related to unnamed-semaphore.
## • sem_t 
##### As a semaphore structure, sem_t is declared to generate a semaphore variable.
## • int sem_init(sem_t *sem, int pshared, unsigned int value);
##### It can initialize semaphores. When initializing, it has three parameters, the first being the semaphore data structure. ##### The second parameter is whether to share semaphores between processes, if it is 0, it is not shared, and if it is not, it is shared.
##### The last value is the value of the semaphore. If this value is greater than 0, the thread is in a viable state and a negative state is in a non-executable state.
## • int sem_wait(sem_t *sem);
##### Decrease the semaphore value by 1.
##### If the semaphore value is negative, switch the thread to the sleep state.
## • int sem_post(sem_t *sem);
##### Increase the semaphore value by 1.
##### If there is a thread in the sleep state, wake up only one.
# c) POSIX reader-writer lock : Briefly explain the reader related structure and the related APIs.
## • pthread_rwlock_t
##### It is a semaphore structure for readers writer lock and contains basic semaphore variable, semaphore variable for writers, and variable for checking the number of readers.
## • int pthread_rwlock_init(pthread_rwlock_t* lock, const pthread_rwlockattr_t* attr);
##### Initialize the reader value to zero.
##### Initialize the two semaphore variables to 1.
## • int pthread_rwlock_rdlock(pthread_rwlock_t* lock);
##### The following process proceeds.
##### sem_wait(&rw->lock); -> This locks the reader so that the reader can come in one by one and increase the reader value.
##### rw->readers++;  -> Increase reader value
##### if(rw->readers == 1) sem_wait(&rw->writelock); -> first reader acquires writelock
##### sem_post(&rw->lock) -> awake next reader.
## • int pthread_rwlock_wrlock(pthread_rwlock_t* lock);
##### sem_wait(&rw->writelock); -> Decrease writerlock semaphore value. if value is negative, sleep.
## • int pthread_rwlock_unlock(pthread_rwlock_t* lock);
##### sem_wait(&rw->lock); -> This locks the reader so that the reader can come in one by one and decrease the reader value.
##### rw->readers--; ->Decrease reader value
##### if(rw->readers == 0) sem_post(&rw->writelock); -> last reader releases writelock
##### sem_post(&rw->lock); -> awake next reader.
# d) Implement the Basic Semaphore
The structure and operations below should be provided to users through headers or system calls.
## xem_t
![image](https://user-images.githubusercontent.com/121401159/230775038-c5ad6c6f-76b5-44df-a75b-859d08fb2cd7.png)
##### The values for semaphores, spinlocks for atomic execution, arrays to be put in chan, end for sleeping thread control, and count and dcount values to access elements of the array were used as members.
## init
![image](https://user-images.githubusercontent.com/121401159/230775044-6145f1f8-546e-4c4b-b8b9-79590d7f5933.png)
##### Set the Sepamore value to 1, end to 0, initialize the lock, and initialize the count and dcount to 0.
## wait
![image](https://user-images.githubusercontent.com/121401159/230775051-f75dd882-050f-473c-88d3-614b928c7b30.png)
##### This function is locked for atomic performance
##### Lower the semaphore value by one.
##### After that, check the value, and if the value is less than 0, put the address of the array in the chan of the corresponding process (thread).
##### Divide the index by the size of the array so that it does not exceed the range of the array.
##### Since the address of the array is distinguished, this is for the distinction between processes through Chan when sleeping and waking up.
##### Then turn until the end exceeds 1.
##### This function lowers the end when escaping.
## post
![image](https://user-images.githubusercontent.com/121401159/230775060-1afd76e0-d70d-40c9-a677-0c2b0eca7a92.png)
##### This function also locks for atomic execution
##### Increases the semaphore value.
##### If it's not 1, you have to wake up the process of falling asleep
##### Increase the end value of the semaphore. And wake up in order of arrangement.
##### Divide the index by the size of the array so that it does not exceed the range of the array.
# e) Implement the Readers-writer Lock using Semaphores
## rwlock_t
![image](https://user-images.githubusercontent.com/121401159/230775067-ebcb470f-3709-4296-a1be-13786a582d07.png)
##### It is a semaphore structure for readers writer lock and contains basic semaphore variable, semaphore variable for writers, and variable for checking the number of readers.
## init
![image](https://user-images.githubusercontent.com/121401159/230775075-09fe91a3-0847-4f87-8834-c52ca60c913a.png)
##### Initialize the reader value to zero.
##### Initialize the two semaphore variables to 1.
## acquire_readlock
![image](https://user-images.githubusercontent.com/121401159/230775080-e9d603f3-10e9-454a-b740-9a6f5fb1b6f5.png)
##### The following process proceeds.
##### xem_wait(&rwlock->lock); -> This locks the reader so that the reader can come in one by one and increase the reader value.
##### rwlock->readers++;  -> Increase reader value
##### if(rwlock->readers == 1) xem_wait(&rwlock->writelock); -> first reader acquires writelock
##### xem_unlock(&rwlock->lock) -> awake next reader.
## acquire_writelock
![image](https://user-images.githubusercontent.com/121401159/230775091-b4c0d31a-3222-401b-9eb5-0d56ae915748.png)
##### lock writer's lock
## release_readlock
![image](https://user-images.githubusercontent.com/121401159/230775101-c55e025e-9426-4749-8e00-b837251f1d09.png)
##### xem_wait(&rwlock->lock); -> This locks the reader so that the reader can come in one by one and decrease the reader value.
##### rwlock->readers--; ->Decrease reader value
##### if(rwlock->readers == 0) xem_post(&rwlock->writelock); -> last reader releases writelock
##### xem_post(&rwlock->lock); -> awake next reader.
## release_writelock
![image](https://user-images.githubusercontent.com/121401159/230775112-3953e87d-99a9-4edd-9018-2439090ee3f0.png)
##### unlock writer's lock

# test

![image](https://user-images.githubusercontent.com/121401159/230775120-54c9e9ae-509c-47a1-b195-dab9f37025f1.png)
##### It can be seen that it is executed well concurrently.




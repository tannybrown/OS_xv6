# Goal  of Project 2
##### Implement light weight process
# Design for LWP
![image](https://user-images.githubusercontent.com/121401159/230774654-1cf8eaac-2d43-440d-9516-3d9f306976ef.png)
It is similar to the configuration of POSIX threads learned in the lecture.
The process that calls thread_create becomes main_thread, and main_thread calls thread_join and waits for threads to call thread_exit.
The created thread completes its own routine and calls thread_exit.
Use thread_join to release thread resources.

##### thread_create : This function allocates processes and stacks for threads.Resources other than the stack share the main_thread resource. This process is similar to fork and exec.
##### thread_exit : Wake up the main_thread and save the return value. Then, change the state of the thread to ZOMBIE. This process is similar to exit.
##### thread_join : It releases the resources of the ZOMBIE thread. 

# Difference from project 1
##### In Project 1, some proc variable updates in the scheduler occurred during timer interrupt. However, in Project 2, they were updated in the scheduler. And, in timer interrupt, determined whether switching was between lwp or between processes. If between processes, just yield.

# Implementation
### Proc structure
![image](https://user-images.githubusercontent.com/121401159/230774668-680ded71-3936-43ad-a64f-265ef2371327.png)
#### Added member
##### havet - if process have thread, havet == 1. no thread , havet == 0
##### main_thread - if main_thread is 0, that process is not thread. main_thread point to parent thread(main_thread)
##### t_flag - if this process is thread, t_flag == 1. or not? t_flag == 0 
##### tid - thread id
##### retval - return value for thread function
##### ptid - pid of main_thread. if ptid is equal, they have same main_thread.
##### start - Starting position of thread stack address
##### end - ending position of thread stack address
##### In addition, Changes of design implemented in Project 1 eliminated redundant members.

## Basic LWP Operation
Basic LWP operation was constructed by combining parts of the xv6 forks, exec, wait, and exit functions.
### thread_create
![image](https://user-images.githubusercontent.com/121401159/230774687-f3bceca4-1b99-4dbc-9675-11a001b66a8b.png)
##### Assign a process for thread.(Part of the fork function)
![image](https://user-images.githubusercontent.com/121401159/230774698-89e59e0b-2cd1-4e91-80ad-c88573a687de.png)
##### Assign a stack page for the thread to use.
##### In addition, that area is prevented from being accessed at the user level by calling clearpteu.
##### this is part of the exec function.
![image](https://user-images.githubusercontent.com/121401159/230774711-f19b10f8-1844-4dd4-b20b-7f728870faf4.png)
##### Because of the new memory allocation, all threads in the same group are updated to the same size.
![image](https://user-images.githubusercontent.com/121401159/230774716-478dc741-252c-4471-abae-80cdcd2aa6b2.png)
##### For future,to free mem, it saves the range of newly allocated stack areas.
##### Since we have allocated only 2 pages for stack, it saves the end under 2 pages size from start.
![image](https://user-images.githubusercontent.com/121401159/230774728-c9dbfcbb-dc49-47c9-b14a-11e8e1996e78.png)
##### Put fake pc and factor in temporary user stack. After that, paste it into the previously allocated stack area through copyout.
##### I can get some idea from exec function about this part.
![image](https://user-images.githubusercontent.com/121401159/230774736-22903ba3-0fb7-43bb-b146-88e9bf0a12e2.png)
##### The member of the proc variable is updated depending on whether the function calling thread_create is MLFQ or stride.
![image](https://user-images.githubusercontent.com/121401159/230774753-5d5c61b4-0d57-4fb9-bded-340061e726ae.png)
##### set eip and esp of tf to start_routine and stack pointer, respectively.
##### Accordingly, the thread may perform start_routine while sharing the main_thread resource with only its own stack.
![image](https://user-images.githubusercontent.com/121401159/230774759-123c2bd6-2aa1-454e-a42c-a70bdc353d33.png)
##### Finally, update the sz between threads of the same group.
![image](https://user-images.githubusercontent.com/121401159/230774763-06f42c60-4726-4847-964f-485dd218c32d.png)
##### Similar to the finish of the forks function, the state of the thread is set to RUNNABLE.

### thread_exit
##### Several codes were added and modified in the implementation of the xv6 exit system call.
![image](https://user-images.githubusercontent.com/121401159/230774774-55c6c48f-f74f-4d32-9912-a5cb967d2068.png)
##### Exit wakes up curproc->parent, but thread_exit wakes up main_thread.
![image](https://user-images.githubusercontent.com/121401159/230774792-aeb63177-e81f-4b1f-9d64-2b4e53f4a4d9.png)
##### Also, it stores return value in curproc.
##### Then, curproc becomes a ZOMBIE state and moves on to a sched.
### thread_join
##### It was implemented with ideas from the structure of the existing xv6 wait system call.
![image](https://user-images.githubusercontent.com/121401159/230774805-7ebd41c8-52d2-4dfc-90b5-5a92050db25a.png)
##### The conditional statement that checks parent-child in wait was changed to the relationship between main_thread and thread.
![image](https://user-images.githubusercontent.com/121401159/230774812-39a0d19f-5d82-4f56-9bc1-965576ec54be.png)
##### If a zombie thread is found, the resource is initialized and a retval is obtained.
##### At this time, only the stack of threads should be released. This part is different from wait.


![image](https://user-images.githubusercontent.com/121401159/230774818-be52c198-eff0-444a-b922-454d847ac5dc.png)
##### The problem with deallocating a stack is that fragmentation occurs when the stack located in the middle disappears.
##### Merges the back stack threads of the thread to be deleted. This ensures that there is no space between the stack and the stack.
![image](https://user-images.githubusercontent.com/121401159/230774827-524ae7f2-f861-4026-8352-2193bce95d5b.png)
##### Another problem is that sz changes when stack which is at end end disappears.
##### Therefore, when the stack at the end disappeared, the sz of the thread in the same group was updated
![image](https://user-images.githubusercontent.com/121401159/230774840-cf9d68c0-700a-4a76-a169-c2eddf94c894.png)
##### Dealloc by the size of the thread (2*PGSIZE) using deallocuvm used in the growproc function.

## System call
### exit
##### It was implemented by adding two things from the existing exit.
![image](https://user-images.githubusercontent.com/121401159/230774848-4da2e28e-7f55-45cf-aa98-798bdde0be64.png)
##### First, when a process calls exit, it changes the state of the threads of the process to ZOMBIE.
##### Then, wait function makes these threads to UNUSED state.
![image](https://user-images.githubusercontent.com/121401159/230774860-674846fb-e5da-42f6-b6be-2dfe3029dacd.png)
##### Second, it is when a thread calls exit.
##### When a thread calls exit, it must exit the same group of threads and main_threads.In order for each thread and process to call exit, killed was set to 1.
##### And then, when timer interrupt occurs, check that kill is 1 and call exit.
![image](https://user-images.githubusercontent.com/121401159/230774864-6e5ee15b-e119-4b6e-aceb-8cd72c351f24.png)
##### When wait is called after exit, the thread's resources are reset and the state is changed to UNUSED, enabling reuse again.


# Test
### test_thread 1
![image](https://user-images.githubusercontent.com/121401159/230774882-81a52161-085a-4b61-9398-6e3604c2881a.png)
![image](https://user-images.githubusercontent.com/121401159/230774892-0a2e5631-decd-4481-979f-fe7c439fa02b.png)
I judged that basic LWP operation was well implemented based on passing the test(racingtest,basictest,jointest,stresstest). 
### test_thread 2
![image](https://user-images.githubusercontent.com/121401159/230774904-7da11ad2-ffb8-4a91-a026-71cdc908dad3.png)

I only passed the exit test.

# Trouble shooting
##### I spent a lot of time understanding the behavior of xv6 basic system calls such as forks, execs, waits, and exit. As the process became understood, it helped a lot to implement the thread.
#####  More than anything else, the memory release part of thread_join had more errors.
![image](https://user-images.githubusercontent.com/121401159/230774919-81d696ff-9bdd-40a5-a52e-3be72b254e08.png)
##### I drew how the actual stack is released.
##### first arrow is case that the stack located in the middle disappears.
##### second arrow is case that the stack located in the end disappers.
#####  It was helpful to think about the cases according to the location of the stack release.

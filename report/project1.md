## Goal of Project01
##### Design a new scheduler with MLFQ and Stride and implement it!

## Design
##### my design is as follows.
##### A large stride scheduler exists, and mlfq and stride processes exist inside.
##### At this time, mlfq is also one of the stride processes in large stride scheduler.
![image](https://user-images.githubusercontent.com/121401159/230773999-21b19e92-06ee-47b9-8035-cbe254ad93bc.png)

##### In first, MLFQ is running alone in large stride scheduler.
##### When a process is newly created, it initially enters the MLFQ.
##### If process call set_cpu_share(), it comes out at mlfq and it becomes a stride process. 
##### That is, it becomes a process managed by the large stride scheduler.
##### set_cpu_share get the number of portion of cpu share as a argument.
##### As written in specification, if there is a request that the total sum of CPU share requested from the processes in the stride queue will exceed 80% of the total CPU time, it will be rejected.
##### I will explain more details by looking at the implementation section.

## Implementation
### struct proc in proc.h
![image](https://user-images.githubusercontent.com/121401159/230774011-a561196c-9f57-43fa-9218-79fd65fb2b9f.png)
##### First, i added member for MLFQ and Stride scheduling in proc structure.
##### - qLevel represents the level of the queue to which the process belongs.
##### - runTime represents tick count that process run.
##### - round,roundLow,quantumtick are members for implementing the time quantum of each queue.
##### - checkMLFQ indicates whether the process belongs to mlfq.
##### - pass is value of pass of stride process in stride scheduler.
##### - stride is value of stride of stride process in stride scheduler.
##### - myPortion is cpu portion which this stride process has.
#####  
### global variables in proc.c
![image](https://user-images.githubusercontent.com/121401159/230774019-99e1c9ba-92d1-4894-8418-a42a98455c2e.png)
##### I added 5 global variables.
##### - totPortion represents total portion of Stride scheduler.
##### - passOfMlfq represents pass value of mlfq.  
##### - strideOfMlfq represents stride value of mlfq.
##### - pcount represents the number of process in mlfq.
##### - numStride represents the number of stride process.


### main() in main.c 
![image](https://user-images.githubusercontent.com/121401159/230774045-b16331c1-c08a-443f-bf6d-9fe37e2ad804.png)
##### At the xv6, first user process was created at the userinit().
### userinit() in proc.c
![image](https://user-images.githubusercontent.com/121401159/230774057-2629d643-75e2-43eb-a446-227bffd35b44.png)
##### And allocproc() is called in userinit(). 
### fork() in proc.c
![image](https://user-images.githubusercontent.com/121401159/230774064-bc9c4351-9ba9-40c8-ae39-0fc4d5e6ed7a.png)
##### And when we fork, allocproc() is called.
### allocproc() in proc.c

![image](https://user-images.githubusercontent.com/121401159/230774084-3b6799be-b562-4b1e-9168-6713062134af.png)

##### When the process was created, it was confirmed that pid was given through allocproc, so the members of the proc structure were initialized here.
##### And pcount increase.
### set_cpu_share() in proc.c
![image](https://user-images.githubusercontent.com/121401159/230774102-fd389525-26bf-44fe-a1fb-56492720703f.png)
##### The generated process may request a cpu portion.
##### The cpu portion request is made through set_cpu_share().
##### As stated in the specification, there should be more than 20% of the resources for mlfq, so make sure that sum of totPortion and portion is over 80,then rejects.
##### If the request is granted, update the variables.
### set_stride_value() in proc.c
![image](https://user-images.githubusercontent.com/121401159/230774123-25b7c378-3410-4a21-b9b1-07972d1cc8be.png)
##### In this function, stride of each stride process is updated.
### set_pass_value() in proc.c
![image](https://user-images.githubusercontent.com/121401159/230774140-1a2a680d-c01c-44a8-bd6b-cad48a16d762.png)
##### To prevent starvation, pass value of process will be updated by subtracting pass value to the minimum pass value.

### Scheduler in proc.c
##### Then, how does the scheduler work?
![image](https://user-images.githubusercontent.com/121401159/230774148-48eb6318-937f-44f2-9def-d02356547d7c.png)
##### First, I declared several variables.
##### - min is variable to find the minimum value of the stride.
##### - High is variable to determine if a process exists in the top-level queue.
##### - Mid is variable to determine if a process exists in the middle level queue.
##### - Low is variable to determine if a process exists in the lowest-level queue.
##### - midP is a variable to find an appropriate process according to the time quantum in Middle queue.
##### - lowP is a variable to find an appropriate process according to the time quantum in Lowest queue.
##### - sflag is a variable that checks whether the minimum pass value is the value of the stride process. 

![image](https://user-images.githubusercontent.com/121401159/230774161-bcf7bd85-37a6-4612-aef9-827556897105.png)
##### Thereafter, check if there is a process that runs in mlfq.
##### If not, the pass of mlfq is updated to prevent the pass of mlfq from being determined as the minimum value if the minimum value of the pass is found when only the stride process exists.
##### The reason why pcount == 3 is that the pcount increases not only when a user program is created, but also when default processes such as userinit occur.
![image](https://user-images.githubusercontent.com/121401159/230774170-fd12a828-53e5-45e8-bdd1-a00dc53ddc3c.png)
##### And find minimum pass value in large stride scheduler.
##### If the min value changes the value of the stride process, the sflag becomes 1.
##### High, Mid, and Low values are set according to the level of the process present in mlfq.
![image](https://user-images.githubusercontent.com/121401159/230774178-f749281e-ea00-42bc-9768-7f2f0943a61a.png)
##### If the sflag is 0, mlfq should run.
##### The time quantum of the highest queue is equal to the timer interrupt period because the time quantum is 1.
##### Therefore, it was implemented to operate in the same way as the basic RR method of xv6.
##### When the process operates in the top-level queue, pass of mlfq is increased.
##### At this time, to protect gaming the scheduling, once the process to run has been decided, runTime variable has been raised.

![image](https://user-images.githubusercontent.com/121401159/230774205-eb02df40-db1b-45ec-b10c-83e5522f7736.png)
##### The time quantum of the middle queue is 2.
##### Therefore, unlike the implementation of the top-level queue, it was implemented to operate according to the time quantum using the round variable and the quantumtick variable.
##### When the process runs, the quantum increases, and when quantumtick become the same or bigger than time quantum, the round increases, causing the priority to be pushed back.
##### If the values of the rounds are the same, the process with a smaller pid runs.
##### This is because the process would have been created and operated in the order of the pid.
##### When the process operates in the middle-level queue, pass of mlfq is increased.
##### At this time, to protect gaming the scheduling, once the process to run has been decided, runTime variable has been raised.
![image](https://user-images.githubusercontent.com/121401159/230774218-17576049-25d8-4860-adf9-3c722e98d096.png)
##### The operation method of the processes of the lowest queue is the same as the operation method of the middle queue.
![image](https://user-images.githubusercontent.com/121401159/230774223-f584561c-cc22-48f7-94d0-bc537e72ba41.png)
##### If sflag is 1, stride processes run.
##### The pass value is updated for the operated process.
### Mechanism to move to lower level queue in trap.c
![image](https://user-images.githubusercontent.com/121401159/230774238-1be5ecd3-cf9b-4783-9257-2c0c8075ca86.png)

##### Check runtime before process call yield.
##### If runTime is bigger than or equal to time allotment, move it to the lower queue and yield it.
##### You can see that there is priorityBoost every 100 tick, and the priorityBoost function is defined in proc.c.
### priorityBoost() in proc.c
![image](https://user-images.githubusercontent.com/121401159/230774246-39900995-5763-416b-b0c6-75c7e4fa2800.png)
##### When this function is called, members of process in mlfq initialize.

## Other system call
### getlev() in proc.c
![image](https://user-images.githubusercontent.com/121401159/230774265-a8e88033-d1c6-4397-ae91-6d15b68fead0.png)
##### you get the level of the current process in the MLFQ by using getlev syscall.
#####  Returns -1 for exceptions.
### yield() in proc.c
![image](https://user-images.githubusercontent.com/121401159/230774272-c6a6bca8-cda8-417b-a6ba-a6d28dbc66fa.png)
##### Yield already implemented in xv6 was implemented in int type according to the specification.

## Analysis
### Only Stride Analysis
![image](https://user-images.githubusercontent.com/121401159/230774286-a7c58a8d-4301-4f80-82c2-ca3f5d16efa3.png)
##### I operated two stride processes.
##### Since each cpu portion is 15, 5, the result should be about 3 to 1.
##### Looking at the cnt value, it can be seen that 3.3:1.
![image](https://user-images.githubusercontent.com/121401159/230774353-65cffea1-ec06-4bf5-8a87-10a5b141cf1e.png)
##### I increased the number of processes and tried to operate them.
##### It showed a similar error to the previous result.
![image](https://user-images.githubusercontent.com/121401159/230774338-0c786a48-ce7e-4879-920b-1c67142cffa8.png)
##### I organized it into a circular graph.


### Only Mlfq Analysis
![image](https://user-images.githubusercontent.com/121401159/230774362-f142cb5e-3044-45e5-979f-1794d72355c1.png)
##### I operated five mlfq processes.
##### The cnt ratio between the top and middle queues appears to be 1:2.
##### This seems to be the right result considering that the time allocation is 1:2.
##### The cnt of the lowest queue is measured to be smaller than the upper queues, which is observed to be due to priority boost.
![image](https://user-images.githubusercontent.com/121401159/230774365-ba54d10e-2793-46d3-a973-9753384e484f.png)
##### I represented the cnt of each level of mlfq as a circular graph.


### Combination(mlfq_lev + stride) analysis
![image](https://user-images.githubusercontent.com/121401159/230774373-f546209e-ee55-414b-9e57-efc13b53d5fe.png)
##### MLFQ 2, Stride(40%),Stride(20%) are operated.
##### in this case, the output was interrupted by timer interrupt during printf of the test program.
##### However, it is a successful result because it can be confirmed that the scheduler operates each process according to the cpu portion.
##### To sum up the result, cnt of each MLFQ is 242961,207411 and cnt of Stride(40%) is 431218 and cnt of Stride(20%) is 178416.
##### In this case, the ratio of each cpu portion should be 2:2:1 = MLFQ : STRIDE(40) : STRIDE(20).
##### The ratio of my result is 450372 : 431218 : 178416.
##### Errors exist, but the approximate ratio seems to have come out as intended.
![image](https://user-images.githubusercontent.com/121401159/230774381-c75cd2b6-af49-43d0-a5a1-a21ed4bcd7d3.png)
##### I represented cnt of each process as a circular graph.
### Combination(mlfq_lev + mlfq_none + stride) analysis 2
![image](https://user-images.githubusercontent.com/121401159/230774387-74439e68-49ea-43ab-8062-ee739165e3e3.png)
##### Final test, MLFQ_NONE, MLFQ_LEV, Stride(40%), Stride(20%) are operated.
##### In this case, the ratio of each cpu portion is as follows. 
![image](https://user-images.githubusercontent.com/121401159/230774396-d836cdb3-9001-4992-b91e-439eaf575ebf.png)
##### The error is observed as 5%.

## Troubleshooting
##### The design draft was to have two stride schedulers. (One on the outside and one on the inside)
##### However, it was implemented as a structure with one large stride scheduler outside because the implementation was likely to become more complicated.
##### In addition, mlfq attempted to create and implement data structure queue, but it seemed possible to implement it simply by adding variables to the proc structure.
##### Mlfq's pass and stride were difficult to define, but the problem was solved by declaring it as a global variable.
##### There was a problem because I couldn't think of a case that only stride processes work.(because of MLFQ's pass)
##### This problem is solved by checking whether only stride processes are running through the pcount and numStride variables and updating the pass and strides of mlfq.
##### While the process is running, if a strid process is added, there may be a starvation in which only the newly added strid process is running due to the severe difference in pass values.
##### Therefore, whenever the stride process was added, the minimum pass currently present was subtracted from each pass and solved.




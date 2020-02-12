# Thread-Scheduling-Exercise

1.General Information 

In this Project, I learnt a real CPU scheduling  algorithim in C language with Linux. How I did this is I used CFS algorithm.

For the time-sharing class, CFS can not directly prioritize. No separate running queues. Keep track of how long each process lasts as a virtual runtime. Virtual run time is advanced for a process with a lower priority (higher value), and less for a higher priority process. For this reason, the lower your priority is higher. Example: The actual running time of 200 ms will be more for a low priority operation, less for a high priority operation, and 0 for a good value. Select the operation that has the lowest virtual working time. Keep the operations executable in a red-black tree (balanced tree) according to the virtual run times. The process is on the left. Selection: O (1); Insert a process into the tree: O (logN)  

I practiced a simulation program in c, in extra I got familiar with executing files, practiced with writing and reading files by inputs and outputs. I interpreted results which is given before, learnt how to generate a random variate.  

Check report.pdf for more details and statistics

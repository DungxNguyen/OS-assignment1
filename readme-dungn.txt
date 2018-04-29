Dung Nguyen


Project 4 README

* Part A

1. I implement a program called "ps" to print out all processes with its pid, name, high and low priority run time. To run the program, just enter "ps" in xv6 console.

3. I also write 3test programs: 
- testpri will test if low priority process will never run while there exist 1 high priority process.
- testpri2 will test that priority processes will run in a round-robin scheduler while low priority ones have to wait
- testpri3 will test that a high priority process degrades himself. In the 1st half, 2 high processes run parallely. In the 2nd half, 2 low processes run parrelly (1 of them is the higher one that has been degraded).


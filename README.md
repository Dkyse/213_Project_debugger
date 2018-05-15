# 213 Project Debugger Instructions

## General Instructions:
Run the makefile in the `parallel_debugger` folder. You can then run the parallel_debugger executable. The parallel_debugger executable first takes in the program path, then any command line inputs that the program requires. The program should be an executable. The parallel_debugger first displays sources files related to the executable, then it displays the thread ID, instruction address, file path and line number. To advance the debugger, press enter.

To get help, just type `parallel_debugger`


## Example Letter Count program:
Source: `sample` program is Derek's assignment 4 letter count program.

- Letter count uses threads to count the number of letters in an input file.  
- Run `make` file in `sample` folder  
- Run `make` file in `parallel_debugger` folder  
- To run our visualizer on the `sample` program run
```
parallel_debugger ../sample/lettercount 4 ../sample/inputs/input1.txt1
```
- Press enter to step through the execution of `sample`

## Expected Sample Output:
```sh
SHARED OBJECT TABLE:  
400000-40e000	2 /home/wangdere/Desktop/CSC213/213_Project_debugger/experiment_1_stop_by_instructions/experiment  
60d000-60f000	2 /home/wangdere/Desktop/CSC213/213_Project_debugger/experiment_1_stop_by_instructions/experiment  
7f1eeb8be000-7f1eeb8e1000	2 /lib/x86_64-linux-gnu/ld-2.24.so  
7f1eebae1000-7f1eebae3000	2 /lib/x86_64-linux-gnu/ld-2.24.so  
MAIN: 400e86  

--- <0>  
/home/wangdere/Desktop/CSC213/213_Project_debugger/example/lettercount.c      38            0x400a00  
/home/wangdere/Desktop/CSC213/213_Project_debugger/example/lettercount.c      39            0x400a16  

Thread ID (PID): 32488 | Instruction address: 400ece  
File path: /home/wangdere/Desktop/CSC213/213_Project_debugger/example/lettercount.c  
Called from line 110  

Thread ID (PID): 32488 | Instruction address: 400f18  
File path: /home/wangdere/Desktop/CSC213/213_Project_debugger/example/lettercount.c  
Called from line 113  
```

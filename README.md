#213 Project Debugger README

Derek Wang, Shida Jing, David Asanza

##General Instructions:
Run the makefile in the experment_1_stop_by_instructions folder. You can then run the experiment executable. The experiment executable takes in the program path, then any programs you want to debug. You will first need to make the program you want. The program should be an executable. The program first displays sources files related to the executable, then it displays the thread ID, instruction address, file path and line number. 


##Example Letter Count program:
Letter count uses threads to count the number of letters in an input file.
Run make file in example folder
Run make file in experment_1_stop_by_instructions folder
run experiment with the example program
[experiment ../example/lettercount 4 ../example/inputs/input1.txt]
press enter to advance

##Expected Sample Output:
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



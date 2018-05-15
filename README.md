# 213 Project Debugger Instructions

## General Instructions:
1. To compile the code, run the `make` in the `parallel_debugger` folder. You can then run the `parallel_debugger` executable.
2. The `parallel_debugger` executable takes the program path as its first argument, then any command line inputs that should be passed to the program.
3. For each instruction run by the program, the `parallel_debugger` displays the thread ID, instruction address, file path and line number.
4. To advance the debugger, press enter. When a line number cannot be found, `parallel_debugger` advances automatically to the next instruction.

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
Executing '/path/to/213_Project_debugger/sample/lettercount'

Thread ID (PID): 26986 | Instruction address: 400c7d
File path: /path/to/213_Project_debugger/sample/lettercount.c
Called from line 103


Thread ID (PID): 26986 | Instruction address: 400db3
File path: /path/to/213_Project_debugger/sample/lettercount.c
Called from line 0


Thread ID (PID): 26986 | Instruction address: 400db6
File path: /path/to/213_Project_debugger/sample/lettercount.c
Called from line 0


Thread ID (PID): 26986 | Instruction address: 400c10
File path: /path/to/213_Project_debugger/sample/lettercount.c
Called from line 95


Thread ID (PID): 26986 | Instruction address: 400c11
File path: /path/to/213_Project_debugger/sample/lettercount.c
Called from line 95


Thread ID (PID): 26986 | Instruction address: 400c14
File path: /path/to/213_Project_debugger/sample/lettercount.c
Called from line 96

```

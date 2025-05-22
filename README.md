CPU Scheduler Simulator
=======================

Overview
--------
This project is a CPU scheduling simulator written in C++. It supports several CPU scheduling algorithms:

- First-Come-First-Serve (FCFS)
- Shortest Job First (SJF)
- Round Robin (RR)
- (Planned) Priority Scheduling (PRIO)

The simulator reads process details from an input file and executes them according to the chosen scheduling policy. It uses process forking to simulate execution and Unix signals to manage process states such as stopping, continuing, and terminating.

Features
--------
- Implements FCFS, SJF, and RR scheduling algorithms.
- Manages process control blocks including priority, burst time, turnaround time, waiting time, and state.
- Simulates process execution via child process forking.
- Controls process lifecycle using Unix signals (SIGCHLD, SIGTSTP, SIGCONT).
- Calculates and displays average waiting and turnaround times.
- Reads processes from an input file.
- Provides detailed execution logs.

Building the Project
--------------------
Compile the project using g++ or another C++ compiler:

    g++ -o cpu_scheduler scheduler.cpp

Running the Simulator
---------------------
Use the following command format:

    ./cpu_scheduler <POLICY> [QUANTUM] <INPUT_FILE>

- <POLICY>: Scheduling algorithm to use (FCFS, SJF, RR).
- [QUANTUM]: Optional quantum time in milliseconds (required for RR and PRIO).
- <INPUT_FILE>: Path to the input file with process data.

Input File Format
-----------------
Each line in the input file should be formatted as:

    <process_name> <priority> <burst_time>

- process_name: Identifier string for the process.
- priority: Integer priority (lower number means higher priority).
- burst_time: CPU burst time in seconds (integer).

Signals and Handlers
--------------------
- SIGCHLD: Handles termination of child processes.
- SIGTSTP: Stops the currently running child process.
- SIGCONT: Resumes a stopped child process.

Code Structure
--------------
- ProcessProfile: Stores process info and scheduling metadata.
- Queue: Doubly linked list to track processes.
- Scheduling algorithms: FCFS(), SJF(), RR().
- Helper functions: Process management, signal handling, and time calculations.
- Main function: Parses command line arguments, loads processes, sets up signals, and starts scheduling.

Limitations and Notes
---------------------
- Priority scheduling (PRIO) is planned but currently disabled.
- Quantum values for RR and PRIO are converted from milliseconds to seconds internally.
- Process execution is simulated using sleep().
- Requires a POSIX-compliant terminal for proper signal handling.
- Includes basic error handling for file operations and process management.

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <cmath>
#include <signal.h>
#include <cstring>

using namespace std;
//---------------------------------------   GLOBAL VARIABLES  AND DATA STRUCTURES TO HOLD THE DATA --------------------------------------------

enum ImplementationStatus
{
    READY,
    RUNNING,
    STOPPED,
    EXITED
};

// process control block (PCB)
struct ProcessProfile
{
    std::string name;
    int burst_time;
    int priority;
    int pid;
    int remain_time;
    double turnaround_time;
    double total_waiting_time;
    int startTime;
    int endTime;
    ImplementationStatus status;
};

struct Node
{
    ProcessProfile processDescription;
    Node *prev;
    Node *next;
};

struct Queue
{
    Node *head;
    Node *tail;
};

void InitializeQueue(Queue &queue)
{
    queue.head = nullptr;
    queue.tail = nullptr;
}

std::string policy;
std::string inputFilename;
int PROCESS_NUM;
int quantum = -1;
ProcessProfile* currentProcessPtr = nullptr;
Queue *globalQueuePtr;
pid_t currentPID;
bool flag = false;

// ----------------------------------   FUNCTIONS   --------------------------------------

//***********************************     adding and removing process from the queue    ****************************************

void DeleteProcess(Queue &queue, pid_t pidToDelete)
{
    Node *current = queue.head;

    while (current != nullptr)
    {
        if (current->processDescription.pid == pidToDelete)
        {
            // found the process with the specified PID, remove it from the queue
            if (current->prev != nullptr)
            {
                current->prev->next = current->next;
           }
            else
            {
              
                queue.head = current->next;
           }

            if (current->next != nullptr)
            {
                current->next->prev = current->prev;
           }
            else
            {
                
                queue.tail = current->prev;
           }

            delete current; 

            return;
       }

        current = current->next;
   }

    // if the process with the specified PID was not found in the queue
    std::cerr << "Process with PID " << pidToDelete << " not found in the queue." << std::endl;
}

void AddProcess(Queue &queue, const ProcessProfile &process)
{
    // create a new node for the new process
    Node *newNode = new Node;

    if (newNode == nullptr)
    {
       std::cout << "Error: Memory allocation failed ." << std::endl;
        exit(1);
   }

    // initialize the new node
    newNode->processDescription = process;
    newNode->prev = nullptr;
    newNode->next = nullptr;

    if (queue.head == nullptr)
    {
        // if the queue is empty, initialize the first node
        queue.head = newNode;
        queue.tail = newNode;
   }
    else
    {
        // add  new node
        newNode->prev = queue.tail;
        queue.tail->next = newNode;
        queue.tail = newNode;
   }
}

// *************************  parse the arguments from the command line  ********************************

bool ParseArguments(int argc, char *argv[])
{
    if (argc < 3 || argc > 4)
    {
       std::cout << "Wrong Format!" << std::endl;
        return false;
   }

    policy = argv[1];

    inputFilename = argv[argc - 1];
    quantum = -1;

    if (argc == 4)
    {
        if ((policy == "RR" || policy == "PRIO"))
        {
            int quantum_milliseconds = std::stoi(argv[2]); // convert to integer
            quantum = quantum_milliseconds / 1000;
            if (quantum <= 0)
            {
               std::cout << "Quantum value is missing!" << std::endl;
                return false;
           }
       }
        else
        {
           std::cout << "Only RR or PRIO require quantum" << std::endl;
            return false;
       }
   }

    return true;
}

// ************************ insert the process from the input file to queue  ***************************
void InsertProcessQueue(Queue &queue, const std::string &inputFilename)
{
    std::ifstream inputFile(inputFilename);
    if (!inputFile.is_open())
    {
       std::cout << "Error opening input file." << std::endl;
        return;
   }

    std::string line;
    while (std::getline(inputFile, line))
    {
        PROCESS_NUM++;

        std::istringstream iss(line);
        std::string process_name;
        int prior;
        int bursttime;

        if (iss >> process_name >> prior >> bursttime)
        {
            ProcessProfile process;
            process.name = process_name;
            process.pid = -1;
            process.status = ImplementationStatus::READY;
            process.priority = prior;
            process.burst_time = bursttime;
            process.turnaround_time = 0;
            process.total_waiting_time = 0;
            process.remain_time = process.burst_time;

            AddProcess(queue, process);
       }
   }

    inputFile.close();

}
//   others 
void SetCurrentProcess(ProcessProfile& process) {
    if (currentProcessPtr == nullptr) {
        currentProcessPtr = &process;
     //   std::cout << "currentProcessPtr is now pointing to PID: " << process.pid << std::endl;
    } else {
        std::cout<<"NULL PTR\n";
    }
}

void ProcessInfo(ProcessProfile &process)
{
    std::cout << "-----------------------------------------\n" << flush;
    std::cout << "| Process name: " << process.name << "           |\n";
    std::cout << "| Process id: " << process.pid << "                      |\n";
    std::cout << "| Start Time: " << process.startTime << "    seconds              |\n";
    std::cout << "| End Time: " << process.endTime << "         seconds           |\n";
    std::cout << "| Burst Time: " << process.burst_time << "     seconds             |\n";
    std::cout << "| Remaining Time: " << process.remain_time << "     seconds             |\n";
    std::cout << "| Turnaround Time: " << process.turnaround_time << " seconds            |\n";
    std::cout << "| Waiting Time: " << process.total_waiting_time << " seconds               |\n";
    std::cout << "-----------------------------------------\n";

}

void TerminationHandler(int signo)
{

    if (currentPID > 0  )
    { 
        if(kill(currentPID,SIGCHLD)== 0)
         {
            std::cout << "Termination Handler called." << std::endl;
            currentProcessPtr->status = ImplementationStatus::EXITED;
            std::cout << "TerminationHandler: Process with PID " << currentPID << " has terminated.\n";

            ProcessInfo(*currentProcessPtr);
            DeleteProcess(*globalQueuePtr, currentProcessPtr->pid);}
                
           }
            
       
}

void ContinueHandler(int signum) {
    if (currentPID > 0) {
        std::cout << "Continue Handler called." << std::endl << std::flush;
        if (kill(currentPID, SIGCONT) == 0) { 
            std::cout << "Process with PID " << currentPID << " has been resumed in " << currentProcessPtr->endTime<<" seconds ."<<std::endl << std::flush;
            currentProcessPtr->status = ImplementationStatus::RUNNING;
        } else {
            std::cerr << "Error stopping process with PID " << currentPID << ". Errno: " << errno << std::endl << std::flush;
        }
    }
}

int CalculateExecutionTime(int quantum, int remain_time) {
    int executionTime;
     if (quantum > remain_time)
        {
           executionTime = remain_time;
        }
        else
        {       
            executionTime = quantum;
        }
    
    return executionTime;
}

void StopHandler(int signum) {
    
    if (currentPID > 0 && flag == true) {
        
        if (kill(currentPID, SIGTSTP) == 0) {
            std::cout << "Stop Handler called." << std::endl << std::flush;
            std::cout << "Process with PID " << currentPID<< " has been stopped in " <<currentProcessPtr->endTime<<" seconds ."<< std::endl << std::flush;
            currentProcessPtr->status = ImplementationStatus::STOPPED;}
          //  std::cout<<currentProcessPtr->status<<"\n";}
            else
            {
            std::cerr << "Error stopping process with PID " << currentPID << ". Errno: " << errno << std::endl << std::flush;}
        }

}
/*
void TerminationHandler(int signo)
{

   std::cout << "Termination Handler called." << std::endl;
    if (globalQueuePtr == nullptr)
    {
       std::cout << "globalQueuePtr is null!" << std::endl;
        return;
   }
    Queue &queue = *globalQueuePtr;
    Node *process = queue.head;

    if (currentPID > 0)
    {
        while (process != nullptr)
        {
            if (process->processDescription.pid == currentPID)
            {
                process->processDescription.status = ImplementationStatus::EXITED;
                ProcessInfo(process->processDescription);
                DeleteProcess(*globalQueuePtr, currentPID);

                std::cout << "TerminationHandler: Process with PID " << currentPID << " has terminated.\n";
                
                break;
           }
            process = process->next;
       }
   }
    else
    {
        // Error occurred (pid < 0)
        std::cerr << "TerminationHandler: Error in waitpid. Errno: " << errno << "\n";
   }
}*/

void PrintStats(double totalWaitTime, double totalTurnaroundTime)
{

    double averageWaitTime = totalWaitTime / PROCESS_NUM;
    double averageTurnaroundTime = totalTurnaroundTime / PROCESS_NUM;

   std::cout << "\nAverage Waiting Time: " << averageWaitTime<<" seconds";
   std::cout << "\nAverage Turnaround Time: " << averageTurnaroundTime<<" seconds" ;
   std::cout << "\n";
}

bool EmptyQueue(const Queue &queue)
{
    return queue.head == nullptr;
}

Node *ShortestProcess(const Queue &queue)
{
    Node *shortest = nullptr;
    Node *process = queue.head;

    while (process != nullptr)
    {
        if (shortest == nullptr || process->processDescription.burst_time < shortest->processDescription.burst_time)
        {
            shortest = process;
       }

        process = process->next; // Move to the next process in the queue
   }

    return shortest;
}

int CountProcesses(const Queue &queue)
{
    int count = 0;
    Node *process = queue.head;

    while (process != nullptr)
    {
        count++;
   }

    return count;
}

void SimulateProcess(ProcessProfile &process, int sleep_time)
{
    if (sleep_time > 0)
    {
        std::cout << "Executing process: " << process.name << " for " << sleep_time << " seconds." << std::endl;
        sleep(sleep_time);

    }
    else
    {
       std::cout << "Executing process: " << process.name << " for " << process.burst_time << " seconds." << std::endl;
        sleep(process.burst_time);
       std::cout << "Process " << process.name << " completed." << std::endl;
   }
}
/*
void  ContinueHandler(int signum){
    std::cout << "Continue Handler called." << std::endl;
    if (globalQueuePtr == nullptr)
    {
       std::cout << "globalQueuePtr is null!" << std::endl;
          return;
    }
    Queue &queue = *globalQueuePtr;
    Node *process = queue.head;


    if ( currentPID > 0)
    {
        while (process != nullptr)
        {
            if (process->processDescription.pid == currentPID && process->processDescription.status == ImplementationStatus::STOPPED )
            {
                process->processDescription.status = ImplementationStatus::RUNNING;
                std::cout << "Process with PID " << process->processDescription.pid << " has been resumed in " << process->processDescription.endTime << " seconds." << std::endl;
                break; // Stop the first running process found
           }    
            process = process->next;
       }
   }
    else
    {
        // Error occurred (pid < 0)
        std::cerr << " ContinueHandler: Error in waitpid. Errno: " << errno << endl;
    }

}



void  StopHandler(int signum){
    
  if (currentPID > 0 )
  {  
    std::cout << "Stop Handler called." << std::endl;
    if (globalQueuePtr == nullptr)
    {
       std::cout << "globalQueuePtr is null!" << std::endl;
          return;
    }
    Queue &queue = *globalQueuePtr;
    Node *process = queue.head;

    
        while (process != nullptr)
        {
            if (process->processDescription.pid == currentPID && process->processDescription.status == ImplementationStatus::RUNNING)
            {
               process->processDescription.status = ImplementationStatus::STOPPED;
                std::cout << "Process with PID " << process->processDescription.pid << " has been stopped in " << process->processDescription.endTime << " seconds." << std::endl;
                break; // Stop the first running process found
           }    
            process = process->next;
       }
   }

}
*/

pid_t ForkChild(const ProcessProfile &currentProcess)
{
    pid_t pid = fork();

   if (pid == -1)
    {
       
        std::cout << "Fork failed for process " << currentProcess.name << std::endl;
        exit(1); 
    }
    else if (pid == 0)
    {
        // child Process
        return 0; 
    }
    else
    {
        // parent Process
        return pid; //
    }
    return pid;
}



void ChildExecutionSimulation(ProcessProfile &currentProcess, int sleep_time)
{
    
    std::cout << "Child process started. PID: " << getpid() << std::endl;
    // Simulate the process execution
    SimulateProcess(currentProcess, sleep_time);
    std::cout << "Child process with PID " << getpid() << " completed." << std::endl;

    exit(0);
    
}

void ParentWaitChld(ProcessProfile &currentProcess,int var)
{
    int status;
    
    if( var > 1)
    {
        std::cout << "Parent process with the child process with PID  " << currentProcess.pid << std::endl;
        // Wait for the child process to finish
        waitpid(currentProcess.pid, &status, 0);
    }
    if (var == 0 )
    {
    std::cout << "Parent process has forked a child process with PID and waits  " << currentProcess.pid << " to finish..." <<std::endl;
    // Wait for the child process to finish
    waitpid(currentProcess.pid, &status, 0);
    std::cout << "Parent process finished waiting for child process with PID " << currentProcess.pid << "." << std::endl;
    }
    if (WIFEXITED(status))
    {
        printf("Child exited with status of %d.\n\n", WEXITSTATUS(status));
        sleep(2);
   }
    else if (WIFSIGNALED(status))
    {
        printf("Child was interrupt by signal %d.\n\n", WTERMSIG(status));
        sleep(2);
   }
    else if (WIFSTOPPED(status))
    {
        printf("Child was stopped by signal %d.\n\n", WSTOPSIG(status));
        sleep(2);
   }
    else
    {
        puts("Reason unknown for child termination.");
   }
    

}

// update times

void updateTimesRRPRIO(ProcessProfile &currentProcess, int &executionTime, int &time) {
    
    currentProcess.endTime = time + executionTime;
    currentProcess.turnaround_time = currentProcess.endTime - currentProcess.startTime;
    currentProcess.total_waiting_time = currentProcess.turnaround_time - currentProcess.burst_time;
}


// update times
void UpdateTimesFCSJ(ProcessProfile &currentProcess)
{
    
    currentProcess.endTime = currentProcess.burst_time + currentProcess.startTime;


    double turnaroundTime = double(currentProcess.endTime - currentProcess.startTime);
    currentProcess.turnaround_time = turnaroundTime;

   
    currentProcess.total_waiting_time = currentProcess.startTime;
    currentProcess.remain_time -= currentProcess.burst_time;
}

// function to handle process termination or send SIGSTOP signal
void TerminateOrStop(ProcessProfile &currentProcess) {
    std::cout<<currentProcess.remain_time<<endl;
    if (currentProcess.remain_time == 0) {
        ParentWaitChld(currentProcess,0);
        exit(0);
   } else {
        
     kill(currentProcess.pid, SIGTSTP);
   }
}


// ------------------------ POLICIES -----------------------

void FCFS(Queue &queue)
{
    double totalWaitTime;
    double totalTurnaroundTime;
    int time = 0;

    std::cout << "#####################   FCFS POLICY: INFORMATION ABOUT EACH PROCESS  ##################### \n\n";

    Node *process = queue.head;

    while (process != nullptr)
    { // loop through the processes in the queue
        ProcessProfile &currentProcess = process->processDescription;
        currentProcess.startTime = time;
        pid_t pid = ForkChild(currentProcess);
      //  int status;

        if (pid == 0)
        { // child

            ChildExecutionSimulation(currentProcess,0);
       }
        else
        { // parent
            currentProcess.status = ImplementationStatus::RUNNING;
            currentProcess.pid = pid;
            currentPID = currentProcess.pid;

            UpdateTimesFCSJ(currentProcess);
            ParentWaitChld(currentProcess,0);
           
            totalWaitTime += currentProcess.total_waiting_time;
            totalTurnaroundTime += currentProcess.turnaround_time;  
       } 

            time += currentProcess.burst_time;
            process = process->next; 
           
   }
    PrintStats(totalWaitTime, totalTurnaroundTime);
}

// SJF policy
void SJF(Queue &queue)
{
    double totalWaitTime = 0;
    double totalTurnaroundTime = 0;
    int time = 0;

    std::cout << "#####################   SJF POLICY: INFORMATION ABOUT EACH PROCESS  ##################### \n\n";

    Node *process = queue.head;

    while (!EmptyQueue(queue))
    { // Continue until all processes are executed
        Node *shortest = ShortestProcess(queue);

        if (shortest == nullptr)
        {
            break; // No process found, break the loop
       }

        ProcessProfile &currentProcess = shortest->processDescription;
        currentProcess.startTime = time;
        pid_t pid = ForkChild(currentProcess);

        if (pid == 0)
        {
            ChildExecutionSimulation(currentProcess,0);
           
       }
        else
        {
           // parent
            currentProcess.status = ImplementationStatus::RUNNING;
            currentProcess.pid = pid;
            currentPID = currentProcess.pid;

            UpdateTimesFCSJ(currentProcess);
            ParentWaitChld(currentProcess,0);
           
            totalWaitTime += currentProcess.total_waiting_time;
            totalTurnaroundTime += currentProcess.turnaround_time; 
       }

        time += currentProcess.burst_time;

   }

    // Print statistics
    PrintStats(totalWaitTime, totalTurnaroundTime);
}

void RR(Queue &queue)
{
    double totalTurnaroundTime = 0;
    double totalWaitingTime = 0;
    int time = 0;
    int iterations = 0;
    int executionTime;

   

    std::cout << "##################### RR POLICY: INFORMATION ABOUT EACH PROCESS ##################### \n\n\n";
    Node *process = queue.head;
    
    std::vector<ProcessProfile> Processes;

  
while (process != nullptr  && Processes.size() < PROCESS_NUM )//first iteration
{
        ProcessProfile &currentProcess = process->processDescription;
        
        executionTime = CalculateExecutionTime(quantum,currentProcess.remain_time);

        pid_t pid = ForkChild(currentProcess);
        if (pid == 0)
        { 
            currentProcess.pid = getpid();
            SetCurrentProcess(currentProcess);
            std::cout << "Child process started with PID: " << getpid() << std::endl;
            SimulateProcess(currentProcess, executionTime);
            flag = true;
            currentProcess.remain_time -= executionTime;
            _exit(0);
            
        }
        else
        {
            // parent Process
            currentProcess.status = ImplementationStatus::RUNNING;
            currentProcess.pid = pid;
            currentPID = currentProcess.pid;
           
   
            updateTimesRRPRIO(currentProcess, executionTime, time);

            totalTurnaroundTime += currentProcess.turnaround_time;
            totalWaitingTime += currentProcess.total_waiting_time;

            time += executionTime;
           
            if(currentProcess.remain_time > 0 && currentProcess.status == ImplementationStatus::RUNNING)
            {
                  
                    kill(currentProcess.pid, SIGTSTP);
    
            }
            else
            {
                
                exit(0);
            }
      
        }
        Processes.push_back(currentProcess);
        process = process->next;
        
}


while (!EmptyQueue(queue))
{
    for (size_t i = 0; i < Processes.size(); ++i)
    {
        ProcessProfile &currentProcess = Processes[i];

        if (currentProcess.status == ImplementationStatus::STOPPED)
        {
            // resume the process
            if (kill(currentProcess.pid, SIGCONT) == 0)
            {
                std::cout << "Resumed process with PID " << currentProcess.pid << std::endl;
                currentProcess.status = ImplementationStatus::RUNNING;

                executionTime = CalculateExecutionTime(quantum,currentProcess.remain_time);
               
                SimulateProcess(currentProcess, executionTime);

                currentProcess.remain_time -= executionTime;

                if (currentProcess.remain_time > 0)
                {
                    // If the process still has remaining time, stop it
                    kill(currentProcess.pid, SIGTSTP);
                    currentProcess.status = ImplementationStatus::STOPPED;
                }
                else
                {
                    // process complete
                
                    exit(0);
                }
            }
           
        }
    }
}
    
}

Node *HighestPriority(Queue &queue) {
    Node *highestPriority = nullptr;
    Node *process = queue.head;

    while (process != nullptr) {
        if (highestPriority == nullptr || process->processDescription.priority < highestPriority->processDescription.priority) {
            highestPriority = process;
        }

        process = process->next;
    }

    return highestPriority;
}            
 /*         
// PRIO policy
void PRIO(Queue &queue)
{
    double totalWaitTime = 0;
    double totalTurnaroundTime = 0;
    int time = 0;

    cout << "#####################   PRIO POLICY: INFORMATION ABOUT EACH PROCESS  ##################### \n\n";

    while (!EmptyQueue(queue))
    {
        Node *highestPriority = HighestPriority(queue); // Get the process with the highest priority

        if (highestPriority == nullptr) {
            break;
        }

        ProcessProfile &currentProcess = highestPriority->processDescription;

        if (currentProcess.status == ImplementationStatus::READY)
        {
            currentProcess.startTime = time;
            pid_t pid = ForkChild(currentProcess);

            if (pid == 0) {
                ChildExecutionSimulation(currentProcess, 0);
            } else {
                // parent
                currentProcess.status = ImplementationStatus::RUNNING;
                currentProcess.pid = pid;
                currentPID = currentProcess.pid;

                // Calculate the execution time (quantum or remaining time, whichever is smaller)
                int executionTime = std::min(quantum, currentProcess.remain_time);

                SimulateProcess(currentProcess, executionTime);

                // Update times and status
                updateTimesRRPRIO(currentProcess, executionTime, time);

                // Check if the process is completed or needs to be stopped
                if (currentProcess.remain_time > 0) {
                    // Send SIGTSTP to stop the process
                    kill(currentProcess.pid, SIGTSTP);
                    currentProcess.status = ImplementationStatus::STOPPED;
                    std::cout<<currentProcess.pid<<"has status "<<currentProcess.status<<"\n";
                } else {
                    // Process is completed, terminate it
                    ParentWaitChld(currentProcess, 1); // Wait for the child process to finish
                }

                totalTurnaroundTime += currentProcess.turnaround_time;
                totalWaitTime += currentProcess.total_waiting_time;
            }

            time += executionTime;
        }
    }

    // Print statistics
    PrintStats(totalWaitTime, totalTurnaroundTime);
}

*/
// ******************************************   main   ******************************************

int main(int argc, char *argv[])
{
   std::cout << "------------------------------- WELCOME TO THE CPU SCHEDULER --------------------------------------\n\n\n";

    // first take the commant line arguments from the user
    if (!ParseArguments(argc, argv))
    {
        return 1;
   }

    // initialize the queue
    Queue queue;
    InitializeQueue(queue);

    // insert processes from the input file into the queue
    InsertProcessQueue(queue, inputFilename);
    globalQueuePtr = &queue;

   // define the signals
    struct sigaction sa;
    sa.sa_handler = TerminationHandler;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

    if (sigaction(SIGCHLD, &sa, nullptr) == -1)
    {

        perror("Error setting up signal TerminationHandler");
        exit(1);
   }
    else
    {
       std::cout << "Signal TerminationHandler set up." << std::endl;
   }

    // Set up signal handler for SIGCONT (ContinueHandler)
    struct sigaction continueAction;
    continueAction.sa_handler = ContinueHandler;
    continueAction.sa_flags = 0;
    if (sigaction(SIGCONT, &continueAction, nullptr) == -1)
    {
        perror("Error setting up signal ContinueHandler");
        exit(1);
   }
    else
    {
       std::cout << "Signal ContinueHandler set up."
                  << std::endl;
   }

    struct sigaction stopAction;
    stopAction.sa_handler = StopHandler;
    stopAction.sa_flags = 0;
    if (sigaction(SIGTSTP, &stopAction, nullptr) == -1)
    {
        perror("Error setting up signal StopHandler");
        exit(1);
   }
    else
    {
       std::cout << "Signal StopHandler set up.\n"
                  << std::endl;
   }


    // apply the scheduling policy based on the user preference
    if (policy == "FCFS")
    {
        FCFS(queue);
        sleep(4);
   }
    else if (policy == "SJF")
    {
        SJF(queue);
        sleep(4);
   } 
     else if (policy == "RR")
     {
        RR(queue);
        sleep(4);
    }/*
     else if (policy == "PRIO")
     {
         PRIO(queue);
         sleep(4);
    }*/
    else
    {
       std::cout << "Invalid policy." << std::endl;
        return 1;
   }

   std:cout<<" #################### CPU SCHEDULER IS DONE !!!!!!!!!!!!!!  #######################";

    return 0;
}

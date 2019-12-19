#include "headers.h"
#include <signal.h>
#include<math.h>

#define KEY_PATH "/home/ahmedwael205/Desktop/OS-Phase1&2/OS_Scheduler/code/tmp/mqueue_key"
#define ID 'M'
#define MAX 10000

FILE * pFile;
FILE * pFile2;
FILE * perf;

int maxFinish = 0;
int minStart = 9999999;
int sumRuningTime = 0;


//Process related Functions
bool StartProcess(struct PCB p);
bool PauseProcess(struct PCB p);
bool ResumeProcess(struct PCB p);
void alarmHandler(int signum);
void ChildSignal(int signum);
void clearResources(int);

//Algorithm chosen Functions
void HPF(int qid, int count,struct FreeListHead* FreeListHead);
void SRTN(int qid, int count,struct FreeListHead* FreeListHead);
void RR(int qid, int count ,int Q,struct FreeListHead* FreeListHead);

//Function 3ashwa2eya 
struct PCB GetProcess(struct Queue* q);

//Handlers
//void ChildTerminated();

int main(int argc, char * argv[])
{
    signal(SIGUSR1, ChildSignal);
    signal(SIGALRM, alarmHandler);
    signal(SIGINT, clearResources);

    initClk();
    int x = getClk();
    printf("current time  at Scheduler is %d\n", x);

    //Open scheduler file to write first line
    pFile = fopen("scheduler.log", "w");
    pFile2 = fopen("memory.log", "w");
    FreeMemLog = fopen("FreeMemLog.log", "w");
    AllocDeallocLog = fopen("AllocDeallocLog.log","w");
    perf = fopen("scheduler.perf", "w");
    fprintf(pFile, "#At time x process y state arr w total z remain y wait k\n");
    fprintf(pFile2, "#At time x allocated y bytes for process z from i to j\n");

    //Message Queue Initialization
    key_t key = ftok(KEY_PATH, ID);
    if (key == -1) {
        perror ("ftok");
        exit (-1);
    }
    //printf("Key = %d\n", key);

    qid = msgget(key, IPC_EXCL| 0644);
    if (qid == -1) {
        perror ("msgget");
        exit (-1);
    }
    //printf("Queue ID = %d\n", qid);

    int count = atoi(argv[0]);
    int algo = atoi(argv[1]);
    //printf("Algorithm Chosen = %d\n", algo);
    int Q = 0;
    if(algo == 3)
    {
        Q = atoi(argv[2]);
        //printf("Q = %d\n", Q);
    }

    struct Tree* root = (struct Tree*)malloc(sizeof(struct Tree));
    root->depth = 0;
    root->FreeSpaceSize = 1024;
    root->start = 0;
    root->end = 1023;
    root->left = NULL;
    root->right = NULL;
    struct FreeListHead* FreeListHead  = CreateList();
    InsertFreeSpace(FreeListHead,root);
    if(FreeListHead->front == NULL)
    {
        fprintf(FreeMemLog,"Error FreeList didn't Insert root\n");
        destroyClk(true);
    }
    fprintf(FreeMemLog,"FreeList root FreeSpace = %d\n",FreeListHead->front->Tree->FreeSpaceSize);

    if(algo == 1)
    {
        HPF(qid, count,FreeListHead);
    }
    else if(algo == 2)
    {
        SRTN(qid, count,FreeListHead);
    }
    else if(algo == 3)
    {
        RR(qid, count, Q,FreeListHead);
    }

    
    fclose(pFile);  
    fclose(pFile2);
    fclose(perf); 
    fclose(FreeMemLog);
    fclose(AllocDeallocLog);
    printf("Scheduler Destroying Clock\n"); 
    destroyClk(true);
}

bool AllocMem(struct PCB p)
{
    fprintf(pFile2, "#At time %d allocated %d bytes for process %d from %d to %d\n",getClk(),p.memsize,p.id,p.mem->start,p.mem->end);
    return true;
}

bool FreeMem(struct PCB p)
{
    fprintf(pFile2, "#At time %d freed %d bytes for process %d from %d to %d\n",getClk(),p.memsize,p.id,p.mem->start,p.mem->end);
    return true;
}

bool StartProcess(struct PCB p)
{
    fprintf(pFile,"At time %d process %d started arr %d total %d remain %d wait %d\n",getClk(), p.id, p.arrivaltime, p.runningtime, p.remainingtime, p.waitingtime );
    return true;
}


bool PauseProcess(struct PCB p)
{
    fprintf(pFile,"At time %d process %d stopped arr %d total %d remain %d wait %d\n",getClk(), p.id, p.arrivaltime, p.runningtime, p.remainingtime, p.waitingtime );
    kill(p.pid, SIGSTOP);
    return true;
}


bool ResumeProcess(struct PCB p)
{
    fprintf(pFile,"At time %d process %d resumed arr %d total %d remain %d wait %d\n",getClk(), p.id, p.arrivaltime, p.runningtime, p.remainingtime, p.waitingtime );
    kill(p.pid, SIGCONT);
    return true;
}

bool FinishProcess(struct PCB p)
{
    fprintf(pFile,"At time %d process %d finished arr %d total %d remain %d wait %d TA %d WTA %0.2f\n",getClk(), p.id, p.arrivaltime, p.runningtime, p.remainingtime, p.waitingtime,p.TA ,(float)p.TA/p.runningtime );
    kill(p.pid, SIGCONT);
    FreeMem(p);
    return true;
}


int curr_ID = -1;
int curr_PID = -1;
int childTerminated = 0;

void RR(int qid, int count ,int Q,struct FreeListHead* FreeListHead)
{
    int rec_val;
    int pid;
    int RecievedCount = 0;

    struct msgbuff rcvmsg;
    struct Node* Process;
    struct Node p;
    struct Queue* ReadyQueue = CreateQueue();
    int count2 = count;

    struct PCB* PCB_Array = (struct PCB*)malloc(count * sizeof(struct PCB));
    while (count !=0 || ReadyQueue->front != NULL)
    {
        if(ReadyQueue->front == NULL)
        {
            printf("Waiting For Process arrival ...\n");
            rec_val = msgrcv(qid,&rcvmsg, sizeof(rcvmsg.p), 7, !IPC_NOWAIT);
            if(rec_val != -1)
            {
                printf("Current Clock = %d\n", getClk());
                printf("Received Message at Scheduler = %d\t%d\t%d\t%d\t%d\n", rcvmsg.p.id, rcvmsg.p.arrivaltime, rcvmsg.p.runningtime, rcvmsg.p.priority,rcvmsg.p.memsize);
                PCB_Array[rcvmsg.p.id-1] = rcvmsg.p;
                p.id = rcvmsg.p.id;
                p.pid = -1;
                p.remainingtime = rcvmsg.p.runningtime;
                PCB_Array[p.id  - 1].mem  = AllocateMemory(FreeListHead,PCB_Array[p.id  - 1]);
                fprintf(FreeMemLog,"After Allocation:\n");
                printFreeSpaces(FreeListHead);
                AllocMem(PCB_Array[p.id  - 1]);
                Enqueue(ReadyQueue, p); 
                RecievedCount++;
                count--;
            }
        } // end if (ReadyQueue->front == NULL)
        else
        {
            Process = Dequeue(ReadyQueue);
            PCB_Array[Process->id - 1].status = RUNNING; // RUNNING = 1
            curr_ID = Process->id;
            if (Process->pid == -1)
            {
                pid = fork();
                if (pid == -1 )perror("Error in forking");
                else if(pid == 0)
                {
                    printf("Forking New Process with ID = %d ....\n",curr_ID);
                    Process->pid = getpid();
                    char r[8];
                    sprintf(r, "%d", Process->remainingtime);
                    char *argvv[] = {r,NULL };
                    execv("./process.out",argvv);
                }
                else 
                {
                    PCB_Array[curr_ID - 1].pid = pid;
                    int z = getClk();
                    if(z< minStart) minStart = z;
                    StartProcess(PCB_Array[curr_ID - 1]);
                    curr_PID = pid;
                    childTerminated = 0;
                    ALARM :
                    printf("Setting Alarm with Quantum = %d, and receiving messages\n",Q);
                    alarm(Q);
                    
                    KEEP_RECIVING:
                    rec_val = msgrcv(qid,&rcvmsg, sizeof(rcvmsg.p), 7, !IPC_NOWAIT);
                    if(rec_val != -1)
                    {
                        printf("Current Clock  = %d\n", getClk());
                        printf("Received Message at Scheduler = %d\t%d\t%d\t%d\t%d\n", rcvmsg.p.id, rcvmsg.p.arrivaltime, rcvmsg.p.runningtime, rcvmsg.p.priority,rcvmsg.p.memsize);
                        PCB_Array[rcvmsg.p.id-1] = rcvmsg.p;
                        p.id = rcvmsg.p.id;
                        p.pid = -1;
                        p.remainingtime = rcvmsg.p.runningtime;
                        PCB_Array[p.id  - 1].mem  = AllocateMemory(FreeListHead,PCB_Array[p.id  - 1]);
                        fprintf(FreeMemLog,"After Allocation:\n");
                        printFreeSpaces(FreeListHead);
                        AllocMem(PCB_Array[p.id  - 1]);
                        Enqueue(ReadyQueue, p); 
                        RecievedCount++;
                        count--;
                        goto KEEP_RECIVING;
                    }
                    else
                    {
                        if (childTerminated == 0)
                        {
                            for (int i = 0; i < RecievedCount; i++)
                            {
                                if (PCB_Array[i].status == READY || PCB_Array[i].status == BLOCKED  )
                                {
                                    PCB_Array[i].waitingtime = getClk() - PCB_Array[i].arrivaltime - (PCB_Array[i].runningtime - PCB_Array[i].remainingtime );
                                }
                            }
                            p.remainingtime = PCB_Array[curr_ID - 1].remainingtime - Q;
                            PCB_Array[curr_ID - 1].remainingtime -= Q;
                            if(ReadyQueue->front == NULL) goto ALARM;
                            printf("Quantum finished ...\n");
                            p.id  = curr_ID;
                            p.pid = curr_PID;
                            p.next = NULL;
                            PCB_Array[curr_ID - 1].status = BLOCKED; // BLOCKED = 2
                            printf("Sending SIGSTOP to process with ID = %d, pid = %d\n",curr_PID,curr_ID);
                            PauseProcess(PCB_Array[curr_ID - 1]);
                            Enqueue(ReadyQueue, p);
                        }
                        else
                        {
                            for (int i = 0; i < RecievedCount; i++)
                            {
                                if (PCB_Array[i].status == READY || PCB_Array[i].status == BLOCKED  )
                                {
                                    PCB_Array[i].waitingtime = getClk() - PCB_Array[i].arrivaltime - (PCB_Array[i].runningtime - PCB_Array[i].remainingtime );
                                }
                            }
                            PCB_Array[curr_ID - 1].remainingtime = 0;
                            PCB_Array[curr_ID - 1].status = FINISHED; // FINISHED = 3
                            FreeMemory(FreeListHead,PCB_Array[curr_ID - 1]);
                            fprintf(FreeMemLog,"After Free Memory:\n");
                            printFreeSpaces(FreeListHead);
                            PCB_Array[curr_ID - 1].TA = getClk() - PCB_Array[curr_ID - 1].arrivaltime;
                            int m = getClk();
                            if(m>maxFinish) maxFinish = m;
                            FinishProcess(PCB_Array[curr_ID - 1]);
                        }
                        
                    }
                    
                }

            } // end if (Process->pid == -1)
            else
            {
                curr_PID = Process->pid;
                printf("Sending SIGCONT to process with ID = %d, pid = %d\n",curr_PID,curr_ID);
                ResumeProcess( PCB_Array[curr_ID - 1]);
                childTerminated = 0;
                goto ALARM;
            }
            
        
        } // end else of (if(ReadyQueue->front == NULL)
    }// end while (count ! = 0 && pr)

    double AVGwaiting = 0;
    double AVGWTA = 0;
    double STD = 0;
    for (int i = 0; i < count2; i++)
    {
        AVGwaiting += PCB_Array[i].waitingtime;
        AVGWTA += (double) (PCB_Array[i].waitingtime / PCB_Array[i].runningtime);
        sumRuningTime += PCB_Array[i].runningtime;
    }
    printf("sumRuningTime = %d, maxFinish = %d, minStart = %d\n",sumRuningTime,maxFinish,minStart);
    double CPU_Utilization = (double) (sumRuningTime/(maxFinish - minStart)) * 100;
    AVGwaiting /= count2;
    AVGWTA /= count2;
    double x = 0;
    double y = 0;
    for (int i = 0; i < count2; i++)
    {
        y = ((double) (PCB_Array[i].waitingtime / PCB_Array[i].runningtime) - AVGWTA);
        x = x + pow(y,2);
    }
    STD = sqrt(x/(count2 - 1));
    fprintf(perf,"CPU utilization = %0.2f \n",CPU_Utilization);
    fprintf(perf,"AVG WTA = %0.2f\n",AVGWTA);
    fprintf(perf,"AVG waiting = %0.2f\n",AVGwaiting);
    fprintf(perf,"STD WTA = %0.2f\n",STD);
}


void HPF(int qid, int count,struct FreeListHead* FreeListHead)
{
    int rec_val;
    int pid;
    int RecievedCount = 0;

    struct msgbuff rcvmsg;
    struct Node* Process;
    struct Node p;
    struct Queue* ReadyQueue = CreateQueue();
    int count2 = count;

    struct PCB* PCB_Array = (struct PCB*)malloc(count * sizeof(struct PCB));
    while (count !=0 || ReadyQueue->front != NULL)
    {
        if(ReadyQueue->front == NULL)
        {
            printf("Waiting For Process arrival ...\n");
            rec_val = msgrcv(qid,&rcvmsg, sizeof(rcvmsg.p), 7, !IPC_NOWAIT);
            if(rec_val != -1)
            {
                printf("Current Clock  = %d\n", getClk());
                printf("Received Message at Scheduler = %d\t%d\t%d\t%d\t%d\n", rcvmsg.p.id, rcvmsg.p.arrivaltime, rcvmsg.p.runningtime, rcvmsg.p.priority,rcvmsg.p.memsize);
                PCB_Array[rcvmsg.p.id-1] = rcvmsg.p;
                p.id = rcvmsg.p.id;
                p.pid = -1;
                p.remainingtime = rcvmsg.p.runningtime;
                p.priority = rcvmsg.p.priority;
                PCB_Array[p.id  - 1].mem  = AllocateMemory(FreeListHead,PCB_Array[p.id  - 1]);
                fprintf(FreeMemLog,"After Allocation:\n");
                printFreeSpaces(FreeListHead);
                AllocMem(PCB_Array[p.id  - 1]);
                priorityEnqueue(ReadyQueue, p); 
                RecievedCount++;
                count--;
            }
        } // end if (ReadyQueue->front == NULL)
        else
        {
            Process = Dequeue(ReadyQueue);
            printf("Dequeue Process ID = %d , priority = %d\n",Process->id,Process->priority);
            PCB_Array[Process->id - 1].status = RUNNING; // RUNNING = 1
            curr_ID = Process->id;
            if (Process->pid == -1)
            {
                pid = fork();
                if (pid == -1 )perror("Error in forking");
                else if(pid == 0)
                {
                    printf("Forking New Process with ID = %d ....\n",curr_ID);
                    Process->pid = getpid();
                    char r[8];
                    sprintf(r, "%d", Process->remainingtime);
                    char *argvv[] = {r,NULL };
                    execv("./process.out",argvv);
                }
                else 
                {
                    
                    PCB_Array[curr_ID - 1].pid = pid;
                    int z = getClk();
                    if(z< minStart) minStart = z;
                    StartProcess(PCB_Array[curr_ID - 1]);
                    curr_PID = pid;
                    KEEP_RECEIVING:
                    rec_val = msgrcv(qid,&rcvmsg, sizeof(rcvmsg.p), 7, !IPC_NOWAIT);
                    if(rec_val != -1)
                    {
                        printf("Current Clock  = %d\n", getClk());
                        printf("Received Message at Scheduler = %d\t%d\t%d\t%d\t%d\n", rcvmsg.p.id, rcvmsg.p.arrivaltime, rcvmsg.p.runningtime, rcvmsg.p.priority,rcvmsg.p.memsize);
                        PCB_Array[rcvmsg.p.id-1] = rcvmsg.p;
                        p.id = rcvmsg.p.id;
                        p.pid = -1;
                        p.remainingtime = rcvmsg.p.runningtime;
                        p.priority = rcvmsg.p.priority;
                        PCB_Array[p.id  - 1].mem  = AllocateMemory(FreeListHead,PCB_Array[p.id  - 1]);
                        fprintf(FreeMemLog,"After Allocation:\n");
                        printFreeSpaces(FreeListHead);
                        AllocMem(PCB_Array[p.id  - 1]);
                        priorityEnqueue(ReadyQueue, p); 
                        RecievedCount++;
                        count--;
                        goto KEEP_RECEIVING;
                    }
                    else
                    {
                        for (int i = 0; i < RecievedCount; i++)
                        {
                            if (PCB_Array[i].status == READY || PCB_Array[i].status == BLOCKED  )
                            {
                                PCB_Array[i].waitingtime = getClk() - PCB_Array[i].arrivaltime - (PCB_Array[i].runningtime - PCB_Array[i].remainingtime );
                            }
                        }

                        PCB_Array[curr_ID - 1].remainingtime = 0;
                        PCB_Array[curr_ID - 1].status = FINISHED; // FINISHED = 3
                        FreeMemory(FreeListHead,PCB_Array[curr_ID - 1]);
                        fprintf(FreeMemLog,"After Free Memory:\n");
                        printFreeSpaces(FreeListHead);
                        PCB_Array[curr_ID - 1].TA = getClk() - PCB_Array[curr_ID - 1].arrivaltime;
                        int m = getClk();
                        if(m>maxFinish) maxFinish = m;
                        FinishProcess(PCB_Array[curr_ID - 1]);
                    }
                } // end fork
            } // end else (if (Process->pid == -1))
        }  // end else (if (ReadyQueue->front == NULL)  )    
    } // end while
    double AVGwaiting = 0;
    double AVGWTA = 0;
    double STD = 0;
    for (int i = 0; i < count2; i++)
    {
        AVGwaiting += PCB_Array[i].waitingtime;
        AVGWTA += (double) (PCB_Array[i].waitingtime / PCB_Array[i].runningtime);
        sumRuningTime += PCB_Array[i].runningtime;
    }
    printf("sumRuningTime = %d, maxFinish = %d, minStart = %d\n",sumRuningTime,maxFinish,minStart);

    double CPU_Utilization = (double) (sumRuningTime/(maxFinish - minStart)) * 100;
    AVGwaiting /= count2;
    AVGWTA /= count2;

    double x = 0;
    double y = 0;
    for (int i = 0; i < count2; i++)
    {
        y = ((double) (PCB_Array[i].waitingtime / PCB_Array[i].runningtime) - AVGWTA);
        x = x + pow(y,2);
    }
    STD = sqrt(x/(count2 - 1));
    fprintf(perf,"CPU utilization = %0.2f\n",CPU_Utilization);
    fprintf(perf,"AVG WTA = %0.2f\n",AVGWTA);
    fprintf(perf,"AVG waiting = %0.2f\n",AVGwaiting);
    fprintf(perf,"STD WTA = %0.2f\n",STD);
}


void SRTN(int qid, int count,struct FreeListHead* FreeListHead)
{
    int rec_val;
    int pid;
    int end;
    int RecievedCount = 0;

    struct msgbuff rcvmsg;
    struct Node* Process;
    struct Node p;
    struct Queue* ReadyQueue = CreateQueue();
    int count2  = count;

    struct PCB* PCB_Array = (struct PCB*)malloc(count * sizeof(struct PCB));
    while (count !=0 || ReadyQueue->front != NULL)
    {
        if(ReadyQueue->front == NULL)
        {
            printf("Waiting For Process arrival ...\n");
            rec_val = msgrcv(qid,&rcvmsg, sizeof(rcvmsg.p), 7, !IPC_NOWAIT);
            if(rec_val != -1)
            {
                printf("Current Clock  = %d\n", getClk());
                printf("Received Message at Scheduler = %d\t%d\t%d\t%d\t%d\n", rcvmsg.p.id, rcvmsg.p.arrivaltime, rcvmsg.p.runningtime, rcvmsg.p.priority,rcvmsg.p.memsize);
                PCB_Array[rcvmsg.p.id-1] = rcvmsg.p;
                p.id = rcvmsg.p.id;
                p.pid = -1;
                p.remainingtime = rcvmsg.p.runningtime;
                PCB_Array[p.id  - 1].mem  = AllocateMemory(FreeListHead,PCB_Array[p.id  - 1]);
                fprintf(FreeMemLog,"After Allocation:\n");
                printFreeSpaces(FreeListHead);
                AllocMem(PCB_Array[p.id  - 1]);
                STimeEnqueue(ReadyQueue, p); 
                RecievedCount++;
                count--;
            }
        } // end if (ReadyQueue->front == NULL)
        else
        {
            Process = Dequeue(ReadyQueue);
            printf("Dequeue Process ID = %d , remaining = %d\n",Process->id,Process->remainingtime);
            PCB_Array[Process->id - 1].status = RUNNING; // RUNNING = 1
            curr_ID = Process->id;
            if (Process->pid == -1)
            {
                PREEMPTIVE:
                pid = fork();
                int start = getClk();
                if (pid == -1 )perror("Error in forking");
                else if(pid == 0)
                {
                    printf("Forking New Process with ID = %d ....\n",curr_ID);
                    Process->pid = getpid();
                    char r[8];
                    sprintf(r, "%d", Process->remainingtime);
                    char *argvv[] = {r,NULL };
                    execv("./process.out",argvv);
                }
                else 
                {
                    PCB_Array[curr_ID - 1].pid = pid;
                    int z = getClk();
                    if(z< minStart) minStart = z;
                    StartProcess(PCB_Array[curr_ID - 1]);
                    curr_PID = pid;
                    KEEP_RECEIVING2:
                    rec_val = msgrcv(qid,&rcvmsg, sizeof(rcvmsg.p), 7, !IPC_NOWAIT);
                    if(rec_val != -1)
                    {
                        printf("Current Clock  = %d\n", getClk());
                        printf("Received Message at Scheduler = %d\t%d\t%d\t%d\t%d\n", rcvmsg.p.id, rcvmsg.p.arrivaltime, rcvmsg.p.runningtime, rcvmsg.p.priority,rcvmsg.p.memsize);
                        end = getClk() - start;
                        PCB_Array[rcvmsg.p.id - 1] = rcvmsg.p;
                        PCB_Array[rcvmsg.p.id  - 1].mem  = AllocateMemory(FreeListHead,PCB_Array[rcvmsg.p.id - 1]);
                        
                        fprintf(FreeMemLog,"After Allocation:\n");
                        printFreeSpaces(FreeListHead);
                        AllocMem(PCB_Array[rcvmsg.p.id - 1]);
                        if(rcvmsg.p.runningtime >= Process->remainingtime - end)
                        {
                            p.id = rcvmsg.p.id;
                            p.pid = -1;
                            p.remainingtime = rcvmsg.p.runningtime;
                            STimeEnqueue(ReadyQueue, p); 
                        }
                        else
                        {
                            for (int i = 0; i < RecievedCount; i++)
                            {
                                if (PCB_Array[i].status == READY || PCB_Array[i].status == BLOCKED  )
                                {
                                    PCB_Array[i].waitingtime += end;
                                }
                            }
                            PCB_Array[curr_ID-1].remainingtime -= end;
                            PCB_Array[curr_ID-1].status = BLOCKED; 
                            PauseProcess(PCB_Array[curr_ID-1]);
                            p.id = curr_ID;
                            p.pid = PCB_Array[curr_ID-1].pid;
                            p.remainingtime = PCB_Array[curr_ID-1].remainingtime;
                            STimeEnqueue(ReadyQueue, p);
                            
                            curr_ID = rcvmsg.p.id;
                            curr_PID = rcvmsg.p.pid;
                            Process->id =rcvmsg.p.id;
                            Process->remainingtime = rcvmsg.p.remainingtime;
                            Process->pid = rcvmsg.p.pid;
                            PCB_Array[rcvmsg.p.id-1].status = RUNNING;
                            StartProcess(PCB_Array[rcvmsg.p.id-1]);
                            RecievedCount++;
                            count--;
                            goto PREEMPTIVE;
                            
                        }
                        
                        RecievedCount++;
                        count--;
                        goto KEEP_RECEIVING2;
                    }
                    else
                    {
                        end =getClk() - start;
                        for (int i = 0; i < RecievedCount; i++)
                        {
                            if (PCB_Array[i].status == READY || PCB_Array[i].status == BLOCKED  )
                            {
                                PCB_Array[i].waitingtime += end;
                            }
                        }
                        PCB_Array[curr_ID - 1].remainingtime = 0;
                        PCB_Array[curr_ID - 1].status = FINISHED; // FINISHED = 3
                        FreeMemory(FreeListHead,PCB_Array[curr_ID - 1]);
                        fprintf(FreeMemLog,"After Free Memory:\n");
                        printFreeSpaces(FreeListHead);
                        PCB_Array[curr_ID - 1].TA = getClk() - PCB_Array[curr_ID - 1].arrivaltime;
                        int m = getClk();
                        if(m>maxFinish) maxFinish = m;
                        FinishProcess(PCB_Array[curr_ID - 1]);
                        
                    }
                } // end fork
            }
            else
            {
                curr_PID = Process->pid;
                printf("Sending SIGCONT to process with ID = %d, pid = %d\n",curr_PID,curr_ID);
                ResumeProcess( PCB_Array[curr_ID - 1]);
                goto KEEP_RECEIVING2;
            }   // end else (if (Process->pid == -1)) 
        }  // end else (if (ReadyQueue->front == NULL)  )    
    } // end while

    double AVGwaiting = 0;
    double AVGWTA = 0;
    double STD = 0;
    for (int i = 0; i < count2; i++)
    {
        AVGwaiting += PCB_Array[i].waitingtime;
        AVGWTA += (double) (PCB_Array[i].waitingtime / PCB_Array[i].runningtime);
        sumRuningTime += PCB_Array[i].runningtime;
    }
    printf("sumRuningTime = %d, maxFinish = %d, minStart = %d\n",sumRuningTime,maxFinish,minStart);

    double CPU_Utilization = (double) (sumRuningTime/(maxFinish - minStart)) * 100;
    AVGwaiting /= count2;
    AVGWTA /= count2;

    double x = 0;
    double y = 0;
    for (int i = 0; i < count2; i++)
    {
        y = ((double) (PCB_Array[i].waitingtime / PCB_Array[i].runningtime) - AVGWTA);
        x = x + pow(y,2);
    }
    STD = sqrt(x/(count2 - 1));
    fprintf(perf,"CPU utilization = %0.2f\n",CPU_Utilization);
    fprintf(perf,"AVG WTA = %0.2f\n",AVGWTA);
    fprintf(perf,"AVG waiting = %0.2f\n",AVGwaiting);
    fprintf(perf,"STD WTA = %0.2f\n",STD);
}


void alarmHandler(int signum)
{
    printf("Alarms ...\n");
    return;
}

void ChildSignal(int signum)
{
    childTerminated = 1;
    printf("Child with ID = %d has Terminated, at Time = %d\n",curr_ID,getClk());
    return;
}

void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
    fclose(pFile);  
    fclose(pFile2); 
    fclose(FreeMemLog);
    fclose(AllocDeallocLog);
    destroyClk(true);
    msgctl(qid, IPC_RMID, (struct msqid_ds *) 0);
    raise(SIGKILL); //For Me.
}
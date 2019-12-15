#include "headers.h"
#include <signal.h>

#define KEY_PATH "/home/ahmedwael205/Desktop/OS-Phase1&2/OS_Scheduler/code/tmp/mqueue_key"
#define ID 'M'
#define MAX 10000

FILE * pFile;

//Process related Functions
bool StartProcess(struct PCB p);
bool PauseProcess(struct PCB p);
bool ResumeProcess(struct PCB p);
void alarmHandler(int signum);
void ChildSignal(int signum);

//Algorithm chosen Functions
void HPF(int qid, int count);
void SRTN(int qid, int count);
void RR(int qid, int count ,int Q);

//Function 3ashwa2eya 
struct PCB GetProcess(struct Queue* q);

//Handlers
//void ChildTerminated();

int main(int argc, char * argv[])
{
    signal(SIGUSR1, ChildSignal);
    signal(SIGALRM, alarmHandler);

    initClk();
    int x = getClk();
    printf("current time  at Scheduler is %d\n", x);

    //Open scheduler file to write first line
    pFile = fopen("scheduler.log", "w");
    fprintf(pFile, "#At time x process y state arr w total z remain y wait k\n");

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

    if(algo == 1)
    {
        HPF(qid, count);
    }
    else if(algo == 2)
    {
        SRTN(qid, count);
    }
    else if(algo == 3)
    {
        RR(qid, count, Q);
    } 
    fclose(pFile);  
    printf("Scheduler Destroying Clock\n"); 
    destroyClk(true);
}

bool StartProcess(struct PCB p)
{
    int x = getClk();
    fprintf(pFile,"At time %d process %d started arr %d total %d remain %d wait %d\n",x, p.id, p.arrivaltime, p.runningtime, p.remainingtime, p.waitingtime );
    return true;
}


bool PauseProcess(struct PCB p)
{
    int x = getClk();
    fprintf(pFile,"At time %d process %d stopped arr %d total %d remain %d wait %d\n",x, p.id, p.arrivaltime, p.runningtime, p.remainingtime, p.waitingtime );
    kill(p.pid, SIGSTOP);
    return true;
}


bool ResumeProcess(struct PCB p)
{
    int x = getClk();
    fprintf(pFile,"At time %d process %d resumed arr %d total %d remain %d wait %d\n",x, p.id, p.arrivaltime, p.runningtime, p.remainingtime, p.waitingtime );
    kill(p.pid, SIGCONT);
    return true;
}

bool FinishProcess(struct PCB p)
{
    int x = getClk();
    fprintf(pFile,"At time %d process %d finished arr %d total %d remain %d wait %d TA %d WTA %f\n",x, p.id, p.arrivaltime, p.runningtime, p.remainingtime, p.waitingtime,x - p.arrivaltime ,(float)(x - p.arrivaltime)/p.runningtime );
    kill(p.pid, SIGCONT);
    return true;
}


void SRTN(int qid, int count)
{
    int rec_val;
    struct msgbuff rcvmsg;
    int i = 0;
}

int curr_ID = -1;
int curr_PID = -1;
int childTerminated = 0;

void RR(int qid, int count ,int Q)
{
    int rec_val;
    int pid;
    int RecievedCount = 0;

    struct msgbuff rcvmsg;
    struct Node* Process;
    struct Node p;
    struct Queue* ReadyQueue = CreateQueue();

    struct PCB* PCB_Array = (struct PCB*)malloc(count * sizeof(struct PCB));
    while (count !=0 || ReadyQueue->front != NULL)
    {
        if(ReadyQueue->front == NULL)
        {
            printf("Waiting For Process arrival ...\n");
            rec_val = msgrcv(qid,&rcvmsg, sizeof(rcvmsg.p), 7, !IPC_NOWAIT);
            if(rec_val != -1)
            {
                printf("Clock = %d\n", getClk());
                printf("Received Message at Scheduler = %d\t%d\t%d\t%d\n", rcvmsg.p.id, rcvmsg.p.arrivaltime, rcvmsg.p.runningtime, rcvmsg.p.priority);
                PCB_Array[rcvmsg.p.id-1] = rcvmsg.p;
                p.id = rcvmsg.p.id;
                p.pid = -1;
                p.remainingtime = rcvmsg.p.runningtime;
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
                    StartProcess(PCB_Array[curr_ID - 1]);
                    curr_PID = pid;
                    childTerminated = 0;
                    ALARM :
                    printf("Setting Alarm with Quantum = %d, and receiving messages\n",Q);
                    alarm(Q);

                    for (int i = 0; i < RecievedCount; i++)
                    {
                        if (PCB_Array[i].status == READY || PCB_Array[i].status == BLOCKED  )
                        {
                            PCB_Array[i].waitingtime += Q;
                        }
                    }
                    KEEP_RECIVING:
                    rec_val = msgrcv(qid,&rcvmsg, sizeof(rcvmsg.p), 7, !IPC_NOWAIT);
                    if(rec_val != -1)
                    {
                        printf("Clock2 = %d\n", getClk());
                        printf("Received Message at Scheduler = %d\t%d\t%d\t%d\n", rcvmsg.p.id, rcvmsg.p.arrivaltime, rcvmsg.p.runningtime, rcvmsg.p.priority);
                        PCB_Array[rcvmsg.p.id-1] = rcvmsg.p;
                        p.id = rcvmsg.p.id;
                        p.pid = -1;
                        p.remainingtime = rcvmsg.p.runningtime;
                        Enqueue(ReadyQueue, p); 
                        RecievedCount++;
                        count--;
                        goto KEEP_RECIVING;
                    }
                    else
                    {
                        if (childTerminated == 0)
                        {
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
                            PCB_Array[curr_ID - 1].remainingtime = 0;
                            PCB_Array[curr_ID - 1].status = FINISHED; // FINISHED = 3
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
}


void HPF(int qid, int count)
{
    int rec_val;
    int pid;
    int RecievedCount = 0;

    struct msgbuff rcvmsg;
    struct Node* Process;
    struct Node p;
    struct Queue* ReadyQueue = CreateQueue();

    struct PCB* PCB_Array = (struct PCB*)malloc(count * sizeof(struct PCB));
    while (count !=0 || ReadyQueue->front != NULL)
    {
    }
}
void alarmHandler(int signum)
{
    printf("Alarms ...\n");
    return;
}

void ChildSignal(int signum)
{
    childTerminated = 1;
    printf("Child with ID = %d has Terminated\n",curr_ID);
    return;
}
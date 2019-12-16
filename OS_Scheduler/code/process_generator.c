#include "headers.h"

#define KEY_PATH "/home/ahmedwael205/Desktop/OS-Phase1&2/OS_Scheduler/code/tmp/mqueue_key"
#define ID 'M'
#define MAX 10000


//Signal Handler
void clearResources(int);

int main(int argc, char * argv[])
{
    signal(SIGINT, clearResources);
    // TODO Initialization
    // 1. Read the input files. DONE

    FILE *pFile;
    char str[MAX];
    char* filename = "/home/ahmedwael205/Desktop/OS-Phase1&2/OS_Scheduler/code/processes.txt";
    int count = 0;
    struct PCB p;
    bool flag;
    pFile = fopen(filename, "r");
    if (pFile == NULL){
        printf("Could not open file %s",filename);
        return (-1);
    }

    while (1)
    {
        fgets(str, MAX, pFile);
        fscanf(pFile,"%d %d %d %d", &p.id, &p.arrivaltime, &p.runningtime, &p.priority);
        if(feof(pFile))
            break;
        count++;
    }
    fclose(pFile);

    pFile = fopen(filename, "r");
    if (pFile == NULL){
        printf("Could not open file %s",filename);
        return (-1);
    }
    struct PCB* PCB_Array = (struct PCB*)malloc(count * sizeof(struct PCB));
    int count2 = count;
    while (count2 != 0)
    {
        fgets(str, MAX, pFile);
        fscanf(pFile,"%d %d %d %d %d", &p.id, &p.arrivaltime, &p.runningtime, &p.priority,&p.memsize);
        p.remainingtime = p.runningtime;
        p.waitingtime = 0;
        p.status = READY;
        p.pid = 0;
        if(feof(pFile))
        {
            break;
        }
        PCB_Array[p.id - 1] = p;
        count2--;
        
    }
    //printf("%d\n",count);    
    fclose(pFile);

    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any. DONE

    for (int i = 0; i < count; i++)
    {
        printf("%d\t%d\t%d\t%d\t%d\n", PCB_Array[i].id, PCB_Array[i].arrivaltime, PCB_Array[i].runningtime, PCB_Array[i].priority,PCB_Array[i].memsize);
    }
    // exit(0);

    flag = true;
    printf("Choose one of the following Scheduling Algorithms:\n");
    printf("1. Non-preemptive Highest Priority First (HPF)\n");
    printf("2. Shortest Remaining Time Next (SRTN)\n");
    printf("3. Round Robin (RR)\n");
    int SchID = 0;
    int Q = 0; // Quantum
    printf("Your choice: ");
    scanf("%d", &SchID);

    //Lock User in While loop to make him enter ONLY right answer :)
    while((SchID <= 0 || SchID > 3))
    {
        printf("Wrong Choice! Please re-enter a valid choice\n");
        printf("Your choice: ");
        scanf("%d", &SchID);
    }
   
    if(SchID == 3)
    {
        printf("Please enter the Quantum needed for selected scheduling algorithm\n");
        scanf("%d", &Q);
    }

    //Message Queue Initialization
    key_t key = ftok(KEY_PATH, ID);
    if (key == -1) {
        perror ("ftok");
        exit (-1);
    }
    //printf("Key = %d\n", key);

    qid = msgget(key, IPC_CREAT| 0644);
    if (qid == -1) {
        perror ("msgget");
        exit (-1);
    }
    //printf("Queue ID = %d\n", qid);

    // 3. Initiate and create the scheduler and clock processes. DONE

    int pid, pid1;

    pid1 = fork();
    if (pid1 == -1)
        perror("error in fork\n");
    else if (pid1 == 0)
    {       
        //printf("I am the the clock , my pid = %d and my parent's pid = %d\n\n", getpid(), getppid());
        char *argvv[] = { NULL };
        execv("./clk.out", argvv);
    }
    else
    {
        pid = fork();
        if(pid == -1)
            perror("error in fork\n");
        else if(pid == 0)
        {
            //printf("I am the scheduler , my pid = %d and my parent's pid = %d \n\n", getpid(), getppid());
            //Send argumenta where it means which algorithm was chosen + needed parameters + Process Count
            char c[32];
            sprintf(c, "%d" ,count);
            char algo[8]; 
            sprintf(algo, "%d", SchID);
            char quan[Q];
            sprintf(quan, "%d", Q);
            char *argvv[] = { c,algo, quan ,NULL }; 
            execv("./scheduler.out", argvv);
        }
    }

    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    // To get time use this
    int x = getClk();
    printf("current time is %d\n", x);
    // TODO Generation Main Loop

    int send_val;
    struct msgbuff msg;
    msg.mtype = 7;

    count2 = 0;

    while(count2 != count)
    {
        x = getClk();
        //printf("current time is %d\n", x);
        if(PCB_Array[count2].arrivaltime <= x)
        {
            
            //printf("Dequeued Item ID = %d\n", temp->key.id);
            //send item to scheduler through message queue
            //msg.p = temp->next;
            msg.p.arrivaltime = PCB_Array[count2].arrivaltime;
            msg.p.id = PCB_Array[count2].id;
            msg.p.pid = PCB_Array[count2].pid;
            msg.p.priority = PCB_Array[count2].priority;
            msg.p.remainingtime = PCB_Array[count2].remainingtime;
            msg.p.runningtime = PCB_Array[count2].runningtime;
            msg.p.status = PCB_Array[count2].status;
            msg.p.waitingtime = PCB_Array[count2].waitingtime;
            msg.p.memsize = PCB_Array[count2].memsize;
            count2++;
            send_val = msgsnd(qid, &msg, sizeof(msg.p), !IPC_NOWAIT);
            if(send_val == -1)
                perror("Errror in send");
            else
            {
                // printf("current time is %d\n", x);
                // printf("Sent Item ID = %d\n", msg.p.id);
            }
            
        }
        //printf("current time is %d\n", x);
        
    }
    sleep(1000);
    // 5. Create a data structure for processes and provide it with its parameters. DONE
    // 6. Send the information to the scheduler at the appropriate time. DONE
    // 7. Clear clock resources. DONE
    destroyClk(true);
    msgctl(qid, IPC_RMID, (struct msqid_ds *) 0);
}

void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
    destroyClk(true);
    msgctl(qid, IPC_RMID, (struct msqid_ds *) 0);
    raise(SIGKILL); //For Me.
}
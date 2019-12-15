#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <ctype.h>
#include <signal.h>
#include <sys/msg.h>

#define KEY_PATH "/home/ayahsamadoni/Desktop/OS-Phase1&2/OS_Synchronization/tmp/mqueue_key"

#define ID_S 'S'
#define ID_Q 'Q'
#define ID_M 'M'

#define MemorySize 16*1024

int shmid;
int qid;
int Mutex;

struct msgbuff
{
    long mtype;
    char mtext[16];
};

struct Buffer
{
    int num;
    int bufferSize;
    char Data [MemorySize];
};

/* arg for semctl system calls. */
union Semun
{
    int val;               		/* value for SETVAL */
    struct semid_ds *buf;  	/* buffer for IPC_STAT & IPC_SET */
    short *array;          	/* array for GETALL & SETALL */
    struct seminfo *__buf;  	/* buffer for IPC_INFO */
    void *__pad;
};


void down(int sem)
{
    struct sembuf p_op;

    p_op.sem_num = 0;
    p_op.sem_op = -1;
    p_op.sem_flg = !IPC_NOWAIT;

    if(semop(sem, &p_op, 1) == -1)
    {
        perror("Error in down()");
        exit(-1);
    }
}


void up(int sem)
{
    struct sembuf v_op;

    v_op.sem_num = 0;
    v_op.sem_op = 1;
    v_op.sem_flg = !IPC_NOWAIT;

    if(semop(sem, &v_op, 1) == -1)
    {
        perror("Error in up()");
        exit(-1);
    }
}


void handler(int signum);


int main(int argc, char** argv)
{
    signal(SIGINT, handler);
    // Initialization
    int pid;
    int bufferSize = atoi(argv[1]);
    printf("buffer size = %d\n",bufferSize);

    union Semun semun;

    key_t keyS = ftok(KEY_PATH, ID_S);
    if (keyS == -1) {
        perror ("ftok");
        exit (-1);
    }


    Mutex = semget(keyS, 1, 0666|IPC_CREAT);

    printf("Mutex= %d\n",Mutex);

    if(Mutex == -1)
    {
        perror("Error in create sem");
        exit(-1);
    }


    semun.val = 1; 
    if(semctl(Mutex, 0, SETVAL, semun) == -1)
    {
        perror("Error in semctl");
        exit(-1);
    }

    key_t keyQ = ftok(KEY_PATH, ID_Q);
    if (keyQ == -1) {
        perror ("ftok");
        exit (-1);
    }

    int send_val,rec_val;

    qid = msgget(keyQ, IPC_CREAT| 0666);
    if (qid == -1) {
        perror ("msgget");
        exit (-1);
    }
    printf("Queue ID = %d\n", qid);

    key_t keyM = ftok(KEY_PATH, ID_M);
    if (keyM == -1) {
        perror ("ftok");
        exit (-1);
    }


    printf("size of Memory: %ld\n",sizeof(struct Buffer));
    printf("size of Buffer: %d\n",bufferSize);

    shmid = shmget(keyM, sizeof(struct Buffer), IPC_CREAT|0666);

    if(shmid == -1)
    {
        perror("Error in create");
        exit(-1);
    }
    printf("Shared memory ID = %d\n", shmid);

   
    int prod_index = 0;

    struct Buffer* SharedMemory;
    SharedMemory = (struct Buffer*) shmat(shmid, (struct Buffer*)0, 0);

    if(SharedMemory == -1)
    {
        perror("Error in attach in writer");
        exit(-1);
    }
    else
    {
        printf("\nShared memory attached at address %p\n", SharedMemory);
        SharedMemory->num = 0;
        //SharedMemory->Data = malloc(bufferSize);
        SharedMemory->bufferSize = bufferSize;
    }
    struct msgbuff Sentmessage,Recievedmessage;
    char msg[16] = "Wakeup P->C\n";
    Sentmessage.mtype = 7;

    while(1)
    {
        down(Mutex);
        
        if(SharedMemory->Data [prod_index] =='C')
            printf("Producing P at %d instead of C ...\n",prod_index);
        else
            printf("Producing P at %d ...\n",prod_index);
        SharedMemory->Data [prod_index] = 'P';
        prod_index =(prod_index + 1)% SharedMemory->bufferSize;
        //prod_index++;
    
        SharedMemory->num++;
        if (SharedMemory->num >= SharedMemory->bufferSize)
        {
            printf("Producer Sleeping ...\n");
            up(Mutex);
            rec_val = msgrcv(qid, &Recievedmessage, sizeof(Recievedmessage.mtext), 5, !IPC_NOWAIT);

            if(rec_val == -1)
                perror("Error in receive");
            printf("Consumer message: %s",Recievedmessage.mtext);
        }
        else
        {
            up(Mutex);
        }
        

        if(SharedMemory->num == 1)
        {
            printf("Waking up consumer ...\n");
            strcpy(Sentmessage.mtext, msg);
            printf("message send: %s\n",Sentmessage.mtext);
            send_val = msgsnd(qid, &Sentmessage, sizeof(Sentmessage.mtext), !IPC_NOWAIT);
            if(send_val == -1) perror("Error in send");
        }
    }

    return 0;
}

void handler(int signum)
{
    printf("\nTerminating Producer ...\n");

    printf("Delete Semaphor= %d\nDelete MessageQueue = %d\nDelete SharedMemory = %d\n", Mutex,qid,shmid);

    msgctl(qid, IPC_RMID, (struct msqid_ds *) 0);
    shmctl(shmid, IPC_RMID, (struct shmid_ds*) 0);
    semctl(Mutex, 0,IPC_RMID);
    raise(SIGKILL);
    exit(0);
}

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

#define KEY_PATH "/home/ahmedwael205/Desktop/OS-Phase1&2/OS_Synchronization/tmp/mqueue_key"
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
    char Data[MemorySize];
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

int main()
{
    signal(SIGINT, handler);
    // Initialization
    int pid;

    union Semun semun;

    key_t keyS = ftok(KEY_PATH, ID_S);
    if (keyS == -1) {
        perror ("ftok");
        exit (-1);
    }

    Mutex = semget(keyS, 1, 0666|IPC_EXCL);

    printf("Mutex= %d\n",Mutex);

    key_t keyQ = ftok(KEY_PATH, ID_Q);
    if (keyQ == -1) {
        perror ("ftok");
        exit (-1);
    }
    qid = msgget(keyQ, IPC_EXCL);
    printf("Queue ID = %d\n", qid);
    int rec_val,send_val;


    key_t keyM = ftok(KEY_PATH, ID_M);
    if (keyM == -1) {
        perror ("ftok");
        exit (-1);
    }

    shmid = shmget(keyM, 4096, IPC_EXCL);
    printf("Shared Memory ID = %d\n", shmid);

   if(Mutex == -1 || qid == -1 || shmid == -1)
    {
        perror("Error in creation");
        exit(-1);
    }

    int cons_index = 0;

    struct Buffer* SharedMemory;
    SharedMemory = (struct Buffer*) shmat(shmid, (struct Buffer*)0, 0);

    if(SharedMemory == -1)
    {
        perror("Error in attach in Consumer");
        exit(-1);
    }
    else
    {
        printf("\nShared memory attached at address %p\n", SharedMemory);
    }
    struct msgbuff Sentmessage,Recievedmessage;
    char msg[16] = "Wakeup C->P\n";
    Sentmessage.mtype = 5;

    while(1)
    {
        down(Mutex);
        
        if (SharedMemory->num == 0)
        {
            printf("Consumer Sleeping ...\n");
            up(Mutex);
            rec_val = msgrcv(qid, &Recievedmessage, sizeof(Recievedmessage.mtext), 7, !IPC_NOWAIT);

            if(rec_val == -1)
                perror("Error in receive");
            printf("Producer message: %s",Recievedmessage.mtext);
        }

        else
        {
            printf("Consuming %c at %d ...\n",SharedMemory->Data [cons_index],cons_index);
            SharedMemory->Data [cons_index] = 'C';
            cons_index = (cons_index+1)% SharedMemory->bufferSize;
            SharedMemory->num--;
            up(Mutex);
        }

        if(SharedMemory->num == SharedMemory->bufferSize - 1)
        {
            printf("Waking up producer ...\n");
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

    // printf("Delete Semaphor= %d\nDelete MessageQueue = %d\nDelete SharedMemory = %d\n", Mutex,qid,shmid);

    // msgctl(qid, IPC_RMID, (struct msqid_ds *) 0);
    // shmctl(shmid, IPC_RMID, (struct shmid_ds*) 0);
    // semctl(Mutex, 0,IPC_RMID);
    raise(SIGKILL);
    exit(0);
}
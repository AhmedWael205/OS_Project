#include <stdio.h>      //if you don't use scanf/printf change this include
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>


typedef short bool;
#define true 1
#define false 1

#define SHKEY 300

#define READY 0
#define RUNNING 1
#define BLOCKED 2
#define FINISHED 3

///==============================
//don't mess with this variable//
int * shmaddr;                 //
//===============================

int qid;


//Process Control Block
struct PCB
{
    int id;
    int pid;
    int remainingtime;
    int waitingtime;
    int status;
    int arrivaltime;
    int priority;
    int runningtime;
    int memsize;
    struct Tree* mem;
};

//Message buffer struct
struct msgbuff
{
    long mtype;
    struct PCB p;
};


//Queue structs
struct Node
{
    int id;
    int pid;
    int remainingtime;
    int priority;
    struct Node* next;
};

struct Tree
{
    struct Tree* left;
    struct Tree* right;
    struct Tree* parent;
    int start,end;
    int depth;
    int FreeSpaceSize;
};


struct Queue
{
    struct Node *front, *rear;
};

int getClk()
{
    return *shmaddr;
}


/*
 * All process call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
*/
void initClk()
{
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1)
    {
        //Make sure that the clock exists
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *) shmat(shmid, (void *)0, 0);
}


/*
 * All process call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
*/

void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        killpg(getpgrp(), SIGINT);
    }
}


//Queue Functions
// struct Node* NewNode(struct PCB p)
// {
//     struct Node* n = (struct Node*)malloc(sizeof(struct Node));
//     n->key = p;
//     n->next = NULL;
//     return n;
// }
struct Queue* CreateQueue()
{
    struct Queue* q = (struct Queue*)malloc(sizeof(struct Queue));
    q->front = NULL;
    q->rear = NULL;
    return q; 
}
bool Enqueue(struct Queue* q, struct Node p)
{
    struct Node* n = (struct Node*)malloc(sizeof(struct Node));
    n->next = NULL;
    n->pid = p.pid;
    n->id = p.id;
    n->remainingtime = p.remainingtime;
    //struct Node* temp = NewNode(p);


    //if queue is empty
    if(q->rear == NULL)
    {
        q->front = n;
        q->rear = n;
        return true;
    }
    else
    {
        //Store new node at back of queue
        q->rear->next = n;
        q->rear = n;
        return true;
    }
    return false;
}

bool priorityEnqueue(struct Queue* q, struct Node p)
{
    struct Node* n = (struct Node*)malloc(sizeof(struct Node));
    n->next = NULL;
    n->pid = p.pid;
    n->id = p.id;
    n->priority = p.priority;
    n->remainingtime = p.remainingtime;
    //struct Node* temp = NewNode(p);


    //if queue is empty
    if(q->rear == NULL)
    {
        q->front = n;
        q->rear = n;
        return true;
    }
    else
    {
        struct Node* temp = q->front;
        if(n->priority < temp->priority)
        {
            n->next = q->front;
            q->front = n;
        }
        else
        {
            while (temp->next!= NULL && temp-> next->priority <= n->priority)
            {
                temp = temp->next;
            }
            n->next = temp ->next;
            temp->next = n;
        }
        

        return true;
    }
    return false;
}

bool STimeEnqueue(struct Queue* q, struct Node p)
{
    struct Node* n = (struct Node*)malloc(sizeof(struct Node));
    n->next = NULL;
    n->pid = p.pid;
    n->id = p.id;
    n->remainingtime = p.remainingtime;
    //struct Node* temp = NewNode(p);


    //if queue is empty
    if(q->rear == NULL)
    {
        q->front = n;
        q->rear = n;
        return true;
    }
    else
    {
        struct Node* temp = q->front;
        if(n->remainingtime < temp->remainingtime)
        {
            n->next = q->front;
            q->front = n;
        }
        else
        {
            while (temp->next!= NULL && n->remainingtime >= temp-> next->remainingtime )
            {
                temp = temp->next;
            }
            n->next = temp ->next;
            temp->next = n;
        }
        

        return true;
    }
    return false;
}

struct Node* Dequeue(struct Queue* q)
{
    //if queue is empty return NULL
    if(q->front == NULL)
        return NULL;
    
    struct Node* temp = q->front;

    //Move front to the next of previous front
    q->front = q->front->next;

    //if front is NULL now that means that the rear should also be NULL (empty queue)
    if(q->front == NULL)
    {
        q->rear = NULL;
    }
    return temp;
}

int power(int base, int exponent)
{
    int result=1;
    for(exponent; exponent>0; exponent--)
    {
        result = result * base;
    }
    return result;
}

struct Tree* AllocateMemory(struct Tree* root,struct PCB p)
{
    int x = ((root->end - root->start) / 2) + 1 ;
    printf("Process Mem = %d , Free Space = %d , at depth = %d, %d -> %d , x = %d\n",p.memsize,root->FreeSpaceSize,root->depth,root->start,root->end,x);
    if(root->FreeSpaceSize < p.memsize) 
    {
        printf("No Free Memory\n");
        return NULL;
    }
    else if (p.memsize < x && (root->left == NULL || root->left->FreeSpaceSize >= p.memsize))
    {
        if(root->left == NULL)
        {
            root->left = (struct Tree*)malloc(sizeof(struct Tree));
            root->left->depth = root->depth + 1;
            root->left->FreeSpaceSize = power(2,10-root->depth) / 2;
            root->left->start = root->start;
            root->left->end = root->start + ((root->end - root->start) / 2);
            root->left->parent = root;
            root->left->left = NULL;
            root->left->right = NULL;
        }
        printf("L\n");
        return AllocateMemory(root->left,p);
    }
    else if (p.memsize <  x  && root->left != NULL &&  (root->right == NULL|| root->right->FreeSpaceSize >= p.memsize))
    {
        if(root->right == NULL)
        {
            root->right = (struct Tree*)malloc(sizeof(struct Tree));
            root->right->depth = root->depth + 1;
            root->right->FreeSpaceSize = power(2,10-root->depth) / 2;
            root->right->start = (root->end/2)+1;
            root->right->end = root->end;
            root->right->parent = root;
            root->right->left = NULL;
            root->right->right = NULL;
        }
        printf("R\n");
        return AllocateMemory(root->right,p);
    }
    else if (root->right == NULL && root->left == NULL )
    {
        struct Tree* temp= root->parent;
        printf("Here\n");

        while(temp != NULL)
        {
            temp->FreeSpaceSize -= root->FreeSpaceSize;
            temp = temp->parent;
        }
        root->FreeSpaceSize = 0;
        return root;
    }
    else
    {
        printf("Memory is Not avaliable as a block\n");
        return NULL;
    }   
}

bool FreeMemory(struct Tree* Node,struct PCB p)
{
    printf("Allocated mem from %d to %d\n",Node->start,Node->end);
    if(p.mem == NULL) 
    {
        printf("Error No memory allocated\n");
        return false;
    }
    struct Tree* parent = Node->parent;

    int x = power(2,10-Node->depth);
    Node->FreeSpaceSize = x;

    while(parent != NULL)
    {
        printf("Parent old Free Space = %d\n",parent->FreeSpaceSize);
        parent->FreeSpaceSize +=  x;
        printf("Parent new Free Space = %d\n",parent->FreeSpaceSize);

        int y = power(2,10-(parent->depth+1));
        
        if(parent->right == NULL && parent->left->FreeSpaceSize == y)
        {
            printf("Merging\n");
            free(parent->left);
            parent->left = NULL;
        }
        else if(parent->left == NULL && parent->right->FreeSpaceSize == y)
        {
            printf("Merging\n");
            free(parent->right);
            parent->right = NULL;
        }
        else if(parent->right->FreeSpaceSize == y && parent->left->FreeSpaceSize == y)
        {
            printf("Merging\n");
            free(parent->left);
            free(parent->right);
            parent->left = NULL;
            parent->right = NULL;
        }
        else
        {
            printf("No Merging\n");
            return true;
        }
        parent = parent->parent;
    }

    if(parent == NULL) 
    {
        printf("Root\n");
        return  true;
    }
    return true;
}

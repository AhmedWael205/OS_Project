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

struct TreeNode
{
    struct Tree *Tree;
    struct TreeNode *next;
};

struct FreeListHead
{
   struct TreeNode* front;
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

struct FreeListHead* CreateList()
{
    struct FreeListHead* q = (struct FreeListHead*)malloc(sizeof(struct FreeListHead));
    q->front = NULL;
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

FILE * FreeMemLog;

FILE *AllocDeallocLog;
struct Tree* AllocateBestFit(struct FreeListHead* FreeListHead, int Pmemsize)
{


    struct TreeNode* temp = FreeListHead->front;
    struct Tree* BestFit = NULL;
    int min = 99999999;
    while(temp != NULL)
    {
        if(temp->Tree->FreeSpaceSize - Pmemsize <= min &&  temp->Tree->FreeSpaceSize - Pmemsize >= 0 && (BestFit == NULL || BestFit->start > temp->Tree->start))
        {
            min = temp->Tree->FreeSpaceSize - Pmemsize;
            fprintf(FreeMemLog,"min = %d\n",min);
            BestFit = temp->Tree;
        }
        temp = temp->next;
    }
    return BestFit;
}

bool RemoveFreeSpace(struct FreeListHead* FreeListHead,struct Tree*FirstBestFit)
{


    struct TreeNode* temp = FreeListHead->front;
    if(temp->Tree->start == FirstBestFit->start && temp->Tree->end == FirstBestFit->end)
    {
        fprintf(FreeMemLog,"Unpoint to root\n");
        FreeListHead->front = FreeListHead->front->next;
        return true;
    }
    else
    {
        while (temp->next != NULL)
        {
            struct TreeNode* del = NULL;
            if(temp->next->Tree->start == FirstBestFit->start && temp->next->Tree->end == FirstBestFit->end)
            {
                fprintf(FreeMemLog,"Unpoint to node\n");
                del = temp->next;
                temp->next = temp->next->next;
                return true;
            }
            temp = temp->next;
        }
        return false;
    }
    return false;
}

bool InsertFreeSpace(struct FreeListHead *FreeListHead,struct Tree* FreeTree)
{

    struct TreeNode* n = (struct TreeNode*)malloc(sizeof(struct TreeNode));
    n->Tree = FreeTree;
    n->next = NULL;
    if(FreeListHead->front == NULL)
    {
        FreeListHead->front = n;
        fprintf(FreeMemLog,"Inserting root at InsertFreeSpace, FreeListHead->front->Tree->FreeSpaceSize = %d\n",FreeListHead->front->Tree->FreeSpaceSize);
        return true;
    }


    struct TreeNode* temp = FreeListHead->front;
    while (temp->next != NULL)
    {
        temp = temp->next;
    }
    temp->next = n;
    fprintf(FreeMemLog,"Insert Node, FreeMem->Freespace = %d\n",temp->next->Tree->FreeSpaceSize);
    return true;
}

struct Tree* AllocateMemory(struct FreeListHead* FreeListHead,struct PCB p)
{
    if(FreeListHead->front == NULL) 
    {
        fprintf(AllocDeallocLog,"Error: FreeList is empty\n");
        return NULL;
    }
    fprintf(AllocDeallocLog,"FreeList root FreeSpace = %d\n",FreeListHead->front->Tree->FreeSpaceSize);
    struct Tree* FirstBestFit = AllocateBestFit(FreeListHead,p.memsize);
    if(FirstBestFit == NULL)
    {
       fprintf(AllocDeallocLog,"Error: Failed Allocated Memory\n");
       return NULL;
    }
    int x = ((FirstBestFit->end - FirstBestFit->start) / 2) + 1 ;
    fprintf(AllocDeallocLog,"Process Mem = %d , FirstBestFit: Free Space = %d , at depth = %d, %d -> %d , x = %d\n",p.memsize,FirstBestFit->FreeSpaceSize,FirstBestFit->depth,FirstBestFit->start,FirstBestFit->end,x);
    if(FirstBestFit->FreeSpaceSize < p.memsize) 
    {
        fprintf(AllocDeallocLog,"Error: No Free Memory\n");
        return NULL;
    }
    else if (p.memsize < x && (FirstBestFit->left == NULL || FirstBestFit->left->FreeSpaceSize >= p.memsize))
    {
        RemoveFreeSpace(FreeListHead,FirstBestFit);
        if(FirstBestFit->left == NULL)
        {
            FirstBestFit->left = (struct Tree*)malloc(sizeof(struct Tree));
            FirstBestFit->left->depth = FirstBestFit->depth + 1;
            FirstBestFit->left->FreeSpaceSize = power(2,10-FirstBestFit->depth) / 2;
            FirstBestFit->left->start = FirstBestFit->start;
            FirstBestFit->left->end = FirstBestFit->start + ((FirstBestFit->end - FirstBestFit->start) / 2);
            FirstBestFit->left->parent = FirstBestFit;
            FirstBestFit->left->left = NULL;
            FirstBestFit->left->right = NULL;
            InsertFreeSpace(FreeListHead,FirstBestFit->left);
        }
        if(FirstBestFit->right == NULL)
        {
            FirstBestFit->right = (struct Tree*)malloc(sizeof(struct Tree));
            FirstBestFit->right->depth = FirstBestFit->depth + 1;
            FirstBestFit->right->FreeSpaceSize = power(2,10-FirstBestFit->depth) / 2;
            FirstBestFit->right->start = (FirstBestFit->end/2)+1;
            FirstBestFit->right->end = FirstBestFit->end;
            FirstBestFit->right->parent = FirstBestFit;
            FirstBestFit->right->left = NULL;
            FirstBestFit->right->right = NULL;
            InsertFreeSpace(FreeListHead,FirstBestFit->right);
        }

        fprintf(AllocDeallocLog,"go to Left Child\n");
        return AllocateMemory(FreeListHead,p);
    }
    else if (p.memsize <  x  && FirstBestFit->left != NULL &&  (FirstBestFit->right == NULL|| FirstBestFit->right->FreeSpaceSize >= p.memsize))
    {
        RemoveFreeSpace(FreeListHead,FirstBestFit);
        if(FirstBestFit->right == NULL)
        {
            FirstBestFit->right = (struct Tree*)malloc(sizeof(struct Tree));
            FirstBestFit->right->depth = FirstBestFit->depth + 1;
            FirstBestFit->right->FreeSpaceSize = power(2,10-FirstBestFit->depth) / 2;
            FirstBestFit->right->start = (FirstBestFit->end/2)+1;
            FirstBestFit->right->end = FirstBestFit->end;
            FirstBestFit->right->parent = FirstBestFit;
            FirstBestFit->right->left = NULL;
            FirstBestFit->right->right = NULL;
            InsertFreeSpace(FreeListHead,FirstBestFit->right);
        }
        fprintf(AllocDeallocLog,"goto Right Child\n");
        return AllocateMemory(FreeListHead,p);
    }
    else if (FirstBestFit->right == NULL && FirstBestFit->left == NULL )
    {
        struct Tree* temp= FirstBestFit->parent;
        fprintf(AllocDeallocLog,"Allocate The Memory Here\n");

        while(temp != NULL)
        {
            temp->FreeSpaceSize -= FirstBestFit->FreeSpaceSize;
            temp = temp->parent;
        }
        FirstBestFit->FreeSpaceSize = 0;
        RemoveFreeSpace(FreeListHead,FirstBestFit);
        return FirstBestFit;
    }
    else
    {
        fprintf(AllocDeallocLog,"Memory is Not avaliable as a block\n");
        return NULL;
    }   
}

bool FreeMemory(struct FreeListHead* FreeListHead,struct PCB p)
{
    fprintf(AllocDeallocLog,"Allocated mem from %d to %d\n",p.mem->start,p.mem->end);
    if(p.mem == NULL) 
    {
        fprintf(AllocDeallocLog,"Error No memory allocated\n");
        return false;
    }
    struct Tree* parent = p.mem->parent;

    int x = power(2,10-p.mem->depth);
    p.mem->FreeSpaceSize = x;
    InsertFreeSpace(FreeListHead,p.mem);
    while(parent != NULL)
    {
        fprintf(AllocDeallocLog,"Parent old Free Space = %d\n",parent->FreeSpaceSize);
        parent->FreeSpaceSize +=  x;
        fprintf(AllocDeallocLog,"Parent new Free Space = %d\n",parent->FreeSpaceSize);

        int y = power(2,10-(parent->depth+1));
        
        if(parent->right->FreeSpaceSize == y && parent->left->FreeSpaceSize == y)
        {
            fprintf(AllocDeallocLog,"Merging\n");
            RemoveFreeSpace(FreeListHead,parent->left);
            RemoveFreeSpace(FreeListHead,parent->right);
            free(parent->left);
            free(parent->right);
            parent->left = NULL;
            parent->right = NULL;
            InsertFreeSpace(FreeListHead,parent);
        }
        else
        {
            fprintf(AllocDeallocLog,"No Merging\n");
            return true;
        }
        parent = parent->parent;
    }

    if(parent == NULL) 
    {
        fprintf(AllocDeallocLog,"Root\n");
        return  true;
    }
    return true;
}

void printFreeSpaces(struct FreeListHead* FreeListHead)
{

    struct TreeNode* temp = FreeListHead->front;
    while(temp != NULL)
    {
        fprintf(FreeMemLog,"Free Mem = %d, location = %d -> %d\n",temp->Tree->FreeSpaceSize,temp->Tree->start,temp->Tree->end);
        temp = temp->next;
    }

    return;
}
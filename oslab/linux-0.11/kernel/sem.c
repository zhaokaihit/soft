#define __LIBRARY__
#include <unistd.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>
#include <asm/system.h>
#include <signal.h>

#define NR_SEMS 20

sem_t *array[20]={};
char tempname[20]={'\0'};
char delname[20]={'\0'};
char sname[20][20] = {'\0'};
sem_t sem_array[20];

int addone(int i)
{
    return ((i + 1) % 10);
}

void MakeNull( queue * q)
{
    q->front = 0;
    q->rear = 9;
}

int empty(queue* q)
{
    if(addone(q->rear) == q->front )
        return 1;
    else
        return 0;
}

struct task_struct * getfront( queue * q )
{
    int temp;
    if( !empty(q) )
    {
        temp = q->front;
        q->front = addone( q->front );
        return q->wait[temp];
    }
    else
        return NULL;
}

void enterrear( struct task_struct * child, queue * q)
{
    if( addone( addone(q->rear)) != q->front )
    {
        q->rear = addone( q->rear );
        q->wait[q->rear] = child;
    }
}

extern void sleep_on_sem(void);
extern void wake_on_sem(struct task_struct *p);

int sys_sem_open(const char *name, unsigned int value)
{
    int i = 0,j,k;
    int flag = 0;
    char temp;

    while((temp = get_fs_byte(name+i))!='\0')
    {
        tempname[i] = temp;
        i++;
    }
    tempname[i] = '\0';
    for (i = 0;i < NR_SEMS; i++)
    {
        flag = strcmp(tempname,sname[i]);
        if (!flag)
        {
            array[i] = &sem_array[i];
            return array[i];
        }
    }
    for (i = 0; i < NR_SEMS; i++)
    {
        if (sem_array[i].used==0)
        {
            array[i] = &sem_array[i];
            strcpy(sname[i],tempname);
            array[i]->value = value;
            array[i]->used = 1;
            MakeNull( &(array[i]->waitsem));
            return array[i];
        }
    }
    return -1;
}

int sys_sem_wait(sem_t* sem)
{
    cli();
    sem->value--;
    if(sem->value < 0)
    {
        enterrear( current, &(sem->waitsem));
        sleep_on_sem();
		sti();
    }
    sti();
    return 0;
}

int sys_sem_post(sem_t * sem)
{
    cli();
    sem->value++;
    if(sem->value<=0)
    {
        wake_on_sem(getfront(&(sem->waitsem)));
    }
    sti();
    return 0;
}

int sys_sem_unlink(const char *name)
{
    int i = 0,j;
    int flag;
    char temp;
    while((temp = get_fs_byte(&name[i]))!='\0')
    {
        delname[i] = temp;
        i++;
    }
    delname[i] = '\0';
    
    for(i = 0;i < NR_SEMS; i++)
    {
        flag =strcmp(sname[i],delname);
        if(!flag)
        {
            sem_array[i].used = 0;
            sem_array[i].value = 0;
            MakeNull(&(sem_array[i].waitsem));
           for(j = 0; j<20; j++)
            sname[i][j] = '\0';
            return 0;
        }
    }
    return -1;
}

void sleep_on_sem(void)
{
        current->state=TASK_UNINTERRUPTIBLE;
        schedule();
}

void wake_on_sem(struct task_struct *p)
{
      if(p != NULL)
            (*p).state = TASK_RUNNING;
}

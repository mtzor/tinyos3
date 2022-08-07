
#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"
#include "kernel_cc.h"
#include "kernel_streams.h"


#define SYSTEM_PAGE_SIZE (1 << 12)

#define PROCESS_THREAD_PTCB_SIZE  (sizeof(PTCB))

Mutex thread_count_spinlock=MUTEX_INIT;


PTCB * spawn_ptcb(Task task, int argl, void* args);
void start_new_thread();
int sys_ThreadJoin(Tid_t tid, int* exitval);
/** 
  @brief Create a new thread in the current process.
  */
Tid_t sys_CreateThread(Task task, int argl, void* args){

 TCB* tcb=NULL;

   PTCB* ptcb = spawn_ptcb(task,argl,args);//Creating and initializing a ptcb for the new thread

  if(ptcb==NULL)
    return NOTHREAD;

  tcb = spawn_thread(CURPROC,start_new_thread);

  
  if(tcb==NULL)
    return NOTHREAD;

 
 ptcb->tcb=tcb;//connecting ptcb to its tcb
 tcb->ptcb=ptcb;//Connecting tcb to ptcb

  /* increase the count of threads in pcb */

  CURPROC->thread_count++;

  
  int ret=wakeup(tcb);
  assert(ret==1);

  return (Tid_t) ptcb;
}

/**
  @brief Return the Tid of the current thread.
 */
Tid_t sys_ThreadSelf(){
  return (Tid_t) CURTHREAD->ptcb;
}

void release_PTCB(PTCB* ptcb);
/**
  @brief Join the given thread.
  */
int sys_ThreadJoin(Tid_t tid, int* exitval)
{
  PTCB* tidc=(PTCB*)tid;

  rlnode* fail=NULL;//initialising node to NULL

  if(tidc->exited==1){
    return 0;
  }

  fail=rlist_find(&CURPROC->thread_list, tidc, fail);//trying to find tid in process thread list

  if(fail==NULL) // if node with tid  does not exist in list
    return -1;
 
  if(tidc==CURTHREAD->ptcb)// cannot join the current thread
  return -1;

  if(tidc->detached==1)// cannot join a detached or exited thread
  return -1;

  tidc->ref_count++;//increasing the tcbs reference count to prevent exit from releasing it 
  
  //wait for thread to either exit or be detached
  while(tidc->exited==0 &&tidc->detached==0){
  kernel_wait(&tidc->exit_cv,SCHED_USER);
  }

  

   if(tidc->exited==1){//thread exited successfully
    
    if(exitval!=NULL)//save the exit value of thread
      exitval=&tidc->exitval;


    tidc->ref_count--;

      //If no other thread is waiting for it to finish
      //remove the ptcb from the threadlist of this process and free it  from the memory 
  
      if(tidc->ref_count==0){
      rlist_remove(&tidc->thread_list_node);

      release_PTCB(tidc);
   }
   return 0;
}
  
  if(tidc->detached==1)// thread has been detached during join
    return -1;
  
return -1;

  
}

/**
  @brief Detach the given thread.
  */
int sys_ThreadDetach(Tid_t tid)
{

   PTCB* tidc=(PTCB*)tid;

  rlnode* fail=NULL;//initialising node to NULL

  fail=rlist_find(&CURPROC->thread_list, tidc, fail);//trying to find tid in process thread list

  if(fail==NULL || tidc->exited==1){// if node with tid  does not exist in list
    return -1;
  }

  tidc->detached=1;// i am detached now.No one will wait for me i must die alone.
  tidc->ref_count=0;//setting reference count to zero so the ptcb can be released during thread exit 

  kernel_broadcast(&tidc->exit_cv);//HELLO ALL ,YOU SHALL NOT WAIT!
  // calling broadcast to notify any threads waiting on this (now detached) thread

  return 0;
}

void release_PTCB(PTCB* ptcb);
/**
  @brief Terminate the current thread.
  */
void sys_ThreadExit(int exitval)
{
  PTCB*  cptcb=  CURTHREAD->ptcb;//cache this variable for better performance

  assert(cptcb!=NULL);
  cptcb->exitval=exitval;
  cptcb->exited=1;

 kernel_broadcast(&cptcb->exit_cv);//broadcasting that this thread is exited

//if no other thread is waiting for it to finish
// remove the ptcb from the threadlist of this process and free it  from the memory 
  if(cptcb->ref_count==0){
      rlist_remove(&cptcb->thread_list_node);
      release_PTCB(cptcb);

   }



//************************************************************************
  if(CURPROC->thread_count==0){//making sure that this process has no more ptcbs before exiting it 
  PCB *curproc = CURPROC;  /* cache for efficiency */

  /* Do all the other cleanup we want here, close files etc. */
  if(curproc->args) {
    free(curproc->args);
    curproc->args = NULL;
  }

  /* Clean up FIDT */
  for(int i=0;i<MAX_FILEID;i++) {
    if(curproc->FIDT[i] != NULL) {
      FCB_decref(curproc->FIDT[i]);
      curproc->FIDT[i] = NULL;
    }
  }

  /* Reparent any children of the exiting process to the 
     initial task */
  PCB* initpcb = get_pcb(1);
  while(!is_rlist_empty(& curproc->children_list)) {
    rlnode* child = rlist_pop_front(& curproc->children_list);
    child->pcb->parent = initpcb;
    rlist_push_front(& initpcb->children_list, child);
  }

  /* Add exited children to the initial task's exited list 
     and signal the initial task */
  if(!is_rlist_empty(& curproc->exited_list)) {
    rlist_append(& initpcb->exited_list, &curproc->exited_list);
    kernel_broadcast(& initpcb->child_exit);
  }

  /* Put me into my parent's exited list */
  if(curproc->parent != NULL) {   /* Maybe this is init */
    rlist_push_front(& curproc->parent->exited_list, &curproc->exited_node);
    kernel_broadcast(& curproc->parent->child_exit);
  }

  /* Disconnect my main_thread */
  curproc->main_thread = NULL;

  /* Now, mark the process as exited. */
  curproc->pstate = ZOMBIE;

  }


  /* Bye-bye cruel world */
  kernel_sleep(EXITED, SCHED_USER);

}

//Create and initialize a ptcb for the new thread
PTCB * spawn_ptcb(Task task, int argl, void* args){

  PTCB* ptcb = (PTCB*)xmalloc(PROCESS_THREAD_PTCB_SIZE);
  assert(ptcb!=NULL);

  ptcb->ref_count=0;

  rlnode_init(&ptcb->thread_list_node, ptcb); /* Intrusive list node */


  rlist_push_back(&CURPROC->thread_list, &ptcb->thread_list_node);//insert the ptcb to the list
  ptcb->task=task; //setting the pointer to the task function in ptcb

  ptcb->argl=argl;
  ptcb->args=args;

  ptcb->exitval=-1;//The value that is returned by the function pointed by task 
  ptcb->exited=0;//boolean variable ,1 if the thread is exited
  ptcb->detached=0;//boolean variable , 1 if the thread is detached
  ptcb->exit_cv=COND_INIT;//initializing the exit condition variable



  return ptcb;
}

/* this funtion ***MUST*** be called after we have called spawn_ptcb()*/
//This is the function that runs when a new thread starts running
void start_new_thread()
{
  int exitval;
  PTCB* new_ptcb=CURTHREAD->ptcb;

  Task call_thread = new_ptcb->task;
  int argl = new_ptcb->argl;
  void* args = new_ptcb->args;

  exitval = call_thread(argl,args);//calling the thread's task and storing its exit value 
  
  new_ptcb->exitval=exitval;//passing the tcb's exitvalue to its ptcb.

  ThreadExit(exitval);//exiting the thread
}

//This function frees the given ptcb and decreases the thread count of the current process.
void release_PTCB(PTCB* ptcb)
{
  free(ptcb);

   CURPROC->thread_count--;
 

return;

}

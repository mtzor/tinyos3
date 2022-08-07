#ifndef __KERNEL_THREADS_H
#define __KERNEL_THREADS_H

#include "bios.h"
#include "tinyos.h"
#include "util.h"



Tid_t sys_CreateThread(Task task, int argl, void* args);

Tid_t sys_ThreadSelf();

int sys_ThreadJoin(Tid_t tid, int* exitval);

int sys_ThreadDetach(Tid_t tid);

void sys_ThreadExit(int exitval);

PTCB * spawn_ptcb(Task task, int argl, void* args);

void start_new_thread();

void release_PTCB(PTCB* ptcb);

  #endif
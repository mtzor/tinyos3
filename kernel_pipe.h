#ifndef __KERNEL_PIPE_H
#define __KERNEL_PIPE_H

#include "tinyos.h"
#include "kernel_dev.h"
#include "kernel_cc.h"
#include "kernel_streams.h"

#define BUFF_SIZE 8192


typedef struct pipe_control_block {
  char buffer[BUFF_SIZE];

  FCB* read;
  FCB* write;

  uint write_pos;
  uint read_pos;

  CondVar has_space; 
  CondVar has_data;	

 
} PIPE_CB;

int sys_Pipe_Read(void* stream_object, char *buf, unsigned int size);

int sys_Pipe(pipe_t* pipe);

int sys_Pipe_Write(void* stream_object, char *buf, unsigned int size);

int sys_Pipe_Writer_Close(void* streamobj);

int sys_Pipe_Reader_Close(void* streamobj);

PIPE_CB* construct_Pipe();

  #endif

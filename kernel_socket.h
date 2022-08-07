#ifndef __KERNEL_SOCKET_H
#define __KERNEL_SOCKET_H

#include "tinyos.h"
#include "kernel_dev.h"
#include "kernel_cc.h"
#include "kernel_streams.h"
#include "kernel_pipe.h"


typedef enum {
	UNBOUND, /**< @brief SOCKET initialising */
	PEER, /**< @brief ready to read n write   */
	LISTENER, /**< @brief listener socket   */
} Socket_type;

typedef struct listener_socket_control_block
{
  rlnode request_queue;  			/**< @brief Reference counter. */
  
  CondVar has_request; // The condition variable that wakes if the listener has a request


} LSCB;
  
typedef struct unbound_socket_control_block
{
  int stupid_variable;//A stupid variable that does nothing 

} USCB;


typedef struct peer_socket_control_block
{

  SCB* peer_socket;//Pointer to the other peer socket
  
  PIPE_CB* read_pipe;
  PIPE_CB* write_pipe;

} PSCB;


typedef struct socket_control_block
{
  port_t port;//The port of the socket 
  Socket_type type;//The type of the socket which is enum (UNBOUND,PEER,LISTENER)
  uint refcount;
  FCB* fcb;//Pointer to the fcb to take the stream object and function

  union {
   PSCB* pscb;
   LSCB* lscb;
   USCB* uscb;
  };

} SCB;


typedef struct request_node_struct
{ 
	rlnode request_node;//The head node of the listeners list 
	uint admitted;//boolean Variable to check if the socket is admitted
	CondVar wakeup_request;//The condition variable 
	SCB* scb;//Pointer to the Listening scb 

}RNS;

Fid_t sys_Socket(port_t port);

int sys_Listen(Fid_t sock);

Fid_t sys_Accept(Fid_t lsock);

int sys_Connect(Fid_t sock, port_t port, timeout_t timeout);

int sys_Socket_Read(void* stream_object, char *buf, unsigned int size);
  
int sys_Socket_Write(void* stream_object, char *buf, unsigned int size);

int sys_Socket_Close(void* streamobj);

void SCB_decref(SCB* scb);

int sys_Socket_Void(void* stream_object, char *buf, unsigned int size);

PIPE_CB* construct_Pipe();
  

 #endif
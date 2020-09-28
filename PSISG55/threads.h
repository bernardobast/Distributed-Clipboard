#ifndef THREADS_H
#define THREADS_H

#define OFFLINE 0 //Constant used in clipboard_type
#define ONLINE 1 //Constant used in clipboard_type
#define NOTLAST 0 //Constant used in clipboard_last
#define LAST 1 //Constant used in clipboard_last

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>

//Stucture used to store the client_id (returned id every time net socket accepts a new connection)
typedef struct clip_thread{
	int client_id; //Client_id
	struct clip_thread *next; //Pointer to the next node
} clip_thread;

//Inicializes the thread list
extern clip_thread *threadList;

extern int clipboard_type; //clipboard_type: ONLINE, OFFLINE
extern int clipboard_last; //clipboard_last: LAST, NOTLAST

//Clipboard
extern char * clipboard_vector[10]; 
extern int word_size[10]; //vector that stores the size of each word
extern int waiting[10]; //vactor that indicates the number of threads waitting for a change in a certain region

//Internet Socket
extern struct sockaddr_in server_adress; 
extern struct sockaddr_in client_adress;
extern struct sockaddr_in connected_adress;
extern socklen_t size_addr; //server size

extern int connected_socket; //Connects to the father clipboard socked
extern int internet_socket; //Connects to the lower clipboard 
extern int sock_fd; //Connects to the app

//Initializes all pthread locks
extern pthread_rwlock_t lockClipboard; //lock used by clipboard (block reads and writes)
extern pthread_mutex_t mux; //mutex used in condwait
extern pthread_mutex_t listmux; //mutex used by thread list
extern pthread_cond_t condwait[10]; //condicional variable for each clipboard region

/*****************************************************
ThreadListAdd: Funtion that adds new thread to the thread list every time a cliboard is connected
******************************************************/
void ThreadListAdd(clip_thread * newthread);

/*****************************************************
ThreadListRemove: Funtion that removes a thread from the list every time a clipboard disconnects
******************************************************/
void ThreadListRemove(int id);

/*****************************************************
ClipboardThread_UP_DOWN: Function used to communicate with LOWER clipboards. 
******************************************************/
void * ClipboardThread_UP_DOWN(void * up_socket);

/*****************************************************
ClipboardThread_DOWN_UP: Function used to communicate with HIGHER clipboards. 
******************************************************/
void * ClipboardThread_DOWN_UP(void * down_socket);

/*****************************************************
thread_code: Function to comunicate with app. Does the COPY, PASTE, WAIT e EXIT 
******************************************************/
void * thread_code(void * client);

#endif
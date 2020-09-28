#ifndef LOCALCLIPBOARD_H
#define LOCALCLIPBOARD_H

#define MESSAGE_SIZE 8000 //Maximum message that can be copied by an App
#define COPY "COPY\n" //Operation copy
#define PASTE "PASTE\n" //Operation paste
#define EXIT "EXIT\n" //Operation exit
#define WAIT "WAIT\n" //Operation wait

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

/*****************************************************
sig_handler: Funtion that closes all sockets, upper lower and the ones connected to the app
******************************************************/
void sig_handler(int sig);

#endif
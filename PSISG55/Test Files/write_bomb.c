#include "clipboard.h"
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#define ADDR "./"

int main()
{

	printf("******Welcome to client comunication********\n");
	int i;
	//Connects with clipboard
	int fd = clipboard_connect(ADDR);
	if(fd== -1){
		exit(-1);
	}
	printf("Connected, ready to comunicate\n");

	char buff[MESSAGE_SIZE];
	strcpy(buff,"WRITE BOMB\n");

	for(i=0;i<100;i++)
	{
		//Selects the operation 
	
		clipboard_copy(fd, 5, buff, strlen(buff)); //STRLEN
	}
}
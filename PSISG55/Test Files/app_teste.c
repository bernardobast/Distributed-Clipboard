#include "clipboard.h"
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#define ADDR "./"

int main()
{

	printf("******Welcome to client comunication********\n");

	//Connects with clipboard
	int fd = clipboard_connect(ADDR);
	if(fd== -1){
		exit(-1);
	}
	printf("Connected, ready to comunicate\n");

	int nbytes; 
	char region_AUX[10]; //aux sting used to get the region
	int region; //region wich the user wants to access
	char buff[MESSAGE_SIZE];
	
	while(1)
	{
		//Selects the operation 
		printf("Choose an operation: COPY, PASTE, WAIT or EXIT\n");
		fgets(buff, MESSAGE_SIZE, stdin);

		if(strcmp(buff, COPY)==0)
		{
			printf("Select a region you want to copy (from 0-9)\n");
			fgets(region_AUX, 10, stdin);
			region = atoi(region_AUX); //converts string to integer

			printf("What message do you want to copy?\n");
			fgets(buff, MESSAGE_SIZE, stdin);
			nbytes=clipboard_copy(fd, region, buff, strlen(buff)); //STRLEN
			printf("Sent: %d\n", nbytes);
		}

		if(strcmp(buff, PASTE)==0)
		{
			printf("Select the region you want to paste (from 0-9)\n");
			fgets(region_AUX, 10, stdin);
			region = atoi(region_AUX); //converts string to integer

			char new_message[100];
			nbytes=clipboard_paste(fd, region, new_message, 100);
			printf("Received: %s with: %d bytes\n", new_message, nbytes);				
		}

		if(strcmp(buff, WAIT)==0)
		{
			printf("Select the region you want to wait for (from 0-9)\n");
			fgets(region_AUX, 10, stdin);
			region = atoi(region_AUX); //converts string to integer

			char new_message[100];
			nbytes=clipboard_wait(fd, region, new_message, 100);
			printf("Received: %s with: %d bytes\n", new_message, nbytes);
		}

		if(strcmp(buff, EXIT)==0)
		{
			clipboard_close(fd);
			printf("BYE\n");
			exit(0);
		}
	}
}

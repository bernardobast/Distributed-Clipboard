#include "clipboard.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

//The code for this function was given by the professor "stream-1.c"
/*********************************************************************
**  clipboard_connect: Function that makes the connection between app and clipboard
**********************************************************************/
int clipboard_connect(char * clipboard_dir){

	struct sockaddr_un server_addr;
	struct sockaddr_un client_addr;

	int sock_fd= socket(AF_UNIX, SOCK_STREAM, 0); // creates a socket and returns socket id stored in socket_fd
	if (sock_fd == -1){
		perror("socket: not created");
		return -1;
	}
	
	client_addr.sun_family = AF_UNIX; // 
	sprintf(client_addr.sun_path, "%ssocket", clipboard_dir);

	server_addr.sun_family = AF_UNIX;
	strcpy(server_addr.sun_path, SOCK_ADDRESS);

	int err_c = connect(sock_fd, (const struct sockaddr *) &server_addr, sizeof(server_addr)); // connects to the clipboard
	if(err_c==-1){
				printf("Error connecting\n");
				return -1;
	}
	return sock_fd;
}

/****************************************************************************
** clipboard_copy: Function to copy information from app to clipboard *******
	/param clipboard_id: ID of the connected clipboard
	/param region: Region the app wants to copy to
	/param *buf: string that stores the message to copy
	/param count: sizeof buf
****************************************************************************/
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count){
	
	message_sent message;
	int numberofbytes = 0;
	int errorcheck=0;
	int operation=1;

	write(clipboard_id, &operation, sizeof(int)); // Sends the type of operation the app wants to perform
	// Verificaçao de erros
	if (region > 9 || region < 0) // Verifies if the region is valid
	{
		printf("ERROR: Region not valid \n");
		errorcheck = 1; // If it's not valid notifies the clipboard anda returns 
		write(clipboard_id, &errorcheck, sizeof(int));
		return 0;
	}
	if(buf == NULL) // Verifies if the message to be copied is NULL and notifies the clipboard and returns
	{
		perror("NULL message sent");
		errorcheck = 1;
		write(clipboard_id, &errorcheck, sizeof(int));
		return 0;
	}

	write(clipboard_id, &errorcheck, sizeof(int)); // Notifies the clipboard there are no errors
	// End of error verification
	message.region = region; // saves the region and the size of the message in the structure 
	message.message_size = count;

	write(clipboard_id, &message, sizeof(message_sent)); // sends the structure to the clipboard

	read(clipboard_id, &errorcheck, sizeof(int)); // reads if the clipboard detected any errors
	if(errorcheck==1)
	{
		perror("Invalid region");
		return 0;
	}

	numberofbytes = write(clipboard_id, buf, count); // writes the message to the clipboard
	if (numberofbytes != (count*sizeof(char)))
	{
		printf("Data is missing from clipboard\n");
		return 0;
	}

	read(clipboard_id, &errorcheck, sizeof(int)); // reads if the clipboard detected any errors
	if(errorcheck==2)
	{
		perror("NULL message sent");
		return 0;
	}

	return numberofbytes;

}

/********************************************************************************
** clipboard_paste: Pastes a message from clipboard to client ******************
	/param clipboard_id: ID of the connected clipboard
	/param region: Region the app wants to copy to
	/param *buf: string that stores the message to paste
	/param count: sizeof buf
********************************************************************************/
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count){
	
	message_sent message;
	int numberofbytes = 0;
	int errorcheck=0;
	int operation=0;

	write(clipboard_id, &operation, sizeof(int)); // Sends the type of operation the app wants to perform
	
	if (region > 9 || region < 0) // Verifies if the region is valid
	{
		printf("ERROR: Region not valid \n");
		errorcheck = 1;
		write(clipboard_id, &errorcheck, sizeof(int)); // If it's not valid notifies the clipboard anda returns 
		return 0;
	}

	write(clipboard_id, &errorcheck, sizeof(int)); // Notifies the clipboard there are no errors
	// End of error verification
	message.region = region; // saves the region and the size of the message in the structure 
	message.message_size = count;

	write(clipboard_id, &message, sizeof(message_sent)); // sends the structure to the clipboard

	read(clipboard_id, &errorcheck, sizeof(int)); // reads if the clipboard detected any errors
	if(errorcheck==1)
	{
		perror("Invalid region");
		return 0;
	}


	numberofbytes = read(clipboard_id, buf, count*sizeof(char)); //reads the message from the clipboard
	if (numberofbytes == 0)
	{
		printf("error sending data");
		return 0;
	}

	return numberofbytes;
}

/********************************************************************************
** clipboard_wait: Waits for a change in a certain region and pastes a message from clipboard to client
	/param clipboard_id: ID of the connected clipboard
	/param region: Region the app wants to copy to
	/param *buf: string that stores the message to paste
	/param count: sizeof buf
********************************************************************************/

int clipboard_wait(int clipboard_id, int region, char *buf, size_t count)
{
	message_sent message;
	int numberofbytes = 0;
	int errorcheck = 0;
	int operation=2;

	write(clipboard_id, &operation, sizeof(int)); // Sends the type of operation the app wants to perform

	if (region > 9 || region < 0) // Verifies if the region is valid
	{
		printf("ERROR: Region not valid \n");
		errorcheck = 1; // If it's not valid notifies the clipboard anda returns 
		write(clipboard_id, &errorcheck, sizeof(int));
		return 0;
	}

	write(clipboard_id, &errorcheck, sizeof(int)); // Notifies the clipboard there are no errors
	// End of error checking
	message.region = region; // saves the region and the size of the message in the structure 
	message.message_size = count;

	printf("WAITING for a change in clipboard\n");

	write(clipboard_id, &message, sizeof(message_sent)); // sends the structure to the clipboard

	read(clipboard_id, &errorcheck, sizeof(int)); // reads if the clipboard detected any errors
	if(errorcheck==1)
	{
		perror("Invalid region");
		return 0;
	}

	numberofbytes=read(clipboard_id, buf, count*sizeof(char)); //reads the message from the clipboard
	if (numberofbytes == 0)
	{
		printf("error sending data");
		return 0;
	}

	return numberofbytes;

}

/******************************************************************
** clipboard_close: Function to close the app
	\ param clipboard_id: ID of the connected clipboard
	****************************************************************/

void clipboard_close(int clipboard_id)
{
	close(clipboard_id); // closes the socket that connects to the clipboard
}
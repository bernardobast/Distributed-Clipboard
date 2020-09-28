#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#define SOCK_ADDRESS "./CLIPBOARD_SOCKET"

#include "localclipboard.h"

//Stucture sent to clipboard
typedef struct message_sent{
	int region; //Clipboard region 
	size_t message_size; //Size of the message sent
} message_sent;

/*****************************************************
clipboard_conect: funtion that conects the app to a local clipboard
******************************************************/
int clipboard_connect(char * clipboard_dir);

/*****************************************************
clipboard_copy: funtion that copies the data pointed by a buf to region on the local clipboard
******************************************************/
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count);

/*****************************************************
clipboard_paste: funtion that copies from the system the data in a certain region. the data is stored in memory pointed by buf
******************************************************/
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count);

/*****************************************************
clipboard_wate: funtion that waits for a change in a certain region, and new data is copied in that region, the new data
is stored in memory pointed by buf
******************************************************/
int clipboard_wait(int clipboard_id, int region, char *buf, size_t count);

/*****************************************************
clipboard_close: funtion that closes the connection between the aplication and the local clipboard
******************************************************/
void clipboard_close(int clipboard_id);

#endif
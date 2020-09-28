#include "clipboard.h"
#include "threads.h"



/*************************************************************************
	thread_code: Thread that communicates between clipboard and apps
	\ param *client: ID of the client used for communication

	**********************************************************************/

void * thread_code(void * client)
{	
	message_sent message_sent_1;
	char * message;
	int nbytes, i;
	int err;
	int operation;
	int * client_fd = client;
	int client_id = *client_fd;
	int checkerror=0;

	while(1)
	{	
		//Types of operations possible: [1] - COPY, [0] - PASTE, [2] WAIT
		nbytes = read(client_id, &operation, sizeof(operation)); //Receives the type of operation: 
		if(nbytes==0) // If app is disconnected breaks from the while loop to reach the end of the thread	
		{
			printf("APP disconnected\n");
			break;
		}
		//COPY 
		if(operation == 1) 
		{
			//Recieves the structure sent by clipboard_copy
			read(client_id, &checkerror, sizeof(int)); //verifica se a library detetou algum erro
			if(checkerror==1)
			{
				continue;
			}
			nbytes = read(client_id, &message_sent_1, sizeof(message_sent_1)); // Reads the message sent by the app
			if(nbytes<=0)
			{
				perror("MESSAGE NOT RECIEVED\n");
				exit(-1);
			}

			if(message_sent_1.region < 0 || message_sent_1.region > 9) // Verifies if the region trying to be accessed is valid if not notifies the library
			{
				checkerror = 1;
				write(client_id, &checkerror, sizeof(int));
			}
			else
			{
				checkerror = 0;
				write(client_id, &checkerror, sizeof(int));

				//Allocs memory for the new message
				message = (char *)malloc(message_sent_1.message_size+1);
				if(message == NULL)
				{
					perror("New message not allocated\n");
					exit(-1);
				}

				//Reads the message from the app and stores it in the string allocated
				err = read(client_id, message, message_sent_1.message_size+1);
				if(err == 0)
				{
					perror("Message not Received\n");
					exit(-1);
				}

				if(message == NULL) //Verifies if the message being sent by the app is NULL and notifies the library
				{
					checkerror = 2;
					write(client_id, &checkerror, sizeof(int));
				}
				else{

					checkerror = 0;
					write(client_id, &checkerror, sizeof(int));

					pthread_rwlock_wrlock(&lockClipboard); // Acquires the writer lock
					//Gets rid of old information
					if(clipboard_vector[message_sent_1.region]!=NULL)
					{
						free(clipboard_vector[message_sent_1.region]);
					}

					clipboard_vector[message_sent_1.region]=message; // Stores the new message in the corresponding clipboard region
					word_size[message_sent_1.region]=message_sent_1.message_size; 
					pthread_rwlock_unlock(&lockClipboard); //Releases the writer lock


					pthread_mutex_lock(&mux); // Acquires the mutex lock
					
					if(waiting[message_sent_1.region]!=0)
					{
						pthread_cond_broadcast(&condwait[message_sent_1.region]); // Broadcasts the condition to all waiting threads
						waiting[message_sent_1.region]=0; // Resets the number of threads waiting for a certain region to zero
					}

					pthread_mutex_unlock(&mux); // Releases the mutex lock

					for(i=0;i<10;i++) // Prints the full clipboard 
					{
						if(clipboard_vector[i]!=NULL)
							printf("clipboard[%d] = %s", i, clipboard_vector[i]);
						if(clipboard_vector[i]==NULL)
							printf("clipboard[%d] = EMPTY\n", i);
					}
					printf("\n");
					
					if(clipboard_type==ONLINE) // If it is a clipboard child, sends the change to its father 
					{
						err = write(connected_socket, &message_sent_1, sizeof(message_sent));
						if(err==0)
						{
							perror("MESSAGE NOT SENT\n");
							exit(-1);
						}
						err = write(connected_socket, message, message_sent_1.message_size);
						if(err==0)
						{
							perror("MESSAGE NOT SENT\n");
							exit(-1);
						}
					}
					if(clipboard_type == OFFLINE && clipboard_last==NOTLAST) // If clipboard is only a father and has at least one child distributes the new message to every child clipboard
					{
						clip_thread * aux=NULL;
						aux = threadList;
						//List has more than one element
						while(aux->next!=NULL)
						{
							err = write(aux->client_id, &message_sent_1, sizeof(message_sent));
							if(err==0)
							{
								perror("MESSAGE NOT SENT\n");
								exit(-1);
							}
							err = write(aux->client_id, message, (message_sent_1.message_size));
							if(err==0)
							{
								perror("MESSAGE NOT SENT\n");
								exit(-1);
							}
							aux=aux->next;
						}
						//If List only has one element
						if(aux->next==NULL)
						{
							err = write(aux->client_id, &message_sent_1, sizeof(message_sent));
							if(err==0)
							{
								perror("MESSAGE NOT SENT\n");
								exit(-1);
							}
							err = write(aux->client_id, message, (message_sent_1.message_size));
							if(err==0)
							{
								perror("MESSAGE NOT SENT\n");
								exit(-1);
							}
						}						
					}
				}
			}
			checkerror=0;
		}

		//PASTE
		if(operation == 0)
		{
			read(client_id, &checkerror, sizeof(int)); //Verifies if the region inserted is valid, if not, returns to the top of the while loop
			if(checkerror==1)
			{
				continue;
			}
			
			nbytes = read(client_id, &message_sent_1, sizeof(message_sent_1)); // Reads the message sent by the app
			if(nbytes<=0)
			{
				perror("MESSAGE NOT RECEIVED\n");
				exit(-1);
			}

			if(message_sent_1.region < 0 || message_sent_1.region > 9 || word_size[message_sent_1.region]==0) // checks if there is any error in the region inserted
			{
				checkerror = 1;
				write(client_id, &checkerror, sizeof(int));
			}
			else
			{
				checkerror = 0;
				write(client_id, &checkerror, sizeof(int));

				char * buf = (char *)malloc(word_size[message_sent_1.region]+1); //Allocs memory for the tempoary buffer

				pthread_rwlock_rdlock(&lockClipboard); //Acquires the read lock
				memcpy(buf, clipboard_vector[message_sent_1.region], word_size[message_sent_1.region]); //copies the content of clipboard to the buffer
				pthread_rwlock_unlock(&lockClipboard); // Releases the lock
				err = write(client_id, buf, word_size[message_sent_1.region]); // sends the message to the app
				if(err == 0)
				{
					perror("MESSAGE NOT SENT");
					exit(-1);
				}
				free(buf); // frees the memory allocated by buffer
			}
			checkerror=0;
		}
		//WAIT
		if(operation == 2)
		{
			read(client_id, &checkerror, sizeof(int));//Verifies if the region inserted is valid, if not, returns to the top of the while loop
			if(checkerror==1)
			{
				continue;
			}

			nbytes = read(client_id, &message_sent_1, sizeof(message_sent_1)); // Reads the message being sent by the library
			if(nbytes==0)
			{
				perror("MESSAGE NOT RECEIVED\n");
				exit(-1);
			}

			if(message_sent_1.region < 0 || message_sent_1.region > 9) //Checks if the region inserted is valid and informs the app
			{
				checkerror = 1;
				write(client_id, &checkerror, sizeof(int));
			}
			else
			{
				checkerror = 0;
				write(client_id, &checkerror, sizeof(int));
				//increments the position of the vector that shows how many threads are waiting, corresponding to the region asked for by the app
				waiting[message_sent_1.region]++;
				
				pthread_mutex_lock(&mux); //Acquires the mutex lock

				pthread_cond_wait(&condwait[message_sent_1.region], &mux); // Waits for the condition to be broadcasted
				char * buf = (char *)malloc(word_size[message_sent_1.region]+1); // allocs memory for a temporary buffer
				pthread_rwlock_rdlock(&lockClipboard); // Acquires the clipboard read lock
				memcpy(buf, clipboard_vector[message_sent_1.region], word_size[message_sent_1.region]); // copies the content of the clipboard to the temporary buffer
				pthread_rwlock_unlock(&lockClipboard); // Releases the clipboard read lock
				pthread_mutex_unlock(&mux); // releases the mutex lock

				err =write(client_id, clipboard_vector[message_sent_1.region], word_size[message_sent_1.region]); // sends the message to the waiting app
				if(err <= 0)
				{
					perror("APP DISCONNECTED");
					free(buf);
					break;
				}
				free(buf); // frees the memory allocated for the buffer
	
			}
			checkerror=0;
		}
	}
	close(client_id); // If the app is disconnected, breaks from the while loop and closes the socket 

	return NULL;
}

/***********************************************
 ThreadListAdd: Adds the structure with new thread information to the thread list
 \ param *newthread: structure with the information of the new clipboard connected
**********************************************/

void ThreadListAdd(clip_thread * newthread) // Adds a the structure of the new connected thread to the thread list 
{	
	clip_thread * aux;
	//Header 
	if (threadList == NULL){ // If the list is empty, newthread becomes head
		threadList = newthread;
		threadList->next = NULL;
	}
	else{ //If not, looks for the last node on the list and inserts newthread at the end of the list
			aux=threadList;

			while(aux->next!=NULL)
				aux=aux->next;

		newthread->next = NULL;
		aux->next = newthread;
	}

}

/***********************************************
	ThreadListRemove: Removes the structure with the information of the disconnected clipboard
	\ param id: id of the clipboard that was disconnected
**********************************************/

void ThreadListRemove(int id) // Removes the structure of the disconnected clipboard from the list
{
	clip_thread * aux=NULL;
	clip_thread * aux2=NULL;
	aux=threadList;
	aux2=threadList;

	if(aux->client_id == id) // If the structure to be removed is the head
	{
		threadList=aux->next;
		free(aux);
	}
	else
	{
		while(aux->client_id != id) // Looks for the corresponding node
		aux=aux->next;

		while(aux2->next->client_id != id) // Puts the aux2 right behind aux
			aux2=aux2->next;

		aux2->next=aux->next; // Points aux 2 to the node after aux and frees aux
		aux->next=NULL; 
		free(aux);
	}
}

/***********************************************
	ClipboardThread_UP_DOWN: Thread that passes information from the upper clipboard to the lower clipboards
	/ param *up_socket: id of the upper socket 
***********************************************/

void * ClipboardThread_UP_DOWN(void * up_socket) // Thread to comunicate from the upper thread to the lower threads
{
	message_sent message_sent_1;
	char* message;
	clip_thread * aux=NULL;
	int * up_socket_fd = up_socket;
	int up_socket_id = *up_socket_fd;
	int err=0;
	
	while(1)
	{
		if(clipboard_type == ONLINE) // If the clipboard is connected to an upper clipboard 
		{
			err = read(up_socket_id, &message_sent_1, sizeof(message_sent_1)); // reads the message being sent by the upper clipboard
			if(err<=0) // if the message cannot be read it means that the clipboard was disconnected
			{
				perror("UPPER CLIPBOARD DISCONNECTED\n"); // Breaks from the while loop to close the socket and leave the thread
				break;
			}
			message = (char *)malloc(message_sent_1.message_size+1); // Allocs memory for the new message
			if(message == NULL)
			{
				perror("New message not allocated");
				exit(-1);
			}

			//Copies the message to the string
			err = read(up_socket_id, message, (message_sent_1.message_size+1)); // Reads the message being sent
			if(err <= 0)
			{
				perror("Message not Recieved");
				exit(-1);
			}

			pthread_rwlock_wrlock(&lockClipboard); // Acquires the write lock
			//Gets rid of old information
			if(clipboard_vector[message_sent_1.region]!=NULL) // If region is not empty frees the information
			{
				//erases old message
				free(clipboard_vector[message_sent_1.region]);
			}
			clipboard_vector[message_sent_1.region]=message; // Copies the message to the vector
			word_size[message_sent_1.region]=message_sent_1.message_size; // Updates the size of the message saved on the clipboard
			pthread_rwlock_unlock(&lockClipboard); // Releases the lock


			pthread_mutex_lock(&mux); // Acquires the mutex lock
			if(waiting[message_sent_1.region]!=0) // checks if there is any thread waiting for this region
			{
				pthread_cond_broadcast(&condwait[message_sent_1.region]); // Broadcasts the signal to all the waiting threads
				waiting[message_sent_1.region]=0; // Resets the number of thread waiting for this region
			}
			pthread_mutex_unlock(&mux); // Releases the mutex lock


			printf("FATHER CLIPBOARD SENT: %s\n", message);
		}
		
		if(clipboard_last == NOTLAST) //If the clipboard has any child it will go through the child clipboard list
		{
			aux = threadList; 
			//List has more than one element
			while(aux->next!=NULL) // goes through the whole list
			{
				err = write(aux->client_id, &message_sent_1, sizeof(message_sent)); // sends message information to child clipboard
				if(err==0)
				{
					perror("MESSAGE NOT RECIEVED\n");
					exit(-1);
				}
				err = write(aux->client_id, message, (message_sent_1.message_size)); // sends message to child clipboard
				if(err==0)
				{
					perror("MESSAGE NOT RECIEVED\n");
					exit(-1);
				}
				aux=aux->next; 
			}
			if(aux->next==NULL) // If List only has one element writes straight away
			{
				err = write(aux->client_id, &message_sent_1, sizeof(message_sent)); // sends message information to child clipboard
				if(err==0)
				{
					perror("MESSAGE NOT RECIEVED\n");
					exit(-1);
				}
				err = write(aux->client_id, message, (message_sent_1.message_size)); // sends message to child clipboard
				if(err==0)
				{
					perror("MESSAGE NOT RECIEVED\n");
					exit(-1);
				}
			}
		}
	}

	close(up_socket_id); // Closes the socket used to communicate with the upper thread
	clipboard_type=OFFLINE; // Makes clipboard offline, meaning it has no upper clipboard
	return NULL;
}

/***********************************************
	ClipboardThread_DOWN_UP: Thread that passes information from the lower clipboard to the  clipboards
	/ param *up_socket: id of the lower socket 
***********************************************/

void * ClipboardThread_DOWN_UP(void * down_socket)
{
	message_sent message_sent_1;
	clip_thread *aux;
	char* message;
	int * down_socket_fd = down_socket;
	int down_socket_id = *down_socket_fd;
	int err=0;

	while(1)
	{		
		err = read(down_socket_id, &message_sent_1, sizeof(message_sent)); // Reads the message information being sent by the lower clipboard
		if(err<=0)
		{
			perror("LOWER CLIPBOARD DISCONNECTED\n"); // if it is unable to read means that the clipboard was disconnected, breaks from the loop to close the socket and leave the thread
			break;
		}
		message = (char *)malloc(message_sent_1.message_size+1); //Allocs memory for the message
		if(message == NULL)
		{
			perror("New message not allocated");
			exit(-1);
		}

		//Copies the message to the string
		err = read(down_socket_id, message, message_sent_1.message_size+1); // Reads the message being sent by the lower clipboard
		if(err <= 0)
		{
			perror("Message not Recieved");
			exit(-1);
		}

		
		if(clipboard_type==ONLINE) // if the clipboard has a father sends the information 
		{
		
			write(connected_socket, &message_sent_1, sizeof(message_sent));
			err = write(connected_socket, message, message_sent_1.message_size);
			if(err == 0)
				{
					perror("MESSAGE NOT SENT");
					exit(-1);
				}

		}
		else if(clipboard_type==OFFLINE) // If the clipboard doesn't have a father, message has reached the top of the tree, starts copying it downwards to every child 
		{
			pthread_rwlock_wrlock(&lockClipboard); // Acquires the write lock
			
			if(clipboard_vector[message_sent_1.region]!=NULL) // if region is not empty frees the old message
			{
				free(clipboard_vector[message_sent_1.region]);
			}
			clipboard_vector[message_sent_1.region]=message; // copies message to clipboard
			word_size[message_sent_1.region]=message_sent_1.message_size; // updates wordsize of the new message
			pthread_rwlock_unlock(&lockClipboard); //releases the clipboard lock
			printf("SON CLIPBOARD SENT: %s\n", message);

			pthread_mutex_lock(&mux); // Acquires the mutex lock
			if(waiting[message_sent_1.region]!=0) // checks if there is any thread waiting for this region
			{
				pthread_cond_broadcast(&condwait[message_sent_1.region]); // Broadcasts the signal to all the waiting threads
				waiting[message_sent_1.region]=0; // Resets the number of thread waiting for this region
			}
			pthread_mutex_unlock(&mux); // Releases the mutex lock

			//Funcao enviar para baixo
			aux = threadList;
			//List has more than one element
			while(aux->next!=NULL) // goes through the whole list
			{
				err = write(aux->client_id, &message_sent_1, sizeof(message_sent));
				if(err==0)
				{
					perror("MESSAGE NOT RECIEVED\n");
					exit(-1);
				}
				err = write(aux->client_id, message, message_sent_1.message_size);
				if(err==0)
				{
					perror("MESSAGE NOT RECIEVED\n");
					exit(-1);
				}
				aux=aux->next;
			}
			//List only has one element
			if(aux->next==NULL) // If List only has one element writes straight away
			{
				err = write(aux->client_id, &message_sent_1, sizeof(message_sent));
				if(err==0)
				{
					perror("MESSAGE NOT RECIEVED\n");
					exit(-1);
				}
				err = write(aux->client_id, message, message_sent_1.message_size);
				if(err==0)
				{
					perror("MESSAGE NOT RECIEVED\n");
					exit(-1);
				}
			}
		}
	}
	pthread_mutex_lock(&listmux); // Acquires the list mutex lock
	ThreadListRemove(down_socket_id); // Removes the disconnected clipboard information from the list
	pthread_mutex_unlock(&listmux); // releases the list mutex lock
	if(threadList==NULL) // if the list becomes empty it means it has no childs, the clipboard is the last on the tree
	{
		clipboard_last=LAST; 
	}
	close(down_socket_id); // closes the socket that communicated with the lower clipboard

	return 0;
}

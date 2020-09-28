#include "clipboard.h"
#include "threads.h"

//CLIPBOARD TYPE: ONLINE, OFFLINE
int clipboard_type = OFFLINE;
int clipboard_last = LAST;

//Clipboard
char * clipboard_vector[10]={NULL};
int word_size[10]={0};
int waiting[10]={0};

//Internet Socket
struct sockaddr_in server_adress;
struct sockaddr_in client_adress;
struct sockaddr_in connected_adress;
socklen_t size_addr;
int connected_socket=0;
int sock_fd;
int internet_socket;

//Initializes all pthread locks
pthread_rwlock_t lockClipboard;
pthread_mutex_t mux;
pthread_mutex_t listmux;
pthread_cond_t condwait[10];

//Header of the List that stores client_id's
clip_thread *threadList=NULL;

/*****************************************************
sig_handler: Funtion that closes all sockets, upper lower and the ones connected to the app
\param sig Crl-c signal 
******************************************************/
void sig_handler(int sig)
{
	int i=0;
	clip_thread *aux = threadList;
	clip_thread *aux2;

	close(sock_fd); //Closes all local sockets connected to clipboard

	close(internet_socket); //closes all sun sockets

	close(connected_socket); //Closes all father sockets

	//Frees all the memory allocated for the clipboard
	for(i=0; i<10; i++) 
	{
		if(clipboard_vector[i]!=NULL)
			free(clipboard_vector[i]);
	}
	//frees the client_id list
	if(aux!=NULL)
	{
		while(aux->next!=NULL)
		{
			aux2 = aux;
			aux = aux->next;
			free(aux2);
		}
		free(aux); //frees last node
	}

	unlink(SOCK_ADDRESS);  //deletes the name of the socket from the fyle system

	exit(0);
}

/**********************************************/
/*recieve_clipboard: Function that receives the clipboard sent by father clipboard
\param client_fd client id that recieves the clipboard sent by the father
****************************************/
void * recieve_clipboard(int client_fd)
{
	int i, err;
	char *message; 

	//Recieves the word_size vector
	for(i = 0; i<10; i++)
	{	
		err = read(client_fd, &word_size[i], sizeof(int));
		if(err==0)
		{
			perror("MESSAGE NOT RECIEVED\n");
			exit(-1);
		}
	}
	//Recieves clipboard vector
	for (i = 0; i < 10; ++i)
	{
		if(word_size[i]!=0)
		{
			message = (char *)malloc(word_size[i]+1);
			clipboard_vector[i] = message;
			err=read(client_fd, clipboard_vector[i], word_size[i]);
			if(err==0)
			{
				perror("MESSAGE NOT RECIEVED\n");
				exit(-1); //if clipboard is not sent, clipboard is closed
			}
			printf("clipboard_vector[%d]: %s\n", i, clipboard_vector[i]);
		}
	}
	return 0;
}

/**********************************************/
/*send_clipboard: Function that sends the clipboard vector once clipboard is connected
\param socket_fd Socked id that sends the clipboard to a son socket
****************************************/
void * send_clipboard(int socket_fd)
{
	int i, err;
	int aux;
	
	//Sends the word_size vector
	for(i = 0; i<10; i++)
	{
		pthread_rwlock_rdlock(&lockClipboard); //Once clipboard is transfering data to the Son clipboard, clipboard cant be changed
		aux=word_size[i];
		pthread_rwlock_unlock(&lockClipboard);
		err = write(socket_fd, &aux, sizeof(aux));
		if(err==0)
		{
			perror("MESSAGE NOT RECIEVED\n");
			exit(-1); // If clipboard cant be sent, clipboard shuts down
		}

	}
	//Sends the clipboard vector
	for(i = 0; i<10; i++)
	{
		//only sends the regions that have words in it
		if(word_size[i]!=0)
		{	
			char * buf = (char *)malloc(word_size[i]+1);
			pthread_rwlock_rdlock(&lockClipboard); //Once clipboard is transfering data to the Son clipboard, clipboard cant be changed
			memcpy(buf, clipboard_vector[i], word_size[i]);
			pthread_rwlock_unlock(&lockClipboard);
			err=write(socket_fd, buf, word_size[i]);
			if(err==0)
			{
				perror("MESSAGE NOT RECIEVED\n");
				exit(-1);
			}
			free(buf); //Frees the used buff
		}
	}	
	return 0;
}


/**********************************************/
/*inet_connections: Function that is always expecting connections from other clipboards
\param internet_socked Internet socked id that
*************************************************/
void * inet_connections(void *internet_socket)
{
	//Convert internet_socket to int 
	int * internet_socket_fd = internet_socket;
	int internet_socket_id = *internet_socket_fd;

	size_addr = sizeof(server_adress);

	//Waits for 5 connections
	listen(internet_socket_id, 5);

	while(1)
	{
		//If socket ready to recieve connections
		int client_fd= accept(internet_socket_id, (struct sockaddr *) &client_adress, &size_addr);
		if(client_fd == -1) {
			perror("accept failed");
			exit(-1);
		}
		//Is not the last clipboard
		clipboard_last = NOTLAST;

		//Sends the actual clipboard to the new son
		send_clipboard(client_fd);

		//Creates a thread that comunicates with the upper clipboard if a change is made
		pthread_t thread_ID;
		pthread_create(&thread_ID, NULL, ClipboardThread_DOWN_UP, &client_fd); 

		//Creates a node and stores the new client fd in the list
		clip_thread * newthread = (clip_thread *)malloc(sizeof(clip_thread));
		pthread_mutex_lock(&listmux); //Once you are adding an element in the list you can not change it
		newthread->client_id = client_fd;
		ThreadListAdd(newthread);
		pthread_mutex_unlock(&listmux);

		printf("CLIPBOARD CONNECTED\n");
	}

}

/**********************************************
main: main funtion, creates binds a inet socket to connect with clipboards, creates a local socket to connect with apps
, creats the threads that expect connections and 
\param internet_socked Internet socked id that
*************************************************/

int main(int argc, char const *argv[]){
	
	int i=0;
	//Local Socket 
	struct sockaddr_un local_addr;
	struct sockaddr_un local_client_addr;

	//IP and port 
	char ip_aux[14];

	//Signal Ctrl-C
	signal(SIGINT, sig_handler);

	//Generates a INET socket to comunicate with others
	internet_socket = socket(AF_INET, SOCK_STREAM, 0);
	 if (internet_socket == -1){
		perror("Internet Socket not created");
		exit(-1);
	}

	//Plants the seed for srand
	srand(time(NULL));
	int port = rand()%(64738-1024)+1024; //Generates a valid port number
	printf("PORTNUMBER: %d\n", port);

	//define the adress 
	server_adress.sin_port = htons(port); 
	server_adress.sin_addr.s_addr= INADDR_ANY;
	server_adress.sin_family = AF_INET;
	size_addr = sizeof(server_adress);

	//Adress the net socket using bind
	int err=bind(internet_socket, (struct sockaddr*) &server_adress, sizeof(server_adress));
	if(err == -1) {
		perror("bind failed");
		exit(-1);
	}
	//Starts listening for connections, maximum os 10
	listen(internet_socket, 10);

	//If argument count is not 1, clipboard is in ONLINE mode
	if(argc!=1)
	{
		//Define clipboard type
		clipboard_type = ONLINE;
		//Socket responsible to comunicate with upper clipboards
		connected_socket = socket(AF_INET, SOCK_STREAM, 0); //stores the i_net socket adress in connected_socket
		 if (connected_socket == -1){
			perror("Internet Socket not created");
			exit(-1);
		} 
		strcpy(ip_aux, argv[2]); //copies the ip to an auxiliar 
		port = atoi(argv[3]); //converts the port from char to int

		//define the adress 
		connected_adress.sin_family = AF_INET;
		connected_adress.sin_port = htons(port);
		inet_aton(ip_aux, &connected_adress.sin_addr);

		//Connects to the single clipboard
		if(connect(connected_socket, (struct sockaddr *) &connected_adress, size_addr)==-1)
		{
			perror("UNABLE TO CONNECT TO CLIPBOARD");
			exit(-1);
		}
		//Recieves the clipboard sent by father
		recieve_clipboard(connected_socket);
	
		//Thread that comunicate with upper clipboards
		pthread_t thread_ID;
		pthread_create(&thread_ID, NULL, ClipboardThread_UP_DOWN, &connected_socket); 

	}

	//initializes the rwlock
	if(pthread_rwlock_init(&lockClipboard, NULL)!=0)
	{
		perror("rwlock init failed\n");
		exit(-1);
	}
	//initializes the mutex
	if(pthread_mutex_init(&mux, NULL)!=0)
	{
		perror("mutex init failed\n");
		exit(-1);
	}
	//initializes the list mutex
	if(pthread_mutex_init(&listmux, NULL)!=0)
	{
		perror("list mutex init failed\n");
		exit(-1);
	}
	//initializes a condion variable used for each region
	for ( i = 0; i < 10; ++i)
	{
		if(pthread_cond_init(&condwait[i], NULL)!=0)
		{
			perror("cond init failed\n");
			exit(-1);
		} 
	}
	
	//Creation of thread waits for connections from other clip.
	pthread_t thread_id_inet;
	pthread_create(&thread_id_inet, NULL, inet_connections, &internet_socket); 

	//Creats the local socket
	sock_fd= socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock_fd == -1){
		perror("Local socket not created");
		exit(-1);
	}
	local_addr.sun_family = AF_UNIX;

	//Adress the socket usind bind function 
	strcpy(local_addr.sun_path, SOCK_ADDRESS);
	unlink(SOCK_ADDRESS);
	err = bind(sock_fd, (struct sockaddr *)&local_addr, sizeof(local_addr));
	if(err == -1) {
		perror("bind failed");
		exit(-1);
	}
	
	//Socket is ready to listen to possible communications
	if(listen(sock_fd, 2) == -1) {
		perror("listen failed");
		exit(-1);
	}

	//size of addr
	size_addr = sizeof(struct sockaddr);

	//Waits for an app comunication and then creats a thread
	while(1)
	{
		int client_fd= accept(sock_fd, (struct sockaddr *) &local_client_addr, &size_addr);
		if(client_fd == -1) {
			perror("accept failed");
			exit(-1);
		}
		//Thread that does the operations on app
		pthread_t thread_id;
		pthread_create(&thread_id, NULL, thread_code, &client_fd); 
	}	
	exit(0);
}

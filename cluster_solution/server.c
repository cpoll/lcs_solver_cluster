#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>

int main(int argc, char *argv[])
{

	/*******************/
	/* Parse Arguments */
	/*******************/

	int max_connections = 2; //Default

	if(argc>1)
	{
		max_connections = atoi(argv[1]);
	}


	/***********************************************/
	/* Setup / Prepare to communicate with workers */
	/***********************************************/

	struct sockaddr_in server_address;
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port = htons(8000);

	int server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(bind(server_socket, (const struct sockaddr *) &server_address, 
						sizeof(server_address))<0)
	{
		printf("Failed to bind server.\n");
		return -1;
	}
	if(listen(server_socket, max_connections)<0)
	{
		printf("Failed to set server socket to listen\n");
		return -1;
	}

	/*************************/
	/* Establish Connections */
	/*************************/

	int client_sockets[max_connections];
	struct sockaddr_in client_address;
	socklen_t sizeof_client_address = sizeof(client_address);
	int num_connected = 0;
	in_addr_t client_IPs[max_connections];

	printf("Waiting on %i clients to connect\n", max_connections);
	while (num_connected < max_connections)
	{

		if ((client_sockets[num_connected] = accept(server_socket,
					(struct sockaddr *) &client_address,
					&sizeof_client_address))<0)
		{	
			printf("Server failed to accept client");
			return -1;
		}
		
		client_IPs[num_connected] = client_address.sin_addr.s_addr;
				
		printf("Client %i has reported in\n", num_connected);
		num_connected++;
	}

	/****************/
	/* Start Timing */
	/*****************/
	
        //Get start time.
        struct timeval tv;
        long int timesec1 = time(NULL);
        gettimeofday(&tv, NULL);
        float timems1 = tv.tv_usec*0.001;


	/************************/
	/* Send Info to Clients */
	/************************/

	/* Notes:
		Each client is informed of its number, the total number of clients, and
		the IP (in network form) of the one next in the sequence).

		The last client (number == total-1) will send data only to the server.	
	*/
	int i, j;
	int buffer_size = 30; 
	char buffer[buffer_size];
	in_addr_t next_IP;

	for(i=0; i<num_connected; i++)
	{	
	
		//Last client is not given a valid IP, as it reports to server.	
		if(i<num_connected-1)
			next_IP = client_IPs[i+1];
		else
			next_IP = 0;
		
		//Hacky Null Termination
		for(j=0; j<buffer_size;j++)
			buffer[j] = '\0';	

		//Send data. Client can use strtok on + later.
		sprintf(buffer, "%i+%i+%i", i, num_connected, next_IP);
		if (send(client_sockets[i], buffer, buffer_size, 0) != buffer_size)
		{
			printf("Sending info to Client %i failed.\n", i);
			return -1;
		}

	}


	/********************************************************/
	/* Listen for response from Client (max_connections -1) */
	/********************************************************/

	int recieve_size;

	
	//MOre Hacky Null Termination
	for(j=0; j<buffer_size;j++)
		buffer[j] = '\0';	



	if((recieve_size = recv(client_sockets[num_connected-1], buffer, buffer_size, 0)) <0)
	{
		printf("Recieve from Client %i failed.\n", num_connected-1);
		return -1;
	}
	
	printf("\nRecieved result from Client %i:\n --- Size of LCS = %s\n", num_connected-1, buffer);


	/********************************************/
	/* Listen for wrap-up data from each client */
	/********************************************/

	//Note: These numbers are useless when we use a blocking send.
	/*
	for(i=0;i<num_connected;i++)
	{

		memset(buffer, '\0', buffer_size);

		if((recieve_size = recv(client_sockets[i], buffer, buffer_size, 0)) <0)
		{
			printf("Recieve from Client %i failed.\n", i);
			return -1;
		}
	
		printf("Runtime of Client %i: %s ms\n", i, buffer);


	}*/

	/***********/
	/* Wrap-Up */
	/***********/

        //Get end time and print time elapsed.
        long int timesec2 = time(NULL);
        gettimeofday(&tv, NULL);
        float timems2 = tv.tv_usec*0.001;
        float runtime = (timesec2-timesec1)*1000 + (timems2-timems1);
        printf("\nRuntime: %f ms\n\n", runtime);


	return 0;

}

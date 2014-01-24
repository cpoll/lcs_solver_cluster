#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>


int main(int argc, char *argv[])
{

	/**************/
	/* Parse args */
	/**************/

	if (argc < 2)
	{
		printf("Usage: %s <Server IP>\n", argv[0]);
		return 0;
	}

	char *server_IP = argv[1];

        /**********************************************/
        /* Setup / Prepare to communicate with server */
        /**********************************************/

        struct sockaddr_in server_address;
        memset(&server_address, 0, sizeof(server_address));
        server_address.sin_family = AF_INET;
        server_address.sin_addr.s_addr = inet_addr(server_IP);
        server_address.sin_port = htons(8000);

        int client_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

        if( (connect(client_socket, (struct sockaddr *) &server_address,
					sizeof(server_address)))<0)
                printf("Failed to connect to server\n");




	return 0;

}

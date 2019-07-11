#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <linux/limits.h>
#include "stdlib.h"

#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>

#define GPIO_CTRL_DEFAULT 16

#define GPIO_DIRECTION_OUT 1
#define GPIO_DIRECTION_IN  0

#define SOCK_ACK   "OK"
#define SOCK_NOACK "FAIL"


int gpio_export(int gpio, int is_out)
{
	char filename[PATH_MAX];
	FILE *file;

	file = fopen("/sys/class/gpio/export", "w");
	if (file == NULL) return -1;
	fprintf(file, "%d\n", gpio);
	fclose(file);

	snprintf(filename, sizeof(filename), "/sys/class/gpio/gpio%d/direction", gpio);
	file = fopen(filename, "w");
	if (file == NULL) return -1;
	fprintf(file, "%s\n", is_out ? "out" : "in");
	fclose(file);

	return 0;
}

int gpio_set(int gpio, int value)
{
	char filename[PATH_MAX];
	FILE *file;

	snprintf(filename, sizeof(filename), "/sys/class/gpio/gpio%d/value", gpio);
	file = fopen(filename, "w");    if (file == NULL) return -1;
	fprintf(file, "%d\n", value ? 1 : 0);
	fclose(file);
	return 0;
}

int gpio_pwm(int gpio, int count, int high_usec, int low_usec)
{
	char filename[PATH_MAX];
	FILE *file;
	snprintf(filename, sizeof(filename), "/sys/class/gpio/gpio%d/value", gpio);

	for (; count; count--)
	{
		file = fopen(filename, "w");    if (file == NULL) return -1;
		fprintf(file, "%d\n", 1);
		fclose(file);

		usleep(high_usec);

		file = fopen(filename, "w");    if (file == NULL) return -1;
		fprintf(file, "%d\n", 0);
		fclose(file);

		usleep(low_usec);
	}
	return 0;
}

/* angle -90..90 */
int sg90_rotate(int gpio, int angle, int rotate_duration)
{
	/* 1000 usec + 500 + angle*1000/80 */
	int duration = 1000 + 500 + angle * 1000 / 180;
	/* 2 ms pulse: |1ms+0-1ms|___rest_of_2ms____| */
	return gpio_pwm(gpio, rotate_duration, duration, 20000 - duration);
}

int run_server(int gpio, int port)
{
	int socket_desc , client_sock , c , read_size;
	struct sockaddr_in server , client;
	char client_message[2000];

	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1)
	{
		printf("Could not create socket");
	}
	puts("Socket created");

	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( port );

	//Bind
	if ( bind(socket_desc, (struct sockaddr *)&server , sizeof(server)) < 0)
	{
		//print the error message
		perror("bind failed. Error");
		return 1;
	}
	puts("bind done");

	//Listen
	listen(socket_desc , 3);

	//Accept and incoming connection
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);

	//accept connection from an incoming client
	client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
	if (client_sock < 0)
	{
		perror("accept failed");
		return 1;
	}
	puts("Connection accepted");

	//Receive a message from client
	while ( (read_size = read(client_sock , client_message , sizeof(client_message))) > 0 )
	{
		//Send the message back to client
		char cmd = client_message[0];
		int rotate_angle, rotate_duration;
		int result = 0;
		switch (cmd)
		{
		case 'R':
			if (sscanf(client_message, "R %d %d", &rotate_angle, &rotate_duration) == 1)
			{
				printf("Rotating %d for %d pulses..\n", rotate_angle, rotate_duration);
				sg90_rotate(gpio, rotate_angle, rotate_duration);
				printf("Done.\n");
			}
			else
			{
				printf("Rotate: unable to parse\n");
				result = 1;
			}
			break;
		default:
			printf("Unknown command %c", cmd);
			result = 1;
			break;
		}
		if (result == 0)
		{
			printf("Sending ACK\n");
			result = (write(client_sock , SOCK_ACK, strlen(SOCK_ACK)) > 0);
		}
		else
		{
			printf("ERROR: Unknown command\n");
			result = (write(client_sock , SOCK_NOACK, strlen(SOCK_NOACK)) > 0);
		}
		memset( &client_message, 0, sizeof(client_message));
	}

	if (read_size == 0)
	{
		puts("Client disconnected");
		fflush(stdout);
	}
	else if (read_size == -1)
	{
		perror("recv failed");
		return 1;
	}
	return 0;
}

int main(int argc, char** argv)
{
	int angle = 0;
	int gpio_ctrl = GPIO_CTRL_DEFAULT;
	int port = 1489;

	if (argc == 3)
	{
		gpio_ctrl = atoi(argv[1]);
		angle = atoi(argv[2]);
		gpio_export(gpio_ctrl, GPIO_DIRECTION_OUT);
		sg90_rotate(gpio_ctrl, angle, 10);
	}
	else if (argc == 4)
	{
		if (!strcmp(argv[1], "srv"))
		{
			port = atoi(argv[2]);
			gpio_ctrl = atoi(argv[3]);
			gpio_export(gpio_ctrl, GPIO_DIRECTION_OUT);
			run_server(gpio_ctrl, port);
		}
	}
	else
	{
		printf("Usage: sgrotate <gpio> <angle>\n");
		printf("Run server: sgrotate srv <port> <gpio>\n");
	}
}
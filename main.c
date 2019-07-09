#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <linux/limits.h>
#include "stdlib.h"

#define GPIO_CTRL_DEFAULT 16

#define GPIO_DIRECTION_OUT 1
#define GPIO_DIRECTION_IN  0

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
}

int gpio_pwm(int gpio, int count, int high_usec, int low_usec)
{
	char filename[PATH_MAX];
    FILE *file;

    snprintf(filename, sizeof(filename), "/sys/class/gpio/gpio%d/value", gpio);
    file = fopen(filename, "w");    if (file == NULL) return -1;
	for(;count;count--)
	{
	    fprintf(file, "%d\n", 1);
	    usleep(high_usec);
	    fprintf(file, "%d\n", 0);
	    usleep(low_usec);
	}

    fclose(file);
}

/* angle -90..90 */
int sg90_rotate(int gpio, int angle)
{
	/* 1000 usec + 500 + angle*1000/80 */
	int duration = 1000 + 500 + angle * 1000/180;
	/* 20 ms pulse: |1ms+0-1ms|___rest_of_20ms____| */
	gpio_pwm(gpio, 1, duration, 20000-duration);
}

int main(int argc, char** argv)
{
	int angle = 0;
	int gpio_ctrl = GPIO_CTRL_DEFAULT;

	if (argc == 3)
	{
		gpio_ctrl = atoi(argv[1]);
		angle = atoi(argv[2]);
	}
	else 
	{
		printf("Usage: sgrotate <gpio> <angle>\n");
	}
	
	gpio_export(gpio_ctrl, GPIO_DIRECTION_OUT);
	sg90_rotate(gpio_ctrl, angle);
}
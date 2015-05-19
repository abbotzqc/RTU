#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>

#include "ledlib.h"

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

#define LEDPATH "/sys/class/leds"

struct led_tab led_tab[] = {
    { "d6", 0 },
    { "d9", 0 },
    { "d13", 0 },
};

/**
* Write string into the file.
* @param pfile a pointer to file path string.
* @param svalue a pointer to string content.
* @return the error code, 0:success.
*/
static int write_file (const char *pfile, char *svalue)
{
    int fd = -1;
    int errcode = 0;

    if (pfile == NULL || svalue == NULL) {
      //  fprintf (stderr, "Warning: %s, null pointer\n", __func__);
        errcode = 2;     /* nop */
        goto out;
    }

	if(strcmp(pfile,"leds/d6/brightness")==0)
		fd=fd_ledsix;
	else 
		if(strcmp(pfile,"leds/d9/brightness")==0)
			fd=fd_lednine;
		else
		{
    fd = open (pfile, O_WRONLY);
    if (fd < 0) {
      //  fprintf (stderr, "Error: %s, open [%s] error\n", __func__, pfile);
        errcode = 1;
	//	close(fd);
        goto out;
    }
		}
    if (write (fd, svalue, strlen(svalue)) < 0) {
      //  fprintf (stderr, "Warning: %s, write error\n", __func__);
        errcode = 1;
        goto out1;
    }
out1:
    close (fd);
out:
    return errcode;
}

/**
 * Turn on or off led.
 * @param name a pointer to led name, eg. "d6".
 * @param onoff led switch, 0: off, 1: on.
 * @return the error code, 0:success.
 */
LED_API int led_ctrl (char *name, int onoff)
{
    int i = 0;
    int errcode = 0;
    while (i < ARRAY_SIZE(led_tab) && strcasecmp(led_tab[i].name, name) && ++i);

    if (i == ARRAY_SIZE(led_tab)) {
   //     fprintf (stderr, "Error: led_ctrl, led %s no found in led_tab\n", name);
        errcode = 1;
        goto out;
    }

    char node[32];
    sprintf (node, "%s/%s/brightness", LEDPATH, led_tab[i].name);

    if (write_file (node, onoff ? "1" : "0") != 0) {
    //    fprintf (stderr, "Error: led_ctrl, write error\n");
        errcode = 1;
        goto out;
    }
out:
    return errcode;
}


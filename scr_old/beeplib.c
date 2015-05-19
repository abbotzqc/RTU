#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>

#include "beeplib.h"

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

#define LEDPATH "/sys/class/leds"

struct beep_tab beep_tab[] = {
    { "beep", 0 },
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

	if(!(fd_beep>0))
	{	
    fd = open (pfile, O_WRONLY);
    if (fd < 0) {
       // fprintf (stderr, "Error: %s, open [%s] error\n", __func__, pfile);
        errcode = 1;
        goto out;
    }
	}
	else
		fd=fd_beep;
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
 * Turn on or off beep.
 * @param name a pointer to beep name.
 * @param onoff beep switch, 0: off, 1: on.
 * @return the error code, 0:success.
 */
BEEP_API int beep_ctrl (char *name, int onoff)
{
    int i = 0;
    int errcode = 0;
    while (i < ARRAY_SIZE(beep_tab) && strcasecmp(beep_tab[i].name, name) && ++i);

    if (i == ARRAY_SIZE(beep_tab)) {
       // fprintf (stderr, "Error: beep_ctrl, beep %s no found in beep_tab\n", name);
        errcode = 1;
        goto out;
    }

    char node[32];
    sprintf (node, "%s/%s/brightness", LEDPATH, beep_tab[i].name);

    if (write_file (node, onoff ? "1" : "0") != 0) {
      //  fprintf (stderr, "Error: beep_ctrl, write error\n");
        errcode = 1;
        goto out;
    }
out:
    return errcode;
}


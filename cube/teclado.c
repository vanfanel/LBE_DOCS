#include <sys/ioctl.h>
#include <linux/input.h>
#include <linux/kd.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

static struct termios oldTerm, newTerm;
static long oldKbmd;
int exit_condition;
float rAngle, rSpeed, rSign;

void restoreKeyboard()
{
    //Es FUNDAMENTAL que esta función sea llamada justo al acabar el programa con atexit(), y que NO
    //la llamemos nosotros, o el modo de teclado NO se restaurará correctamente.
    ioctl(0, KDSKBMODE, oldKbmd);
    tcsetattr(0, TCSAFLUSH, &oldTerm);
}

int setupKeyboard()
{
    
    /*if (!isatty (STDIN_FILENO))
    {
       printf ("Not a terminal. Exit Xorg!\n");
       exit (EXIT_FAILURE);
    }*/

    int flags;
    //RECUERDA que el descriptor de fichero de stdin es el entero 0, que es lo que le estamos
    //pasando aquí.
    /* make stdin non-blocking */
    flags = fcntl(0, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(0, F_SETFL, flags);

    /* save old keyboard mode */
    if (ioctl(0, KDGKBMODE, &oldKbmd) < 0) {
	return 0;
    }

    tcgetattr(0, &oldTerm);

    /* turn off buffering, echo and key processing */
    newTerm = oldTerm;
    newTerm.c_lflag &= ~(ECHO | ICANON | ISIG);
    newTerm.c_iflag &= ~(ISTRIP | IGNCR | ICRNL | INLCR | IXOFF | IXON);
    //newTerm.c_cc[VMIN] = 0;
    //newTerm.c_cc[VTIME] = 0;    
   
    ioctl(0, KDGKBMODE, &oldKbmd);
 
    tcsetattr(0, TCSAFLUSH, &newTerm);

    ioctl(0, KDSKBMODE, K_MEDIUMRAW);
    
    return 1;
}

void readKeyboard()
{
    //MAC Mientras el teclado está en modo RAW, podemos leer del file descriptor de stdin, que es 0, 
    //usando read() 
    char buf[1];
    int res;

    //Una vez más, no olvidemos que a read() le estamos pasando el descriptor de fichero de STDIN, que es 0
    /* read scan code from stdin */
    res = read(0, &buf[0], 1);
    /* keep reading til there's no more*/
    //Se supone que read va avanzando, de tal manera que vamos consumiendo los caracteres
    //pulsados hasta que no quedan más, guardando cada uno en buf. Esto nos permite leer varias 
    //pulsaciones a la vez.
    while (res >= 0) {
	//fprintf (fp,"Leído scancode: %x \n",buf[0]);	

	switch (buf[0]) {
	case 0x01:
 	    //fprintf(fp, "ESCAPE scancode detected\n");
            exit_condition = 1;
            return;
	
	case 0x6a:
 	    //fprintf(fp, "Right Arrow held down\n");
            //poner rSign a 1 o a -1 sirve tanto para iniciar el movimiento como para mantenerlo 
	    //si se pasa por aquí de nuevo, o cambiar su dirección cambiando de 1 a -1.
	    //Si usamos aceleración, no es necesario rSign ya que el movimiento no se inicia hasta que
	    //rSpeed tiene un valor distinto de cero.
	    //rSign = 1;
	    rSpeed += 0.40; 
	break;
	
	case 0x69:
 	    //fprintf(fp, "Left Arrow held down\n");
            //rSign = -1;
	    rSpeed -= 0.40; 
	break;
	
	case 0x67:
 	    //fprintf(fp, "Up Arrow held down\n");	
	break;
	
	case 0x6c:
 	    //fprintf(fp, "Down Arrow held down\n");
	break;
	
	case 0x1d:
 	    //fprintf(fp, "Control key held down\n");
	break;
	
	case 0x39:
 	    //fprintf(fp, "Space held down\n");
	break;
	
        //case 0x81:
        //    break;
        // process more scan code possibilities here! 
	}
	res = read(0, &buf[0], 1);
    }
}

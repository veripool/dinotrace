/******************************************************************************
 *
 * Filename:
 *     dt_socket.c
 *
 * Subsystem:
 *     Dinotrace
 *
 * Version:
 *     Dinotrace V7.1
 *
 * Author:
 *     Wilson Snyder
 *
 * Abstract:
 *     Socket Interface
 *
 */
static char rcsid[] = "$Id$";

#include <sys/signal.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>

#include <X11/Xlib.h>
#include <Xm/Xm.h>

#include "dinotrace.h"
#include "callbacks.h"

#define MAXCMDLEN	2000		/* Maximum length of command line */

extern int errno;

/* Structure per client connection for storing command, etc. */
typedef struct st_client {
    char	*cmdptr;		/* Pointer to command char being loaded */
    int		cmdnumber;		/* Serial number of this command, for debugging */
    char	name[MAXHOSTNAMELEN+10];	/* Name of the client */
    char	command[MAXCMDLEN];	/* Command being formed */
    } CLIENT;


/*** MAIN ********************************************************************/

void socket_input_cb (CLIENT *client,
		      int *sock_client_ptr,
		      XtInputId	*id_ptr)
{
    int	sock_client = *sock_client_ptr;
    char c;
    Boolean cont=TRUE;
    int len;

    /*
    static int call = 0;
    if (DTPRINT_SOCKET && call<100) {
	printf ("In socket_input_cb %d.\n", sock_client);
	call++;
	}
	*/

    /* Read till blocking, EOF or carrage-return */
    while (cont) {
	len = read (sock_client, &c, 1);
	if (len==0) {
	    /* End of file */
	    config_read_socket (client->command, client->name, client->cmdnumber, TRUE);
	    cont=FALSE;
	    if (DTPRINT_SOCKET) printf ("End of stream fd %d\n", sock_client);
	    XtRemoveInput (*id_ptr);
	    close (sock_client);
	    DFree (client);
	    return;
	    }
	else if (len < 0) {
	    cont=FALSE;
	    if (errno != EWOULDBLOCK) {
		perror ("socket_input_cb read");
		}
	    }
	else if (len > 0) {
	    if (c=='\n') {
		/* Snarfed a whole command */
		cont=FALSE;
		*(client->cmdptr)++ = '\0';
		if (DTPRINT_SOCKET) printf ("Socket %d Command: '%s'\n", sock_client, client->command);

		/* Execute */
		config_read_socket (client->command, client->name, client->cmdnumber, FALSE);

		/* Prepare for next command */
		client->cmdptr = client->command;
		client->cmdnumber++;
		}
	    else if (c!='\r') {
		*(client->cmdptr)++ = c;
		}
	    }
	}
    }

void socket_accept_cb (XtPointer usr,
		      int *sock_server_ptr,
		      XtInputId	*id)
{
    int sock_client;		/* Specific connection socket number */
    int	clen;			/* Client packet length */
    struct sockaddr_in sa_client;	/* Socket addresses */
    int		sock_server = *sock_server_ptr;
    CLIENT *client;		/* Buffer and other client info */

    if (DTPRINT_SOCKET) printf ("In socket_accept_cb %d.\n", sock_server);

    clen = sizeof (sa_client);
  retry:
    if ( (sock_client=accept (sock_server, &sa_client, &clen)) == -1) {
	if (errno == EINTR)  {
	    goto retry;
	    }
	if (errno == EWOULDBLOCK) {
	    /* False alarm */
	    if (DTPRINT_SOCKET) printf ("No work, returning.\n");
	    return;
	    }
	}

    if (DTPRINT_SOCKET) {
	printf ("Accepted Connection from %s %d, new fd %d.\n",
		inet_ntoa(sa_client.sin_addr.s_addr),
		ntohs (sa_client.sin_port), sock_client);
	}

    /* Don't block on this socket */
    if (fcntl (sock_client, F_SETFL, O_NONBLOCK) < 0) {
	perror ("fcntl");
	}

    /* Create buffer */
    client = XtNew (CLIENT);
    memset (client, 0, sizeof (CLIENT));
    client->cmdptr = client->command;
    sprintf (client->name, "%s %d",
	     inet_ntoa(sa_client.sin_addr.s_addr),
	     ntohs (sa_client.sin_port));

    /* Have X call us when socket needs servicing */
    XtAppAddInput (global->appcontext, sock_client,
		   (XtInputExceptMask | XtInputWriteMask | XtInputReadMask),
		   socket_input_cb, client);
    }

void socket_create ()
    /* Create a socket for this dinotrace program */
{
    int sock_server;		/* Demon socket number */
    int	clen;			/* Client packet length */
    struct sockaddr_in sa_server;	/* Socket addresses */
    struct hostent *he_server_ptr;	/* Server host name */
    char   host_name[MAXHOSTNAMELEN];	/* Name of host */

    if (DTPRINT_ENTRY || DTPRINT_SOCKET) printf ("In socket_create\n");

    /* Exit if created */
    if (global->anno_socket[0]) return;
    strcpy (global->anno_socket, "*UNAVAILABLE*");

    /* Open the socket */
    if ((sock_server = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf (stderr,"Dinotrace: could not get socket.\n");
	perror ("socket");
	exit (1);
	}

    /* Grab port number of this machine */
    gethostname (host_name, MAXHOSTNAMELEN);
    he_server_ptr = gethostbyname (host_name);

    /* Assign to specific port, network visible on this machine */
    bzero ((char *) &sa_server, sizeof(sa_server));
    bcopy (he_server_ptr->h_addr, (char *) &sa_server.sin_addr,
	   he_server_ptr->h_length);
    sa_server.sin_family = AF_INET;
    sa_server.sin_port = INADDR_ANY;

    if (bind (sock_server, (struct sockaddr *) &sa_server, sizeof(sa_server)) == -1) {
        fprintf (stderr, "Dinotrace: could not bind to port.\n");
        perror ("bind");
        exit (1);
	}

    /* Grab port name */
    clen = sizeof (sa_server);
    if (getsockname (sock_server, &sa_server, &clen)) {
        perror ("getsockname");
        exit (1);
	}

    if (DTPRINT_SOCKET) printf ("Listening fd %d on %s %d (%s %d).\n", sock_server,
				host_name,
				ntohs (sa_server.sin_port),
				inet_ntoa(sa_server.sin_addr.s_addr),
				ntohs (sa_server.sin_port)
				);
    sprintf (global->anno_socket, "%s %d", inet_ntoa(sa_server.sin_addr.s_addr),
	     ntohs (sa_server.sin_port));

    /* Tell the OS to que requests to us */
    if (listen (sock_server, SOMAXCONN)) {
        perror ("listen");
        exit (1);
	}

    /* Don't block on this socket */
    if (fcntl (sock_server, F_SETFL, O_NONBLOCK) < 0) {
	perror ("fcntl");
	}

    /* Have X call us when socket needs servicing */
    XtAppAddInput (global->appcontext, sock_server,
		   (XtInputExceptMask | XtInputWriteMask | XtInputReadMask),
		   socket_accept_cb, NULL);
    }



#ident "$Id$"
/******************************************************************************
 * DESCRIPTION: Dinotrace source: socket interface to command engine
 *
 * This file is part of Dinotrace.  
 *
 * Author: Wilson Snyder <wsnyder@wsnyder.org> or <wsnyder@iname.com>
 *
 * Code available from: http://www.veripool.com/dinotrace
 *
 ******************************************************************************
 *
 * Some of the code in this file was originally developed for Digital
 * Semiconductor, a division of Digital Equipment Corporation.  They
 * gratefuly have agreed to share it, and thus the base version has been
 * released to the public with the following provisions:
 *
 * 
 * This software is provided 'AS IS'.
 * 
 * DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THE INFORMATION
 * (INCLUDING ANY SOFTWARE) PROVIDED, INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR ANY PARTICULAR PURPOSE, AND
 * NON-INFRINGEMENT. DIGITAL NEITHER WARRANTS NOR REPRESENTS THAT THE USE
 * OF ANY SOURCE, OR ANY DERIVATIVE WORK THEREOF, WILL BE UNINTERRUPTED OR
 * ERROR FREE.  In no event shall DIGITAL be liable for any damages
 * whatsoever, and in particular DIGITAL shall not be liable for special,
 * indirect, consequential, or incidental damages, or damages for lost
 * profits, loss of revenue, or loss of use, arising out of or related to
 * any use of this software or the information contained in it, whether
 * such damages arise in contract, tort, negligence, under statute, in
 * equity, at law or otherwise. This Software is made available solely for
 * use by end users for information and non-commercial or personal use
 * only.  Any reproduction for sale of this Software is expressly
 * prohibited. Any rights not expressly granted herein are reserved.
 *
 ******************************************************************************
 *
 * Changes made over the basic version are covered by the GNU public licence.
 *
 * Dinotrace is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * Dinotrace is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Dinotrace; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 *****************************************************************************/

#include "dinotrace.h"

#if HAVE_SOCKETS

#include <sys/signal.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/param.h>

#if HAVE_SYS_UN_H
#include <sys/un.h>
#endif

#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>

#include "functions.h"

/**********************************************************************/

#define MAXCMDLEN	2000		/* Maximum length of command line */

extern int errno;

/* Structure per client connection for storing command, etc. */
typedef struct st_client {
    char	*cmdptr;		/* Pointer to command char being loaded */
    int		cmdnumber;		/* Serial number of this command, for debugging */
    char	name[MAXHOSTNAMELEN+10];	/* Name of the client */
    char	command[MAXCMDLEN];	/* Command being formed */
} Client_t;

#if !HAVE_GETHOSTNAME_PROTO
extern int gethostname (char *name, int namelen);
#endif

/*** OS SIGNALS ********************************************************************/

void socket_sig_hup (int sig)
{
    if (DTPRINT_SOCKET) printf ("SIG HUP\n");
    trace_reread_all_cb(NULL,NULL);
}

void socket_set_os_signals ()
{
    signal (SIGUSR1, &socket_sig_hup);	/* Reread All*/
}

/*** MAIN ********************************************************************/

void socket_input_cb (
    Client_t *client,
    int *sock_client_ptr,
    XtInputId	*id_ptr)
{
    int	sock_client = *sock_client_ptr;
    char c;
    Boolean_t cont=TRUE;
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

void socket_accept_cb (
    XtPointer usr,
    int *sock_server_ptr,
    XtInputId	*id)
{
    int sock_client;		/* Specific connection socket number */
    int	clen;			/* Client packet length */
    struct sockaddr_in sa_client;	/* Socket addresses */
    int		sock_server = *sock_server_ptr;
    Client_t *client;		/* Buffer and other client info */

    if (DTPRINT_SOCKET) printf ("In socket_accept_cb %d.\n", sock_server);

    clen = sizeof (sa_client);
  retry:
    if ( (sock_client=accept (sock_server, (struct sockaddr *)&sa_client, &clen)) == -1) {
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
		inet_ntoa(sa_client.sin_addr),
		ntohs (sa_client.sin_port), sock_client);
    }

    /* Don't block on this socket */
    if (fcntl (sock_client, F_SETFL, O_NONBLOCK) < 0) {
	perror ("fcntl");
    }

    /* Create buffer */
    client = DNewCalloc (Client_t);
    client->cmdptr = client->command;
    sprintf (client->name, "%s %d",
	     inet_ntoa(sa_client.sin_addr),
	     ntohs (sa_client.sin_port));

    /* Have X call us when socket needs servicing */
    XtAppAddInput (global->appcontext, sock_client,
		   (XtPointer)(XtInputExceptMask | XtInputWriteMask | XtInputReadMask),
		   (XtInputCallbackProc)socket_input_cb, client);
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
	exit (1L);
    }

    /* Grab port number of this machine */
    gethostname (host_name, MAXHOSTNAMELEN);
    he_server_ptr = gethostbyname (host_name);

    /* Assign to any port, network visible on this machine */
    memset ((char *) &sa_server, 0, sizeof(sa_server));
    memcpy ((char *) &sa_server.sin_addr, he_server_ptr->h_addr, 
	    he_server_ptr->h_length);
    sa_server.sin_family = AF_INET;
    sa_server.sin_port = INADDR_ANY;

    if (bind (sock_server, (struct sockaddr *) &sa_server, sizeof(sa_server)) == -1) {
        fprintf (stderr, "Dinotrace: could not bind to port.\n");
        perror ("bind");
        exit (1L);
    }

    /* Grab port name */
    clen = sizeof (sa_server);
    if (getsockname (sock_server, (struct sockaddr *)&sa_server, &clen)) {
        perror ("getsockname");
        exit (1L);
    }

    if (DTPRINT_SOCKET) printf ("Listening fd %d on %s %d (%s %d).\n", sock_server,
				host_name,
				ntohs (sa_server.sin_port),
				inet_ntoa(sa_server.sin_addr),
				ntohs (sa_server.sin_port)
				);
    sprintf (global->anno_socket, "%s %d", inet_ntoa(sa_server.sin_addr),
	     ntohs (sa_server.sin_port));

    /* Tell the OS to que requests to us */
    if (listen (sock_server, SOMAXCONN)) {
        perror ("listen");
        exit (1L);
    }

    /* Don't block on this socket */
    if (fcntl (sock_server, F_SETFL, O_NONBLOCK) < 0) {
	perror ("fcntl");
    }

    /* Have X call us when socket needs servicing */
    XtAppAddInput (global->appcontext, sock_server,
		   (XtPointer)(/*XtInputExceptMask | XtInputWriteMask | */XtInputReadMask),
		   socket_accept_cb, NULL);
}
#endif /* if HAVE_SOCKETS */

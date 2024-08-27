/************
 * net_tcp.c
 *
 * common code for
 * multi-user networking protocol implementations for TCP/IP
 * (net_bsd_tcp.c and net_sysv_tcp.c)
 *
 */

static in_addr_t bind_local_ip = INADDR_ANY;

const char *
proto_usage_string(void)
{
    return "[+O|-O] [-a ip_address] [[-p] port]";
}


static int
tcp_arguments(struct proto *proto, int argc, char **argv, int *pport)
{
    char *p = 0;

    for ( ; argc > 0; argc--, argv++) {
	if (argc > 0
	    && (argv[0][0] == '-' || argv[0][0] == '+')
	    && argv[0][1] == 'O'
	    && argv[0][2] == 0
	    ) {
#ifdef OUTBOUND_NETWORK
	    proto->can_connect_outbound = (argv[0][0] == '+');
#else
	    if (argv[0][0] == '+') {
		fprintf(stderr, "Outbound network not supported.\n");
		oklog("CMDLINE: *** Ignoring %s (outbound network not supported)\n", argv[0]);
	    }
#endif
	}
	else if (0 == strcmp(argv[0],"-a")) {
            if (argc <= 1)
                return 0;
            argc--;
            argv++;
            bind_local_ip = inet_addr(argv[0]);
            if (bind_local_ip == INADDR_NONE)
                return 0;
	    oklog("CMDLINE: Source address restricted to %s\n", argv[0]);
        }
        else {
            if (p != 0) /* strtoul always sets p */
                return 0;
            if (0 == strcmp(argv[0],"-p")) {
                if (argc <= 1)
                    return 0;
                argc--;
                argv++;
            }
            *pport = strtoul(argv[0], &p, 10);
            if (*p != '\0')
                return 0;
	    oklog("CMDLINE: Initial port = %d\n", *pport);
        }
    }
#ifdef OUTBOUND_NETWORK
    oklog("CMDLINE: Outbound network connections %s.\n",
          proto->can_connect_outbound ? "enabled" : "disabled");
#endif
    return 1;
}


int
proto_initialize(struct proto *proto, Var * desc, int argc, char **argv)
{
    proto->can_connect_outbound =
#ifdef OUTBOUND_NETWORK
	OUTBOUND_NETWORK;
#else
	0;
#endif

    int port = DEFAULT_PORT;

    proto->pocket_size = 1;
    proto->believe_eof = 1;
    proto->eol_out_string = "\r\n";

    if (!tcp_arguments(proto, argc, argv, &port))
	return 0;

    initialize_name_lookup();

    desc->type = TYPE_INT;
    desc->v.num = port;

    return 1;
}


static int
open_connection_arguments(Var arglist,
			  const char **host_name, int *port)
{
    if (arglist.v.list[0].v.num != 2)
	return E_ARGS;
    if (arglist.v.list[1].type != TYPE_STR ||
	     arglist.v.list[2].type != TYPE_INT)
	return E_TYPE;

    *host_name = arglist.v.list[1].v.str;
    *port = arglist.v.list[2].v.num;
    return E_NONE;
}


char rcsid_net_tcp[] = "$Id$";

/*
 * $Log$
 * Revision 1.2  2004/05/22 01:25:44  wrog
 * merging in WROGUE changes (W_SRCIP, W_STARTUP, W_OOB)
 *
 * Revision 1.1.2.2  2003/06/10 00:14:52  wrog
 * fixed printf warning
 *
 * Revision 1.1.2.1  2003/06/01 12:42:30  wrog
 * added cmdline options -a (source address) +O/-O (enable/disable outbound network)
 *
 *
 */

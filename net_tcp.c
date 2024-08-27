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


/* So, historically, only 'name_lookup_timeout' existed and applied to
 * both inbound and outbound connections.  But these are really
 * different situations and need to have separate settings.  Which
 * means one of them has to be a new option, but defaulting to the old
 * one for backwards compatibility.
 *
 * We *could* have made the new one be the one governing outbound
 * connections, but since the setting for inbound connections is the
 * one people are way more likely to want to change (i.e., to set to 0
 * because, now that the year is 2024 and reverse-DNS is almost
 * completely useless), having the default for outbound be the inbound
 * setting would force everyone wanting to turn off inbound lookups to
 * make a *second* setting to restore the outbound default, which
 * would be a huge pain.  So that is why it's the other way around
 * (inbound defaults to outbound).
 *
 * And, yes, the naming is confusing:  "name lookup" can EITHER mean
 * you *have* a name that you are looking up to get information about
 * it, or you have the other information and want to look up the
 * corresponding name.  English is stupid; film at 11.
 *
 * Here we are going with the former interpretation, which also
 * happens to put the new name with the new option, hence,
 * the option is named after the thing you provide:
 *
 *   "name_lookup" is for when you have a name and need an address
 *      (i.e., "ordinary" DNS A query for outbound connections)
 *   "address_lookup" is for when you have an address and want a name
 *      (i.e., "reverse" DNS PTR query for inbound connections)
 *
 * Sorry if this seems backwards to you, but we're in a no-win
 * situation here.                  --wrog (8/27/2024)
 */
static int
address_lookup_timeout(server_listener sl)
{
    int timeout = server_listener_int_option(sl, "address_lookup_timeout", -2);

    if (timeout == -2)
	timeout = server_listener_int_option(sl, "name_lookup_timeout", 5);
    return timeout < 0 ? 0 : timeout;
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

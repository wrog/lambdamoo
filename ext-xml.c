/*
 * XML for the MOO Server using the expat library
 */

#include "bf_register.h"

#if EXPAT_XML_1_2_OR_BEFORE
#  include "xmlparse/xmlparse.h"
#else
#  include <expat.h>
#endif

#include "exceptions.h"
#include "functions.h"
#include "list.h"
#include "server.h"
#include "storage.h"
#include "streams.h"
#include "utils.h"

/*
 * quick'n'dirty
 * <foo a="1">
 *   <bar>11</bar>
 * </foo>
 * =
 * {"foo", {{"a", "1"}}, {{"bar", {}, {"11"}}}}
 */

typedef struct XMLnode XMLnode;
typedef struct XMLclient XMLclient;

/* client data structure */
struct XMLclient {
    enum error error;
    XMLnode *current; /* most-recently/deepest started node      */
    Var kidstack;     /* LIST of pending constructed child nodes */
    size_t ksize;     /* allocated size of kidstack		 */
    Stream *sa;	      /* place to collect attribute strings      */
};

#define KIDSTACK_TOP(client)  ((client)->kidstack.v.list[0].v.num)

/* element in progress */
struct XMLnode {
    XMLnode *parent;
    const char *name;
    Var attribs;
    Stream *text;
    UNum firstkid;	/* index into client->kidstack */
};


#define KSIZE_INITIAL 32

static void
init_client(XMLclient *client)
{
    client->error = E_NONE;
    client->current = NULL;
    client->kidstack = new_list(KSIZE_INITIAL-1);
    client->ksize = KSIZE_INITIAL;
    KIDSTACK_TOP(client) = 0;
    client->sa = new_stream(0);
}

static void
deallocate_node(XMLnode *node)
{
    myfree(node, M_XML_DATA);
}

static void
free_client(XMLclient *client)
{
    XMLnode *node = client->current;
    while (node) {
	free_str(node->name);
	free_var(node->attribs);
	if (node->text)
	    free_stream(node->text);

	XMLnode *parent = node->parent;
	deallocate_node(node);
	node = parent;
    }
    free_stream(client->sa);
    free_var(client->kidstack);
}

/* INVARIANT for all functions called by the handlers
 * (everything from here down to abort_parse()):
 *
 *   At any point where 'stream_too_big' can be raised
 *   there will be no allocations unreachable from client.
 */

/* Ensure that there is room to add one element onto kidstack.
 * Realloc if necessary, but raise stream_too_big
 * if MAX_LIST_CONCAT limit would be exceeded
 */
static void
ensure_kidstack_space(XMLclient *client)
{
    if ((UNum)(KIDSTACK_TOP(client) + 1)
	< client->ksize)
	return;

    size_t kmaxsize = 1 + server_int_option_cached(SVO_MAX_LIST_CONCAT);
    if (client->ksize >= kmaxsize)
	RAISE(stream_too_big, 1);

    if ((client->ksize *= 2) >= kmaxsize)
	client->ksize = kmaxsize;
    client->kidstack.v.list =
	(Var *)myrealloc(client->kidstack.v.list,
			 client->ksize * sizeof(Var), M_LIST);
}

/*
 * Return kidstack[FIRST..KIDSTACK_TOP] as a single list.
 * FIRST > KIDSTACK_TOP (returning empty list) is the only
 * situation where 'stream_too_big' can be raised.
 * On return, kidstack[FIRST == KIDSTACK_TOP] is garbage;
 * caller must either --KIDSTACK_TOP or write something there.
 */
static Var
collect_kids(XMLclient *client, size_t first)
{
    Num nkids = KIDSTACK_TOP(client) + 1 - first;
    Var klist;
    if (nkids) {
	klist = new_list(nkids);
	memcpy(klist.v.list + 1, client->kidstack.v.list + first,
	       nkids * sizeof(Var));
    }
    else {
	ensure_kidstack_space(client);
	klist = new_list(0);
    }
    KIDSTACK_TOP(client) = first;
    return klist;
}

static void
make_text_node(XMLclient *client, Stream *s)
{
    ensure_kidstack_space(client);
    Num i = ++KIDSTACK_TOP(client);
    client->kidstack.v.list[i].type = TYPE_STR;
    client->kidstack.v.list[i].v.str = str_dup(reset_stream(s));
}

static void
nodify_accumulated_text(XMLclient *client)
{
    XMLnode *node = client->current;
    if (node->text && stream_length(node->text))
	make_text_node(client, node->text);
}

static Var
do_attributes(XMLclient *client, const char **atts)
{
    const char **patts;
    Num i;

    UNum first = KIDSTACK_TOP(client) + 1;
    for (patts = atts, i = 1; *patts; ++patts, ++i) {
	stream_add_moobinary_from_raw_bytes(
	    client->sa, *patts, strlen(*patts));
	make_text_node(client, client->sa);
	if (i&1)
	    continue;
	/*
	 * make a pair; trust the parser to only
	 * ever give us an even-length list
	 */
	UNum here = KIDSTACK_TOP(client) - 1;
	client->kidstack.v.list[here] = collect_kids(client, here);
    }

    Var vattrs = collect_kids(client, first);
    --KIDSTACK_TOP(client);
    return vattrs;
}

static void
begin_element(XMLclient *client, const char *name, const char **atts)
{
    Var vattrs = do_attributes(client, atts);

    /* FIXME: Maybe allocate a small array of these up front,
       since they're being used in an entirely stacklike manner.
       */
    XMLnode *node = (XMLnode *)mymalloc(sizeof(XMLnode), M_XML_DATA);

    node->parent = client->current;
    node->name = str_dup(name);
    node->text = NULL;
    node->attribs = vattrs;
    node->firstkid = KIDSTACK_TOP(client) + 1;
    client->current = node;
}

static void
make_element(XMLclient *client)
{
    XMLnode *node = client->current;
    Var kids = collect_kids(client, node->firstkid);
    Var element = new_list(4);
    element.v.list[1].type = TYPE_STR;
    element.v.list[1].v.str = node->name;
    element.v.list[2] = node->attribs;
    element.v.list[3].type = TYPE_STR;
    element.v.list[3].v.str =
	node->text
	? str_dup_then_free_stream(node->text)
	: str_dup("");

    element.v.list[4] = kids;
    client->kidstack.v.list[node->firstkid] = element;
    client->current = node->parent;
    deallocate_node(node);
}

#define ALL_XML_HANDLERS(DEFINE, ARGS)				\
								\
  DEFINE(xml_TstartElement, client, ARGS##START(name, atts),	\
      _STATEMENT({						\
          begin_element(client, name, atts);			\
      }))							\
								\
  DEFINE(xml_DstartElement, client, ARGS##START(name, atts),	\
      _STATEMENT({						\
	  if (client->current)					\
	      nodify_accumulated_text(client);			\
	  begin_element(client, name, atts);			\
      }))							\
								\
  DEFINE(xml_charDataHandler, client, ARGS##CDATA(s, len),	\
      _STATEMENT({						\
	  XMLnode *node = client->current;			\
	  if (!node->text)					\
	      node->text = new_stream(0);			\
	  stream_add_moobinary_from_raw_bytes(			\
	      node->text, s, len);				\
      }))							\
								\
  DEFINE(xml_TendElement, client, ARGS##END(name),		\
      _STATEMENT({						\
	  make_element(client);					\
      }))							\
								\
  DEFINE(xml_DendElement, client, ARGS##END(name),		\
      _STATEMENT({						\
	  nodify_accumulated_text(client);			\
	  make_element(client);					\
      }))							\


static void
abort_parse(XML_Parser parser)
{
    /* sadly, with expat, the following is not *completely* sufficient
     * to prevent handlers from continuing to be called, so we still
     * need to check ->error in handlers, below.
     */
    XML_SetCharacterDataHandler(parser, NULL);
    XML_SetElementHandler(parser, NULL, NULL);

    XMLclient *client = XML_GetUserData(parser);
    client->error = E_QUOTA;

#if EXPAT_XML_1_2_OR_BEFORE
    /*
     *   parser runs through the entire rest of the document, we drop
     *   it on the floor, and have to check client->error even on a
     *   successful return; bleah.
     */
#else
    XML_StopParser(parser, 0);
#endif
}

/* wrap all handlers, since longjmp()ing through
 * the parser is probably a bad idea.
 */
#define DEF_XMLHANDLER(fn, clt, REST__, BODY)	\
static void					\
fn(void *arg1, REST__)				\
{						\
    XML_Parser parser = (XML_Parser)arg1;	\
    XMLclient *clt = XML_GetUserData(parser);	\
    if (clt->error != E_NONE)			\
	return;					\
						\
    TRY_STREAM					\
	BODY;					\
    EXCEPT_STREAM				\
	abort_parse(parser);			\
    ENDTRY_STREAM				\
}						\


#define XMLARGS_START(n,a)  const char *n, const char **a
#define XMLARGS_END(n)      const char *n UNUSED_
#define XMLARGS_CDATA(s,l)  const XML_Char *s, int l

ALL_XML_HANDLERS(DEF_XMLHANDLER, XMLARGS_)


/**
 * Parse an XML string into a nested list.
 * The second parameter indicates whether
 * (true) text content shows up as child text nodes [4] with all
 * content being kept in order or
 * (false) text content is concatenated into a single string [3]
 * even if there were originally intervening child elements.
 */
static package
parse_xml(const char *string, int keep_order)
{
    /*
     * FIXME: Feed expat smaller chunks of the string and
     * check for task timeout between chunks, or maybe
     * think about doing a suspendable version?
     */
    package result;
    XML_Parser parser = XML_ParserCreate("utf-8");
    XMLclient client_struct;

    init_client(&client_struct);

    XML_SetUserData(parser, &client_struct);
    XML_UseParserAsHandlerArg(parser);
    XML_SetCharacterDataHandler(parser, xml_charDataHandler);
    if (keep_order)
	XML_SetElementHandler(parser,
			      xml_DstartElement, xml_DendElement);
    else
	XML_SetElementHandler(parser,
			      xml_TstartElement, xml_TendElement);

    if (XML_Parse(parser, string, memo_strlen(string), 1)
#if EXPAT_XML_1_2_OR_BEFORE
	&& client_struct.error == E_NONE
#endif
	) {
	if (KIDSTACK_TOP(&client_struct)-- != 1)
	    panic("XML_Parse produced the wrong number of nodes");
	result = make_var_pack(client_struct.kidstack.v.list[1]);
    }
    else if (client_struct.error == E_QUOTA)
	result = make_space_pack();
    else {
	Var r;
	r.type = TYPE_INT;
	r.v.num =
#if EXPAT_XML_INT64
	    XML_GetCurrentByteIndex64
#else
	    XML_GetCurrentByteIndex
#endif
	    (parser);
	result = make_raise_pack(E_INVARG,
				 XML_ErrorString(XML_GetErrorCode(parser)),
				 r);
    }

    free_client(&client_struct);
    XML_ParserFree(parser);
    return result;
}


static package
bf_parse_xml_document(Var arglist, Byte next UNUSED_, void *vdata UNUSED_, Objid progr UNUSED_)
{
    package result = parse_xml(arglist.v.list[1].v.str, 1);
    free_var(arglist);
    return result;
}

static package
bf_parse_xml_tree(Var arglist, Byte next UNUSED_, void *vdata UNUSED_, Objid progr UNUSED_)
{
    package result = parse_xml(arglist.v.list[1].v.str, 0);
    free_var(arglist);
    return result;
}

void
register_xml(void)
{
    register_function("xml_parse_tree", 1, 1, bf_parse_xml_tree, TYPE_STR);
    register_function("xml_parse_document", 1, 1, bf_parse_xml_document, TYPE_STR);
}

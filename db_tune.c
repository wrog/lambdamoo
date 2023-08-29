/* This is just a stub for now; various db-layer-specific tuning functions
 * will eventually live here.
 */


#include "db_tune.h"

#include "bf_register.h"
#include "functions.h"
#include "utils.h"

#ifdef VERB_CACHE

static package
bf_verb_cache_stats(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;

    free_var(arglist);

    if (!is_wizard(progr)) {
	return make_error_pack(E_PERM);
    }
    r = db_verb_cache_stats();

    return make_var_pack(r);
}

static package
bf_log_cache_stats(Var arglist, Byte next, void *vdata, Objid progr)
{
    free_var(arglist);

    if (!is_wizard(progr)) {
	return make_error_pack(E_PERM);
    }
    db_log_cache_stats();

    return no_var_pack();
}

#endif /* VERB_CACHE */

void
register_db_tune(void)
{
#ifdef VERB_CACHE
    register_function("log_cache_stats", 0, 0, bf_log_cache_stats);
    register_function("verb_cache_stats", 0, 0, bf_verb_cache_stats);
#endif /* VERB_CACHE */
}

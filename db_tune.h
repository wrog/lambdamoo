/* This is just a stub for now; various db-layer-specific tuning functions
 * will eventually live here.
 */

#include "db_private.h"

#ifdef VERB_CACHE

extern void db_log_cache_stats(void);
extern Var db_verb_cache_stats(void);

#endif /* VERB_CACHE */

extern void register_db_tune(void);

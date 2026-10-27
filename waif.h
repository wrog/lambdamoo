/* Copyright (c) 1998-2002 Ben Jackson (ben@ben.com).  All rights reserved.
 *
 * Use and copying of this software and preparation of derivative works based
 * upon this software are permitted provided this copyright notice remains
 * intact.
 *
 * THIS SOFTWARE IS PROVIDED BY BEN J JACKSON ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT BEN J JACKSON BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef Waif_H
#define Waif_H

#include "config.h"
#include "options.h"

/* Stubs available regardless of WAIF support: */

/* for call_verb2() */
#ifdef WAIF_CORE
#  define WAIF_COMMA_ARG(arg) ,arg
#else
#  define WAIF_COMMA_ARG(arg)
#endif

/* Begin actual WAIF support: */

#ifdef WAIF_CORE

#include "db_private.h"
#include "structures.h"


#define WAIF_PROP_PREFIX	':'
#define WAIF_VERB_PREFIX	':'

#ifdef WAIF_DICT
#  define  WAIF_INDEX_VERB	":_index"
#  define  WAIF_INDEXSET_VERB	":_set_index"
#endif

#if __STDC_VERSION__ >= 202311L     /* allow repeated typedef */
typedef struct Waif         Waif;   /* from structures.h */
#endif
typedef struct WaifPropdefs WaifPropdefs;


/*--------------*
 |  WAIF_MAPSZ  |
 *--------------*/

/* (original Ben comment from 1998-2002:)
 * Try to make struct Waif fit into 32 bytes with this mapsz.  These
 * bytes are probably "free" (from a powers-of-two allocator) and we
 * can use them to save lots of space.  With 64bit addresses I think
 * the right value is 8.  If checkpoints are unforked, save space for
 * an index used while saving.  Otherwise we can alias propdefs and
 * clobber it in the child.
 *
 * (wrog continues in 2024:)
 * Assumptions needed to make this comment work:
 *    (1) size here includes the hidden 4 byte (at-the-time) refcount,
 *    (2) the original 'unsigned long' type for pmap[] elements was
 *        intended to be 32 bits (not *entirely* certain since 'long'
 *        *is* a way to get 64 bit ints on some platforms),
 *        but the hardwired 32 in count_set_bits() gives the game away.
 *    (3) the target size in 64-bit land is 64 bytes
 *
 * at which point we get
 *            (4)+4+4+4+4+4*(WAIF_MAPSZ)==32 -> WAIF_MAPSZ=3
 * along with
 *            (4)+4+4+8+8+4*(WAIF_MAPSZ)==64 -> WAIF_MAPSZ=8
 * for the 64-bit-pointers/32-bit-everything-else world
 * which then makes the above comment work...
 *
 * ... but 2024 is a yet different world with likely-64-bit Objids/ints
 * and also the 'int' refcount *might* be 64 bits as well,

 * ... though even if it isn't, the two pointers preceding pmap likely
 * need to be aligned on 8-byte boundaries, at which point a 4-byte
 * refcount will then produce 4 bytes of padding, so we may as well
 * assume an 8-byte refcount in all cases, which then means
 *
 *            (8)+4+4+8+8+4*(WAIF_MAPSZ)==64 -> WAIF_MAPSZ=8
 *
 * for the INT_TYPE_BITSIZE=32 case (... 32 byte Waifs now being
 * impossible because there is no way to get 4-byte pointers on a
 * 64-bit architecture without invoking stupid memory models and I
 * imagine we *really* don't want to go there..) --- with the same
 * story for the INT_TYPE_BITSIZE=16 case, since even if that might
 * theoretically yield 4 more bytes of space from the Objid,
 * the padding daemons will probably take it away.
 *
 * Meanwhile we have
 *
 *            (8)+8+8+8+8+4*(WAIF_MAPSZ)==64 -> WAIF_MAPSZ=6
 *
 * for the INT_TYPE_BITSIZE=64 case.  6 (=6*32=192 property values) is
 * likely to be plenty anyway, so we *could* just make it 6 always.
 * ...  but I like the idea of preserving the !UNFORKED_CHECKPOINTS
 * code (i.e., having .save_index steal space from .pmap[]) just in
 * case space again becomes tight in the future for some reasons
 * (e.g., Waifs getting more slots, or Apocalyptic Wilderness...)
 *
 * However, I put in the struct/union .u to make the memory
 * overlap more obvious and *not* constrain us to a particular
 * type for .save_index should we want to widen it some day)
 */

#if YES_THERE_WILL_BE_BILLIONS_OF_WAIFS
  /* figure other things will break if this were so */
typedef intmax_t waif_count_type;
#  define        PRIuWCT "jd"
#  define        SCNuWCT "jd"
  /* c preprocessor cannot do sizeof();
     pretend intmax_t=int64_t for now */
#  define        WAIF_MAPSZ_CTN 2

#else  /* objective reality */
typedef unsigned waif_count_type;
#  define        PRIuWCT "u"
#  define        SCNuWCT "u"
#  define        WAIF_MAPSZ_CTN 1
#endif

#if USE_ORIGINAL_WAIF_MAPSZ
#  define  WAIF_MAPSZ_ITBN  3

#elif  INT_TYPE_BITSIZE == 64
#  define  WAIF_MAPSZ_ITBN  6

#else  /* INT_TYPE_BITSIZE in (32,16) */
#  define  WAIF_MAPSZ_ITBN  8
#endif

#if UNFORKED_CHECKPOINTS
#  define WAIF_MAPSZ  ((WAIF_MAPSZ_ITBN) - (WAIF_MAPSZ_CTN))
#else
#  define WAIF_MAPSZ  (WAIF_MAPSZ_ITBN)
#endif


/*---------------*
 |  struct Waif  |
 *---------------*/

/*................................
  :  begin DEC ALPHA experiment  :
  :  (see structures.h comment)  :
 */
#ifdef SHORT_ALPHA_VAR_POINTERS
#pragma pointer_size save
#pragma pointer_size short
#endif

struct Waif {
    Objid class;
    Objid owner;
    WaifPropdefs *propdefs;
    Var          *propvals;

#ifdef UNFORKED_CHECKPOINTS
    struct
#else
    union
#endif
    {
	uint32_t pmap[WAIF_MAPSZ];
	waif_count_type save_index;
    } u;
};
#define BQM_DESCRIBE_Waif(B,F,V,X)     (7 * V)


#ifdef SHORT_ALPHA_VAR_POINTERS
#pragma pointer_size restore
#endif
/*
 :  end of DEC ALPHA experiment  :
 :...............................:*/


/*-----------------------*
 |  struct WaifPropdefs  |
 *-----------------------*/

struct WaifPropdefs {
    int		    refcount;
    int		    length;
    struct Propdef  defs[];
};


extern void free_waif(Waif *);
extern Waif *dup_waif(Waif *);
extern enum error waif_get_prop(Waif *, const char *, Var *, Objid progr);
extern enum error waif_put_prop(Waif *, const char *, Var, Objid progr);
extern  int waif_bytes(Waif *);
extern void dbio_write_waif(Var);
extern  int dbio_read_waif(Var *);
extern void free_waif_propdefs(WaifPropdefs *);
extern void waif_rename_propdef(Object *, const char *, const char *);

#endif		/* WAIF_CORE */

#endif		/* Waif_H */

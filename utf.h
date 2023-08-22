/*
 * utf.h
 *
 * Prototypes for UTF-8 handling
 */

#ifndef UTF_h
#define UTF_h 1

#define INVALID_RUNE	0xfffd

inline int is_utf8_cont_byte(uint8_t c)
{ return (c & 0xc0) == 0x80; }

extern uint32_t get_utf(const char **);
extern int put_utf(char **, uint32_t);      /* -> true if failed */
extern size_t skip_utf(const char *, int);
extern size_t strlen_utf(const char *);
extern size_t clearance_utf(const uint8_t);

/* Use
 *    get_byte, state1
 *      where get_byte(state1) returns either a next byte or EOF,
 *      updating state1 accordingly, and
 *    state2
 *      which is additional state needed, to be initialized to -1
 *      before the first call to get_utf_call
 * as a stream from which this function will retrieve (and
 * return) successive characters, and then EOF after the end is
 * reached.
 */
extern int32_t get_utf_call(int32_t (*get_byte)(void *), void *state1, int32_t *state2);

#endif		/* !UTF_h */

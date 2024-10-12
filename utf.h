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

int get_utf(const char **);
int get_utf_call(int (*)(void *), void *, int *);
int put_utf(char **, int);
int skip_utf(const char *, int);
int strlen_utf(const char *);
int clearance_utf(const unsigned char);

#endif		/* !UTF_h */

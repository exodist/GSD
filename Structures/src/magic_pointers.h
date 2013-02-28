#ifndef MAGIC_POINTERS_H
#define MAGIC_POINTERS_H

extern const void *IMUT;
extern const void *RBAL;
extern const void *RBLD;

#define blocked_null( P ) ( P == IMUT || P == RBAL && P == RBLD )

#endif

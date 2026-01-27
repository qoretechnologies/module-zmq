/*  Stub zargs.h for macports czmq without draft API headers installed
    The zargs_t type is already forward-declared in czmq_library.h
*/

#ifndef __ZARGS_H_INCLUDED__
#define __ZARGS_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

// zargs_t is already typedef'd in czmq_library.h as:
// typedef struct _zargs_t zargs_t;

// Stub function declarations (not implemented - will link error if actually used)
CZMQ_EXPORT zargs_t *zargs_new(int argc, char **argv);
CZMQ_EXPORT void zargs_destroy(zargs_t **self_p);
CZMQ_EXPORT const char *zargs_progname(zargs_t *self);
CZMQ_EXPORT size_t zargs_arguments(zargs_t *self);
CZMQ_EXPORT const char *zargs_first(zargs_t *self);
CZMQ_EXPORT const char *zargs_next(zargs_t *self);
CZMQ_EXPORT const char *zargs_param_first(zargs_t *self);
CZMQ_EXPORT const char *zargs_param_next(zargs_t *self);
CZMQ_EXPORT const char *zargs_param_name(zargs_t *self);
CZMQ_EXPORT const char *zargs_get(zargs_t *self, const char *name);
CZMQ_EXPORT const char *zargs_getx(zargs_t *self, const char *name, ...);
CZMQ_EXPORT bool zargs_has(zargs_t *self, const char *name);
CZMQ_EXPORT bool zargs_hasx(zargs_t *self, const char *name, ...);
CZMQ_EXPORT void zargs_print(zargs_t *self);

#ifdef __cplusplus
}
#endif

#endif

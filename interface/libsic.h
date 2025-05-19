/**
 *
 * SimpleContainer library interface.
 *
 * Copyright Vito Tomas, 2025.
 *
 */

#ifndef _LIBSIC_
#define _LIBSIC_

#include <stdint.h>
#include <internal/libsic_inter.h>

/**
 * Return codes.
 *
 * RET_SUCC          Successful execution.
 * RET_INTER         Internal error.
 * RET_PARAM         Incorrect function parameters.
 * RET_ABN           Abnormal controller instance behaviour.
 * ...
 * RET_NOSUPP        Operation not supported.
 *
 */
#define RET_SUCC     0
#define RET_INTER    1
#define RET_PARAM    2
#define RET_ABN      3
#define RET_NOSUPP   9

/**
 * Config flags.
 *
 */
#define LS_DIRS_CREAT (1 << 0)
#define LS_DAEMONIZE  (1 << 1)

struct LIBSIC_CONTAINER_CONFIG {
 uint8_t flags;
 int32_t namespaces;
 char container_path[PATH_LEN_MAX];
 char rootfs_path[PATH_LEN_MAX];
 char tar_archive[PATH_LEN_MAX];
 char unix_socket[PATH_LEN_MAX];
};

typedef struct LIBSIC_CONTAINER_CONFIG libsic_cconf_t;

/**
 *
 * Initialize the container workspace with parameters.
 *
 *
 */
int libsic_init_container(libsic_cconf_t conf);

/**
 *
 * Execute a program inside an existing container.
 *
 *
 */
void libsic_execute(const libsic_cconf_t *conf,
                    const char *path,
                    const char *arg0);

/**
 *
 * Destroy an existing container controller instance(s).
 *
 *
 */
int libsic_destroy_container(const libsic_cconf_t *conf);

#endif

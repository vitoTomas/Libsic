/**
 *
 * SimpleContainer library interface.
 *
 * Copyright Vito Tomas, 2025.
 *
 */

#ifndef _LIBSIC_
#define _LIBSIC_

/**
 *
 * Initialize the container workspace with parameters.
 *
 *
 */
int init_container();

/**
 *
 * Run a process inside an existing container.
 *
 *
 */
int run_process();

/**
 *
 * Destroy an existing container workspace.
 *
 *
 */
int destroy_container();

#endif

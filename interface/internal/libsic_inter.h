/**
 *
 * SimpleContainer librarby internal interface.
 * NOT for programming reference.
 *
 * Copyright Vito Tomas, 2025.
 *
 */

#ifndef _INTERNAL_LIBSIC_INTER_
#define _INTERNAL_LIBSIC_INTER_

#include <stdint.h>
#include <sys/types.h>

 /**
  * Maxs and mins.
  *
  */
#define PATH_LEN_MAX    512
#define CMD_EXP_MAX     256
#define STR_COUNT_MAX   32

enum COM_TYPE {
  REQ_PID,
  REQ_STATUS,
  REQ_KILL,
  RES_PID,
  RES_STATUS,
  RES_KILL,
  RES_UNKNOWN
};

struct COM {
 enum COM_TYPE type;
 uint8_t data[256];
};

struct CONT_DATA {
 pid_t cont_inter_pid;
};

#define TAR_EXP        "tar -xf %s -C %s"

#endif

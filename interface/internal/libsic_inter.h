/**
 *
 * SimpleContainer librarby internal interface.
 * NOT for programming reference.
 *
 * Copyright 2025 Vito Tomas
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the “Software”), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
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
#define PATH_LEN_MAX 512
#define CMD_EXP_MAX 256
#define STR_COUNT_MAX 32

enum COM_TYPE {
  /**
   * Requests
   *
   */
  REQ_PID,
  REQ_STATUS,
  REQ_KILL,
  REQ_ALIVE,
  /**
   * Responses
   *
   */
  RES_PID,
  RES_STATUS,
  RES_KILL,
  RES_ALIVE,
  RES_UNKNOWN
};

struct COM {
  enum COM_TYPE type;
  uint8_t data[256];
};

struct CONT_DATA {
  pid_t cont_inter_pid;
};

#define TAR_EXP "tar -xf %s -C %s"

#endif

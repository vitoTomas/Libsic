/*
 * Simple container runtime library implementation. 
 *
 *
 *
 * Copyright Vito Tomas, 2025
 *
 */

#define _GNU_SOURCE

#include <sched.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <poll.h>

#include <stdio.h>

#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>

#define CONTAINER_DIR  "/run/scontainer"
#define ROOTFS_DIR     "/run/scontainer/rootfs"

/* Return codes */
#define RET_SUCC       0
#define RET_INTER      1 /* Internal error */
#define RET_PARAM      2 /* Incorrect parameters */
#define RET_ABN        3 /* Abnormal container behaviour */

/* Maximums and minimums */
#define PATH_LEN_MAX   512
#define STR_COUNT_MAX  32

/* Config flags */
#define LS_DIRS_CREAT  (1 << 0)
#define LS_DAEMONIZE   (1 << 1)

/* Internal messsage */
#define IM_TYPE_REQ    (1 << 0)
#define IM_TYPE_RES    (1 << 1)

#define IM_EXECUTE     (1 << 0)
#define IM_GET_STAT    (1 << 1)
#define IM_DIE         (1 << 2)

#define IM_PAYLOAD_S   512

struct container_config {
  int flags;
  int namespaces;
  char container_dir[PATH_LEN_MAX];
  char rootfs_dir[PATH_LEN_MAX];
  char entrypoint[PATH_LEN_MAX];
  char *argv[STR_COUNT_MAX];
  char *environ[STR_COUNT_MAX];
  char unix_socket[PATH_LEN_MAX];
};

struct intermessg {
  int type;
  int flags;
  char payload[IM_PAYLOAD_S];
};

static int check_dirs(const char *path)
{
  struct stat buffer;
  int ret;

  if (!path) {
    return RET_PARAM;
  }

  if (strnlen(path, PATH_LEN_MAX) >= PATH_LEN_MAX) {
    return RET_PARAM;
  }

  ret = stat(path, &buffer);
  if (ret) {
    RET_INTER;
  }
  
  return RET_SUCC; 
}

static int create_dir(const char *path)
{
  int ret;
  ret = check_dirs(path);
  if (ret) {
    syslog(LOG_ERR, "Incorrect pathname parameter");
    return ret;
  }

  ret = mkdir(path, S_IRUSR | S_IWUSR);
  if (ret) {
    syslog(LOG_ERR, "Failed at creating container directory, [%d] : %s.",
           errno, strerror(errno));
    return RET_INTER;
  }
  
  return RET_SUCC;
}

static int ctrl_inter_msg(struct intermessg *req)
{
  
  return RET_SUCC;
}

static int controller_internal(struct container_config conf, int sock_inter)
{
  FILE *fp;
  pid_t pid;
  struct pollfd pfd;
  struct intermessg req;
  struct intermessg res;
  int ret;
  /* TODO: image creation and mounting */
  
  ret = chroot(conf.rootfs_dir);
  if (ret) {
    ret = RET_INTER;
    goto inter_exit;
  }
  
  ret = chdir("/");
  if (ret) {
    ret = RET_INTER;
    goto inter_exit;
  }
  
  fp = fopen("test.txt", "a");
  if (!fp) {
    return RET_INTER; 
  }
  fprintf(fp, "Hello from container init process!");
  fclose(fp);
  close(sock_inter);

  ret = RET_SUCC;
  
  if (conf.flags & LS_DAEMONIZE) {
    pfd.fd = sock_inter;
    pfd.events = POLLIN;

    while (1) {
      poll(&pfd, 1, -1);
      read(sock_inter, &req, sizeof(req));
      ctrl_inter_msg(&req);
    }
  }

  close(sock_inter);
  
inter_exit:
  exit(ret);
}

static int controller_external(int flags, int sock_inter)
{
  int status, ret;
  pid_t pid_inter;

  /* Send a message to the container */
  close(sock_inter);

  pid_inter = wait(&status);
  if (WIFEXITED(status)) {
    syslog(LOG_INFO, "[%d] Internal controller [%d] exited with code %d.",
           getpid(), pid_inter, WEXITSTATUS(status));
    ret = WEXITSTATUS(status);
  } else {
    syslog(LOG_WARNING, "[%d] Internal controller [%d] exited abnormally.",
           getpid(), pid_inter);
    ret = RET_ABN;
  }
  
  exit(ret);
}

int init_container()
{
  struct container_config conf;
  pid_t pid;
  int sv[2], ret;

  /* TODO: make settable from interface */
  conf.flags = 0;
  // conf.flags |= LS_DAEMONIZE;
  conf.namespaces = CLONE_NEWNS | CLONE_NEWNET | CLONE_NEWIPC | CLONE_NEWPID;
  strncpy(conf.container_dir, CONTAINER_DIR, PATH_LEN_MAX);
  strncpy(conf.rootfs_dir, ROOTFS_DIR, PATH_LEN_MAX);
  /* ======================================================================= */

  /* Create required directories if they don't exist */
  ret = create_dir(conf.container_dir);
  if (ret == RET_PARAM) return ret;
  ret = create_dir(conf.rootfs_dir);
  if (ret == RET_PARAM) return ret;
  

  pid = fork();
  if (pid > 0) {
    /* Calling parent, return to the caller */
    return RET_SUCC;
  } else if (pid == 0) {
    setsid();

    freopen("/dev/null", "w", stdout);
    freopen("/dev/null", "w", stderr);

    ret = unshare(conf.namespaces);
    if (ret) {
        syslog(LOG_ERR, "Failed to create isolated namespaces, [%d] : %s.",
               errno, strerror(errno));
        return RET_INTER;
    }

    ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    if (ret) {
      syslog(LOG_ERR, "Cannot create a socket pair, [%d] : %s.", errno,
             strerror(errno));
      return RET_INTER;
    }
    
    pid = fork();
    if (pid > 0) {
      
      /* External controller */
      syslog(LOG_INFO, "Detached external controller");
      syslog(LOG_INFO, "Detached internal controller");
      controller_external(conf.flags, sv[0]);
      return RET_SUCC;
      
    } else if (pid == 0) {
      
      /* Internal controller */
      controller_internal(conf, sv[1]);
      return RET_SUCC;
      
    } else {
      syslog(LOG_ERR, "Detachment error, [%d] : %s.", errno, strerror(errno));
      return RET_INTER;
    }
    
  } else {
    
    syslog(LOG_ERR, "Cannot fork, [%d] : %s.", errno, strerror(errno));
    return RET_INTER;
    
  }

  return RET_SUCC;
}

/* Run a process inside the container */
int run_process() {

  /* TODO: send message via UNIX socket*/
  return RET_SUCC;
}

/* Shut down currently running container handler */
int destroy_container()
{

  /* TODO: send message via UNIX socket */
  return RET_SUCC;
}

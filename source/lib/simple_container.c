/**
 *
 * SimpleContainer library implementation.
 *
 * Copyright Vito Tomas, 2025.
 *
 */

#define _GNU_SOURCE

#include <libsic.h>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/capability.h>
#include <poll.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <syslog.h>
#include <unistd.h>

#define DEBUG

static int check_dirs(const char *path) {
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
    return RET_INTER;
  }

  return RET_SUCC;
}

static int create_dir(const char *path) {
  int ret;

  ret = check_dirs(path);
  /* if (ret) {
    syslog(LOG_ERR, "Incorrect pathname parameter.");
    return ret;
  } */

  ret = mkdir(path, S_IRWXU);
  if (ret) {
    syslog(LOG_ERR, "Failed at creating container directory, [%d] : %s.", errno,
           strerror(errno));
    return RET_INTER;
  }

  return RET_SUCC;
}

static int controller_internal(libsic_cconf_t conf, int inter_fd) {
  struct COM req;
  ssize_t data_count;
  int ret;

  ret = chroot(conf.rootfs_path);
  if (ret) {
    ret = RET_INTER;
    goto exit_inter_cont;
  }

  ret = chdir("/");
  if (ret) {
    ret = RET_INTER;
    goto exit_inter_cont;
  }

  ret = RET_SUCC;

  if (conf.flags & LS_DAEMONIZE) {
    while (1) {
      data_count = read(inter_fd, &req, sizeof(req));
      if (data_count < 0) {
        ret = RET_INTER;
        break;
      }

      switch (req.type) {
      case REQ_KILL:
        exit(RET_SUCC);
      default:;
        /* Unknown, do nothing. */
      }
    }
  }

  close(inter_fd);

exit_inter_cont:
  exit(ret);
}

static int controller_listener_setup(const char *unix_socket) {
  int server_fd, ret;
  struct sockaddr_un server_addr;

  server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (server_fd == -1) {
    syslog(LOG_ERR, "Could not create socket, [%d] : %s.", errno,
           strerror(errno));
    return -1;
  }

  memset(&server_addr, 0, sizeof(struct sockaddr_un));
  server_addr.sun_family = AF_UNIX;
  strncpy(server_addr.sun_path, unix_socket, sizeof(server_addr.sun_path) - 1);

  unlink(unix_socket);

  ret = bind(server_fd, (struct sockaddr *)&server_addr,
             sizeof(struct sockaddr_un));
  if (ret == -1) {
    syslog(LOG_ERR, "Could not bind, [%d] : %s.", errno, strerror(errno));
    close(server_fd);
    return -1;
  }

  ret = listen(server_fd, 1);
  if (ret == -1) {
    syslog(LOG_ERR, "Could not listner, [%d] : %s.", errno, strerror(errno));
    close(server_fd);
    return -1;
  }

  syslog(LOG_INFO, "External controller listening on %s.", unix_socket);
  return server_fd;
}

static int controller_listen(int server_fd, int inter_fd,
                             struct CONT_DATA *data) {
  struct pollfd pfd;
  struct COM req, res;
  struct sockaddr_un client_addr;
  socklen_t client_len = sizeof(struct sockaddr_un);
  ssize_t data_count;
  int client_fd, ret;

  pfd.fd = server_fd;
  pfd.events = POLLIN;

  while (1) {
    memset(&req, 0, sizeof(req));
    memset(&res, 0, sizeof(res));

    ret = poll(&pfd, 1, -1);
    if (ret == -1) {
      syslog(LOG_ERR, "Controller polling failed, [%d] : %s.", errno,
             strerror(errno));
      return RET_INTER;
    }

    if (pfd.revents & POLLIN) {
      client_fd =
          accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
      if (client_fd == -1) {
        syslog(LOG_ERR,
               "Client failed to connect to the external controller, "
               "[%d] : %s.",
               errno, strerror(errno));
        continue;
      }

      recv(client_fd, &req, sizeof(req), 0);

      switch (req.type) {
      case REQ_PID:
        res.type = RES_PID;
        *((pid_t *)res.data) = data->cont_inter_pid;
        syslog(LOG_INFO, "Received PID request.");
        break;
      case REQ_KILL:
        syslog(LOG_INFO, "Received kill request.");
        res.type = RES_KILL;

        /* Kill the internal controller. */
        data_count = write(inter_fd, &req, sizeof(req));
        if (data_count != sizeof(req))
          syslog(LOG_ERR, "Internal instance messaging error.");
        break;
      default:
        syslog(LOG_INFO, "Received unknown request.");
        res.type = RES_UNKNOWN;
      }

      send(client_fd, &res, sizeof(res), 0);

      if (req.type == REQ_KILL)
        break;
    }
  }

  return RET_SUCC;
}

static void debug_config(libsic_cconf_t *conf) {
  syslog(LOG_DEBUG, "Container path: %s.", conf->container_path);
  syslog(LOG_DEBUG, "Rootfs path: %s.", conf->rootfs_path);
  syslog(LOG_DEBUG, "Tar archive path: %s.", conf->tar_archive);
  syslog(LOG_DEBUG, "Unix socket: %s.", conf->unix_socket);
}

static int controller_external(libsic_cconf_t conf, int inter_fd, pid_t pid) {
  struct CONT_DATA data;
  int status, server_fd, ret;
  pid_t pid_inter;

#ifdef DEBUG
  debug_config(&conf);
#endif

  data.cont_inter_pid = pid;

  server_fd = controller_listener_setup(conf.unix_socket);
  if (server_fd != -1)
    controller_listen(server_fd, inter_fd, &data);

  close(inter_fd);
  close(server_fd);

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

static int load_container(const char *tar_path, const char *rootfs_path) {
  char command[CMD_EXP_MAX];
  int ret;

  /* Untar the desired rootfs contents to the rootfs path. */
  snprintf(command, CMD_EXP_MAX, TAR_EXP, tar_path, rootfs_path);
  ret = system(command);
  if (ret) {
    syslog(LOG_ERR, "Could not untar rootfs archive, [%d] : %s.", errno,
           strerror(errno));
    return RET_INTER;
  }

  return RET_SUCC;
}

/* Initialize a container instance. */
int libsic_init_container(libsic_cconf_t conf) {
  struct __user_cap_header_struct cap_hdr;
  struct __user_cap_data_struct cap_data[2];
  pid_t pid;
  int sv[2], ret;

  /* Create required directories if they don't exist. */
  ret = create_dir(conf.container_path);
  if (ret)
    return ret;

  ret = create_dir(conf.rootfs_path);
  if (ret)
    return ret;

  ret = load_container(conf.tar_archive, conf.rootfs_path);
  if (ret)
    return ret;

  memset(&cap_hdr, 0, sizeof(cap_hdr));
  memset(cap_data, 0, sizeof(cap_data));

  cap_hdr.version = _LINUX_CAPABILITY_VERSION_3;
  cap_hdr.pid = 0;

  pid = fork();
  if (pid > 0) {
    /* Calling parent, return to the caller. */
    return RET_SUCC;
  } else if (pid == 0) {
    setsid();

    /* Current user needs to be an administrator. */
    /* Capabilities will be cloned to the new user namespace (if applicable). */
    ret = syscall(SYS_capget, &cap_hdr, cap_data);
    if (ret) {
      syslog(LOG_ERR, "Failed to get user capabilities, [%d] : %s.\n", errno,
             strerror(errno));
      exit(RET_INTER);
    }

    ret = unshare(conf.namespaces);
    if (ret) {
      syslog(LOG_ERR, "Failed to create isolated namespaces, [%d] : %s.", errno,
             strerror(errno));
      exit(RET_INTER);
    }

    ret = socketpair(AF_UNIX, SOCK_DGRAM, 0, sv);
    if (ret) {
      syslog(LOG_ERR, "Cannot create a socket pair, [%d] : %s.", errno,
             strerror(errno));
      exit(RET_INTER);
    }

    pid = fork();
    if (pid > 0) {

      /* External controller. */
      syslog(LOG_INFO, "Detached external and internal controllers.");
      close(sv[1]);
      controller_external(conf, sv[0], pid);

    } else if (pid == 0) {

      /* Internal controller. */
      close(sv[0]);

      ret = syscall(SYS_capset, &cap_hdr, cap_data);
      if (ret) {
        syslog(LOG_ERR, "Failed to set user capabilites, [%d] : %s.\n", errno,
               strerror(errno));
        exit(RET_INTER);
      }
      
      controller_internal(conf, sv[1]);

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

/* Run a process inside the container. */
int run_process() {

  /* TODO: Implement. */
  return RET_NOSUPP;
}

static int request(struct COM *req, struct COM *res, const char *unix_socket) {
  struct sockaddr_un addr;
  int sockfd, ret;
  ssize_t bytes;

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, unix_socket, sizeof(addr.sun_path) - 1);

  sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sockfd < 0) {
    syslog(LOG_ERR, "Could not open container socket, [%d] : %s.", errno,
           strerror(errno));
    return RET_INTER;
  }

  ret = connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un));
  if (ret < 0) {
    syslog(LOG_ERR,
           "Could not connect to the container controller, "
           "[%d] : %s.",
           errno, strerror(errno));
    close(sockfd);
    return RET_INTER;
  }

  bytes = send(sockfd, req, sizeof(struct COM), 0);
  if (bytes < 0) {
    syslog(LOG_ERR, "Failed to send request, [%d] : %s.", errno,
           strerror(errno));
    close(sockfd);
    return RET_INTER;
  }

  bytes = recv(sockfd, res, sizeof(struct COM) - 1, 0);
  if (bytes < 0) {
    syslog(LOG_ERR, "Failed to receive response, [%d] : %s.", errno,
           strerror(errno));
    close(sockfd);
    return RET_INTER;
  }

  close(sockfd);
  return RET_SUCC;
}

void libsic_execute(const libsic_cconf_t *conf, const char *path,
                    const char *arg0) {
  struct COM req, res;
  pid_t pid;
  struct __user_cap_header_struct cap_hdr;
  struct __user_cap_data_struct cap_data[2];
  int fd, ret = 0;

  req.type = REQ_PID;
  ret = request(&req, &res, conf->unix_socket);
  if (ret) {
    syslog(LOG_ERR, "Could not send PID request to the container "
                    "controller instance.");
    return;
  }

  if (res.type != RES_PID) {
    syslog(LOG_ERR, "Incorrect packet received from the container "
                    "controller instance.");
    return;
  }

  pid = *((pid_t *)res.data);

  syslog(LOG_INFO, "Executor received PID from controller: %d.", pid);

  fd = syscall(SYS_pidfd_open, pid, 0);
  if (fd < 0) {
    syslog(LOG_ERR, "Failed to attain PID fd, [%d] : %s.\n", errno,
           strerror(errno));
    return;
  }

  memset(&cap_hdr, 0, sizeof(cap_hdr));
  memset(cap_data, 0, sizeof(cap_data));

  cap_hdr.version = _LINUX_CAPABILITY_VERSION_3;
  cap_hdr.pid = 0;

  /* Current user needs to be an administrator. */
  /* Capabilities will be cloned to the new user namespace (if applicable). */
  ret = syscall(SYS_capget, &cap_hdr, cap_data);
  if (ret) {
    syslog(LOG_ERR, "Failed to get user capabilities, [%d] : %s.\n", errno,
           strerror(errno));
    return;
  }

  ret = setns(fd, conf->namespaces);
  if (ret) {
    syslog(LOG_ERR, "Failed to set new namespace, [%d] : %s.\n", errno,
           strerror(errno));
    return;
  }

  ret = syscall(SYS_capset, &cap_hdr, cap_data);
  if (ret) {
    syslog(LOG_ERR, "Failed to set user capabilites, [%d] : %s.\n", errno,
           strerror(errno));
    return;
  }

  ret = chroot(conf->rootfs_path);
  if (ret) {
    syslog(LOG_ERR, "Failed to change the root directory, [%d] : %s.\n", errno,
           strerror(errno));
    return;
  }

  ret = chdir("/");
  if (ret)
    return;

  execl(path, arg0, NULL);
}

/* Shut down currently running container controller instance(s). */
int libsic_destroy_container(const libsic_cconf_t *conf) {
  struct COM req, res;
  int ret;

  req.type = REQ_KILL;
  ret = request(&req, &res, conf->unix_socket);
  if (ret) {
    syslog(LOG_ERR, "Could not send request to kill the container "
                    "controller instance.");
    return ret;
  }

  if (res.type == RES_KILL)
    return RET_SUCC;
  syslog(LOG_ERR, "Incorrect packet received from the container "
                  "controller instance.");

  return RET_ABN;
}

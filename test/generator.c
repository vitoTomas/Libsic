#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sched.h>
#include <string.h>

#include <libsic.h>

#define ARCHIVE_PATH   "../test_files/alpine_rootfs.tar"
#define SOCKET         "/tmp/socket_1"

#define CONTAINER_DIR  "/tmp/scontainer"
#define ROOTFS_DIR     "/tmp/scontainer/rootfs"

int main()
{
  libsic_cconf_t conf;

  /* Set the container configuration. */
  conf.flags = LS_DAEMONIZE;
  conf.namespaces = CLONE_NEWPID | CLONE_NEWIPC | CLONE_NEWUTS;
  strncpy(conf.container_path, CONTAINER_DIR, PATH_LEN_MAX);
  strncpy(conf.rootfs_path, ROOTFS_DIR, PATH_LEN_MAX);
  strncpy(conf.tar_archive, ARCHIVE_PATH, PATH_LEN_MAX);
  strncpy(conf.unix_socket, SOCKET, PATH_LEN_MAX);
  
  printf("Running libsic test...\n");
  libsic_init_container(conf);
  printf("Container initialized!\n");
  libsic_execute(&conf, "/bin/sh", "/bin/sh");

  return 0;
}

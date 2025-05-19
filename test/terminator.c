#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sched.h>
#include <string.h>

#include <libsic.h>

#define ARCHIVE_PATH   "/home/vito/Libsic/test_files/alpine_rootfs.tar"
#define SOCKET         "/tmp/socket_1"

#define CONTAINER_DIR  "/home/vito/scontainer"
#define ROOTFS_DIR     "/home/vito/scontainer/rootfs"

int main()
{
  libsic_cconf_t conf;

  /* Set the container configuration. */
  conf.flags = LS_DAEMONIZE;
  conf.namespaces = CLONE_NEWIPC | CLONE_NEWPID | CLONE_NEWUSER | CLONE_NEWUTS;
  strncpy(conf.container_path, CONTAINER_DIR, PATH_LEN_MAX);
  strncpy(conf.rootfs_path, ROOTFS_DIR, PATH_LEN_MAX);
  strncpy(conf.tar_archive, ARCHIVE_PATH, PATH_LEN_MAX);
  strncpy(conf.unix_socket, SOCKET, PATH_LEN_MAX);
  
  printf("Killing container...\n");
  libsic_destroy_container(&conf);

  return 0;
}

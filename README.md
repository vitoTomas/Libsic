 _     ___________  _____ _____ _____
| |   |_   _| ___ \/  ___|_   _/  __ \
| |     | | | |_/ /\ `--.  | | | /  \/
| |     | | | ___ \ `--. \ | | | |
| |_____| |_| |_/ //\__/ /_| |_| \__/\
\_____/\___/\____/ \____/ \___/ \____/

# SimpleContainer Runtime Library

SimpleContainer Runtime Library is a container runtime library written in C
programming language. It is designed to be used in new or existing software
that requires Linux *containerization*.

# Implementation

Library creates two processes to instance a container. They are the external
controller (ECO) and the internal controller (ICO). ECO is used as an endpoint
with which host system processes can communicate with using *Unix Domain Socket*
(UDS). ICO is the container root process and as such is resposible for keeping
the container instace alive. ECO and ICO can communicate between each other
using a pair od UDS sockets created before forking - ICO is created from ECO.

To kill a container instance one needs to send a message to ECO so it can
notify ICO it can die. All of this is done using the library interface.

When starting new processes in the existing container instance, namespace
properties are copied from ICO. This is done through PID file descriptor(s).
Container(s) must be instanced correctly before new processes are created.

> [!CAUTION]
> Killing ICO without following library defined protocol can lead to an
> *unkillable* instance which will keep running in the background until the
> system reboots.

# Diagram

┌────────────┐                   ┌───────────────┐
│            │                   │               │
│  Program  ◄┼───────────────────┼► Libsic (.so) │
│            │                   │               │
└────────────┘                   └───────▲───────┘
                                         │
                                         │
                        ┌────────────────┘
                        │
                        │
                   ┌────▼─────┐        ┌─────────┐
                   │          │        │         │
                   │   ECO   ◄┼────────┼─► ICO   │
                   │          │        │         │
                   └──────────┘        └─────────┘

                       HOST             CONTAINER

# License

Defined in the `LICENSE` file and valid for all of the source code defined
in the `src` and `interface` directories and their respective subdirectories
and files.

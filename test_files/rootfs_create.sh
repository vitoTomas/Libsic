#!/bin/bash

podman build -t alpine .
podman create --name alpine_container alpine
podman export alpine_container -o alpine_rootfs.tar

#!/bin/bash

/storage/.kodi/addons/service.system.docker/bin/docker run -d  --rm -v /storage/browser/cefbrowser:/app -v /dev/shm:/dev/shm --ipc="host" --net=host ghcr.io/zabrimus/cefbrowser-base:latest -ini sockets.ini &> /storage/browser/browser.log

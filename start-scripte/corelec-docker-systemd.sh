[Unit]
Description=cefbrowser
After=docker.service
Requires=docker.service

[Service]
TimeoutStartSec=2
StandardOutput=file:/storage/browser/cefbrowser/browser.log
StandardError=file:/storage/browser/cefbrowser/browser-error.log
TimeoutStartSec=0
Restart=always
ExecStop=/storage/.kodi/addons/service.system.docker/bin/docker container stop --time=2 cefbrowser
ExecStartPre=-/storage/.kodi/addons/service.system.docker/bin/docker exec cefbrowser stop
ExecStartPre=-/storage/.kodi/addons/service.system.docker/bin/docker rm cefbrowser
ExecStartPre=/storage/.kodi/addons/service.system.docker/bin/docker pull ghcr.io/zabrimus/cefbrowser-base:latest
ExecStart=/storage/.kodi/addons/service.system.docker/bin/docker run --rm --name cefbrowser \
    -v /storage/browser/cefbrowser:/app \
    -v /dev/shm:/dev/shm \
    --ipc="host" \
    --net=host \
    ghcr.io/zabrimus/cefbrowser-base:latest \
    -ini sockets.ini \

[Install]
WantedBy=default.target
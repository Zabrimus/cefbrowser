# cefbrowser 
Works together with [vdr-plugin-web](https://github.com/Zabrimus/vdr-plugin-web) and [remotetranscode](https://github.com/Zabrimus/remotetranscode) to show HbbTV application and stream videos.

### Dependencies (cross compile, Ubuntu 22.04)
#### arm
```
apt install -y zlib1g-dev:armhf libssl-dev:armhf libcrypt-dev:armhf libglib2.0-dev:armhf libnss3-dev:armhf \
        libatk1.0-dev:armhf libatk-bridge2.0-dev:armhf libcups2-dev:armhf libxcomposite-dev:armhf libxdamage-dev:armhf \
        libxrandr-dev:armhf libgbm-dev:armhf libxkbcommon-dev:armhf libpango1.0-dev:armhf libasound2-dev:armhf
```

#### arm64
```
sudo apt install -y zlib1g-dev:arm64 libssl-dev:arm64 libcrypt-dev:arm64 libglib2.0-dev:arm64 libnss3-dev:arm64 \
        libatk1.0-dev:arm64 libatk-bridge2.0-dev:arm64 libcups2-dev:arm64 libxcomposite-dev:arm64 libxdamage-dev:arm64 \
        libxrandr-dev:arm64 libgbm-dev:arm64 libxkbcommon-dev:arm64 libpango1.0-dev:arm64 libasound2-dev:arm64
```

### Build (default is x86)
```
./setup.sh <arm, arm64 or x86>
meson setup build
cd build
meson compile
meson install
```
The whole binary including all necessary files/libs exists in folder ```build/Release```.

### Configuration
A default configuration can be found in folder config: ```sockets.ini```.

:fire: All ports/ip addresses in ```sockets.ini``` must be the same as for ```vdr-plugin-web``` and ```remotetranscoder```.
It's safe to use the same sockets.ini for all of the three parts (vdr-plugin-web, cefbrowser, remotetranscoder). 

### Start
Change to directory ```build/Release```.

```./cefbrowser  --config=/path/to/sockets.ini --remote-allow-origins=http://localhost:9222```

or if you don't need the possibility to debug the application in Chrome

```./cefbrowser  --config=/path/to/sockets.ini```

### Parameters
cefbrowser knows parameters which exists exclusively for the browser. 
Apart from these parameter a huge list of other parameters exists. 
Nearly of parameters of Chromium can be used. See e.g. [here](https://peter.sh/experiments/chromium-command-line-switches/). 
It's up to you to decide what you want to use and test ;)

```
-c / --config </path/to/sockets.ini>   (mandatory parameter)
-l / --loglevel <level> (from 0 to 4, where 4 means very, very verbose logging)
-z / --zoom <value> Zooms the http page to the desired width. The result often looks much better than the default value 1280.
         The downside is, that more data needs to be processed or send.
         Valid values are: 1280, 1920, 2560 and 3840
-q / --osdqoi The images between the browser and VDR are compressed using [qoi](https://qoiformat.org/) and are transferred via TCP/IP over the network.
         With this option the browser can be started on a different machine as VDR.                      
-f / --fulloas Normally only changed parts of the OSD are exchanged between the browser and VDR.
         With this option the full OSD will exchanged. This is much slower but could be lead to visual better results.
-p / --profilePath <path> Defines the path where cefbrowser shall save his profile data
-s / --staticPath <path> Mandatory static content can be found in this directory.
        E.g. css, html, javascript and so on.
-b / --bindall The browser uses every network interface. Could lead to interesting effects if Docker/Incus/LXD is also installed.                  
```

### Logging
Log entries will be written to stdout/stderr.

### Tested channels
- Das Erste
- ZDF

## Releases
The binary releases can be used in VDR*ELEC.

| Distro     | Version | Release                             |
|------------|---------|-------------------------------------|
| CoreELEC   | 19      | cefbrowser-armhf-openssl-3.tar.gz   |
| CoreELEC   | 20      | cefbrowser-armhf-openssl-3.tar.gz   |
| CoreELEC   | 21-ng   | cefbrowser-armhf-openssl-3.tar.gz   |
| CoreELEC   | 21-ne   | cefbrowser-arm64-openssl-3.tar.gz   |
| LibreELEC  | all     | cefbrowser-arm64-openssl-3.tar.gz   |

## VDR*ELEC, CoreELEC-19 (sample installation/configuration) 
- Install Kodi docker addon
    - System 
    - Addons 
    - Aus Repository installieren
    - CoreELEC Add-ons
    - Dienste
    - Docker
  
- Add docker bin to PATH
    ```
    nano /storage/.profile 
    ```
    add 
    ```
    export PATH=/storage/.kodi/addons/service.system.docker/bin/:$PATH
    ```
  reboot

#### Tested installation of cefbrowser in VDR*ELEC/CE-19
- Pull latest docker base runtime image of the cefbrowser
  ```
  /storage/.kodi/addons/service.system.docker/bin/docker pull ghcr.io/zabrimus/cefbrowser-base:latest
  ```
- Get and install cefbrowser binary (adapt version if desired)
    ```
    mkdir -p /storage/browser
    cd /storage/browser
    wget https://github.com/Zabrimus/cefbrowser/releases/download/2023-07-01/cefbrowser-armhf-openssl-3-5ab33f2.tar.gz
    tar -xf cefbrowser-armhf-openssl-3-5ab33f2.tar.gz
    ln -s cefbrowser-armhf-openssl-3-5ab33f2 cefbrowser
    ```

- Adapt sockets.ini accordingly and copy sockets.ini to ```/storage/browser/cefbrowser```
  sample sockets.ini can be found in the repository ```/config/sockets.ini```
  ```
  [vdr]
  http_ip = 192.168.178.12
  http_port = 50000

  [browser]
  http_ip = 192.168.178.12
  http_port = 50001

  [transcoder]
  http_ip = 192.168.178.20
  http_port = 50002
  ```
- Optional: hbbtv sqllite3 database

   For Vodafone West user a prefilled database exists in /storage/browser/cefbrowser/database/.
   ```
   cp /storage/browser/cefbrowser/database/Vodafone_West_hbbtv_urls.db /storage/browser/cefbrowser/database/hbbtv_urls.db
   ```
   All others needs to copy an already existing database or have to prefill the database. 
   A channel switch to the desired channel and wait some minutes is sufficient.
- start the browser via docker (first test, manual start)
  ```
    cd /storage/browser/cefbrowser
    /storage/.kodi/addons/service.system.docker/bin/docker run -d  --rm -v /storage/browser/cefbrowser:/app -v /dev/shm:/dev/shm --ipc="host" --net=host ghcr.io/zabrimus/cefbrowser-base:latest -ini sockets.ini &> /storage/browser/browser.log
  ```
- start the browser automatically via systemd
The systemd service ```start-scripte/cefbrowser.service``` contains a sample service configuration which i use on my system.
  ```
  [Unit]
  Description=cefbrowser
  Requires=network-online.target graphical.target docker.service
  StartLimitIntervalSec=400
  StartLimitBurst=5
  
  [Service]
  RestartSec=30
  Restart=always
  StandardOutput=file:/storage/browser/cefbrowser/browser.log
  StandardError=file:/storage/browser/cefbrowser/browser-error.log
  TimeoutStartSec=0
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
  -ini sockets.ini
  
  [Install]
  WantedBy=multi-user.target
  ```
  which can be copied to ```/storage/.config/system.d/cefbrowser.service```
  An important configuration is ```RestartSec=30```, otherwise the browser will be started, before docker is up and running.
- In VDR*ELEC i've added a configuration in ```/storage/.profile``` 
  ```
  START_CEFBROWSER=yes
  ```
  and in ```/storage/.config/autostart.sh``` i've added the following entries
  ```
  . /storage/.profile
  if [ "${START_CEFBROWSER}" = "yes" ]; then
      systemctl start cefbrowser
  fi
  ```
- Choose a channel in VDR and goto Menu/Web. Wait or press directly the red button.
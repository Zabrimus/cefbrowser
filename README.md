# Warning
- highly instable 
- does not works as desired 
- could destroy your system 
- in development phase


## cefbrowser (Part 2)
Works together with ```vdr-plugin-web``` and ```remotetranscoder```

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
- Choose a channel in VDR and goto Menu/Web. Wait or press directly the red button.
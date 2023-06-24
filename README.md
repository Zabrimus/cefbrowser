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

| Distro     | Version   | Release                             |
|------------|-----------|-------------------------------------|
| CoreELEC   | 19        | cefbrowser-armhf-openssl-1.tar.gz   |
| CoreELEC   | 19/Docker | cefbrowser-armhf-openssl-3.tar.gz   |
| CoreELEC   | 20        | cefbrowser-armhf-openssl-3.tar.gz   |
| CoreELEC   | 21-ng     | cefbrowser-armhf-openssl-3.tar.gz   |
| CoreELEC   | 21-ne     | cefbrowser-arm64-openssl-3.tar.gz   |
| LibreELEC  | all       | cefbrowser-arm64-openssl-3.tar.gz   |

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

- Get and install cefbrowser binary (adapt version if desired)
    ```
    mkdir -p /opt/cefbrowser
    wget https://github.com/Zabrimus/cefbrowser/releases/download/2023-06-19/cefbrowser-armhf-openssl-3-2c14cfa.tar.gz
    tar -xf cefbrowser-armhf-openssl-3-2c14cfa.tar.gz
    ```

- Adapt sockets.ini accordingly and copy sockets.ini to ```/opt/cefbrowser/cefbrowser-armhf-openssl-3-2c14cfa```
- start the browser via docker
  ```
    cd /opt/cefbrowser/cefbrowser-armhf-openssl-3-2c14cfa
    docker run -it --rm -v .:/app -v /dev/shm:/dev/shm --ipc="host" --net=host ghcr.io/zabrimus/cefbrowser-base -ini sockets.ini
  ```
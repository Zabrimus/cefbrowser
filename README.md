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

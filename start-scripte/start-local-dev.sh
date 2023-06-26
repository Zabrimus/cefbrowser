#!/bin/bash

 ./cefbrowser -l 3  --config=sockets.ini --remote-allow-origins=http://localhost:9222 | tee browser.log

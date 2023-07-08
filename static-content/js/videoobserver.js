const PLAY_STATES = {
    stopped: 0,
    playing: 1,
    paused: 2,
    connecting: 3,
    buffering: 4,
    finished: 5,
    error: 6,
};

window.promoteVideoSize = (node, considerLayer) => {
    let position = node.getBoundingClientRect();
    let bodyPos = document.getElementsByTagName('body')[0].getBoundingClientRect();
    let overlay = document.getElementById('_video_color_overlay_');

    /*
    console.log("In PromoteVideoSize: body (" + bodyPos.x + "," + bodyPos.y + "," + bodyPos.width + "," + bodyPos.height + ")");
    console.log("In PromoteVideoSize: node (" + position.x + "," + position.y + "," + position.width + "," + position.height + ")");
    console.log("In PromoteVideoSize: Fullscreen: " + ((position.x === bodyPos.x) && (position.y === bodyPos.y) && (position.height === bodyPos.height) && (position.width === bodyPos.width)));
    */

    // if width or height of body is 0, then set values to the maximum size
    if (bodyPos.width == 0 || bodyPos.height == 0) {
        bodyPos.width = 1280;
        bodyPos.height = 720;
    }

    if ((position.width == 300) && (position.height == 150)) {
        // sometimes the wrong size is requested. Ignore this to prevent flickering
        // console.log("Size 300x150 requested");

        overlay.style.visibility = "hidden";
        window.cefVideoFullscreen();
        return;
    }

    if ((position.x === bodyPos.x) && (position.y === bodyPos.y) && (position.height === bodyPos.height) && (position.width === bodyPos.width)) {
        overlay.style.visibility = "hidden";

        window.cefVideoFullscreen();
    } else {
        if (overlay && considerLayer) {
            overlay.style.visibility = "visible";
            overlay.style.left = position.x.toString() + "px";
            overlay.style.top = position.y.toString() + "px";
            overlay.style.width = position.width.toString() + "px";
            overlay.style.height = position.height.toString() + "px";
            overlay.style.backgroundColor = "rgb(254, 46, 154)";
        }

        window.cefVideoSize(position.x | 0, position.y | 0, position.width | 0, position.height | 0);
    }
}

function watchAndHandleVideoObjectMutations() {
    const addVideoNode = (node, url) => {
        let video = document.createElement('video');

        node.play = node.play || function(speed) {
            console.log("Node Play Speed " + speed);
            if (speed === 0) {
                setTimeout(() => {
                    window.cefPauseVideo();

                    video.pause();
                    node.speed = 0;
                    node.playState = PLAY_STATES.paused;
                }, 0);
            }
            else if (speed > 0) {
                setTimeout(() => {
                    if (node.playState === PLAY_STATES.paused) {
                        window.cefResumeVideo(String(video.currentTime));
                    }

                    node.speed = speed;
                    node.playState = PLAY_STATES.playing;
                    video.playbackRate = speed;
                    video.play().catch((e) => {
                        console.error(e.message);
                        node.error = 5;
                    });

                    window.start_video_quirk();

                }, 0);
            }
            else if (speed < 0) {
                node.speed = speed;
                node.playState = PLAY_STATES.playing;
                video.playbackRate = 1.0;
                video.play().catch((e) => {
                    console.error(e.message);
                    node.error = 5;
                });
                this.rewindInterval = setInterval(() => {
                    if (video.currentTime < 0.1) {
                        video.currentTime = 0;
                        video.pause();
                        node.speed = 0;
                        clearInterval(this.rewindInterval);
                    } else {
                        video.currentTime = video.currentTime -= 0.1;
                    }
                }, 100);
            }
            return true;
        };

        node.stop = node.stop || function() {
            console.log("Node Stop ");
            window.cefStopVideo();

            video.pause();
            video.currentTime = 0;
            node.playState = PLAY_STATES.stopped;
            node.playPosition = 0;
            node.speed = 0;

            window.stop_video_quirk();

            return true;
        }

        node.seek = node.seek || function(posInMs) {
            console.log("Node seek video " + video.currentTime + ", node " + node.playPosition + " --> " + posInMs + " ==> " + (posInMs / 1000));

            window.cefSeekVideo(String(posInMs / 1000));

            video.currentTime = posInMs / 1000;
            node.playPosition = posInMs;

            console.log("Node nach seek video " + video.currentTime + ", node " + node.playPosition + " --> " + posInMs);
        }

        video && video.addEventListener && video.addEventListener('playing', function () {
            console.log("Video event playing");

            node.playState = PLAY_STATES.playing;
            if (node.onPlayStateChange) {
                node.onPlayStateChange(node.playState);
            } else {
                var playerEvent = new Event('PlayStateChange');
                playerEvent.state = node.playState;
                node.dispatchEvent(playerEvent);
            }

            window.start_video_quirk();

        }, false);

        video && video.addEventListener && video.addEventListener('pause', function () {
            console.log("Video event pause");

            node.playState = PLAY_STATES.paused;
            if (node.onPlayStateChange) {
                node.onPlayStateChange(node.playState);
            } else {
                let playerEvent = new Event('PlayStateChange');
                playerEvent.state = node.playState;
                node.dispatchEvent(playerEvent);
            }
        }, false);

        video && video.addEventListener && video.addEventListener('ended', function () {
            console.log("Video event ended");

            node.playState = PLAY_STATES.finished;
            if (node.onPlayStateChange) {
                node.onPlayStateChange(node.playState);
            } else {
                var playerEvent = new Event('PlayStateChange');
                playerEvent.state = node.playState;
                node.dispatchEvent(playerEvent);
            }

            window.stop_video_quirk();

        }, false);

        video && video.addEventListener && video.addEventListener('error', function (e) {
            console.log("Video event error");

            node.playState = PLAY_STATES.error;
            if (node.onPlayStateChange) {
                node.onPlayStateChange(node.playState);
            } else {
                var playerEvent = new Event('PlayStateChange');
                playerEvent.state = node.playState;
                node.error = 0; // 0 - A/V format not supported
                node.dispatchEvent(playerEvent);
            }
        }, false);

        video && video.addEventListener && video.addEventListener('durationchange', () => {
            console.log("Video event durationchange");
            node.duration = video.duration * 1000;
        }, false);

        video && video.addEventListener && video.addEventListener('timeupdate', function () {
            // console.log("Event timeupdate video " + video.currentTime + ", node " + node.playPosition);
            // console.log("Duration: " + node.duration + ", time: " + video.currentTime + ", left " + (node.duration - video.currentTime));

            var pos = Math.floor(video.currentTime * 1000);
            node.playPostion = pos;
            if (node.PlayPositionChanged) {
                node.PlayPositionChanged(pos);
            }
            node.playPosition = pos;

            var playerEvent = new Event('PlayPositionChanged');
            playerEvent.position = pos;
            node.dispatchEvent(playerEvent);
        }, false);

        video && video.addEventListener && video.addEventListener('ratechange', function () {
            var playSpeed = video.playbackRate;

            var playerEvent = new Event('PlaySpeedChanged');
            playerEvent.speed = playSpeed;
            node.dispatchEvent(playerEvent);

        }, false);

        video && video.addEventListener && video.addEventListener('seeked', function () {
            console.log("Event seek video " + video.currentTime + ", node " + node.playPosition);

            var pos = Math.floor(video.currentTime * 1000);
            if (node.onPlayPositionChanged) {
                node.playPosition = pos;
                node.PlayPositionChanged(pos);
            } else {
                var playerEvent = new Event('PlayPositionChanged');
                playerEvent.position = pos;
                node.dispatchEvent(playerEvent);
            }
        }, false);

        node.playTime = video.duration * 1000;
        node.error = -1;
        node.type = "video/webm";

        video.autoplay = true;
        video.src = url;
        video.style = 'top:0px; left:0px; width:100%; height:100%;background-color:rgb(254, 46, 154)';

        node.replaceChildren(video);
    }

    const watchAndHandleVideoAttributes = (videoObject, layer) => {
        const handleAttributeChanged = (event) => {
            window.promoteVideoSize(videoObject, layer);
        };

        const handleAttributeMutation = (mutationList, mutationObserver) => {
            mutationList.forEach((mutation) => {
                switch (mutation.type) {
                    case 'childList':
                        break;
                    case 'attributes':
                        handleAttributeChanged(mutation);
                        break;
                }
            });
        };

        const mutationAttributeObserver = new MutationObserver(handleAttributeMutation);
        mutationAttributeObserver.observe(videoObject, {
            'subtree': true,
            'childList': true,
            'attributes': true,
            'characterData': true
        });
    }

    const checkNode = (node) => {
        let mimeType = node.type;
        if (!node.type) {
            return;
        }

        let considerLayer = false;

        mimeType = mimeType.toLowerCase();

        if (mimeType.lastIndexOf('video/broadcast', 0) === 0) { // TV
            console.log("Found TV on node: " + node);
            considerLayer = false;
        } else if (mimeType.lastIndexOf('video/mpeg4', 0) === 0 ||          // mpeg4 video
                   mimeType.lastIndexOf('video/mp4', 0) === 0 ||            // h.264 video
                   mimeType.lastIndexOf('audio/mp4', 0) === 0 ||            // aac audio
                   mimeType.lastIndexOf('audio/mpeg', 0) === 0 ||           // mp3 audio
                   mimeType.lastIndexOf('application/dash+xml', 0) === 0 || // mpeg-dash
                   mimeType.lastIndexOf('video/mpeg', 0) === 0) {           // mpeg-ts

            console.log("Found Video on node: " + node + " -> " + node.data);
            // console.log("All Cookies: " + document.cookie);

            let newUrl = window.cefStreamVideo(node.data);
            addVideoNode(node, newUrl);
            node.data = newUrl;
            node.style.visibility = 'hidden';

            considerLayer = true;
        }

        node.bindToCurrentChannel = node.bindToCurrentChannel || function() {
            console.log('Node bindToCurrentChannel');
            return window.HBBTV_POLYFILL_NS.currentChannel;
        }

        node.setChannel = node.setChannel || function () {
            console.log('Node setChannel() ...');
        };

        node.prevChannel = node.prevChannel || function () {
            console.log('Node prevChannel() ...');
            return window.HBBTV_POLYFILL_NS.currentChannel;
        };

        node.nextChannel = node.nextChannel || function () {
            console.log('Node BroadcastVideo nextChannel() ...');
            return window.HBBTV_POLYFILL_NS.currentChannel;
        };

        node.release = node.release || function () {
            console.log('Node BroadcastVideo release() ...');
        };

        node.setFullScreen = node.setFullScreen || function() {
            let bodyPos = document.getElementsByTagName('body')[0].getBoundingClientRect();

            node.x = 0;
            node.y = 0;
            node.width = bodyPos.width;
            node.height = bodyPos.height;
        }

        node.createChannelObject = node.createChannelObject || function () {
            console.log('Node createChannelObject() ...');
        };

        node.addStreamEventListener = node.addStreamEventListener || function (url, eventName, listener) {
            console.log('Node register listener -', eventName);
            window.HBBTV_POLYFILL_NS.streamEventListeners.push({ url, eventName, listener });
        };

        node.removeStreamEventListener = node.removeStreamEventListener || function (url, eventName, listener) {
            console.log('Node remove listener -', eventName);

            var idx = window.HBBTV_POLYFILL_NS.streamEventListeners.findIndex((e) => {
                return e.listener === listener && e.eventName === eventName && e.url === url;
            });

            window.HBBTV_POLYFILL_NS.streamEventListeners.splice(idx, 1);
        };

        watchAndHandleVideoAttributes(node, considerLayer);
    }

    const handleChildAddedRemoved = (mutation) => {
        if (mutation.addedNodes.length > 0) {
            mutation.addedNodes.forEach((node) => {
                checkNode(node);
            });
        } else if (mutation.removedNodes.length > 0) {
            // TODO: handle object removal
        }
    };

    const handleMutation = (mutationList) => {
        mutationList.forEach((mutation) => {
            switch (mutation.type) {
                case 'childList':
                    handleChildAddedRemoved(mutation);
                    break;
                case 'attributes':
                    break;
            }
        });
    };

    // prepare at first all object nodes
    const objects = document.getElementsByTagName("object");
    for (let i = 0; i < objects.length; i++) {
        checkNode(objects[i]);
    }

    this.mutationObserver = new MutationObserver(handleMutation);
    this.mutationObserver.observe(document.body, {
        'subtree': true,
        'childList': true,
        'attributes': true,
        'characterData': true,
        'attributeFilter': ["type"]
    });
}

watchAndHandleVideoObjectMutations();

setTimeout(function() {
    const objects = document.getElementsByTagName("object");

    for (let i = 0; i < objects.length; i++) {
        objects[i].style.visibility = 'hidden';

        let mimeType = objects[i].type;
        if (!objects[i].type) {
            continue;
        }

        mimeType = mimeType.toLowerCase();
        if (mimeType.lastIndexOf('video/broadcast', 0) === 0) { // TV
            window.promoteVideoSize(objects[i], true);
        }
    }
}, 250);

const PLAY_STATES = {
    stopped: 0,
    playing: 1,
    paused: 2,
    connecting: 3,
    buffering: 4,
    finished: 5,
    error: 6,
};

window.displayVideoOverlay = (x,y,w,h) => {
    let showOverlay = false;

    if (document.URL.includes("hbbtv-apps.redbutton.de")) {
        showOverlay = true;
        h = h - 8;
    }

    if (showOverlay) {
        let el = document.getElementById("_video_color_overlay_");
        if (el) {
            el.style.width = w + "px";
            el.style.height = h + "px";
            el.style.left = x + "px";
            el.style.top = y + "px";
            el.style.visibility = "visible";
        }
    }
}

window.hideVideoOverlay = () => {
    let el = document.getElementById("_video_color_overlay_");
    if (el) {
        el.style.visibility = "hidden";
    }
}

window.promoteVideoSize = (node) => {
    let position = node.getBoundingClientRect();
    let bodyPos = document.getElementsByTagName('body')[0].getBoundingClientRect();

    if ((position.x === bodyPos.x) && (position.y === bodyPos.y) && (position.height === bodyPos.height) && (position.width === bodyPos.width)) {
        window.cefVideoFullscreen();
        window.hideVideoOverlay();
    } else {
        let posx = position.x | 0;
        let posy = position.y | 0;
        let posw = position.width | 0;
        let posh = position.height | 0;

        console.log("Video: " + posx + "," + posy + " -> " + posw + " x " + posh);

        window.displayVideoOverlay(posx, posy, posw, posh);
        window.cefVideoSize(posx, posy, posw, posh);
    }
}

function addNodeFunctions(node) {
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

        let idx = window.HBBTV_POLYFILL_NS.streamEventListeners.findIndex((e) => {
            return e.listener === listener && e.eventName === eventName && e.url === url;
        });

        window.HBBTV_POLYFILL_NS.streamEventListeners.splice(idx, 1);
    };
}

function addVideoNode(node, url) {
    let video = document.createElement('video');

    node.play = node.play || function (speed) {
        console.log("Node Play Speed " + speed);
        if (speed === 0) {
            setTimeout(() => {
                window.cefPauseVideo();

                video.pause();
                node.speed = 0;
                node.playState = PLAY_STATES.paused;
            }, 0);
        } else if (speed > 0) {
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
        } else if (speed < 0) {
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

    node.stop = node.stop || function () {
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

    node.seek = node.seek || function (posInMs) {
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
    node.data = url;
    node.style.visibility = 'hidden';

    video.autoplay = true;
    video.src = url;
    video.style = 'top:0px; left:0px; width:100%; height:100%;';

    node.replaceChildren(video);
}

function checkObjectNode(summaries) {
    if (summaries.added === undefined || summaries.added.length === 0 || summaries.added[0].type === undefined) {
        return;
    }

    let node = summaries.added[0];

    if (node.type === 'video/broadcast') {
        console.log("NEU Found TV on node: " + node);
    } else if ((node.type === 'video/mpeg4') ||             // mpeg4 video
               (node.type === 'video/mp4') ||               // h.264 video
               (node.type === 'audio/mp4') ||               // aac audio
               (node.type === 'audio/mpeg') ||              // mp3 audio
               (node.type === 'application/dash+xml') ||    // mpeg-dash
               (node.type === 'video/mpeg')) {              // mpeg-ts
        console.log("NEU Found Video on node: " + node);
        console.log("Video URL: " + node.getAttribute('data'));
        let newUrl = window.cefStreamVideo(node.data);

        node.setAttribute('data', "http://404.gibbt.nixx");
        addVideoNode(node, newUrl);
    } else {
        // ignore all others
        console.log("Ignore type " + node.type);
        return;
    }

    addNodeFunctions(node);

    let clientRect = node.getBoundingClientRect();
    console.log("   => " + clientRect.left + "," + clientRect.top + " => " + clientRect.width + " x " + clientRect.height);

    promoteVideoSize(node);
}

const ms = new MutationSummary({
    callback(summaries) {
        summaries.forEach((summary) => checkObjectNode(summary));
    },
    queries: [
        {
          element: 'object',
          elementAttributes: 'data type',
        }
    ]
});

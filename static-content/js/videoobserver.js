const PLAY_STATES = {
    stopped: 0,
    playing: 1,
    paused: 2,
    connecting: 3,
    buffering: 4,
    finished: 5,
    error: 6,
};

window.addVideoOverlay = (node) => {
    let tv = document.createElement('div');
    tv.id = "_video_color_overlay_";
    tv.style.width = "100%";
    tv.style.height = "100%";
    tv.style.background = "rgb(254, 46, 154)";
    tv.style.zIndex = 999;
    tv.style.position = "relative";
    node.visibility = "hidden";
    node.append(tv);
}

window.displayVideoOverlay = (x,y,w,h) => {
    if (document.URL.includes("hbbtv-apps.redbutton.de")) {
        h = h - 8;
    }

   let el = document.getElementById("_video_color_overlay_");
    if (el) {
        el.style.visibility = "visible";
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

    if ( // video and body have nearly the same size
        ((Math.abs(position.x - bodyPos.x) < 20) &&
         (Math.abs(position.y - bodyPos.y) < 20) &&
         (Math.abs(position.height - bodyPos.height) < 20) &&
         (Math.abs(position.width - bodyPos.width) < 20)) ||

        // special case: if video width = 1280 and heigth = 720 is also fullscreen
        ( position.width === 1280 && position.height === 720)
    ) {
        window.cefVideoFullscreen();
        window.hideVideoOverlay();
        window.start_video_quirk_Fullscreen();
    } else {
        let posx = position.x | 0;
        let posy = position.y | 0;
        let posw = position.width | 0;
        let posh = position.height | 0;

        console.log("Body:  " + bodyPos.x + "," + bodyPos.y + " -> " + bodyPos.width + " x " + bodyPos.height);
        console.log("Video: " + posx + "," + posy + " -> " + posw + " x " + posh);

        window.displayVideoOverlay(posx, posy, posw, posh);
        window.stop_video_quirk_Fullscreen();
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

function addVideoNodeTypeObject(node, url, originalUrl) {
    let video = document.createElement('video');
    video.type = "video/webm";
    video.src = url;
    // video.autoplay = false;
    video.style = 'top:0px; left:0px; width:100%; height:100%;';
    video.preload = "auto";

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
                video.play()
                    .then((v) => {
                        window.start_video_quirk();
                    })
                    .catch((e) => {
                        console.error(e.message);
                        node.error = 5;
                    });
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
        video.removeAttribute('src'); // empty source
        video.load();

        video.currentTime = 0;
        node.playState = PLAY_STATES.stopped;
        node.playPosition = 0;
        node.speed = 0;

        // remove all event listeners
        document.getElementsByTagName('video')[0].outerHTML = document.getElementsByTagName('video')[0].outerHTML;
        node.outerHTML = node.outerHTML;

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
        console.log("Video event durationchange to " + video.duration);
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

    video && video.addEventListener && video.addEventListener('loadedmetadata', function () {
        console.log("Video loaded metadata, duration " + video.duration);
    }, false);

    console.log("video.duration: " + video.duration);

    // delete all children
    node.innerHTML = "";

    window.addVideoOverlay(node);

    node.playTime = video.duration * 1000;
    node.error = -1;
    node.type = "video/webm";
    node.data = "http://gibbet.nix"; // url;
    if (originalUrl != "http://gibbet.nix") {
        node.setAttribute("olddata", originalUrl);
    }
    node.style.visibility = 'hidden';

    node.append(video);

    setTimeout(() => {
        video.load();
        video.play();
    }, 0);
}

function addVideoNodeTypeVideo(node, url) {
    let video = node;

    // check if source element exists
    let videoSrc = video;
    let videoSrcElementArr = video.getElementsByTagName('source');

    if (videoSrcElementArr && videoSrcElementArr.length > 0) {
        videoSrc = videoSrcElementArr[0];
    }

    videoSrc.src = url;
    video.type = "video/webm";

    video && video.addEventListener && video.addEventListener('playing', function () {
        console.log("video tag playing");
        if (video.playState === PLAY_STATES.paused) {
            window.cefResumeVideo(String(video.currentTime));
        }

        video.playState = PLAY_STATES.playing;
    }, false);

    video && video.addEventListener && video.addEventListener('pause', function () {
        console.log("video tag pause");
        video.playState = PLAY_STATES.paused;
        window.cefPauseVideo();
    }, false);

    video && video.addEventListener && video.addEventListener('ended', function () {
        console.log("video tag ended: " + video.playState);
    }, false);

    video && video.addEventListener && video.addEventListener('error', function (e) {
        console.log("video tag error: ");
        if (video.playState !== PLAY_STATES.error) {
            window.cefStopVideo();
        }
    }, false);

    video && video.addEventListener && video.addEventListener('durationchange', () => {
        console.log("video tag durationchange");
    }, false);

    video && video.addEventListener && video.addEventListener('timeupdate', function () {

    }, false);

    video && video.addEventListener && video.addEventListener('ratechange', function () {
        console.log("video tag ratechange");
    }, false);

    video && video.addEventListener && video.addEventListener('seeked', function () {
        console.log("video tag seeked" + video.currentTime);
        window.cefSeekVideo(String(video.currentTime));
    }, false);

    video && video.addEventListener && video.addEventListener('loadedmetadata', function () {
        console.log("video tag loadedmetadata, duration " + video.duration);
    }, false);

    console.log("video.duration: " + video.duration);

    setTimeout(() => {
        video.load();
        video.play();
    }, 0);
}


function checkPositionObjectNode(summary) {
    // collect all nodes
    let nodeSet = new Set();

    for (const attribute of ['width', 'height', 'left', 'top', 'style']) {
        if (summary.attributeChanged !== undefined) {
            let arr = summary.attributeChanged[attribute];
            if (arr !== undefined) {
                for (let i = 0; i < arr.length; ++i) {
                    nodeSet.add(arr[i]);
                }
            }
        }
    }

    nodeSet.forEach(node => {
        if ((node.type === 'video/mpeg4') ||             // mpeg4 video
            (node.type === 'video/mp4') ||               // h.264 video
            (node.type === 'audio/mp4') ||               // aac audio
            (node.type === 'audio/mpeg') ||              // mp3 audio
            (node.type === 'application/dash+xml') ||    // mpeg-dash
            (node.type === 'video/mpeg') ||              // ts
            (node.type === 'video/broadcast') ||         // TV
            (node.type === 'video/webm')) {              // webm

            console.log("checkPositionObjectNode: promoteVideoSize");
            promoteVideoSize(node);
        }
    })
}

function checkAddedObjectNode(summaries) {
    if (summaries.added.length === 0) {
        return;
    }

    let node = summaries.added[0];
    checkSingleObjectNode(node);
}

function checkSingleObjectNode(node) {
    async function checkMpd(nodedata) {
        let mpdStart = 0;

        if (nodedata.endsWith(".mpd") || nodedata.indexOf("hbbtv.zdf.de/zdfm3/dyn/mpd.php") !== -1) {
            const mpd = await fetch(nodedata);
            const data = await mpd.text();

            let parser = new DOMParser();
            let xmlDoc = parser.parseFromString(data, "text/xml");

            let timeShiftBufferDepth = xmlDoc.documentElement.getAttribute("timeShiftBufferDepth");
            let periodStart = xmlDoc.getElementsByTagName("Period")[0].getAttribute("start");

            let startTime = durationToSeconds(parseWithTinyDuration(periodStart));
            mpdStart = durationToSeconds(parseWithTinyDuration(timeShiftBufferDepth)) - startTime;
            console.log("timeShiftBufferDepth: " + timeShiftBufferDepth + ", periodStart: " + periodStart + ", mpdStart: " + mpdStart);
        }

        return mpdStart;
    }

    if (node.type === 'video/broadcast') {
        console.log("Found TV on node: " + node);
        window.addVideoOverlay(node);

        addNodeFunctions(node);
        promoteVideoSize(node);
    } else if ((node.type === 'video/mpeg4') ||             // mpeg4 video
        (node.type === 'video/mp4') ||               // h.264 video
        (node.type === 'audio/mp4') ||               // aac audio
        (node.type === 'audio/mpeg') ||              // mp3 audio
        (node.type === 'application/dash+xml') ||    // mpeg-dash
        (node.type === 'video/mpeg')) {              // mpeg-ts
        console.log("Found Video on node: " + node);
        console.log("Video URL: " + node.getAttribute('data'));
        console.log("Node HTML: " + node.outerHTML);

        let location = document.location.toString();
        let checkVideoTag = location.includes("hbbtv.zdf.de");

        if (node.nodeName.toUpperCase() === 'object'.toUpperCase()) {
            (async() => {
                console.log("node.data = " + node.data);
                if (node.data !== undefined) {
                    let mpdStart = await checkMpd(node.data);
                    let newUrl = window.cefStreamVideo(node.data, document.cookie, document.referrer, navigator.userAgent, mpdStart);
                    addVideoNodeTypeObject(node, newUrl, node.data);
                    addNodeFunctions(node);
                    promoteVideoSize(node);
                }
            })();
        } else if (node.nodeName.toUpperCase() === 'video'.toUpperCase() && checkVideoTag) {
            (async() => {
                // ZDF inserts a video node itself
                console.log("node.data = " + node.data);
                console.log("node.type = " + node.type);
                console.log("node.src = " + node.src);

                // Node HTML: <video xmlns="http://www.w3.org/1999/xhtml" autoplay="true" type="application/dash+xml" width="1280" height="720" src="https://zdf-dash-15.akamaized.net/dash/live/2016508/de/manifest.mpd" style="position: absolute; left: 0px; top: 0px; width: 1280px; height: 720px; outline: transparent;"></video>

                if (node.data !== undefined) {
                    let mpdStart = await checkMpd(node.data);
                    let newUrl = window.cefStreamVideo(node.src, document.cookie, document.referrer, navigator.userAgent, mpdStart);
                    addVideoNodeTypeVideo(node, newUrl, node.src);
                    promoteVideoSize(node);
                } else if (node.src !== undefined) {
                    let mpdStart = await checkMpd(node.src);
                    let newUrl = window.cefStreamVideo(node.src, document.cookie, document.referrer, navigator.userAgent, mpdStart);
                    addVideoNodeTypeVideo(node, newUrl, node.src);
                    promoteVideoSize(node);
                }
            })();
        }
    } else if (node.type === 'video/webm') { // webm video
        // RTL changes only data attribute, but not the type
        (async() => {
            console.log("node.data = " + node.data);
            if (node.data !== undefined) {
                let mpdStart = await checkMpd(node.data);
                let newUrl = window.cefStreamVideo(node.data, document.cookie, document.referrer, navigator.userAgent, mpdStart);
                addVideoNodeTypeObject(node, newUrl, node.data);
                addNodeFunctions(node);
                promoteVideoSize(node);
            }
        })();
    } else {
        // ignore all others
        console.log("Ignore type " + node.type);
    }
}

// if an local application is loaded, then don't a create a MutationSummary
if (!document.location.href.includes('application/iptv/catalogue/index.html')) {
    const ms = new MutationSummary({
        callback(summaries) {
            summaries.forEach((summary) => {
                console.log(summary);

                checkAddedObjectNode(summary);
                checkPositionObjectNode(summary);
            });
        },
        queries: [
            {
                element: 'object',
                elementAttributes: 'data type width height left top style',
            },
            {
                element: 'video',
            },
        ]
    });
}
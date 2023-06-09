const PLAY_STATES = {
    stopped: 0,
    playing: 1,
    paused: 2,
    connecting: 3,
    buffering: 4,
    finished: 5,
    error: 6,
};

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
                    window.cefResumeVideo();

                    node.speed = speed;
                    node.playState = PLAY_STATES.playing;
                    video.playbackRate = speed;
                    video.play().catch((e) => {
                        console.error(e.message);
                        node.error = 5;
                    });
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
            node.playState = PLAY_STATES.playing;
            if (node.onPlayStateChange) {
                node.onPlayStateChange(node.playState);
            } else {
                var playerEvent = new Event('PlayStateChange');
                playerEvent.state = node.playState;
                node.dispatchEvent(playerEvent);
            }
        }, false);

        video && video.addEventListener && video.addEventListener('pause', function () {
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
            node.playState = PLAY_STATES.finished;
            if (node.onPlayStateChange) {
                node.onPlayStateChange(node.playState);
            } else {
                var playerEvent = new Event('PlayStateChange');
                playerEvent.state = node.playState;
                node.dispatchEvent(playerEvent);
            }
        }, false);

        video && video.addEventListener && video.addEventListener('error', function (e) {
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
            node.duration = video.duration * 1000;
        }, false);

        video && video.addEventListener && video.addEventListener('timeupdate', function () {
            console.log("Event timeupdate video " + video.currentTime + ", node " + node.playPosition);

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

        video.autoplay = true;
        video.src = url;
        video.style = 'top:0px; left:0px; width:100%; height:100%;';

        node.replaceChildren(video);
    }

    const checkNode = (node) => {
        let mimeType = node.type;
        if (!node.type) {
            return;
        }

        mimeType = mimeType.toLowerCase();

        if (mimeType.lastIndexOf('video/broadcast', 0) === 0) { // TV
            console.log("Found TV on node: " + node);
        } else if (mimeType.lastIndexOf('video/mpeg4', 0) === 0 ||
                   mimeType.lastIndexOf('video/mp4', 0) === 0 ||  // h.264 video
                   mimeType.lastIndexOf('audio/mp4', 0) === 0 ||  // aac audio
                   mimeType.lastIndexOf('audio/mpeg', 0) === 0) { // mp3 audio
            console.log("Found Video on node: " + node + " -> " + node.data);

            let newUrl = window.cefStreamVideo(node.data);
            addVideoNode(node, newUrl);
            // node.data = "";
            node.data = newUrl;
        } else if (mimeType.lastIndexOf('application/dash+xml', 0) === 0 || // mpeg-dash
                   mimeType.lastIndexOf('video/mpeg', 0) === 0) { // mpeg-ts
            console.log("Found MPEG on node: " + node + " -> " + node.data);

            let newUrl = window.cefStreamVideo(node.data);
            addVideoNode(node, newUrl);
            node.data = "";
        }
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

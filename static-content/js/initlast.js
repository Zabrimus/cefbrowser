function prepareElements() {
    const objects = document.getElementsByTagName("object");
    for (let i = 0; i < objects.length; i++) {
        let node = objects[i];

        let mimeType = node.type;
        if (!node.type) {
            continue;
        }

        mimeType = mimeType.toLowerCase();

        if (mimeType.lastIndexOf('video/broadcast', 0) >= 0) { // TV
            console.log("Init Found TV on node: " + node);
            node.style.visibility = "hidden";
        }
    }

    const videoElement = document.getElementById('video');
    if (videoElement) {
        videoElement.bindToCurrentChannel = videoElement.bindToCurrentChannel || function() {
            return window.HBBTV_POLYFILL_NS.currentChannel;
        }

        videoElement.setFullScreen = videoElement.setFullScreen || function() {
            let bodyPos = document.getElementsByTagName('body')[0].getBoundingClientRect();

            videoElement.x = 0;
            videoElement.y = 0;
            videoElement.width = bodyPos.width;
            videoElement.height = bodyPos.height;
        }
    }
}

prepareElements();

document.body.style["font-family"] = "Tiresias";
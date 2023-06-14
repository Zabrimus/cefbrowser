// modify object with type video/broadcast it the element already exists

/* Funktioniert nicht wie gewüscht
function prepareBroadcast() {
    const objects = document.getElementsByTagName("object");
    for (let i = 0; i < objects.length; i++) {
        let node = objects[i];

        let mimeType = node.type;
        if (!node.type) {
            continue;
        }

        mimeType = mimeType.toLowerCase();

        if (mimeType.lastIndexOf('video/broadcast', 0) === 0) { // TV
            console.log("Init Found TV on node: " + node);

            let div = document.createElement('div');
            div.style.width = "100%";
            div.style.height = "100%";
            div.style.backgroundColor = "rgb(254, 46, 154)";
            node.appendChild(div);
            node.style.visibility = "visible";
        }
    }
}
*/

function prepareBroadcast() {
    const objects = document.getElementsByTagName("object");
    for (let i = 0; i < objects.length; i++) {
        let node = objects[i];

        let mimeType = node.type;
        if (!node.type) {
            continue;
        }

        mimeType = mimeType.toLowerCase();

        if (mimeType.lastIndexOf('video/broadcast', 0) === 0) { // TV
            console.log("Init Found TV on node: " + node);
            node.style.visibility = "hidden";
        }
    }
}

prepareBroadcast();
var rcUtils = {
    MASK_CONSTANT_RED: 0x1,
    MASK_CONSTANT_GREEN: 0x2,
    MASK_CONSTANT_YELLOW: 0x4,
    MASK_CONSTANT_BLUE: 0x8,
    MASK_CONSTANT_NAVIGATION: 0x10,
    MASK_CONSTANT_PLAYBACK: 0x20,
    MASK_CONSTANT_NUMERIC: 0x100,

    setKeyset:function(app, mask) {
        try {
            app.privateData.keyset.setValue(mask);
        } catch (e) {
            // try as per OIPF DAE v1.1
            try {
                app.private.keyset.setValue(mask);
            }
            catch (ee) {
                // catch the error while setting keyset value
            }
        }
    },

    registerKeyEventListener:function() {
        document.addEventListener('keydown', function(e) {
            if (handleKeyCode(e.keyCode)) {
                e.preventDefault();
            }
        }, false);
    }
};

// RC button press handler function
function handleKeyCode(kc) {
    try {
        // process buttons
        switch (kc) {
            case VK_RED:
                // load TV Station Application
                window.cefRedButton(window.HBBTV_POLYFILL_NS.currentChannel.channelId);
                break;

            case VK_RIGHT:
                window.MyCar.right();
                break;

            case VK_LEFT:
                window.MyCar.left();
                break;

            case VK_DOWN:
                window.MyCar.down();
                break;

            case VK_UP:
                window.MyCar.up();
                break;

            case VK_ENTER:
                let url = window.MyCar.getSelectedUrl();

                let ov = document.getElementById("videocontainer");
                ov.innerHTML = "";

                let v = document.createElement('object');
                v.setAttribute('type', 'video/mp4');
                v.setAttribute('style', 'position: absolute; left: 0px; top: 0px; width: 1280px; height: 720px;')
                v.setAttribute('data', url);

                ov.append(v);
                break;

            default:
                break;
        }
    }
    catch (e) {
        // pressed unhandled key, catch the error
    }
    // we return true to prevent default action for processed keys
    return true;
}

// app entry function
function start() 
{
    try {
        // attempt to acquire the Application object
        var appManager = document.getElementById('applicationManager');
        var appObject = appManager.getOwnerApplication(document);
        // check if Application object was a success
        if (appObject === null) {
            // error acquiring the Application object!
        } 
        else {
            // we have the Application object, and we can initialize the scene and show our app
            scene.initialize(appObject);
            appObject.show();
        }
    }
    catch (e) {
        // this is not an HbbTV client, catch the error.
    }
}
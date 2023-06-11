var rcUtils = {
    registerKeyEventListener:function() {
        document.addEventListener('keydown', function(e) {
            if (handleKeyCode(e.keyCode)) {
                e.preventDefault();
            }
        }, false);
    }
};

// scene implementation
var scene = {
    theAppObject:null,
    appAreaDiv: null,
    isAppAreaVisible: false,
    redButtonDiv: null,
    apps: null,
    liSelected: null,
    index: null,
    initialize: function(appObj) {
        this.theAppObject = appObj;
        this.appAreaDiv = document.getElementById('app_area');
        rcUtils.registerKeyEventListener();

        this.apps = document.getElementById('applist');
        this.index = -1;

        this.render();
    },
    render: function() {
    },
};

// RC button press handler function
function handleKeyCode(kc) {
    try {
        var shouldRender = true;
        var len = scene.apps.getElementsByTagName('li').length-1;
        var next;

        // process buttons
        switch (kc) {
            case VK_RED:
                // load TV Station Application
                window.cefRedButton(window.HBBTV_POLYFILL_NS.currentChannel.channelId);
                break;
            case VK_DOWN:
                scene.index++;

                if (scene.liSelected) {
                    scene.liSelected.classList.remove('selected');
                    next = scene.apps.getElementsByTagName('li')[scene.index];

                    if(typeof next !== undefined && scene.index <= len) {
                        scene.liSelected = next;
                    }
                    else {
                        scene.index = 0;
                        scene.liSelected = scene.apps.getElementsByTagName('li')[0];
                    }
                }
                else {
                    scene.index = 0;
                    scene.liSelected = scene.apps.getElementsByTagName('li')[0];
                }
                scene.liSelected.classList.add('selected');
                break;
            case VK_UP:
                if (scene.liSelected) {
                    scene.liSelected.classList.remove('selected');
                    scene.index--;
                    next = scene.apps.getElementsByTagName('li')[scene.index];

                    if(typeof next !== undefined && scene.index >= 0) {
                        scene.liSelected = next;
                    }
                    else {
                        scene.index = len;
                        scene.liSelected = scene.apps.getElementsByTagName('li')[len];
                    }
                }
                else {
                    scene.index = 0;
                    scene.liSelected = scene.apps.getElementsByTagName('li')[len];
                }
                scene.liSelected.classList.add('selected');
                break;
            case VK_ENTER:
                let url = scene.liSelected.getAttribute('data-url');
                window.cefLoadUrl(url);
                break;
            default:
                // pressed unhandled key
                shouldRender = false;
        }
        if (shouldRender) {
            // render scene
            scene.render();
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
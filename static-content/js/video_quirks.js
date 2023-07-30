function _quirk_body_background(value) {
    document.body.style.backgroundColor = value;
}

function  _quirk_style_id(id, key, value) {
    let el = document.querySelectorAll('#' + id);
    if (typeof el !== 'undefined' && el != null && el.length > 0) {
        for (let i = 0; i < el.length; ++i) {
            el[i].style[key] = value;
        }
    }
}

function _quirk_save_set(name, id, key, value) {
    let el = document.querySelectorAll('#' + id);
    if (typeof el !== 'undefined' && el != null && el.length > 0) {
        for (let i = 0; i < el.length; ++i) {
            document[name + '_' + i] = el[i].style[key];
            el[i].style[key] = value;
        }
    }
}

function _quirk_restore_set(name, id, key) {
    let el = document.querySelectorAll('#' + id);
    if (typeof el !== 'undefined' && el != null && el.length > 0) {
        for (let i = 0; i < el.length; ++i) {
            el[i].style[key] = document[name + '_' + i];
        }
    }
}

function activate_quirks(isStart) {
    // Arte.tv
    if (document.location.href.search("www.arte.tv") > 0) {
        if (isStart) {
            _quirk_style_id('player', 'visibility', 'hidden');
        } else {
            _quirk_style_id('player', 'visibility', 'visible');
        }
    } else if (document.location.href.search("hbbtv.redbutton.de/extern/redorbit/hbbtv/apps/") > 0) {
        if (isStart) {
            _quirk_style_id('contentHub-home', 'visibility', 'hidden');
            _quirk_style_id('stage', 'visibility', 'hidden');
            _quirk_style_id('screen', 'background', 'transparent');
            _quirk_body_background('transparent');

            /*
            _quirk_style_id('content', 'visibility', 'hidden');
            _quirk_style_id('footer', 'visibility', 'hidden');
            _quirk_body_background('transparent');
            _quirk_save_set('HBBTV_SAVE_CONTENT_HUB_HOME', 'contentHub-home', 'background', 'transparent');
            _quirk_save_set('HBBTV_SAVE_SCREEN', 'screen', 'background', 'transparent');
             */
        } else {
            _quirk_style_id('contentHub-home', 'visibility', 'visible');
            _quirk_style_id('stage', 'visibility', 'visible');
            _quirk_style_id('screen', 'background', '#efefef');
            _quirk_body_background('#2c2c2c');

            /*
            _quirk_style_id('content', 'visibility', 'visible');
            _quirk_style_id('footer', 'visibility', 'visible');
            _quirk_body_background('#2c2c2c');
            _quirk_restore_set('HBBTV_SAVE_CONTENT_HUB_HOME', 'contentHub-home', 'background');
            _quirk_restore_set('HBBTV_SAVE_SCREEN', 'screen', 'background');
            */
        }
    }
}

window.start_video_quirk = function() {
    activate_quirks(true);
};

window.stop_video_quirk = function() {
    activate_quirks(false);
};
function _quirk_add_class(id, tagName, classname) {
    let sel;
    if (id !== null) {
        sel = '#' + id;
    } else {
        sel = tagName;
    }

    el = document.querySelectorAll(sel);

    if (typeof el !== 'undefined' && el != null && el.length > 0) {
        for (let i = 0; i < el.length; ++i) {
            el[i].classList.add(classname);
        }
    }
}

function _quirk_remove_class(id, tagName, classname) {
    let sel;
    if (id !== null) {
        sel = '#' + id;
    } else {
        sel = tagName;
    }

    el = document.querySelectorAll(sel);

    if (typeof el !== 'undefined' && el != null && el.length > 0) {
        for (let i = 0; i < el.length; ++i) {
            el[i].classList.remove(classname);
        }
    }
}

function _quirk_hide_element(id, tagName) {
    _quirk_add_class(id, tagName, 'quirk_hide_element');}

function _quirk_unhide_element(id, tagName) {
    _quirk_remove_class(id, tagName, 'quirk_hide_element');
}

function activate_quirks(isStart) {
    // Arte.tv
    if (document.location.href.search("www.arte.tv") > 0) {
        if (isStart) {
            _quirk_hide_element('player', null);
        } else {
            _quirk_unhide_element('player', null);
        }
    } else if (document.location.href.search("hbbtv.redbutton.de/extern/redorbit/hbbtv/apps/") > 0) {
        if (isStart) {
            _quirk_hide_element('contentHub-home', null);
            _quirk_hide_element('stage', null);
            _quirk_hide_element('slide-wrapper', null);

            _quirk_add_class('screen', null, 'quirk_background_transparent');
            _quirk_add_class(null, 'body', 'quirk_background_transparent');
        } else {
            _quirk_unhide_element('contentHub-home', null);
            _quirk_unhide_element('stage', null);
            _quirk_unhide_element('slide-wrapper', null);

            _quirk_remove_class('screen', null, 'quirk_background_color1');
            _quirk_remove_class(null, 'body', 'quirk_background_color2');
        }
    }
}

window.start_video_quirk = function() {
    activate_quirks(true);
};

window.stop_video_quirk = function() {
    activate_quirks(false);
};
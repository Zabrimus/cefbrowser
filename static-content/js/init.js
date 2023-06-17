window._HBBTV_APPURL_ = new Map();

window.HBBTV_POLYFILL_NS = window.HBBTV_POLYFILL_NS || {
};

window.HBBTV_POLYFILL_NS = {
    ...window.HBBTV_POLYFILL_NS, ...{
        keysetSetValueCount: 0,
        streamEventListeners: [],
    }
};
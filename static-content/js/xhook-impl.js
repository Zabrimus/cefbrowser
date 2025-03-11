/*
TODO: This needs to be fixed.

xhook.after(function (request, response) {
    if (response.finalUrl.match(/hbbtv\.zdf\.de\/zdfm3\/dyn\/get\.php/)) {
        try {
            let jsonObject = JSON.parse(response.text);
            if (jsonObject['fsk']['age'] !== undefined) {
                delete jsonObject.fsk;
                response.text = JSON.stringify(jsonObject);
            }
        } catch (err) {
            // ignore this error
        }
    }
});
*/
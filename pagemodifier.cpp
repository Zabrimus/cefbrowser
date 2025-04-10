#include "tools.h"
#include "pagemodifier.h"

void PageModifier::init(std::string sp) {
    if (!cache.empty()) {
        // already filled
        return;
    }

    staticPath = std::move(sp);

    // -----------------------------------------
    // PreJavascript (Fehlt: _dynamic.js)
    // -----------------------------------------
    std::string preJs[] = {"simple-tinyduration.js", "mutation-summary.js", "init.js", "keyhandler.js" };

    std::string preJsResult;
    for (const auto & file : preJs) {
        preJsResult += readFile((staticPath + "/js/" + file).c_str());
    }

    cache.emplace("PRE_JS", std::string("\n<script type=\"text/javascript\">\n//<![CDATA[\n") + preJsResult + "\n// ]]>\n</script>\n");

    // -----------------------------------------
    // PreCSS
    // -----------------------------------------
    std::string preCss[] = { "TiresiasPCfont.css", "videoquirks.css" };

    std::string preCssResult;
    for (const auto & file : preCss) {
        preCssResult += readFile((staticPath + "/css/" + file).c_str());
    }

    cache.emplace("PRE_CSS", std::string("\n<style>\n") + preCssResult + "\n</style>\n");

    // -----------------------------------------
    // PostJavascript (fehlt: _zoom_level.js)
    // -----------------------------------------
    std::string postJs[] = { "video_quirks.js", "videoobserver.js", "hbbtv.js", "initlast.js" };

    std::string postJsResult;
    for (const auto & file : postJs) {
        postJsResult += readFile((staticPath + "/js/" + file).c_str());
    }

    cache.emplace("POST_JS", std::string("\n<script type=\"text/javascript\">\n//<![CDATA[\n") + postJsResult + "\n// ]]>\n</script>\n");
}

std::string PageModifier::insertAfterHead(std::string &origStr, std::string &addStr) {
    std::size_t found = origStr.find("<head");
    if (found == std::string::npos) {
        return origStr;
    }

    found = origStr.find(">", found + 1, 1);
    if (found == std::string::npos) {
        return origStr;
    }

    return origStr.substr(0, found + 1) + addStr + origStr.substr(found + 1);
}

std::string PageModifier::insertBeforeEnd(std::string &origStr, std::string &addStr) {
    std::size_t found = origStr.find("</body>");
    if (found == std::string::npos) {
        return origStr;
    }

    return origStr.substr(0, found) + addStr + origStr.substr(found);
}

std::string PageModifier::getDynamicJs() {
    std::string dynamicJs = readFile((staticPath + "/js/_dynamic.js").c_str());
    return std::string("\n<script type=\"text/javascript\">\n//<![CDATA[\n") + dynamicJs + "\n// ]]>\n</script>\n";
}

std::string PageModifier::getDynamicZoom() {
    std::string dynamicJs = readFile((staticPath + "/js/_zoom_level.js").c_str());
    return std::string("\n<script type=\"text/javascript\">\n//<![CDATA[\n") + dynamicJs + "\n// ]]>\n</script>\n";
}

std::string PageModifier::injectAll(std::string &source) {
    std::string result;

    result = insertAfterHead(source, cache["PRE_JS"]);

    std::string dyn1 = getDynamicJs();
    result = insertAfterHead(result, dyn1);

    result = insertBeforeEnd(result, cache["POST_JS"]);

    std::string dyn2 = getDynamicZoom();
    result = insertBeforeEnd(result, dyn2);

    result = insertAfterHead(result, cache["PRE_CSS"]);

    TRACE("Inject all:\n{}", result);

    return result;
}
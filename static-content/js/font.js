if (document && document.body) {
    if (window === window.top) {
        document.body.style["font-family"] = "Tiresias";
    }
} else {
    document.addEventListener("DOMContentLoaded", () => {
        document.body.style["font-family"] = "Tiresias";
    });
}
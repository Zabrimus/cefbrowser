var MyCar = (function () {
    'use strict';

    let mainMenuActive;
    let MainSlider;
    let SubSlider;

    let createDiv = function (title) {
        let d = document.createElement('div');
        d.innerText = title;

        return d;
    }

    let setSelected = function (search, classSelected, classUnselected, none = false){
        let children = document.querySelector(search).children;
        for (let i = (none ? 0 : 1); i < children.length; i++) {
            children[i].classList.remove(classSelected);
            children[i].classList.add(classUnselected);
        }

        if (!none) {
            children[0].classList.remove(classUnselected);
            children[0].classList.add(classSelected);
        }
    }

    let fillSubMenu = function (index) {
        if (SubSlider !== undefined) {
            SubSlider.destroy()
        }

        let m3uCatalogue = window.HBBTV_POLYFILL_NS.paramBody;

        let mc = document.querySelector('.sub-glide');
        mc.innerHTML = "";

        let submenu = m3uCatalogue.menus[0].items[index].submenu;
        let submenuItems = m3uCatalogue.menus[submenu].items;

        submenuItems.forEach(function (menu) {
            let d = createDiv("");

            /*
            <figure>
                <img src='image.jpg' alt='missing' />
                <figcaption>Caption goes here</figcaption>
            </figure>
            */

            let figure = document.createElement('figure');
            figure.setAttribute('style', 'margin-inline-start: 0px; margin-inline-end: 0px');

            let te = document.createElement('img');
            te.setAttribute('src', menu.img);
            te.setAttribute('alt', menu.title);
            te.setAttribute('data', menu.url);
            te.setAttribute('style', 'width: 200px; height: 200px; object-fit: contain;');
            figure.appendChild(te);

            let figureCaption = document.createElement('figcaption');
            figureCaption.setAttribute('style', 'background-color: black;');
            figureCaption.innerText = menu.title;
            figure.appendChild(figureCaption);

            d.appendChild(figure);

            mc.append(d);
        })

        SubSlider = new BlazeSlider(document.querySelector('.sub-slider'), {
            all: {
                slidesToShow: Math.min(5, submenuItems.length),
                transitionDuration: 200
            }
        });

        setSelected(".sub-glide", "sub-selected", "sub-unselected", true);

        const unsubscribe = SubSlider.onSlide((pageIndex, firstSlideIndex, lastSlideIndex) => {
            setSelected(".sub-glide", "sub-selected", "sub-unselected", false);
        })
    }

    class MyCar {
        initMainMenu() {
            let m3uCatalogue = window.HBBTV_POLYFILL_NS.paramBody;
            let mc = document.querySelector('.main-glide');

            m3uCatalogue.menus[0].items.forEach(function (menu) {
                let div = createDiv(menu.title);
                div.setAttribute('style', 'width:auto;');
                mc.append(div);
            });

            MainSlider = new BlazeSlider(document.querySelector('.main-slider'), {
                all: {
                    slidesToShow: 5,
                    transitionDuration: 200
                }
            })

            setSelected(".main-glide", "main-selected", "main-unselected", false);
            fillSubMenu(0);
            mainMenuActive = true;

            const unsubscribe = MainSlider.onSlide((pageIndex, firstSlideIndex, lastSlideIndex) => {
                fillSubMenu(pageIndex);
                setSelected(".main-glide", "main-selected", "main-unselected", false);
            })
        }

        left() {
            mainMenuActive ? MainSlider.prev() : SubSlider.prev();
        }

        right() {
            mainMenuActive ? MainSlider.next() : SubSlider.next();
        }

        down() {
            if (mainMenuActive) {
                setSelected(".sub-glide", "sub-selected", "sub-unselected", false);
            }

            mainMenuActive = false;
        }

        up() {
            if (!mainMenuActive) {
                setSelected(".sub-glide", "sub-selected", "sub-unselected", true);
            }

            mainMenuActive = true;
        }

        getSelectedUrl() {
            return document.querySelector('.sub-selected > img').getAttribute('data');
        }
    }

    return MyCar;
})();

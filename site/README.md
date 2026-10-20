# Ostrakon homepage

A static page: plain HTML, CSS and JavaScript, no build step and no
dependencies. Upload the contents of this folder to any web server.

* `index.html`, `css/site.css`, `js/site.js`
* `img/`: copies of the icon and screenshots from `store/`

The only outside requests are made by the browser to the GitHub API, for
the star count and the latest release (the download button points to the
`.pbw` of the newest release once there is one).

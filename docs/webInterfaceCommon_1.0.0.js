/* Shared theme and page-script loader for the three Glow Worm firmware pages. */
(function () {
    var pageScripts = {
        '/': 'webInterface_1.6.4.slim.min.js',
        '/setsettings': 'webInterfaceSettings_1.6.5.slim.min.js',
        '/setldr': 'webInterfaceLdr_1.4.2.slim.min.js'
    };
    var path = window.location.pathname.replace(/\/+$/, '') || '/';
    var pageScript = pageScripts[path];
    if (!pageScript) return;

    var commonScriptUrl = document.currentScript.src;
    var cookieName = 'luciferin_theme';
    var systemTheme = window.matchMedia('(prefers-color-scheme: dark)');
    var savedCookie = document.cookie.split(';').map(function (entry) {
        return entry.trim();
    }).find(function (entry) {
        return entry.indexOf(cookieName + '=') === 0;
    });
    var preference = savedCookie ? savedCookie.substring(cookieName.length + 1) : null;
    if (preference !== 'dark' && preference !== 'light') preference = null;

    function applyTheme() {
        var dark = preference ? preference === 'dark' : systemTheme.matches;
        document.documentElement.setAttribute('data-luciferin-theme', dark ? 'dark' : 'light');
        var button = document.getElementById('themeToggle');
        if (button) {
            button.querySelector('i').className = dark ? 'fa-solid fa-sun' : 'fa-solid fa-moon';
            button.setAttribute('aria-label', dark ? 'Switch to light theme' : 'Switch to dark theme');
            button.setAttribute('title', dark ? 'Switch to light theme' : 'Switch to dark theme');
            button.setAttribute('aria-pressed', String(dark));
        }
    }

    function moveHeaderButtons(shell) {
        document.querySelectorAll('body > .container .btn.back-btn, body > .container .btn.right-btn').forEach(function (button) {
            shell.appendChild(button);
        });
    }

    function addThemeButton() {
        var header = document.querySelector('body > .container-fluid.sticky-top');
        if (!header || document.getElementById('themeToggle')) return;

        var shell = document.createElement('div');
        shell.className = 'luciferin-header-shell sticky-top';
        header.parentNode.insertBefore(shell, header);
        header.classList.remove('sticky-top');
        shell.appendChild(header);

        var button = document.createElement('button');
        button.id = 'themeToggle';
        button.type = 'button';
        button.innerHTML = '<i class="fa-solid fa-moon" aria-hidden="true"></i>';
        button.addEventListener('click', function () {
            preference = document.documentElement.getAttribute('data-luciferin-theme') === 'dark' ? 'light' : 'dark';
            document.cookie = cookieName + '=' + preference + '; Path=/; Max-Age=31536000; SameSite=Lax';
            applyTheme();
        });
        shell.appendChild(button);
        moveHeaderButtons(shell);
        applyTheme();
    }

    applyTheme();
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', addThemeButton);
    } else {
        addThemeButton();
    }

    function showChristmasSnow() {
        var now = new Date();
        var month = now.getMonth();
        var day = now.getDate();
        if (!((month === 11 && day >= 14) || (month === 0 && day <= 6)) ||
            window.matchMedia('(prefers-reduced-motion: reduce)').matches ||
            document.getElementById('snow')) {
            return;
        }

        var snow = document.createElement('div');
        snow.id = 'snow';
        snow.setAttribute('aria-hidden', 'true');
        document.body.appendChild(snow);

        var flakesCount = window.innerWidth >= 1200 ? 40 :
            window.innerWidth >= 992 ? 30 :
                window.innerWidth >= 768 ? 24 : 16;
        var svgNamespace = 'http://www.w3.org/2000/svg';
        var arms = [
            'M12 12V2 M12 5L9.5 3 M12 5L14.5 3',
            'M12 12V2 M12 8L9.5 6 M12 8L14.5 6 M12 5L10 3.5 M12 5L14 3.5',
            'M12 12V2 M12 6L9 3.5 M12 6L15 3.5 M12 3.5L10.5 2.5 M12 3.5L13.5 2.5'
        ];
        for (var i = 0; i < flakesCount; i++) {
            var flake = document.createElement('div');
            var depth = Math.random();
            var layer = depth < 0.45 ? 'far' : depth < 0.85 ? 'middle' : 'near';
            var duration = layer === 'far' ? 18 + Math.random() * 9 :
                layer === 'middle' ? 12 + Math.random() * 7 : 8 + Math.random() * 5;
            var size = layer === 'far' ? 9 + Math.random() * 7 :
                layer === 'middle' ? 15 + Math.random() * 8 : 23 + Math.random() * 9;
            flake.className = 'snowflake snowflake--' + layer;
            flake.style.left = Math.random() * 100 + 'vw';
            flake.style.opacity = (layer === 'far' ? 0.3 : layer === 'middle' ? 0.55 : 0.75) + Math.random() * 0.2;
            flake.style.setProperty('--drift', Math.random() * 160 - 80 + 'px');
            flake.style.setProperty('--turn', Math.random() * 120 - 60 + 'deg');
            flake.style.animationDuration = duration + 's';
            flake.style.animationDelay = -Math.random() * duration + 's';

            var shape = document.createElementNS(svgNamespace, 'svg');
            shape.setAttribute('class', 'snowflake-shape');
            shape.setAttribute('viewBox', '0 0 24 24');
            shape.style.width = size + 'px';
            shape.style.height = size + 'px';
            shape.style.setProperty('--sway', 5 + Math.random() * 13 + 'px');
            shape.style.animationDuration = 2 + Math.random() * 3 + 's';
            shape.style.animationDelay = -Math.random() * 5 + 's';
            var arm = arms[Math.floor(Math.random() * arms.length)];
            for (var spoke = 0; spoke < 6; spoke++) {
                var path = document.createElementNS(svgNamespace, 'path');
                path.setAttribute('d', arm);
                path.setAttribute('transform', 'rotate(' + spoke * 60 + ' 12 12)');
                shape.appendChild(path);
            }
            flake.appendChild(shape);
            snow.appendChild(flake);
        }
    }

    showChristmasSnow();

    function followSystemTheme() {
        if (!preference) applyTheme();
    }
    if (systemTheme.addEventListener) {
        systemTheme.addEventListener('change', followSystemTheme);
    } else {
        systemTheme.addListener(followSystemTheme);
    }

    // Keep Save state inactive until the home page's asCBAction function is ready.
    var saveButton = path === '/' ? document.getElementById('autosave') : null;
    var pageContainer = document.querySelector('body > .container');
    if (saveButton) saveButton.disabled = true;
    function revealPage() {
        var shell = document.querySelector('.luciferin-header-shell');
        if (shell) moveHeaderButtons(shell);
        if (pageContainer) pageContainer.classList.add('page-ready');
    }
    var script = document.createElement('script');
    script.src = new URL(pageScript, commonScriptUrl).href;
    script.async = false;
    script.onload = function () {
        if (saveButton) saveButton.disabled = false;
        revealPage();
    };
    script.onerror = function () {
        revealPage();
        console.error('Unable to load ' + pageScript);
    };
    document.head.appendChild(script);
})();

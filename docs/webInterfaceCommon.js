/* Shared theme and page-script loader for the three Glow Worm firmware pages. */
(function () {
    var pageScripts = {
        '/': 'webInterface_1.6.3.slim.min.js',
        '/setsettings': 'webInterfaceSettings_1.6.4.slim.min.js',
        '/setldr': 'webInterfaceLdr_1.4.1.slim.min.js'
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
        applyTheme();
    }

    applyTheme();
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', addThemeButton);
    } else {
        addThemeButton();
    }

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

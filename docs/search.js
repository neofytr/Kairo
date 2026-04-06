/* ===== Kairo Docs - Client-side Search ===== */
(function () {
    'use strict';

    let debounceTimer = null;
    const DEBOUNCE_MS = 150;

    function init() {
        const input = document.getElementById('search-input');
        if (!input) return;

        input.addEventListener('input', function () {
            clearTimeout(debounceTimer);
            debounceTimer = setTimeout(function () {
                performSearch(input.value.trim());
            }, DEBOUNCE_MS);
        });

        // "/" focuses search
        document.addEventListener('keydown', function (e) {
            if (e.key === '/' && document.activeElement !== input &&
                document.activeElement.tagName !== 'INPUT' &&
                document.activeElement.tagName !== 'TEXTAREA') {
                e.preventDefault();
                input.focus();
            }
            if (e.key === 'Escape' && document.activeElement === input) {
                input.value = '';
                performSearch('');
                input.blur();
            }
        });
    }

    function performSearch(query) {
        const sidebarLinks = document.querySelectorAll('.sidebar a[data-section]');
        const sections = document.querySelectorAll('.api-section');

        // Clear previous highlights
        clearHighlights();

        if (!query) {
            // Show everything
            sidebarLinks.forEach(function (link) {
                link.style.display = '';
                link.innerHTML = link.textContent;
            });
            sections.forEach(function (sec) {
                sec.style.display = '';
            });
            return;
        }

        var lowerQuery = query.toLowerCase();

        sidebarLinks.forEach(function (link) {
            var sectionId = link.getAttribute('data-section');
            var section = sectionId ? document.getElementById(sectionId) : null;
            var linkText = link.textContent;
            var linkMatch = linkText.toLowerCase().indexOf(lowerQuery) !== -1;
            var contentMatch = false;

            if (section) {
                var textElements = section.querySelectorAll('h2, h3, h4, p, li, code');
                for (var i = 0; i < textElements.length; i++) {
                    if (textElements[i].textContent.toLowerCase().indexOf(lowerQuery) !== -1) {
                        contentMatch = true;
                        break;
                    }
                }
            }

            var isMatch = linkMatch || contentMatch;

            link.style.display = isMatch ? '' : 'none';

            if (linkMatch) {
                link.innerHTML = highlightText(linkText, query);
            } else {
                link.innerHTML = linkText;
            }

            if (section) {
                section.style.display = isMatch ? '' : 'none';
            }
        });

        // Also hide sidebar section titles if all their links are hidden
        var sectionTitles = document.querySelectorAll('.sidebar-section-title');
        sectionTitles.forEach(function (title) {
            var next = title.nextElementSibling;
            var anyVisible = false;
            while (next && !next.classList.contains('sidebar-section-title')) {
                if (next.tagName === 'A' && next.style.display !== 'none') {
                    anyVisible = true;
                }
                next = next.nextElementSibling;
            }
            title.style.display = anyVisible ? '' : 'none';
        });
    }

    function highlightText(text, query) {
        if (!query) return text;
        var escaped = query.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
        var regex = new RegExp('(' + escaped + ')', 'gi');
        return text.replace(regex, '<span class="search-highlight">$1</span>');
    }

    function clearHighlights() {
        var highlighted = document.querySelectorAll('.search-highlight');
        highlighted.forEach(function (el) {
            var parent = el.parentNode;
            parent.replaceChild(document.createTextNode(el.textContent), el);
            parent.normalize();
        });
    }

    // Sidebar hamburger toggle
    function initHamburger() {
        var btn = document.getElementById('hamburger-btn');
        var sidebar = document.querySelector('.sidebar');
        var overlay = document.querySelector('.sidebar-overlay');
        if (!btn || !sidebar) return;

        btn.addEventListener('click', function () {
            sidebar.classList.toggle('open');
            if (overlay) overlay.classList.toggle('open');
        });

        if (overlay) {
            overlay.addEventListener('click', function () {
                sidebar.classList.remove('open');
                overlay.classList.remove('open');
            });
        }

        // Close sidebar when a link is clicked (mobile)
        sidebar.querySelectorAll('a').forEach(function (a) {
            a.addEventListener('click', function () {
                if (window.innerWidth <= 768) {
                    sidebar.classList.remove('open');
                    if (overlay) overlay.classList.remove('open');
                }
            });
        });
    }

    // Active sidebar link tracking
    function initScrollSpy() {
        var sections = document.querySelectorAll('.api-section');
        var links = document.querySelectorAll('.sidebar a[data-section]');
        if (!sections.length || !links.length) return;

        var observer = new IntersectionObserver(function (entries) {
            entries.forEach(function (entry) {
                if (entry.isIntersecting) {
                    links.forEach(function (l) { l.classList.remove('active'); });
                    var activeLink = document.querySelector('.sidebar a[data-section="' + entry.target.id + '"]');
                    if (activeLink) activeLink.classList.add('active');
                }
            });
        }, { rootMargin: '-80px 0px -60% 0px', threshold: 0 });

        sections.forEach(function (sec) { observer.observe(sec); });
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', function () { init(); initHamburger(); initScrollSpy(); });
    } else {
        init(); initHamburger(); initScrollSpy();
    }
})();

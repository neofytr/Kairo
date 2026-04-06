/* ============================================================
   Kairo Docs — Client-side Search
   ============================================================ */
(function () {
  'use strict';

  let debounceTimer = null;
  const DEBOUNCE_MS = 150;

  /** Collect searchable sections from the page. */
  function buildIndex() {
    const sections = [];
    // Index from sidebar links + page headings
    const headings = document.querySelectorAll('.content-body h2[id], .content-body h3[id]');
    headings.forEach(function (h) {
      // Grab text of the heading and next ~200 chars of sibling text
      let text = h.textContent.replace(/#/g, '').trim();
      let body = '';
      let node = h.nextElementSibling;
      let chars = 0;
      while (node && chars < 400) {
        if (node.tagName === 'H2' || node.tagName === 'H3') break;
        body += ' ' + node.textContent;
        chars += node.textContent.length;
        node = node.nextElementSibling;
      }
      sections.push({
        id: h.id,
        title: text,
        body: body.toLowerCase(),
        level: h.tagName
      });
    });
    return sections;
  }

  function init() {
    const input = document.getElementById('search-input');
    const resultsBox = document.getElementById('search-results');
    if (!input || !resultsBox) return;

    let index = null;
    let focusIdx = -1;

    function getIndex() {
      if (!index) index = buildIndex();
      return index;
    }

    function search(query) {
      const q = query.toLowerCase().trim();
      if (!q) { hide(); return; }
      const idx = getIndex();
      const matches = idx.filter(function (s) {
        return s.title.toLowerCase().includes(q) || s.body.includes(q);
      }).slice(0, 15);

      if (matches.length === 0) {
        resultsBox.innerHTML = '<div class="search-results-empty">No results found</div>';
        resultsBox.classList.add('active');
        return;
      }

      resultsBox.innerHTML = matches.map(function (m, i) {
        var cls = i === 0 ? ' class="focused"' : '';
        return '<a href="#' + m.id + '"' + cls + '>' + escapeHtml(m.title) +
          (m.level === 'H3' ? '<span class="sr-section">subsection</span>' : '') + '</a>';
      }).join('');
      focusIdx = 0;
      resultsBox.classList.add('active');
    }

    function hide() {
      resultsBox.classList.remove('active');
      resultsBox.innerHTML = '';
      focusIdx = -1;
    }

    function escapeHtml(s) {
      return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
    }

    function moveFocus(dir) {
      var items = resultsBox.querySelectorAll('a');
      if (!items.length) return;
      items.forEach(function(a){ a.classList.remove('focused'); });
      focusIdx = (focusIdx + dir + items.length) % items.length;
      items[focusIdx].classList.add('focused');
      items[focusIdx].scrollIntoView({ block: 'nearest' });
    }

    input.addEventListener('input', function () {
      clearTimeout(debounceTimer);
      debounceTimer = setTimeout(function () { search(input.value); }, DEBOUNCE_MS);
    });

    input.addEventListener('keydown', function (e) {
      if (e.key === 'Escape') { hide(); input.blur(); }
      if (e.key === 'ArrowDown') { e.preventDefault(); moveFocus(1); }
      if (e.key === 'ArrowUp') { e.preventDefault(); moveFocus(-1); }
      if (e.key === 'Enter') {
        e.preventDefault();
        var focused = resultsBox.querySelector('a.focused');
        if (focused) { focused.click(); hide(); input.blur(); }
      }
    });

    resultsBox.addEventListener('click', function (e) {
      var link = e.target.closest('a');
      if (link) { hide(); input.value = ''; }
    });

    // Click outside to close
    document.addEventListener('click', function (e) {
      if (!input.contains(e.target) && !resultsBox.contains(e.target)) hide();
    });

    // "/" shortcut to focus search
    document.addEventListener('keydown', function (e) {
      if (e.key === '/' && document.activeElement !== input &&
          !e.ctrlKey && !e.metaKey && !e.altKey &&
          document.activeElement.tagName !== 'INPUT' && document.activeElement.tagName !== 'TEXTAREA') {
        e.preventDefault();
        input.focus();
      }
    });
  }

  /* --- Sidebar active state tracking on scroll --- */
  function initScrollSpy() {
    var headings = document.querySelectorAll('.content-body h2[id], .content-body h3[id]');
    var navLinks = document.querySelectorAll('.nav-items a[href^="#"]');
    var tocLinks = document.querySelectorAll('.toc-rail a[href^="#"]');
    if (!headings.length) return;

    var observer = new IntersectionObserver(function (entries) {
      entries.forEach(function (entry) {
        if (entry.isIntersecting) {
          var id = entry.target.id;
          // Update sidebar
          navLinks.forEach(function (a) {
            a.classList.toggle('active', a.getAttribute('href') === '#' + id);
          });
          // Update toc rail
          tocLinks.forEach(function (a) {
            a.classList.toggle('active', a.getAttribute('href') === '#' + id);
          });
        }
      });
    }, { rootMargin: '-80px 0px -60% 0px', threshold: 0 });

    headings.forEach(function (h) { observer.observe(h); });
  }

  /* --- Mobile hamburger --- */
  function initMobile() {
    var btn = document.getElementById('hamburger-btn');
    var sidebar = document.querySelector('.sidebar');
    var overlay = document.querySelector('.sidebar-overlay');
    if (!btn || !sidebar) return;

    function toggle() {
      sidebar.classList.toggle('open');
      if (overlay) overlay.classList.toggle('active');
    }
    btn.addEventListener('click', toggle);
    if (overlay) overlay.addEventListener('click', toggle);

    // Close sidebar on nav click (mobile)
    sidebar.querySelectorAll('a').forEach(function (a) {
      a.addEventListener('click', function () {
        if (window.innerWidth <= 860) {
          sidebar.classList.remove('open');
          if (overlay) overlay.classList.remove('active');
        }
      });
    });
  }

  /* --- Collapsible nav groups --- */
  function initNavGroups() {
    document.querySelectorAll('.nav-group-title').forEach(function (title) {
      title.addEventListener('click', function () {
        title.parentElement.classList.toggle('collapsed');
      });
    });
  }

  /* --- Copy buttons on code blocks --- */
  function initCopyButtons() {
    document.querySelectorAll('.code-card .copy-btn').forEach(function (btn) {
      btn.addEventListener('click', function () {
        var code = btn.closest('.code-card').querySelector('code');
        if (!code) return;
        navigator.clipboard.writeText(code.textContent).then(function () {
          btn.textContent = 'Copied!';
          btn.classList.add('copied');
          setTimeout(function () {
            btn.textContent = 'Copy';
            btn.classList.remove('copied');
          }, 1500);
        });
      });
    });
  }

  /* --- Init on DOM ready --- */
  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', onReady);
  } else {
    onReady();
  }

  function onReady() {
    init();
    initScrollSpy();
    initMobile();
    initNavGroups();
    initCopyButtons();
  }
})();

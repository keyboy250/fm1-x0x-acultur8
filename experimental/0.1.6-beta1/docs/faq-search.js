/* Offline full-document search. Queries are never interpreted as HTML. */
(() => {
  const panel = document.querySelector('.faq-search');
  const input = document.getElementById('faq-query');
  const results = document.getElementById('search-results');
  const status = document.getElementById('search-status');
  const guide = document.getElementById('full-illustrated-guide');
  const normalize = value => value.normalize('NFKD').replace(/[\u0300-\u036f]/g, '').toLowerCase();
  const headings = Array.from(document.querySelectorAll('main h2, main h3'));
  const entries = headings.map((heading, index) => {
    if (!heading.id) heading.id = 'faq-topic-' + index;
    const range = document.createRange();
    range.setStartAfter(heading);
    if (headings[index + 1]) range.setEndBefore(headings[index + 1]);
    else range.setEndAfter(heading.closest('main').lastChild);
    const body = range.toString().replace(/\s+/g, ' ').trim();
    return {heading, title: heading.textContent.trim(), body,
      haystack: normalize(heading.textContent + ' ' + body)};
  });
  const search = () => {
    const words = normalize(input.value.trim()).split(/\s+/).filter(Boolean);
    results.replaceChildren();
    if (!words.length) {
      status.textContent = 'Search current release guidance and the complete illustrated history.';
      return;
    }
    const matches = entries.filter(entry => words.every(word => entry.haystack.includes(word)));
    status.textContent = matches.length ? matches.length + ' matching topics. Select a result to read it in context.' : 'No matching topics. Try fewer words or another term.';
    matches.forEach(entry => {
      const item = document.createElement('li');
      const link = document.createElement('a');
      link.href = '#' + entry.heading.id;
      link.textContent = entry.title + (guide.contains(entry.heading) ? ' (illustrated history)' : ' (current release)');
      link.addEventListener('click', () => {
        if (guide.contains(entry.heading)) guide.open = true;
        entry.heading.tabIndex = -1;
        entry.heading.focus({preventScroll:true});
      });
      const excerpt = document.createElement('p');
      const first = Math.max(0, normalize(entry.body).indexOf(words[0]) - 65);
      excerpt.textContent = (first ? '…' : '') + entry.body.slice(first, first + 230) + (entry.body.length > first + 230 ? '…' : '');
      item.append(link, excerpt); results.append(item);
    });
  };
  input.addEventListener('input', search);
  document.getElementById('faq-clear').addEventListener('click', () => {input.value='';search();input.focus();});
  const revealFragment = () => {
    const target = document.getElementById(location.hash.slice(1));
    if (target && guide.contains(target)) guide.open = true;
  };
  window.addEventListener('hashchange', revealFragment); revealFragment();
  panel.hidden = false;
})();

/* Browser UI. Assessment and report validation are shared with the Node tests. */
(function () {
  'use strict';
  const C = HardwareCatalog, E = HardwareEngine;
  const $ = id => document.getElementById(id);
  // Keep the storage prefix so existing browser runs can still be recovered.
  const storageKey = `rpnce-hardware:${C.revision}`;
  let run = E.makeRun(C), section = 'All', storageOK = true, acknowledged = false, recovery = null;
  let noticeTimer;
  function el(tag, content, className) {
    const node = document.createElement(tag);
    if (content !== undefined) node.textContent = content;
    if (className) node.className = className;
    return node;
  }
  function notify(message) {
    $('notice').textContent = message;
    $('notice').classList.add('visible');
    clearTimeout(noticeTimer);
    noticeTimer = setTimeout(() => $('notice').classList.remove('visible'), 6500);
  }
  function persist() {
    if (recovery !== null) {
      $('storage-state').textContent = 'An earlier saved run could not be resumed. Save previous data before starting a new run. Its stored data has been preserved; use Save report for new work.';
      return;
    }
    try { localStorage.setItem(storageKey, JSON.stringify(run)); storageOK = true; }
    catch (_) { storageOK = false; }
    $('storage-state').textContent = storageOK ? 'Saved in this browser. Save a report file before closing or changing browsers.' : 'Browser storage is unavailable. Use Save report to keep your work, including unfinished answers.';
  }
  try {
    let saved = localStorage.getItem(storageKey);
    if (!saved) {
      // Keep earlier catalog runs recoverable when a new revision is installed.
      const earlier = Object.keys(localStorage).filter(key => key.startsWith('rpnce-hardware:') && key !== storageKey)
        .map(key => {
          const data = localStorage.getItem(key);
          let created = '';
          try { created = JSON.parse(data)?.created || ''; } catch (_) { }
          return { data, created: typeof created === 'string' ? created : '' };
        }).sort((a, b) => b.created.localeCompare(a.created));
      saved = earlier[0]?.data;
    }
    if (saved) {
      try { run = E.validateRun(JSON.parse(saved), C); }
      catch (_) { recovery = saved; }
    }
  } catch (_) { storageOK = false; }

  const sections = [...new Set(C.cases.map(c => c.section))];
  const status = c => E.result(c, E.latest(run, c.id)).status;
  const selected = () => { const ids = new Set(run.selected); return C.cases.filter(c => ids.has(c.id)); };
  const locked = () => Object.values(run.records).some(items => items.length);
  function visible() {
    const query = $('search').value.trim().toLowerCase();
    const filter = $('status-filter').value;
    return selected().filter(c => (section === 'All' || c.section === section) &&
      (!query || `${c.title} ${c.id} ${c.covers.join(' ')} ${c.steps.join(' ')}`.toLowerCase().includes(query)) &&
      (filter === 'all' || (filter === 'known' ? c.knownIssue : filter === 'edge' ? ['boundary', 'domain', 'regression'].includes(c.focus) : filter === 'pending' ? ['pending', 'invalid', 'incomplete'].includes(status(c)) : status(c) === filter)));
  }
  function updateProgress() {
    const counts = E.totals(C, run);
    const done = counts.pass + counts.fail + counts.skipped;
    $('progress-title').textContent = `${done} of ${run.selected.length} recorded`;
    $('progress').max = Math.max(1, run.selected.length);
    $('progress').value = done;
    $('counts').replaceChildren();
    for (const [key, label] of [['pass', 'Passed'], ['fail', 'Failed'], ['skipped', 'Skipped'], ['pending', 'Remaining']]) {
      const cell = el('div', undefined, `count ${key}`);
      cell.append(el('strong', String(key === 'pending' ? counts.pending + counts.invalid + counts.incomplete : counts[key])), el('span', label));
      $('counts').append(cell);
    }
    $('run-label').textContent = `${run.metadata.build || 'BUILD NOT SET'} · ${run.metadata.version || C.version}`;
  }
  function renderMetadata() {
    const labels = { tester: 'Tester (optional)', device: 'Calculator model', os: 'TI-OS version', version: 'rpnCE version', build: 'Build / commit', notes: 'Run notes (optional)' };
    $('metadata').replaceChildren();
    for (const [key, title] of Object.entries(labels)) {
      const label = el('label', title);
      const input = el('input');
      input.id = `meta-${key}`;
      input.value = run.metadata[key];
      input.maxLength = 4000;
      input.disabled = locked();
      input.autocomplete = 'off';
      input.addEventListener('input', () => { run.metadata[key] = input.value; persist(); updateProgress(); });
      label.append(input); $('metadata').append(label);
    }
    $('identity-status').textContent = locked() ? 'Locked for this run' : 'Set these before recording';
    $('ack').checked = acknowledged;
    $('ack').disabled = locked();
  }
  function ready() {
    if (!['device', 'os', 'version', 'build'].every(key => run.metadata[key].trim())) {
      $('setup').open = true;
      notify('Enter the calculator model, TI-OS version, program version and build before recording.');
      $('meta-build').focus(); return false;
    }
    if (!acknowledged) { $('setup').open = true; $('ack').focus(); notify('Confirm that the stated build is installed and any needed values are saved.'); return false; }
    return true;
  }
  function renderSections() {
    $('sections').replaceChildren();
    for (const name of ['All', ...sections]) {
      const items = selected().filter(c => name === 'All' || c.section === name);
      if (!items.length && name !== 'All') continue;
      const button = el('button', undefined, 'section-button');
      button.type = 'button';
      button.append(el('span', name === 'All' ? 'All sections' : name), el('span', `${items.filter(c => status(c) !== 'pending').length}/${items.length}`));
      button.setAttribute('aria-current', String(section === name));
      button.addEventListener('click', () => { section = name; renderList(); });
      $('sections').append(button);
    }
  }
  function renderList() {
    renderSections(); updateProgress();
    const items = visible();
    if (!items.some(c => c.id === run.cursor)) run.cursor = items[0]?.id || null;
    $('test-list').replaceChildren();
    $('list-title').textContent = section === 'All' ? 'Cases' : section;
    $('list-count').textContent = items.length;
    for (const c of items) {
      const button = el('button', undefined, 'case-item');
      button.type = 'button'; button.dataset.id = c.id;
      button.setAttribute('aria-current', String(run.cursor === c.id));
      const top = el('small'); top.append(el('span', c.section), el('span', status(c).toUpperCase(), `status-${status(c)}`));
      button.append(top, el('span', c.title));
      button.addEventListener('click', () => { run.cursor = c.id; renderList(); persist(); });
      $('test-list').append(button);
    }
    if (!items.length) $('test-list').append(el('p', 'No cases match. Change the section, search or result filter.', 'empty'));
    renderCase();
  }
  function expectedDescription(x) {
    if (x.kind === 'number') return `${x.value} · relative tolerance ${x.relative || '0'}, absolute tolerance ${x.absolute || '0'}${x.errors?.length ? ' · alternatively: ' + x.errors.join(' or ') : ''}`;
    if (x.kind === 'error') return x.values.join(' or ');
    if (x.kind === 'text') return x.value;
    if (x.kind === 'range') return `${x.min} ≤ result < ${x.max}`;
    return 'Err: Underflow. The nonzero answer is below the supported range; the operands should be preserved.';
  }
  function renderCase() {
    const card = $('case'); card.replaceChildren();
    const c = C.cases.find(c => c.id === run.cursor);
    if (!c) { card.append(el('p', 'Choose another section or clear the filters to continue.', 'empty')); return; }
    const previous = E.latest(run, c.id);
    const draft = run.drafts[c.id] || previous || { answer: '', notes: '', observations: {}, locale: 'point' };
    const heading = el('div', undefined, 'case-heading');
    heading.append(el('p', `${c.section} / ${c.focus === 'normal' ? '' : c.focus.toUpperCase() + ' / '}${c.kind === 'navigation' ? 'MENU CHECK' : c.expected ? 'RESULT CHECK' : 'OBSERVATION'}`, 'eyebrow'), el('span', status(c).toUpperCase(), `badge ${status(c)}`));
    card.append(heading, el('h3', c.title), el('p', c.id, 'case-id'));
    if (c.knownIssue) card.append(el('p', 'Regression case for a known numerical problem. Record what this device actually does; this case still has to meet the stated expectation.', 'known-issue'));
    const steps = el('ol', undefined, 'steps'); c.steps.forEach(step => steps.append(el('li', step))); card.append(steps);
    const routeCommands = C.commands.filter(cmd => c.covers.includes(cmd.id) || c.steps.some(step => step.includes(cmd.path.join(' → '))));
    if (routeCommands.length) {
      const routes = el('details', undefined, 'routes'); routes.append(el('summary', 'Physical keys for the named selections'));
      for (const cmd of routeCommands) { const row = el('div', undefined, 'route'); row.append(el('strong', cmd.path.join(' → ')), el('code', cmd.route.join(' → '))); routes.append(row); }
      card.append(routes);
    }
    const expectation = el('details', undefined, 'expectation'); expectation.append(el('summary', 'Expected result & reference'));
    if (c.expected) expectation.append(el('p', expectedDescription(c.expected)));
    expectation.append(el('p', c.reference)); card.append(expectation);

    const form = el('form', undefined, 'answer-form');
    const answer = el('input', undefined, 'answer-input'); answer.id = 'answer'; answer.maxLength = 256; answer.autocomplete = 'off'; answer.spellcheck = false; answer.value = draft.answer;
    const locale = el('select'); locale.id = 'answer-locale';
    for (const [value, name] of [['point', 'Period decimal'], ['comma', 'Comma decimal']]) { const option = el('option', name); option.value = value; locale.append(option); }
    locale.value = draft.locale;
    if (c.expected) {
      const grid = el('div', undefined, 'answer-grid');
      const aLabel = el('label', c.expected.kind === 'error' ? 'Exact error message' : c.expected.kind === 'text' ? 'Raw object / text in X' : 'Result in X (use SHOW)'); aLabel.htmlFor = answer.id; aLabel.append(answer);
      const lLabel = el('label', 'Answer notation'); lLabel.htmlFor = locale.id; lLabel.append(locale);
      grid.append(aLabel, lLabel); form.append(grid);
    }
    const observations = {};
    if (c.checks.length) {
      const checks = el('fieldset', undefined, 'observations'); checks.append(el('legend', 'Check on the device'));
      c.checks.forEach((check, i) => {
        const item = el('div', undefined, 'observation'); const description = el('p', check); description.id = `check-${i}`; item.append(description);
        const choices = el('div', undefined, 'choices'); choices.setAttribute('role', 'group'); choices.setAttribute('aria-labelledby', description.id);
        observations[i] = draft.observations[i] || '';
        for (const [value, caption] of [['yes', 'Yes'], ['no', 'No']]) {
          const label = el('label'); const radio = el('input'); radio.type = 'radio'; radio.name = `observation-${i}`; radio.value = value; radio.checked = observations[i] === value;
          radio.addEventListener('change', () => { observations[i] = value; saveDraft(); });
          label.append(radio, el('span', caption)); choices.append(label);
        }
        item.append(choices); checks.append(item);
      }); form.append(checks);
    }
    const notesLabel = el('label', 'Notes / unexpected behavior', 'notes-label');
    const notes = el('textarea'); notes.id = 'case-notes'; notes.maxLength = 4000; notes.value = draft.notes; notes.placeholder = 'What happened? Include intermediate values or a skip reason.'; notesLabel.append(notes); form.append(notesLabel);
    function saveDraft() { run.drafts[c.id] = { answer: answer.value, notes: notes.value, locale: locale.value, observations: { ...observations } }; persist(); }
    [answer, notes, locale].forEach(input => input.addEventListener('input', saveDraft));
    const actions = el('div', undefined, 'actions');
    const record = el('button', 'Record result', 'primary'); record.type = 'submit'; record.id = 'record-result';
    const skip = el('button', 'Skip this case'); skip.type = 'button'; skip.id = 'skip-case';
    actions.append(record, skip); form.append(actions);
    const feedback = el('div', undefined, 'case-feedback'); feedback.id = 'case-feedback'; feedback.setAttribute('role', 'status'); feedback.hidden = !previous;
    function showFeedback(r) { feedback.hidden = false; feedback.className = `case-feedback ${r.status}`; feedback.textContent = `${r.status.toUpperCase()} · ${r.detail}`; }
    if (previous) showFeedback(E.result(c, previous)); form.append(feedback);
    function recordAttempt(skipped) {
      if (!ready()) return;
      const assessment = skipped ? { status: 'skipped', detail: notes.value || 'Skipped.' } : E.assess(c, answer.value, observations, locale.value);
      if (['invalid', 'incomplete'].includes(assessment.status)) { showFeedback(assessment); return; }
      const attempts = run.records[c.id] || [];
      if (attempts.length >= 100) { notify('This case already has 100 attempts. Save the report and start a new run.'); return; }
      attempts.push({ answer: answer.value, notes: notes.value, locale: locale.value, observations: { ...observations }, at: new Date().toISOString(), skipped });
      run.records[c.id] = attempts; delete run.drafts[c.id];
      persist(); renderMetadata(); $('setup').open = false; renderList();
      notify(assessment.status === 'pass' ? 'Result recorded as passed.' : assessment.status === 'fail' ? 'Result recorded as failed. Your answer and notes are preserved.' : 'Case skipped. It is not counted as passed.');
    }
    form.addEventListener('submit', event => { event.preventDefault(); recordAttempt(false); });
    skip.addEventListener('click', () => { if (!notes.value.trim()) { showFeedback({ status: 'incomplete', detail: 'Add a short reason in Notes before skipping.' }); notes.focus(); return; } recordAttempt(true); });
    card.append(form);
    const attempts = run.records[c.id] || [];
    if (attempts.length) {
      const history = el('details', undefined, 'history'); history.append(el('summary', `${attempts.length} recorded attempt${attempts.length === 1 ? '' : 's'} · latest result counts`));
      attempts.forEach(a => history.append(el('p', `${a.at} · ${E.result(c, a).status.toUpperCase()} · ${a.answer || '(observation)'}${a.notes ? ` · ${a.notes}` : ''}`))); card.append(history);
    }
    const navigation = el('div', undefined, 'case-navigation'); const items = visible(), index = items.findIndex(item => item.id === c.id);
    for (const [offset, caption] of [[-1, '← Previous'], [1, 'Next case →']]) {
      const button = el('button', caption); button.type = 'button'; button.disabled = !items[index + offset];
      button.addEventListener('click', () => { saveDraft(); run.cursor = items[index + offset].id; renderList(); persist(); }); navigation.append(button);
    }
    card.append(navigation);
  }

  function renderCoverage() {
    const available = C.commands.filter(cmd => cmd.available !== false);
    const functional = new Set(C.cases.filter(c => !['navigation', 'availability'].includes(c.focus)).flatMap(c => c.covers));
    const gaps = available.filter(cmd => !functional.has(cmd.id));
    const matrix = E.coverage(C);
    $('coverage-count').textContent = `${available.length - gaps.length}/${available.length} implemented commands have functional cases`;
    const body = $('coverage-body');
    body.append(el('p', `${C.groups.length} menu navigation cases; ${C.cases.filter(c => c.expected).length} result cases; ${C.cases.filter(c => c.kind === 'observation').length} observation cases. Catalog coverage describes available cases, not tests completed in this run.`), el('p', C.reference));
    body.append(el('p', `${matrix.length} menu and keyboard functions have explicit coverage requirements. ${matrix.filter(row => !row.missing.length).length} meet all their catalog requirements, including edge and invalid-input cases where required. ${C.commands.length - available.length} reserved complex-number commands are checked for availability only.`));
    const detail = el('details', undefined, 'function-coverage'); detail.append(el('summary', 'Per-function coverage in the catalog'));
    const table = el('table'); const head = el('thead'); const titles = el('tr');
    for (const caption of ['Function', 'Cases', 'Edges', 'Invalid', 'Regressions']) { const cell = el('th', caption); cell.scope = 'col'; titles.append(cell); }
    head.append(titles); table.append(head); const rows = el('tbody');
    for (const row of matrix) {
      const tr = el('tr'); const command = C.commands.find(cmd => cmd.id === row.id);
      const title = el('th', command ? command.path.slice(1).join(' → ') : 'Keyboard ' + row.id.slice(4)); title.scope = 'row'; tr.append(title);
      for (const key of ['functional', 'edge', 'domain', 'regression']) tr.append(el('td', String(row.planned[key])));
      rows.append(tr);
    }
    table.append(rows); detail.append(table); body.append(detail);
    const limits = el('ul'); C.gaps.forEach(gap => limits.append(el('li', gap))); body.append(limits);
    body.append(el('strong', `Functional gaps (${gaps.length} commands)`));
    const list = el('div', undefined, 'gap-list'); gaps.forEach(cmd => list.append(el('span', cmd.path.join(' → ')))); body.append(list);
    if (!gaps.length) body.append(el('p', 'Every command has at least one functional case. This does not cover every input, numeric boundary or interaction.'));
  }
  const options = [['full', 'All cases'], ...sections.map(s => [`section:${s}`, `${s} only`])];
  options.forEach(([value, caption]) => { const option = el('option', caption); option.value = value; $('scope').append(option); });
  function scopeIds(value) {
    return C.cases.filter(c => value === 'full' || c.section === value.slice(8)).map(c => c.id);
  }
  function syncScope() {
    const matches = ids => ids.length === run.selected.length && ids.every(id => run.selected.includes(id));
    const option = options.find(([value]) => matches(scopeIds(value)));
    $('scope').value = option?.[0] || 'full';
  }
  $('scope').addEventListener('change', () => {
    const value = $('scope').value;
    run.selected = scopeIds(value);
    section = 'All'; $('search').value = ''; $('status-filter').value = 'all'; renderList(); persist();
  });
  $('search').addEventListener('input', renderList);
  $('status-filter').addEventListener('change', renderList);
  $('ack').addEventListener('change', () => { acknowledged = $('ack').checked; });
  function download(contents, name) {
    const blob = new Blob([contents], { type: 'application/json' });
    const url = URL.createObjectURL(blob), anchor = el('a');
    anchor.href = url; anchor.download = name;
    document.body.append(anchor); anchor.click(); anchor.remove(); setTimeout(() => URL.revokeObjectURL(url), 30000);
  }
  if (recovery !== null) {
    const button = el('button', 'Save previous data'); button.type = 'button'; button.id = 'recover-run';
    button.addEventListener('click', () => { download(recovery, 'rpnCE-hardware-recovered.json'); notify('Previous data saved unchanged. Open it with its matching tool version, or keep it for developer review.'); });
    document.querySelector('.file-actions').append(button);
  }
  $('save').addEventListener('click', () => {
    const report = E.report(C, run);
    download(JSON.stringify(report, null, 2) + '\n', `rpnCE-hardware-${run.runId}.json`);
    notify('Report saved. It includes unfinished answers and attempt history. Add the file to the repo or share it with the developer for review.');
  });
  $('resume').addEventListener('change', async () => {
    const file = $('resume').files[0]; $('resume').value = ''; if (!file) return;
    try {
      if (file.size > E.MAX_FILE_BYTES) throw new Error('Report is too large (limit 8 MB).');
      const imported = E.validateRun(JSON.parse(await file.text()), C);
      if ((recovery !== null || locked() || Object.keys(run.drafts).length) && !window.confirm(recovery !== null ? 'Replace the earlier browser data with this run? Use Save previous data first if you need it.' : 'Replace this browser run with the saved run? Save the current report first if you need it.')) return;
      recovery = null; $('recover-run')?.remove();
      run = imported; acknowledged = locked(); section = 'All'; $('search').value = ''; $('status-filter').value = 'all';
      syncScope(); renderMetadata(); renderList(); persist(); notify('Saved run opened. Result labels were recalculated from this catalog.');
    } catch (error) { notify(`Could not open the run: ${error.message}`); }
  });
  $('new-run').addEventListener('click', () => {
    if (!window.confirm(recovery !== null ? 'Start a new run and replace the earlier browser data? Use Save previous data first if you need it.' : 'Start a new run? Save the current report first if you need it.')) return;
    recovery = null; $('recover-run')?.remove();
    run = E.makeRun(C); acknowledged = false; section = 'All'; $('search').value = ''; $('status-filter').value = 'all';
    syncScope(); renderMetadata(); renderList(); persist(); $('setup').open = true;
  });
  C.baseline.forEach(step => $('baseline').append(el('li', step)));
  $('version').textContent = `${C.version} · ${C.revision}`;
  acknowledged = locked(); syncScope(); renderMetadata(); renderCoverage(); renderList(); persist();
})();

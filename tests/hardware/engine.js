(function (root) {
  'use strict';
  const SCHEMA = 'rpnce-hardware-run/1';
  const MAX_FILE_BYTES = 8 * 1024 * 1024;
  function signature(catalog) {
    // A content identity prevents resuming against accidentally edited cases.
    let hash = 0xcbf29ce484222325n;
    for (const byte of new TextEncoder().encode(JSON.stringify([catalog.version, catalog.baseline, catalog.commands, catalog.cases]))) {
      hash = BigInt.asUintN(64, (hash ^ BigInt(byte)) * 0x100000001b3n);
    }
    return hash.toString(16).padStart(16, '0');
  }

  function decimal(text, locale = 'point') {
    if (typeof text !== 'string' || text.length > 256) throw new Error('Enter a number of at most 256 characters.');
    let s = text.trim().replace(/[−–]/g, '-').replace(/[ᴇｅ]/g, 'E');
    const parts = s.split(/[eE]/);
    if (parts.length > 2) throw new Error('Use one exponent, for example 1.25E-9.');
    const exponent = parts[1] === undefined ? '0' : parts[1];
    if (!/^[+-]?\d{1,4}$/.test(exponent) || Math.abs(Number(exponent)) > 1000) throw new Error('Exponent must be between -1000 and 1000.');
    const mark = locale === 'comma' ? ',' : '.';
    const group = locale === 'comma' ? '.' : ',';
    const escaped = mark === '.' ? '\\.' : ',';
    const grouped = group === '.' ? '\\.' : ',';
    const pattern = new RegExp('^[+-]?(?:(?:\\d+|\\d{1,3}(?:' + grouped + '\\d{3})+)(?:' + escaped + '\\d*)?|' + escaped + '\\d+)$');
    if (!pattern.test(parts[0])) throw new Error('Check the decimal mark and three-digit grouping. Do not include a unit.');
    s = parts[0].split(group).join('').replace(mark, '.');
    const fraction = (s.split('.')[1] || '').length;
    let n = BigInt(s.replace('.', ''));
    let e = Number(exponent) - fraction;
    if (n === 0n) return { n: 0n, e: 0 };
    while (n % 10n === 0n) { n /= 10n; e++; }
    return { n, e };
  }
  function align(a, b) {
    const e = Math.min(a.e, b.e);
    return [a.n * 10n ** BigInt(a.e - e), b.n * 10n ** BigInt(b.e - e), e];
  }
  function add(a, b) { const [x, y, e] = align(a, b); return { n: x + y, e }; }
  function abs(a) { return { n: a.n < 0n ? -a.n : a.n, e: a.e }; }
  function sub(a, b) { return add(a, { n: -b.n, e: b.e }); }
  function mul(a, b) { return { n: a.n * b.n, e: a.e + b.e }; }
  function compare(a, b) { const [x, y] = align(a, b); return x < y ? -1 : x > y ? 1 : 0; }
  function canonical(a) { return `${a.n}E${a.e}`; }
  function normalize(text) { return text.trim().replace(/[−–]/g, '-').replace(/\s+/g, ' ').toLowerCase(); }

  function assess(test, answer = '', observations = {}, locale = 'point') {
    let numeric = true, detail = 'Observations recorded.';
    const x = test.expected;
    if (x) {
      if (!answer.trim()) return { status: 'incomplete', detail: 'Enter the result or error shown on the calculator.' };
      try {
        const reportedError = /^(err(?:or)?:|tvm not found$|not available yet$)/.test(normalize(answer));
        if (['number', 'range'].includes(x.kind) && reportedError) {
          numeric = false;
          detail = 'A numerical result was expected; the calculator reported an error.';
        } else if (x.kind === 'number') {
          const actual = decimal(answer, locale), target = decimal(x.value);
          const rel = decimal(x.relative || '0'), absolute = decimal(x.absolute || '0');
          if (rel.n < 0n || absolute.n < 0n) throw new Error('Invalid tolerance.');
          const difference = abs(sub(actual, target));
          const limit = add(absolute, mul(rel, abs(target)));
          numeric = compare(difference, limit) <= 0;
          detail = `Absolute error ${canonical(difference)}; allowed ${canonical(limit)}.`;
        } else if (x.kind === 'error') {
          numeric = x.values.some(v => normalize(v) === normalize(answer));
          detail = numeric ? 'Expected error reported.' : 'The message differs from the expected error.';
        } else if (x.kind === 'text') {
          numeric = normalize(answer).replace(/\s/g, '') === normalize(x.value).replace(/\s/g, '');
          detail = numeric ? 'Expected text reported.' : 'The text differs from the expected result.';
        } else if (x.kind === 'range') {
          const actual = decimal(answer, locale);
          numeric = compare(actual, decimal(x.min)) >= 0 && compare(actual, decimal(x.max)) < 0;
          detail = numeric ? 'Result lies in the expected interval.' : 'Result lies outside the expected interval.';
        } else if (x.kind === 'underflow') {
          if (normalize(answer).startsWith('err:')) {
            numeric = ['err: overflow', 'err: underflow'].includes(normalize(answer));
          } else {
            numeric = decimal(answer, locale).n === 0n;
          }
          detail = 'Underflow: only zero or an explicit underflow/overflow error is accepted. The final zero-versus-error policy remains open.';
        } else throw new Error('Unknown expectation type.');
      } catch (error) { return { status: 'invalid', detail: error.message }; }
    }
    const checks = test.checks || [];
    const missing = checks.filter((_, i) => !['yes', 'no'].includes(observations[i]));
    if (missing.length) return { status: 'incomplete', detail: 'Answer each observation before recording.' };
    const passed = numeric && checks.every((_, i) => observations[i] === 'yes');
    return { status: passed ? 'pass' : 'fail', detail };
  }

  function makeRun(catalog) {
    return { schema: SCHEMA, catalog: catalog.revision, catalogSignature: signature(catalog), runId: globalThis.crypto?.randomUUID?.() || `${Date.now()}-${Math.random().toString(16).slice(2)}`,
      created: new Date().toISOString(), metadata: { tester: '', device: 'TI-84 Plus CE', os: '', version: catalog.version, build: '', notes: '' },
      selected: catalog.cases.filter(c => c.quick).map(c => c.id), records: {}, drafts: {}, cursor: null };
  }
  function latest(run, id) { const items = run.records[id] || []; return items[items.length - 1]; }
  function result(test, attempt) {
    if (!attempt) return { status: 'pending', detail: 'Not recorded.' };
    if (attempt.skipped) return { status: 'skipped', detail: attempt.notes || 'Skipped.' };
    return assess(test, attempt.answer, attempt.observations, attempt.locale);
  }
  function totals(catalog, run) {
    const counts = { pass: 0, fail: 0, skipped: 0, pending: 0, incomplete: 0, invalid: 0 };
    const selected = new Set(run.selected);
    for (const c of catalog.cases.filter(c => selected.has(c.id))) counts[result(c, latest(run, c.id)).status]++;
    return counts;
  }
  function coverage(catalog, run) {
    const byFunction = new Map();
    for (const c of catalog.cases) {
      if (['navigation', 'availability'].includes(c.focus)) continue;
      const ids = new Set([...c.covers, ...(c.keyFunction ? ['key:' + c.keyFunction] : [])]);
      for (const id of ids) { if (!byFunction.has(id)) byFunction.set(id, []); byFunction.get(id).push(c); }
    }
    const selected = new Set(run?.selected || []);
    return (catalog.requirements || []).map(requirement => {
      const rows = byFunction.get(requirement.id) || [];
      const planned = { functional: rows.length, edge: rows.filter(c => ['boundary', 'domain', 'regression', 'interaction'].includes(c.focus)).length,
        domain: rows.filter(c => c.focus === 'domain').length, regression: rows.filter(c => c.focus === 'regression').length };
      const counts = { selected: 0, pass: 0, fail: 0, skipped: 0, pending: 0, invalid: 0, incomplete: 0 };
      if (run) for (const c of rows.filter(c => selected.has(c.id))) { counts.selected++; counts[result(c, latest(run, c.id)).status]++; }
      return { id: requirement.id, required: requirement.required, planned, missing: requirement.required.filter(key => !planned[key]), caseIds: rows.map(c => c.id), ...(run ? { run: counts } : {}) };
    });
  }
  function report(catalog, run) {
    const copy = JSON.parse(JSON.stringify(run));
    const selected = new Set(run.selected);
    copy.exported = new Date().toISOString();
    copy.summary = totals(catalog, run);
    copy.reference = catalog.reference;
    copy.definitions = catalog.cases.filter(c => selected.has(c.id) || run.records[c.id]?.length || run.drafts[c.id]);
    copy.results = copy.definitions.map(c => ({ id: c.id, selected: selected.has(c.id), ...result(c, latest(run, c.id)) }));
    copy.baseline = catalog.baseline;
    copy.coverageLimits = catalog.gaps;
    copy.coverageRequirements = catalog.requirements;
    copy.functionCoverage = coverage(catalog, run);
    copy.menuRoutes = catalog.commands;
    copy.unavailableCommands = catalog.commands.filter(cmd => cmd.available === false).map(cmd => cmd.id);
    copy.uncoveredCommands = catalog.commands.filter(cmd => cmd.available !== false && !catalog.cases.some(c => !['navigation', 'availability'].includes(c.focus) && c.covers.includes(cmd.id))).map(cmd => cmd.id);
    return copy;
  }
  function validateRun(input, catalog) {
    if (!input || input.schema !== SCHEMA) throw new Error('This is not an RPNCE hardware report.');
    if (input.catalog !== catalog.revision || input.catalogSignature !== signature(catalog)) throw new Error('This report uses a different test catalog. Keep it for review; resume it with its matching tool version.');
    const ids = new Set(catalog.cases.map(c => c.id));
    if (!Array.isArray(input.selected) || input.selected.length > ids.size || input.selected.some(id => !ids.has(id)) || new Set(input.selected).size !== input.selected.length) throw new Error('Invalid test selection.');
    if (!input.metadata || typeof input.metadata !== 'object' || Array.isArray(input.metadata)) throw new Error('Missing device details.');
    const clean = makeRun(catalog);
    clean.selected = [...input.selected];
    for (const key of Object.keys(clean.metadata)) {
      if (typeof input.metadata[key] !== 'string' || input.metadata[key].length > 4000) throw new Error('Invalid device details.');
      clean.metadata[key] = input.metadata[key];
    }
    for (const key of ['created', 'runId']) {
      if (typeof input[key] !== 'string' || input[key].length > 100) throw new Error('Invalid report identity.');
      clean[key] = input[key];
    }
    if (!input.records || typeof input.records !== 'object' || Array.isArray(input.records)) throw new Error('Missing results.');
    for (const [id, attempts] of Object.entries(input.records)) {
      if (!ids.has(id) || !Array.isArray(attempts) || attempts.length > 100) throw new Error('Invalid result history.');
      clean.records[id] = attempts.map(a => {
        if (!a || typeof a.answer !== 'string' || a.answer.length > 256 || typeof a.notes !== 'string' || a.notes.length > 4000 || !['point', 'comma'].includes(a.locale) || typeof a.at !== 'string' || a.at.length > 100 || typeof a.skipped !== 'boolean') throw new Error('Invalid recorded answer.');
        const observations = {};
        const test = catalog.cases.find(c => c.id === id);
        if (!a.observations || typeof a.observations !== 'object' || Array.isArray(a.observations)) throw new Error('Invalid observations.');
        (test.checks || []).forEach((_, i) => {
          if (a.observations[i] !== undefined && !['yes', 'no', ''].includes(a.observations[i])) throw new Error('Invalid observation.');
          observations[i] = a.observations[i] || '';
        });
        return { answer: a.answer, notes: a.notes, locale: a.locale, at: a.at, skipped: a.skipped, observations };
      });
    }
    // A draft is ungraded input; never restore imported result labels or expectations.
    if (input.drafts && typeof input.drafts === 'object') {
      for (const [id, draft] of Object.entries(input.drafts)) {
        if (!ids.has(id) || !draft || typeof draft.answer !== 'string' || draft.answer.length > 256 || typeof draft.notes !== 'string' || draft.notes.length > 4000 || !['point', 'comma'].includes(draft.locale)) throw new Error('Invalid saved draft.');
        const observations = {};
        (catalog.cases.find(c => c.id === id).checks || []).forEach((_, i) => { observations[i] = ['yes', 'no'].includes(draft.observations?.[i]) ? draft.observations[i] : ''; });
        clean.drafts[id] = { answer: draft.answer, notes: draft.notes, locale: draft.locale, observations };
      }
    }
    clean.cursor = ids.has(input.cursor) ? input.cursor : null;
    return clean;
  }
  const api = { SCHEMA, MAX_FILE_BYTES, decimal, compare, assess, makeRun, latest, result, totals, coverage, report, validateRun };
  if (typeof module !== 'undefined' && module.exports) module.exports = api;
  else root.HardwareEngine = api;
})(globalThis);

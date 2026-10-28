const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const E = require('./engine.js');
const C = require('./catalog.js');
const number = (value, relative = '0', absolute = '0') => ({ expected: { kind: 'number', value, relative, absolute }, checks: [] });
const attempt = (answer, extra = {}) => ({ answer, notes: '', observations: {}, locale: 'point', at: '2026-10-23T00:00:00Z', skipped: false, ...extra });

test('decimal parsing accepts scientific notation and the selected grouping convention', () => {
  assert.equal(E.compare(E.decimal('1,234,567.89'), E.decimal('1234567.89')), 0);
  assert.equal(E.compare(E.decimal('1.234.567,89', 'comma'), E.decimal('1234567.89')), 0);
  assert.equal(E.compare(E.decimal('−1,25ᴇ−99', 'comma'), E.decimal('-1.25E-99')), 0);
  assert.equal(E.compare(E.decimal('.5'), E.decimal('5E-1')), 0);
  assert.equal(E.compare(E.decimal('+0'), E.decimal('-0')), 0);
  for (const invalid of ['NaN', 'Infinity', '1E', '1E2E3', '1E1001', '1,23.4', '1 000', '3 m', '0x10', '', '--1', '1.' + '0'.repeat(256)]) assert.throws(() => E.decimal(invalid), undefined, invalid);
  assert.throws(() => E.decimal('1.23,4', 'comma'));
});

test('exact answers do not silently lose stored decimal digits or large integers', () => {
  assert.equal(E.assess(number('1.0000000000001'), '1').status, 'fail');
  assert.equal(E.assess(number('17592186044415'), '17592186044415').status, 'pass');
  assert.equal(E.assess(number('17592186044415'), '17592186044414').status, 'fail');
  assert.equal(E.assess(number('2.68'), '2.675').status, 'fail');
});

test('relative tolerances reject zero for tiny answers and work at both exponent limits', () => {
  for (const value of ['1E-99', '-5E-90', '6.3657280768316E99']) {
    assert.equal(E.assess(number(value, '5E-11'), '0').status, 'fail');
    assert.equal(E.assess(number(value, '5E-11'), value).status, 'pass');
  }
  const tiny = number('1E-99', '1E-12');
  assert.equal(E.assess(tiny, '1.000000000001E-99').status, 'pass');
  assert.equal(E.assess(tiny, '1.000000000002E-99').status, 'fail');
  assert.equal(E.assess(number('0', '0', '1E-14'), '-1E-14').status, 'pass');
  assert.equal(E.assess(number('0', '0', '1E-14'), '1.0001E-14').status, 'fail');
});

test('comma answers are interpreted explicitly, not guessed', () => {
  assert.equal(E.assess(number('1234.5'), '1.234,5', {}, 'comma').status, 'pass');
  assert.equal(E.assess(number('1234.5'), '1.234,5').status, 'invalid');
});

test('observations must be answered and a failed observation defeats a correct number', () => {
  const c = { ...number('5'), checks: ['Unit is m', 'Y is preserved'] };
  assert.equal(E.assess(c, '5', {}).status, 'incomplete');
  assert.equal(E.assess(c, '5', { 0: 'yes', 1: 'no' }).status, 'fail');
  assert.equal(E.assess(c, '5', { 0: 'yes', 1: 'yes' }).status, 'pass');
  assert.equal(E.assess(c, 'not a number', { 0: 'yes', 1: 'yes' }).status, 'invalid');
});

test('errors, DATE text, random intervals and range policies have distinct grading', () => {
  assert.equal(E.assess({ expected: { kind: 'error', values: ['Err: Division By 0'] } }, 'err: division by 0').status, 'pass');
  assert.equal(E.assess({ expected: { kind: 'error', values: ['Err: Domain'] } }, '0').status, 'fail');
  assert.equal(E.assess({ expected: { kind: 'text', value: 'TZ{-7,0}' } }, 'TZ{ −7, 0 }').status, 'pass');
  const random = { expected: { kind: 'range', min: '0', max: '1' } };
  assert.equal(E.assess(random, '0').status, 'pass');
  assert.equal(E.assess(random, '1').status, 'fail');
  const underflow = { expected: { kind: 'underflow' } };
  assert.equal(E.assess(underflow, '1E-100').status, 'fail');
  assert.equal(E.assess(underflow, '0').status, 'fail');
  assert.equal(E.assess(underflow, 'Err: Overflow').status, 'fail');
  assert.equal(E.assess(underflow, 'Err: Underflow').status, 'pass');
  assert.equal(E.assess(underflow, 'Err: Data Type').status, 'fail');
  assert.equal(E.assess(underflow, '1').status, 'fail');
  assert.equal(E.assess(underflow, 'Err: Domain').status, 'fail');
  const overflow = { expected: { kind: 'error', values: ['Err: Overflow'] } };
  for (const wrong of ['0', '1', '1E99', '1E100']) assert.equal(E.assess(overflow, wrong).status, 'fail');
});

test('an unexpected calculator error is a recordable failure, not malformed numeric input', () => {
  for (const message of ['Err: Overflow', 'Err: Domain', 'Err: Division By 0', 'TVM Not Found']) {
    assert.equal(E.assess(number('6.3657280768316E99', '5E-11'), message).status, 'fail');
  }
  const c = C.cases.find(c => c.id === 'command-lcm');
  const run = E.makeRun(C); run.selected = [c.id]; run.records[c.id] = [attempt('Err: Overflow')];
  assert.equal(E.report(C, run).summary.fail, 1);
  assert.equal(E.report(C, E.validateRun(E.report(C, run), C)).summary.fail, 1);
  assert.equal(E.assess(number('1'), 'misspelled number').status, 'invalid');
});

test('reports keep attempts, drafts and definitions without counting skips or pending cases as passed', () => {
  const run = E.makeRun(C);
  const a = C.cases.find(c => c.expected?.kind === 'number' && !c.checks.length);
  const b = C.cases.find(c => c.id !== a.id);
  const pending = C.cases.find(c => ![a.id, b.id].includes(c.id));
  run.selected = [a.id, b.id, pending.id];
  run.records[a.id] = [attempt('0'), attempt(a.expected.value)];
  run.records[b.id] = [attempt('', { skipped: true, notes: 'Battery charging' })];
  run.drafts[pending.id] = { answer: 'unfinished', notes: 'come back', locale: 'point', observations: {} };
  const report = E.report(C, run);
  assert.deepEqual(report.summary, { pass: 1, fail: 0, skipped: 1, pending: 1, incomplete: 0, invalid: 0 });
  assert.equal(report.records[a.id].length, 2);
  assert.equal(report.drafts[pending.id].answer, 'unfinished');
  assert.equal(report.definitions.length, 3);
  assert.deepEqual(E.report(C, E.validateRun(JSON.parse(JSON.stringify(report)), C)).summary, report.summary);
  run.selected = [pending.id];
  assert.ok(E.report(C, run).definitions.some(c => c.id === a.id), 'Out-of-scope attempt definitions are preserved.');
});

test('import discards supplied verdicts and expectations and rejects incompatible catalogs', () => {
  const c = C.cases.find(c => c.expected?.kind === 'number' && c.expected.value !== '0' && !c.checks.length);
  const run = E.makeRun(C); run.selected = [c.id]; run.records[c.id] = [attempt('0')];
  const report = E.report(C, run);
  report.results[0].status = 'pass'; report.summary.pass = 999;
  report.definitions[0] = { ...report.definitions[0], expected: { kind: 'number', value: '0' } };
  const loaded = E.validateRun(report, C);
  assert.equal(E.totals(C, loaded).fail, 1);
  assert.equal(E.totals(C, loaded).pass, 0);
  assert.throws(() => E.validateRun({ ...report, catalog: 'old' }, C), /different test catalog/);
  const changed = { ...C, baseline: [...C.baseline, 'Changed procedure'] };
  assert.throws(() => E.validateRun(report, changed), /different test catalog/);
  assert.throws(() => E.validateRun({ ...report, selected: ['not-a-test'] }, C), /selection/);
  assert.throws(() => E.validateRun({ ...report, selected: [c.id, c.id] }, C), /selection/);
  assert.throws(() => E.validateRun({ ...report, records: { [c.id]: [attempt('0', { locale: 'guess' })] } }, C), /recorded answer/);
});

test('completed numerical cases are retired without changing historical reports', () => {
  const retired = [
    'expand-guard-cancellation', 'boundary-divide-underflow', 'boundary-add-overflow',
    'boundary-multiply-overflow', 'boundary-divide-overflow', 'boundary-large-polar',
    'boundary-small-polar', 'boundary-sinh-large', 'expand-guard-acosh',
    'expand-guard-atanh', 'expand-guard-ln1p', 'boundary-stat-cancellation',
    'boundary-unit-overflow', 'boundary-unit-underflow'
  ];
  for (const id of retired) assert.ok(!C.cases.some(c => c.id === id), id);
  assert.ok(!C.suites?.some(s => s.id === 'numerical-confirmation'));
  for (const [filename, revision] of [
    ['rpnce-hardware-c3165aca-19a2-48e9-b053-c58681dee30e.json', 'hardware-0.7.0-2'],
    ['rpnce-hardware-2a9a2054-33d6-4bce-82e9-f1fb8e8df91a.json', 'hardware-0.7.0-3']
  ]) {
    const file = path.join(__dirname, 'results', filename);
    if (!fs.existsSync(file)) continue;
    const report = JSON.parse(fs.readFileSync(file, 'utf8'));
    assert.equal(report.catalog, revision);
    assert.equal(report.definitions.length, 14);
    assert.throws(() => E.validateRun(report, C), /different test catalog/);
  }
});

test('limit cases accept only the stated warning and never a misleading zero', () => {
  const cases = [
    ['expand-stat-offset-stat_sample_sdev-0', 'Err: Precision'],
    ['expand-polar-6-0', 'Err: Overflow'],
    ['expand-polar-7-0', 'Err: Underflow'],
    ['expand-hyp-sinh-6', 'Err: Overflow']
  ];
  for (const [id, allowed] of cases) {
    const c = C.cases.find(c => c.id === id);
    const checks = Object.fromEntries(c.checks.map((_, i) => [i, 'yes']));
    assert.equal(E.assess(c, c.expected.value, checks).status, 'pass', id);
    assert.equal(E.assess(c, allowed, checks).status, 'pass', id);
    assert.equal(E.assess(c, allowed.replace(': ', ':'), checks).status, 'pass', id);
    assert.equal(E.assess(c, '0', checks).status, 'fail', id);
    assert.equal(E.assess(c, 'Err: Domain', checks).status, 'fail', id);
  }
  const old = E.report(C, E.makeRun(C));
  old.catalog = 'hardware-0.7.0-2';
  assert.throws(() => E.validateRun(old, C), /different test catalog/);
});

test('every error expectation rejects numbers and unrelated errors throughout the catalog', () => {
  const core = path.join(__dirname, '../../src/core');
  const messages = new Set(['TVM Not Found']);
  for (const file of fs.readdirSync(core).filter(f => f.endsWith('.c'))) {
    for (const match of fs.readFileSync(path.join(core, file), 'utf8').matchAll(/"(Err: [^"\n]+)"/g)) messages.add(match[1]);
  }
  for (const c of C.cases) {
    const x = c.expected;
    if (!x) continue;
    const checks = Object.fromEntries(c.checks.map((_, i) => [i, 'yes']));
    const allowed = x.kind === 'error' ? x.values : x.errors || [];
    assert.equal(new Set(allowed).size, allowed.length, c.id);
    for (const message of allowed) {
      assert.ok(messages.has(message), `${c.id}: unknown error ${message}`);
      assert.equal(E.assess(c, message, checks).status, 'pass', c.id);
    }
    if (x.kind === 'error') {
      assert.ok(allowed.length, c.id);
      for (const answer of ['0', '1E99', '1E-99', '1E100', '1E-100', 'NaN', 'Infinity']) {
        assert.equal(E.assess(c, answer, checks).status, 'fail', `${c.id}: ${answer}`);
      }
    }
    if (['error', 'number', 'range'].includes(x.kind)) {
      for (const message of messages) if (!allowed.includes(message)) {
        assert.equal(E.assess(c, message, checks).status, 'fail', `${c.id}: ${message}`);
      }
    }
    if (x.kind === 'number') {
      assert.equal(E.assess(c, x.value, checks).status, 'pass', c.id);
      const v = E.decimal(x.value);
      if (v.n && E.compare(E.decimal(x.absolute || '0'), { n: v.n < 0n ? -v.n : v.n, e: v.e }) < 0) {
        assert.equal(E.assess(c, '0', checks).status, 'fail', c.id);
      }
    }
  }
});

test('weekday, display rounding and numerical-limit references retain their audited contracts', () => {
  const find = id => C.cases.find(c => c.id === id);
  for (const id of ['iso_dow_number_to_day_of_week', 'unix_dow_number_to_day_of_week']) {
    const c = find('expand-date-wrong-type-' + id);
    assert.ok(c.steps.some(s => s.includes('2024, 3, 14')), c.id);
    assert.deepEqual(c.expected.values, ['Err: Data Type']);
  }
  assert.equal(find('expand-date-number-boundary-unix_dow_number_to_day_of_week0').expected.value, 'DW{7}');
  for (const style of ['SCI', 'ENG']) assert.equal(E.compare(E.decimal(find('expand-round-display-' + style).expected.value), E.decimal('1230')), 0);
  for (const op of ['recip', 'square', 'alog', 'exp', 'pow', 'cube', 'two_pow', 'percent']) {
    const c = find('expand-range-underflow-' + op);
    assert.deepEqual(c.expected.values, ['Err: Underflow'], c.id);
    assert.ok(c.checks.some(s => s.includes('X still contains') || s.includes('left X')), c.id);
  }
  const mod = find('expand-quantity-binary-mod');
  assert.equal(E.assess(mod, '.1666666666665', { 0: 'yes' }).status, 'fail');
  assert.equal(E.assess(mod, '0', { 0: 'yes' }).status, 'pass');
  assert.equal(E.assess(mod, 'Err: Precision', { 0: 'yes' }).status, 'pass');
  assert.equal(E.assess(mod, 'Err: Domain', { 0: 'yes' }).status, 'fail');
});

test('catalog has current menu IDs, all units, executable expectations and useful coverage', () => {
  const ids = new Set();
  const walk = file => {
    for (const raw of fs.readFileSync(path.join(__dirname, '../../menus', file), 'utf8').split('\n')) {
      const line = raw.split('#')[0].trim().split(/\s+/);
      if (line[0] === 'include') walk(line[1]);
      if (line[0] === 'item') ids.add(line[2]);
      if (line[0] === 'alt') ids.add(line[3]);
    }
  };
  walk('menus.txt');
  assert.deepEqual([...ids].sort(), C.commands.map(c => c.id).sort(), 'Refresh menu inventory when commands change.');
  const files = new Set();
  const find = file => { files.add(file); for (const raw of fs.readFileSync(path.join(__dirname, '../../menus', file), 'utf8').split('\n')) { const line = raw.trim().split(/\s+/); if (line[0] === 'include') find(line[1]); } };
  find('menus.txt'); const hash = crypto.createHash('sha256');
  for (const file of [...files].sort()) { hash.update(file); hash.update(fs.readFileSync(path.join(__dirname, '../../menus', file))); }
  assert.equal(hash.digest('hex'), C.menuDigest, 'Menu rows changed: update routes and cases.');
  assert.equal(C.groups.length, 56);
  assert.equal(new Set(C.cases.map(c => c.id)).size, C.cases.length);
  assert.equal(C.cases.filter(c => c.id.startsWith('unit-unit_')).length, 169);
  assert.equal(C.cases.filter(c => c.kind === 'navigation').length, C.groups.length);
  for (const c of C.cases) {
    assert.ok(c.steps.length && c.steps.every(s => typeof s === 'string' && !s.includes('undefined')), c.id);
    assert.ok(c.expected || c.checks.length, `${c.id} has nothing to assess`);
    c.covers.forEach(id => assert.ok(ids.has(id), `${c.id}: unknown ${id}`));
    const observations = Object.fromEntries(c.checks.map((_, i) => [i, 'yes']));
    let answer = '';
    if (c.expected?.kind === 'number' || c.expected?.kind === 'text') answer = c.expected.value;
    if (c.expected?.kind === 'error') answer = c.expected.values[0];
    if (c.expected?.kind === 'range') answer = c.expected.min;
    if (c.expected?.kind === 'underflow') answer = 'Err: Underflow';
    assert.equal(E.assess(c, answer, observations).status, 'pass', c.id);
    if (c.expected?.kind === 'number' && E.decimal(c.expected.value).n !== 0n && !c.expected.absolute?.match(/[1-9]/)) assert.equal(E.assess(c, '0', observations).status, 'fail', c.id);
  }
  for (const id of ids) assert.ok(C.cases.some(c => c.kind !== 'navigation' && c.covers.includes(id)), `No functional case for ${id}`);
});

test('every implemented command and keyboard function meets its required risk coverage', () => {
  const matrix = E.coverage(C);
  const ids = matrix.map(row => row.id);
  assert.equal(new Set(ids).size, ids.length);
  for (const cmd of C.commands.filter(cmd => cmd.available !== false)) assert.ok(ids.includes(cmd.id), `No requirements for ${cmd.id}`);
  for (const row of matrix) {
    assert.deepEqual(row.missing, [], row.id);
    assert.ok(row.planned.functional > 0 && row.planned.edge > 0, row.id);
    if (row.required.includes('domain')) assert.ok(row.planned.domain > 0, row.id);
  }
  for (const cmd of C.commands.filter(cmd => cmd.available === false)) {
    assert.ok(!ids.includes(cmd.id));
    assert.ok(C.cases.some(c => c.focus === 'availability' && c.covers.includes(cmd.id)));
  }
  const run = E.makeRun(C); run.selected = C.cases.map(c => c.id);
  const report = E.report(C, run);
  assert.equal(report.summary.pass, 0);
  assert.ok(report.functionCoverage.every(row => row.run.pass === 0));
  assert.ok(report.functionCoverage.every(row => row.run.pending === row.run.selected));
  assert.ok(Buffer.byteLength(JSON.stringify(report)) < E.MAX_FILE_BYTES, 'The complete report must fit its resume limit.');
});

test('expanded question data tracks its source and exact unit definitions', () => {
  const questions = require('./questions.js');
  const digest = file => crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
  assert.equal(questions.sourceDigest, digest(path.join(__dirname, 'questions.py')), 'Regenerate questions.js.');
  assert.equal(questions.unitDefinitionsDigest, digest(path.join(__dirname, '../../tools/genunits.py')), 'Regenerate unit references.');
  for (const c of questions.cases) {
    assert.ok(['boundary', 'domain', 'regression', 'interaction', 'normal'].includes(c.focus), c.id);
    if (c.expected?.kind === 'number') {
      const d = E.decimal(c.expected.value);
      const exponent = d.n ? d.e + (d.n < 0n ? -d.n : d.n).toString().length - 1 : 0;
      assert.ok(exponent >= -99 && exponent <= 99, `${c.id}: expected a supported numerical result`);
    }
    if (c.operands && !c.wordSize) for (const operand of c.operands) {
      const digits = operand.replace(/^[+-]/, '').split(/[eE]/)[0].replace('.', '').length;
      assert.ok(digits <= 12, `${c.id}: operand must fit the typing limit`);
    }
  }
  assert.ok(!questions.cases.some(c => c.id.startsWith('expand-guard-')));
  for (const ident of ['unit_fermi','unit_inch','unit_celsius','unit_miles_per_gallon']) {
    assert.ok(questions.cases.some(c => c.id.startsWith('expand-unit-scale-'+ident)), ident);
    assert.ok(questions.cases.some(c => c.id === 'expand-unit-roundtrip-'+ident), ident);
  }
});

test('arithmetic references agree with independent exact rational arithmetic', () => {
  const rational = s => { const d = E.decimal(s); return d.e < 0 ? [d.n, 10n ** BigInt(-d.e)] : [d.n * 10n ** BigInt(d.e), 1n]; };
  const render = (n, d) => { const sign = n * d < 0n ? '-' : ''; n = n < 0n ? -n : n; d = d < 0n ? -d : d; const scale = 10n ** 150n; return sign + (n * scale / d).toString() + 'E-150'; };
  for (const c of require('./questions.js').cases.filter(c => c.expected?.kind === 'number' && c.operands?.length === 2 && ['ADD','SUB','MUL','DIV'].includes(c.keyFunction))) {
    const [a,b] = rational(c.operands[0]), [x,y] = rational(c.operands[1]);
    const [n,d] = c.keyFunction === 'ADD' ? [a*y+x*b,b*y] : c.keyFunction === 'SUB' ? [a*y-x*b,b*y] : c.keyFunction === 'MUL' ? [a*x,b*y] : [a*y,b*x];
    assert.equal(E.assess(c, render(n,d)).status, 'pass', c.id);
  }
});

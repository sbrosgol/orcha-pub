#pragma once

namespace Orcha::Agent {

// Embedded admin dashboard served at /admin. Vanilla HTML/JS, no build step,
// no external assets. Talks to the /api/plugins and /api/jobs JSON APIs.
//
// Features: custom login view, Light/Dark/System themes, Plugins admin, Jobs
// (CRUD + run + history), and an interactive HTML Canvas workflow flow chart.
inline constexpr const char kDashboardHtml[] = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>Orcha Admin</title>
  <script>document.documentElement.dataset.theme = localStorage.getItem('orcha_theme') || 'system';</script>
  <script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.1/dist/chart.umd.min.js" defer></script>
  <style>
    /* Light theme. Deep, saturated severity colors so chips read as labels on
       a near-white panel; teal-700 as the primary accent. */
    :root {
      --bg:#f6f8fa; --panel:#ffffff; --panel-2:#f0f3f7; --line:#d0d7de; --fg:#1f2328;
      --muted:#656d76; --accent:#0d9488; --on-accent:#ffffff;
      --crit:#dc2626; --high:#ea580c; --med:#ca8a04; --low:#16a34a;
      --info:#0284c7; --violet:#7c3aed; --pink:#be185d;
      --ok:var(--low); --warn:var(--med); --err:var(--crit);
      --grid:#e6ebf0;
      --shadow:0 1px 3px rgba(27,31,36,.12), 0 8px 24px rgba(27,31,36,.08);
    }
    /* Dark theme — matches the dashboard reference: deeper navy bg, teal accent,
       full severity palette (red/orange/yellow/green) + info/violet/pink. */
    :root[data-theme="dark"] {
      --bg:#0b0f17; --panel:#131a26; --panel-2:#1a2333; --line:#1f2a3c; --fg:#e6ecf5;
      --muted:#8a97ac; --accent:#5eead4; --on-accent:#0b1220;
      --crit:#ef4444; --high:#f97316; --med:#eab308; --low:#22c55e;
      --info:#38bdf8; --violet:#a78bfa; --pink:#f472b6;
      --ok:var(--low); --warn:var(--med); --err:var(--crit);
      --grid:#1f2a3c;
      --shadow:0 1px 3px rgba(0,0,0,.5), 0 8px 24px rgba(0,0,0,.4);
    }
    @media (prefers-color-scheme: dark) {
      :root[data-theme="system"] {
        --bg:#0b0f17; --panel:#131a26; --panel-2:#1a2333; --line:#1f2a3c; --fg:#e6ecf5;
        --muted:#8a97ac; --accent:#5eead4; --on-accent:#0b1220;
        --crit:#ef4444; --high:#f97316; --med:#eab308; --low:#22c55e;
        --info:#38bdf8; --violet:#a78bfa; --pink:#f472b6;
        --ok:var(--low); --warn:var(--med); --err:var(--crit);
        --grid:#1f2a3c;
        --shadow:0 1px 3px rgba(0,0,0,.5), 0 8px 24px rgba(0,0,0,.4);
      }
    }
    * { box-sizing: border-box; }
    body { margin:0; font:14px/1.5 -apple-system,Segoe UI,Roboto,sans-serif;
           background:var(--bg); color:var(--fg); }
    header { padding:14px 24px; border-bottom:1px solid var(--line);
             display:flex; align-items:center; gap:18px; }
    header h1 { font-size:17px; margin:0; font-weight:600; }
    nav { display:flex; gap:4px; }
    nav button { background:none; border:none; color:var(--muted); padding:6px 12px;
                 border-radius:6px; cursor:pointer; font-size:14px; }
    nav button.active { color:var(--fg); background:var(--bg); font-weight:600; }
    .spacer { flex:1; }
    main { padding:24px; max-width:1180px; margin:0 auto; }
    button.btn { background:var(--panel); color:var(--fg); border:1px solid var(--line);
                 border-radius:6px; padding:6px 12px; cursor:pointer; font-size:13px; }
    button.btn:hover { border-color:var(--accent); }
    button.btn.primary { background:var(--accent); color:var(--on-accent); border-color:var(--accent); }
    button.btn.danger:hover { border-color:var(--err); color:var(--err); }
    select, input[type=text], input[type=password], textarea {
      background:var(--bg); color:var(--fg); border:1px solid var(--line);
      border-radius:6px; padding:8px 10px; font-size:13px; font-family:inherit; }
    textarea { width:100%; min-height:200px; font-family:ui-monospace,SFMono-Regular,Menlo,monospace; }
    input:focus, textarea:focus, select:focus { outline:none; border-color:var(--accent); }
    table { width:100%; border-collapse:collapse; background:var(--panel);
            border:1px solid var(--line); border-radius:8px; overflow:hidden; }
    th,td { text-align:left; padding:10px 14px; border-bottom:1px solid var(--line); vertical-align:top; }
    th { color:var(--muted); font-weight:500; font-size:12px; text-transform:uppercase; }
    tr:last-child td { border-bottom:none; }
    .badge { display:inline-block; padding:2px 8px; border-radius:999px; font-size:11px; font-weight:600; }
    .badge.loaded,.badge.success { background:color-mix(in srgb,var(--low) 18%,transparent); color:var(--low); }
    .badge.available { background:color-mix(in srgb,var(--info) 18%,transparent); color:var(--info); }
    .badge.disabled { background:color-mix(in srgb,var(--muted) 22%,transparent); color:var(--muted); }
    .badge.failed { background:color-mix(in srgb,var(--crit) 18%,transparent); color:var(--crit); }
    .badge.kev    { background:color-mix(in srgb,var(--accent) 18%,transparent); color:var(--accent); }
    .badge.cron   { background:color-mix(in srgb,var(--violet) 22%,transparent); color:var(--violet); }
    .badge.manual { background:color-mix(in srgb,var(--pink) 22%,transparent); color:var(--pink); }
    .badge.api    { background:color-mix(in srgb,var(--info) 22%,transparent); color:var(--info); }
    .badge.schedule { background:color-mix(in srgb,var(--violet) 22%,transparent); color:var(--violet); }
    .tags span { background:var(--bg); border:1px solid var(--line); border-radius:4px;
                 padding:1px 6px; margin-right:4px; font-size:11px; color:var(--muted); }
    .actions { display:flex; gap:6px; flex-wrap:wrap; }
    .muted { color:var(--muted); }
    .toolbar { display:flex; align-items:center; gap:12px; margin-bottom:16px; }
    .grid { display:grid; grid-template-columns: 320px 1fr; gap:20px; }
    .grid > * { min-width:0; }  /* let columns shrink instead of being widened by long unbreakable content */
    .panel { background:var(--panel); border:1px solid var(--line); border-radius:8px; }
    .panel h3 { margin:0; padding:12px 14px; border-bottom:1px solid var(--line); font-size:13px;
                text-transform:uppercase; color:var(--muted); letter-spacing:.03em; }
    .joblist .item { padding:10px 14px; border-bottom:1px solid var(--line); cursor:pointer; }
    .joblist .item:last-child { border-bottom:none; }
    .joblist .item.active { background:var(--bg); }
    .joblist .item .nm { font-weight:600; }
    .pad { padding:14px; }
    #toast { position:fixed; bottom:20px; right:20px; padding:10px 16px; border-radius:6px;
             background:var(--panel); border:1px solid var(--line); display:none; max-width:380px;
             box-shadow:var(--shadow); z-index:80; }
    #toast.err { border-color:var(--err); color:var(--err); }
    #toast.ok { border-color:var(--ok); color:var(--ok); }
    .theme-ctl { display:flex; align-items:center; gap:6px; }
    .theme-ctl > span { font-size:11px; color:var(--muted); }

    /* Login */
    #login { position:fixed; inset:0; z-index:50; background:var(--bg);
             display:flex; align-items:center; justify-content:center; padding:20px; }
    .card { background:var(--panel); border:1px solid var(--line); border-radius:12px;
            padding:30px 28px; width:340px; box-shadow:var(--shadow); }
    .card .logo { width:38px; height:38px; border-radius:9px; background:var(--accent);
                  display:flex; align-items:center; justify-content:center; color:var(--on-accent);
                  font-weight:700; font-size:18px; margin-bottom:16px; }
    .card h2 { margin:0 0 4px; font-size:19px; }
    .card .sub { color:var(--muted); margin:0 0 20px; font-size:13px; }
    .field { margin-bottom:14px; }
    .field label { display:block; font-size:12px; color:var(--muted); margin-bottom:6px; }
    .field input { width:100%; }
    .field input[type="checkbox"] { width:auto; }
    .btn-primary { width:100%; background:var(--accent); color:var(--on-accent); border:none;
      padding:10px; border-radius:6px; font-size:14px; cursor:pointer; font-weight:600; margin-top:4px; }
    .login-err { color:var(--err); font-size:13px; margin-top:12px; min-height:18px; }

    body:not(.authed) #app { display:none; }
    body.authed #login { display:none; }
    #logout { display:none; } body.authed #logout { display:inline-block; }
    .view { display:none; } .view.active { display:block; }

    /* Flow chart */
    .chartwrap { position:relative; overflow:auto; background:var(--bg);
                 border:1px solid var(--line); border-radius:8px; }
    #flow { display:block; }
    #fctip { position:absolute; pointer-events:none; display:none; max-width:280px;
             background:var(--panel); border:1px solid var(--line); border-radius:6px;
             padding:8px 10px; font-size:12px; box-shadow:var(--shadow); z-index:5; white-space:pre-wrap; }

    /* Modal */
    #modal { position:fixed; inset:0; z-index:70; background:rgba(0,0,0,.45);
             display:none; align-items:center; justify-content:center; padding:20px; }
    #modal.show { display:flex; }
    .modal-card { background:var(--panel); border:1px solid var(--line); border-radius:12px;
                  width:640px; max-width:100%; max-height:90vh; overflow:auto; box-shadow:var(--shadow); }
    .modal-card .hd { padding:16px 20px; border-bottom:1px solid var(--line); font-weight:600; }
    .modal-card .bd { padding:20px; } .modal-card .ft { padding:16px 20px; border-top:1px solid var(--line);
                  display:flex; gap:8px; justify-content:flex-end; }
    .runrow { cursor:pointer; }
    .mono { font-family:ui-monospace,SFMono-Regular,Menlo,monospace; font-size:12px;
            white-space:pre-wrap; word-break:break-word; }
    .outcard { border:1px solid var(--line); border-radius:8px; margin:10px 0; overflow:hidden; }
    .outcard .hd { display:flex; align-items:center; gap:10px; padding:8px 12px;
                   background:var(--bg); border-bottom:1px solid var(--line); font-size:13px; }
    .outcard .hd .sp { flex:1; }
    .outcard pre { margin:0; padding:12px; max-height:280px; overflow:auto; font-size:12px;
                   font-family:ui-monospace,SFMono-Regular,Menlo,monospace;
                   white-space:pre-wrap; word-break:break-word; }
    .outcard .err { color:var(--err); padding:10px 12px; font-size:13px; }
    .outcard.flash { box-shadow:0 0 0 2px var(--accent); transition:box-shadow .2s; }
    .runrow.sel td { background:var(--bg); }

    /* Editor: tabs and step builder */
    .tabs { display:flex; gap:2px; border-bottom:1px solid var(--line); margin-bottom:10px; }
    .tab { background:none; border:none; color:var(--muted); padding:8px 14px; cursor:pointer;
           font-size:13px; border-bottom:2px solid transparent; margin-bottom:-1px; font-family:inherit; }
    .tab.active { color:var(--fg); border-bottom-color:var(--accent); font-weight:600; }
    .tab:hover { color:var(--fg); }
    .tab-warn { background:color-mix(in srgb,var(--warn) 14%,transparent); color:var(--warn);
                border:1px solid color-mix(in srgb,var(--warn) 40%,transparent);
                border-radius:6px; padding:8px 10px; font-size:12px; margin-bottom:10px; }
    .step-card { border:1px solid var(--line); border-radius:8px; padding:10px 12px;
                 margin-bottom:8px; background:var(--bg); }
    .step-card .shd { display:flex; align-items:center; gap:6px; margin-bottom:8px; }
    .step-card .shd .lbl { font-weight:600; font-size:11px; color:var(--muted);
                           text-transform:uppercase; letter-spacing:.03em; }
    .step-card .shd .cmd { font-weight:600; color:var(--fg); }
    .step-card .shd .sp { flex:1; }
    .step-card .shd button { background:var(--panel); border:1px solid var(--line);
                             border-radius:4px; padding:2px 7px; cursor:pointer; font-size:12px;
                             color:var(--fg); font-family:inherit; }
    .step-card .shd button:hover { border-color:var(--accent); }
    .step-card .shd button:disabled { opacity:.35; cursor:not-allowed; }
    .step-card .shd button.danger:hover { border-color:var(--err); color:var(--err); }
    .step-card .param { display:grid; grid-template-columns:130px 1fr; gap:8px; align-items:start;
                        margin-bottom:6px; }
    .step-card .param .pname { font-size:12px; color:var(--muted); padding-top:8px; }
    .step-card .param .pname .req { color:var(--err); margin-left:2px; }
    .step-card .param input[type=text],
    .step-card .param input[type=number] { width:100%; }
    .step-card .param input[type=checkbox] { margin-top:10px; }
    .kv-rows { display:flex; flex-direction:column; gap:4px; }
    .kv-row { display:flex; gap:4px; }
    .kv-row input { flex:1; min-width:0; }
    .kv-row .x { background:var(--panel); border:1px solid var(--line); border-radius:4px;
                 padding:0 8px; cursor:pointer; color:var(--muted); font-family:inherit; }
    .kv-row .x:hover { color:var(--err); border-color:var(--err); }
    .kv-add { background:none; border:1px dashed var(--line); border-radius:4px;
              padding:4px 8px; cursor:pointer; color:var(--muted); font-size:12px;
              align-self:flex-start; font-family:inherit; }
    .kv-add:hover { color:var(--accent); border-color:var(--accent); }
    .builder-add { display:flex; gap:8px; align-items:center; margin-top:8px; }
    .builder-add select { flex:1 1 0; min-width:0; }
    .builder-add button { flex:0 0 auto; white-space:nowrap; }
    .empty-hint { color:var(--muted); font-size:13px; padding:10px 12px;
                  border:1px dashed var(--line); border-radius:8px; margin-bottom:8px; }
    .cron-row { display:grid; grid-template-columns:200px 1fr; gap:8px; align-items:center; }
    .cron-row select, .cron-row input { width:100%; }
    .cron-preview { font-size:12px; color:var(--muted); margin-top:6px; min-height:18px; }
    .cron-preview.err { color:var(--err); }

    /* ---------------- Overview dashboard ---------------- */
    .kpis { display:grid; grid-template-columns:repeat(auto-fit,minmax(150px,1fr));
            gap:12px; margin-bottom:18px; }
    .kpi  { background:var(--panel); border:1px solid var(--line); border-radius:10px;
            padding:14px 16px; min-height:84px;
            display:flex; flex-direction:column; justify-content:space-between; }
    .kpi .label { color:var(--muted); font-size:11px; text-transform:uppercase; letter-spacing:1px; }
    .kpi .value { font-size:26px; font-weight:700; line-height:1.1; }
    .kpi.crit   .value { color:var(--crit); }
    .kpi.high   .value { color:var(--high); }
    .kpi.med    .value { color:var(--med); }
    .kpi.low    .value { color:var(--low); }
    .kpi.info   .value { color:var(--info); }
    .kpi.kev    .value { color:var(--accent); }
    .kpi.violet .value { color:var(--violet); }
    .kpi.pink   .value { color:var(--pink); }

    .dgrid { display:grid; grid-template-columns:repeat(12,1fr); gap:16px; margin-bottom:16px; }
    .dcard { background:var(--panel); border:1px solid var(--line); border-radius:10px; padding:16px; }
    .dcard h2 { margin:0 0 12px; font-size:12px; font-weight:600;
                color:var(--muted); text-transform:uppercase; letter-spacing:1px; }
    .span-3  { grid-column:span 3; }
    .span-4  { grid-column:span 4; }
    .span-6  { grid-column:span 6; }
    .span-8  { grid-column:span 8; }
    .span-12 { grid-column:span 12; }
    @media (max-width:1100px) {
      .span-3, .span-4 { grid-column:span 6; }
      .span-6, .span-8 { grid-column:span 12; }
    }
    @media (max-width:640px) {
      .span-3, .span-4, .span-6, .span-8 { grid-column:span 12; }
    }
    .chart-box      { position:relative; height:220px; }
    .chart-box-tall { position:relative; height:300px; }
    .recent-runs td { font-size:12.5px; }
    .recent-runs td.mono { font-family:ui-monospace,SFMono-Regular,Menlo,monospace; }
  </style>
</head>
<body>
  <div id="login">
    <form class="card" id="loginForm" autocomplete="on">
      <div class="logo">O</div>
      <h2>Orcha Admin</h2>
      <p class="sub">Sign in to manage plugins and jobs.</p>
      <div class="field"><label for="user">Username</label>
        <input id="user" type="text" name="username" autocomplete="username" required /></div>
      <div class="field"><label for="pass">Password</label>
        <input id="pass" type="password" name="current-password" autocomplete="current-password" required /></div>
      <button class="btn-primary" type="submit" id="signin">Sign in</button>
      <div class="login-err" id="loginErr"></div>
      <div class="theme-ctl" style="margin-top:18px; justify-content:center;">
        <span>Theme</span>
        <select class="theme-select" aria-label="Theme">
          <option value="system">System</option><option value="light">Light</option><option value="dark">Dark</option>
        </select>
      </div>
    </form>
  </div>

  <div id="app">
    <header>
      <h1>Orcha Admin</h1>
      <nav>
        <button data-view="overview" class="active">Overview</button>
        <button data-view="plugins">Plugins</button>
        <button data-view="jobs">Jobs</button>
      </nav>
      <span class="spacer"></span>
      <label class="theme-ctl"><span>Theme</span>
        <select class="theme-select" aria-label="Theme">
          <option value="system">System</option><option value="light">Light</option><option value="dark">Dark</option>
        </select></label>
      <span class="muted" id="who"></span>
      <button class="btn" id="logout">Sign out</button>
    </header>
    <main>
      <!-- Overview view -->
      <section class="view active" id="view-overview">
        <div class="toolbar">
          <button class="btn" id="refreshOverview">Refresh</button>
          <span class="muted" id="overviewMeta"></span>
        </div>
        <section class="kpis" id="kpis"></section>
        <section class="dgrid">
          <div class="dcard span-3"><h2>Plugin status</h2>
            <div class="chart-box"><canvas id="chPlugins"></canvas></div></div>
          <div class="dcard span-3"><h2>Job status</h2>
            <div class="chart-box"><canvas id="chJobs"></canvas></div></div>
          <div class="dcard span-3"><h2>Run outcomes</h2>
            <div class="chart-box"><canvas id="chOutcomes"></canvas></div></div>
          <div class="dcard span-3"><h2>Run triggers</h2>
            <div class="chart-box"><canvas id="chTriggers"></canvas></div></div>
        </section>
        <section class="dgrid">
          <div class="dcard span-12"><h2>Runs by day (last 14 days)</h2>
            <div class="chart-box-tall"><canvas id="chRuns"></canvas></div></div>
        </section>
        <section class="dgrid">
          <div class="dcard span-12"><h2>Recent runs</h2>
            <table class="recent-runs">
              <thead><tr><th>When</th><th>Job</th><th>Trigger</th><th>Status</th><th>Error</th></tr></thead>
              <tbody id="recentRuns"></tbody>
            </table></div>
        </section>
      </section>

      <!-- Plugins view -->
      <section class="view" id="view-plugins">
        <div class="toolbar">
          <button class="btn" id="refresh">Refresh</button>
          <span class="muted" id="count"></span>
          <label style="margin-left:auto; display:flex; align-items:center; gap:8px;">
            Watch directory <input type="checkbox" id="watch" /></label>
        </div>
        <table>
          <thead><tr><th>Plugin</th><th>Version</th><th>Status</th><th>Dependencies</th><th>Tags</th><th>Actions</th></tr></thead>
          <tbody id="rows"></tbody>
        </table>
      </section>

      <!-- Jobs view -->
      <section class="view" id="view-jobs">
        <div class="toolbar">
          <button class="btn primary" id="newJob">New job</button>
          <button class="btn" id="refreshJobs">Refresh</button>
          <span class="muted" id="jobsCount"></span>
        </div>
        <div class="grid">
          <div class="panel joblist"><h3>Jobs</h3><div id="jobItems"></div></div>
          <div class="panel"><h3>Detail</h3><div class="pad" id="jobDetail">
            <span class="muted">Select a job to view its workflow and run history.</span></div></div>
        </div>
      </section>
    </main>
  </div>

  <!-- Editor modal -->
  <div id="modal">
    <div class="modal-card">
      <div class="hd" id="modalTitle">New job</div>
      <div class="bd">
        <div class="field"><label>Name</label><input id="jName" type="text" style="width:100%" /></div>
        <div class="field"><label>Description</label><input id="jDesc" type="text" style="width:100%" /></div>
        <div class="field"><label>Schedule (cron, optional &mdash; "m h dom mon dow")</label>
          <div class="cron-row">
            <select id="cronPreset">
              <option value="">Custom / none</option>
              <option value="0 * * * *">Hourly (top of hour)</option>
              <option value="0 */6 * * *">Every 6 hours</option>
              <option value="0 9 * * *">Daily at 09:00</option>
              <option value="0 9 * * 1-5">Weekdays at 09:00</option>
              <option value="0 9 * * 1">Weekly (Mon 09:00)</option>
              <option value="0 0 1 * *">Monthly (1st 00:00)</option>
            </select>
            <input id="jSchedule" type="text" placeholder='e.g. 0 9 * * 1-5  (blank = manual only)' />
          </div>
          <div id="cronPreview" class="cron-preview"></div>
        </div>
        <div class="field"><label style="display:flex; align-items:center; gap:8px; color:var(--fg)">
          <input id="jEnabled" type="checkbox" /> Enabled (the scheduler runs this job when due)</label></div>
        <div class="field"><label>Definition</label>
          <div class="tabs">
            <button class="tab active" type="button" data-deftab="visual">Visual</button>
            <button class="tab" type="button" data-deftab="json">JSON</button>
          </div>
          <div id="defWarn" class="tab-warn" style="display:none"></div>
          <div id="defVisual">
            <div id="defSteps"></div>
            <div class="builder-add">
              <select id="addStepCmd"><option value="">— pick a command —</option></select>
              <button type="button" class="btn" id="addStepBtn">+ Add step</button>
            </div>
          </div>
          <div id="defJson" style="display:none">
            <textarea id="jDef" spellcheck="false"></textarea>
          </div>
        </div>
        <div class="login-err" id="modalErr"></div>
      </div>
      <div class="ft">
        <button class="btn" id="modalCancel">Cancel</button>
        <button class="btn primary" id="modalSave">Save</button>
      </div>
    </div>
  </div>

  <div id="toast"></div>

  <script>
    const $ = (id) => document.getElementById(id);
    const esc = (s) => String(s==null?'':s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');

    // ---------- Theme (multiple synced selectors: header + login) ----------
    const THEME_KEY='orcha_theme';
    function setTheme(t){
      document.documentElement.dataset.theme=t;
      localStorage.setItem(THEME_KEY, t);
      document.querySelectorAll('.theme-select').forEach(s=>{ s.value=t; });
      if (flow.job) flow.render(); // recolor for the new theme
    }
    (function(){
      const t=localStorage.getItem(THEME_KEY)||'system';
      document.documentElement.dataset.theme=t;
      document.querySelectorAll('.theme-select').forEach(s=>{
        s.value=t;
        s.addEventListener('change', e=>setTheme(e.target.value));
      });
    })();

    // ---------- Toast ----------
    function toast(msg, kind){ const t=$('toast'); t.textContent=msg; t.className=kind||'';
      t.style.display='block'; clearTimeout(toast._t); toast._t=setTimeout(()=>t.style.display='none',4000); }

    // ---------- Auth ----------
    const AUTH_KEY='orcha_auth';
    const getAuth=()=>sessionStorage.getItem(AUTH_KEY);
    const setAuth=(v)=>sessionStorage.setItem(AUTH_KEY,v);
    const clearAuth=()=>sessionStorage.removeItem(AUTH_KEY);
    const b64=(s)=>btoa(unescape(encodeURIComponent(s)));

    async function api(path, opts){
      opts=opts||{}; const headers=Object.assign({}, opts.headers||{});
      const auth=getAuth(); if(auth) headers['Authorization']=auth;
      const res=await fetch(path, Object.assign({}, opts, {headers}));
      if(res.status===401){ clearAuth(); showLogin('Your session is invalid or has expired.'); throw new Error('Unauthorized'); }
      let body=null; try{ body=await res.json(); }catch(e){}
      if(!res.ok) throw new Error((body&&body.error)||('HTTP '+res.status));
      return body;
    }
    function showLogin(msg){ document.body.classList.remove('authed'); $('loginErr').textContent=msg||'';
      $('pass').value=''; setTimeout(()=>$('user').focus(),0); }
    function enterApp(){ document.body.classList.add('authed');
      loadOverview(); loadPlugins(); loadJobs(); }

    $('loginForm').addEventListener('submit', async e => {
      e.preventDefault();
      const user=$('user').value.trim(), pass=$('pass').value, btn=$('signin');
      btn.disabled=true; btn.textContent='Signing in...'; $('loginErr').textContent='';
      try {
        const token='Basic '+b64(user+':'+pass);
        const res=await fetch('/api/plugins',{headers:{Authorization:token}});
        if(res.status===401){ $('loginErr').textContent='Invalid username or password.'; return; }
        if(!res.ok){ $('loginErr').textContent='Server error (HTTP '+res.status+').'; return; }
        setAuth(token); $('who').textContent=user; enterApp();
      } catch(err){ $('loginErr').textContent='Could not reach the server.'; }
      finally { btn.disabled=false; btn.textContent='Sign in'; }
    });
    $('logout').addEventListener('click', ()=>{ clearAuth(); $('rows').innerHTML=''; $('jobItems').innerHTML='';
      showLogin('Signed out.'); });

    // ---------- View tabs ----------
    document.querySelectorAll('nav button').forEach(b => b.addEventListener('click', ()=>{
      document.querySelectorAll('nav button').forEach(x=>x.classList.remove('active'));
      b.classList.add('active');
      document.querySelectorAll('.view').forEach(v=>v.classList.remove('active'));
      $('view-'+b.dataset.view).classList.add('active');
      if (b.dataset.view==='jobs' && flow.job) flow.resize();
      if (b.dataset.view==='overview') loadOverview();
    }));

    // ================= Overview =================
    // Build palette from the live CSS custom properties so the charts track
    // whatever theme is active (light/dark/system) without hardcoded hex values.
    function tokenColor(name){
      const v = getComputedStyle(document.documentElement).getPropertyValue('--'+name).trim();
      return v || '#888';
    }
    const overview = {
      charts: {},
      destroy(){ for (const k in this.charts) { this.charts[k]?.destroy?.(); } this.charts = {}; },
      configureChartDefaults(){
        if (!window.Chart) return false;
        Chart.defaults.color = tokenColor('muted');
        Chart.defaults.borderColor = tokenColor('grid');
        Chart.defaults.font.family = getComputedStyle(document.body).fontFamily || 'sans-serif';
        Chart.defaults.responsive = true;
        Chart.defaults.maintainAspectRatio = false;
        return true;
      },
      donut(id, labels, values, colors){
        const ctx = document.getElementById(id); if (!ctx) return;
        this.charts[id] = new Chart(ctx, {
          type: 'doughnut',
          data: { labels, datasets: [{ data: values, backgroundColor: colors, borderWidth: 0 }] },
          options: { cutout:'60%',
                     plugins:{ legend:{ position:'bottom',
                                        labels:{ boxWidth:10, font:{size:11} } } } }
        });
      },
      line(id, labels, opened, closed){
        const ctx = document.getElementById(id); if (!ctx) return;
        this.charts[id] = new Chart(ctx, {
          type: 'line',
          data: { labels,
            datasets: [
              { label:'Success', data:opened,
                borderColor:tokenColor('low'),
                backgroundColor:'color-mix(in srgb, '+tokenColor('low')+' 18%, transparent)',
                fill:true, tension:.3 },
              { label:'Failed', data:closed,
                borderColor:tokenColor('crit'),
                backgroundColor:'color-mix(in srgb, '+tokenColor('crit')+' 14%, transparent)',
                fill:true, tension:.3 },
            ]
          },
          options: { plugins:{ legend:{ position:'bottom' } },
                     scales:{ x:{ grid:{ display:false } },
                              y:{ grid:{ color:tokenColor('grid') },
                                  beginAtZero:true, ticks:{ precision:0 } } } }
        });
      }
    };

    function fmtNum(n){ return (n==null) ? '—' : Number(n).toLocaleString(); }
    function fmtAgo(iso){
      if (!iso) return '—';
      const d = new Date(iso); if (isNaN(d)) return iso;
      const s = (Date.now() - d.getTime()) / 1000;
      if (s < 60)        return Math.floor(s) + 's ago';
      if (s < 3600)      return Math.floor(s/60) + 'm ago';
      if (s < 86400)     return Math.floor(s/3600) + 'h ago';
      if (s < 86400*30)  return Math.floor(s/86400) + 'd ago';
      return d.toISOString().slice(0,10);
    }

    async function loadOverview(){
      // Fail soft: even if one of the three endpoints is missing we still
      // render whatever sections we can.
      let plugins = null, jobs = null, runs = null;
      try { plugins = await api('/api/plugins'); } catch(e){ /* leave null */ }
      try { jobs    = await api('/api/jobs');    } catch(e){}
      try { runs    = await api('/api/runs?limit=200'); } catch(e){}

      // ---- KPI tiles ----
      const pluginList = (plugins && plugins.plugins) || [];
      const loaded   = pluginList.filter(p=>p.status==='loaded').length;
      const disabled = pluginList.filter(p=>p.status==='disabled').length;
      const available= pluginList.filter(p=>p.status==='available').length;

      const jobList = (jobs && jobs.jobs) || [];
      const enabled  = jobList.filter(j=>j.enabled).length;
      const scheduled= jobList.filter(j=>j.schedule_cron).length;

      const runList = (runs && runs.runs) || [];
      const succ = runList.filter(r=>r.status==='success').length;
      const fail = runList.filter(r=>r.status==='failed').length;
      const successRate = runList.length ? Math.round(100 * succ / runList.length) : null;

      const kpiSpecs = [
        { label:'Loaded plugins', value: fmtNum(loaded),    cls:'kev' },
        { label:'Available',      value: fmtNum(available), cls:'info' },
        { label:'Disabled',       value: fmtNum(disabled) },
        { label:'Jobs',           value: fmtNum(jobList.length) },
        { label:'Enabled',        value: fmtNum(enabled),   cls:'low' },
        { label:'Scheduled',      value: fmtNum(scheduled), cls:'violet' },
        { label:'Runs (recent)',  value: fmtNum(runList.length) },
        { label:'Success rate',
          value: successRate==null ? '—' : (successRate + '%'),
          cls: successRate==null ? '' : (successRate>=90?'low':successRate>=60?'med':'crit') },
      ];
      $('kpis').innerHTML = kpiSpecs.map(s =>
        `<div class="kpi ${s.cls||''}"><div class="label">${s.label}</div><div class="value">${s.value}</div></div>`
      ).join('');

      // ---- Charts ----
      overview.destroy();
      if (!overview.configureChartDefaults()) {
        // Chart.js still loading; retry after the script settles.
        setTimeout(loadOverview, 250);
        return;
      }

      overview.donut('chPlugins',
        ['Loaded','Available','Disabled'],
        [loaded, available, disabled],
        [tokenColor('low'), tokenColor('info'), tokenColor('muted')]);

      overview.donut('chJobs',
        ['Enabled','Disabled','Scheduled'],
        [enabled, jobList.length - enabled, scheduled],
        [tokenColor('low'), tokenColor('muted'), tokenColor('violet')]);

      overview.donut('chOutcomes',
        ['Success','Failed'],
        [succ, fail],
        [tokenColor('low'), tokenColor('crit')]);

      // Group runs by trigger.
      const trigCounts = {};
      runList.forEach(r => { const t = r.trigger || 'unknown';
                             trigCounts[t] = (trigCounts[t]||0) + 1; });
      const trigOrder = ['manual','api','schedule','unknown'];
      const trigLabels = trigOrder.filter(t => trigCounts[t]);
      const trigValues = trigLabels.map(t => trigCounts[t]);
      const trigColorMap = { manual:tokenColor('pink'), api:tokenColor('info'),
                             schedule:tokenColor('violet'), unknown:tokenColor('muted') };
      overview.donut('chTriggers', trigLabels, trigValues,
                     trigLabels.map(t => trigColorMap[t] || tokenColor('muted')));

      // Runs by day: bucket the last 14 calendar days (UTC).
      const days = []; const today = new Date(); today.setUTCHours(0,0,0,0);
      for (let i = 13; i >= 0; --i) {
        const d = new Date(today); d.setUTCDate(d.getUTCDate() - i);
        days.push(d.toISOString().slice(0,10));
      }
      const succByDay = Object.fromEntries(days.map(d => [d, 0]));
      const failByDay = Object.fromEntries(days.map(d => [d, 0]));
      runList.forEach(r => {
        const key = (r.started_at || '').slice(0,10);
        if (key in succByDay) {
          if (r.status === 'success') succByDay[key]++;
          else if (r.status === 'failed') failByDay[key]++;
        }
      });
      overview.line('chRuns', days,
        days.map(d => succByDay[d]),
        days.map(d => failByDay[d]));

      // ---- Recent runs table ----
      const jobNameById = Object.fromEntries(jobList.map(j => [j.id, j.name]));
      const recent = runList.slice(0, 10);
      $('recentRuns').innerHTML = recent.length
        ? recent.map(r => {
            const jobName = r.job_id ? (jobNameById[r.job_id] || r.job_id.slice(0,8)+'…')
                                     : '<span class="muted">ad-hoc</span>';
            const statusCls = r.status === 'success' ? 'success'
                            : r.status === 'failed'  ? 'failed' : '';
            const trigCls = r.trigger || 'manual';
            const err = r.error ? esc(r.error).slice(0,80) : '';
            return `<tr><td class="mono">${esc(fmtAgo(r.started_at))}</td>
              <td>${jobName}</td>
              <td><span class="badge ${trigCls}">${esc(r.trigger||'-')}</span></td>
              <td><span class="badge ${statusCls}">${esc(r.status||'-')}</span></td>
              <td class="muted">${err}</td></tr>`;
          }).join('')
        : '<tr><td colspan="5" class="muted">No runs yet.</td></tr>';

      $('overviewMeta').textContent =
        pluginList.length + ' plugin(s) · ' + jobList.length + ' job(s) · '
        + runList.length + ' recent run(s)';
    }
    $('refreshOverview').addEventListener('click', loadOverview);

    // Re-render charts when the theme changes so colors track the palette.
    document.querySelectorAll('.theme-select').forEach(sel =>
      sel.addEventListener('change', () => {
        if ($('view-overview').classList.contains('active')) loadOverview();
      }));

    // ================= Plugins =================
    function pRow(p){
      const deps=(p.dependencies||[]).map(esc).join(', ')||'<span class="muted">-</span>';
      const tags=(p.tags||[]).map(t=>'<span>'+esc(t)+'</span>').join('')||'<span class="muted">-</span>';
      const actions = p.status==='loaded'
        ? `<button class="btn" data-pa="reload" data-name="${esc(p.name)}">Reload</button>
           <button class="btn" data-pa="disable" data-name="${esc(p.name)}">Disable</button>`
        : `<button class="btn" data-pa="enable" data-name="${esc(p.name)}">Enable</button>`;
      return `<tr><td><strong>${esc(p.name)}</strong><br><span class="muted">${esc(p.description||'')}</span></td>
        <td>${esc(p.version||'')}</td><td><span class="badge ${esc(p.status)}">${esc(p.status)}</span></td>
        <td>${deps}</td><td class="tags">${tags}</td><td class="actions">${actions}</td></tr>`;
    }
    async function loadPlugins(){
      try { const d=await api('/api/plugins');
        $('count').textContent=(d.count||0)+' plugin(s), '+((d.commands||[]).length)+' command(s)';
        $('watch').checked=!!d.watching;
        $('rows').innerHTML=(d.plugins||[]).map(pRow).join('')||'<tr><td colspan="6" class="muted">No plugins found.</td></tr>';
      } catch(e){ if(e.message!=='Unauthorized') toast('Failed to load plugins: '+e.message,'err'); }
    }
    async function pluginAct(name, action){
      try { const r=await api('/api/plugins/'+encodeURIComponent(name)+'/'+action,{method:'POST'});
        toast((r&&r.message)||(action+' ok'),'ok'); await loadPlugins();
      } catch(e){ if(e.message!=='Unauthorized') toast(action+' failed: '+e.message,'err'); }
    }
    $('refresh').addEventListener('click', loadPlugins);
    $('watch').addEventListener('change', async e=>{
      try { await api('/api/plugins/_watch',{method:'PUT',headers:{'Content-Type':'application/json'},
              body:JSON.stringify({enabled:e.target.checked})});
        toast('Watcher '+(e.target.checked?'enabled':'disabled'),'ok');
      } catch(err){ if(err.message!=='Unauthorized') toast('Watcher toggle failed: '+err.message,'err');
        e.target.checked=!e.target.checked; }
    });

    // ================= Jobs =================
    let jobs=[], currentJob=null, selectedRunId=null;

    function markSelectedRun(){
      document.querySelectorAll('.runrow').forEach(tr =>
        tr.classList.toggle('sel', tr.dataset.run===selectedRunId));
    }

    async function loadJobs(){
      try { const d=await api('/api/jobs'); jobs=d.jobs||[];
        $('jobsCount').textContent=jobs.length+' job(s)';
        $('jobItems').innerHTML = jobs.length ? jobs.map(j =>
          `<div class="item ${currentJob&&currentJob.id===j.id?'active':''}" data-job="${esc(j.id)}">
             <div class="nm">${esc(j.name)}${j.enabled?'':' <span class="muted">(disabled)</span>'}${j.schedule_cron?' <span class="muted">&#9201;</span>':''}</div>
             <div class="muted">${esc(j.description||'')}</div></div>`).join('')
          : '<div class="pad muted">No jobs yet. Click "New job".</div>';
      } catch(e){ if(e.message!=='Unauthorized') toast('Failed to load jobs: '+e.message,'err'); }
    }

    async function selectJob(id){
      try {
        currentJob = await api('/api/jobs/'+encodeURIComponent(id));
        selectedRunId = null; // re-auto-select the latest run for this job
        document.querySelectorAll('.joblist .item').forEach(x =>
          x.classList.toggle('active', x.dataset.job===id));
        renderJobDetail();
      } catch(e){ if(e.message!=='Unauthorized') toast(e.message,'err'); }
    }

    function renderJobDetail(){
      const j=currentJob; if(!j){ return; }
      const sched = j.schedule_cron
        ? `<code>${esc(j.schedule_cron)}</code>` : '<span class="muted">manual only</span>';
      const enBadge = j.enabled
        ? '<span class="badge success">enabled</span>'
        : '<span class="badge failed">disabled</span>';
      $('jobDetail').innerHTML = `
        <div class="toolbar">
          <strong style="font-size:15px">${esc(j.name)}</strong>
          <span class="muted">${esc(j.description||'')}</span>
          <span style="margin-left:auto"></span>
          <button class="btn primary" id="runJob">Run now</button>
          <button class="btn" id="toggleJob">${j.enabled ? 'Disable' : 'Enable'}</button>
          <button class="btn" id="editJob">Edit</button>
          <button class="btn danger" id="delJob">Delete</button>
        </div>
        <div class="muted" style="margin:-6px 0 12px; display:flex; gap:14px; align-items:center">
          <span>Schedule: ${sched}</span> ${enBadge}
        </div>
        <div class="chartwrap"><canvas id="flow"></canvas><div id="fctip"></div></div>
        <h3 style="margin:18px 0 8px; font-size:12px; text-transform:uppercase; color:var(--muted)">Run output</h3>
        <div id="runOutput" class="muted">Run the job or pick a run below to see each step's output.</div>
        <h3 style="margin:18px 0 8px; font-size:12px; text-transform:uppercase; color:var(--muted)">Run history</h3>
        <div id="runs" class="muted">Loading runs...</div>`;
      $('runJob').onclick = ()=>runJob(j.id);
      $('toggleJob').onclick = ()=>toggleJob(j);
      $('editJob').onclick = ()=>openEditor(j);
      $('delJob').onclick = ()=>delJob(j);
      flow.setJob(j);
      loadRuns(j.id);
    }

    async function toggleJob(j){
      const payload = { name:j.name, description:j.description||'',
                        definition:j.definition, enabled:!j.enabled };
      if(j.schedule_cron) payload.schedule_cron = j.schedule_cron;
      try {
        await api('/api/jobs/'+encodeURIComponent(j.id),
          {method:'PUT', headers:{'Content-Type':'application/json'}, body:JSON.stringify(payload)});
        toast(j.enabled ? 'Disabled' : 'Enabled', 'ok');
        await loadJobs();
        await selectJob(j.id);
      } catch(e){ if(e.message!=='Unauthorized') toast('Toggle failed: '+e.message,'err'); }
    }

    function fmtOutput(o){ try { return JSON.stringify(o, null, 2); } catch(e){ return String(o); } }

    // Render each step's output (and error) for a run.
    function renderRunOutput(run){
      const el=$('runOutput'); if(!el) return;
      const steps=Array.isArray(run.result)?run.result:[];
      if(!steps.length){
        el.innerHTML='<span class="muted">No step output for this run'+
          (run.error?': '+esc(run.error):'')+'.</span>'; return;
      }
      const head=`<div class="muted" style="margin-bottom:8px">Run <code>${esc(run.id.slice(0,8))}</code>
        · ${esc(run.trigger)} · <span class="badge ${esc(run.status)}">${esc(run.status)}</span>
        · ${esc(run.started_at)}</div>`;
      el.innerHTML=head+steps.map((s,i)=>{
        const label='STEP '+(i+1)+(s.name?(' · '+esc(s.name)):'')+' · '+esc(s.command||'');
        const st=s.success?'success':'failed';
        const body=(!s.success && s.error_message)
          ? `<div class="err">${esc(s.error_message)}</div>`+
            (s.output!==undefined?`<pre>${esc(fmtOutput(s.output))}</pre>`:'')
          : `<pre>${esc(fmtOutput(s.output))}</pre>`;
        return `<div class="outcard" id="out-step-${i}"><div class="hd"><strong>${label}</strong>
          <span class="sp"></span><span class="badge ${st}">${st}</span></div>${body}</div>`;
      }).join('');
    }

    async function runJob(id){
      try { const r=await api('/api/jobs/'+encodeURIComponent(id)+'/run',{method:'POST'});
        toast('Run '+r.status,(r.status==='success')?'ok':'err');
        selectedRunId=r.id; flow.applyRun(r); renderRunOutput(r); loadRuns(id);
      } catch(e){ if(e.message!=='Unauthorized') toast('Run failed: '+e.message,'err'); }
    }
    async function delJob(j){
      if(!confirm('Delete job "'+j.name+'"?')) return;
      try { await api('/api/jobs/'+encodeURIComponent(j.id),{method:'DELETE'});
        toast('Deleted','ok'); currentJob=null;
        $('jobDetail').innerHTML='<span class="muted">Select a job to view its workflow and run history.</span>';
        loadJobs();
      } catch(e){ if(e.message!=='Unauthorized') toast('Delete failed: '+e.message,'err'); }
    }
    async function loadRuns(id){
      try { const d=await api('/api/jobs/'+encodeURIComponent(id)+'/runs?limit=20');
        const runs=d.runs||[];
        // Auto-select the latest run (its full result is already in the list).
        if(runs.length && (!selectedRunId || !runs.some(r=>r.id===selectedRunId))){
          selectedRunId=runs[0].id; flow.applyRun(runs[0]); renderRunOutput(runs[0]);
        }
        $('runs').innerHTML = runs.length ? `<table><thead><tr><th>Started</th><th>Trigger</th><th>Status</th></tr></thead>
          <tbody>${runs.map(r=>`<tr class="runrow${r.id===selectedRunId?' sel':''}" data-run="${esc(r.id)}"><td>${esc(r.started_at)}</td>
            <td>${esc(r.trigger)}</td><td><span class="badge ${esc(r.status)}">${esc(r.status)}</span></td></tr>`).join('')}
          </tbody></table>` : '<span class="muted">No runs yet.</span>';
      } catch(e){ if(e.message!=='Unauthorized') $('runs').textContent='Failed to load runs.'; }
    }
    async function showRun(id){
      try { const r=await api('/api/runs/'+encodeURIComponent(id));
        selectedRunId=id; markSelectedRun();
        flow.applyRun(r); renderRunOutput(r);
        toast('Loaded run '+r.id.slice(0,8)+' ('+r.status+')',(r.status==='success')?'ok':'err');
      } catch(e){ if(e.message!=='Unauthorized') toast(e.message,'err'); }
    }

    $('newJob').addEventListener('click', ()=>openEditor(null));
    $('refreshJobs').addEventListener('click', loadJobs);

    // Delegated clicks for plugin actions, job selection, run rows.
    document.addEventListener('click', ev=>{
      const pa=ev.target.closest('button[data-pa]'); if(pa){ pluginAct(pa.dataset.name, pa.dataset.pa); return; }
      const ji=ev.target.closest('.joblist .item'); if(ji){ selectJob(ji.dataset.job); return; }
      const rr=ev.target.closest('.runrow'); if(rr){ showRun(rr.dataset.run); return; }
    });

    // ---------- Editor modal ----------
    let editingId=null;
    let manifests=null;       // { [command_name]: pluginMeta }
    let allCommands=[];        // string[] of all registered commands
    let editorVisual=[];       // [{command, name?, params}]
    let editorTab='visual';
    const DEFAULT_DEF = JSON.stringify({steps:[
      {name:"greet", command:"echo", params:{message:"Hello from Orcha"}},
      {name:"repeat", command:"echo", params:{message:"greet said: {{steps.greet.output.echoed}}"}}
    ]}, null, 2);

    async function loadManifests(){
      if(manifests) return;
      const d = await api('/api/plugins');
      manifests = {};
      (d.plugins||[]).forEach(p=>{ if(p.name) manifests[p.name]=p; });
      allCommands = (d.commands||[]).slice().sort();
      // Populate the add-step dropdown.
      const sel = $('addStepCmd');
      sel.innerHTML = '<option value="">— pick a command —</option>' +
        allCommands.map(c=>{
          const m=manifests[c];
          if(m) return `<option value="${esc(c)}">${esc(c)}${m.description?(' — '+esc(m.description)):''}</option>`;
          return `<option value="${esc(c)}" disabled>${esc(c)} (no manifest — use JSON tab)</option>`;
        }).join('');
    }

    // Manifest helpers -------------------------------------------------------
    function paramSpec(cmd, pname){
      const m=manifests && manifests[cmd];
      if(!m || !m.parameters) return null;
      return m.parameters.find(p=>p.name===pname) || null;
    }
    function paramDefault(p){
      // Manifest "default" is always a string; coerce by declared type.
      if(p.default===undefined || p.default===null) {
        if(p.type==='bool') return false;
        if(p.type==='int') return '';
        if(p.type==='object') return {};
        return '';
      }
      if(p.type==='bool') return String(p.default).toLowerCase()==='true';
      if(p.type==='int'){ const n=parseInt(p.default,10); return isNaN(n)?'':n; }
      if(p.type==='object'){ try { return JSON.parse(p.default); } catch(e){ return {}; } }
      return p.default;
    }
    function defaultParamsFor(cmd){
      const m=manifests && manifests[cmd]; const out={};
      if(!m || !m.parameters) return out;
      m.parameters.forEach(p=>{ if(p.required || p.default!==undefined) out[p.name]=paramDefault(p); });
      return out;
    }

    // Visual <-> JSON sync ---------------------------------------------------
    function visualToJson(){
      const steps = editorVisual.map(s=>{
        const o={};
        if(s.name) o.name=s.name;
        o.command=s.command;
        o.params=s.params||{};
        return o;
      });
      $('jDef').value = JSON.stringify({steps}, null, 2);
    }
    function jsonToVisual(){
      const raw=$('jDef').value.trim();
      if(!raw){ editorVisual=[]; return {ok:true}; }
      let parsed;
      try { parsed=JSON.parse(raw); } catch(e){ return {ok:false, err:'Invalid JSON: '+e.message}; }
      if(!parsed || !Array.isArray(parsed.steps)) return {ok:false, err:'Expected an object with a "steps" array.'};
      editorVisual = parsed.steps.map(s=>({
        name: s.name || '',
        command: s.command || '',
        params: (s.params && typeof s.params==='object' && !Array.isArray(s.params)) ? s.params : {}
      }));
      return {ok:true};
    }

    // Renderers --------------------------------------------------------------
    function renderVisual(){
      const host=$('defSteps');
      if(!editorVisual.length){
        host.innerHTML = '<div class="empty-hint">No steps yet. Pick a command below and click <em>+ Add step</em> — or switch to the JSON tab.</div>';
        return;
      }
      host.innerHTML = editorVisual.map((s,i)=>renderStepCard(s,i)).join('');
    }
    function renderStepCard(s, i){
      const m = manifests && manifests[s.command];
      const cmdLabel = esc(s.command || '(no command)') + (m ? '' : ' <span class="muted" title="No manifest for this command">(raw)</span>');
      const params = m && m.parameters ? m.parameters.map(p=>renderParamField(i, p, s.params[p.name])).join('') : renderRawParams(i, s.params);
      return `<div class="step-card" data-step="${i}">
        <div class="shd">
          <span class="lbl">Step ${i+1}</span>
          <span class="cmd">· ${cmdLabel}</span>
          <span class="sp"></span>
          <button type="button" data-act="up"   data-i="${i}" ${i===0?'disabled':''} title="Move up">↑</button>
          <button type="button" data-act="down" data-i="${i}" ${i===editorVisual.length-1?'disabled':''} title="Move down">↓</button>
          <button type="button" class="danger" data-act="del" data-i="${i}" title="Remove">🗑</button>
        </div>
        <div class="param">
          <div class="pname">name <span class="muted" title="Used for {{steps.NAME.output...}} references">(optional)</span></div>
          <div><input type="text" data-bind="name" data-i="${i}" value="${esc(s.name||'')}" placeholder="e.g. greet" /></div>
        </div>
        ${params}
      </div>`;
    }
    function renderParamField(i, p, value){
      const req = p.required ? ' <span class="req">*</span>' : '';
      const title = p.description ? ` title="${esc(p.description)}"` : '';
      const label = `<div class="pname"${title}>${esc(p.name)}${req}</div>`;
      let ctrl='';
      const v = (value===undefined) ? paramDefault(p) : value;
      if(p.type==='bool'){
        ctrl = `<div><input type="checkbox" data-bind="param" data-pname="${esc(p.name)}" data-ptype="bool" data-i="${i}" ${v?'checked':''} /></div>`;
      } else if(p.type==='int'){
        ctrl = `<div><input type="number" data-bind="param" data-pname="${esc(p.name)}" data-ptype="int" data-i="${i}" value="${v===''?'':esc(String(v))}" placeholder="${esc(p.example||'')}" /></div>`;
      } else if(p.type==='object'){
        ctrl = `<div>${renderObjectRows(i, p.name, (v && typeof v==='object') ? v : {})}</div>`;
      } else { // string (default)
        ctrl = `<div><input type="text" data-bind="param" data-pname="${esc(p.name)}" data-ptype="string" data-i="${i}" value="${esc(v==null?'':String(v))}" placeholder="${esc(p.example||'')}" /></div>`;
      }
      return `<div class="param">${label}${ctrl}</div>`;
    }
    function renderObjectRows(i, pname, obj){
      const entries=Object.entries(obj||{});
      const rows = entries.map(([k,val],ri)=>`
        <div class="kv-row" data-ri="${ri}">
          <input type="text" data-bind="kvk" data-i="${i}" data-pname="${esc(pname)}" data-ri="${ri}" value="${esc(k)}" placeholder="header" />
          <input type="text" data-bind="kvv" data-i="${i}" data-pname="${esc(pname)}" data-ri="${ri}" value="${esc(val==null?'':String(val))}" placeholder="value" />
          <button type="button" class="x" data-act="kvdel" data-i="${i}" data-pname="${esc(pname)}" data-ri="${ri}" title="Remove">×</button>
        </div>`).join('');
      return `<div class="kv-rows">${rows}
        <button type="button" class="kv-add" data-act="kvadd" data-i="${i}" data-pname="${esc(pname)}">+ Add</button></div>`;
    }
    function renderRawParams(i, params){
      // Unknown command: render a single raw-JSON textarea for params.
      const txt = JSON.stringify(params||{}, null, 2);
      return `<div class="param">
        <div class="pname">params <span class="muted">(JSON)</span></div>
        <div><textarea data-bind="rawparams" data-i="${i}" style="width:100%;min-height:80px" spellcheck="false">${esc(txt)}</textarea></div>
      </div>`;
    }

    // Visual-tab event handling ---------------------------------------------
    function rerenderAndSync(){ renderVisual(); visualToJson(); }
    $('defSteps').addEventListener('input', ev=>{
      const t=ev.target;
      const i=parseInt(t.dataset.i,10);
      if(isNaN(i) || !editorVisual[i]) return;
      const bind=t.dataset.bind;
      if(bind==='name'){ editorVisual[i].name = t.value; }
      else if(bind==='param'){
        const pname=t.dataset.pname, ptype=t.dataset.ptype;
        if(ptype==='bool') editorVisual[i].params[pname] = t.checked;
        else if(ptype==='int'){ const n=parseInt(t.value,10); editorVisual[i].params[pname] = isNaN(n) ? '' : n; }
        else editorVisual[i].params[pname] = t.value;
      } else if(bind==='kvk' || bind==='kvv'){
        const pname=t.dataset.pname, ri=parseInt(t.dataset.ri,10);
        const obj=editorVisual[i].params[pname] || (editorVisual[i].params[pname]={});
        const entries=Object.entries(obj);
        // ri may exceed entries length when adding; just rebuild from DOM.
        const card=t.closest('.step-card');
        const rows=card.querySelectorAll(`.kv-row`);
        const next={};
        rows.forEach((row,idx)=>{
          const k=row.querySelector(`input[data-bind="kvk"]`).value;
          const v=row.querySelector(`input[data-bind="kvv"]`).value;
          if(k!=='') next[k]=v;
        });
        editorVisual[i].params[pname]=next;
      } else if(bind==='rawparams'){
        try { editorVisual[i].params = JSON.parse(t.value || '{}'); } catch(e){ /* keep typing */ }
      }
      visualToJson();
    });
    $('defSteps').addEventListener('click', ev=>{
      const b=ev.target.closest('button[data-act]'); if(!b) return;
      const i=parseInt(b.dataset.i,10); if(isNaN(i)) return;
      const act=b.dataset.act;
      if(act==='up' && i>0){ const t=editorVisual[i-1]; editorVisual[i-1]=editorVisual[i]; editorVisual[i]=t; rerenderAndSync(); }
      else if(act==='down' && i<editorVisual.length-1){ const t=editorVisual[i+1]; editorVisual[i+1]=editorVisual[i]; editorVisual[i]=t; rerenderAndSync(); }
      else if(act==='del'){ editorVisual.splice(i,1); rerenderAndSync(); }
      else if(act==='kvadd'){
        const pname=b.dataset.pname;
        const obj=editorVisual[i].params[pname] || (editorVisual[i].params[pname]={});
        // Append a fresh empty key under a placeholder name so the row renders.
        let key='', n=1; while((''+(n)) in obj){ n++; } key=''+n;
        // Use empty string key — but objects can't have duplicate empty keys.
        // Render a temp row by storing under a unique non-empty key, then user edits.
        obj['']=obj['']!==undefined?obj['']:'';
        rerenderAndSync();
      }
      else if(act==='kvdel'){
        const pname=b.dataset.pname, ri=parseInt(b.dataset.ri,10);
        const obj=editorVisual[i].params[pname]||{};
        const keys=Object.keys(obj);
        if(ri>=0 && ri<keys.length){ delete obj[keys[ri]]; rerenderAndSync(); }
      }
    });
    $('addStepBtn').addEventListener('click', ()=>{
      const cmd=$('addStepCmd').value;
      if(!cmd){ toast('Pick a command first','err'); return; }
      editorVisual.push({ name:'', command:cmd, params:defaultParamsFor(cmd) });
      $('addStepCmd').value='';
      rerenderAndSync();
    });

    // Tab switching ----------------------------------------------------------
    function setDefTab(name){
      if(name==='visual'){
        const r = jsonToVisual();
        if(!r.ok){
          $('defWarn').textContent = "Can't switch to Visual: "+r.err+" — fix the JSON or stay on this tab.";
          $('defWarn').style.display='block';
          return; // stay on JSON
        }
        $('defWarn').style.display='none';
        renderVisual();
      } else {
        // Switching to JSON: ensure JSON is current (visual is source while on visual).
        visualToJson();
        $('defWarn').style.display='none';
      }
      editorTab = name;
      $('defVisual').style.display = (name==='visual') ? '' : 'none';
      $('defJson').style.display   = (name==='json')   ? '' : 'none';
      document.querySelectorAll('.tab[data-deftab]').forEach(t=>{
        t.classList.toggle('active', t.dataset.deftab===name);
      });
    }
    document.querySelectorAll('.tab[data-deftab]').forEach(t=>{
      t.addEventListener('click', ()=>setDefTab(t.dataset.deftab));
    });

    // Cron helpers -----------------------------------------------------------
    function cronField(spec, lo, hi){
      const set=new Set(); const wildcard = (spec==='*');
      spec.split(',').forEach(part=>{
        let step=1, range=part;
        const slash=part.indexOf('/');
        if(slash>=0){ step=parseInt(part.slice(slash+1),10); range=part.slice(0,slash); }
        let a, b;
        if(range==='*'){ a=lo; b=hi; }
        else if(range.includes('-')){ const [x,y]=range.split('-'); a=parseInt(x,10); b=parseInt(y,10); }
        else { a=b=parseInt(range,10); }
        if(isNaN(a)||isNaN(b)||isNaN(step)||step<=0) throw new Error('bad field: '+spec);
        for(let v=a; v<=b; v+=step) if(v>=lo && v<=hi) set.add(v);
      });
      if(lo===0 && hi===6 && set.has(7)){ set.delete(7); set.add(0); } // dow: 7→0
      return {set, wildcard};
    }
    function cronParse(expr){
      const parts=expr.trim().split(/\s+/);
      if(parts.length!==5) throw new Error('Expected 5 fields (m h dom mon dow)');
      const ranges=[[0,59],[0,23],[1,31],[1,12],[0,6]];
      return parts.map((p,i)=>cronField(p, ranges[i][0], ranges[i][1]));
    }
    function cronNext(expr, n){
      const [m,h,dom,mon,dow]=cronParse(expr);
      const out=[]; const d=new Date();
      d.setSeconds(0,0); d.setMinutes(d.getMinutes()+1);
      const limit = 366*24*60;
      for(let i=0; i<limit && out.length<n; i++){
        const dayOK = (!dom.wildcard && !dow.wildcard)
          ? (dom.set.has(d.getDate()) || dow.set.has(d.getDay()))
          : (!dom.wildcard ? dom.set.has(d.getDate())
             : (!dow.wildcard ? dow.set.has(d.getDay()) : true));
        if(m.set.has(d.getMinutes()) && h.set.has(d.getHours()) &&
           mon.set.has(d.getMonth()+1) && dayOK){
          out.push(new Date(d));
        }
        d.setMinutes(d.getMinutes()+1);
      }
      return out;
    }
    function cronFmt(d){
      const p=n=>String(n).padStart(2,'0');
      return d.getFullYear()+'-'+p(d.getMonth()+1)+'-'+p(d.getDate())+' '+p(d.getHours())+':'+p(d.getMinutes());
    }
    function updateCronPreview(){
      const el=$('cronPreview'); const expr=$('jSchedule').value.trim();
      if(!expr){ el.className='cron-preview'; el.textContent='Manual only (no schedule).'; return; }
      try {
        const runs=cronNext(expr, 3);
        if(!runs.length){ el.className='cron-preview err'; el.textContent='No upcoming runs within a year.'; return; }
        el.className='cron-preview';
        el.textContent = 'Next: ' + runs.map(cronFmt).join(' · ');
      } catch(e){
        el.className='cron-preview err'; el.textContent='Invalid cron: '+e.message;
      }
    }
    let cronDeb=null;
    $('jSchedule').addEventListener('input', ()=>{
      clearTimeout(cronDeb); cronDeb=setTimeout(updateCronPreview, 150);
      // Update preset dropdown if expr happens to match a preset.
      const v=$('jSchedule').value.trim();
      const opt=Array.from($('cronPreset').options).find(o=>o.value===v);
      $('cronPreset').value = opt ? v : '';
    });
    $('cronPreset').addEventListener('change', e=>{
      $('jSchedule').value = e.target.value;
      updateCronPreview();
    });

    // Open / save -----------------------------------------------------------
    async function openEditor(job){
      editingId = job ? job.id : null;
      $('modalTitle').textContent = job ? 'Edit job' : 'New job';
      $('jName').value = job ? job.name : '';
      $('jDesc').value = job ? (job.description||'') : '';
      $('jSchedule').value = (job && job.schedule_cron) ? job.schedule_cron : '';
      $('cronPreset').value = ''; // sync may not be exact; user can repick
      $('jEnabled').checked = job ? !!job.enabled : true;
      $('jDef').value  = job ? JSON.stringify(job.definition, null, 2) : DEFAULT_DEF;
      $('modalErr').textContent='';
      $('defWarn').style.display='none';
      $('modal').classList.add('show');
      updateCronPreview();

      // Load manifests, then try to populate visual. On parse failure, go to JSON tab.
      try { await loadManifests(); }
      catch(e){ if(e.message!=='Unauthorized') toast('Failed to load command list: '+e.message,'err'); }

      const r = jsonToVisual();
      if(r.ok){
        editorTab='visual';
        $('defVisual').style.display=''; $('defJson').style.display='none';
        document.querySelectorAll('.tab[data-deftab]').forEach(t=>{
          t.classList.toggle('active', t.dataset.deftab==='visual');
        });
        renderVisual();
      } else {
        editorTab='json';
        $('defVisual').style.display='none'; $('defJson').style.display='';
        document.querySelectorAll('.tab[data-deftab]').forEach(t=>{
          t.classList.toggle('active', t.dataset.deftab==='json');
        });
        $('defWarn').textContent = "Couldn't parse this definition into the Visual builder: "+r.err+" — edit JSON directly, or fix and switch to Visual.";
        $('defWarn').style.display='block';
      }
    }
    $('modalCancel').addEventListener('click', ()=>$('modal').classList.remove('show'));
    $('modalSave').addEventListener('click', async ()=>{
      // If we're on Visual, ensure JSON reflects the current visual state.
      if(editorTab==='visual') visualToJson();
      let def;
      try { def=JSON.parse($('jDef').value); }
      catch(e){ $('modalErr').textContent='Definition is not valid JSON: '+e.message; return; }
      const payload={ name:$('jName').value.trim(), description:$('jDesc').value.trim(),
                      definition:def, enabled:$('jEnabled').checked };
      const sched=$('jSchedule').value.trim();
      if(sched) payload.schedule_cron=sched;   // omitted => cleared (manual only)
      if(!payload.name){ $('modalErr').textContent='Name is required.'; return; }
      try {
        if(editingId){ await api('/api/jobs/'+encodeURIComponent(editingId),
            {method:'PUT',headers:{'Content-Type':'application/json'},body:JSON.stringify(payload)}); }
        else { await api('/api/jobs',
            {method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(payload)}); }
        $('modal').classList.remove('show'); toast('Saved','ok');
        await loadJobs();
        if(editingId) await selectJob(editingId);
      } catch(e){ if(e.message!=='Unauthorized') $('modalErr').textContent=e.message; }
    });

    // ================= Flow chart (Canvas) =================
    const flow = (function(){
      const NODE_W=158, NODE_H=46, COL_GAP=66, ROW_GAP=22, PAD=20;
      let canvas=null, ctx=null, tip=null;
      let nodes=[], edges=[], job=null, runStatus=null; // runStatus: array of bool|null per step
      let hover=-1, sel=-1, drag=-1, dragDX=0, dragDY=0, dpr=1, downX=0, downY=0;
      let onSelect=null; // callback(stepIndex) on node click (not drag)

      function cssVar(n){ return getComputedStyle(document.documentElement).getPropertyValue(n).trim(); }

      function parse(def){
        const steps=(def&&def.steps)||[];
        const n=steps.length;
        const nameToIndex={};
        for(let i=0;i<n;i++){ if(steps[i].name) nameToIndex[steps[i].name]=i; }
        const deps=Array.from({length:n},()=>new Set());
        for(let i=0;i<n;i++){
          const s=JSON.stringify(steps[i].params||{}); let m;
          // positional {{stepN.output}}
          const re=/\{\{\s*step(\d+)\b/g;
          while((m=re.exec(s))){ const k=parseInt(m[1],10)-1; if(k>=0&&k<n&&k!==i) deps[i].add(k); }
          // named {{steps.<name>.output}}
          const reN=/\{\{\s*steps\.([A-Za-z_]\w*)/g;
          while((m=reN.exec(s))){ const k=nameToIndex[m[1]]; if(k!==undefined&&k!==i) deps[i].add(k); }
        }
        const level=new Array(n).fill(0);
        for(let it=0;it<n;it++) for(let i=0;i<n;i++) for(const d of deps[i]) level[i]=Math.max(level[i],level[d]+1);
        const rowOf={}; nodes=[]; edges=[];
        for(let i=0;i<n;i++){
          const L=level[i]; rowOf[L]=(rowOf[L]||0);
          const row=rowOf[L]++;
          nodes.push({ i, name:(steps[i].name||''), cmd:(steps[i].command||'?'), params:steps[i].params||{},
            x:PAD+L*(NODE_W+COL_GAP), y:PAD+row*(NODE_H+ROW_GAP) });
        }
        for(let i=0;i<n;i++) for(const d of deps[i]) edges.push([d,i]);
      }

      function resize(){
        if(!canvas) return;
        let maxX=300, maxY=120;
        nodes.forEach(nd=>{ maxX=Math.max(maxX, nd.x+NODE_W+PAD); maxY=Math.max(maxY, nd.y+NODE_H+PAD); });
        dpr=window.devicePixelRatio||1;
        canvas.style.width=maxX+'px'; canvas.style.height=maxY+'px';
        canvas.width=maxX*dpr; canvas.height=maxY*dpr;
        ctx.setTransform(dpr,0,0,dpr,0,0);
        render();
      }

      function nodeColor(nd){
        if(runStatus){ const st=runStatus[nd.i];
          if(st===true) return {bg:'color-mix', stroke:cssVar('--ok')};
          if(st===false) return {bg:'color-mix', stroke:cssVar('--err')}; }
        return {bg:null, stroke: (nd.i===sel)?cssVar('--accent'):cssVar('--line')};
      }

      function roundRect(x,y,w,h,r){ ctx.beginPath(); ctx.moveTo(x+r,y);
        ctx.arcTo(x+w,y,x+w,y+h,r); ctx.arcTo(x+w,y+h,x,y+h,r);
        ctx.arcTo(x,y+h,x,y,r); ctx.arcTo(x,y,x+w,y,r); ctx.closePath(); }

      function render(){
        if(!ctx) return;
        ctx.clearRect(0,0,canvas.width,canvas.height);
        const line=cssVar('--line'), fg=cssVar('--fg'), muted=cssVar('--muted'),
              panel=cssVar('--panel'), accent=cssVar('--accent');
        // edges (use --muted, not --line: --line is too faint against the chart bg)
        ctx.lineWidth=2;
        edges.forEach(([a,b])=>{
          const na=nodes[a], nb=nodes[b];
          const x1=na.x+NODE_W, y1=na.y+NODE_H/2, x2=nb.x, y2=nb.y+NODE_H/2;
          const hl = (hover===a||hover===b||sel===a||sel===b);
          ctx.strokeStyle = hl?accent:muted;
          ctx.beginPath(); ctx.moveTo(x1,y1);
          const mx=(x1+x2)/2; ctx.bezierCurveTo(mx,y1,mx,y2,x2,y2); ctx.stroke();
          // arrowhead (points left into the target node's left edge)
          ctx.fillStyle=hl?accent:muted;
          ctx.beginPath(); ctx.moveTo(x2,y2); ctx.lineTo(x2-8,y2-5); ctx.lineTo(x2-8,y2+5); ctx.closePath(); ctx.fill();
        });
        // nodes
        nodes.forEach(nd=>{
          const c=nodeColor(nd);
          roundRect(nd.x,nd.y,NODE_W,NODE_H,8);
          ctx.fillStyle=panel; ctx.fill();
          if(runStatus && runStatus[nd.i]!==null && runStatus[nd.i]!==undefined){
            roundRect(nd.x,nd.y,NODE_W,NODE_H,8);
            ctx.fillStyle = (runStatus[nd.i]? 'rgba(63,185,80,.14)':'rgba(248,81,73,.14)'); ctx.fill();
          }
          ctx.lineWidth=(nd.i===sel||nd.i===hover)?2:1.2;
          ctx.strokeStyle=(nd.i===hover)?accent:c.stroke; ctx.stroke();
          ctx.fillStyle=muted; ctx.font='600 11px -apple-system,sans-serif';
          let tag='STEP '+(nd.i+1)+(nd.name?(' · '+nd.name):'');
          if(tag.length>22) tag=tag.slice(0,21)+'…';
          ctx.fillText(tag, nd.x+12, nd.y+17);
          ctx.fillStyle=fg; ctx.font='13px -apple-system,sans-serif';
          let label=nd.cmd; if(label.length>18) label=label.slice(0,17)+'…';
          ctx.fillText(label, nd.x+12, nd.y+34);
        });
      }

      function hit(mx,my){ for(let i=nodes.length-1;i>=0;i--){ const n=nodes[i];
        if(mx>=n.x&&mx<=n.x+NODE_W&&my>=n.y&&my<=n.y+NODE_H) return i; } return -1; }

      function bind(){
        canvas.addEventListener('mousedown', e=>{ const r=canvas.getBoundingClientRect();
          const mx=e.clientX-r.left, my=e.clientY-r.top; const i=hit(mx,my);
          sel=i; downX=e.clientX; downY=e.clientY;
          if(i>=0){ drag=i; dragDX=mx-nodes[i].x; dragDY=my-nodes[i].y; } render(); });
        canvas.addEventListener('mousemove', e=>{ const r=canvas.getBoundingClientRect();
          const mx=e.clientX-r.left, my=e.clientY-r.top;
          if(drag>=0){ nodes[drag].x=mx-dragDX; nodes[drag].y=my-dragDY; render(); return; }
          const i=hit(mx,my);
          if(i!==hover){ hover=i; canvas.style.cursor=i>=0?'grab':'default'; render(); }
          if(i>=0){ const nd=nodes[i];
            tip.style.display='block'; tip.style.left=(nd.x+NODE_W+8)+'px'; tip.style.top=nd.y+'px';
            tip.textContent=(nd.name?('"'+nd.name+'"  '):'')+'step'+(i+1)+' · '+nd.cmd+'\n'+JSON.stringify(nd.params,null,1).slice(0,260);
          } else tip.style.display='none';
        });
        window.addEventListener('mouseup', e=>{
          const moved = Math.abs(e.clientX-downX) + Math.abs(e.clientY-downY) > 4;
          if(!moved && sel>=0 && onSelect) onSelect(sel); // click (not drag) -> jump to output
          drag=-1;
        });
        canvas.addEventListener('mouseleave', ()=>{ hover=-1; tip.style.display='none'; render(); });
      }

      return {
        get job(){ return job; },
        setOnSelect(fn){ onSelect=fn; },
        setJob(j){ job=j; runStatus=null; sel=-1; hover=-1;
          canvas=$('flow'); ctx=canvas.getContext('2d'); tip=$('fctip');
          parse(j.definition); bind(); resize(); },
        applyRun(run){ // color nodes from a run's step results
          const arr=Array.isArray(run.result)?run.result:[];
          runStatus=nodes.map(nd=>{ const sr=arr[nd.i]; return sr? !!sr.success : null; });
          render();
        },
        resize, render
      };
    })();

    // Clicking a flow-chart node jumps to (and flashes) that step's output card.
    flow.setOnSelect(function(i){
      const card=document.getElementById('out-step-'+i);
      if(!card) return;
      card.scrollIntoView({behavior:'smooth', block:'center'});
      card.classList.add('flash');
      setTimeout(()=>card.classList.remove('flash'), 1200);
    });

    // ---------- Boot ----------
    if(getAuth()) enterApp(); else showLogin();
  </script>
</body>
</html>
)HTML";

} // namespace Orcha::Agent

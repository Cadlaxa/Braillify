#ifndef UPDATE_PAGE_H
#define UPDATE_PAGE_H

#include <Arduino.h>

const char update_html[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>Braillify Firmware Manager</title>
  <link rel="icon" type="image/png" href="/braillify icon.png">
  <style>
    :root {
      --bg: #f4f5ff;
      --panel: #ffffff;
      --text: #111827;
      --border: #111827;
      --orange: #ffaa29;
      --orange-dark: #e65100;
      --blue-light: #e3f2fd;
      --blue: #0288d1;
      --green: #2e7d32;
      --green-bg: #e8f5e9;
      --red: #c62828;
      --red-bg: #ffebee;
      --purple: #6a1b9a;
      --purple-bg: #f3e5f5;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: system-ui, -apple-system, sans-serif; }
    body {
      background: var(--bg);
      color: var(--text);
      min-height: 100dvh;
      display: flex;
      justify-content: center;
      align-items: center;
      padding: 16px;
    }
    .card {
      width: 100%;
      max-width: 460px;
      background: var(--panel);
      border: 3px solid var(--border);
      box-shadow: 6px 6px 0px var(--border);
      border-radius: 12px;
      padding: 20px;
    }
    .header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      border-bottom: 2px dashed var(--border);
      padding-bottom: 12px;
      margin-bottom: 16px;
    }
    .header h1 {
      font-size: 1.3rem;
      font-weight: 900;
      font-family: 'Courier New', monospace;
      letter-spacing: 0.5px;
    }
    .header .sub {
      font-size: 0.72rem;
      font-weight: 800;
      color: var(--orange-dark);
    }
    .ver-badge {
      background: var(--blue-light);
      color: var(--blue);
      border: 2px solid var(--border);
      border-radius: 8px;
      padding: 4px 10px;
      font-size: 0.78rem;
      font-weight: 900;
      font-family: 'Courier New', monospace;
      box-shadow: 2px 2px 0px var(--border);
    }
    .section-box {
      background: var(--bg);
      border: 2px solid var(--border);
      border-radius: 10px;
      padding: 14px;
      margin-bottom: 14px;
    }
    .section-title {
      display: flex;
      justify-content: space-between;
      align-items: center;
      font-size: 0.85rem;
      font-weight: 900;
      margin-bottom: 10px;
    }
    .refresh-link {
      background: var(--panel);
      border: 1.5px solid var(--border);
      border-radius: 6px;
      padding: 3px 8px;
      font-size: 0.72rem;
      font-weight: 800;
      cursor: pointer;
      box-shadow: 1px 2px 0px var(--border);
    }
    .refresh-link:active { transform: translate(1px, 1px); box-shadow: none; }
    select.ver-select {
      width: 100%;
      padding: 10px;
      border: 2px solid var(--border);
      border-radius: 8px;
      background: var(--panel);
      color: var(--text);
      font-size: 0.84rem;
      font-weight: 800;
      font-family: 'Courier New', monospace;
      margin-bottom: 8px;
      cursor: pointer;
    }
    .rel-meta {
      display: flex;
      justify-content: space-between;
      align-items: center;
      gap: 8px;
      margin-bottom: 8px;
      font-size: 0.74rem;
      font-weight: 700;
      color: #57606a;
    }
    .action-tag {
      padding: 2px 8px;
      border-radius: 999px;
      border: 1.5px solid var(--border);
      font-size: 0.7rem;
      font-weight: 900;
    }
    .tag-upgrade { background: var(--green-bg); color: var(--green); }
    .tag-downgrade { background: var(--purple-bg); color: var(--purple); }
    .tag-current { background: var(--blue-light); color: var(--blue); }
    .rel-notes {
      background: var(--panel);
      border: 1.5px solid var(--border);
      border-radius: 6px;
      padding: 8px 10px;
      font-size: 0.75rem;
      line-height: 1.4;
      color: #374151;
      max-height: 78px;
      overflow-y: auto;
      margin-bottom: 10px;
      white-space: pre-wrap;
    }
    .drop-zone {
      border: 2px dashed var(--border);
      background: var(--panel);
      border-radius: 8px;
      padding: 16px 12px;
      text-align: center;
      cursor: pointer;
      transition: background 0.15s, border-color 0.15s;
      margin-bottom: 10px;
    }
    .drop-zone:hover, .drop-zone.dragover {
      background: var(--blue-light);
      border-color: var(--blue);
    }
    .drop-zone.has-file {
      background: var(--green-bg);
      border-style: solid;
      border-color: var(--green);
    }
    .drop-title { font-size: 0.85rem; font-weight: 800; }
    .drop-hint { font-size: 0.72rem; color: #57606a; font-weight: 600; margin-top: 2px; }
    .file-meta {
      margin-top: 6px;
      font-size: 0.75rem;
      font-weight: 800;
      color: var(--green);
      font-family: 'Courier New', monospace;
      word-break: break-all;
    }
    .progress-wrap {
      display: none;
      margin-bottom: 14px;
    }
    .progress-header {
      display: flex;
      justify-content: space-between;
      font-size: 0.78rem;
      font-weight: 800;
      margin-bottom: 5px;
    }
    .progress-track {
      width: 100%;
      height: 18px;
      background: var(--bg);
      border: 2px solid var(--border);
      border-radius: 999px;
      overflow: hidden;
    }
    .progress-fill {
      height: 100%;
      width: 0%;
      background: var(--orange);
      transition: width 0.15s ease;
    }
    .status-box {
      display: none;
      border: 2px solid var(--border);
      border-radius: 8px;
      padding: 10px 12px;
      font-size: 0.8rem;
      font-weight: 800;
      margin-bottom: 14px;
      text-align: center;
    }
    .status-box.info { display: block; background: #fff3e0; color: var(--orange-dark); }
    .status-box.ok { display: block; background: var(--green-bg); color: var(--green); }
    .status-box.err { display: block; background: var(--red-bg); color: var(--red); }
    .btn {
      width: 100%;
      padding: 10px 14px;
      border: 2px solid var(--border);
      border-radius: 8px;
      font-size: 0.84rem;
      font-weight: 800;
      cursor: pointer;
      box-shadow: 2px 3px 0px var(--border);
      transition: all 0.06s;
      text-decoration: none;
      text-align: center;
      display: inline-flex;
      align-items: center;
      justify-content: center;
      gap: 6px;
    }
    .btn:active:not(:disabled) {
      transform: translate(2px, 2px);
      box-shadow: 0px 0px 0px var(--border);
    }
    .btn:disabled {
      opacity: 0.55;
      cursor: not-allowed;
      background: #e5e7eb;
      color: #6b7280;
    }
    .btn-primary { background: var(--green-bg); color: var(--green); }
    .btn-downgrade { background: var(--purple-bg); color: var(--purple); }
    .btn-back { background: var(--panel); color: var(--text); }
    .warn-note {
      margin-top: 12px;
      font-size: 0.71rem;
      color: #57606a;
      font-weight: 600;
      text-align: center;
      line-height: 1.4;
    }
  </style>
</head>
<body>
  <div class="card">
    <div class="header">
      <div>
        <h1>BRAILLIFY OTA</h1>
        <div class="sub">FIRMWARE VERSION MANAGER</div>
      </div>
      <div class="ver-badge" id="cur-ver-badge">v%VERSION%</div>
    </div>

    <!-- GitHub Releases Section -->
    <div class="section-box">
      <div class="section-title">
        <span>☁️ GitHub Releases (Upgrade / Downgrade)</span>
        <button type="button" class="refresh-link" onclick="loadGitHubReleases()">↻ Refresh</button>
      </div>

      <select id="gh-ver-select" class="ver-select" onchange="onReleaseSelected()" disabled>
        <option value="">Loading GitHub versions...</option>
      </select>

      <div class="rel-meta" id="rel-meta" style="display:none;">
        <span id="rel-date"></span>
        <span class="action-tag" id="rel-action-tag"></span>
      </div>

      <div class="rel-notes" id="rel-notes" style="display:none;"></div>

      <button type="button" class="btn btn-primary" id="gh-flash-btn" disabled onclick="flashGitHubVersion()">
        ⚡ Install Selected Version
      </button>
    </div>

    <!-- Local Binary Upload Section -->
    <div class="section-box">
      <div class="section-title">
        <span>📦 Offline Local Binary (.bin)</span>
      </div>

      <div class="drop-zone" id="drop-zone" onclick="document.getElementById('fw-file').click()">
        <div class="drop-title">Tap or Drop Firmware (.bin)</div>
        <div class="drop-hint">Flash directly without internet access</div>
        <div class="file-meta" id="file-meta"></div>
        <input type="file" id="fw-file" accept=".bin,application/octet-stream" style="display:none">
      </div>

      <button type="button" class="btn btn-primary" id="flash-btn" disabled onclick="startUpload()">
        📂 Upload &amp; Flash Local .bin
      </button>
    </div>

    <div class="progress-wrap" id="progress-wrap">
      <div class="progress-header">
        <span id="progress-label">Flashing firmware...</span>
        <span id="progress-pct">0%</span>
      </div>
      <div class="progress-track">
        <div class="progress-fill" id="progress-fill"></div>
      </div>
    </div>

    <div class="status-box" id="status-box"></div>

    <a href="/" class="btn btn-back" id="back-btn">← Back to Controller</a>

    <div class="warn-note">
      Keep Braillify powered on during flashing. The ESP32 will reboot automatically once verification completes.
    </div>
  </div>

  <script>
    const GITHUB_REPO = 'Cadlaxa/Braillify';
    const CURRENT_VER = '%VERSION%';

    const ghSelect = document.getElementById('gh-ver-select');
    const ghFlashBtn = document.getElementById('gh-flash-btn');
    const relMeta = document.getElementById('rel-meta');
    const relDate = document.getElementById('rel-date');
    const relActionTag = document.getElementById('rel-action-tag');
    const relNotes = document.getElementById('rel-notes');

    const fileInput = document.getElementById('fw-file');
    const dropZone = document.getElementById('drop-zone');
    const fileMeta = document.getElementById('file-meta');
    const flashBtn = document.getElementById('flash-btn');
    const backBtn = document.getElementById('back-btn');
    const progressWrap = document.getElementById('progress-wrap');
    const progressFill = document.getElementById('progress-fill');
    const progressPct = document.getElementById('progress-pct');
    const progressLabel = document.getElementById('progress-label');
    const statusBox = document.getElementById('status-box');

    let releasesData = [];
    let selectedFile = null;
    let isFlashing = false;

    function normalizeVer(v) {
      return String(v || '').trim().replace(/^v/i, '');
    }

    function compareVersions(a, b) {
      const pa = normalizeVer(a).split('.').map(n => parseInt(n, 10) || 0);
      const pb = normalizeVer(b).split('.').map(n => parseInt(n, 10) || 0);
      const len = Math.max(pa.length, pb.length);
      for (let i = 0; i < len; i++) {
        const na = pa[i] || 0;
        const nb = pb[i] || 0;
        if (na > nb) return 1;
        if (na < nb) return -1;
      }
      return 0;
    }

    function setStatus(type, msg) {
      statusBox.className = 'status-box ' + type;
      statusBox.innerText = msg;
    }

    function lockControls(locked) {
      isFlashing = locked;
      ghSelect.disabled = locked || releasesData.length === 0;
      ghFlashBtn.disabled = locked || !ghSelect.value;
      flashBtn.disabled = locked || !selectedFile;
      backBtn.style.pointerEvents = locked ? 'none' : '';
      backBtn.style.opacity = locked ? '0.5' : '';
    }

    async function loadGitHubReleases() {
      ghSelect.disabled = true;
      ghFlashBtn.disabled = true;
      ghSelect.innerHTML = '<option value="">Fetching releases from GitHub...</option>';
      relMeta.style.display = 'none';
      relNotes.style.display = 'none';

      try {
        const res = await fetch(`https://api.github.com/repos/${GITHUB_REPO}/releases`);
        if (!res.ok) throw new Error(`GitHub API HTTP ${res.status}`);
        const raw = await res.json();

        releasesData = raw
          .map(rel => {
            const binAsset = (rel.assets || []).find(a => a.name.toLowerCase().endsWith('.bin'));
            return binAsset ? {
              tag: rel.tag_name,
              name: rel.name || rel.tag_name,
              date: rel.published_at ? new Date(rel.published_at).toLocaleDateString() : '',
              notes: (rel.body || 'No release notes provided.').trim(),
              binUrl: binAsset.browser_download_url,
              binName: binAsset.name,
              binSize: binAsset.size
            } : null;
          })
          .filter(Boolean);

        if (releasesData.length === 0) {
          ghSelect.innerHTML = '<option value="">No .bin releases found on GitHub</option>';
          return;
        }

        ghSelect.innerHTML = '';
        releasesData.forEach((rel, idx) => {
          const cmp = compareVersions(rel.tag, CURRENT_VER);
          let label = `${rel.tag}`;
          if (idx === 0) label += ' (Latest)';
          if (cmp === 0) label += ' [Installed]';
          else if (cmp < 0) label += ' [Downgrade]';

          const opt = document.createElement('option');
          opt.value = String(idx);
          opt.innerText = label;
          ghSelect.appendChild(opt);
        });

        ghSelect.disabled = false;
        onReleaseSelected();
      } catch (err) {
        ghSelect.innerHTML = '<option value="">Offline / Unable to fetch GitHub releases</option>';
      }
    }

    function onReleaseSelected() {
      const idx = parseInt(ghSelect.value, 10);
      const rel = releasesData[idx];
      if (!rel) {
        ghFlashBtn.disabled = true;
        relMeta.style.display = 'none';
        relNotes.style.display = 'none';
        return;
      }

      const cmp = compareVersions(rel.tag, CURRENT_VER);
      relMeta.style.display = 'flex';
      relDate.innerText = `Released: ${rel.date} • ${rel.binName}`;
      relNotes.style.display = 'block';
      relNotes.innerText = rel.notes;

      ghFlashBtn.disabled = isFlashing;
      if (cmp > 0) {
        relActionTag.className = 'action-tag tag-upgrade';
        relActionTag.innerText = '⬆ UPGRADE';
        ghFlashBtn.className = 'btn btn-primary';
        ghFlashBtn.innerText = `⬆ Upgrade to ${rel.tag}`;
      } else if (cmp < 0) {
        relActionTag.className = 'action-tag tag-downgrade';
        relActionTag.innerText = '⬇ DOWNGRADE';
        ghFlashBtn.className = 'btn btn-downgrade';
        ghFlashBtn.innerText = `⬇ Downgrade to ${rel.tag}`;
      } else {
        relActionTag.className = 'action-tag tag-current';
        relActionTag.innerText = '✔ CURRENT';
        ghFlashBtn.className = 'btn btn-primary';
        ghFlashBtn.innerText = `↻ Reinstall ${rel.tag}`;
      }
    }

    function startCountdownRedirect(msgPrefix) {
      let sec = 7;
      setStatus('ok', `✅ ${msgPrefix} Redirecting in ${sec}s...`);
      const timer = setInterval(() => {
        sec--;
        if (sec <= 0) {
          clearInterval(timer);
          window.location.href = '/';
        } else {
          setStatus('ok', `✅ ${msgPrefix} Redirecting in ${sec}s...`);
        }
      }, 1000);
    }

    async function flashGitHubVersion() {
      const idx = parseInt(ghSelect.value, 10);
      const rel = releasesData[idx];
      if (!rel || isFlashing) return;

      const cmp = compareVersions(rel.tag, CURRENT_VER);
      if (cmp < 0 && !confirm(`Downgrade Braillify from v${CURRENT_VER} to ${rel.tag}?`)) {
        return;
      }

      lockControls(true);
      progressWrap.style.display = 'block';
      progressFill.style.background = 'var(--orange)';
      progressFill.style.width = '35%';
      progressPct.innerText = 'OTA';
      progressLabel.innerText = `Downloading ${rel.tag} on ESP32...`;
      setStatus('info', `⏳ ESP32 is downloading & flashing ${rel.tag} from GitHub. Do not power off...`);

      try {
        const body = new URLSearchParams();
        body.append('url', rel.binUrl);
        body.append('tag', rel.tag);

        const res = await fetch('/ota-url', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: body.toString()
        });
        const text = await res.text();

        if (res.ok && text.indexOf('Success') !== -1) {
          progressFill.style.width = '100%';
          progressFill.style.background = '#4caf50';
          progressPct.innerText = '100%';
          progressLabel.innerText = 'Flash complete!';
          startCountdownRedirect(`Installed ${rel.tag}! Rebooting...`);
        } else {
          lockControls(false);
          progressWrap.style.display = 'none';
          setStatus('err', '❌ ' + (text || 'GitHub OTA flash failed.'));
        }
      } catch (e) {
        // ESP32 often reboots immediately after sending response
        progressFill.style.width = '100%';
        progressFill.style.background = '#4caf50';
        progressPct.innerText = '100%';
        progressLabel.innerText = 'Rebooting device...';
        startCountdownRedirect(`Flashed ${rel.tag}!`);
      }
    }

    function formatBytes(bytes) {
      if (bytes < 1024) return bytes + ' B';
      if (bytes < 1048576) return (bytes / 1024).toFixed(1) + ' KB';
      return (bytes / 1048576).toFixed(2) + ' MB';
    }

    function handleChosenFile(file) {
      if (!file) return;
      if (!file.name.toLowerCase().endsWith('.bin')) {
        selectedFile = null;
        dropZone.classList.remove('has-file');
        fileMeta.innerText = '';
        flashBtn.disabled = true;
        setStatus('err', '❌ Invalid file type. Please choose a .bin firmware file.');
        return;
      }
      selectedFile = file;
      dropZone.classList.add('has-file');
      fileMeta.innerText = `✔ ${file.name} (${formatBytes(file.size)})`;
      flashBtn.disabled = isFlashing;
      statusBox.className = 'status-box';
    }

    fileInput.addEventListener('change', (e) => handleChosenFile(e.target.files[0]));
    dropZone.addEventListener('dragover', (e) => {
      e.preventDefault();
      if (!isFlashing) dropZone.classList.add('dragover');
    });
    dropZone.addEventListener('dragleave', () => dropZone.classList.remove('dragover'));
    dropZone.addEventListener('drop', (e) => {
      e.preventDefault();
      dropZone.classList.remove('dragover');
      if (!isFlashing && e.dataTransfer.files.length > 0) {
        handleChosenFile(e.dataTransfer.files[0]);
      }
    });

    function startUpload() {
      if (!selectedFile || isFlashing) return;
      lockControls(true);
      progressWrap.style.display = 'block';
      progressFill.style.background = 'var(--orange)';
      progressFill.style.width = '0%';
      progressPct.innerText = '0%';
      progressLabel.innerText = 'Uploading firmware...';
      setStatus('info', '⏳ Flashing in progress. Do not disconnect power...');

      const formData = new FormData();
      formData.append('update', selectedFile, selectedFile.name);

      const xhr = new XMLHttpRequest();
      xhr.open('POST', '/update', true);

      xhr.upload.onprogress = (e) => {
        if (e.lengthComputable) {
          const pct = Math.min(100, Math.round((e.loaded / e.total) * 100));
          progressFill.style.width = pct + '%';
          progressPct.innerText = pct + '%';
          if (pct === 100) progressLabel.innerText = 'Verifying & writing flash...';
        }
      };

      xhr.onload = () => {
        if (xhr.status === 200 && xhr.responseText.indexOf('Success') !== -1) {
          progressFill.style.background = '#4caf50';
          progressLabel.innerText = 'Flash complete!';
          startCountdownRedirect('Update Success! Rebooting...');
        } else {
          lockControls(false);
          setStatus('err', '❌ ' + (xhr.responseText || 'Firmware verification failed.'));
        }
      };

      xhr.onerror = () => {
        lockControls(false);
        setStatus('err', '❌ Connection lost during upload. Check Wi-Fi and retry.');
      };

      xhr.send(formData);
    }

    window.addEventListener('DOMContentLoaded', loadGitHubReleases);
  </script>
</body>
</html>)rawliteral";

#endif
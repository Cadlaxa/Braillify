const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>BRAILLIFY WEB CONTROLLER</title>
  <style>
    :root {
      --bg: #f0f4f8;
      --panel: #ffffff;
      --text: #111827;
      --border: #111827;
      --orange: #ff9800;
      --orange-dark: #e65100;
      --blue-light: #e3f2fd;
      --blue: #0288d1;
      --green: #4caf50;
      --red: #f44336;
      --lcd-bg: #e8f5e9;
      --lcd-text: #1b4332;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: system-ui, -apple-system, sans-serif; }
    html, body { touch-action: manipulation; -webkit-text-size-adjust: 100%; }
    body { background: var(--bg); color: var(--text); display: flex; justify-content: center; align-items: center; min-height: 100vh; padding: 15px; }
    
    body.modal-open { overflow: hidden !important; height: 100vh; touch-action: none; }

    .container { width: 100%; max-width: 480px; background: var(--panel); border: 3px solid var(--border); box-shadow: 6px 6px 0px var(--border); border-radius: 12px; padding: 20px; position: relative; }
    
    .header { display: flex; justify-content: space-between; align-items: center; border-bottom: 2px dashed var(--border); padding-bottom: 12px; margin-bottom: 16px; }
    .header-title h1 { font-size: 1.5rem; letter-spacing: 1px; color: var(--text); font-weight: 900; font-family: 'Courier New', monospace; }
    .header-title .sub { font-size: 0.7rem; color: var(--orange-dark); font-weight: bold; }
    .header-btns { display: flex; gap: 6px; align-items: center; }

    .modal-trigger-btn { background: #e8f5e9; border: 2px solid var(--border); color: #2e7d32; font-size: 0.75rem; font-weight: bold; padding: 6px 10px; border-radius: 8px; cursor: pointer; display: flex; align-items: center; gap: 4px; box-shadow: 2px 2px 0px var(--border); transition: all .05s; }
    .modal-trigger-btn:active { transform: translate(1px, 1px); box-shadow: 0px 0px 0px var(--border); }
    .wifi-btn { background: var(--blue-light); color: var(--blue); }

    /* Top Bar Signal Badge */
    .wifi-signal-badge { display: inline-flex; align-items: center; gap: 4px; background: #ffffff; border: 1.5px solid var(--border); padding: 2px 6px; border-radius: 6px; font-size: 0.75rem; font-weight: 800; color: var(--blue); }
    .signal-icon-symbol { font-size: 0.95rem; line-height: 1; vertical-align: middle; }

    /* Modals */
    .modal-overlay { position: fixed; top: 0; left: 0; width: 100vw; height: 100vh; background: rgba(17,24,39,0.5); backdrop-filter: blur(3px); display: none; justify-content: center; align-items: center; z-index: 999; padding: 15px; overflow-y: auto; -webkit-overflow-scrolling: touch}
    .modal-overlay.active { display: flex; }
    .modal-content { width: 100%; max-width: 420px; background: #ffffff; border: 3px solid var(--border); border-radius: 12px; box-shadow: 8px 8px 0px var(--border); padding: 20px; position: relative; animation: popIn 0.2s cubic-bezier(0.175, 0.885, 0.32, 1.275); max-height: 80vh; overflow-y: auto;}
    @keyframes popIn { from { transform: scale(0.8); opacity: 0; } to { transform: scale(1); opacity: 1; } }

    .modal-header { display: flex; justify-content: space-between; align-items: center; border-bottom: 2px solid var(--border); padding-bottom: 10px; margin-bottom: 14px; }
    .modal-title { font-size: 1.1rem; font-weight: 900; color: var(--blue); display: flex; align-items: center; gap: 6px; }
    .modal-close-btn { background: none; border: none; font-size: 1.5rem; font-weight: 900; color: var(--text); cursor: pointer; line-height: 1; padding: 0 4px; }

    /* Setup Wizard Steps */
    .wizard-progress { display: flex; justify-content: center; gap: 6px; margin-bottom: 16px; }
    .wizard-dot { width: 12px; height: 12px; border-radius: 50%; border: 2px solid var(--border); background: #d0d7de; transition: all 0.2s; }
    .wizard-dot.active { background: var(--orange); transform: scale(1.2); }
    .wizard-dot.completed { background: var(--green); }
    
    .wizard-step { display: none; }
    .wizard-step.active { display: block; }

    .wizard-hero { text-align: center; margin-bottom: 16px; }
    .wizard-hero-icon { font-size: 3rem; margin-bottom: 8px; }
    .wizard-hero h2 { font-size: 1.3rem; font-weight: 900; color: var(--orange-dark); margin-bottom: 6px; }
    .wizard-hero p { font-size: 0.85rem; color: #57606a; line-height: 1.4; font-weight: 600; }

    .wizard-nav { display: flex; justify-content: space-between; gap: 10px; margin-top: 20px; border-top: 2px dashed var(--border); padding-top: 14px; }
    .wizard-btn { flex: 1; padding: 10px; border: 2px solid var(--border); border-radius: 8px; font-weight: 800; font-size: 0.85rem; cursor: pointer; box-shadow: 2px 2px 0px var(--border); transition: all 0.05s; text-align: center; }
    .wizard-btn:active { transform: translate(1px, 1px); box-shadow: 0px 0px 0px var(--border); }
    .wizard-btn.primary { background: #e8f5e9; color: #2e7d32; }
    .wizard-btn.secondary { background: var(--bg); color: var(--text); }

    .key-guide-grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 8px; margin-top: 10px; }
    .key-guide-item { background: var(--bg); border: 2px solid var(--border); border-radius: 8px; padding: 8px; display: flex; align-items: center; gap: 8px; font-size: 0.78rem; font-weight: 700; }
    .key-guide-badge { background: var(--orange); color: white; padding: 4px 8px; border-radius: 6px; font-weight: 900; font-family: 'Courier New', monospace; }

    .fc-scores { display: flex; gap: 10px; font-size: 0.8rem; font-weight: 800; font-family: 'Courier New', monospace; margin-bottom: 14px; justify-content: center; }
    .fc-score-badge { background: var(--blue-light); border: 2px solid var(--border); padding: 4px 10px; border-radius: 6px; }
    .fc-card-display { display: flex; flex-direction: column; align-items: center; background: var(--bg); border: 2px solid var(--border); border-radius: 10px; padding: 16px; margin-bottom: 14px; text-align: center; }
    .fc-target-label { font-size: 0.75rem; font-weight: 800; color: #57606a; margin-bottom: 4px; }
    .fc-target-char { font-size: 3.5rem; font-weight: 900; color: var(--orange-dark); font-family: 'Courier New', monospace; line-height: 1; }
    .fc-trigger-info { font-size: 0.78rem; font-weight: 800; color: var(--blue); margin-top: 6px; background: #ffffff; padding: 4px 8px; border: 1.5px solid var(--border); border-radius: 6px; display: inline-block; }
    .fc-hint-box { margin-top: 8px; font-size: 0.9rem; font-weight: bold; color: var(--text); display: none; }
    .fc-hint-braille { font-size: 2rem; line-height: 1; color: var(--blue); vertical-align: middle; }
    .fc-status-msg { font-size: 0.95rem; font-weight: 900; text-align: center; min-height: 1.4em; margin-bottom: 14px; }
    .fc-status-msg.correct { color: var(--green); }
    .fc-status-msg.wrong { color: var(--red); }
    .fc-actions { display: flex; gap: 8px; }
    .fc-btn { flex: 1; background: #fff3e0; border: 2px solid var(--border); color: var(--orange-dark); font-size: 0.8rem; font-weight: 800; padding: 8px; border-radius: 6px; cursor: pointer; box-shadow: 2px 2px 0px var(--border); text-align: center; transition: all .05s; }
    .fc-btn:active { transform: translate(1px, 1px); box-shadow: 0px 0px 0px var(--border); }

    .wifi-list { max-height: 160px; overflow-y: auto; border: 2px solid var(--border); border-radius: 8px; margin-bottom: 12px; background: var(--bg); }
    .wifi-item { padding: 8px 12px; border-bottom: 1px solid #d0d7de; display: flex; justify-content: space-between; align-items: center; cursor: pointer; font-weight: 700; font-size: 0.85rem; }
    .wifi-item:last-child { border-bottom: none; }
    .wifi-item:hover { background: var(--blue-light); }
    .wifi-rssi { font-size: 0.75rem; color: #57606a; font-family: 'Courier New', monospace; }
    .wifi-input-group { display: flex; flex-direction: column; gap: 8px; margin-bottom: 14px; }
    .wifi-input { width: 100%; padding: 8px 10px; border: 2px solid var(--border); border-radius: 6px; font-size: 0.85rem; font-weight: 600; }
    .wifi-status-info { font-size: 0.75rem; font-weight: bold; color: var(--orange-dark); margin-bottom: 10px; text-align: center; }

    .setting-group { margin-bottom: 16px; }
    .setting-header { display: flex; justify-content: space-between; font-size: 0.85rem; font-weight: 800; margin-bottom: 6px; color: var(--text); }
    .setting-slider { width: 100%; accent-color: var(--orange-dark); cursor: pointer; height: 8px; }
    .setting-select { width: 100%; padding: 8px 10px; border: 2px solid var(--border); border-radius: 6px; font-size: 0.85rem; font-weight: bold; background: #fff; }

    .camera-box { width: 100%; height: 210px; background: #000; border: 2px solid var(--border); border-radius: 8px; margin-bottom: 12px; overflow: hidden; position: relative; display: flex; justify-content: center; align-items: center; }
    .camera-video { width: 100%; height: 100%; object-fit: cover; }
    .ocr-status { font-size: 0.8rem; font-weight: bold; color: var(--blue); text-align: center; margin-bottom: 10px; min-height: 1.2em; }

    .full-text-section { margin-bottom: 16px; }
    .section-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 6px; }
    .section-title { font-size: 0.8rem; color: var(--orange-dark); font-weight: bold; text-transform: uppercase; }
    .action-btn-group { display: flex; gap: 6px; }
    .action-btn { background: #fff3e0; border: 2px solid var(--border); color: var(--orange-dark); font-size: 0.75rem; font-weight: bold; padding: 4px 10px; border-radius: 6px; cursor: pointer; display: flex; align-items: center; gap: 4px; box-shadow: 1px 2px 0px var(--border); transition: all .05s; }
    .action-btn:active { transform: translate(1px, 1px); box-shadow: 0px 0px 0px var(--border); }
    .action-btn svg { width: 14px; height: 14px; fill: currentColor; }

    .full-text-box { background: #ffffff; border: 2px solid var(--border); border-radius: 8px; padding: 10px; min-height: 64px; max-height: 120px; overflow-y: auto; overflow-x: hidden; box-shadow: inset 0 0 4px rgba(0,0,0,0.06); display: flex; flex-wrap: wrap; align-content: flex-start; gap: 2px; user-select: none; -webkit-user-select: none; touch-action: none; }
    .char-pair { display: inline-flex; flex-direction: column; align-items: center; justify-content: center; cursor: pointer; padding: 2px 4px; border-radius: 4px; user-select: none; min-width: 18px; transition: background 0.1s; position: relative; }
    .char-pair:hover { background: var(--blue-light); }
    .char-pair.selected { background: #bbdefb !important; color: #0d47a1 !important; border: 1px solid var(--blue); border-radius: 3px; }
    .char-pair.cursor-here { background: var(--orange); color: #ffffff; font-weight: bold; animation: blink 1s infinite; }
    @keyframes blink { 0%, 100% { opacity: 1; } 50% { opacity: 0.4; } }
    .char-pair .top-char { font-size: 1.1rem; font-weight: 900; font-family: 'Courier New', monospace; line-height: 1; pointer-events: none; }
    .char-pair .sub-braille { font-size: 0.85rem; line-height: 1; margin-top: 2px; opacity: 0.85; font-family: sans-serif; pointer-events: none; }

    .selection-toolbar { display: none; margin-top: 8px; background: #e3f2fd; border: 2px solid var(--border); padding: 6px 10px; border-radius: 8px; justify-content: space-between; align-items: center; font-size: 0.75rem; font-weight: bold; }
    .selection-toolbar.active { display: flex; }

    .lcd-frame { background: var(--lcd-bg); border: 3px solid var(--border); border-radius: 8px; padding: 12px 16px; margin-bottom: 16px; box-shadow: inset 0 0 5px rgba(0,0,0,0.08); position: relative; }
    .lcd-line { font-size: 1.2rem; color: var(--lcd-text); letter-spacing: 2px; white-space: pre; font-weight: bold; font-family: 'Courier New', monospace; min-height: 1.4em; }

    .status-card { display: flex; align-items: center; justify-content: space-between; background: var(--blue-light); border: 2px solid var(--border); padding: 14px; border-radius: 8px; margin-bottom: 16px; }
    .cell-preview-box { display: flex; align-items: center; gap: 16px; }
    .braille-pattern-cell { display: grid; grid-template-columns: repeat(2, 16px); grid-gap: 8px; background: #ffffff; border: 2px solid var(--border); padding: 8px; border-radius: 6px; }
    .v-dot { width: 16px; height: 16px; border-radius: 50%; background: #d0d7de; border: 1.5px solid var(--border); transition: all .15s; }
    .v-dot.active { background: var(--green); box-shadow: 0 0 6px rgba(76,175,80,0.6); }

    .char-output-box { display: flex; flex-direction: column; align-items: flex-start; }
    .char-label { font-size: 0.7rem; font-weight: bold; color: var(--text); text-transform: uppercase; letter-spacing: 1px; }
    .char-val { font-size: 1.8rem; font-weight: 900; color: var(--orange-dark); font-family: 'Courier New', monospace; }
    .bit-badge { display: flex; flex-direction: column; align-items: flex-end; font-size: 0.75rem; font-weight: bold; font-family: 'Courier New', monospace; }
    .bit-badge span { color: var(--blue); }

    .keypad-grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 10px; }
    .key-btn { background: #ffffff; border: 2px solid var(--border); color: var(--text); height: 58px; display: flex; flex-direction: column; align-items: center; justify-content: center; border-radius: 8px; cursor: pointer; box-shadow: 2px 3px 0px var(--border); transition: all .05s; user-select: none; -webkit-touch-callout: none; }
    .key-btn:active { transform: translate(2px, 2px); box-shadow: 0px 0px 0px var(--border); background: var(--blue-light); }
    .key-btn.cmd { background: #fff3e0; border-color: var(--border); color: var(--orange-dark); }
    .key-btn.cmd:active { background: var(--orange); color: #ffffff; }
    .key-btn svg { width: 22px; height: 22px; fill: currentColor; }
    .key-label { font-size: 0.65rem; font-weight: 800; margin-top: 2px; font-family: 'Courier New', monospace; }
    .dot-key-num { font-size: 1.1rem; font-weight: 900; font-family: 'Courier New', monospace; }
    .dot-key-sub { font-size: 0.6rem; color: #57606a; font-weight: 700; }
    .footer { text-align: center; font-size: .7rem; font-weight: bold; color: #57606a; margin-top: 14px; border-top: 2px solid var(--border); padding-top: 8px; }

    .ota-section { margin-top: 12px; text-align: center; }
    .btn-update { width: 100%; background: #e8f5e9; border: 2px solid var(--border); color: #2e7d32; font-size: 0.85rem; font-weight: 800; padding: 10px; border-radius: 8px; cursor: pointer; box-shadow: 2px 2px 0px var(--border); transition: all .05s; display: flex; align-items: center; justify-content: center; gap: 6px; }
    .btn-update:active { transform: translate(1px, 1px); box-shadow: 0px 0px 0px var(--border); }
    .btn-update:disabled { opacity: 0.6; cursor: not-allowed; background: #d0d7de; color: #57606a; }
    #ota-status { font-size: 0.75rem; font-weight: bold; color: var(--orange-dark); margin-top: 6px; min-height: 1.2em; }
  </style>
</head>
<body>

<div class="container">
  <div class="header">
    <div class="header-title">
      <h1>BRAILLIFY</h1>
      <div class="sub">HARDWARE TRANSLATOR</div>
    </div>
    <div class="header-btns">
      <div class="wifi-signal-badge" id="wifi-signal-badge" title="Wi-Fi Signal Strength">
        <span class="signal-icon-symbol" id="sig-symbol">📡</span>
        <span id="sig-dbm-label">--</span>
      </div>
      <button class="modal-trigger-btn wifi-btn" onclick="openWifiModal()">📶 Wi-Fi</button>
      <button class="modal-trigger-btn" onclick="openFlashcardModal()">🎓 Practice</button>
    </div>
  </div>

  <div class="full-text-section">
    <div class="section-header">
      <div class="section-title">> Transformed Full Text</div>
      <div class="action-btn-group">
        <button class="action-btn" id="tts-btn" onclick="speakFullTextOrSelection()" title="Read Text Aloud">
          <svg viewBox="0 0 24 24"><path d="M3 9v6h4l5 5V4L7 9H3zm13.5 3c0-1.77-1.02-3.29-2.5-4.03v8.05c1.48-.73 2.5-2.25 2.5-4.02zM14 3.23v2.06c2.89 0.86 5 3.54 5 6.71s-2.11 5.85-5 6.71v2.06c4.01-0.91 7-4.49 7-8.77s-2.99-7.86-7-8.77z"/></svg>
          <span id="tts-btn-text">Speak</span>
        </button>
        <button class="action-btn" id="copy-btn" onclick="copyFullText()" title="Copy to Clipboard">
          <svg viewBox="0 0 24 24"><path d="M16 1H4c-1.1 0-2 0.9-2 2v14h2V3h12V1zm3 4H8c-1.1 0-2 0.9-2 2v14c0 1.1 0.9 2 2 2h11c1.1 0 2-0.9 2-2V7c0-1.1-0.9-2-2-2zm0 16H8V7h11v14z"/></svg>
          <span id="copy-btn-text">Copy Text</span>
        </button>
      </div>
    </div>
    
    <div class="full-text-box" id="full-text-box"></div>

    <div class="selection-toolbar" id="selection-toolbar">
      <span><span id="sel-count-label">0</span> chars selected</span>
      <div style="display:flex; gap:6px;">
        <button class="action-btn" onclick="speakSelection()">🗣️ Speak</button>
        <button class="action-btn" id="sel-copy-btn" onclick="copySelection()">📋 Copy</button>
        <button class="action-btn" style="background:#ffebee; color:#c62828;" onclick="deleteSelectionWeb()">🗑️ Del</button>
      </div>
    </div>
  </div>

  <div class="lcd-frame">
    <div class="lcd-line" id="lcd-line0">Mode: AUTO     </div>
    <div class="lcd-line" id="lcd-line1">_               </div>
  </div>

  <div class="status-card">
    <div class="cell-preview-box">
      <div class="braille-pattern-cell">
        <div class="v-dot" id="vdot1"></div>
        <div class="v-dot" id="vdot4"></div>
        <div class="v-dot" id="vdot2"></div>
        <div class="v-dot" id="vdot5"></div>
        <div class="v-dot" id="vdot3"></div>
        <div class="v-dot" id="vdot6"></div>
      </div>
      <div class="char-output-box">
        <div class="char-label">Cell Preview</div>
        <div class="char-val" id="char-preview">~</div>
      </div>
    </div>
    <div class="bit-badge">
      <div>BITS: <span id="bit-val">0b000000</span></div>
      <div>DEC: <span id="dec-val">0</span></div>
    </div>
  </div>

  <div class="keypad-grid">
    <button class="key-btn cmd" onclick="openCameraModal()" title="Camera OCR / Scan">
      <svg viewBox="0 0 24 24"><path d="M12 15a3 3 0 1 0 0-6 3 3 0 0 0 0 6zm9-9h-3.17L16 4H8L6.17 6H3a2 2 0 0 0-2 2v12a2 2 0 0 0 2 2h18a2 2 0 0 0 2-2V8a2 2 0 0 0-2-2zm-9 12a5 5 0 1 1 0-10 5 5 0 0 1 0 10z"/></svg>
      <span class="key-label">SCAN</span>
    </button>
    <button class="key-btn" data-key="2"><span class="dot-key-num">2</span><span class="dot-key-sub">• Dot 1</span></button>
    <button class="key-btn" data-key="3"><span class="dot-key-num">3</span><span class="dot-key-sub">• Dot 4</span></button>
    <button class="key-btn cmd" data-key="A" title="Save Memory">
      <svg viewBox="0 0 24 24"><path d="M17 3H5c-1.11 0-2 0.9-2 2v14c0 1.1 0.89 2 2 2h14c1.1 0 2-0.9 2-2V7l-4-4zm-5 16c-1.66 0-3-1.34-3-3s1.34-3 3-3 3 1.34 3 3-1.34 3-3 3zm3-10H5V5h10v4z"/></svg>
      <span class="key-label">SAVE</span>
    </button>

    <button class="key-btn cmd" data-key="4" title="Mute Toggle">
      <svg viewBox="0 0 24 24"><path d="M3 9v6h4l5 5V4L7 9H3zm13.5 3c0-1.77-1.02-3.29-2.5-4.03v8.05c1.48-.73 2.5-2.25 2.5-4.02zM14 3.23v2.06c2.89 0.86 5 3.54 5 6.71s-2.11 5.85-5 6.71v2.06c4.01-0.91 7-4.49 7-8.77s-2.99-7.86-7-8.77z"/></svg>
      <span class="key-label">MUTE</span>
    </button>
    <button class="key-btn" data-key="5"><span class="dot-key-num">5</span><span class="dot-key-sub">• Dot 2</span></button>
    <button class="key-btn" data-key="6"><span class="dot-key-num">6</span><span class="dot-key-sub">• Dot 5</span></button>
    <button class="key-btn cmd" data-key="B" title="Load Memory">
      <svg viewBox="0 0 24 24"><path d="M20 6h-8l-2-2H4c-1.1 0-1.99 0.9-1.99 2L2 18c0 1.1 0.9 2 2 2h16c1.1 0 2-0.9 2-2V8c0-1.1-0.9-2-2-2zm0 12H4V8h16v10z"/></svg>
      <span class="key-label">LOAD</span>
    </button>

    <button class="key-btn cmd" data-key="7" title="Cursor Left">
      <svg viewBox="0 0 24 24"><path d="M20 11H7.83l5.59-5.59L12 4l-8 8 8 8 1.41-1.41L7.83 13H20v-2z"/></svg>
      <span class="key-label">LEFT</span>
    </button>
    <button class="key-btn" data-key="8"><span class="dot-key-num">8</span><span class="dot-key-sub">• Dot 3</span></button>
    <button class="key-btn" data-key="9"><span class="dot-key-num">9</span><span class="dot-key-sub">• Dot 6</span></button>
    <button class="key-btn cmd" data-key="C" title="Cursor Right">
      <svg viewBox="0 0 24 24"><path d="M12 4l-1.41 1.41L16.17 11H4v2h12.17l-5.58 5.59L12 20l8-8z"/></svg>
      <span class="key-label">RIGHT</span>
    </button>

    <button class="key-btn cmd" data-key="*" title="Cycle Mode">
      <svg viewBox="0 0 24 24"><path d="M12 4V1L8 5l4 4V6c3.31 0 6 2.69 6 6 0 1.01-0.25 1.97-0.7 2.8l1.46 1.46C19.54 15.03 20 13.57 20 12c0-4.42-3.58-8-8-8zm0 14c-3.31 0-6-2.69-6-6 0-1.01 0.25-1.97 0.7-2.8L5.24 7.74C4.46 8.97 4 10.43 4 12c0 4.42 3.58 8 8 8v3l4-4-4-4v3z"/></svg>
      <span class="key-label">MODE</span>
    </button>
    <button class="key-btn" data-key="0" title="Space">
      <svg viewBox="0 0 24 24"><path d="M18 9v4H6V9H4v6h16V9h-2z"/></svg>
      <span class="key-label">SPACE</span>
    </button>
    <button class="key-btn cmd" data-key="#" title="Enter / Confirm Cell">
      <svg viewBox="0 0 24 24"><path d="M19 7v4H5.83l3.58-3.59L8 6l-6 6 6 6 1.41-1.41L5.83 13H21V7h-2z"/></svg>
      <span class="key-label">ENTER</span>
    </button>
    <button class="key-btn cmd" data-key="D" title="Backspace / Delete">
      <svg viewBox="0 0 24 24"><path d="M22 3H7c-0.69 0-1.23 0.35-1.59 0.88L0 12l5.41 8.11c0.36 0.53 0.9 0.89 1.59 0.89h15c1.1 0 2-0.9 2-2V5c0-1.1-0.9-2-2-2zm0 16H7.07L2.4 12l4.66-7H22v14zm-11.59-2L14 13.41 17.59 17 19 15.59 15.41 12 19 8.41 17.59 7 14 10.59 10.41 7 9 8.41 12.59 12 9 15.59z"/></svg>
      <span class="key-label">DEL</span>
    </button>
  </div>

  <div class="footer">ESP32 BRAILLIFY // AP: 192.168.4.1 // Local: http://braillify.local</div>
  
  <div class="ota-section">
    <button onclick="openSettingsModal()" class="btn-update" style="background: var(--blue-light); color: var(--blue);">
      ⚙️ Device Settings
    </button>
  </div>

</div>

<!-- FIRST TIME SETUP WIZARD MODAL -->
<div class="modal-overlay" id="wizard-modal">
  <div class="modal-content" style="max-width: 440px;">
    <div class="wizard-progress">
      <div class="wizard-dot active" id="wdot-1"></div>
      <div class="wizard-dot" id="wdot-2"></div>
      <div class="wizard-dot" id="wdot-3"></div>
      <div class="wizard-dot" id="wdot-4"></div>
      <div class="wizard-dot" id="wdot-5"></div>
      <div class="wizard-dot" id="wdot-6"></div>
    </div>

    <!-- Step 1: Welcome -->
    <div class="wizard-step active" id="wstep-1">
      <div class="wizard-hero">
        <div class="wizard-hero-icon">⠃⠗⠁⠊⠇⠇⠊⠋⠽</div>
        <h2>Welcome to Braillify!</h2>
        <p>Your hardware tactile Braille translator is ready. Let's customize your audio, screen brightness, security PIN, and Wi-Fi preferences in a few quick steps.</p>
      </div>
      <div class="wizard-nav">
        <div></div>
        <button class="wizard-btn primary" onclick="nextWizardStep(2)">Get Started →</button>
      </div>
    </div>

    <!-- Step 2: Audio & Accessibility -->
    <div class="wizard-step" id="wstep-2">
      <div class="modal-header">
        <div class="modal-title">🔊 Audio & Accessibility</div>
      </div>
      <div class="setting-group">
        <div class="setting-header"><span>🗣️ TTS Speech Mode</span></div>
        <select id="w-tts-mode-select" class="setting-select" onchange="setTtsMode(this.value)">
          <option value="1">Mode 1: Speak Char + Full Word on Space</option>
          <option value="2">Mode 2: Speak Full Word Only (after space)</option>
          <option value="3">Mode 3: Speak Full Buffer Line</option>
          <option value="0">Mode 0: Web Speech TTS Disabled</option>
        </select>
      </div>
      <div class="setting-group">
        <div class="setting-header"><span>🔊 Buzzer Volume</span><span id="w-vol-label">100%</span></div>
        <input type="range" id="w-vol-slider" min="0" max="100" value="100" class="setting-slider" onchange="setVolume(this.value)" oninput="document.getElementById('w-vol-label').innerText = (this.value > 0 ? this.value + '%' : 'MUTED')">
      </div>
      <div class="wizard-nav">
        <button class="wizard-btn secondary" onclick="prevWizardStep(1)">← Back</button>
        <button class="wizard-btn primary" onclick="nextWizardStep(3)">Next: Display →</button>
      </div>
    </div>

    <!-- Step 3: Screen & Brightness -->
    <div class="wizard-step" id="wstep-3">
      <div class="modal-header">
        <div class="modal-title">☀️ Screen & Display</div>
      </div>
      <div class="setting-group" style="margin-bottom: 24px;">
        <div class="setting-header"><span>LCD Backlight Brightness</span><span id="w-bright-label">100%</span></div>
        <input type="range" id="w-bright-slider" min="0" max="100" value="100" class="setting-slider" onchange="setBrightness(this.value)" oninput="document.getElementById('w-bright-label').innerText=this.value+'%'">
      </div>
      <div class="wizard-nav">
        <button class="wizard-btn secondary" onclick="prevWizardStep(2)">← Back</button>
        <button class="wizard-btn primary" onclick="nextWizardStep(4)">Next: Security PIN →</button>
      </div>
    </div>

    <!-- Step 4: Security PIN & Question Setup -->
    <div class="wizard-step" id="wstep-4">
      <div class="modal-header">
        <div class="modal-title">🔐 Device Security PIN</div>
      </div>
      <p style="font-size:0.8rem; color:#57606a; margin-bottom:12px; font-weight:600;">Set a 4-digit PIN and recovery question to secure web interface access.</p>

      <div class="wifi-input-group">
        <input type="password" maxlength="4" id="w-pin-input" class="wifi-input" inputmode="numeric" pattern="[0-9]*" placeholder="Set 4-Digit PIN (e.g. 1234)">
        <input type="text" id="w-sec-q-input" class="wifi-input" placeholder="Security Question (e.g. Pet Name?)">
        <input type="text" id="w-sec-a-input" class="wifi-input" placeholder="Security Answer">
      </div>

      <div class="wizard-nav">
        <button class="wizard-btn secondary" onclick="prevWizardStep(3)">← Back</button>
        <button class="wizard-btn primary" onclick="saveWizardSecurity()">Next: Wi-Fi →</button>
      </div>
    </div>

    <!-- Step 5: Wi-Fi Setup -->
    <div class="wizard-step" id="wstep-5">
      <div class="modal-header">
        <div class="modal-title">📶 Connect Wi-Fi</div>
      </div>
      <p style="font-size:0.8rem; color:#57606a; margin-bottom:10px; font-weight:600;">Connecting Braillify to your network enables Cloud OCR scanning and GitHub wireless firmware updates.</p>
      <div class="fc-actions" style="margin-bottom:10px;">
        <button class="fc-btn" onclick="scanWifiNetworks()">🔍 Scan Wi-Fi Networks</button>
      </div>

      <div class="wifi-list" id="w-wifi-list" style="max-height: 120px; margin-bottom: 12px;">
        <div class="wifi-item" style="justify-content:center;color:#57606a;">Click Scan to search networks</div>
      </div>

      <div class="wifi-input-group">
        <input type="text" id="w-wifi-ssid" class="wifi-input" placeholder="Wi-Fi Network SSID">
        <input type="password" id="w-wifi-pass" class="wifi-input" placeholder="Wi-Fi Password">
      </div>
      <div class="fc-actions">
        <button class="fc-btn" style="background:#e8f5e9;color:#2e7d32;" onclick="connectWifiFromWizard()">Connect Network</button>
      </div>
      <div class="wizard-nav">
        <button class="wizard-btn secondary" onclick="prevWizardStep(4)">← Back</button>
        <button class="wizard-btn primary" onclick="nextWizardStep(6)">Next: Overview →</button>
      </div>
    </div>

    <!-- Step 6: Quickstart & Finish -->
    <div class="wizard-step" id="wstep-6">
      <div class="modal-header">
        <div class="modal-title">✨ Keypad Quick Guide</div>
      </div>
      <div class="key-guide-grid">
        <div class="key-guide-item"><span class="key-guide-badge">#</span> Confirm Cell</div>
        <div class="key-guide-item"><span class="key-guide-badge">0</span> Space Character</div>
        <div class="key-guide-item"><span class="key-guide-badge">*</span> Switch Mode</div>
        <div class="key-guide-item"><span class="key-guide-badge">1</span> Camera OCR</div>
        <div class="key-guide-item"><span class="key-guide-badge">A</span> Save to Memory</div>
        <div class="key-guide-item"><span class="key-guide-badge">B</span> Load Memory</div>
      </div>
      <div class="wizard-nav">
        <button class="wizard-btn secondary" onclick="prevWizardStep(5)">← Back</button>
        <button class="wizard-btn primary" onclick="finishWizardSetup()">🚀 Complete Setup</button>
      </div>
    </div>

  </div>
</div>

<!-- PIN ACCESS LOCK MODAL -->
<div class="modal-overlay active" id="pin-lock-modal">
  <div class="modal-content" style="max-width: 360px; text-align: center;">
    
    <div id="pin-entry-view">
      <div class="wizard-hero-icon" style="font-size: 2.5rem; margin-bottom: 6px;">🔒</div>
      <h2 style="font-size: 1.3rem; font-weight: 900; color: var(--text); margin-bottom: 4px;">Braillify Security</h2>
      <p style="font-size: 0.8rem; color: #57606a; margin-bottom: 14px; font-weight: 600;">Enter 4-digit PIN to connect</p>

      <div style="display: flex; justify-content: center; gap: 8px; margin-bottom: 16px;">
        <input type="password" maxlength="1" id="pin-d1" class="wifi-input" inputmode="numeric" pattern="[0-9]*" style="width: 45px; height: 50px; text-align: center; font-size: 1.4rem; font-weight: 900;" oninput="focusNextPin(1)">
        <input type="password" maxlength="1" id="pin-d2" class="wifi-input" inputmode="numeric" pattern="[0-9]*" style="width: 45px; height: 50px; text-align: center; font-size: 1.4rem; font-weight: 900;" oninput="focusNextPin(2)">
        <input type="password" maxlength="1" id="pin-d3" class="wifi-input" inputmode="numeric" pattern="[0-9]*" style="width: 45px; height: 50px; text-align: center; font-size: 1.4rem; font-weight: 900;" oninput="focusNextPin(3)">
        <input type="password" maxlength="1" id="pin-d4" class="wifi-input" inputmode="numeric" pattern="[0-9]*" style="width: 45px; height: 50px; text-align: center; font-size: 1.4rem; font-weight: 900;" oninput="focusNextPin(4)">
      </div>

      <div class="fc-actions" style="margin-bottom: 12px;">
        <button class="fc-btn" style="background:#e8f5e9; color:#2e7d32; font-size:0.9rem; padding:10px;" onclick="submitPinAuth()">Connect to Braillify</button>
      </div>

      <div id="pin-error-msg" style="font-size: 0.78rem; font-weight: 800; color: var(--red); min-height: 1.2em; margin-bottom: 8px;"></div>

      <button onclick="showForgotPinView()" style="background: none; border: none; color: var(--blue); font-size: 0.78rem; font-weight: 800; cursor: pointer; text-decoration: underline;">
        Forgot PIN?
      </button>
    </div>

    <div id="pin-forgot-view" style="display: none;">
      <div class="modal-header" style="justify-content: center; margin-bottom: 10px;">
        <div class="modal-title" style="font-size: 1rem;">🔑 Reset PIN</div>
      </div>

      <div style="font-size: 0.8rem; font-weight: 800; color: var(--text); margin-bottom: 6px; text-align: left;">Security Question:</div>
      <div id="forgot-sec-question" style="font-size: 0.85rem; font-weight: 700; color: var(--orange-dark); background: var(--bg); padding: 8px; border-radius: 6px; border: 1.5px solid var(--border); margin-bottom: 12px; text-align: left;">
        Loading question...
      </div>

      <div class="wifi-input-group" style="margin-bottom: 10px;">
        <input type="text" id="forgot-answer-input" class="wifi-input" placeholder="Your Security Answer">
        <input type="password" maxlength="4" id="forgot-new-pin" class="wifi-input" inputmode="numeric" pattern="[0-9]*" placeholder="New 4-Digit PIN">
      </div>

      <div class="fc-actions" style="margin-bottom: 8px;">
        <button class="fc-btn" style="background:#e8f5e9; color:#2e7d32;" onclick="submitForgotPinReset()">Reset PIN & Connect</button>
      </div>
      <button onclick="hideForgotPinView()" style="background: none; border: none; color: #57606a; font-size: 0.75rem; font-weight: bold; cursor: pointer;">
        ← Back to PIN Entry
      </button>
    </div>

  </div>
</div>

<!-- Device Settings Modal -->
<div class="modal-overlay" id="settings-modal" onclick="handleModalBackdropClick(event)">
  <div class="modal-content">
    <div class="modal-header">
      <div class="modal-title">⚙️ Device Settings</div>
      <button class="modal-close-btn" onclick="closeSettingsModal()">&times;</button>
    </div>

    <div class="setting-group">
      <div class="setting-header">
        <span>🗣️ TTS Speech Mode</span>
      </div>
      <select id="tts-mode-select" class="setting-select" onchange="setTtsMode(this.value)">
        <option value="1">Mode 1: Speak Char + Full Word on Space</option>
        <option value="2">Mode 2: Speak Full Word Only (after space)</option>
        <option value="3">Mode 3: Speak Full Buffer Line</option>
        <option value="0">Mode 0: Web Speech TTS Disabled</option>
      </select>
    </div>

    <div class="setting-group">
      <div class="setting-header">
        <span>⚡ Speech Rate / Speed</span>
        <span id="speech-rate-label">1.0x</span>
      </div>
      <input type="range" id="speech-rate-slider" min="0.5" max="2.0" step="0.1" value="1.0" class="setting-slider" oninput="document.getElementById('speech-rate-label').innerText=this.value+'x'">
    </div>

    <div class="setting-group">
      <div class="setting-header">
        <span>☀️ LCD Backlight Brightness</span>
        <span id="brightness-val-label">100%</span>
      </div>
      <input type="range" id="brightness-slider" min="0" max="100" value="100" class="setting-slider" onchange="setBrightness(this.value)" oninput="document.getElementById('brightness-val-label').innerText=this.value+'%'">
    </div>

    <div class="setting-group" style="margin-bottom: 20px;">
      <div class="setting-header">
        <span>🔊 Buzzer Volume</span>
        <span id="volume-val-label">100%</span>
      </div>
      <input type="range" id="volume-slider" min="0" max="100" value="100" class="setting-slider" onchange="setVolume(this.value)" oninput="document.getElementById('volume-val-label').innerText = (this.value > 0 ? this.value + '%' : 'MUTED (0%)')">
    </div>

    <div class="setting-group" style="margin-bottom: 16px;">
      <button onclick="closeSettingsModal(); openWizardModal();" class="btn-update" style="background: var(--blue-light); color: var(--blue);">
        🪄 Re-run Setup Wizard
      </button>
    </div>

    <div class="setting-group" style="border-top: 2px dashed var(--border); padding-top: 14px; margin-top: 14px;">
      <div class="setting-header"><span>🔐 Security & Authentication</span></div>
      <div class="wifi-input-group" style="margin-bottom: 8px;">
        <input type="password" maxlength="4" id="cfg-pin" class="wifi-input" inputmode="numeric" pattern="[0-9]*" placeholder="New 4-Digit PIN">
        <input type="text" id="cfg-sec-q" class="wifi-input" placeholder="Security Question">
        <input type="text" id="cfg-sec-a" class="wifi-input" placeholder="Security Answer">
      </div>
      <button onclick="updateSecurityConfigFromSettings()" class="btn-update" style="background: #e8f5e9; color: #2e7d32;">
        💾 Save Security Preferences
      </button>
    </div>

    <div class="ota-section" style="border-top: 2px dashed var(--border); padding-top: 14px;">
      <div class="setting-header"><span>⚡ Firmware Update</span></div>
      <button id="ota-btn" onclick="triggerOtaUpdate()" class="btn-update">
        ⚡ Check & Update Firmware (GitHub OTA)
      </button>
      <div id="ota-status"></div>
    </div>

    <div class="setting-group" style="border-top: 2px dashed var(--border); padding-top: 14px; margin-top: 14px;">
      <div style="display: flex; gap: 8px; margin-bottom: 10px;">
        <button onclick="setPowerState('off')" class="btn-update" style="flex: 1; background: #ffebee; color: #c62828;">
          🌙 Sleep
        </button>
        <button onclick="setPowerState('on')" class="btn-update" style="flex: 1; background: #e8f5e9; color: #2e7d32;">
          ☀️ Turn On
        </button>
        <button onclick="rebootEsp32()" class="btn-update" style="flex: 1; background: #fff3e0; color: #e65100;">
          🔄 Reboot
        </button>
      </div>

      <button onclick="triggerFactoryReset()" class="btn-update" style="background: #ffebee; color: #c62828; border-color: var(--red);">
        💣 Factory Reset Device
      </button>
    </div>

  </div>
</div>

<!-- Flashcard Practice Modal -->
<div class="modal-overlay" id="flashcard-modal" onclick="handleModalBackdropClick(event)">
  <div class="modal-content">
    <div class="modal-header">
      <div class="modal-title">🎓 Flashcard Practice</div>
      <button class="modal-close-btn" onclick="closeFlashcardModal()">&times;</button>
    </div>
    <div class="fc-scores">
      <div class="fc-score-badge">SCORE: <span id="fc-score">0</span></div>
      <div class="fc-score-badge">STREAK: <span id="fc-streak">0</span></div>
    </div>
    <div class="fc-card-display">
      <div class="fc-target-label">TYPE THIS CHARACTER</div>
      <div class="fc-target-char" id="fc-target">A</div>
      <div class="fc-trigger-info" id="fc-trigger-info">AUTO Mode Trigger: Dot 6 (⠠)</div>
      <div class="fc-hint-box" id="fc-hint">Hint: <span class="fc-hint-braille" id="fc-hint-braille">⠠⠁</span></div>
    </div>
    <div class="fc-status-msg" id="fc-status">Set dots on keypad & press ENTER (#)</div>
    <div class="fc-actions">
      <button class="fc-btn" onclick="toggleFlashcardHint()">Toggle Hint</button>
      <button class="fc-btn" onclick="nextFlashcardTarget()">Skip / Next</button>
    </div>
  </div>
</div>

<!-- Wi-Fi Setup Modal -->
<div class="modal-overlay" id="wifi-modal" onclick="handleModalBackdropClick(event)">
  <div class="modal-content">
    <div class="modal-header">
      <div class="modal-title">📶 Wi-Fi Settings</div>
      <button class="modal-close-btn" onclick="closeWifiModal()">&times;</button>
    </div>
    
    <div class="wifi-status-info" id="wifi-status-label">AP Mode Active (192.168.4.1)</div>

    <div class="section-title" style="margin-bottom: 6px;">Saved Networks</div>
    <div class="wifi-list" id="saved-wifi-list" style="max-height: 100px; margin-bottom: 12px;">
      <div class="wifi-item" style="justify-content:center;color:#57606a;">Loading saved networks...</div>
    </div>

    <div class="fc-actions" style="margin-bottom:10px;">
      <button class="fc-btn" onclick="scanWifiNetworks()">🔍 Scan Available Wi-Fi</button>
    </div>

    <div class="wifi-list" id="wifi-list">
      <div class="wifi-item" style="justify-content:center;color:#57606a;">Click Scan to search networks</div>
    </div>

    <div class="wifi-input-group">
      <input type="text" id="wifi-ssid" class="wifi-input" placeholder="Wi-Fi SSID (Network Name)">
      <input type="password" id="wifi-pass" class="wifi-input" placeholder="Wi-Fi Password">
    </div>

    <div class="fc-actions">
      <button class="fc-btn" style="background:#e8f5e9;color:#2e7d32;" onclick="connectWifi()">Save & Connect</button>
    </div>
  </div>
</div>

<!-- Camera OCR Scanner Modal -->
<div class="modal-overlay" id="camera-modal" onclick="handleModalBackdropClick(event)">
  <div class="modal-content">
    <div class="modal-header">
      <div class="modal-title">📷 Cloud Vision OCR</div>
      <button class="modal-close-btn" onclick="closeCameraModal()">&times;</button>
    </div>
    
    <div class="camera-box">
      <video id="camera-stream" class="camera-video" autoplay playsinline></video>
      <canvas id="ocr-canvas" style="display:none;"></canvas>
    </div>

    <div class="ocr-status" id="ocr-status-label">Align printed text & click Capture</div>

    <input type="file" id="camera-capture-input" accept="image/*" capture="environment" style="display:none;" onchange="handleFileSelect(event)">
    <input type="file" id="gallery-file-input" accept="image/*" style="display:none;" onchange="handleFileSelect(event)">

    <div class="fc-actions">
      <button class="fc-btn" style="background:#e8f5e9;color:#2e7d32;" onclick="openNativeCamera()">📸 Capture & Scan</button>
      <button class="fc-btn" onclick="openNativeGallery()">📁 Choose Image</button>
    </div>
  </div>
</div>

<script>
let currentBits = 0;
let currentFullText = "";
let currentCursorPos = 0;
let currentSelStart = 0;
let currentSelEnd = 0;
let currentTtsMode = 1;

let isSelecting = false;
let selectionAnchor = -1;

let lastSpokenText = "";
let lastSpokenCursor = 0;
let isFirstStateUpdate = true;
let hasSetupWizardPrompted = false;

let holdTimer = null;
let repeatInterval = null;

const fcDataset = [
  {ch:'a', bits:1},  {ch:'b', bits:3},  {ch:'c', bits:9},  {ch:'d', bits:25}, {ch:'e', bits:17},
  {ch:'f', bits:11}, {ch:'g', bits:27}, {ch:'h', bits:19}, {ch:'i', bits:10}, {ch:'j', bits:26},
  {ch:'k', bits:5},  {ch:'l', bits:7},  {ch:'m', bits:13}, {ch:'n', bits:29}, {ch:'o', bits:21},
  {ch:'p', bits:15}, {ch:'q', bits:31}, {ch:'r', bits:23}, {ch:'s', bits:14}, {ch:'t', bits:30},
  {ch:'u', bits:37}, {ch:'v', bits:39}, {ch:'w', bits:58}, {ch:'x', bits:45}, {ch:'y', bits:61}, {ch:'z', bits:53},
  {ch:'A', bits:1,  prefixBits:32, prefixName:'Dot 6 (⠠)'}, {ch:'B', bits:3,  prefixBits:32, prefixName:'Dot 6 (⠠)'},
  {ch:'C', bits:9,  prefixBits:32, prefixName:'Dot 6 (⠠)'}, {ch:'D', bits:25, prefixBits:32, prefixName:'Dot 6 (⠠)'},
  {ch:'E', bits:17, prefixBits:32, prefixName:'Dot 6 (⠠)'}, {ch:'F', bits:11, prefixBits:32, prefixName:'Dot 6 (⠠)'},
  {ch:'G', bits:27, prefixBits:32, prefixName:'Dot 6 (⠠)'}, {ch:'H', bits:19, prefixBits:32, prefixName:'Dot 6 (⠠)'},
  {ch:'I', bits:10, prefixBits:32, prefixName:'Dot 6 (⠠)'}, {ch:'J', bits:26, prefixBits:32, prefixName:'Dot 6 (⠠)'},
  {ch:'K', bits:5,  prefixBits:32, prefixName:'Dot 6 (⠠)'}, {ch:'L', bits:7,  prefixBits:32, prefixName:'Dot 6 (⠠)'},
  {ch:'M', bits:13, prefixBits:32, prefixName:'Dot 6 (⠠)'}, {ch:'N', bits:29, prefixBits:32, prefixName:'Dot 6 (⠠)'},
  {ch:'O', bits:21, prefixBits:32, prefixName:'Dot 6 (⠠)'}, {ch:'P', bits:15, prefixBits:32, prefixName:'Dot 6 (⠠)'},
  {ch:'Q', bits:31, prefixBits:32, prefixName:'Dot 6 (⠠)'}, {ch:'R', bits:23, prefixBits:32, prefixName:'Dot 6 (⠠)'},
  {ch:'S', bits:14, prefixBits:32, prefixName:'Dot 6 (⠠)'}, {ch:'T', bits:30, prefixBits:32, prefixName:'Dot 6 (⠠)'},
  {ch:'U', bits:37, prefixBits:32, prefixName:'Dot 6 (⠠)'}, {ch:'V', bits:39, prefixBits:32, prefixName:'Dot 6 (⠠)'},
  {ch:'W', bits:58, prefixBits:32, prefixName:'Dot 6 (⠠)'}, {ch:'X', bits:45, prefixBits:32, prefixName:'Dot 6 (⠠)'},
  {ch:'Y', bits:61, prefixBits:32, prefixName:'Dot 6 (⠠)'}, {ch:'Z', bits:53, prefixBits:32, prefixName:'Dot 6 (⠠)'},
  {ch:'1', bits:1,  prefixBits:60, prefixName:'Num Ind # (⠼)'}, {ch:'2', bits:3,  prefixBits:60, prefixName:'Num Ind # (⠼)'},
  {ch:'3', bits:9,  prefixBits:60, prefixName:'Num Ind # (⠼)'}, {ch:'4', bits:25, prefixBits:60, prefixName:'Num Ind # (⠼)'},
  {ch:'5', bits:17, prefixBits:60, prefixName:'Num Ind # (⠼)'}, {ch:'6', bits:11, prefixBits:60, prefixName:'Num Ind # (⠼)'},
  {ch:'7', bits:27, prefixBits:60, prefixName:'Num Ind # (⠼)'}, {ch:'8', bits:19, prefixBits:60, prefixName:'Num Ind # (⠼)'},
  {ch:'9', bits:10, prefixBits:60, prefixName:'Num Ind # (⠼)'}, {ch:'0', bits:26, prefixBits:60, prefixName:'Num Ind # (⠼)'},
  {ch:'?', bits:38, prefixBits:56, prefixName:'Spec Ind $ (⠸)'}, {ch:'!', bits:22, prefixBits:56, prefixName:'Spec Ind $ (⠸)'},
  {ch:'&', bits:47, prefixBits:56, prefixName:'Spec Ind $ (⠸)'}, {ch:',', bits:2,  prefixBits:56, prefixName:'Spec Ind $ (⠸)'},
  {ch:'.', bits:50, prefixBits:56, prefixName:'Spec Ind $ (⠸)'}, {ch:';', bits:6,  prefixBits:56, prefixName:'Spec Ind $ (⠸)'},
  {ch:':', bits:18, prefixBits:56, prefixName:'Spec Ind $ (⠸)'}, {ch:'@', bits:44, prefixBits:56, prefixName:'Spec Ind $ (⠸)'},
  {ch:'*', bits:20, prefixBits:56, prefixName:'Spec Ind $ (⠸)'}, {ch:'/', bits:12, prefixBits:56, prefixName:'Spec Ind $ (⠸)'},
  {ch:'\'', bits:4, prefixBits:56, prefixName:'Spec Ind $ (⠸)'}
];

let fcTargetIndex = 0;
let fcScore = 0;
let fcStreak = 0;
let fcShowHint = false;
let lastEvaluatedTextLen = 0;
let currentBrightness = 100;
let currentVolume = 100;

(function breakoutCaptivePortal() {
  const ua = navigator.userAgent || '';
  const isAndroid = /Android/i.test(ua);
  const isWebView = /wv|Android.*Version\/[0-9]\.[0-9]|FBAN|FBAV/i.test(ua);

  if (isAndroid && (isWebView || !window.speechSynthesis)) {
    const targetUrl = window.location.host || '192.168.4.1';
    window.location.href = 'intent://' + targetUrl + '/#Intent;scheme=http;action=android.intent.action.VIEW;end;';
  }
})();

function updateBodyScrollLock() {
  const anyModalActive = document.querySelector('.modal-overlay.active');
  if (anyModalActive) {
    document.body.classList.add('modal-open');
  } else {
    document.body.classList.remove('modal-open');
  }
}

function openWizardModal() {
  nextWizardStep(1);
  document.getElementById('wizard-modal').classList.add('active');
  updateBodyScrollLock();
}

function closeWizardModal() {
  document.getElementById('wizard-modal').classList.remove('active');
  updateBodyScrollLock();
}

function nextWizardStep(stepNum) {
  for (let i = 1; i <= 6; i++) {
    const stepEl = document.getElementById('wstep-' + i);
    const dotEl = document.getElementById('wdot-' + i);
    if (stepEl) stepEl.classList.remove('active');
    if (dotEl) {
      dotEl.classList.remove('active');
      if (i < stepNum) dotEl.classList.add('completed');
    }
  }
  const targetStep = document.getElementById('wstep-' + stepNum);
  const targetDot = document.getElementById('wdot-' + stepNum);
  if (targetStep) targetStep.classList.add('active');
  if (targetDot) targetDot.classList.add('active');
}

function prevWizardStep(stepNum) {
  nextWizardStep(stepNum);
}

function saveWizardSecurity() {
  const pin = document.getElementById('w-pin-input').value;
  const q = document.getElementById('w-sec-q-input').value;
  const a = document.getElementById('w-sec-a-input').value;

  if (pin.length !== 4 || !q || !a) {
    alert('Please enter a valid 4-digit PIN, security question, and answer.');
    return;
  }

  fetch(`/api/save_security?pin=${encodeURIComponent(pin)}&secQ=${encodeURIComponent(q)}&secA=${encodeURIComponent(a)}`)
    .then(r => r.json())
    .then(() => nextWizardStep(5))
    .catch(() => nextWizardStep(5));
}

function updateSecurityConfigFromSettings() {
  const pin = document.getElementById('cfg-pin').value;
  const q = document.getElementById('cfg-sec-q').value;
  const a = document.getElementById('cfg-sec-a').value;

  if (pin.length !== 4 || !q || !a) {
    alert('Please enter a valid 4-digit PIN, security question, and answer.');
    return;
  }

  fetch(`/api/save_security?pin=${encodeURIComponent(pin)}&secQ=${encodeURIComponent(q)}&secA=${encodeURIComponent(a)}`)
    .then(r => r.json())
    .then(d => {
      alert('✅ ' + (d.msg || 'Security configuration saved to Preferences!'));
      document.getElementById('cfg-pin').value = '';
      document.getElementById('cfg-sec-q').value = '';
      document.getElementById('cfg-sec-a').value = '';
    })
    .catch(() => alert('❌ Failed to update security preferences.'));
}

let isAuthenticated = false;

function checkPinAuthOnLoad() {
  const isAuthed = sessionStorage.getItem('braillify_authed') === 'true';
  const lockModal = document.getElementById('pin-lock-modal');

  if (isAuthed) {
    isAuthenticated = true;
    if (lockModal) lockModal.classList.remove('active');
  } else {
    isAuthenticated = false;
    if (lockModal) lockModal.classList.add('active');
  }
  updateBodyScrollLock();
}

function focusNextPin(currentIdx) {
  const current = document.getElementById('pin-d' + currentIdx);
  if (current.value.length === 1 && currentIdx < 4) {
    document.getElementById('pin-d' + (currentIdx + 1)).focus();
  }
  if (currentIdx === 4 && current.value.length === 1) {
    submitPinAuth();
  }
}

function submitPinAuth() {
  const p1 = document.getElementById('pin-d1').value;
  const p2 = document.getElementById('pin-d2').value;
  const p3 = document.getElementById('pin-d3').value;
  const p4 = document.getElementById('pin-d4').value;
  const fullPin = p1 + p2 + p3 + p4;

  const errMsg = document.getElementById('pin-error-msg');
  if (fullPin.length !== 4) {
    errMsg.innerText = 'Please enter all 4 digits.';
    return;
  }

  fetch('/api/verify_pin?pin=' + encodeURIComponent(fullPin))
    .then(r => {
      if (!r.ok) throw new Error('Incorrect PIN');
      return r.json();
    })
    .then(data => {
      sessionStorage.setItem('braillify_authed', 'true');
      isAuthenticated = true;
      const lockModal = document.getElementById('pin-lock-modal');
      if (lockModal) lockModal.classList.remove('active');
      updateBodyScrollLock();
      
      speakTextUtterance('Connected to Braillify', true);
      alert('✅ ' + (data.msg || 'Connected to Braillify!'));
    })
    .catch(() => {
      errMsg.innerText = '❌ Incorrect PIN. Try again.';
      document.getElementById('pin-d1').value = '';
      document.getElementById('pin-d2').value = '';
      document.getElementById('pin-d3').value = '';
      document.getElementById('pin-d4').value = '';
      document.getElementById('pin-d1').focus();
    });
}

function showForgotPinView() {
  document.getElementById('pin-entry-view').style.display = 'none';
  document.getElementById('pin-forgot-view').style.display = 'block';

  fetch('/api/get_sec_question')
    .then(r => r.json())
    .then(d => {
      document.getElementById('forgot-sec-question').innerText = d.question || 'What is your pet\'s name?';
    });
}

function hideForgotPinView() {
  document.getElementById('pin-forgot-view').style.display = 'none';
  document.getElementById('pin-entry-view').style.display = 'block';
}

function submitForgotPinReset() {
  const ans = document.getElementById('forgot-answer-input').value;
  const newPin = document.getElementById('forgot-new-pin').value;

  if (!ans || newPin.length !== 4) {
    alert('Please enter your security answer and a new 4-digit PIN.');
    return;
  }

  fetch(`/api/reset_pin?answer=${encodeURIComponent(ans)}&newPin=${encodeURIComponent(newPin)}`)
    .then(r => {
      if (!r.ok) throw new Error('Reset failed');
      return r.json();
    })
    .then(d => {
      sessionStorage.setItem('braillify_authed', 'true');
      isAuthenticated = true;
      document.getElementById('pin-lock-modal').classList.remove('active');
      updateBodyScrollLock();
      alert('✅ ' + d.msg);
    })
    .catch(() => {
      alert('❌ Incorrect security answer. Reset failed.');
    });
}

checkPinAuthOnLoad();

function triggerFactoryReset() {
  const confirmPin = prompt('⚠️ FACTORY RESET WARNING:\nThis will permanently erase all Wi-Fi credentials, PIN security, and memory.\n\nEnter your 4-digit PIN to confirm:');
  if (!confirmPin) return;

  fetch('/api/factory_reset?pin=' + encodeURIComponent(confirmPin))
    .then(r => {
      if (!r.ok) throw new Error('Incorrect PIN');
      return r.json();
    })
    .then(d => {
      alert('💣 ' + (d.status || 'Factory reset authorized. ESP32 is wiping data...'));
      setTimeout(() => location.reload(), 3000);
    })
    .catch(err => alert('❌ Factory Reset failed: Incorrect PIN.'));
}

function connectWifiFromWizard() {
  const ssid = document.getElementById('w-wifi-ssid').value;
  const pass = document.getElementById('w-wifi-pass').value;
  if(!ssid){ alert('Please enter a Wi-Fi SSID'); return; }

  fetch(`/api/connect?ssid=${encodeURIComponent(ssid)}&pass=${encodeURIComponent(pass)}`)
    .then(r => r.json())
    .then(d => { alert(d.status || 'Connecting Wi-Fi...'); });
}

function finishWizardSetup() {
  fetch('/api/finish_setup')
    .then(r => r.json())
    .then(() => {
      closeWizardModal();
      speakTextUtterance('Braillify Setup Complete!', true);
    })
    .catch(() => closeWizardModal());
}

function openFlashcardModal(){ 
  document.getElementById('flashcard-modal').classList.add('active'); 
  updateBodyScrollLock();
  lastEvaluatedTextLen = currentFullText.length;
}
function closeFlashcardModal(){ 
  document.getElementById('flashcard-modal').classList.remove('active'); 
  updateBodyScrollLock();
}

function toggleFlashcardHint() {
  fcShowHint = !fcShowHint;
  document.getElementById('fc-hint').style.display = fcShowHint ? 'block' : 'none';
}

function nextFlashcardTarget(){
  let nextIdx;
  do { nextIdx = Math.floor(Math.random() * fcDataset.length); } while (nextIdx === fcTargetIndex);
  fcTargetIndex = nextIdx;
  const targetObj = fcDataset[fcTargetIndex];
  
  document.getElementById('fc-target').innerText = targetObj.ch;
  
  const triggerEl = document.getElementById('fc-trigger-info');
  if (targetObj.prefixName) {
    triggerEl.style.display = 'inline-block';
    triggerEl.innerText = `AUTO Mode Trigger: ${targetObj.prefixName}`;
  } else {
    triggerEl.style.display = 'none';
  }

  let hintBraille = "";
  if (targetObj.prefixBits) {
    hintBraille += String.fromCharCode(0x2800 + targetObj.prefixBits);
  }
  hintBraille += String.fromCharCode(0x2800 + targetObj.bits);
  document.getElementById('fc-hint-braille').innerText = hintBraille;

  const statusEl = document.getElementById('fc-status');
  if (statusEl) {
    statusEl.className = 'fc-status-msg';
    statusEl.innerText = 'Set dots on keypad & press ENTER (#)';
  }
}

function checkFlashcardAnswer(d) {
  const modal = document.getElementById('flashcard-modal');
  if (!modal || !modal.classList.contains('active')) {
    lastEvaluatedTextLen = d.fullText ? d.fullText.length : 0;
    return;
  }

  const currentLen = d.fullText ? d.fullText.length : 0;
  if (currentLen > lastEvaluatedTextLen) {
    const typedChar = d.fullText[d.cursor - 1] ? d.fullText[d.cursor - 1] : '';
    const targetObj = fcDataset[fcTargetIndex];
    const statusEl = document.getElementById('fc-status');

    if (typedChar === targetObj.ch) {
      fcScore += 10;
      fcStreak += 1;
      document.getElementById('fc-score').innerText = fcScore;
      document.getElementById('fc-streak').innerText = fcStreak;
      statusEl.className = 'fc-status-msg correct';
      statusEl.innerText = '✨ Correct! Excellent job!';
      speakTextUtterance('Correct', true);
      setTimeout(() => {
        nextFlashcardTarget();
      }, 800);
    } else if (typedChar !== ' ' && typedChar !== '') {
      fcStreak = 0;
      document.getElementById('fc-streak').innerText = fcStreak;
      statusEl.className = 'fc-status-msg wrong';
      statusEl.innerText = `❌ Got '${typedChar}', expected '${targetObj.ch}'`;
      speakTextUtterance('Try again', true);
    }
  }
  lastEvaluatedTextLen = currentLen;
}

function openWifiModal(){ 
  document.getElementById('wifi-modal').classList.add('active'); 
  updateBodyScrollLock();
  loadSavedWifis();
  scanWifiNetworks();
}

function loadSavedWifis() {
  const listEl = document.getElementById('saved-wifi-list');
  fetch('/api/wifi/saved')
    .then(async res => {
      if (!res.ok) throw new Error('HTTP Status ' + res.status);
      const contentType = res.headers.get('content-type');
      if (!contentType || !contentType.includes('application/json')) {
        throw new Error('ESP32 returned non-JSON response');
      }
      return res.json();
    })
    .then(networks => {
      listEl.innerHTML = '';
      if (!networks || networks.length === 0) {
        listEl.innerHTML = '<div class="wifi-item" style="justify-content:center;color:#57606a;">No saved networks</div>';
        return;
      }
      networks.forEach(net => {
        const item = document.createElement('div');
        item.className = 'wifi-item';
        item.innerHTML = `
          <span>${net.ssid}</span>
          <div style="display:flex; gap:6px;">
            <button class="action-btn" style="padding:2px 6px; font-size:0.7rem;" onclick="editSavedWifi('${net.ssid}', '${net.pass}')">✏️ Edit</button>
            <button class="action-btn" style="padding:2px 6px; font-size:0.7rem; background:#ffebee; color:#c62828;" onclick="forgetWifi('${net.ssid}')">🗑️ Forget</button>
          </div>
        `;
        listEl.appendChild(item);
      });
    })
    .catch(err => {
      console.error('[Wi-Fi Load Error]', err);
      listEl.innerHTML = '<div class="wifi-item" style="justify-content:center;color:red;">Failed to load saved networks</div>';
    });
}

function editSavedWifi(ssid, pass) {
  document.getElementById('wifi-ssid').value = ssid;
  document.getElementById('wifi-pass').value = pass;
}

function forgetWifi(ssid) {
  if (confirm(`Forget saved network "${ssid}"?`)) {
    fetch('/api/wifi/forget?ssid=' + encodeURIComponent(ssid))
      .then(r => r.json())
      .then(() => loadSavedWifis());
  }
}

function closeWifiModal(){ 
  document.getElementById('wifi-modal').classList.remove('active'); 
  updateBodyScrollLock();
}

let currentFeedbackMode = 0;

function openSettingsModal(){ 
  document.getElementById('brightness-slider').value = currentBrightness;
  document.getElementById('brightness-val-label').innerText = currentBrightness + '%';
  
  const activeVol = (currentFeedbackMode === 1) ? 0 : currentVolume;
  document.getElementById('volume-slider').value = activeVol;
  document.getElementById('volume-val-label').innerText = (activeVol > 0) ? activeVol + '%' : 'MUTED (0%)';
  
  document.getElementById('tts-mode-select').value = currentTtsMode;
  document.getElementById('settings-modal').classList.add('active'); 
  updateBodyScrollLock();
}

function closeSettingsModal(){ 
  document.getElementById('settings-modal').classList.remove('active'); 
  updateBodyScrollLock();
}

function setBrightness(val) {
  currentBrightness = parseInt(val);
  fetch('/api/settings?brightness=' + val)
    .then(r => r.json())
    .then(d => updateState(d));
}

function setVolume(val) {
  currentVolume = parseInt(val);
  fetch('/api/settings?volume=' + val)
    .then(r => r.json())
    .then(d => updateState(d));
}

function setTtsMode(val) {
  currentTtsMode = parseInt(val);
  fetch('/api/settings?ttsMode=' + val)
    .then(r => r.json())
    .then(d => updateState(d));
}

let cameraStream = null;
function openCameraModal(){
  document.getElementById('camera-modal').classList.add('active');
  updateBodyScrollLock();
  startCameraStream();
}

function closeCameraModal(){
  stopCameraStream();
  document.getElementById('camera-modal').classList.remove('active');
  updateBodyScrollLock();
}

function startCameraStream(){
  const video = document.getElementById('camera-stream');
  const label = document.getElementById('ocr-status-label');
  label.innerText = 'Initializing camera...';

  if (navigator.mediaDevices && navigator.mediaDevices.getUserMedia) {
    navigator.mediaDevices.getUserMedia({ video: { facingMode: 'environment' } })
      .then(stream => {
        cameraStream = stream;
        video.srcObject = stream;
        label.innerText = 'Align printed text & click Capture';
      })
      .catch(err => {
        console.error('[Camera Access Error]', err);
        label.innerText = 'Camera feed unavailable. Use "Choose Image"';
      });
  } else {
    label.innerText = 'Camera API unavailable. Use "Choose Image"';
  }
}

function stopCameraStream(){
  if (cameraStream) {
    cameraStream.getTracks().forEach(track => track.stop());
    cameraStream = null;
  }
}

function openNativeCamera() {
  document.getElementById('camera-capture-input').click();
}

function openNativeGallery() {
  document.getElementById('gallery-file-input').click();
}

function setPowerState(state) {
  fetch('/api/power?state=' + state)
    .then(r => r.json())
    .then(d => updateState(d));
}

function rebootEsp32() {
  if (confirm("Reboot the ESP32 device now?")) {
    fetch('/api/reboot')
      .then(r => r.json())
      .then(d => alert(d.status || "Rebooting ESP32..."))
      .catch(() => alert("Reboot command sent."));
  }
}

async function runCloudOCR(base64DataUrl) {
  const label = document.getElementById('ocr-status-label');
  label.innerText = '⏳ Analyzing image with Cloud Vision API...';

  try {
    const formData = new FormData();
    formData.append('base64Image', base64DataUrl);
    formData.append('language', 'eng');
    formData.append('isTable', 'false');
    formData.append('scale', 'true');
    formData.append('OCREngine', '2');
    formData.append('apikey', 'helloworld');

    const res = await fetch('https://api.ocr.space/parse/image', {
      method: 'POST',
      body: formData
    });

    if (!res.ok) throw new Error('Cloud OCR service unreachable');
    const data = await res.json();

    if (data && data.ParsedResults && data.ParsedResults.length > 0) {
      let text = data.ParsedResults[0].ParsedText ? data.ParsedResults[0].ParsedText.trim().replace(/[\r\n]+/g, ' ') : '';
      text = text.replace(/[^\x20-\x7E]/g, '');

      if (text.length > 0) {
        label.innerText = `✅ Found: "${text}"`;

        let updatedText = currentFullText;
        if (updatedText.length > 0 && !updatedText.endsWith(' ')) updatedText += ' ';
        updatedText += text;

        if (updatedText.length > 120) updatedText = updatedText.substring(0, 120);

        fetch(`/api/settext?text=${encodeURIComponent(updatedText)}`)
          .then(r => r.json())
          .then(d => {
            updateState(d);
            setTimeout(closeCameraModal, 1200);
          });
      } else {
        label.innerText = '⚠️ No readable text detected.';
      }
    } else {
      label.innerText = '❌ OCR Error: Image not readable.';
    }
  } catch (err) {
    console.error('[Cloud OCR Error]', err);
    label.innerText = '❌ Network error during OCR scan.';
  }
}

function handleFileSelect(e) {
  const file = e.target.files[0];
  if (!file) return;

  const label = document.getElementById('ocr-status-label');
  label.innerText = 'Reading image file...';

  const reader = new FileReader();
  reader.onload = function(evt) {
    runCloudOCR(evt.target.result);
  };
  reader.readAsDataURL(file);
}

function handleModalBackdropClick(e){
  if(e.target.id === 'flashcard-modal') closeFlashcardModal();
  if(e.target.id === 'wifi-modal') closeWifiModal();
  if(e.target.id === 'camera-modal') closeCameraModal();
  if(e.target.id === 'settings-modal') closeSettingsModal();
}

function scanWifiNetworks(){
  const listEl = document.getElementById('wifi-list');
  const wizardListEl = document.getElementById('w-wifi-list');

  const loadingHtml = '<div class="wifi-item" style="justify-content:center;">Scanning networks...</div>';
  if (listEl) listEl.innerHTML = loadingHtml;
  if (wizardListEl) wizardListEl.innerHTML = loadingHtml;

  fetch('/api/scan')
    .then(r => r.json())
    .then(networks => {
      const render = (target) => {
        if (!target) return;
        target.innerHTML = '';
        if (!networks || networks.length === 0) {
          target.innerHTML = '<div class="wifi-item" style="justify-content:center;">No networks found</div>';
          return;
        }
        networks.forEach(net => {
          const item = document.createElement('div');
          item.className = 'wifi-item';
          item.innerHTML = `<span>${net.ssid}</span><span class="wifi-rssi">${net.rssi} dBm</span>`;
          item.onclick = () => { 
            const mainInput = document.getElementById('wifi-ssid');
            const wizardInput = document.getElementById('w-wifi-ssid');
            if (mainInput) mainInput.value = net.ssid;
            if (wizardInput) wizardInput.value = net.ssid;
          };
          target.appendChild(item);
        });
      };

      render(listEl);
      render(wizardListEl);
    })
    .catch(() => {
      const errHtml = '<div class="wifi-item" style="justify-content:center;color:red;">Scan failed</div>';
      if (listEl) listEl.innerHTML = errHtml;
      if (wizardListEl) wizardListEl.innerHTML = errHtml;
    });
}

function connectWifi(){
  const ssid = document.getElementById('wifi-ssid').value;
  const pass = document.getElementById('wifi-pass').value;
  if(!ssid){ alert('Please enter a Wi-Fi SSID'); return; }

  const label = document.getElementById('wifi-status-label');
  label.innerText = 'Connecting to ' + ssid + '...';

  fetch(`/api/connect?ssid=${encodeURIComponent(ssid)}&pass=${encodeURIComponent(pass)}`)
    .then(r => r.json())
    .then(d => { label.innerText = d.status; });
}

function updateVisualSelection(start, end) {
  currentSelStart = start;
  currentSelEnd = end;
  const hasRange = start !== end && start < end;
  const pairs = document.querySelectorAll('#full-text-box .char-pair');

  pairs.forEach((pair) => {
    const i = parseInt(pair.getAttribute('data-idx'));
    if (hasRange && i >= start && i < end) {
      pair.classList.add('selected');
      pair.classList.remove('cursor-here');
    } else if (!hasRange && i === currentCursorPos) {
      pair.classList.remove('selected');
      pair.classList.add('cursor-here');
    } else {
      pair.classList.remove('selected');
      pair.classList.remove('cursor-here');
    }
  });

  const toolbar = document.getElementById('selection-toolbar');
  if (toolbar) {
    if (hasRange) {
      toolbar.classList.add('active');
      document.getElementById('sel-count-label').innerText = end - start;
    } else {
      toolbar.classList.remove('active');
    }
  }
}

function renderFullText(text, pos, brailleArr, selStart, selEnd){
  currentFullText = text;
  currentCursorPos = pos;
  
  if (isSelecting) {
    return;
  }

  currentSelStart = selStart;
  currentSelEnd = selEnd;

  const container = document.getElementById('full-text-box');
  container.innerHTML = '';
  const len = text.length;

  const hasRange = selStart !== selEnd && selStart < selEnd;
  const toolbar = document.getElementById('selection-toolbar');
  
  if (toolbar) {
    if (hasRange) {
      toolbar.classList.add('active');
      document.getElementById('sel-count-label').innerText = selEnd - selStart;
    } else {
      toolbar.classList.remove('active');
    }
  }

  for(let i = 0; i <= len; i++){
    const pairDiv = document.createElement('div');
    pairDiv.className = 'char-pair';
    pairDiv.setAttribute('data-idx', i);

    if (hasRange && i >= selStart && i < selEnd) {
      pairDiv.classList.add('selected');
    } else if (i === pos && !hasRange) {
      pairDiv.classList.add('cursor-here');
    }

    const topChar = document.createElement('div');
    topChar.className = 'top-char';
    const subBraille = document.createElement('div');
    subBraille.className = 'sub-braille';

    if(i < len){
      let ch = text[i];
      topChar.innerText = (ch === ' ') ? '\u00A0' : ch;
      let bits = (brailleArr && brailleArr[i] !== undefined) ? brailleArr[i] : 0;
      subBraille.innerText = bits > 0 ? String.fromCharCode(0x2800 + bits) : '\u2800';
    } else {
      topChar.innerText = '\u00A0';
      subBraille.innerText = '\u2800';
    }
    pairDiv.appendChild(topChar);
    pairDiv.appendChild(subBraille);

    pairDiv.addEventListener('mousedown', (e) => { e.preventDefault(); startSelection(i); });
    pairDiv.addEventListener('mouseenter', () => { if (isSelecting) updateSelection(i); });
    
    container.appendChild(pairDiv);
  }
}

function startSelection(idx) {
  isSelecting = true;
  selectionAnchor = idx;
  currentSelStart = idx;
  currentSelEnd = idx;
  currentCursorPos = idx;
  updateVisualSelection(idx, idx);
}

function updateSelection(idx) {
  if (!isSelecting || selectionAnchor === -1 || idx === null) return;
  const start = Math.min(selectionAnchor, idx);
  const end = Math.max(selectionAnchor, idx);
  currentCursorPos = end;
  updateVisualSelection(start, end);
}

function finishSelection(e) {
  if (!isSelecting) return;
  isSelecting = false;
  selectionAnchor = -1;

  if (currentSelStart === currentSelEnd) {
    fetch('/api/setcursor?pos=' + currentSelStart).then(r => r.json()).then(d => updateState(d));
  } else {
    fetch(`/api/setcursor?start=${currentSelStart}&end=${currentSelEnd}`).then(r => r.json()).then(d => updateState(d));
  }
}

document.addEventListener('mouseup', finishSelection);
document.addEventListener('touchend', finishSelection);

const textContainer = document.getElementById('full-text-box');

function getCharIdxFromTouch(touch) {
  if (!touch) return null;
  const target = document.elementFromPoint(touch.clientX, touch.clientY);
  if (!target) return null;
  const pair = target.closest('.char-pair');
  if (!pair) return null;
  const idx = parseInt(pair.getAttribute('data-idx'));
  return isNaN(idx) ? null : idx;
}

textContainer.addEventListener('touchstart', (e) => {
  if (e.touches.length > 1) return;
  const idx = getCharIdxFromTouch(e.touches[0]);
  if (idx !== null) {
    if (e.cancelable) e.preventDefault();
    startSelection(idx);
  }
}, { passive: false });

textContainer.addEventListener('touchmove', (e) => {
  if (!isSelecting) return;
  const idx = getCharIdxFromTouch(e.touches[0]);
  if (idx !== null) {
    if (e.cancelable) e.preventDefault();
    updateSelection(idx);
  }
}, { passive: false });

function fallbackCopyText(text, successCallback) {
  const ta = document.createElement('textarea');
  ta.value = text;
  ta.style.position = 'fixed';
  ta.style.top = '0';
  ta.style.left = '0';
  ta.style.width = '2em';
  ta.style.height = '2em';
  ta.style.padding = '0';
  ta.style.border = 'none';
  ta.style.outline = 'none';
  ta.style.boxShadow = 'none';
  ta.style.background = 'transparent';
  document.body.appendChild(ta);
  ta.focus();
  ta.select();
  try {
    const successful = document.execCommand('copy');
    if (successful && successCallback) successCallback();
  } catch (err) {
    console.error('Fallback copy error:', err);
  }
  document.body.removeChild(ta);
}

let currentUtterance = null;
let ttsUnlocked = false;

function unlockTts() {
  if (!('speechSynthesis' in window)) return;
  try {
    if (window.speechSynthesis.paused) {
      window.speechSynthesis.resume();
    }
    window.speechSynthesis.getVoices();
    if (!ttsUnlocked) {
      const dummy = new SpeechSynthesisUtterance(' ');
      dummy.volume = 0.01;
      window.speechSynthesis.speak(dummy);
      ttsUnlocked = true;
    }
  } catch (e) {
    console.error('[TTS Unlock Warning]', e);
  }
}

document.addEventListener('click', unlockTts);
document.addEventListener('touchstart', unlockTts);

if ('speechSynthesis' in window) {
  window.speechSynthesis.onvoiceschanged = () => {
    try { window.speechSynthesis.getVoices(); } catch(e){}
  };
}

function speakTextUtterance(str, force = false) {
  if (!str || str.trim() === '') return;
  if (!force && currentTtsMode === 0) return;

  if ('speechSynthesis' in window) {
    try {
      window.speechSynthesis.cancel();
      
      if (window.speechSynthesis.paused) {
        window.speechSynthesis.resume();
      }

      const rateVal = parseFloat(document.getElementById('speech-rate-slider').value) || 1.0;
      currentUtterance = new SpeechSynthesisUtterance(str);
      currentUtterance.rate = rateVal;

      const voices = window.speechSynthesis.getVoices();
      if (voices && voices.length > 0) {
        const prefVoice = voices.find(v => v.lang && v.lang.startsWith('en')) || voices[0];
        if (prefVoice) currentUtterance.voice = prefVoice;
      }

      currentUtterance.onerror = function(e) {
        console.error('[TTS Speech Error]', e);
      };

      window.speechSynthesis.speak(currentUtterance);
    } catch (err) {
      console.error('[TTS Exception]', err);
    }
  } else {
    alert('Web Speech TTS is not supported in this captive browser popup. Please open in Chrome or Safari at http://192.168.4.1');
  }
}

function handleLiveTtsFeedback(d) {
  if (currentTtsMode === 0) return;

  const newText = d.fullText || "";
  const oldText = lastSpokenText;
  const newPos = d.cursor || 0;

  if (newText !== oldText && newText.length > 0) {
    const spacePressed = (newPos > 0 && newText[newPos - 1] === ' ');

    if (spacePressed) {
      if (currentTtsMode === 1 || currentTtsMode === 2) {
        const leftText = newText.substring(0, newPos - 1);
        const match = leftText.match(/([a-zA-Z0-9]+)$/);
        if (match) {
          speakTextUtterance(match[1]);
        } else if (currentTtsMode === 1) {
          speakTextUtterance("space");
        }
      } else if (currentTtsMode === 3) {
        speakTextUtterance(newText.trim());
      }
    } else {
      const diffLen = newText.length - oldText.length;
      if (diffLen > 0 || (oldText.length === 0 && newText.length > 0)) {
        const actualDiff = diffLen > 0 ? diffLen : newText.length;
        const insertedStart = Math.max(0, newPos - actualDiff);
        const insertedChunk = newText.substring(insertedStart, newPos);

        if (currentTtsMode === 1) {
          let chunkToSpeak = insertedChunk;
          const lowerChunk = chunkToSpeak.toLowerCase();
          
          const phoneticMap = {
            'ch': 'ch', 'gh': 'G H', 'sh': 'sh', 'th': 'th', 'wh': 'W H',
            'ed': 'E D', 'er': 'er', 'ow': 'ow', 'en': 'E N', 'ar': 'ar',
            'ou': 'ow', 'ea': 'E A', 'bb': 'B B', 'cc': 'C C', 'dd': 'D D',
            'ff': 'F F', 'gg': 'G G', 'in': 'in', 'by': 'by', 'st': 'S T', 'ing': 'ing'
          };
          if (phoneticMap[lowerChunk]) {
            chunkToSpeak = phoneticMap[lowerChunk];
          }

          if (chunkToSpeak.trim().length > 0) {
            speakTextUtterance(chunkToSpeak);
          }
        } else if (currentTtsMode === 3) {
          speakTextUtterance(newText.trim());
        }
      }
    }
  }

  lastSpokenText = newText;
  lastSpokenCursor = newPos;
}

function speakSelection() {
  const start = Math.min(currentSelStart, currentSelEnd);
  const end = Math.max(currentSelStart, currentSelEnd);
  if (start < end && currentFullText) {
    const selText = currentFullText.substring(start, end);
    speakTextUtterance(selText, true);
  }
}

function speakFullTextOrSelection() {
  if (currentSelStart < currentSelEnd) {
    speakSelection();
  } else {
    speakTextUtterance(currentFullText, true);
  }
}

function copySelection() {
  const start = Math.min(currentSelStart, currentSelEnd);
  const end = Math.max(currentSelStart, currentSelEnd);

  if (start < end && currentFullText) {
    const selText = currentFullText.substring(start, end);
    const selCopyBtn = document.getElementById('sel-copy-btn');

    const flashSelCopyStatus = () => {
      if (selCopyBtn) {
        selCopyBtn.innerText = 'Copied!';
        setTimeout(() => selCopyBtn.innerText = '📋 Copy', 1500);
      }
    };

    if (navigator.clipboard && navigator.clipboard.writeText) {
      navigator.clipboard.writeText(selText)
        .then(flashSelCopyStatus)
        .catch(() => fallbackCopyText(selText, flashSelCopyStatus));
    } else {
      fallbackCopyText(selText, flashSelCopyStatus);
    }
  }
}

function copyFullText(){
  const text = currentFullText;
  if (!text) return;

  if (navigator.clipboard && navigator.clipboard.writeText) {
    navigator.clipboard.writeText(text)
      .then(() => flashCopyStatus())
      .catch(() => fallbackCopyText(text, flashCopyStatus));
  } else {
    fallbackCopyText(text, flashCopyStatus);
  }
}

function flashCopyStatus(){
  const label = document.getElementById('copy-btn-text');
  label.innerText = 'Copied!';
  setTimeout(() => label.innerText = 'Copy Text', 1500);
}

function deleteSelectionWeb() {
  sendKey('D');
}

function setCursorPos(idx){ fetch('/api/setcursor?pos=' + idx).then(r => r.json()).then(d => updateState(d)); }
function sendKey(k){ 
  fetch('/api/key?k=' + encodeURIComponent(k))
    .then(r => r.json())
    .then(d => updateState(d)); 
}

function startKeyHold(k){
  sendKey(k);
  if(['0','D','7','C'].includes(k)){
    holdTimer = setTimeout(() => { repeatInterval = setInterval(() => sendKey(k), 120); }, 600);
  }
}

function stopKeyHold(){
  if(holdTimer){ clearTimeout(holdTimer); holdTimer = null; }
  if(repeatInterval){ clearInterval(repeatInterval); repeatInterval = null; }
}

document.querySelectorAll('.key-btn').forEach(btn => {
  const k = btn.getAttribute('data-key');
  if(!k) return;
  btn.addEventListener('mousedown', (e) => { e.preventDefault(); startKeyHold(k); });
  btn.addEventListener('mouseup', stopKeyHold);
  btn.addEventListener('mouseleave', stopKeyHold);
  btn.addEventListener('touchstart', (e) => { e.preventDefault(); startKeyHold(k); });
  btn.addEventListener('touchend', stopKeyHold);
  btn.addEventListener('touchcancel', stopKeyHold);
});

function updateDotsUI(){
  const dots=[1,2,4,8,16,32];
  const btnMap=[1,2,3,4,5,6];
  dots.forEach((bit,idx)=>{
    let dotNum=btnMap[idx];
    let el=document.getElementById('vdot'+dotNum);
    if(el) (bit & currentBits)?el.classList.add('active'):el.classList.remove('active');
  });
  document.getElementById('bit-val').innerText='0b'+currentBits.toString(2).padStart(6,'0');
  document.getElementById('dec-val').innerText=currentBits;
}

function updateWifiSignalUI(connected, rssi) {
  const symbolEl = document.getElementById('sig-symbol');
  const dbmLabel = document.getElementById('sig-dbm-label');

  if (!connected || rssi <= -99) {
    symbolEl.innerText = '📡';
    dbmLabel.innerText = 'AP Mode';
    return;
  }

  dbmLabel.innerText = rssi + ' dBm';

  if (rssi >= -55) symbolEl.innerText = '🛜';
  else symbolEl.innerText = '📶';
}

let currentBootId = null;

function updateState(d){
  if (isFirstStateUpdate) {
    lastSpokenText = d.fullText || "";
    lastSpokenCursor = d.cursor || 0;
    isFirstStateUpdate = false;
  } else {
    handleLiveTtsFeedback(d);
  }

  if (d.setupDone === false && !hasSetupWizardPrompted) {
    hasSetupWizardPrompted = true;
    openWizardModal();
  }

  currentBits = d.bits;
  updateDotsUI();

  updateWifiSignalUI(d.wifiConnected, d.rssi);

  document.getElementById('lcd-line0').innerText = d.lcd0;
  document.getElementById('lcd-line1').innerText = d.lcd1;
  document.getElementById('char-preview').innerText = d.preview || '~';
  
  renderFullText(d.fullText || "", d.cursor || 0, d.brailleArray || [], d.selStart || 0, d.selEnd || 0);

  if (d.bootId !== undefined) {
    if (currentBootId === null) {
      currentBootId = d.bootId;
    } else if (currentBootId !== d.bootId) {
      location.reload();
      return;
    }
  }
    
  if(d.wifiStatus) document.getElementById('wifi-status-label').innerText = d.wifiStatus;

  if (d.feedbackMode !== undefined) {
    currentFeedbackMode = d.feedbackMode;
  }

  if (d.ttsMode !== undefined) {
    currentTtsMode = d.ttsMode;
  }

  if (d.brightness !== undefined && document.activeElement !== document.getElementById('brightness-slider')) {
    currentBrightness = d.brightness;
    document.getElementById('brightness-slider').value = d.brightness;
    document.getElementById('brightness-val-label').innerText = d.brightness + '%';
  }

  if (d.volume !== undefined) {
    if (currentFeedbackMode === 0 && d.volume > 0) {
      currentVolume = d.volume;
    }
    if (document.activeElement !== document.getElementById('volume-slider')) {
      const displayVol = (currentFeedbackMode === 1) ? 0 : d.volume;
      document.getElementById('volume-slider').value = displayVol;
      document.getElementById('volume-val-label').innerText = (displayVol > 0) ? displayVol + '%' : 'MUTED (0%)';
    }
  }

  currentFullText = d.fullText || "";
  currentCursorPos = d.cursor || 0;

  checkFlashcardAnswer(d);
}

let isUpdatingOta = false;
function resetOtaBtn(msg) {
  isUpdatingOta = false;
  const btn = document.getElementById('ota-btn');
  const statusEl = document.getElementById('ota-status');
  if (btn) {
    btn.disabled = false;
    btn.innerText = '⚡ Check & Update Firmware (GitHub OTA)';
  }
  if (statusEl && msg) {
    statusEl.innerText = msg;
  }
}

async function triggerOtaUpdate() {
  if (isUpdatingOta) return;
  const btn = document.getElementById('ota-btn');
  const statusEl = document.getElementById('ota-status');

  if (!confirm('Start firmware update directly from GitHub?')) return;

  isUpdatingOta = true;
  btn.disabled = true;
  btn.innerText = '⚡ Updating...';
  statusEl.innerText = 'Triggering OTA process...';

  try {
    const res = await fetch('/api/ota_update');
    const data = await res.json();
    statusEl.innerText = data.status || 'OTA update initiated on ESP32!';
  } catch (err) {
    resetOtaBtn('❌ Connection lost or failed to initiate OTA update.');
  }
}

let isPolling = false;
async function pollState() {
  if (isPolling) return;
  isPolling = true;

  try {
    const res = await fetch('/api/state');
    if (res.ok) {
      const d = await res.json();
      updateState(d);
    }
  } catch (e) {
  } finally {
    isPolling = false;
    const interval = document.hidden ? 1000 : 300;
    setTimeout(pollState, interval);
  }
}

nextFlashcardTarget();
pollState();
</script>
</body>
</html>
)rawliteral";
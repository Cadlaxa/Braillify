const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>BRAILLIFY WEB CONTROLLER</title>
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
      --green: #4caf50;
      --red: #f44336;
      --lcd-bg: #e8f5e9;
      --lcd-text: #1b4332;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: system-ui, -apple-system, sans-serif; touch-action: manipulation; }
    html, body {
      height: 100%;
      overflow: hidden;
      touch-action: manipulation;
      -webkit-text-size-adjust: 100%;
      overscroll-behavior: none;
    }
    body {
      background: var(--bg);
      color: var(--text);
      display: flex;
      justify-content: center;
      align-items: center;
      height: 100vh;
      height: 100dvh;
      overflow: hidden;
      padding: 15px;
    }
    button, .key-btn, .deck-pill, .slot-pill, .action-btn, .modal-trigger-btn, .wifi-signal-badge { touch-action: manipulation; }
    
    body.modal-open { overflow: hidden !important; height: 100dvh; touch-action: none; }

    .container {
      width: 100%;
      height: 96vh;
      height: 96dvh;
      max-height: 96dvh;
      background: var(--panel);
      border: 3px solid var(--border);
      box-shadow: 6px 6px 0px var(--border);
      border-radius: 12px;
      padding: 20px;
      position: relative;
      display: flex;
      flex-direction: column;
      overflow-y: auto;
      overflow-x: hidden;
      -webkit-overflow-scrolling: touch;
      overscroll-behavior: contain;
    }

    /* Prevent flex children from squishing when .container scrolls */
    .container > * {
      flex-shrink: 0;
    }
    
    .header { display: flex; justify-content: space-between; align-items: center; border-bottom: 2px dashed var(--border); padding-bottom: 12px; margin-bottom: 16px; }
    .header-title h1 { font-size: 1.5rem; letter-spacing: 1px; color: var(--text); font-weight: 900; font-family: 'Courier New', monospace; }
    .header-title .sub { font-size: 0.7rem; color: var(--orange-dark); font-weight: bold; }
    .header-btns { display: flex; gap: 6px; align-items: center; }

    .modal-trigger-btn { background: #e8f5e9; border: 2px solid var(--border); color: #2e7d32; font-size: 0.75rem; font-weight: bold; padding: 6px 6px; border-radius: 8px; cursor: pointer; display: flex; align-items: center; gap: 4px; box-shadow: 2px 2px 0px var(--border); transition: all .05s; }
    .modal-trigger-btn:active { transform: translate(1px, 1px); box-shadow: 0px 0px 0px var(--border); }

    .wifi-signal-badge { display: inline-flex; align-items: center; gap: 4px; background: #ffffff; border: 1.5px solid var(--border); padding: 6px 6px; border-radius: 6px; font-size: 0.75rem; font-weight: 800; color: var(--blue); cursor: pointer; box-shadow: 2px 2px 0px var(--border); transition: all .05s; }
    .wifi-signal-badge:active { transform: translate(1px, 1px); box-shadow: 0px 0px 0px var(--border); }
    .signal-icon-symbol { font-size: 0.95rem; line-height: 1; vertical-align: middle; }

    /* Modals */
    .modal-overlay { position: fixed; top: 0; left: 0; width: 100vw; height: 100vh; background: rgba(17,24,39,0.5); backdrop-filter: blur(3px); display: none; justify-content: center; align-items: center; z-index: 999; padding: 15px; overflow-y: auto; -webkit-overflow-scrolling: touch; }
    .modal-overlay.active { display: flex; }
    .modal-content { width: 100%; max-width: 440px; background: #ffffff; border: 3px solid var(--border); border-radius: 12px; box-shadow: 8px 8px 0px var(--border); padding: 20px; position: relative; animation: popIn 0.2s cubic-bezier(0.175, 0.885, 0.32, 1.275); max-height: 85vh; overflow-y: auto; }
    @keyframes popIn { from { transform: scale(0.7); opacity: 0; } to { transform: scale(1); opacity: 1; } }

    .modal-header { display: flex; justify-content: space-between; align-items: center; border-bottom: 2px solid var(--border); padding-bottom: 10px; margin-bottom: 12px; }
    .modal-title { font-size: 1.1rem; font-weight: 900; color: var(--blue); display: flex; align-items: center; gap: 6px; }
    .modal-close-btn { background: none; border: none; font-size: 1.5rem; font-weight: 900; color: var(--text); cursor: pointer; line-height: 1; padding: 0 4px; }

    /* Settings Tab Bar Navigation */
    .settings-tab-nav { display: flex; gap: 4px; border-bottom: 2px solid var(--border); margin-bottom: 16px; padding-bottom: 8px; overflow-x: auto; -webkit-overflow-scrolling: touch; }
    .settings-tab-btn { flex: 1 1 0px; min-width: 0; background: var(--bg); border: 2px solid var(--border); border-radius: 8px; padding: 6px 2px; font-size: 0.65rem; font-weight: 800; color: var(--text); cursor: pointer; display: flex; flex-direction: column; align-items: center; justify-content: center; gap: 2px; box-shadow: 2px 2px 0px var(--border); transition: all 0.05s; white-space: normal; text-align: center; word-break: break-word; line-height: 1.1; }
    .settings-tab-btn:active { transform: translate(1px, 1px); box-shadow: 0px 0px 0px var(--border); }
    .settings-tab-btn .tab-icon { font-size: 1rem; line-height: 1; }
    .settings-tab-btn.active { background: var(--blue-light); color: var(--blue); border-color: var(--blue); box-shadow: none; transform: translate(1px, 1px); }

    .settings-tab-pane { display: none; }
    .settings-tab-pane.active { display: block; animation: fadeIn 0.15s ease-in-out; }
    @keyframes fadeIn { from { opacity: 0; transform: translateY(2px); } to { opacity: 1; transform: translateY(0); } }

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

    .wifi-list { max-height: 140px; overflow-y: auto; border: 2px solid var(--border); border-radius: 8px; margin-bottom: 12px; background: var(--bg); }
    .wifi-item { padding: 8px 12px; border-bottom: 1px solid #d0d7de; display: flex; justify-content: space-between; align-items: center; cursor: pointer; font-weight: 700; font-size: 0.85rem; }
    .wifi-item:last-child { border-bottom: none; }
    .wifi-item:hover { background: var(--blue-light); }
    .wifi-rssi { font-size: 0.75rem; color: #57606a; font-family: 'Courier New', monospace; }
    .wifi-input-group { display: flex; flex-direction: column; gap: 8px; margin-bottom: 14px; }
    .wifi-input { width: 100%; padding: 8px 10px; border: 2px solid var(--border); border-radius: 6px; font-size: 16px; font-weight: 600; }
    .wifi-status-info { font-size: 0.75rem; font-weight: bold; color: var(--orange-dark); margin-bottom: 10px; text-align: center; }

    .setting-group { margin-bottom: 16px; }
    .setting-header { display: flex; justify-content: space-between; font-size: 0.85rem; font-weight: 800; margin-bottom: 6px; color: var(--text); }
    .setting-slider { width: 100%; accent-color: var(--orange-dark); cursor: pointer; height: 8px; }
    .setting-select { width: 100%; padding: 8px 10px; border: 2px solid var(--border); border-radius: 6px; font-size: 16px; font-weight: bold; background: #fff; }

    .camera-box { width: 100%; height: 210px; background: #000; border: 2px solid var(--border); border-radius: 8px; margin-bottom: 12px; overflow: hidden; position: relative; display: flex; justify-content: center; align-items: center; }
    .camera-video { width: 100%; height: 100%; object-fit: cover; }
    .ocr-status { font-size: 0.8rem; font-weight: bold; color: var(--blue); text-align: center; margin-bottom: 10px; min-height: 1.2em; }
    .section-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 6px; }
    .section-title { font-size: 0.8rem; color: var(--orange-dark); font-weight: bold; text-transform: uppercase; }
    .action-btn-group { display: flex; gap: 6px; }
    .action-btn { background: #fff3e0; border: 2px solid var(--border); color: var(--orange-dark); font-size: 0.75rem; font-weight: bold; padding: 4px 10px; border-radius: 6px; cursor: pointer; display: flex; align-items: center; gap: 4px; box-shadow: 1px 2px 0px var(--border); transition: all .05s; }
    .action-btn:active { transform: translate(1px, 1px); box-shadow: 0px 0px 0px var(--border); }
    .action-btn svg { width: 14px; height: 14px; fill: currentColor; }

    /* RESIZABLE VIEWER SHELL + NATIVE SCROLL + CARET HANDLES */
    .container > * {
      flex-shrink: 0;
    }

    .full-text-section {
      flex: 1 0 auto;
      margin-bottom: 10px;
      display: flex;
      flex-direction: column;
    }

    /* RESIZABLE VIEWER SHELL + NATIVE SCROLL + CARET HANDLES */
    .viewer-shell {
      flex: 1 0 auto;
      position: relative;
      border: 2px solid var(--border);
      border-radius: 8px;
      background: #ffffff;
      overflow: hidden;
      display: flex;
      flex-direction: column;
      box-shadow: inset 0 0 4px rgba(0,0,0,0.06);
    }

    .full-text-box {
      flex: 1 1 auto;
      position: relative;
      background: #ffffff;
      padding: 5px 8px;
      min-height: 60px;
      overflow-y: auto;
      overflow-x: hidden;
      -webkit-overflow-scrolling: touch;
      touch-action: pan-y;
      display: flex;
      flex-wrap: wrap;
      align-content: flex-start;
      gap: 2px;
      user-select: none;
      -webkit-user-select: none;
    }

    /* Slide-to-Resize Handle Bar */
    .viewer-resizer {
      flex-shrink: 0;
      height: 20px;
      background: var(--bg);
      border-top: 2px solid var(--border);
      display: flex;
      align-items: center;
      justify-content: center;
      cursor: ns-resize;
      touch-action: none;
      user-select: none;
    }
    .viewer-resizer:active { background: var(--blue-light); }
    .viewer-resizer-pill { width: 38px; height: 4px; border-radius: 999px; background: var(--border); opacity: 0.55; }
    .char-pair { display: inline-flex; flex-direction: column; align-items: center; justify-content: center; cursor: pointer; padding: 2px 4px; border-radius: 4px; user-select: none; min-width: 18px; transition: background 0.1s; position: relative; }
    .char-pair:hover { background: var(--blue-light); }
    .char-pair.selected { background: #bbdefb !important; color: #0d47a1 !important; border: 1px solid var(--blue); border-radius: 3px; }
    .char-pair.cursor-here { background: var(--orange); color: #ffffff; font-weight: bold; animation: blink 1s infinite; }
    @keyframes blink { 0%, 100% { opacity: 1; } 50% { opacity: 0.4; } }
    .char-pair .top-char { font-size: 1.1rem; font-weight: 900; font-family: 'Courier New', monospace; line-height: 1; pointer-events: none; }
    .char-pair .sub-braille { font-size: 0.85rem; line-height: 1; margin-top: 2px; opacity: 0.85; font-family: sans-serif; pointer-events: none; }

    /* Draggable Teardrop Selection Caret Handles */
    .sel-handle { position: absolute; width: 20px; height: 20px; background: var(--blue); border: 2px solid var(--border); box-shadow: 0 2px 6px rgba(0,0,0,0.28); z-index: 25; display: none; touch-action: none; cursor: grab; transition: background 0.1s; }
    .sel-handle:active, .sel-handle.dragging { cursor: grabbing; background: var(--orange) !important; }
    .sel-handle.start-handle { border-radius: 50% 0 50% 50%; transform: translate(-80%, -1px) rotate(45deg); }
    .sel-handle.end-handle { border-radius: 0 50% 50% 50%; transform: translate(-20%, -1px) rotate(-45deg); }

    /* Slide-to-Resize Handle Bar */
    .viewer-resizer { height: 20px; background: var(--bg); border-top: 2px solid var(--border); display: flex; align-items: center; justify-content: center; cursor: ns-resize; touch-action: none; user-select: none; }
    .viewer-resizer:active { background: var(--blue-light); }
    .viewer-resizer-pill { width: 38px; height: 4px; border-radius: 999px; background: var(--border); opacity: 0.55; }

    .selection-toolbar { display: none; margin-top: 8px; background: #e3f2fd; border: 2px solid var(--border); padding: 6px 10px; border-radius: 8px; justify-content: space-between; align-items: center; font-size: 0.75rem; font-weight: bold; }
    .selection-toolbar.active { display: flex; }

    .lcd-frame { background: var(--lcd-bg); border: 3px solid var(--border); border-radius: 8px; padding: 12px 14px; margin-bottom: 10px; margin-top: 10px; box-shadow: inset 0 0 5px rgba(0,0,0,0.08); position: relative; }
    .lcd-line { font-size: 1.2rem; color: var(--lcd-text); letter-spacing: 2px; white-space: pre; font-weight: bold; font-family: 'Courier New', monospace; min-height: 1.4em; }

    .status-card { display: flex; align-items: center; justify-content: space-between; background: var(--blue-light); border: 2px solid var(--border); padding: 14px 14px; border-radius: 8px; margin-bottom: 10px; }
    .cell-preview-box { display: flex; align-items: center; gap: 16px; }
    .braille-pattern-cell { display: grid; grid-template-columns: repeat(2, 16px); grid-gap: 8px; background: #ffffff; border: 2px solid var(--border); padding: 8px; border-radius: 6px; }
    .v-dot { width: 16px; height: 16px; border-radius: 50%; background: #d0d7de; border: 1.5px solid var(--border); transition: all .15s; }
    .v-dot.active { background: var(--green); box-shadow: 0 0 6px rgba(76,175,80,0.6); }

    .char-output-box { display: flex; flex-direction: column; align-items: flex-start; }
    .char-label { font-size: 0.7rem; font-weight: bold; color: var(--text); text-transform: uppercase; letter-spacing: 1px; }
    .char-val { font-size: 1.8rem; font-weight: 900; color: var(--orange-dark); font-family: 'Courier New', monospace; }
    .bit-badge { display: flex; flex-direction: column; align-items: flex-end; font-size: 0.75rem; font-weight: bold; font-family: 'Courier New', monospace; }
    .bit-badge span { color: var(--blue); }

    /* SLIDABLE DECK NAVIGATION & VIEWPORT */
    .deck-nav-bar { display: flex; align-items: center; gap: 4px; margin-bottom: 10px; }
    .deck-arrow-btn { background: #ffffff; border: 2px solid var(--border); border-radius: 8px; padding: 6px 8px; font-weight: 900; font-size: 0.8rem; cursor: pointer; box-shadow: 1px 2px 0px var(--border); }
    .deck-arrow-btn:active { transform: translate(1px, 1px); box-shadow: none; }
    .deck-pills { display: flex; flex: 1; gap: 4px; }
    .deck-pill { flex: 1; background: var(--bg); border: 2px solid var(--border); border-radius: 8px; padding: 6px 2px; font-size: 0.7rem; font-weight: 800; cursor: pointer; text-align: center; transition: all 0.1s; white-space: nowrap; }
    .deck-pill.active { background: var(--orange); color: #ffffff; border-color: var(--border); box-shadow: 1px 2px 0px var(--border); }

    .keypad-carousel { display: flex; overflow-x: auto; scroll-snap-type: x mandatory; scroll-behavior: smooth; scrollbar-width: none; -ms-overflow-style: none; border-radius: 8px; touch-action: pan-x; }
    .keypad-carousel::-webkit-scrollbar { display: none; }
    .keypad-page { flex: 0 0 100%; width: 100%; scroll-snap-align: start; scroll-snap-stop: always; padding: 2px; }

    .keypad-grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 8px; }
    .key-btn { background: #ffffff; border: 2px solid var(--border); color: var(--text); height: 58px; display: flex; flex-direction: column; align-items: center; justify-content: center; border-radius: 8px; cursor: pointer; box-shadow: 2px 3px 0px var(--border); transition: all .05s; user-select: none; -webkit-touch-callout: none; padding: 2px; text-align: center; }
    .key-btn:active { transform: translate(2px, 2px); box-shadow: 0px 0px 0px var(--border); background: var(--blue-light); }
    .key-btn.cmd { background: #fff3e0; border-color: var(--border); color: var(--orange-dark); }
    .key-btn.cmd:active { background: var(--orange); color: #ffffff; }
    .key-btn.blue-btn { background: var(--blue-light); color: var(--blue); }
    .key-btn.green-btn { background: #e8f5e9; color: #2e7d32; }
    .key-btn.yellow-btn { background: #fffde0; color: #928800; }
    .key-btn.red-btn { background: #ffebee; color: #c62828; }
    .key-btn svg { width: 20px; height: 20px; fill: currentColor; }
    .key-label { font-size: 0.63rem; font-weight: 800; margin-top: 2px; font-family: 'Courier New', monospace; line-height: 1.05; }
    .dot-key-num { font-size: 1.1rem; font-weight: 900; font-family: 'Courier New', monospace; }
    .dot-key-sub { font-size: 0.6rem; color: #57606a; font-weight: 700; }

    /* Deck Sub-Controls */
    .deck-inline-bar { display: flex; gap: 6px; margin-top: 8px; }
    .deck-inline-input { flex: 1; padding: 8px 10px; border: 2px solid var(--border); border-radius: 8px; font-size: 16px; font-weight: 700; }
    .deck-inline-send { background: #e8f5e9; border: 2px solid var(--border); color: #2e7d32; font-weight: 800; font-size: 0.78rem; padding: 0 12px; border-radius: 8px; cursor: pointer; box-shadow: 2px 2px 0px var(--border); }
    .deck-inline-send:active { transform: translate(1px, 1px); box-shadow: none; }

    .slot-selector-row { display: flex; gap: 6px; margin-bottom: 8px; }
    .slot-pill { flex: 1; padding: 6px; border: 2px solid var(--border); border-radius: 8px; background: var(--bg); font-weight: 800; font-size: 0.75rem; cursor: pointer; text-align: center; }
    .slot-pill.active { background: var(--blue); color: #ffffff; }

    .footer { text-align: center; font-size: .7rem; font-weight: bold; color: #57606a; margin-top: 14px; border-top: 2px solid var(--border); padding-top: 8px; }

    .ota-section { text-align: center; }
    .btn-update { width: 100%; background: #e8f5e9; border: 2px solid var(--border); color: #2e7d32; font-size: 0.85rem; font-weight: 800; padding: 10px; border-radius: 8px; cursor: pointer; box-shadow: 2px 2px 0px var(--border); transition: all .05s; display: flex; align-items: center; justify-content: center; gap: 6px; }
    .btn-update:active { transform: translate(1px, 1px); box-shadow: 0px 0px 0px var(--border); }
    .btn-update:disabled { opacity: 0.6; cursor: not-allowed; background: #d0d7de; color: #57606a; }
    #ota-status { font-size: 0.75rem; font-weight: bold; color: var(--orange-dark); margin-top: 6px; min-height: 1.2em; }

    /* ---------- LANDSCAPE SPLIT-PANE LAYOUT (TABLET / DESKTOP / PHONE) ---------- */
    @media (orientation: landscape) and (min-width: 560px) {
      body {
        padding: 8px 12px;
        height: 100vh;
        height: 100dvh;
        align-items: center;
      }

      .container {
        width: 100%;
        height: 90vh;
        height: 90dvh;
        max-height: 90dvh;
        padding: 12px 16px;
        display: grid;
        grid-template-columns: 1.7fr 1fr;
        grid-template-rows: auto auto auto minmax(0, 1fr) auto;
        column-gap: 14px;
        row-gap: 6px;
        align-items: stretch;
        overflow: hidden;
      }

      .header {
        grid-column: 1 / -1;
        grid-row: 1;
        padding-bottom: 6px;
        margin-bottom: 0;
      }

      .full-text-section {
        grid-column: 1;
        grid-row: 2 / 5;
        margin-bottom: 0;
        min-height: 0;
        overflow: hidden;
        display: flex;
        flex-direction: column;
      }

      .lcd-frame {
        flex-shrink: 0;
        margin-top: 2px;
        margin-bottom: 6px;
        padding: 6px 10px;
      }

      .lcd-line {
        font-size: 1rem;
      }

      .viewer-shell {
        flex: 1 1 0%;
        min-height: 0;
        display: flex;
        flex-direction: column;
        overflow: hidden;
      }

      .full-text-box {
        flex: 1 1 0%;
        height: auto !important;
        min-height: 0;
        max-height: none;
        overflow-y: auto;
      }

      .viewer-resizer {
        display: none;
      }

      .status-card {
        grid-column: 2;
        grid-row: 2;
        margin-bottom: 0;
        padding: 6px 10px;
      }

      .braille-pattern-cell {
        padding: 5px;
        grid-gap: 5px;
        grid-template-columns: repeat(2, 12px);
      }

      .v-dot {
        width: 12px;
        height: 12px;
      }

      .char-val {
        font-size: 1.4rem;
      }

      .deck-nav-bar {
        grid-column: 2;
        grid-row: 3;
        margin-bottom: 0;
      }

      .keypad-carousel {
        grid-column: 2;
        grid-row: 4;
        min-height: 0;
        align-self: start;
      }

      .keypad-page {
        display: flex;
        flex-direction: column;
      }

      .keypad-grid {
        gap: 6px;
      }

      .key-btn {
        height: clamp(20px, 10dvh, 80px);
        min-height: 20px;
        max-height: 120px;
      }

      .footer {
        grid-column: 1 / -1;
        grid-row: 5;
        margin-top: 2px;
        padding-top: 4px;
      }
    }

    /* ---------- COMPACT MOBILE PHONE LANDSCAPE (SHORT VIEWPORT) ---------- */
    @media (orientation: landscape) and (max-height: 480px) {
      body {
        padding: 4px 8px;
      }

      .container {
        width: 100%;
        max-width: 98%;
        height: 100vh;
        height: 100dvh;
        max-height: 100dvh;
        border-radius: 8px;
        padding: 8px 12px;
        row-gap: 4px;
        column-gap: 10px;
      }

      .header {
        padding-bottom: 4px;
      }

      .header-title h1 {
        font-size: 1.1rem;
      }

      .header-title .sub,
      .dot-key-sub,
      .footer {
        display: none;
      }

      .section-header {
        margin-bottom: 3px;
      }

      .lcd-frame {
        padding: 4px 8px;
        margin-bottom: 4px;
      }

      .lcd-line {
        font-size: 0.9rem;
        line-height: 1.25;
      }

      .status-card {
        padding: 4px 8px;
      }

      .char-val {
        font-size: 1.2rem;
      }

      .deck-pill,
      .deck-arrow-btn {
        padding: 4px 2px;
        font-size: 0.65rem;
      }

      .keypad-carousel {
        align-self: stretch;
        height: 100%;
        min-height: 0;
      }

      .keypad-page {
        height: 100%;
        min-height: 0;
        display: flex;
        flex-direction: column;
      }

      .keypad-grid {
        flex: 1;
        min-height: 0;
        grid-template-rows: repeat(4, minmax(0, 1fr));
        gap: 4px;
      }

      .key-btn {
        height: 100%;
        min-height: 0;
        max-height: 48px;
        padding: 1px;
        border-radius: 6px;
        box-shadow: 1px 2px 0px var(--border);
      }

      .key-btn svg {
        width: 14px;
        height: 14px;
      }

      .dot-key-num {
        font-size: 0.9rem;
        line-height: 1;
      }

      .key-label {
        font-size: 0.54rem;
        margin-top: 1px;
      }

      .modal-content {
        max-height: 92dvh;
        padding: 12px;
      }

      .camera-box {
        height: 130px;
      }
    }
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
      <div class="wifi-signal-badge" id="wifi-signal-badge" title="Click to Open Wi-Fi Settings" onclick="openSettingsModal('wifi')">
        <span class="signal-icon-symbol" id="sig-symbol">📡</span>
        <span id="sig-dbm-label">--</span>
      </div>
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

    <div class="lcd-frame">
      <div class="lcd-line" id="lcd-line0">Mode: AUTO     </div>
      <div class="lcd-line" id="lcd-line1">_               </div>
    </div>
    
    <div class="viewer-shell" id="viewer-shell">
      <div class="full-text-box" id="full-text-box"></div>
      <div class="sel-handle start-handle" id="sel-handle-start" data-handle="start"></div>
      <div class="sel-handle end-handle" id="sel-handle-end" data-handle="end"></div>
      <div class="viewer-resizer" id="viewer-resizer" title="Slide up or down to resize viewer">
        <span class="viewer-resizer-pill"></span>
      </div>
    </div>

    <div class="selection-toolbar" id="selection-toolbar">
      <span><span id="sel-count-label">0</span> chars selected</span>
      <div style="display:flex; gap:6px;">
        <button class="action-btn" onclick="speakSelection()">🗣️ Speak</button>
        <button class="action-btn" id="sel-copy-btn" onclick="copySelection()">📋 Copy</button>
        <button class="action-btn" style="background:#ffebee; color:#c62828;" onclick="deleteSelectionWeb()">🗑️ Del</button>
      </div>
    </div>
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
      <div style="margin-top:2px;color:var(--orange-dark);" id="con-mode-badge">UEB G2</div>
    </div>
  </div>

  <!-- SLIDABLE KEYPAD DECK NAVIGATION -->
  <div class="deck-nav-bar">
    <button class="deck-arrow-btn" onclick="stepDeck(-1)" title="Previous Deck">&lt;</button>
    <div class="deck-pills">
      <button class="deck-pill active" id="deck-pill-0" onclick="goToDeck(0)">⌨️ Keypad</button>
      <button class="deck-pill" id="deck-pill-1" onclick="goToDeck(1)">✂️ Edit</button>
      <button class="deck-pill" id="deck-pill-2" onclick="goToDeck(2)">⠿ UEB</button>
      <button class="deck-pill" id="deck-pill-3" onclick="goToDeck(3)">💾 Files</button>
    </div>
    <button class="deck-arrow-btn" onclick="stepDeck(1)" title="Next Deck">&gt;</button>
  </div>

  <!-- HORIZONTAL SWIPE CAROUSEL -->
  <div class="keypad-carousel" id="keypad-carousel">
    
    <!-- PAGE 1: PRIMARY BRAILLE KEYPAD + UNDO -->
    <div class="keypad-page" data-page="0" data-name="Braille Keypad">
      <div class="keypad-grid">
        <button class="key-btn blue-btn" data-key="1" title="Undo Last Action">
          <svg viewBox="0 0 24 24"><path d="M12.5 8c-2.65 0-5.05.99-6.9 2.6L2 7v9h9l-3.62-3.62c1.39-1.16 3.16-1.88 5.12-1.88 3.54 0 6.55 2.31 7.6 5.5l2.37-.78C21.08 11.03 17.15 8 12.5 8z"/></svg>
          <span class="key-label">UNDO</span>
        </button>
        <button class="key-btn cmd" data-key="2"><span class="dot-key-num">2</span><span class="dot-key-sub">• Dot 1</span></button>
        <button class="key-btn cmd" data-key="3"><span class="dot-key-num">3</span><span class="dot-key-sub">• Dot 4</span></button>
        <button class="key-btn yellow-btn" data-key="A" title="Save Memory">
          <svg viewBox="0 0 24 24"><path d="M17 3H5c-1.11 0-2 0.9-2 2v14c0 1.1 0.89 2 2 2h14c1.1 0 2-0.9 2-2V7l-4-4zm-5 16c-1.66 0-3-1.34-3-3s1.34-3 3-3 3 1.34 3 3-1.34 3-3 3zm3-10H5V5h10v4z"/></svg>
          <span class="key-label">SAVE</span>
        </button>

        <button class="key-btn blue-btn" data-key="4" title="Mute Toggle">
          <svg viewBox="0 0 24 24"><path d="M3 9v6h4l5 5V4L7 9H3zm13.5 3c0-1.77-1.02-3.29-2.5-4.03v8.05c1.48-.73 2.5-2.25 2.5-4.02zM14 3.23v2.06c2.89 0.86 5 3.54 5 6.71s-2.11 5.85-5 6.71v2.06c4.01-0.91 7-4.49 7-8.77s-2.99-7.86-7-8.77z"/></svg>
          <span class="key-label">MUTE</span>
        </button>
        <button class="key-btn cmd" data-key="5"><span class="dot-key-num">5</span><span class="dot-key-sub">• Dot 2</span></button>
        <button class="key-btn cmd" data-key="6"><span class="dot-key-num">6</span><span class="dot-key-sub">• Dot 5</span></button>
        <button class="key-btn yellow-btn" data-key="B" title="Load Memory">
          <svg viewBox="0 0 24 24"><path d="M20 6h-8l-2-2H4c-1.1 0-1.99 0.9-1.99 2L2 18c0 1.1 0.9 2 2 2h16c1.1 0 2-0.9 2-2V8c0-1.1-0.9-2-2-2zm0 12H4V8h16v10z"/></svg>
          <span class="key-label">LOAD</span>
        </button>

        <button class="key-btn cmd" data-key="7" title="Cursor Left">
          <svg viewBox="0 0 24 24"><path d="M20 11H7.83l5.59-5.59L12 4l-8 8 8 8 1.41-1.41L7.83 13H20v-2z"/></svg>
          <span class="key-label">LEFT</span>
        </button>
        <button class="key-btn cmd" data-key="8"><span class="dot-key-num">8</span><span class="dot-key-sub">• Dot 3</span></button>
        <button class="key-btn cmd" data-key="9"><span class="dot-key-num">9</span><span class="dot-key-sub">• Dot 6</span></button>
        <button class="key-btn cmd" data-key="C" title="Cursor Right">
          <svg viewBox="0 0 24 24"><path d="M12 4l-1.41 1.41L16.17 11H4v2h12.17l-5.58 5.59L12 20l8-8z"/></svg>
          <span class="key-label">RIGHT</span>
        </button>

        <button class="key-btn green-btn" data-key="*" title="Cycle Mode">
          <svg viewBox="0 0 24 24"><path d="M12 4V1L8 5l4 4V6c3.31 0 6 2.69 6 6 0 1.01-0.25 1.97-0.7 2.8l1.46 1.46C19.54 15.03 20 13.57 20 12c0-4.42-3.58-8-8-8zm0 14c-3.31 0-6-2.69-6-6 0-1.01 0.25-1.97 0.7-2.8L5.24 7.74C4.46 8.97 4 10.43 4 12c0 4.42 3.58 8 8 8v3l4-4-4-4v3z"/></svg>
          <span class="key-label">MODE</span>
        </button>
        <button class="key-btn green-btn" data-key="0" title="Space">
          <svg viewBox="0 0 24 24"><path d="M18 9v4H6V9H4v6h16V9h-2z"/></svg>
          <span class="key-label">SPACE</span>
        </button>
        <button class="key-btn green-btn" data-key="#" title="Enter / Confirm Cell">
          <svg viewBox="0 0 24 24"><path d="M19 7v4H5.83l3.58-3.59L8 6l-6 6 6 6 1.41-1.41L5.83 13H21V7h-2z"/></svg>
          <span class="key-label">ENTER</span>
        </button>
        <button class="key-btn green-btn" data-key="D" title="Backspace / Delete">
          <svg viewBox="0 0 24 24"><path d="M22 3H7c-0.69 0-1.23 0.35-1.59 0.88L0 12l5.41 8.11c0.36 0.53 0.9 0.89 1.59 0.89h15c1.1 0 2-0.9 2-2V5c0-1.1-0.9-2-2-2zm0 16H7.07L2.4 12l4.66-7H22v14zm-11.59-2L14 13.41 17.59 17 19 15.59 15.41 12 19 8.41 17.59 7 14 10.59 10.41 7 9 8.41 12.59 12 9 15.59z"/></svg>
          <span class="key-label">DEL</span>
        </button>
      </div>
    </div>

    <!-- PAGE 2: EDITING & SELECTION DECK -->
    <div class="keypad-page" data-page="1" data-name="Editing Deck">
      <div class="keypad-grid">
        <button class="key-btn blue-btn" onclick="setCursorPos(0)">
          <span class="dot-key-num">|&lt;</span>
          <span class="key-label">HOME</span>
        </button>
        <button class="key-btn blue-btn" onclick="sendCmd('word_left')">
          <span class="dot-key-num">&lt;&lt;</span>
          <span class="key-label">WORD</span>
        </button>
        <button class="key-btn blue-btn" onclick="sendCmd('word_right')">
          <span class="dot-key-num">&gt;&gt;</span>
          <span class="key-label">WORD</span>
        </button>
        <button class="key-btn blue-btn" onclick="setCursorPos(currentFullText.length)">
          <span class="dot-key-num">&gt;|</span>
          <span class="key-label">END</span>
        </button>

        <button class="key-btn cmd" onclick="sendCmd('select_word')">
          <span class="dot-key-num">[W]</span>
          <span class="key-label">SEL WORD</span>
        </button>
        <button class="key-btn cmd" onclick="sendCmd('select_all')">
          <span class="dot-key-num">[A]</span>
          <span class="key-label">SEL ALL</span>
        </button>
        <button class="key-btn green-btn" onclick="pasteFromClipboard()">
          <span class="dot-key-num">📋</span>
          <span class="key-label">PASTE</span>
        </button>
        <button class="key-btn red-btn" onclick="confirmClearAll()">
          <span class="dot-key-num">🧹</span>
          <span class="key-label">CLEAR ALL</span>
        </button>
      </div>

      <div class="deck-inline-bar">
        <input type="text" id="qwerty-push-input" class="deck-inline-input" placeholder="Type text to insert onto LCD..." onkeydown="if(event.key==='Enter') pushQwertyText()">
        <button class="deck-inline-send" onclick="pushQwertyText()">Insert</button>
      </div>
    </div>

    <!-- PAGE 3: UEB INDICATORS & CONTRACTIONS DECK -->
    <div class="keypad-page" data-page="2" data-name="UEB Contractions Deck">
      <div class="keypad-grid">
        <button class="key-btn cmd" onclick="sendCmd('prefix_cap')">
          <span class="dot-key-num">⠠</span>
          <span class="key-label">CAP (D6)</span>
        </button>
        <button class="key-btn cmd" onclick="sendCmd('prefix_capslock')">
          <span class="dot-key-num">⠠⠠</span>
          <span class="key-label">CAPS LOCK</span>
        </button>
        <button class="key-btn blue-btn" onclick="sendCmd('prefix_num')">
          <span class="dot-key-num">⠼</span>
          <span class="key-label">NUM SIGN</span>
        </button>
        <button class="key-btn blue-btn" onclick="sendCmd('prefix_spec')">
          <span class="dot-key-num">⠸</span>
          <span class="key-label">SPEC SIGN</span>
        </button>

        <button class="key-btn green-btn" onclick="cycleContractionLang()">
          <span class="dot-key-num" id="con-lang-icon">G2</span>
          <span class="key-label" id="con-lang-btn">UEB / FIL</span>
        </button>
        <button class="key-btn" onclick="insertShortform('about')">
          <span class="dot-key-num">ab</span>
          <span class="key-label">about</span>
        </button>
        <button class="key-btn" onclick="insertShortform('because')">
          <span class="dot-key-num">bbc</span>
          <span class="key-label">because</span>
        </button>
        <button class="key-btn" onclick="insertShortform('children')">
          <span class="dot-key-num">chn</span>
          <span class="key-label">children</span>
        </button>

        <button class="key-btn" onclick="insertShortform('through')">
          <span class="dot-key-num">&gt;th</span>
          <span class="key-label">through</span>
        </button>
        <button class="key-btn" onclick="insertShortform('together')">
          <span class="dot-key-num">tgr</span>
          <span class="key-label">together</span>
        </button>
        <button class="key-btn" onclick="insertShortform('should')">
          <span class="dot-key-num">shd</span>
          <span class="key-label">should</span>
        </button>
        <button class="key-btn cmd" onclick="openUebCheatModal()">
          <span class="dot-key-num">📖</span>
          <span class="key-label">LIST</span>
        </button>
      </div>
    </div>

    <!-- PAGE 4: MULTI-SLOT FILES, EXPORT & TOOLS DECK -->
    <div class="keypad-page" data-page="3" data-name="Files and Tools Deck">
      <div class="slot-selector-row">
        <button class="slot-pill active" id="slot-pill-1" onclick="selectMemorySlot(1)">Slot 1</button>
        <button class="slot-pill" id="slot-pill-2" onclick="selectMemorySlot(2)">Slot 2</button>
        <button class="slot-pill" id="slot-pill-3" onclick="selectMemorySlot(3)">Slot 3</button>
        <button class="slot-pill" id="slot-pill-4" onclick="selectMemorySlot(4)">Slot 4</button>
      </div>

      <div class="keypad-grid">
        <button class="key-btn green-btn" onclick="slotAction('save')">
          <span class="dot-key-num">💾</span>
          <span class="key-label" id="slot-save-lbl">SAVE S1</span>
        </button>
        <button class="key-btn blue-btn" onclick="slotAction('load')">
          <span class="dot-key-num">📂</span>
          <span class="key-label" id="slot-load-lbl">LOAD S1</span>
        </button>
        <button class="key-btn cmd" onclick="downloadNoteFile('txt')">
          <span class="dot-key-num">.TXT</span>
          <span class="key-label">EXPORT</span>
        </button>
        <button class="key-btn cmd" onclick="downloadNoteFile('brf')">
          <span class="dot-key-num">.BRF</span>
          <span class="key-label">BRAILLE</span>
        </button>

        <button class="key-btn cmd" onclick="openCameraModal()">
          <span class="dot-key-num">📷</span>
          <span class="key-label">SCAN OCR</span>
        </button>
        <button class="key-btn blue-btn" onclick="repeatLastSpeech()">
          <span class="dot-key-num">🔁</span>
          <span class="key-label">REPEAT</span>
        </button>
        <button class="key-btn" onclick="openFlashcardModal()">
          <span class="dot-key-num">🎓</span>
          <span class="key-label">QUIZ</span>
        </button>
        <button class="key-btn" onclick="openSettingsModal('sound')">
          <span class="dot-key-num">⚙️</span>
          <span class="key-label">SETTINGS</span>
        </button>
      </div>
    </div>

  </div>

  <div class="footer">BRAILLIFY: http://braillify.local</div>
</div>

<!-- UEB / FILIPINO CONTRACTION CHEAT-SHEET MODAL -->
<div class="modal-overlay" id="ueb-cheat-modal" onclick="handleModalBackdropClick(event)">
  <div class="modal-content">
    <div class="modal-header">
      <div class="modal-title">📖 Contraction Quick-Insert</div>
      <button class="modal-close-btn" onclick="closeUebCheatModal()">&times;</button>
    </div>
    <input type="text" id="ueb-search-input" class="wifi-input" style="margin-bottom:10px;" placeholder="Search contraction (e.g. about, hindi, children)..." oninput="filterCheatList()">
    <div class="wifi-list" id="ueb-cheat-list" style="max-height:240px;"></div>
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

    <div class="wizard-step" id="wstep-4">
      <div class="modal-header">
        <div class="modal-title">🔐 Device Security PIN</div>
      </div>
      <p style="font-size:0.8rem; color:#57606a; margin-bottom:12px; font-weight:600;">Set a 4-digit PIN and recovery question to secure web interface access.</p>

      <div class="wifi-input-group">
        <input type="password" maxlength="4" id="w-pin-input" class="wifi-input" inputmode="numeric" pattern="[0-9]*" placeholder="Set 4-Digit PIN">
        <input type="text" id="w-sec-q-input" class="wifi-input" placeholder="Security Question (e.g. Pet Name?)">
        <input type="text" id="w-sec-a-input" class="wifi-input" placeholder="Security Answer">
      </div>

      <div class="wizard-nav">
        <button class="wizard-btn secondary" onclick="prevWizardStep(3)">← Back</button>
        <button class="wizard-btn primary" onclick="saveWizardSecurity()">Next: Wi-Fi →</button>
      </div>
    </div>

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

    <div class="wizard-step" id="wstep-6">
      <div class="modal-header">
        <div class="modal-title">✨ Keypad Quick Guide</div>
      </div>
      <div class="key-guide-grid">
        <div class="key-guide-item"><span class="key-guide-badge">#</span> Confirm Cell</div>
        <div class="key-guide-item"><span class="key-guide-badge">0</span> Space Character</div>
        <div class="key-guide-item"><span class="key-guide-badge">*</span> Switch Mode</div>
        <div class="key-guide-item"><span class="key-guide-badge">1</span> Undo Typing</div>
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
<div class="modal-overlay" id="pin-lock-modal">
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

<!-- TABBED DEVICE SETTINGS MODAL -->
<div class="modal-overlay" id="settings-modal" onclick="handleModalBackdropClick(event)">
  <div class="modal-content">
    <div class="modal-header">
      <div class="modal-title">⚙️ Device Settings</div>
      <button class="modal-close-btn" onclick="closeSettingsModal()">&times;</button>
    </div>

    <div class="settings-tab-nav">
      <button class="settings-tab-btn active" id="tab-btn-sound" onclick="switchSettingsTab('sound')">
        <span class="tab-icon">🔊</span>
        <span>Sound & Screen</span>
      </button>
      <button class="settings-tab-btn" id="tab-btn-tts" onclick="switchSettingsTab('tts')">
        <span class="tab-icon">🗣️</span>
        <span>TTS</span>
      </button>
      <button class="settings-tab-btn" id="tab-btn-wifi" onclick="switchSettingsTab('wifi')">
        <span class="tab-icon">📶</span>
        <span>Wi-Fi</span>
      </button>
      <button class="settings-tab-btn" id="tab-btn-updates" onclick="switchSettingsTab('updates')">
        <span class="tab-icon">⚡</span>
        <span>Updates</span>
      </button>
      <button class="settings-tab-btn" id="tab-btn-system" onclick="switchSettingsTab('system')">
        <span class="tab-icon">🔐</span>
        <span>System</span>
      </button>
    </div>

    <div class="settings-tab-pane active" id="tab-pane-sound">
      <div class="setting-group">
        <div class="setting-header">
          <span>☀️ LCD Backlight Brightness</span>
          <span id="brightness-val-label">100%</span>
        </div>
        <input type="range" id="brightness-slider" min="0" max="100" value="100" class="setting-slider" onchange="setBrightness(this.value)" oninput="document.getElementById('brightness-val-label').innerText=this.value+'%'">
      </div>

      <div class="setting-group" style="margin-bottom: 8px;">
        <div class="setting-header">
          <span>🔊 Buzzer Volume</span>
          <span id="volume-val-label">100%</span>
        </div>
        <input type="range" id="volume-slider" min="0" max="100" value="100" class="setting-slider" onchange="setVolume(this.value)" oninput="document.getElementById('volume-val-label').innerText = (this.value > 0 ? this.value + '%' : 'MUTED (0%)')">
      </div>
    </div>

    <div class="settings-tab-pane" id="tab-pane-tts">
      <div class="setting-group">
        <div class="setting-header">
          <span>🗣️ Speech Mode</span>
        </div>
        <select id="tts-mode-select" class="setting-select" onchange="setTtsMode(this.value)">
          <option value="1">Mode 1: Speak Char + Full Word on Space</option>
          <option value="2">Mode 2: Speak Full Word Only (after space)</option>
          <option value="3">Mode 3: Speak Full Buffer Line</option>
          <option value="0">Mode 0: Web Speech TTS Disabled</option>
        </select>
      </div>

      <div class="setting-group" style="margin-bottom: 8px;">
        <div class="setting-header">
          <span>⚡ Speech Rate / Speed</span>
          <span id="speech-rate-label">1.0x</span>
        </div>
        <input type="range" id="speech-rate-slider" min="0.5" max="2.0" step="0.1" value="1.0" class="setting-slider" oninput="document.getElementById('speech-rate-label').innerText=this.value+'x'">
      </div>
    </div>

    <div class="settings-tab-pane" id="tab-pane-wifi">
      <div class="wifi-status-info" id="wifi-status-label">AP Mode Active (192.168.4.1)</div>

      <div class="section-title" style="margin-bottom: 6px;">Saved Networks</div>
      <div class="wifi-list" id="saved-wifi-list" style="max-height: 120px; margin-bottom: 12px;">
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

    <div class="settings-tab-pane" id="tab-pane-updates">
      <div class="ota-section">
        <div class="setting-header" style="margin-bottom:8px;"><span>⚡ Firmware Update</span></div>
        <p style="font-size: 0.8rem; color: #57606a; margin-bottom: 14px; font-weight: 600;">Check GitHub releases for the latest firmware binary over Wi-Fi.</p>
        <button id="ota-btn" onclick="triggerOtaUpdate()" class="btn-update">
          ⚡ Check & Update Firmware (GitHub OTA)
        </button>
        <div id="ota-status"></div>
      </div>
    </div>

    <div class="settings-tab-pane" id="tab-pane-system">
      <div class="setting-group" style="margin-bottom: 16px;">
        <div class="setting-header" style="margin-bottom: 8px;"><span>🔐 Security & Authentication</span></div>
        <div class="wifi-input-group" style="margin-bottom: 8px;">
          <input type="password" maxlength="4" id="cfg-pin" class="wifi-input" inputmode="numeric" pattern="[0-9]*" placeholder="New 4-Digit PIN">
          <input type="text" id="cfg-sec-q" class="wifi-input" placeholder="Security Question">
          <input type="text" id="cfg-sec-a" class="wifi-input" placeholder="Security Answer">
        </div>
        <button onclick="updateSecurityConfigFromSettings()" class="btn-update" style="background: #e8f5e9; color: #2e7d32;">
          💾 Save Security Preferences
        </button>
      </div>

      <div class="setting-group" style="border-top: 2px dashed var(--border); padding-top: 14px; margin-top: 14px;">
        <button onclick="closeSettingsModal(); openWizardModal();" class="btn-update" style="background: var(--blue-light); color: var(--blue); margin-bottom: 12px;">
          🪄 Re-run Setup Wizard
        </button>

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
let currentContractionMode = 0;
let activeDeckPage = 0;
let selectedSlot = 1;
let lastUtteranceText = "";

let isSelecting = false;
let selectionAnchor = -1;

// Touch scroll vs hold-to-select & caret handles state
let touchHoldTimer = null;
let touchStartClientX = 0;
let touchStartClientY = 0;
let touchIsScrolling = false;
let touchLongPressFired = false;
let activeCaretHandle = null; // 'start' | 'end' | null

let lastSpokenText = "";
let lastSpokenCursor = 0;
let isFirstStateUpdate = true;
let hasSetupWizardPrompted = false;

let holdTimer = null;
let repeatInterval = null;

/* ---------- LOCK TOUCHSCREEN ZOOM ---------- */
document.addEventListener('gesturestart', function (e) {
  e.preventDefault();
}, { passive: false });

document.addEventListener('touchmove', function (e) {
  if (e.touches.length > 1) {
    e.preventDefault();
  }
}, { passive: false });

let lastTouchEnd = 0;
document.addEventListener('touchend', function (e) {
  const now = Date.now();
  if (now - lastTouchEnd <= 300) {
    e.preventDefault();
  }
  lastTouchEnd = now;
}, { passive: false });

/* ---------- SLIDABLE KEYPAD DECK LOGIC ---------- */
const deckNames = ["Braille Keypad", "Editing Deck", "UEB Contractions Deck", "Files and Tools Deck"];

function goToDeck(idx) {
  const carousel = document.getElementById('keypad-carousel');
  if (!carousel) return;
  const clamped = Math.max(0, Math.min(3, idx));
  carousel.scrollTo({ left: clamped * carousel.clientWidth, behavior: 'smooth' });
  setActiveDeckUI(clamped, true);
}

function stepDeck(delta) {
  goToDeck(activeDeckPage + delta);
}

function setActiveDeckUI(idx, announce = false) {
  if (activeDeckPage === idx) return;
  activeDeckPage = idx;
  for (let i = 0; i < 4; i++) {
    const pill = document.getElementById('deck-pill-' + i);
    if (pill) {
      if (i === idx) pill.classList.add('active');
      else pill.classList.remove('active');
    }
  }
  if (navigator.vibrate) {
    try { navigator.vibrate(20); } catch (e) {}
  }
  if (announce && currentTtsMode > 0) {
    speakTextUtterance(deckNames[idx]);
  }
}

(function initDeckScrollObserver() {
  const carousel = document.getElementById('keypad-carousel');
  if (!carousel) return;
  let scrollTimeout = null;
  carousel.addEventListener('scroll', () => {
    clearTimeout(scrollTimeout);
    scrollTimeout = setTimeout(() => {
      const pageIdx = Math.round(carousel.scrollLeft / Math.max(1, carousel.clientWidth));
      if (pageIdx !== activeDeckPage && pageIdx >= 0 && pageIdx <= 3) {
        setActiveDeckUI(pageIdx, true);
      }
    }, 90);
  }, { passive: true });
})();

/* ---------- DECK COMMANDS & SHORTFORMS ---------- */
function sendCmd(action, val = null) {
  let url = '/api/cmd?action=' + encodeURIComponent(action);
  if (val !== null) url += '&val=' + encodeURIComponent(val);
  fetch(url).then(r => r.json()).then(d => updateState(d));
}

function confirmClearAll() {
  if (!currentFullText) return;
  sendCmd('clear');
  speakTextUtterance('Buffer cleared');
}

async function pasteFromClipboard() {
  let text = "";
  try {
    if (navigator.clipboard && navigator.clipboard.readText) {
      text = await navigator.clipboard.readText();
    }
  } catch (e) {}
  if (!text) {
    text = prompt("Paste or enter text to insert:") || "";
  }
  if (text.trim().length > 0) {
    fetch('/api/insert?text=' + encodeURIComponent(text))
      .then(r => r.json())
      .then(d => updateState(d));
  }
}

function pushQwertyText() {
  const inp = document.getElementById('qwerty-push-input');
  if (!inp || !inp.value) return;
  const val = inp.value;
  inp.value = '';
  fetch('/api/insert?text=' + encodeURIComponent(val))
    .then(r => r.json())
    .then(d => updateState(d));
}

function cycleContractionLang() {
  const nextMode = (currentContractionMode + 1) % 3;
  sendCmd('set_contraction_mode', nextMode);
  const labels = ["UEB Grade 2 Enabled", "Filipino Braille Enabled", "Grade 1 Uncontracted"];
  speakTextUtterance(labels[nextMode]);
}

function updateContractionBadge(mode) {
  currentContractionMode = mode;
  const badge = document.getElementById('con-mode-badge');
  const icon = document.getElementById('con-lang-icon');
  const btn = document.getElementById('con-lang-btn');
  if (mode === 0) {
    if (badge) badge.innerText = "UEB G2";
    if (icon) icon.innerText = "UEB";
    if (btn) btn.innerText = "GRADE 2";
  } else if (mode === 1) {
    if (badge) badge.innerText = "FIL G2";
    if (icon) icon.innerText = "FIL";
    if (btn) btn.innerText = "FILIPINO";
  } else {
    if (badge) badge.innerText = "G1 OFF";
    if (icon) icon.innerText = "G1";
    if (btn) btn.innerText = "UNCONTRACTED";
  }
}

function insertShortform(word) {
  const needsLeadingSpace = (currentFullText.length > 0 && currentCursorPos > 0 && currentFullText[currentCursorPos - 1] !== ' ');
  const payload = (needsLeadingSpace ? " " : "") + word + " ";
  fetch('/api/insert?text=' + encodeURIComponent(payload))
    .then(r => r.json())
    .then(d => {
      updateState(d);
      speakTextUtterance(word);
    });
}

const cheatSheetItems = [
  { abbr: "ab", word: "about", lang: "UEB" }, { abbr: "abv", word: "above", lang: "UEB" },
  { abbr: "acc", word: "according", lang: "UEB" }, { abbr: "acr", word: "across", lang: "UEB" },
  { abbr: "adm", word: "administration", lang: "UEB" }, { abbr: "af", word: "after", lang: "UEB" },
  { abbr: "afn", word: "afternoon", lang: "UEB" }, { abbr: "afw", word: "afterward", lang: "UEB" },
  { abbr: "ag", word: "again", lang: "UEB" }, { abbr: "agst", word: "against", lang: "UEB" },
  { abbr: "alm", word: "almost", lang: "UEB" }, { abbr: "alr", word: "already", lang: "UEB" },
  { abbr: "alth", word: "although", lang: "UEB" }, { abbr: "altg", word: "altogether", lang: "UEB" },
  { abbr: "alw", word: "always", lang: "UEB" }, { abbr: "z", word: "as", lang: "UEB" },

  { abbr: "bb", word: "be", lang: "UEB" }, { abbr: "bbc", word: "because", lang: "UEB" },
  { abbr: "bbf", word: "before", lang: "UEB" }, { abbr: "bbh", word: "behind", lang: "UEB" },
  { abbr: "bbl", word: "below", lang: "UEB" }, { abbr: "bbn", word: "beneath", lang: "UEB" },
  { abbr: "bbs", word: "beside", lang: "UEB" }, { abbr: "bbt", word: "between", lang: "UEB" },
  { abbr: "bby", word: "beyond", lang: "UEB" }, { abbr: "bl", word: "blind", lang: "UEB" },
  { abbr: "brl", word: "braille", lang: "UEB" }, { abbr: "b", word: "but", lang: "UEB" },

  { abbr: "c", word: "can", lang: "UEB" }, { abbr: ">ch", word: "character", lang: "UEB" },
  { abbr: "ch", word: "child", lang: "UEB" }, { abbr: "chn", word: "children", lang: "UEB" },
  { abbr: "cccv", word: "conceive", lang: "UEB" }, { abbr: "cccvg", word: "conceiving", lang: "UEB" },
  { abbr: "cd", word: "could", lang: "UEB" },

  { abbr: ">d", word: "day", lang: "UEB" }, { abbr: "dcv", word: "deceive", lang: "UEB" },
  { abbr: "dcvg", word: "deceiving", lang: "UEB" }, { abbr: "dcl", word: "declare", lang: "UEB" },
  { abbr: "dclg", word: "declaring", lang: "UEB" }, { abbr: "d", word: "do", lang: "UEB" },

  { abbr: "ei", word: "either", lang: "UEB" }, { abbr: "en", word: "enough", lang: "UEB" },
  { abbr: ">e", word: "ever", lang: "UEB" },

  { abbr: ">f", word: "father", lang: "UEB" }, { abbr: "fst", word: "first", lang: "UEB" },
  { abbr: "fr", word: "friend", lang: "UEB" }, { abbr: "f", word: "from", lang: "UEB" },

  { abbr: "g", word: "go", lang: "UEB" }, { abbr: "gd", word: "good", lang: "UEB" },
  { abbr: "grt", word: "great", lang: "UEB" },

  { abbr: "h", word: "have", lang: "UEB" }, { abbr: ">h", word: "here", lang: "UEB" },
  { abbr: "herf", word: "herself", lang: "UEB" }, { abbr: "hmf", word: "himself", lang: "UEB" },
  { abbr: "in", word: "his", lang: "UEB" },

  { abbr: "imm", word: "immediate", lang: "UEB" }, { abbr: "x", word: "it", lang: "UEB" },
  { abbr: "xs", word: "its", lang: "UEB" }, { abbr: "xf", word: "itself", lang: "UEB" },

  { abbr: "j", word: "just", lang: "UEB" }, { abbr: ">k", word: "know", lang: "UEB" },
  { abbr: "k", word: "knowledge", lang: "UEB" },

  { abbr: "lr", word: "letter", lang: "UEB" }, { abbr: "l", word: "like", lang: "UEB" },
  { abbr: "ll", word: "little", lang: "UEB" }, { abbr: ">l", word: "lord", lang: "UEB" },

  { abbr: "m", word: "more", lang: "UEB" }, { abbr: ">m", word: "mother", lang: "UEB" },
  { abbr: "mch", word: "much", lang: "UEB" }, { abbr: "mst", word: "must", lang: "UEB" },
  { abbr: "myf", word: "myself", lang: "UEB" },

  { abbr: ">n", word: "name", lang: "UEB" }, { abbr: "nec", word: "necessary", lang: "UEB" },
  { abbr: "nei", word: "neither", lang: "UEB" }, { abbr: "n", word: "not", lang: "UEB" },

  { abbr: ">o", word: "one", lang: "UEB" }, { abbr: ">of", word: "oneself", lang: "UEB" },
  { abbr: ">ou", word: "ought", lang: "UEB" }, { abbr: "ourvs", word: "ourselves", lang: "UEB" },
  { abbr: "ou", word: "out", lang: "UEB" },

  { abbr: "-d", word: "ound", lang: "UEB" }, { abbr: "-t", word: "ount", lang: "UEB" },
  { abbr: "-e", word: "ance", lang: "UEB" }, { abbr: "-n", word: "sion", lang: "UEB" },
  { abbr: "-s", word: "less", lang: "UEB" }, { abbr: ">e", word: "ence", lang: "UEB" },
  { abbr: ">g", word: "ong", lang: "UEB" }, { abbr: ">l", word: "ful", lang: "UEB" },
  { abbr: ">n", word: "tion", lang: "UEB" }, { abbr: ">t", word: "ment", lang: "UEB" },
  { abbr: ">s", word: "ness", lang: "UEB" }, { abbr: ">y", word: "ity", lang: "UEB" },

  { abbr: "pd", word: "paid", lang: "UEB" }, { abbr: "p", word: "people", lang: "UEB" },
  { abbr: "percv", word: "perceive", lang: "UEB" }, { abbr: "percvg", word: "perceiving", lang: "UEB" },
  { abbr: "perh", word: "perhaps", lang: "UEB" },

  { abbr: ">q", word: "question", lang: "UEB" }, { abbr: "qk", word: "quick", lang: "UEB" },
  { abbr: "q", word: "quite", lang: "UEB" },

  { abbr: "r", word: "rather", lang: "UEB" }, { abbr: "rcv", word: "receive", lang: "UEB" },
  { abbr: "rcvg", word: "receiving", lang: "UEB" }, { abbr: "rjc", word: "rejoice", lang: "UEB" },
  { abbr: "rjcg", word: "rejoicing", lang: "UEB" }, { abbr: ">r", word: "right", lang: "UEB" },

  { abbr: "sd", word: "said", lang: "UEB" }, { abbr: "sh", word: "shall", lang: "UEB" },
  { abbr: "s", word: "so", lang: "UEB" }, { abbr: ">s", word: "some", lang: "UEB" },
  { abbr: "st", word: "still", lang: "UEB" }, { abbr: "sch", word: "such", lang: "UEB" },

  { abbr: "t", word: "that", lang: "UEB" }, { abbr: "themvs", word: "themselves", lang: "UEB" },
  { abbr: ">the", word: "there", lang: "UEB" }, { abbr: "th", word: "this", lang: "UEB" },
  { abbr: "-th", word: "those", lang: "UEB" }, { abbr: ">th", word: "through", lang: "UEB" },
  { abbr: "thyf", word: "thyself", lang: "UEB" }, { abbr: ">t", word: "time", lang: "UEB" },
  { abbr: "td", word: "today", lang: "UEB" }, { abbr: "tgr", word: "together", lang: "UEB" },
  { abbr: "tm", word: "tomorrow", lang: "UEB" }, { abbr: "tn", word: "tonight", lang: "UEB" },

  { abbr: ">u", word: "under", lang: "UEB" }, { abbr: "u", word: "us", lang: "UEB" },
  { abbr: "v", word: "very", lang: "UEB" },

  { abbr: "by", word: "was", lang: "UEB" }, { abbr: "gg", word: "were", lang: "UEB" },
  { abbr: ">wh", word: "where", lang: "UEB" }, { abbr: "wh", word: "which", lang: "UEB" },
  { abbr: "w", word: "will", lang: "UEB" }, { abbr: ">w", word: "work", lang: "UEB" },
  { abbr: "wd", word: "would", lang: "UEB" },

  { abbr: "y", word: "you", lang: "UEB" }, { abbr: ">y", word: "young", lang: "UEB" },
  { abbr: "yr", word: "your", lang: "UEB" }, { abbr: "yrf", word: "yourself", lang: "UEB" },
  { abbr: "yrvs", word: "yourselves", lang: "UEB" },

  { abbr: "ag", word: "ang", lang: "FIL" }, { abbr: "mg", word: "mga", lang: "FIL" },
  { abbr: "ng", word: "ng", lang: "FIL" }, { abbr: "s", word: "sa", lang: "FIL" },
  { abbr: "ky", word: "kay", lang: "FIL" }, { abbr: "at", word: "at", lang: "FIL" },
  { abbr: "na", word: "na", lang: "FIL" }, { abbr: "pa", word: "pa", lang: "FIL" },
  { abbr: "ba", word: "ba", lang: "FIL" }, { abbr: "ko", word: "ko", lang: "FIL" },
  { abbr: "mo", word: "mo", lang: "FIL" }, { abbr: "ni", word: "ni", lang: "FIL" },
  { abbr: "si", word: "si", lang: "FIL" }, { abbr: "ix", word: "ito", lang: "FIL" },
  { abbr: "iy", word: "iyon", lang: "FIL" }, { abbr: "ny", word: "niya", lang: "FIL" },
  { abbr: "sl", word: "sila", lang: "FIL" }, { abbr: "km", word: "kami", lang: "FIL" },
  { abbr: "ty", word: "tayo", lang: "FIL" }, { abbr: "ky", word: "kayo", lang: "FIL" },
  { abbr: "hd", word: "hindi", lang: "FIL" }, { abbr: "oo", word: "oo", lang: "FIL" },
  { abbr: "bk", word: "bakit", lang: "FIL" }, { abbr: "pn", word: "paano", lang: "FIL" },
  { abbr: "sn", word: "saan", lang: "FIL" }, { abbr: "kl", word: "kailan", lang: "FIL" },
  { abbr: "sn", word: "sino", lang: "FIL" }, { abbr: "an", word: "ano", lang: "FIL" }
];

function openUebCheatModal() {
  document.getElementById('ueb-cheat-modal').classList.add('active');
  updateBodyScrollLock();
  filterCheatList();
}

function closeUebCheatModal() {
  document.getElementById('ueb-cheat-modal').classList.remove('active');
  updateBodyScrollLock();
}

function filterCheatList() {
  const q = (document.getElementById('ueb-search-input').value || '').toLowerCase();
  const list = document.getElementById('ueb-cheat-list');
  list.innerHTML = '';
  cheatSheetItems
    .filter(item => item.word.includes(q) || item.abbr.includes(q) || item.lang.toLowerCase().includes(q))
    .forEach(item => {
      const div = document.createElement('div');
      div.className = 'wifi-item';
      div.innerHTML = `<span><b>${item.word}</b> <small style="color:#57606a;">(${item.abbr})</small></span><span class="wifi-rssi">${item.lang}</span>`;
      div.onclick = () => {
        insertShortform(item.word);
        closeUebCheatModal();
      };
      list.appendChild(div);
    });
}

/* ---------- MULTI-SLOT STORAGE & EXPORT ---------- */
function selectMemorySlot(slotNum) {
  selectedSlot = slotNum;
  for (let i = 1; i <= 4; i++) {
    const p = document.getElementById('slot-pill-' + i);
    if (p) {
      if (i === slotNum) p.classList.add('active');
      else p.classList.remove('active');
    }
  }
  document.getElementById('slot-save-lbl').innerText = 'SAVE S' + slotNum;
  document.getElementById('slot-load-lbl').innerText = 'LOAD S' + slotNum;
}

function slotAction(op) {
  fetch(`/api/slot?op=${op}&slot=${selectedSlot}`)
    .then(r => r.json())
    .then(d => updateState(d));
}

function downloadNoteFile(ext) {
  if (!currentFullText || currentFullText.trim().length === 0) {
    alert('Buffer is empty. Type or load text before exporting.');
    return;
  }
  let content = currentFullText;
  if (ext === 'brf') {
    content = content.toUpperCase();
  }
  const blob = new Blob([content], { type: 'text/plain;charset=utf-8' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = `braillify_slot${selectedSlot}.${ext}`;
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
  URL.revokeObjectURL(url);
}

function repeatLastSpeech() {
  const toSpeak = lastUtteranceText || currentFullText;
  if (toSpeak) speakTextUtterance(toSpeak, true);
}

/* ---------- FLASHCARD DATASET ---------- */
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

function syncPinLockState(hasPin) {
  const isAuthed = sessionStorage.getItem('braillify_authed') === 'true';
  const lockModal = document.getElementById('pin-lock-modal');

  if (!hasPin || isAuthed) {
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
      document.getElementById('forgot-sec-question').innerText = d.question || 'No security question set.';
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
    .catch(() => alert('❌ Factory Reset failed: Incorrect PIN.'));
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

function loadSavedWifis() {
  const listEl = document.getElementById('saved-wifi-list');
  if (!listEl) return;

  fetch('/api/wifi/saved')
    .then(async res => {
      if (!res.ok) throw new Error('HTTP Status ' + res.status);
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
          <div style="flex:1; cursor:pointer;" onclick="connectSavedWifi('${net.ssid}', '${net.pass}')">
            <span style="font-weight:800;">${net.ssid}</span>
          </div>
          <div style="display:flex; gap:4px;" onclick="event.stopPropagation()">
            <button class="action-btn" style="padding:2px 6px; font-size:0.7rem;" onclick="editSavedWifi('${net.ssid}', '${net.pass}')">✏️ Edit</button>
            <button class="action-btn" style="padding:2px 6px; font-size:0.7rem; background:#ffebee; color:#c62828;" onclick="forgetWifi('${net.ssid}')">🗑️ Forget</button>
          </div>
        `;
        listEl.appendChild(item);
      });
    })
    .catch(() => {
      listEl.innerHTML = '<div class="wifi-item" style="justify-content:center;color:red;">Failed to load saved networks</div>';
    });
}

function connectSavedWifi(ssid, pass) {
  document.getElementById('wifi-ssid').value = ssid;
  document.getElementById('wifi-pass').value = pass;
  connectWifi();
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

let currentFeedbackMode = 0;

function switchSettingsTab(tabId) {
  document.querySelectorAll('.settings-tab-btn').forEach(btn => btn.classList.remove('active'));
  document.querySelectorAll('.settings-tab-pane').forEach(pane => pane.classList.remove('active'));

  const targetBtn = document.getElementById('tab-btn-' + tabId);
  const targetPane = document.getElementById('tab-pane-' + tabId);

  if (targetBtn) targetBtn.classList.add('active');
  if (targetPane) targetPane.classList.add('active');

  if (tabId === 'wifi') {
    loadSavedWifis();
    scanWifiNetworks();
  }
}

function openSettingsModal(defaultTab = 'sound'){ 
  document.getElementById('brightness-slider').value = currentBrightness;
  document.getElementById('brightness-val-label').innerText = currentBrightness + '%';
  
  const activeVol = (currentFeedbackMode === 1) ? 0 : currentVolume;
  document.getElementById('volume-slider').value = activeVol;
  document.getElementById('volume-val-label').innerText = (activeVol > 0) ? activeVol + '%' : 'MUTED (0%)';
  
  document.getElementById('tts-mode-select').value = currentTtsMode;
  
  switchSettingsTab(defaultTab);
  
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
      .catch(() => {
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
  if(e.target.id === 'camera-modal') closeCameraModal();
  if(e.target.id === 'settings-modal') closeSettingsModal();
  if(e.target.id === 'ueb-cheat-modal') closeUebCheatModal();
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

/* ---------- SELECTION CARET HANDLES & VIEWER RESIZER ---------- */
const textContainer = document.getElementById('full-text-box');
const viewerShell = document.getElementById('viewer-shell');
const startCaretHandle = document.getElementById('sel-handle-start');
const endCaretHandle = document.getElementById('sel-handle-end');
const viewerResizer = document.getElementById('viewer-resizer');

// Restore saved viewer height if previously resized
(function restoreViewerHeight() {
  try {
    const savedH = localStorage.getItem('braillify_viewer_height');
    if (savedH && textContainer) {
      textContainer.style.height = savedH + 'px';
    }
  } catch (e) {}
})();

function positionSelectionHandles(start, end) {
  if (!startCaretHandle || !endCaretHandle || !textContainer || !viewerShell) return;
  if (start === end || start >= end) {
    startCaretHandle.style.display = 'none';
    endCaretHandle.style.display = 'none';
    return;
  }

  const firstPair = textContainer.querySelector(`.char-pair[data-idx="${start}"]`);
  const lastPair = textContainer.querySelector(`.char-pair[data-idx="${end - 1}"]`);
  if (!firstPair || !lastPair) {
    startCaretHandle.style.display = 'none';
    endCaretHandle.style.display = 'none';
    return;
  }

  const shellRect = viewerShell.getBoundingClientRect();
  const boxRect = textContainer.getBoundingClientRect();
  const r1 = firstPair.getBoundingClientRect();
  const r2 = lastPair.getBoundingClientRect();

  // Hide handles if scrolled out of the visible text box viewport
  const startVisible = r1.bottom >= boxRect.top && r1.top <= boxRect.bottom;
  const endVisible = r2.bottom >= boxRect.top && r2.top <= boxRect.bottom;

  if (startVisible) {
    startCaretHandle.style.display = 'block';
    startCaretHandle.style.left = (r1.left - shellRect.left) + 'px';
    startCaretHandle.style.top = (r1.bottom - shellRect.top) + 'px';
  } else {
    startCaretHandle.style.display = 'none';
  }

  if (endVisible) {
    endCaretHandle.style.display = 'block';
    endCaretHandle.style.left = (r2.right - shellRect.left) + 'px';
    endCaretHandle.style.top = (r2.bottom - shellRect.top) + 'px';
  } else {
    endCaretHandle.style.display = 'none';
  }
}

textContainer.addEventListener('scroll', () => {
  positionSelectionHandles(currentSelStart, currentSelEnd);
}, { passive: true });

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

  positionSelectionHandles(start, end);
}

function renderFullText(text, pos, brailleArr, selStart, selEnd){
  currentFullText = text;
  currentCursorPos = pos;
  
  if (isSelecting || activeCaretHandle) {
    return;
  }

  currentSelStart = selStart;
  currentSelEnd = selEnd;

  const container = document.getElementById('full-text-box');
  const prevScrollTop = container.scrollTop;
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

  container.scrollTop = prevScrollTop;
  positionSelectionHandles(selStart, selEnd);
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

function finishSelection() {
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

function getCharIdxFromPoint(clientX, clientY) {
  const target = document.elementFromPoint(clientX, clientY);
  if (!target) return null;
  const pair = target.closest('.char-pair');
  if (!pair) return null;
  const idx = parseInt(pair.getAttribute('data-idx'));
  return isNaN(idx) ? null : idx;
}

function selectWordAroundIndex(idx) {
  const text = currentFullText || "";
  if (text.length === 0) {
    setCursorPos(0);
    return;
  }
  let s = Math.min(idx, text.length - 1);
  if (s < 0) s = 0;
  while (s > 0 && text[s - 1] !== ' ') s--;
  let e = s;
  while (e < text.length && text[e] !== ' ') e++;
  if (s === e && e < text.length) e++;

  currentSelStart = s;
  currentSelEnd = e;
  currentCursorPos = e;
  updateVisualSelection(s, e);
  fetch(`/api/setcursor?start=${s}&end=${e}`).then(r => r.json()).then(d => updateState(d));
}

// Touchscreen: Swipe = native scroll, Quick Tap = move cursor, Long-Press (400ms) = select word + show caret handles
textContainer.addEventListener('touchstart', (e) => {
  if (e.touches.length > 1 || activeCaretHandle) return;
  const t = e.touches[0];
  touchStartClientX = t.clientX;
  touchStartClientY = t.clientY;
  touchIsScrolling = false;
  touchLongPressFired = false;

  clearTimeout(touchHoldTimer);
  touchHoldTimer = setTimeout(() => {
    if (!touchIsScrolling) {
      touchLongPressFired = true;
      if (navigator.vibrate) {
        try { navigator.vibrate(25); } catch (err) {}
      }
      const idx = getCharIdxFromPoint(touchStartClientX, touchStartClientY);
      if (idx !== null) {
        selectWordAroundIndex(idx);
      }
    }
  }, 400);
}, { passive: true });

textContainer.addEventListener('touchmove', (e) => {
  if (activeCaretHandle) return;
  const t = e.touches[0];
  if (Math.abs(t.clientX - touchStartClientX) > 8 || Math.abs(t.clientY - touchStartClientY) > 8) {
    touchIsScrolling = true;
    clearTimeout(touchHoldTimer); // Cancel hold timer so browser scrolls smoothly
  }
}, { passive: true });

textContainer.addEventListener('touchend', (e) => {
  clearTimeout(touchHoldTimer);
  if (touchIsScrolling || touchLongPressFired || activeCaretHandle) return;

  const t = e.changedTouches[0];
  const idx = getCharIdxFromPoint(t.clientX, t.clientY);
  if (idx !== null) {
    setCursorPos(idx);
  }
}, { passive: true });

// Draggable Start/End Caret Handles (Touch + Mouse)
function bindCaretHandleEvents(handleEl, side) {
  if (!handleEl) return;

  const onStart = (e) => {
    e.stopPropagation();
    if (e.cancelable) e.preventDefault();
    activeCaretHandle = side;
    handleEl.classList.add('dragging');
  };

  const onMove = (clientX, clientY) => {
    if (activeCaretHandle !== side) return;
    const idx = getCharIdxFromPoint(clientX, clientY - 18);
    if (idx === null) return;

    let s = currentSelStart;
    let end = currentSelEnd;
    if (side === 'start') {
      s = Math.max(0, Math.min(idx, end - 1));
    } else {
      end = Math.min(currentFullText.length, Math.max(idx + 1, s + 1));
    }
    currentCursorPos = end;
    updateVisualSelection(s, end);
  };

  const onEnd = () => {
    if (activeCaretHandle !== side) return;
    activeCaretHandle = null;
    handleEl.classList.remove('dragging');
    fetch(`/api/setcursor?start=${currentSelStart}&end=${currentSelEnd}`)
      .then(r => r.json())
      .then(d => updateState(d));
  };

  handleEl.addEventListener('touchstart', onStart, { passive: false });
  handleEl.addEventListener('touchmove', (e) => {
    if (activeCaretHandle !== side) return;
    if (e.cancelable) e.preventDefault();
    const t = e.touches[0];
    onMove(t.clientX, t.clientY);
  }, { passive: false });
  handleEl.addEventListener('touchend', onEnd);
  handleEl.addEventListener('touchcancel', onEnd);

  handleEl.addEventListener('mousedown', onStart);
  window.addEventListener('mousemove', (e) => {
    if (activeCaretHandle === side) onMove(e.clientX, e.clientY);
  });
  window.addEventListener('mouseup', onEnd);
}

bindCaretHandleEvents(startCaretHandle, 'start');
bindCaretHandleEvents(endCaretHandle, 'end');

// Adjustable Viewer Slide Resizer (Touch + Mouse)
let isResizingViewer = false;
let viewerResizeStartY = 0;
let viewerResizeStartH = 0;

function beginViewerResize(clientY) {
  isResizingViewer = true;
  viewerResizeStartY = clientY;
  viewerResizeStartH = textContainer.offsetHeight;
}

function updateViewerResize(clientY) {
  if (!isResizingViewer) return;
  const maxH = Math.round(window.innerHeight * 0.55);
  const newH = Math.max(68, Math.min(maxH, viewerResizeStartH + (clientY - viewerResizeStartY)));
  textContainer.style.height = newH + 'px';
  positionSelectionHandles(currentSelStart, currentSelEnd);
}

function endViewerResize() {
  if (!isResizingViewer) return;
  isResizingViewer = false;
  try {
    localStorage.setItem('braillify_viewer_height', textContainer.offsetHeight);
  } catch (e) {}
}

if (viewerResizer) {
  viewerResizer.addEventListener('touchstart', (e) => {
    if (e.cancelable) e.preventDefault();
    beginViewerResize(e.touches[0].clientY);
  }, { passive: false });

  viewerResizer.addEventListener('mousedown', (e) => {
    e.preventDefault();
    beginViewerResize(e.clientY);
  });
}

window.addEventListener('touchmove', (e) => {
  if (isResizingViewer) {
    if (e.cancelable) e.preventDefault();
    updateViewerResize(e.touches[0].clientY);
  }
}, { passive: false });

window.addEventListener('touchend', endViewerResize);
window.addEventListener('mousemove', (e) => {
  if (isResizingViewer) updateViewerResize(e.clientY);
});
window.addEventListener('mouseup', endViewerResize);

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
  } catch (err) {}
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
  } catch (e) {}
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
  lastUtteranceText = str;
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

      window.speechSynthesis.speak(currentUtterance);
    } catch (err) {}
  } else if (force) {
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

  syncPinLockState(Boolean(d.hasPin));

  if (d.setupDone === false && !hasSetupWizardPrompted) {
    hasSetupWizardPrompted = true;
    openWizardModal();
  }

  if (d.contractionMode !== undefined) {
    updateContractionBadge(d.contractionMode);
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

import time
from pathlib import Path

base = Path(__file__).parent
html_path = base / "index.html"
header_path = base / "web_pages.h"

last_mtime = 0
print(f"Watching {html_path.name} -> {header_path.name} ...")

while True:
    try:
        mtime = html_path.stat().st_mtime
        if mtime != last_mtime:
            last_mtime = mtime
            html = html_path.read_text(encoding="utf-8")
            header = (
                'const char index_html[] PROGMEM = R"rawliteral(\n'
                + html
                + '\n)rawliteral";\n'
            )
            header_path.write_text(header, encoding="utf-8")
            print(f"[{time.strftime('%H:%M:%S')}] Updated {header_path.name} ({len(html)} bytes)")
    except Exception as e:
        print("Error:", e)
    time.sleep(0.4)
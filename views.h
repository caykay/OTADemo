#pragma once

// =============================================================================
//  web_views.h  —  ESP32 OTA Demo  |  HTML views + shared CSS
//
//  USAGE in your .ino:
//    #include "web_views.h"
//
//    server.on("/",       HTTP_GET, [](AsyncWebServerRequest *r){
//      r->send(200, "text/html", pageHome());
//    });
//    server.on("/update", HTTP_GET, [](AsyncWebServerRequest *r){
//      r->send(200, "text/html", pageUpdate());
//    });
//
//  HOW THE CSS ECOSYSTEM WORKS:
//    SHARED_CSS         ──►  one place for all styling; edit to restyle every
//    page buildPage(t, body) ──►  runtime helper; injects SHARED_CSS into a
//    full HTML doc _BODY_*            ──►  per-page body markup (PROGMEM raw
//    string literals) page*()            ──►  thin wrappers that call
//    buildPage(); use in .ino handlers
//
//  Adding a new page:
//    1. Write body markup as a PROGMEM raw string:
//         static const char _BODY_FOO[] PROGMEM = R"html( ... )html";
//    2. Add a wrapper:
//         inline String pageFoo() { return buildPage("Foo", _BODY_FOO); }
//    3. Register the route in your .ino.
// =============================================================================

// ---------------------------------------------------------------------------
//  SHARED CSS  —  edit this to restyle every page at once
// ---------------------------------------------------------------------------
static const char SHARED_CSS[] PROGMEM =
    R"css(
  *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

  :root {
    --bg:        #0d1117;
    --surface:   #161b22;
    --border:    #30363d;
    --accent:    #58a6ff;
    --accent2:   #3fb950;
    --text:      #e6edf3;
    --muted:     #8b949e;
    --danger:    #f85149;
    --radius:    8px;
    --font-mono: 'Courier New', monospace;
    --font-sans: 'Segoe UI', system-ui, sans-serif;
  }

  body {
    background: var(--bg);
    color: var(--text);
    font-family: var(--font-sans);
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    align-items: center;
    padding: 2rem 1rem;
  }

  /* ── Layout ── */
  .card {
    background: var(--surface);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    padding: 2rem;
    width: 100%;
    max-width: 520px;
    margin-bottom: 1.5rem;
  }

  /* ── Header bar ── */
  .topbar {
    width: 100%;
    max-width: 520px;
    display: flex;
    align-items: center;
    justify-content: space-between;
    margin-bottom: 2rem;
    padding-bottom: 1rem;
    border-bottom: 1px solid var(--border);
  }
  .topbar .logo {
    font-family: var(--font-mono);
    font-size: 0.8rem;
    color: var(--accent);
    letter-spacing: 0.1em;
    text-transform: uppercase;
  }
  .topbar .chip {
    font-family: var(--font-mono);
    font-size: 0.7rem;
    color: var(--accent2);
    background: rgba(63,185,80,0.1);
    border: 1px solid rgba(63,185,80,0.3);
    padding: 2px 8px;
    border-radius: 20px;
  }

  /* ── Typography ── */
  h1 { font-size: 1.5rem; margin-bottom: 0.4rem; }
  h2 { font-size: 1.1rem; margin-bottom: 1rem; color: var(--muted); font-weight: 400; }
  p  { line-height: 1.6; color: var(--muted); font-size: 0.9rem; margin-bottom: 0.8rem; }

  /* ── Status badge ── */
  .badge {
    display: inline-block;
    font-family: var(--font-mono);
    font-size: 0.7rem;
    padding: 2px 10px;
    border-radius: 20px;
    margin-bottom: 1.2rem;
  }
  .badge.online  { background: rgba(63,185,80,0.15); color: var(--accent2); border: 1px solid rgba(63,185,80,0.3); }
  .badge.warning { background: rgba(248,81,73,0.15);  color: var(--danger);  border: 1px solid rgba(248,81,73,0.3); }

  /* ── Stat grid ── */
  .stats {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 0.8rem;
    margin-top: 1.2rem;
  }
  .stat {
    background: var(--bg);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    padding: 0.8rem 1rem;
  }
  .stat .label { font-size: 0.7rem; color: var(--muted); text-transform: uppercase; letter-spacing: 0.08em; }
  .stat .value { font-family: var(--font-mono); font-size: 1rem; color: var(--accent); margin-top: 2px; }

  /* ── Buttons ── */
  .btn {
    display: inline-block;
    padding: 0.6rem 1.4rem;
    border-radius: var(--radius);
    font-size: 0.85rem;
    font-family: var(--font-sans);
    cursor: pointer;
    border: none;
    text-decoration: none;
    transition: opacity 0.15s;
  }
  .btn:hover   { opacity: 0.85; }
  .btn.primary { background: var(--accent);  color: #0d1117; font-weight: 600; }
  .btn.success { background: var(--accent2); color: #0d1117; font-weight: 600; }
  .btn.outline { background: transparent; color: var(--text); border: 1px solid var(--border); }

  /* ── Nav links ── */
  .nav { margin-top: 1rem; display: flex; gap: 1rem; }
  .nav a { font-size: 0.8rem; color: var(--accent); text-decoration: none; }
  .nav a:hover { text-decoration: underline; }

  /* ── Upload form ── */
  .upload-area {
    border: 2px dashed var(--border);
    border-radius: var(--radius);
    padding: 2rem;
    text-align: center;
    margin: 1rem 0;
    transition: border-color 0.2s;
  }
  .upload-area:hover { border-color: var(--accent); }
  input[type=file] { display: none; }

  /* ── Progress bar ── */
  .progress-wrap {
    background: var(--bg);
    border-radius: 4px;
    height: 6px;
    margin-top: 1rem;
    overflow: hidden;
    display: none;
  }
  .progress-bar {
    height: 100%;
    width: 0%;
    background: var(--accent2);
    border-radius: 4px;
    transition: width 0.3s ease;
  }

  /* ── Footer ── */
  .footer {
    font-family: var(--font-mono);
    font-size: 0.65rem;
    color: var(--muted);
    text-align: center;
    margin-top: auto;
    padding-top: 2rem;
    letter-spacing: 0.05em;
  }
)css";

// ---------------------------------------------------------------------------
//  buildPage(title, body)  —  runtime helper (call in your .ino handlers)
//  Concatenates SHARED_CSS with a body string into a full HTML document.
//
//  Example:
//    String html = buildPage("Home", "<h1>Hello</h1>");
//    request->send(200, "text/html", html);
// ---------------------------------------------------------------------------
inline String buildPage(const char *title, const char *body) {
  String html;
  html.reserve(4096);
  html +=
      F("<!DOCTYPE html><html lang='en'><head>"
        "<meta charset='UTF-8'>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>");
  html += title;
  html += F(" | ESP32 OTA</title><style>");
  html += SHARED_CSS; // inject shared styles
  html += F("</style></head><body>");
  html += body;
  html +=
      F("<p class='footer'>ESP32 OTA DEMO &nbsp;&middot;&nbsp; " __DATE__ "</p>"
        "</body></html>");
  return html;
}

// ===========================================================================
//  PAGE BODIES  —  only the <body> markup, no <html>/<head>/<style> needed
// ===========================================================================

// ---------------------------------------------------------------------------
//  PAGE_HOME  —  landing / status page
// ---------------------------------------------------------------------------
static const char _BODY_HOME[] PROGMEM = R"html(
  <div class='topbar'>
    <span class='logo'>&#9632; ESP32 OTA Demo</span>
    <span class='chip'>ONLINE</span>
  </div>

  <div class='card'>
    <span class='badge online'>&#9679; System Online</span>
    <h1>OTA Update Portal</h1>
    <h2>ESP32 Development Board</h2>
    <p>
      This device is running the OTA demo firmware. Use the update page to
      flash a new <code>.bin</code> over Wi-Fi without a USB cable.
    </p>

    <div class='stats'>
      <div class='stat'>
        <div class='label'>Board</div>
        <div class='value'>ESP32</div>
      </div>
      <div class='stat'>
        <div class='label'>Protocol</div>
        <div class='value'>HTTP OTA</div>
      </div>
      <div class='stat'>
        <div class='label'>Firmware</div>
        <div class='value'>v0.1.0</div>
      </div>
      <div class='stat'>
        <div class='label'>Status</div>
        <div class='value' style='color:var(--accent2)'>Ready</div>
      </div>
    </div>

    <div class='nav'>
      <a href='/update'>&#8599; Flash Firmware</a>
      <a href='/files'>&#8599; File System</a>
      <a href='/info'>&#8599; Device Info</a>
    </div>
  </div>
)html";

// ---------------------------------------------------------------------------
//  PAGE_UPDATE  —  firmware upload page
// ---------------------------------------------------------------------------
static const char _BODY_UPDATE[] PROGMEM = R"html(
  <div class='topbar'>
    <span class='logo'>&#9632; ESP32 OTA Demo</span>
    <span class='chip'>UPDATE</span>
  </div>

  <div class='card'>
    <h1>Flash Firmware</h1>
    <h2>Upload a compiled .bin file</h2>
    <p>Select your firmware binary and press <strong>Upload</strong>.
       Do not power off the device during flashing.</p>

    <label for='file-input'>
      <div class='upload-area' id='drop-area'>
        <p style='font-size:2rem;margin-bottom:0.5rem'>&#128190;</p>
        <p style='color:var(--text)'>Click to select <code>.bin</code></p>
        <p>or drag &amp; drop here</p>
      </div>
    </label>
    <input type='file' id='file-input' accept='.bin'>

    <div id='file-name' style='font-size:0.8rem;color:var(--accent);margin:0.5rem 0;display:none'></div>

    <button class='btn success' onclick='startUpload()' style='margin-top:0.5rem'>
      &#8593; Upload Firmware
    </button>
    &nbsp;
    <a href='/' class='btn outline'>Cancel</a>

    <div class='progress-wrap' id='progress-wrap'>
      <div class='progress-bar' id='progress-bar'></div>
    </div>
    <div id='status-msg' style='font-size:0.8rem;color:var(--muted);margin-top:0.8rem'></div>
  </div>

  <script>
    const fileInput = document.getElementById('file-input');
    const fileName  = document.getElementById('file-name');
    const progWrap  = document.getElementById('progress-wrap');
    const progBar   = document.getElementById('progress-bar');
    const statusMsg = document.getElementById('status-msg');

    fileInput.addEventListener('change', () => {
      if (fileInput.files.length) {
        fileName.textContent = '✓ ' + fileInput.files[0].name;
        fileName.style.display = 'block';
      }
    });

    function startUpload() {
      if (!fileInput.files.length) {
        statusMsg.textContent = 'Please select a .bin file first.';
        statusMsg.style.color = 'var(--danger)';
        return;
      }
      const formData = new FormData();
      formData.append('firmware', fileInput.files[0]);

      const xhr = new XMLHttpRequest();
      xhr.open('POST', '/update', true);

      xhr.upload.onprogress = (e) => {
        if (e.lengthComputable) {
          const pct = Math.round((e.loaded / e.total) * 100);
          progWrap.style.display = 'block';
          progBar.style.width = pct + '%';
          statusMsg.textContent = 'Uploading... ' + pct + '%';
          statusMsg.style.color = 'var(--muted)';
        }
      };
      xhr.onload = () => {
        if (xhr.status === 200) {
          statusMsg.textContent = '✓ Upload complete — rebooting…';
          statusMsg.style.color = 'var(--accent2)';
        } else {
          statusMsg.textContent = '✗ Upload failed (' + xhr.status + ')';
          statusMsg.style.color = 'var(--danger)';
        }
      };
      xhr.send(formData);
    }
  </script>
)html";
//
// ---------------------------------------------------------------------------
//  PAGE_FILE_UPLOAD  —  file system upload page
// ---------------------------------------------------------------------------
static const char _BODY_FILES[] PROGMEM = R"html(
  <div class='topbar'>
    <span class='logo'>&#9632; ESP32 OTA Demo</span>
    <span class='chip'>FILES</span>
  </div>

  <div class='card'>
    <h1>File Upload</h1>
    <h2>Write files to LittleFS</h2>
    <p>Uploaded files are stored on the ESP32 filesystem and survive reboots.</p>

    <label for='fs-file-input'>
      <div class='upload-area'>
        <p style='font-size:2rem;margin-bottom:0.5rem'>&#128193;</p>
        <p style='color:var(--text)'>Click to select a file</p>
      </div>
    </label>
    <input type='file' id='fs-file-input'>

    <div id='fs-file-name' style='font-size:0.8rem;color:var(--accent);margin:0.5rem 0;display:none'></div>

    <button class='btn primary' onclick='uploadFile()' style='margin-top:0.5rem'>&#8593; Upload File</button>
    &nbsp;
    <a href='/' class='btn outline'>Cancel</a>

    <div class='progress-wrap' id='fs-progress-wrap'>
      <div class='progress-bar' id='fs-progress-bar'></div>
    </div>
    <div id='fs-status' style='font-size:0.8rem;color:var(--muted);margin-top:0.8rem'></div>
  </div>

  <div class='card'>
    <h1>Files</h1>
    <h2>LittleFS contents</h2>
    <div id='file-list'><p>Loading...</p></div>
  </div>

  <script>
    const input    = document.getElementById('fs-file-input');
    const label    = document.getElementById('fs-file-name');
    const progWrap = document.getElementById('fs-progress-wrap');
    const progBar  = document.getElementById('fs-progress-bar');
    const status   = document.getElementById('fs-status');
    const fileList = document.getElementById('file-list');

    input.addEventListener('change', () => {
      if (input.files.length) {
        label.textContent = '✓ ' + input.files[0].name;
        label.style.display = 'block';
      }
    });

    function uploadFile() {
      if (!input.files.length) {
        status.textContent = 'Please select a file first.';
        status.style.color = 'var(--danger)';
        return;
      }
      const formData = new FormData();
      formData.append('file', input.files[0], input.files[0].name);

      const xhr = new XMLHttpRequest();
      xhr.open('POST', '/fs-upload', true);

      xhr.upload.onprogress = (e) => {
        if (e.lengthComputable) {
          const pct = Math.round((e.loaded / e.total) * 100);
          progWrap.style.display = 'block';
          progBar.style.width = pct + '%';
          status.textContent = 'Uploading... ' + pct + '%';
          status.style.color = 'var(--muted)';
        }
      };
      xhr.onload = () => {
        if (xhr.status === 200) {
          status.textContent = '✓ Upload complete';
          status.style.color = 'var(--accent2)';
        } else {
          status.textContent = '✗ Upload failed (' + xhr.status + ')';
          status.style.color = 'var(--danger)';
        }
      };
      xhr.send(formData);
    }

    function deleteFile(name) {
      fetch('/delete?name=' + encodeURIComponent(name), { method: 'DELETE' })
        .then(r => {
          if (r.ok)
            loadFiles();
          else
            alert('Delete failed: ' + r.status);
        });
    }

    function loadFiles() {
      fetch('/files', { headers: { 'Accept': 'application/json' } })
        .then(r => r.json())
        .then(files => {
          if (!files.length) {
            fileList.innerHTML = '<p>No files found.</p>';
            return;
          }
          fileList.innerHTML = files.map(f => `
            <div class='stat' style='display:flex;justify-content:space-between;align-items:center;grid-column:span 2'>
              <div>
                <div class='label'>${f.name}</div>
                <div class='value' style='font-size:0.75rem'>${f.size} bytes</div>
              </div>
              <button class='btn outline' style='color:var(--danger);border-color:var(--danger)'
                onclick='deleteFile("${f.name}")'>Delete</button>
            </div>
          `).join('');
        })
        .catch((e) => 
        { 
            fileList.innerHTML = '<p style="color:var(--danger)">Failed to load files.</p>'; 
            console.error("An error occured while fetching files: ", e.message);
        });
    }

    loadFiles();
  </script>
)html";

// ---------------------------------------------------------------------------
//  PAGE_NOT_FOUND  —  404 page
// ---------------------------------------------------------------------------
static const char _BODY_NOT_FOUND[] PROGMEM = R"html(
  <div class='topbar'>
    <span class='logo'>&#9632; ESP32 OTA Demo</span>
    <span class='chip' style='color:var(--danger);background:rgba(248,81,73,0.1);border-color:rgba(248,81,73,0.3)'>404</span>
  </div>
 
  <div class='card' style='text-align:center'>
    <p style='font-family:var(--font-mono);font-size:4rem;color:var(--border);margin-bottom:0.5rem'>404</p>
    <h1>Page Not Found</h1>
    <h2>That route doesn't exist on this device</h2>
    <div class='nav' style='justify-content:center;margin-top:1.5rem'>
      <a href='/'>&#8592; Back to Home</a>
    </div>
  </div>
)html";

inline String pageHome() { return buildPage("Home", _BODY_HOME); }
inline String pageUpdate() { return buildPage("Update", _BODY_UPDATE); }
inline String pageFSFiles() { return buildPage("Files", _BODY_FILES); }
inline String pageNotFound() { return buildPage("404", _BODY_NOT_FOUND); }

// Add more pages following the same pattern:
//   static const char _BODY_INFO[] PROGMEM = R"html( ... )html";
//   inline String pageInfo() { return buildPage("Info", _BODY_INFO); }

#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

const char* ssid     = "Airtel_anku_7025";
const char* password = "Air@79898";

WebServer server(80);

#define BUZZER 21

// ✅ USE THIS (IMPORTANT — NOT googleusercontent)
String googleURL = "https://script.google.com/macros/s/AKfycbyhErp9frApiDH5TXlS073fgDwGjfSsBhNbNqVesGgQT84MONxPSm9yf176m-5DSd9y/exec";

// Storage
String scannedIDs[100];
int scannedCount = 0;

String lastScan = "None";
String lastStatus = "Waiting...";


// ───── BUZZER ─────
void beepSuccess() {
  digitalWrite(BUZZER, HIGH); delay(150);
  digitalWrite(BUZZER, LOW);
}
void beepError() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER, HIGH); delay(100);
    digitalWrite(BUZZER, LOW); delay(100);
  }
}
void beepDuplicate() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(BUZZER, HIGH); delay(200);
    digitalWrite(BUZZER, LOW); delay(200);
  }
}


// ───── BASIC API ─────
void handleLast()   { server.send(200, "text/plain", lastScan); }
void handleStatus() { server.send(200, "text/plain", lastStatus); }


// ───── SEND TO GOOGLE ─────
void sendToGoogle(String id) {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;

  String url = googleURL + "?data=" + id;

  http.begin(client, url);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  int code = http.GET();

  Serial.print("Google Response: ");
  Serial.println(code);

  if (code > 0) {
    Serial.println(http.getString());
  }

  http.end();
}


// ───── FETCH DATA ─────
String fetchAttendance() {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;

  String url = googleURL + "?action=get";

  http.begin(client, url);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  int code = http.GET();

  String payload = "[]";

  if (code > 0) payload = http.getString();

  http.end();
  return payload;
}

void handleAll() {
  String data = fetchAttendance();
  server.send(200, "application/json", data);
}


// ───── VALIDATION ─────
String clean(String s) {
  s.trim();
  s.replace("\n", "");
  s.replace("\r", "");
  s.replace(" ", "");
  return s;
}

bool isDuplicate(String id) {
  for (int i = 0; i < scannedCount; i++) {
    if (scannedIDs[i] == id) return true;
  }
  return false;
}


// ───── SCAN ─────
void handleScan() {
  if (!server.hasArg("data")) return;

  String id = clean(server.arg("data"));

  Serial.println(id);

  if (isDuplicate(id)) {
    beepDuplicate();
    lastStatus = "Duplicate";
    lastScan = id;
    server.send(200, "text/plain", "Duplicate");
    return;
  }

  beepSuccess();
  lastStatus = "Present";
  lastScan = id;

  scannedIDs[scannedCount++] = id;

  sendToGoogle(id);

  server.send(200, "text/plain", "OK");
}


// ───── DASHBOARD UI ─────
void handleHome() {
  String page = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1.0"/>
<title>Attendance Dashboard</title>
<style>
@import url('https://fonts.googleapis.com/css2?family=Space+Mono:wght@400;700&family=DM+Sans:wght@400;500;600&display=swap');
*,*::before,*::after{box-sizing:border-box;margin:0;padding:0}
:root{
  --bg-primary:#ffffff;--bg-secondary:#f5f5f3;--bg-tertiary:#eeede8;
  --text-primary:#1a1a18;--text-secondary:#6b6b67;--text-tertiary:#9e9e99;
  --border:rgba(0,0,0,0.12);--border-md:rgba(0,0,0,0.20);
  --radius-md:8px;--radius-lg:12px;
  --green:#1D9E75;--green-light:#E1F5EE;--green-dark:#0F6E56;
  --red:#E24B4A;--red-light:#FCEBEB;--red-dark:#A32D2D;
  --amber:#BA7517;--amber-light:#FAEEDA;
}
@media(prefers-color-scheme:dark){:root{
  --bg-primary:#1e1e1c;--bg-secondary:#252523;--bg-tertiary:#2c2c2a;
  --text-primary:#f0efe8;--text-secondary:#9e9e99;--text-tertiary:#6b6b67;
  --border:rgba(255,255,255,0.10);--border-md:rgba(255,255,255,0.18);
  --green-light:#04342C;--green-dark:#9FE1CB;
  --red-light:#501313;--red-dark:#F7C1C1;--amber-light:#412402;
}}
body{font-family:'DM Sans',sans-serif;background:var(--bg-tertiary);color:var(--text-primary);min-height:100vh;padding:1.5rem}
.dash{max-width:960px;margin:0 auto}
.top-bar{display:flex;align-items:center;justify-content:space-between;margin-bottom:1.5rem}
.logo-area{display:flex;align-items:center;gap:10px}
.logo-icon{width:36px;height:36px;background:var(--green);border-radius:9px;display:flex;align-items:center;justify-content:center}
.logo-icon svg{width:20px;height:20px;fill:none;stroke:#fff;stroke-width:2}
.heading{font-family:'Space Mono',monospace;font-size:24px;font-weight:700;letter-spacing:3px}
.live-badge{display:flex;align-items:center;gap:6px;background:var(--green-light);color:var(--green-dark);font-size:12px;font-weight:500;padding:5px 12px;border-radius:20px}
.pulse{width:7px;height:7px;background:var(--green);border-radius:50%;animation:pulse 1.4s infinite}
@keyframes pulse{0%,100%{opacity:1;transform:scale(1)}50%{opacity:.5;transform:scale(1.5)}}
.stat-grid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:10px;margin-bottom:1.25rem}
.stat-card{background:var(--bg-primary);border:0.5px solid var(--border);border-radius:var(--radius-lg);padding:16px 18px}
.stat-label{font-size:11px;color:var(--text-secondary);text-transform:uppercase;letter-spacing:1px;margin-bottom:6px}
.stat-value{font-family:'Space Mono',monospace;font-size:28px;font-weight:700}
.stat-sub{font-size:12px;color:var(--text-tertiary);margin-top:4px}
.scan-card{background:var(--bg-primary);border:0.5px solid var(--border);border-radius:var(--radius-lg);padding:16px 18px;margin-bottom:1.25rem}
.scan-header{display:flex;align-items:center;justify-content:space-between;margin-bottom:12px}
.section-label{font-size:11px;font-weight:500;color:var(--text-secondary);text-transform:uppercase;letter-spacing:1px}
.scan-row{display:grid;grid-template-columns:1fr 1fr 1fr;gap:12px}
.scan-field{display:flex;flex-direction:column;gap:3px}
.scan-field-label{font-size:10px;color:var(--text-tertiary)}
.scan-field-value{font-family:'Space Mono',monospace;font-size:14px;font-weight:700}
.status-dot{display:inline-block;width:8px;height:8px;border-radius:50%;margin-right:5px}
.s-present{color:var(--green-dark)}.s-duplicate{color:var(--amber)}.s-waiting{color:var(--text-tertiary)}
.dot-green{background:var(--green)}.dot-amber{background:var(--amber)}.dot-gray{background:var(--text-tertiary)}
.filter-row{display:flex;gap:8px;margin-bottom:10px;flex-wrap:wrap}
.filter-btn{font-size:11px;padding:5px 12px;border-radius:20px;border:0.5px solid var(--border-md);background:transparent;color:var(--text-secondary);cursor:pointer;font-family:'DM Sans',sans-serif;transition:all .15s}
.filter-btn:hover{background:var(--bg-secondary)}
.filter-btn.active{background:var(--green);color:#fff;border-color:var(--green)}
.table-card{background:var(--bg-primary);border:0.5px solid var(--border);border-radius:var(--radius-lg);overflow:hidden;margin-bottom:1.25rem}
.table-head{display:grid;grid-template-columns:2.2fr 1fr 1.4fr 1fr;background:var(--bg-secondary);padding:9px 16px;font-size:11px;color:var(--text-secondary);text-transform:uppercase;letter-spacing:0.8px;font-weight:500}
.table-row{display:grid;grid-template-columns:2.2fr 1fr 1.4fr 1fr;padding:11px 16px;border-top:0.5px solid var(--border);align-items:center;font-size:13px;transition:background .15s}
.table-row:hover{background:var(--bg-secondary)}
.student-name{font-family:'Space Mono',monospace;font-size:12px;font-weight:700}
.badge{display:inline-block;font-size:10px;font-weight:500;padding:3px 8px;border-radius:12px}
.badge-present{background:var(--green-light);color:var(--green-dark)}
.badge-absent{background:var(--red-light);color:var(--red-dark)}
.badge-duplicate{background:var(--amber-light);color:var(--amber)}
.pct-bar-wrap{display:flex;align-items:center;gap:8px}
.pct-bar{flex:1;height:4px;background:var(--border);border-radius:2px;overflow:hidden}
.pct-fill{height:100%;border-radius:2px}
.pct-text{font-family:'Space Mono',monospace;font-size:11px;min-width:32px;text-align:right}
.split-grid{display:grid;grid-template-columns:1fr 1fr;gap:10px}
.alert-card{background:var(--bg-primary);border:0.5px solid var(--border);border-radius:var(--radius-lg);overflow:hidden}
.alert-header{padding:10px 14px;font-size:11px;font-weight:500;text-transform:uppercase;letter-spacing:1px}
.alert-header.danger{background:var(--red-light);color:var(--red-dark);border-bottom:0.5px solid rgba(226,75,74,0.2)}
.alert-header.success{background:var(--green-light);color:var(--green-dark);border-bottom:0.5px solid rgba(29,158,117,0.2)}
.alert-row{display:flex;align-items:center;justify-content:space-between;padding:10px 14px;border-top:0.5px solid var(--border);font-size:12px}
.alert-name{font-family:'Space Mono',monospace;font-size:12px;font-weight:700;color:var(--text-primary)}
.alert-pct{font-family:'Space Mono',monospace;font-weight:700;font-size:13px}
.alert-pct.danger{color:var(--red-dark)}.alert-pct.success{color:var(--green-dark)}
.mini-bar{width:55px;height:3px;border-radius:2px;background:var(--border);overflow:hidden;margin-right:4px}
.mini-fill-d{height:100%;background:var(--red);border-radius:2px}
.mini-fill-s{height:100%;background:var(--green);border-radius:2px}
.section-title{font-size:11px;font-weight:500;color:var(--text-secondary);text-transform:uppercase;letter-spacing:1px;margin-bottom:10px}
.row-hd{display:flex;align-items:center;justify-content:space-between;margin-bottom:10px}
.sync-txt{font-size:11px;color:var(--text-tertiary);font-family:'Space Mono',monospace}
.empty{padding:20px 16px;text-align:center;color:var(--text-tertiary);font-size:13px}
@media(max-width:600px){
  .stat-grid{grid-template-columns:1fr 1fr}
  .split-grid{grid-template-columns:1fr}
  .table-head,.table-row{grid-template-columns:2fr 1fr 1fr}
  .table-head span:last-child,.table-row div:last-child{display:none}
  .heading{font-size:18px}
}
</style>
</head>
<body>
<div class="dash">

  <div class="top-bar">
    <div class="logo-area">
      <div class="logo-icon">
        <svg viewBox="0 0 24 24"><rect x="3" y="11" width="18" height="11" rx="2"/><path d="M7 11V7a5 5 0 0110 0v4"/><circle cx="12" cy="16" r="1" fill="white" stroke="none"/></svg>
      </div>
      <span class="heading">ATTENDANCE</span>
    </div>
    <div class="live-badge"><div class="pulse"></div>Live</div>
  </div>

  <div class="stat-grid">
    <div class="stat-card">
      <div class="stat-label">Total Scanned</div>
      <div class="stat-value" id="s-total" style="color:var(--text-primary)">—</div>
      <div class="stat-sub">Students today</div>
    </div>
    <div class="stat-card">
      <div class="stat-label">Present</div>
      <div class="stat-value" id="s-present" style="color:var(--green)">—</div>
      <div class="stat-sub" id="s-pct">— attendance rate</div>
    </div>
    <div class="stat-card">
      <div class="stat-label">Absent</div>
      <div class="stat-value" id="s-absent" style="color:var(--red)">—</div>
      <div class="stat-sub" id="s-apct">—</div>
    </div>
  </div>

  <div class="scan-card">
    <div class="scan-header">
      <span class="section-label">Last Scan</span>
      <span style="font-family:'Space Mono',monospace;font-size:11px;color:var(--text-tertiary)">RFID · ESP32</span>
    </div>
    <div class="scan-row">
      <div class="scan-field">
        <span class="scan-field-label">Student ID</span>
        <span class="scan-field-value" id="ls-id">Waiting...</span>
      </div>
      <div class="scan-field">
        <span class="scan-field-label">Scanned At</span>
        <span class="scan-field-value" id="ls-time">—</span>
      </div>
      <div class="scan-field">
        <span class="scan-field-label">Result</span>
        <span class="scan-field-value s-waiting" id="ls-status">—</span>
      </div>
    </div>
  </div>

  <div class="row-hd">
    <div class="section-title" style="margin-bottom:0">Records</div>
    <span class="sync-txt" id="sync-lbl">Connecting...</span>
  </div>
  <div class="filter-row">
    <button class="filter-btn active" onclick="applyFilter('all',this)">All</button>
    <button class="filter-btn" onclick="applyFilter('Present',this)">Present</button>
    <button class="filter-btn" onclick="applyFilter('Absent',this)">Absent</button>
    <button class="filter-btn" onclick="applyFilter('Duplicate',this)">Duplicate</button>
  </div>
  <div class="table-card">
    <div class="table-head">
      <span>Student ID</span><span>Status</span><span>Attendance %</span><span>Time</span>
    </div>
    <div id="tbl-body"><div class="empty">Loading records...</div></div>
  </div>

  <div class="section-title">Attendance Alerts</div>
  <div class="split-grid">
    <div class="alert-card">
      <div class="alert-header danger">&#9888; Below 35% — At Risk</div>
      <div id="low-list"></div>
    </div>
    <div class="alert-card">
      <div class="alert-header success">&#10003; Above 35% — Good Standing</div>
      <div id="high-list"></div>
    </div>
  </div>

</div>
<script>
let allStudents  = [];
let activeFilter = "all";

function pctColor(p){
  if(p>=75) return "var(--green)";
  if(p>=35) return "var(--amber)";
  return "var(--red)";
}

async function loadAll(){
  try{
    const res  = await fetch('/all');
    const data = await res.json();
    allStudents = [];
    for(let i=1;i<data.length;i++){
      const r = data[i];
      allStudents.push({
        id:     r[0] || "—",
        time:   r[1] || "—",
        status: r[2] || "Present",
        pct:    parseInt(r[3]) || 0
      });
    }
    renderStats();
    renderTable(activeFilter);
    renderAlerts();
    document.getElementById("sync-lbl").textContent =
      "Synced " + new Date().toLocaleTimeString();
  } catch(e){
    document.getElementById("sync-lbl").textContent = "ESP32 offline";
  }
}

function renderStats(){
  const total   = allStudents.length;
  const present = allStudents.filter(s=>s.status==="Present").length;
  const absent  = total - present;
  const pct     = total ? Math.round((present/total)*100) : 0;
  document.getElementById("s-total").textContent   = total;
  document.getElementById("s-present").textContent = present;
  document.getElementById("s-absent").textContent  = absent;
  document.getElementById("s-pct").textContent     = pct + "% attendance rate";
  document.getElementById("s-apct").textContent    = (100-pct) + "% not marked";
}

function renderTable(filter){
  activeFilter = filter;
  const body = document.getElementById("tbl-body");
  const rows = filter==="all" ? allStudents : allStudents.filter(s=>s.status===filter);
  if(!rows.length){
    body.innerHTML = '<div class="empty">No records found</div>';
    return;
  }
  body.innerHTML = rows.map(s=>{
    const col = pctColor(s.pct);
    const badge = s.status==="Present"   ? "badge-present"   :
                  s.status==="Duplicate" ? "badge-duplicate" : "badge-absent";
    return `
    <div class="table-row">
      <div class="student-name">${s.id}</div>
      <div><span class="badge ${badge}">${s.status}</span></div>
      <div class="pct-bar-wrap">
        <div class="pct-bar"><div class="pct-fill" style="width:${s.pct}%;background:${col}"></div></div>
        <span class="pct-text" style="color:${col}">${s.pct}%</span>
      </div>
      <div style="font-family:'Space Mono',monospace;font-size:11px;color:var(--text-secondary)">${s.time}</div>
    </div>`;
  }).join("");
}

function applyFilter(f,btn){
  document.querySelectorAll(".filter-btn").forEach(b=>b.classList.remove("active"));
  btn.classList.add("active");
  renderTable(f);
}

function renderAlerts(){
  const low  = allStudents.filter(s=>s.pct<35&&s.pct>0).sort((a,b)=>a.pct-b.pct);
  const high = allStudents.filter(s=>s.pct>=35).sort((a,b)=>b.pct-a.pct).slice(0,6);
  const alertRow=(s,t)=>`
    <div class="alert-row">
      <span class="alert-name">${s.id}</span>
      <div style="display:flex;align-items:center;gap:6px">
        <div class="mini-bar"><div class="mini-fill-${t}" style="width:${s.pct}%"></div></div>
        <span class="alert-pct ${t==='d'?'danger':'success'}">${s.pct}%</span>
      </div>
    </div>`;
  document.getElementById("low-list").innerHTML  = low.length
    ? low.map(s=>alertRow(s,'d')).join("")
    : '<div class="alert-row" style="color:var(--text-tertiary)">No students at risk</div>';
  document.getElementById("high-list").innerHTML = high.length
    ? high.map(s=>alertRow(s,'s')).join("")
    : '<div class="alert-row" style="color:var(--text-tertiary)">No data yet</div>';
}

async function loadLast(){
  try{
    const [r1,r2] = await Promise.all([fetch('/last'),fetch('/status')]);
    const id = await r1.text();
    const st = await r2.text();
    document.getElementById("ls-id").textContent   = id || "—";
    document.getElementById("ls-time").textContent = new Date().toLocaleTimeString();
    const el = document.getElementById("ls-status");
    if(st==="Present"){
      el.innerHTML = '<span class="status-dot dot-green"></span>PRESENT';
      el.className = "scan-field-value s-present";
    } else if(st==="Duplicate"){
      el.innerHTML = '<span class="status-dot dot-amber"></span>DUPLICATE';
      el.className = "scan-field-value s-duplicate";
    } else {
      el.innerHTML = '<span class="status-dot dot-gray"></span>' + st.toUpperCase();
      el.className = "scan-field-value s-waiting";
    }
  } catch(e){}
}

loadAll();
loadLast();
setInterval(loadAll,  5000);
setInterval(loadLast, 2000);
</script>
</body>
</html>
)=====";
  server.send(200, "text/html", page);
}


// ───── SETUP ─────
void setup() {
  Serial.begin(115200);
  pinMode(BUZZER, OUTPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nIP: " + WiFi.localIP().toString());

  server.on("/", handleHome);
  server.on("/scan", handleScan);
  server.on("/last", handleLast);
  server.on("/status", handleStatus);
  server.on("/all", handleAll);

  server.begin();
}


// ───── LOOP ─────
void loop() {
  server.handleClient();
}
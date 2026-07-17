#pragma once
#include <pgmspace.h>

static const char PAGE_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8"/>
<meta name="viewport" content="width=device-width, initial-scale=1"/>
<title>WordClock Setup</title>
<style>
:root{
  --bg:#050607;
  --panel:#0a0c0e;
  --panel2:#07090a;
  --border:rgba(255,255,255,.10);
  --text:#f1e7d6;
  --muted:rgba(241,231,214,.62);
  --muted2:rgba(241,231,214,.45);
  --accentY:#f3c623;
  --accentR:#ff3b30;
  --ok:#d6ff9f;
  --shadow:0 18px 60px rgba(0,0,0,.55);
}

@font-face{
  font-family:"WordClock";
  src:url("/wordclock.woff2") format("woff2");
  font-display:swap;
}

*{box-sizing:border-box}
html,body{height:100%}
body{
  margin:0;
  background:
    radial-gradient(900px 500px at 20% -20%, rgba(243,198,35,.10), transparent 60%),
    radial-gradient(900px 500px at 100% 0%, rgba(255,59,48,.08), transparent 55%),
    var(--bg);
  color:var(--text);
  font-family: ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Arial;
}

.wrap{max-width:980px;margin:26px auto;padding:14px}

header{
  display:flex;align-items:flex-end;justify-content:space-between;
  padding:10px 12px;margin-bottom:14px
}
.brand{display:flex;flex-direction:column;gap:4px}
.brand .t{
  font-family:"WordClock", ui-sans-serif, system-ui;
  font-size:30px;letter-spacing:1.2px;line-height:1;
}
.brand .s{font-size:12px;color:var(--muted2);letter-spacing:.2px}
.chip{
  font-size:12px;color:var(--muted);
  border:1px solid var(--border);
  border-radius:999px;padding:6px 10px;
  background:linear-gradient(180deg, rgba(255,255,255,.03), transparent);
}

.grid{display:grid;grid-template-columns:1fr;gap:12px}
@media(min-width:860px){.grid{grid-template-columns:1fr 1fr}}

.card{
  background:linear-gradient(180deg, rgba(255,255,255,.03), transparent 55%), var(--panel);
  border:1px solid var(--border);
  border-radius:18px;
  box-shadow:var(--shadow);
  padding:14px;
}

.advancedWrap{
  margin-top:12px;
  border-radius:18px;
  border:1px solid rgba(255,255,255,.07);
  background:linear-gradient(180deg, rgba(255,255,255,.015), transparent 55%), rgba(7,9,10,.65);
  box-shadow:none;
  filter:saturate(.85);
}
.advancedWrap details{padding:14px}
.advancedWrap details[open]{filter:saturate(1)}
.advancedWrap .inner{
  margin-top:10px;padding-top:10px;
  border-top:1px solid rgba(255,255,255,.08);
}

.advSummary{
  font-family:"WordClock", ui-sans-serif, system-ui;
  font-size:14px;
  letter-spacing:1px;
  display:flex;align-items:center;gap:10px;
  color:var(--text);
  user-select:none;
}
.advSummary .bar{
  height:1px;flex:1;
  background:linear-gradient(90deg, rgba(255,255,255,.10), rgba(255,255,255,.06), transparent);
}
.advancedWrap summary{
  cursor:pointer;
  list-style:none;
}
.advancedWrap summary::-webkit-details-marker{display:none}
.advancedWrap summary::after{
  content:"▾";
  float:right;
  color:rgba(241,231,214,.45);
  margin-left:10px;
}
.advancedWrap details[open] summary::after{content:"▴"}

.section-title{
  font-family:"WordClock", ui-sans-serif, system-ui;
  font-size:14px;
  letter-spacing:1px;
  margin:0 0 10px 0;
  display:flex;align-items:center;gap:10px;
}
.section-title .bar{
  height:1px;flex:1;
  background:linear-gradient(90deg, rgba(243,198,35,.55), rgba(255,59,48,.25), transparent);
}

label{display:block;margin:10px 0 6px;color:var(--muted);font-size:12px;letter-spacing:.2px}

input,select{
  width:100%;
  padding:11px 12px;
  border-radius:12px;
  border:1px solid var(--border);
  background:var(--panel2);
  color:var(--text);
  outline:none;
}

input[type="color"]{padding:0;height:44px}
input[type="range"]{padding:0;height:44px}
input[type="number"]{appearance:textfield}

.row{display:grid;grid-template-columns:1fr 1fr;gap:10px}

.hr{
  height:1px;margin:12px 0;
  background:linear-gradient(90deg, transparent, rgba(255,255,255,.10), transparent);
}

.status{margin-top:10px;font-size:13px;color:var(--muted);text-align:end}
.status.ok{color:var(--ok)}
.status.bad{color:var(--accentR)}
.mono{font-family:ui-monospace,SFMono-Regular,Menlo,Monaco,Consolas,monospace}
.small{font-size:12px;color:var(--muted2);margin-top:8px}

.badge{
  display:inline-flex;align-items:center;gap:8px;
  padding:6px 10px;border-radius:999px;
  border:1px solid var(--border);
  background:rgba(255,255,255,.02);
  color:var(--muted);
  font-size:12px;
}
.sw{
  display:inline-block;width:10px;height:10px;border-radius:3px;
  border:1px solid rgba(255,255,255,.18);
  vertical-align:middle;
}
.themeRow{display:flex;align-items:center;gap:10px}
.themePreview{
  display:flex;gap:6px;align-items:center;
  padding:6px 8px;border:1px solid var(--border);
  border-radius:999px;background:rgba(255,255,255,.02);
}
.themePreview .sw{margin-right:0}

.rangeRow{
  display:grid;
  grid-template-columns:48px 1fr 48px;
  gap:10px;
  align-items:center;
}
.rangeRow button{padding:10px 10px;border-radius:12px;font-weight:800}

/* ---- Password eye toggle ---- */
.pwWrap{position:relative;width:100%}
.pwWrap input{padding-right:44px}
.pwEye{
  position:absolute;right:8px;top:50%;
  transform:translateY(-50%);
  width:34px;height:34px;border-radius:12px;
  border:1px solid rgba(255,255,255,.10);
  background:linear-gradient(180deg, rgba(255,255,255,.03), transparent 70%), rgba(7,9,10,.25);
  display:inline-flex;align-items:center;justify-content:center;
  cursor:pointer;
  transition:transform .05s ease, border-color .15s ease, background .15s ease, filter .15s ease;
}
.pwEye:hover{
  border-color:rgba(243,198,35,.22);
  background:linear-gradient(180deg, rgba(243,198,35,.06), rgba(255,59,48,.03));
}
.pwEye:active{ transform:translateY(-50%) translateY(1px) }
.pwEye:focus-visible{outline:none;box-shadow:0 0 0 3px rgba(243,198,35,.18)}
.pwEye svg{width:18px;height:18px;stroke:rgba(241,231,214,.70);transition:stroke .15s ease, opacity .15s ease}
.pwEye:hover svg{stroke:rgba(243,198,35,.90)}
.pwEye[data-show="1"] svg{stroke:rgba(243,198,35,.95)}
.pwEye .slash{opacity:0}
.pwEye[data-show="1"] .slash{opacity:0}
.pwEye[data-show="0"] .slash{opacity:1}

/* -------- Buttons -------- */
.btn{
  appearance:none;
  border-radius:999px;
  border:1px solid rgba(255,255,255,.12);
  padding:11px 14px;
  background:rgba(255,255,255,.03);
  color:var(--text);
  cursor:pointer;
  font-weight:750;
  letter-spacing:.2px;
  display:inline-flex;align-items:center;gap:10px;
  transition:transform .05s ease, border-color .15s ease, background .15s ease, filter .15s ease;
}
.btn:active{transform:translateY(1px)}
.btn:hover{border-color:rgba(255,255,255,.18); background:rgba(255,255,255,.04)}
.btn .dot{
  width:9px;height:9px;border-radius:999px;
  background:rgba(255,255,255,.18);
  box-shadow:0 0 0 3px rgba(255,255,255,.06);
}

/* Scan button states */
.btn.scan{border-color:rgba(243,198,35,.22)}
.btn.scan .dot{
  background:rgba(243,198,35,.35);
  box-shadow:0 0 0 3px rgba(243,198,35,.06);
}
@keyframes blinkY {
  0%,100% { opacity: .25; transform: scale(.95); }
  50%     { opacity: 1.0; transform: scale(1.0); }
}
.btn.scan.scanning .dot{
  background:rgba(243,198,35,.95);
  box-shadow:0 0 0 3px rgba(243,198,35,.10), 0 0 18px rgba(243,198,35,.22);
  animation:blinkY .6s infinite;
}
.btn.scan.scanned .dot{
  opacity:1;
  background:rgba(243,198,35,.95);
  box-shadow:0 0 0 3px rgba(243,198,35,.10), 0 0 18px rgba(243,198,35,.22);
  animation:none;
}

.btn.save{
  border-color:rgba(243,198,35,.30);
  background:linear-gradient(180deg, rgba(243,198,35,.14), rgba(255,59,48,.06));
}
.btn.save .dot{
  background:rgba(255,59,48,.90);
  box-shadow:0 0 0 3px rgba(255,59,48,.12), 0 0 22px rgba(255,59,48,.18);
}
@keyframes blinkG {
  0%,100% { opacity: .25; transform: scale(.95); }
  50%     { opacity: 1.0; transform: scale(1.0); }
}
.btn.save.saving .dot{
  background:rgba(54, 255, 120, .95);
  box-shadow:0 0 0 3px rgba(54,255,120,.10), 0 0 18px rgba(54,255,120,.22);
  animation:blinkG .6s infinite;
}
.btn.save.ok .dot{
  background:rgba(54, 255, 120, .95);
  box-shadow:0 0 0 3px rgba(54,255,120,.10), 0 0 18px rgba(54,255,120,.22);
  animation:none;
}
.btn.save.fail .dot{
  background:rgba(255,59,48,.95);
  box-shadow:0 0 0 3px rgba(255,59,48,.12), 0 0 22px rgba(255,59,48,.18);
  animation:none;
}

.halfBtnRow{
  display:grid;
  grid-template-columns:1fr 1fr;
  gap:10px;
  margin-top:12px;
  align-items:center;
}
.halfBtnRow .btn{width:100%;justify-content:center}

.saveBar{
  margin-top:14px;
  padding:12px 14px;
  border:1px solid rgba(255,255,255,.08);
  border-radius:18px;
  background:linear-gradient(180deg, rgba(255,255,255,.02), transparent 55%), rgba(10,12,14,.55);
  display:grid;
  grid-template-columns:1fr 1fr;
  gap:10px;
  align-items:center;
}
.saveMeta{min-width:0}
.saveMeta .title{font-size:12px;color:var(--muted2);letter-spacing:.2px}
.saveMeta .note{
  font-size:12px;color:var(--muted);
  white-space:nowrap;overflow:hidden;text-overflow:ellipsis;
}

/* -------- Switch -------- */
.switchRow{
  display:flex;
  align-items:center;
  justify-content:space-between;
  gap:12px;
  margin-top:12px;
  padding:10px 12px;
  border-radius:14px;
  border:1px solid rgba(255,255,255,.08);
  background:linear-gradient(180deg, rgba(255,255,255,.02), transparent 70%), rgba(7,9,10,.35);
}
.switchRow .labelWrap{min-width:0}
.switchRow .labelWrap .title{
  font-size:12px;
  color:var(--text);
  letter-spacing:.2px;
  font-weight:750;
}
.switchRow .labelWrap .sub{
  font-size:12px;
  color:var(--muted2);
  margin-top:2px;
  white-space:nowrap;
  overflow:hidden;
  text-overflow:ellipsis;
}

.switch{
  position:relative;
  display:inline-flex;
  align-items:center;
  justify-content:center;
  width:48px;height:28px;flex:0 0 auto;
}
.switch input{
  position:absolute;inset:0;
  opacity:0;cursor:pointer;margin:0;
}
.track{
  width:48px;height:28px;border-radius:999px;
  border:1px solid rgba(255,255,255,.12);
  background:rgba(255,255,255,.03);
  box-shadow: inset 0 0 0 1px rgba(0,0,0,.25);
  transition: background .20s ease, border-color .20s ease, box-shadow .20s ease, filter .20s ease;
}
.thumb{
  position:absolute;left:4px;
  width:20px;height:20px;border-radius:999px;
  background:rgba(241,231,214,.85);
  box-shadow:0 8px 18px rgba(0,0,0,.45), inset 0 0 0 1px rgba(255,255,255,.22);
  transform: translateX(0);
  transition: transform .20s cubic-bezier(.2,.9,.2,1), background .20s ease;
}
.switch input:focus-visible + .track{
  outline: none;
  box-shadow: 0 0 0 3px rgba(243,198,35,.18), inset 0 0 0 1px rgba(0,0,0,.25);
}
.switch input:checked + .track{
  border-color: rgba(243,198,35,.35);
  background: linear-gradient(180deg, rgba(243,198,35,.16), rgba(255,59,48,.06));
  filter: saturate(1.05);
}
.switch input:checked + .track .thumb{
  transform: translateX(20px);
  background: rgba(243,198,35,.95);
}

.reveal{
  margin-top:10px;
  padding:10px 12px;
  border-radius:14px;
  border:1px solid rgba(255,255,255,.08);
  background:rgba(7,9,10,.25);
}
.warn{margin-top:8px;font-size:12px;color:rgba(255,59,48,.85)}
</style>
</head>
<body>
<div class="wrap">

  <header>
    <div class="brand">
      <div class="t">WORTUHR</div>
      <div class="s" id="subtitle">Setup • Live preview</div>
    </div>

    <div style="display:flex; gap:10px; align-items:center;">
      <select id="lang" class="chip mono" style="padding:6px 10px; border-radius:999px;">
        <option value="en">EN</option>
        <option value="de">DE</option>
      </select>
      <div class="chip mono" id="chip">ESP32</div>
    </div>
  </header>

  <div class="grid">

    <div class="card">
      <h2 class="section-title"><span id="t_wifi">WIFI SETTINGS</span> <span class="bar"></span></h2>

      <label id="l_ssid">Network (SSID)</label>
      <select id="ssid"></select>

      <div id="pskBlock">
        <label id="l_pass">Password</label>
        <div class="pwWrap">
          <input id="pass" type="password" placeholder="Password"/>
          <button class="pwEye" type="button" id="passEye" aria-label="Show password" data-show="0">
            <svg viewBox="0 0 24 24" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
              <path d="M2 12s3.5-7 10-7 10 7 10 7-3.5 7-10 7S2 12 2 12z"></path>
              <circle cx="12" cy="12" r="3"></circle>
              <path class="slash" d="M4 4L20 20"></path>
            </svg>
          </button>
        </div>
        <input id="passUnchanged" type="hidden" value="1"/>
      </div>

      <div id="enterpriseBlock" class="reveal" style="display:none;">
        <div class="small mono" style="margin-top:0;" id="t_eap_head">WPA2-Enterprise (802.1X)</div>

        <label id="l_eap_identity">Identity (optional)</label>
        <input id="eapIdentity" type="text" placeholder="anonymous@realm (optional)"/>

        <label id="l_eap_user">Username</label>
        <input id="eapUser" type="text" placeholder="user@realm"/>

        <label id="l_eap_pass">Password</label>
        <div class="pwWrap">
          <input id="eapPass" type="password" placeholder="Password"/>
          <button class="pwEye" type="button" id="eapPassEye" aria-label="Show password" data-show="0">
            <svg viewBox="0 0 24 24" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
              <path d="M2 12s3.5-7 10-7 10 7 10 7-3.5 7-10 7S2 12 2 12z"></path>
              <circle cx="12" cy="12" r="3"></circle>
              <path class="slash" d="M4 4L20 20"></path>
            </svg>
          </button>
        </div>
        <input id="eapPassUnchanged" type="hidden" value="1"/>

        <div class="warn mono" id="t_eap_warn">No certificate validation (insecure).</div>
      </div>

      <div class="halfBtnRow">
        <div></div>
        <button id="scanBtn" class="btn scan" type="button" onclick="scan()">
          <span class="dot"></span>
          <span id="b_scan">Scan</span>
        </button>
      </div>

      <div class="hr"></div>

      <h2 class="section-title"><span id="t_tz">TIMEZONE</span> <span class="bar"></span></h2>

      <label id="l_tz">Timezone</label>
      <select id="tz">
        <option value="Europe/Berlin">Europe/Berlin</option>
        <option value="Europe/Amsterdam">Europe/Amsterdam</option>
        <option value="Europe/Paris">Europe/Paris</option>
        <option value="Europe/Rome">Europe/Rome</option>
        <option value="Europe/Madrid">Europe/Madrid</option>
        <option value="Europe/London">Europe/London</option>
        <option value="America/New_York">America/New_York</option>
        <option value="America/Chicago">America/Chicago</option>
        <option value="America/Denver">America/Denver</option>
        <option value="America/Los_Angeles">America/Los_Angeles</option>
        <option value="Asia/Tokyo">Asia/Tokyo</option>
        <option value="Asia/Shanghai">Asia/Shanghai</option>
        <option value="Asia/Singapore">Asia/Singapore</option>
        <option value="Asia/Dubai">Asia/Dubai</option>
        <option value="Australia/Sydney">Australia/Sydney</option>
        <option value="UTC">UTC</option>
      </select>

      <div class="small mono" id="tzmini">detected local time: —</div>
    </div>

    <div class="card">
      <h2 class="section-title"><span id="t_themes">THEMES</span> <span class="bar"></span></h2>

      <label id="l_theme">Theme</label>
      <div class="row">
        <div><select id="themeSelect"></select></div>
        <div class="themeRow">
          <div class="themePreview" title="Preview">
            <span class="sw" id="swM"></span>
            <span class="sw" id="swH"></span>
          </div>
          <span class="badge mono" id="themeMeta">—</span>
        </div>
      </div>

      <div class="row" style="margin-top:10px">
        <div>
          <label id="l_min_color">Minute Color</label>
          <input id="colorM" type="color"/>
        </div>
        <div>
          <label id="l_hour_color">Hour Color</label>
          <input id="colorH" type="color"/>
        </div>
      </div>

      <div class="row">
        <div>
          <label id="l_fade_mode">Fade Mode</label>
          <select id="fadeMode">
            <option value="0" id="opt_fade_0">Off</option>
            <option value="1" id="opt_fade_1">2-Phase</option>
            <option value="2" id="opt_fade_2">Cross-Fade</option>
            <option value="3" id="opt_fade_3">Fade-Wipe</option>
          </select>
        </div>
        <div>
          <label><span id="l_fade_time">Fade Time:</span> <span class="mono" id="fadeMsLabel">200</span> ms</label>
          <input id="fadeMs" type="range" min="0" max="2000" step="100"/>
        </div>
      </div>

      <div class="hr"></div>

      <h2 class="section-title"><span id="t_bri">BRIGHTNESS</span> <span class="bar"></span></h2>

      <label><span id="l_max_bri">Max Brightness:</span> <span class="mono" id="briLabel">40</span> / 255</label>
      <div class="rangeRow">
        <button type="button" id="briDown" class="btn" aria-label="Darker" style="justify-content:center;width:48px;padding:10px 0;border-radius:12px">◀</button>
        <input id="maxBrightness" type="range" min="1" max="255" step="1"/>
        <button type="button" id="briUp" class="btn" aria-label="Brighter" style="justify-content:center;width:48px;padding:10px 0;border-radius:12px">▶</button>
      </div>

      <div class="switchRow">
        <div class="labelWrap">
          <div class="title" id="t_night_enable">Enable Nightmode</div>
          <div class="sub" id="t_night_sub">Activates at low light in time window</div>
        </div>
        <label class="switch" aria-label="Enable nightmode">
          <input id="enableNightmode" type="checkbox"/>
          <span class="track"><span class="thumb"></span></span>
        </label>
      </div>

      <div id="nightFields" class="reveal" style="display:none;">
        <label id="l_night_behavior">Nightmode behavior</label>
        <select id="nightBehavior">
          <option value="0" id="opt_night_0">Disable minute dots</option>
          <option value="1" id="opt_night_1">Show only UHR/HALB</option>
          <option value="2" id="opt_night_2">Disable LEDs</option>
        </select>

        <div class="row" style="margin-top:10px">
          <div>
            <label id="l_night_start">Start</label>
            <input id="nightStart" type="time" value="22:00"/>
          </div>
          <div>
            <label id="l_night_end">End</label>
            <input id="nightEnd" type="time" value="07:00"/>
          </div>
        </div>
      </div>
    </div>
  </div>

  <div class="advancedWrap">
    <details>
      <summary>
        <div class="advSummary">
          <span id="t_adv">ADVANCED SETTINGS</span> <span class="bar"></span>
        </div>
      </summary>

      <div class="inner">
        <div class="switchRow">
          <div class="labelWrap">
            <div class="title" id="t_enable_eap">Enable WPA2-Enterprise</div>
            <div class="sub" id="t_enable_eap_sub">Shows 802.1X login fields in WiFi settings</div>
          </div>
          <label class="switch" aria-label="Enable WPA2-Enterprise">
            <input id="enableEnterprise" type="checkbox"/>
            <span class="track"><span class="thumb"></span></span>
          </label>
        </div>

        <div class="switchRow">
          <div class="labelWrap">
            <div class="title" id="t_min_dots">Enable minute dots</div>
            <div class="sub" id="t_min_dots_sub">Show 1–4 dots for minutes</div>
          </div>
          <label class="switch" aria-label="Enable minute dots">
            <input id="enableMinuteDots" type="checkbox"/>
            <span class="track"><span class="thumb"></span></span>
          </label>
        </div>

        <div class="switchRow">
          <div class="labelWrap">
            <div class="title" id="t_disable_auto">Disable AutoBrightness</div>
            <div class="sub" id="t_disable_auto_sub">Use fixed brightness (Max Brightness)</div>
          </div>
          <label class="switch" aria-label="Disable autobrightness">
            <input id="disableAutoBrightness" type="checkbox"/>
            <span class="track"><span class="thumb"></span></span>
          </label>
        </div>

        <div class="hr"></div>
        <div class="row">
          <div>
            <label id="l_lux_live">Ambient Lux (live)</label>
            <div class="badge mono" id="luxLive" style="width:100%; justify-content:center;">—</div>
          </div>
          <div>
            <label id="l_out_live">Output now (live)</label>
            <div class="badge mono" id="briLive" style="width:100%; justify-content:center;">—</div>
          </div>
        </div>
        <div class="small mono" id="luxNote">—</div>

        <div class="row">
          <div>
            <label id="l_boot_color">Boot Color</label>
            <input id="bootColor" type="color"/>
          </div>
          <div>
            <label id="l_boot_fade">Boot Fade-In (ms)</label>
            <input id="bootFadeMs" type="number" min="0" max="20000"/>
          </div>
        </div>

        <div class="row">
          <div>
            <label id="l_conn_color">Connect Color</label>
            <input id="connectColor" type="color"/>
          </div>
          <div>
            <label id="l_conn_pulse">Connect Pulse (ms)</label>
            <input id="connectPulseMs" type="number" min="200" max="5000"/>
          </div>
        </div>

        <div class="row">
          <div>
            <label id="l_ntp">NTP Sync (hours)</label>
            <input id="ntpSyncHours" type="number" min="1" max="168"/>
          </div>
          <div>
            <label id="l_wifi_to">WiFi Timeout (sec)</label>
            <input id="connectionTimeoutSec" type="number" min="3" max="60"/>
          </div>
        </div>

        <div class="row">
          <div>
            <label id="l_step">Animation Step (ms)</label>
            <input id="stepMs" type="number" min="5" max="200"/>
          </div>
          <div>
            <label id="l_portal">Portal Inactive (sec)</label>
            <input id="portalInactiveMaxSec" type="number" min="30" max="3600" step="5"/>
          </div>
        </div>

        <div class="row">
          <div>
            <label id="l_lux_int">Lux Check (sec)</label>
            <input id="luxCheckIntervalSec" type="number" min="1" max="60" step="1"/>
          </div>
          <div>
            <label id="l_hyst">Hysteresis Delta</label>
            <input id="hysteresisDelta" type="number" min="1" max="50" step="1"/>
          </div>
        </div>

        <div class="row">
          <div>
            <label id="l_lux_min">Lux Min</label>
            <input id="luxMin" type="number" min="0" max="50000" step="0.1"/>
          </div>
          <div>
            <label id="l_lux_max">Lux Max</label>
            <input id="luxMax" type="number" min="0" max="50000" step="1"/>
          </div>
        </div>

        <div class="row">
          <div>
            <label id="l_night_thr">Nightmode Lux Threshold</label>
            <input id="nightLuxThreshold" type="number" min="0" max="500" step="0.5"/>
          </div>
          <div>
            <label id="l_ota">OTA Update</label>
            <button class="btn scan" type="button" id="otaBtn" onclick="openOta()" style="width:100%; justify-content:center;">
              <span class="dot"></span>
              <span id="b_ota">open in Browser</span>
            </button>
          </div>
        </div>

      </div>
    </details>
  </div>

  <div class="saveBar">
    <div class="saveMeta">
      <div class="title mono" id="t_finish">finish setup of</div>
      <div class="note mono" id="savehint">your WordClock</div>
    </div>

    <button id="saveBtn" class="btn save" onclick="save()" style="width:100%;justify-content:center">
      <span class="dot"></span>
      <span id="b_save">Save</span>
    </button>
  </div>

  <div class="status" id="savestatus"></div>

</div>

<script>
/* -------- DOM helpers / cached refs -------- */
const $ = (id)=>document.getElementById(id);
const els = {
  chip: $('chip'),
  lang: $('lang'),

  ssid: $('ssid'),
  scanBtn: $('scanBtn'),

  pskBlock: $('pskBlock'),
  pass: $('pass'),
  passEye: $('passEye'),
  passUnchanged: $('passUnchanged'),

  enterpriseBlock: $('enterpriseBlock'),
  eapIdentity: $('eapIdentity'),
  eapUser: $('eapUser'),
  eapPass: $('eapPass'),
  eapPassEye: $('eapPassEye'),
  eapPassUnchanged: $('eapPassUnchanged'),

  tz: $('tz'),
  tzmini: $('tzmini'),

  themeSelect: $('themeSelect'),
  themeMeta: $('themeMeta'),
  swM: $('swM'),
  swH: $('swH'),

  colorM: $('colorM'),
  colorH: $('colorH'),

  fadeMode: $('fadeMode'),
  fadeMs: $('fadeMs'),
  fadeMsLabel: $('fadeMsLabel'),

  maxBrightness: $('maxBrightness'),
  briLabel: $('briLabel'),
  briDown: $('briDown'),
  briUp: $('briUp'),

  enableNightmode: $('enableNightmode'),
  nightFields: $('nightFields'),
  nightBehavior: $('nightBehavior'),
  nightStart: $('nightStart'),
  nightEnd: $('nightEnd'),

  enableEnterprise: $('enableEnterprise'),
  enableMinuteDots: $('enableMinuteDots'),
  disableAutoBrightness: $('disableAutoBrightness'),

  luxLive: $('luxLive'),
  briLive: $('briLive'),
  luxNote: $('luxNote'),

  bootColor: $('bootColor'),
  bootFadeMs: $('bootFadeMs'),

  connectColor: $('connectColor'),
  connectPulseMs: $('connectPulseMs'),

  ntpSyncHours: $('ntpSyncHours'),
  connectionTimeoutSec: $('connectionTimeoutSec'),

  stepMs: $('stepMs'),
  portalInactiveMaxSec: $('portalInactiveMaxSec'),
  luxCheckIntervalSec: $('luxCheckIntervalSec'),
  hysteresisDelta: $('hysteresisDelta'),
  nightLuxThreshold: $('nightLuxThreshold'),

  luxMin: $('luxMin'),
  luxMax: $('luxMax'),

  saveBtn: $('saveBtn'),
  savestatus: $('savestatus'),
};

/* -------- i18n -------- */
const I18N = {
  en: {
    subtitle: "Setup • Live preview",
    t_wifi: "WIFI SETTINGS",
    l_ssid: "Network (SSID)",
    l_pass: "Password",
    b_scan: "Scan",

    t_tz: "TIMEZONE",
    l_tz: "Timezone",
    tzmini: (local, sel) => `detected local time: ${local} • selected: ${sel}`,

    t_themes: "THEMES",
    l_theme: "Theme",
    l_min_color: "Minute Color",
    l_hour_color: "Hour Color",
    l_fade_mode: "Fade Mode",
    l_fade_time: "Fade Time:",
    fade0: "Off",
    fade1: "2-Phase",
    fade2: "Cross-Fade",
    fade3: "Fade-Wipe",

    t_bri: "BRIGHTNESS",
    l_max_bri: "Max Brightness:",

    t_night_enable: "Enable Nightmode",
    t_night_sub: "Activates at low light in time window",
    l_night_behavior: "Nightmode behavior",
    opt_night_0: "Disable minute dots",
    opt_night_1: "Show only UHR/HALB",
    opt_night_2: "Disable LEDs",
    l_night_start: "Start",
    l_night_end: "End",

    t_adv: "ADVANCED SETTINGS",
    t_enable_eap: "Enable WPA2-Enterprise",
    t_enable_eap_sub: "Shows 802.1X login fields in WiFi settings",
    t_min_dots: "Enable minute dots",
    t_min_dots_sub: "Show 1–4 dots for minutes",
    t_disable_auto: "Disable AutoBrightness",
    t_disable_auto_sub: "Use fixed brightness (Max Brightness)",

    l_lux_live: "Ambient Lux (live)",
    l_out_live: "Output now (live)",
    l_boot_color: "Boot Color",
    l_boot_fade: "Boot Fade-In (ms)",
    l_conn_color: "Connect Color",
    l_conn_pulse: "Connect Pulse (ms)",
    l_ntp: "NTP Sync (hours)",
    l_wifi_to: "WiFi Timeout (sec)",
    l_step: "Animation Step (ms)",
    l_portal: "Portal Inactive (sec)",
    l_lux_int: "Lux Check (sec)",
    l_hyst: "Hysteresis Delta",
    l_lux_min: "Lux Min",
    l_lux_max: "Lux Max",
    l_night_thr: "Nightmode Lux Threshold",
    l_ota: "OTA Update",
    b_ota: "open in Browser",

    t_eap_head: "WPA2-Enterprise (802.1X)",
    l_eap_identity: "Identity (optional)",
    l_eap_user: "Username",
    l_eap_pass: "Password",
    t_eap_warn: "No certificate validation (insecure).",

    t_finish: "finish setup of",
    savehint: "your WordClock",
    b_save: "Save",

    // status/validation
    err_ssid_missing: "Wifi SSID is missing.",
    err_stored_unreachable: "Stored Wifi SSID is currently not reachable",
    err_pass_required: "Password required for this SSID.",
    err_eap_user_missing: "Enterprise username is missing.",
    err_eap_pass_required: "Enterprise password required for this SSID.",
    st_connecting: "Connecting...",
    st_connecting_switch: "Connecting... (network switching)",
    st_ok_switch: (msg) => `${msg || "WiFi connected"} (switching to runtime...)`,
    st_fail: (msg) => (msg || "Connection failed. Please check credentials."),
    st_timeout: "Connection failed (timeout). Please check credentials.",

    // placeholders
    ph_password: "Password",
    ph_unchanged: "(unchanged)",
    ph_eap_identity: "anonymous@realm (optional)",
    ph_eap_user: "user@realm",
  },

  de: {
    subtitle: "Einrichtung • Live-Vorschau",
    t_wifi: "WLAN EINSTELLUNGEN",
    l_ssid: "Netzwerk (SSID)",
    l_pass: "Passwort",
    b_scan: "Scannen",

    t_tz: "ZEITZONE",
    l_tz: "Zeitzone",
    tzmini: (local, sel) => `lokale Zeit: ${local} • ausgewählt: ${sel}`,

    t_themes: "DESIGNS",
    l_theme: "Design",
    l_min_color: "Minutenfarbe",
    l_hour_color: "Stundenfarbe",
    l_fade_mode: "Übergang",
    l_fade_time: "Übergangszeit:",
    fade0: "Aus",
    fade1: "2-Phasen",
    fade2: "Überblenden",
    fade3: "Wischen",

    t_bri: "HELLIGKEIT",
    l_max_bri: "Max. Helligkeit:",

    t_night_enable: "Nachtmodus aktivieren",
    t_night_sub: "Aktiviert bei wenig Licht im Zeitfenster",
    l_night_behavior: "Nachtmodus-Verhalten",
    opt_night_0: "Minutenpunkte aus",
    opt_night_1: "Nur UHR/HALB anzeigen",
    opt_night_2: "LEDs aus",
    l_night_start: "Start",
    l_night_end: "Ende",

    t_adv: "EXPERTEN EINSTELLUNGEN",
    t_enable_eap: "WPA2-Enterprise aktivieren",
    t_enable_eap_sub: "Zeigt 802.1X Login-Felder in den WLAN-Einstellungen",
    t_min_dots: "Minutenpunkte aktivieren",
    t_min_dots_sub: "Zeigt 1–4 Punkte für Minuten",
    t_disable_auto: "Auto-Helligkeit deaktivieren",
    t_disable_auto_sub: "Feste Helligkeit verwenden (Max. Helligkeit)",

    l_lux_live: "Umgebungs-Lux (live)",
    l_out_live: "Ausgabe jetzt (live)",
    l_boot_color: "Boot-Farbe",
    l_boot_fade: "Boot Einblenden (ms)",
    l_conn_color: "Verbindungsfarbe",
    l_conn_pulse: "Verbindung Puls (ms)",
    l_ntp: "NTP Sync (Stunden)",
    l_wifi_to: "WLAN Timeout (Sek.)",
    l_step: "Animationsschritt (ms)",
    l_portal: "Portal inaktiv (Sek.)",
    l_lux_int: "Lux-Check (Sek.)",
    l_hyst: "Hysterese-Delta",
    l_lux_min: "Lux Min",
    l_lux_max: "Lux Max",
    l_night_thr: "Nachtmodus Lux-Schwelle",
    l_ota: "OTA Update",
    b_ota: "im Browser öffnen",

    t_eap_head: "WPA2-Enterprise (802.1X)",
    l_eap_identity: "Identität (optional)",
    l_eap_user: "Benutzername",
    l_eap_pass: "Passwort",
    t_eap_warn: "Keine Zertifikatsprüfung (unsicher).",

    t_finish: "Setup abschließen für",
    savehint: "deine WortUhr",
    b_save: "Speichern",

    // status/validation
    err_ssid_missing: "WLAN-SSID fehlt.",
    err_stored_unreachable: "Gespeicherte WLAN-SSID ist aktuell nicht erreichbar",
    err_pass_required: "Passwort für diese SSID erforderlich.",
    err_eap_user_missing: "Enterprise-Benutzername fehlt.",
    err_eap_pass_required: "Enterprise-Passwort erforderlich.",
    st_connecting: "Verbinde...",
    st_connecting_switch: "Verbinde... (Netzwerkwechsel)",
    st_ok_switch: (msg) => `${msg || "WLAN verbunden"} (Wechsel in den Laufzeitmodus...)`,
    st_fail: (msg) => (msg || "Verbindung fehlgeschlagen. Bitte Zugangsdaten prüfen."),
    st_timeout: "Verbindung fehlgeschlagen (Timeout). Bitte Zugangsdaten prüfen.",

    // placeholders
    ph_password: "Passwort",
    ph_unchanged: "(unverändert)",
    ph_eap_identity: "anonymous@realm (optional)",
    ph_eap_user: "user@realm",
  }
};

let LANG = "de";
function T(){ return I18N[LANG] || I18N.en; }
function setTxt(id, v){ const el = $(id); if(el) el.textContent = v; }

function initLanguage(){
  const stored = localStorage.getItem("wc_lang");
  const preferDe = ((navigator.language || "").toLowerCase().startsWith("de"));
  LANG = (stored === "de" || stored === "en") ? stored : (preferDe ? "de" : "en");

  if(els.lang){
    els.lang.value = LANG;
    els.lang.addEventListener("change", ()=>{
      LANG = (els.lang.value === "de") ? "de" : "en";
      localStorage.setItem("wc_lang", LANG);
      applyLang();
    });
  }
  applyLang();
}

function applyLang(){
  const t = T();

  setTxt("subtitle", t.subtitle);

  setTxt("t_wifi", t.t_wifi);
  setTxt("l_ssid", t.l_ssid);
  setTxt("l_pass", t.l_pass);
  setTxt("b_scan", t.b_scan);

  setTxt("t_tz", t.t_tz);
  setTxt("l_tz", t.l_tz);

  setTxt("t_themes", t.t_themes);
  setTxt("l_theme", t.l_theme);
  setTxt("l_min_color", t.l_min_color);
  setTxt("l_hour_color", t.l_hour_color);
  setTxt("l_fade_mode", t.l_fade_mode);
  setTxt("l_fade_time", t.l_fade_time);

  setTxt("opt_fade_0", t.fade0);
  setTxt("opt_fade_1", t.fade1);
  setTxt("opt_fade_2", t.fade2);
  setTxt("opt_fade_3", t.fade3);

  setTxt("t_bri", t.t_bri);
  setTxt("l_max_bri", t.l_max_bri);

  setTxt("t_night_enable", t.t_night_enable);
  setTxt("t_night_sub", t.t_night_sub);
  setTxt("l_night_behavior", t.l_night_behavior);
  setTxt("opt_night_0", t.opt_night_0);
  setTxt("opt_night_1", t.opt_night_1);
  setTxt("opt_night_2", t.opt_night_2);
  setTxt("l_night_start", t.l_night_start);
  setTxt("l_night_end", t.l_night_end);

  setTxt("t_adv", t.t_adv);
  setTxt("t_enable_eap", t.t_enable_eap);
  setTxt("t_enable_eap_sub", t.t_enable_eap_sub);
  setTxt("t_min_dots", t.t_min_dots);
  setTxt("t_min_dots_sub", t.t_min_dots_sub);
  setTxt("t_disable_auto", t.t_disable_auto);
  setTxt("t_disable_auto_sub", t.t_disable_auto_sub);

  setTxt("l_lux_live", t.l_lux_live);
  setTxt("l_out_live", t.l_out_live);
  setTxt("l_boot_color", t.l_boot_color);
  setTxt("l_boot_fade", t.l_boot_fade);
  setTxt("l_conn_color", t.l_conn_color);
  setTxt("l_conn_pulse", t.l_conn_pulse);
  setTxt("l_ntp", t.l_ntp);
  setTxt("l_wifi_to", t.l_wifi_to);
  setTxt("l_step", t.l_step);
  setTxt("l_portal", t.l_portal);
  setTxt("l_lux_int", t.l_lux_int);
  setTxt("l_hyst", t.l_hyst);
  setTxt("l_lux_min", t.l_lux_min);
  setTxt("l_lux_max", t.l_lux_max);
  setTxt("l_night_thr", t.l_night_thr);
  setTxt("l_ota", t.l_ota);
  setTxt("b_ota", t.b_ota);

  setTxt("t_eap_head", t.t_eap_head);
  setTxt("l_eap_identity", t.l_eap_identity);
  setTxt("l_eap_user", t.l_eap_user);
  setTxt("l_eap_pass", t.l_eap_pass);
  setTxt("t_eap_warn", t.t_eap_warn);

  setTxt("t_finish", t.t_finish);
  setTxt("savehint", t.savehint);
  setTxt("b_save", t.b_save);

  // placeholders (EAP identity/user)
  if(els.eapIdentity) els.eapIdentity.placeholder = t.ph_eap_identity;
  if(els.eapUser)     els.eapUser.placeholder     = t.ph_eap_user;

  // update pw placeholders
  setPwPlaceholder(!!els.enableEnterprise.checked);

  // update tz mini line
  updateTzMini();

  // update theme meta text to current language
  const tid = detectThemeFromFields();
  if(tid){
    const theme = THEMES.find(x=>x.id===tid);
    if(theme) setThemeMeta(theme);
  } else {
    els.themeMeta.textContent = (LANG === "de") ? "Benutzerdefiniert" : "Custom";
  }
}

/* -------- Stored WiFi state (SSID only, never password) -------- */
let storedSsid = "";
let haveStoredCreds = false;
let storedSsidAvailable = false;

// Stored wifi mode + EAP meta (no passwords)
let storedWifiMode = 0; // 0=PSK, 1=Enterprise
let storedEapIdentity = "";
let storedEapUser = "";

/* -------- UI helpers -------- */
function setTextStatus(text, ok){
  els.savestatus.textContent = text;
  els.savestatus.className = 'status ' + (ok ? 'ok' : 'bad');
}
function setSaveState(state){
  const btn = els.saveBtn;
  btn.classList.remove('saving','ok','fail');
  if(state) btn.classList.add(state);
}
function clamp(n, lo, hi){ return Math.min(hi, Math.max(lo, n)); }
function pad2(n){ return String(n).padStart(2,'0'); }
function timeLocal(){ const d=new Date(); return pad2(d.getHours())+':'+pad2(d.getMinutes()); }
function tryBrowserTZ(){ try{ return Intl.DateTimeFormat().resolvedOptions().timeZone || null; }catch(e){ return null; } }
function timeInTZ(tz){
  try{
    return new Intl.DateTimeFormat('en-US',{timeZone:tz,hour:'2-digit',minute:'2-digit',hour12:false}).format(new Date());
  }catch(e){ return '—'; }
}
function updateTzMini(){
  const local = timeLocal();
  const sel = timeInTZ(els.tz.value);
  const t = T();
  els.tzmini.textContent = (t.tzmini ? t.tzmini(local, sel) : `detected local time: ${local} • selected: ${sel}`);
}
function updateBriLabel(){ els.briLabel.textContent = String(els.maxBrightness.value); }
function bumpBrightness(delta){
  els.maxBrightness.value = String(clamp(Number(els.maxBrightness.value) + delta, 1, 255));
  updateBriLabel();
  schedulePreview();
}
function updateFadeMsLabel(){ els.fadeMsLabel.textContent = els.fadeMs.value; }
function updateNightUI(){ els.nightFields.style.display = els.enableNightmode.checked ? 'block' : 'none'; }
function getDisableAutoBrightness(){ return !!els.disableAutoBrightness?.checked; }

/* -------- Password eye toggle -------- */
function wirePasswordEye(inputEl, btnEl){
  if(!inputEl || !btnEl) return;
  const setState = (show)=>{
    inputEl.type = show ? "text" : "password";
    btnEl.dataset.show = show ? "1" : "0";
    btnEl.setAttribute("aria-label", show ? "Hide password" : "Show password");
  };
  btnEl.addEventListener("click", ()=>{ setState(btnEl.dataset.show !== "1"); });
  setState(false);
}
wirePasswordEye(els.pass, els.passEye);
wirePasswordEye(els.eapPass, els.eapPassEye);

/* -------- Enterprise / PSK UI + placeholders -------- */
function isSameStoredSsid(ssidVal){ return haveStoredCreds && ssidVal === storedSsid; }

function setPwPlaceholder(modeEnterprise){
  const t = T();
  const ssidVal = (els.ssid.value || "");
  if(modeEnterprise){
    const wantUnchanged = isSameStoredSsid(ssidVal) && storedWifiMode === 1;
    els.eapPass.placeholder = wantUnchanged ? t.ph_unchanged : t.ph_password;
    els.eapPassUnchanged.value = wantUnchanged ? "1" : "0";
    els.eapPass.value = "";
  }else{
    const wantUnchanged = isSameStoredSsid(ssidVal) && storedWifiMode === 0;
    els.pass.placeholder = wantUnchanged ? t.ph_unchanged : t.ph_password;
    els.passUnchanged.value = wantUnchanged ? "1" : "0";
    els.pass.value = "";
  }
}

function updateEnterpriseUI(scrollToWifi){
  const en = !!els.enableEnterprise.checked;
  els.enterpriseBlock.style.display = en ? 'block' : 'none';
  els.pskBlock.style.display = en ? 'none' : 'block';

  setPwPlaceholder(en);

  if(scrollToWifi){
    document.querySelector('.wrap')?.scrollTo?.({top:0, behavior:'smooth'});
  }
}

/* -------- THEMES -------- */
const THEMES = [
  { id:"ember",   name:"Ember",  colorM:"#FFFF00", colorH:"#FF0000", fadeMode:"1", fadeMs:500 },
  { id:"nordic",  name:"Nordic", colorM:"#00FFFF", colorH:"#0000FF", fadeMode:"2", fadeMs:300 },
  { id:"neon",    name:"Neon",   colorM:"#00FF00", colorH:"#FF00FF", fadeMode:"3", fadeMs:500 },
  { id:"studio",  name:"Studio", colorM:"#FFFFFF", colorH:"#FFFFFF", fadeMode:"0", fadeMs:800 },
];

function setSwatches(mHex, hHex){
  els.swM.style.background = mHex;
  els.swH.style.background = hHex;
}
function setThemeMeta(t){
  const tt = T();
  const fm = (t.fadeMode==="0") ? tt.fade0
           : (t.fadeMode==="1") ? tt.fade1
           : (t.fadeMode==="2") ? tt.fade2
           : tt.fade3;
  // short, UI-friendly meta
  els.themeMeta.textContent = `${fm} • ${t.fadeMs}ms`;
}
function populateThemes(){
  els.themeSelect.innerHTML = "";
  for(const t of THEMES){
    const o = document.createElement('option');
    o.value = t.id;
    o.textContent = t.name;
    els.themeSelect.appendChild(o);
  }
}
function applyTheme(themeId, doPreview=true){
  const t = THEMES.find(x=>x.id===themeId);
  if(!t) return;

  els.colorM.value = t.colorM;
  els.colorH.value = t.colorH;
  els.fadeMode.value = t.fadeMode;
  els.fadeMs.value = String(t.fadeMs);
  updateFadeMsLabel();

  setSwatches(t.colorM, t.colorH);
  setThemeMeta(t);
  if(doPreview) schedulePreview();
}
function detectThemeFromFields(){
  const cm = els.colorM.value.toUpperCase();
  const ch = els.colorH.value.toUpperCase();
  const fm = els.fadeMode.value;
  const fms= Number(els.fadeMs.value);
  const found = THEMES.find(t =>
    t.colorM.toUpperCase()===cm &&
    t.colorH.toUpperCase()===ch &&
    t.fadeMode===fm &&
    Number(t.fadeMs)===fms
  );
  return found ? found.id : null;
}

/* -------- Live Preview (debounced) -------- */
let previewTimer = null;
function schedulePreview(){
  if(previewTimer) clearTimeout(previewTimer);
  previewTimer = setTimeout(sendPreview, 150);
}

async function sendPreview(){
  const body = new URLSearchParams();
  body.set('tzName', els.tz.value);
  body.set('maxBrightness', els.maxBrightness.value);
  body.set('disableAutoBrightness', getDisableAutoBrightness() ? '1' : '0');

  body.set('fadeMode', els.fadeMode.value);
  body.set('fadeMs', els.fadeMs.value);

  body.set('colorM', els.colorM.value);
  body.set('colorH', els.colorH.value);

  body.set('enableMinuteDots', els.enableMinuteDots.checked ? '1' : '0');
  body.set('stepMs', els.stepMs.value);

  body.set('luxCheckIntervalSec', els.luxCheckIntervalSec.value);
  body.set('hysteresisDelta', els.hysteresisDelta.value);
  body.set('portalInactiveMaxSec', els.portalInactiveMaxSec.value);

  body.set('enableNightmode', els.enableNightmode.checked ? '1' : '0');
  body.set('nightBehavior', els.nightBehavior.value);
  body.set('nightStart', els.nightStart.value);
  body.set('nightEnd', els.nightEnd.value);
  if(els.nightLuxThreshold) body.set('nightLuxThreshold', els.nightLuxThreshold.value);

  body.set('luxMin', els.luxMin.value);
  body.set('luxMax', els.luxMax.value);

  await fetch('/preview', {method:'POST', body});
  pollLux();
}

/* -------- WiFi scan -------- */
async function scan(){
  els.scanBtn?.classList.remove('scanned');
  els.scanBtn?.classList.add('scanning');

  storedSsidAvailable = false;

  const r = await fetch('/scan');
  const j = await r.json();

  els.ssid.innerHTML = '';

  (j.networks || []).forEach(n=>{
    const o=document.createElement('option');
    o.value=n.ssid;
    o.textContent = `${n.ssid} (${n.rssi} dBm)`;
    els.ssid.appendChild(o);
  });

  const stored = (j.currentSsid || "").trim();
  if (stored.length > 0) {
    for(const o of els.ssid.options){ if(o.value === stored) storedSsidAvailable = true; }

    if (!storedSsidAvailable) {
      const o = document.createElement('option');
      o.value = stored;
      o.textContent = `${stored} (${(LANG==="de") ? "gespeichert/nicht gefunden" : "stored/not found"})`;
      els.ssid.insertBefore(o, els.ssid.firstChild);
    }
    els.ssid.value = stored;
  } else {
    if (els.ssid.options.length > 0) els.ssid.selectedIndex = 0;
  }

  els.scanBtn?.classList.remove('scanning');
  els.scanBtn?.classList.add('scanned');

  updateEnterpriseUI(false);
}

/* -------- Live Lux polling -------- */
async function pollLux(){
  try{
    const r = await fetch('/lux', {cache:'no-store'});
    const j = await r.json();

    if(j.luxOk){
      els.luxLive.textContent = `raw ${Number(j.lux).toFixed(2)}lx`;
      els.briLive.textContent = `${j.effectiveBrightness} / ${j.maxBrightness}  ${j.autoEnabled ? (LANG==="de" ? "(auto)" : "(auto)") : (LANG==="de" ? "(manuell)" : "(manual)")}`;

      els.luxNote.textContent  = j.autoEnabled
        ? (LANG==="de" ? "Auto-Helligkeit Vorschau nutzt die Kurve (Lux → Ausgabe)." : "AutoBrightness preview uses the curve (lux → output).")
        : (LANG==="de" ? "Auto-Helligkeit deaktiviert: Vorschau nutzt Max. Helligkeit direkt." : "AutoBrightness disabled: preview uses MaxBrightness directly.");
    } else {
      els.luxLive.textContent = (LANG==="de") ? "Sensor nicht verfügbar" : "sensor unavailable";
      els.briLive.textContent = `${j.effectiveBrightness} / ${j.maxBrightness}`;
      els.luxNote.textContent  = (LANG==="de")
        ? "Lux-Sensor nicht verfügbar. Vorschau nutzt Max. Helligkeit."
        : "Lux sensor not available. Preview falls back to MaxBrightness.";
    }
  }catch(e){
    // ignore
  }
}

/* -------- Load cfg -------- */
async function loadCfg(){
  populateThemes();
  setSaveState('fail');

  const r = await fetch('/cfg');
  const j = await r.json();

  els.chip.textContent = j.chip;

  const browserTz = tryBrowserTZ();
  const storedTz = (j.tzName && j.tzName.length) ? j.tzName : null;
  const desired = storedTz || browserTz || 'Europe/Berlin';

  let found = false;
  for(const o of els.tz.options){ if(o.value===desired){ found=true; break; } }
  els.tz.value = found ? desired : 'Europe/Berlin';
  updateTzMini();

  els.maxBrightness.value = (j.maxBrightness !== undefined && j.maxBrightness !== null) ? Number(j.maxBrightness) : 40;
  updateBriLabel();

  if (j.disableAutoBrightness !== undefined && j.disableAutoBrightness !== null) {
    els.disableAutoBrightness.checked = !!j.disableAutoBrightness;
  } else {
    els.disableAutoBrightness.checked = !(!!j.enableAutoBrightness);
  }

  els.fadeMode.value = String(j.fadeMode ?? "1");
  els.fadeMs.value = (typeof j.fadeMs === 'number') ? j.fadeMs : 500;
  updateFadeMsLabel();

  els.colorM.value = '#'+j.colorM;
  els.colorH.value = '#'+j.colorH;
  setSwatches(els.colorM.value, els.colorH.value);

  els.enableNightmode.checked = !!j.enableNightmode;
  els.nightBehavior.value = String(j.nightBehavior ?? "0");
  els.nightStart.value = (j.nightStart || "22:00");
  els.nightEnd.value   = (j.nightEnd || "07:00");
  els.nightLuxThreshold.value = (j.nightLuxThreshold !== undefined && j.nightLuxThreshold !== null) ? Number(j.nightLuxThreshold) : 5.0;
  updateNightUI();

  // Advanced
  els.bootColor.value = '#'+j.bootColor;
  els.bootFadeMs.value = j.bootFadeMs;

  els.connectColor.value = '#'+j.connectColor;
  els.connectPulseMs.value = j.connectPulseMs;

  els.ntpSyncHours.value = j.ntpSyncHours;
  els.connectionTimeoutSec.value = j.connectionTimeoutSec;

  els.enableMinuteDots.checked = !!j.enableMinuteDots;
  els.stepMs.value = j.stepMs;

  if (j.luxCheckIntervalSec !== undefined) els.luxCheckIntervalSec.value = j.luxCheckIntervalSec;
  if (j.portalInactiveMaxSec !== undefined) els.portalInactiveMaxSec.value = j.portalInactiveMaxSec;
  if (j.hysteresisDelta !== undefined) els.hysteresisDelta.value = j.hysteresisDelta;

  if (j.luxMin !== undefined) els.luxMin.value = Number(j.luxMin);
  else els.luxMin.value = 2.0;

  if (j.luxMax !== undefined) els.luxMax.value = Number(j.luxMax);
  else els.luxMax.value = 800.0;

  const tid = detectThemeFromFields() || "ember";
  els.themeSelect.value = tid;
  const t = THEMES.find(x=>x.id===tid);
  els.themeMeta.textContent = t ? (setThemeMeta(t), els.themeMeta.textContent) : ((LANG==="de") ? "Benutzerdefiniert" : "Custom");

  storedSsid = j.currentSsid || "";
  haveStoredCreds = !!j.currentSsid;

  storedWifiMode = Number(j.wifiMode ?? 0);
  storedEapIdentity = j.eapIdentity || "";
  storedEapUser     = j.eapUser || "";

  els.eapIdentity.value = storedEapIdentity;
  els.eapUser.value     = storedEapUser;

  els.enableEnterprise.checked = (storedWifiMode === 1);

  await scan();
  if(storedSsid) els.ssid.value = storedSsid;

  updateEnterpriseUI(false);
  hookPreview();
  await sendPreview();

  pollLux();
}

/* -------- Preview wiring -------- */
function handleFieldChange(id){
  if(id==='tz') updateTzMini();
  else if(id==='fadeMs') updateFadeMsLabel();
  else if(id==='maxBrightness') updateBriLabel();
  else if(id==='enableNightmode') updateNightUI();

  if(id==='colorM' || id==='colorH' || id==='fadeMode' || id==='fadeMs'){
    setSwatches(els.colorM.value, els.colorH.value);
    const tid = detectThemeFromFields();
    if(tid){
      els.themeSelect.value = tid;
      const t = THEMES.find(x=>x.id===tid);
      if(t) setThemeMeta(t);
    } else {
      els.themeMeta.textContent = (LANG==="de") ? "Benutzerdefiniert" : "Custom";
    }
  }

  schedulePreview();
}

function hookPreview(){
  const ids = [
    'tz','maxBrightness',
    'fadeMode','fadeMs','colorM','colorH',
    'enableMinuteDots','stepMs',
    'luxCheckIntervalSec','hysteresisDelta','portalInactiveMaxSec',
    'enableNightmode','nightBehavior','nightStart','nightEnd',
    'disableAutoBrightness','nightLuxThreshold','luxMin','luxMax'
  ];

  ids.forEach(id=>{
    const el = $(id);
    if(!el) return;
    const h = ()=>handleFieldChange(id);
    el.addEventListener('input', h);
    el.addEventListener('change', h);
  });

  els.briDown?.addEventListener('click', ()=> bumpBrightness(-1));
  els.briUp?.addEventListener('click', ()=> bumpBrightness(+1));

  els.themeSelect?.addEventListener('change', (e)=> applyTheme(e.target.value, true));

  els.enableNightmode?.addEventListener('change', ()=>{ updateNightUI(); schedulePreview(); });

  els.enableEnterprise?.addEventListener('change', ()=> updateEnterpriseUI(true));

  els.ssid?.addEventListener('change', ()=>{
    if(els.passEye?.dataset) els.passEye.dataset.show = "0";
    els.pass.type = "password";
    setPwPlaceholder(!!els.enableEnterprise.checked);
  });

  els.eapPass?.addEventListener('input', ()=>{
    const v = els.eapPass.value || "";
    els.eapPassUnchanged.value = (v.length === 0) ? "1" : "0";
    if (v.length === 0) els.eapPass.placeholder = T().ph_unchanged;
  });

  els.pass?.addEventListener('input', ()=>{
    const v = els.pass.value || "";
    els.passUnchanged.value = (v.length === 0) ? "1" : "0";
    if (v.length === 0) els.pass.placeholder = T().ph_unchanged;
  });
}

/* -------- Save flow -------- */
async function save(){
  const t = T();

  const ssidVal = els.ssid.value || "";
  const enterpriseOn = !!els.enableEnterprise.checked;

  const passVal = (els.pass?.value || "");
  const eapIdentity = els.eapIdentity.value || "";
  const eapUser = els.eapUser.value || "";
  const eapPassVal = (els.eapPass?.value || "");

  if(!ssidVal){
    setTextStatus(t.err_ssid_missing, false);
    return;
  }
  if(ssidVal === storedSsid && !storedSsidAvailable) {
    setTextStatus(t.err_stored_unreachable, false);
    return;
  }

  if(!enterpriseOn){
    const wantUnchanged = (isSameStoredSsid(ssidVal) && storedWifiMode === 0 && passVal.length === 0);
    if(!wantUnchanged && passVal.length === 0){
      setTextStatus(t.err_pass_required, false);
      return;
    }
  } else {
    if (!eapUser.length) {
      setTextStatus(t.err_eap_user_missing, false);
      return;
    }
    const wantUnchangedEap = (isSameStoredSsid(ssidVal) && storedWifiMode === 1 && eapPassVal.length === 0);
    if(!wantUnchangedEap && eapPassVal.length === 0){
      setTextStatus(t.err_eap_pass_required, false);
      return;
    }
  }

  const body = new URLSearchParams();
  body.set('ssid', ssidVal);
  body.set('wifiMode', enterpriseOn ? '1' : '0');

  const wantUnchanged = (isSameStoredSsid(ssidVal) && storedWifiMode === 0 && passVal.length === 0);
  body.set('pass', (!enterpriseOn && !wantUnchanged) ? passVal : "");
  body.set('passUnchanged', (!enterpriseOn && wantUnchanged) ? "1" : "0");

  const wantUnchangedEap = (isSameStoredSsid(ssidVal) && storedWifiMode === 1 && eapPassVal.length === 0);
  body.set('eapIdentity', enterpriseOn ? eapIdentity : "");
  body.set('eapUser', enterpriseOn ? eapUser : "");
  body.set('eapPass', (enterpriseOn && !wantUnchangedEap) ? eapPassVal : "");
  body.set('eapPassUnchanged', (enterpriseOn && wantUnchangedEap) ? "1" : "0");

  body.set('tzName', els.tz.value);
  body.set('maxBrightness', els.maxBrightness.value);
  body.set('disableAutoBrightness', getDisableAutoBrightness() ? '1' : '0');

  body.set('fadeMode', els.fadeMode.value);
  body.set('fadeMs', els.fadeMs.value);

  body.set('colorM', els.colorM.value);
  body.set('colorH', els.colorH.value);

  body.set('enableNightmode', els.enableNightmode.checked ? '1' : '0');
  body.set('nightBehavior', els.nightBehavior.value);
  body.set('nightStart', els.nightStart.value);
  body.set('nightEnd', els.nightEnd.value);
  body.set('nightLuxThreshold', els.nightLuxThreshold.value);

  body.set('bootColor', els.bootColor.value);
  body.set('bootFadeMs', els.bootFadeMs.value);

  body.set('connectColor', els.connectColor.value);
  body.set('connectPulseMs', els.connectPulseMs.value);

  body.set('ntpSyncHours', els.ntpSyncHours.value);
  body.set('connectionTimeoutSec', els.connectionTimeoutSec.value);

  body.set('enableMinuteDots', els.enableMinuteDots.checked ? '1' : '0');
  body.set('stepMs', els.stepMs.value);

  body.set('luxCheckIntervalSec', els.luxCheckIntervalSec.value);
  body.set('hysteresisDelta', els.hysteresisDelta.value);
  body.set('portalInactiveMaxSec', els.portalInactiveMaxSec.value);

  body.set('luxMin', els.luxMin.value);
  body.set('luxMax', els.luxMax.value);

  setSaveState('saving');
  setTextStatus(t.st_connecting, true);

  const ctrl = new AbortController();
  const to = setTimeout(()=> ctrl.abort(), 3000);

  try {
    const r = await fetch('/save', { method:'POST', body, signal: ctrl.signal });
    clearTimeout(to);
    const j = await r.json();

    if (!j?.ok) {
      setSaveState('fail');
      setTextStatus(j?.msg || (LANG==="de" ? "Speichern fehlgeschlagen." : "Save failed."), false);
      return;
    }
    await pollStatusAndHandle();
  } catch (e) {
    clearTimeout(to);
    setSaveState('saving');
    setTextStatus(t.st_connecting_switch, true);
    await pollStatusAndHandle();
  }
}

async function pollStatusAndHandle(){
  const t = T();
  const start = Date.now();
  const hardTimeoutMs = (Number(els.connectionTimeoutSec.value || 15) + 8) * 1000;

  while (Date.now() - start < hardTimeoutMs) {
    try {
      const ctrl = new AbortController();
      const to = setTimeout(()=> ctrl.abort(), 1200);

      const r = await fetch('/status', { signal: ctrl.signal, cache:'no-store' });
      clearTimeout(to);

      const j = await r.json();

      if (j.state === 'ok') {
        setSaveState('ok');
        setTextStatus(t.st_ok_switch(j.msg), true);
        return;
      }
      if (j.state === 'fail') {
        setSaveState('fail');
        setTextStatus(t.st_fail(j.msg), false);
        return;
      }

      setSaveState('saving');
      setTextStatus(j.msg || t.st_connecting, true);
    } catch (e) {
      setSaveState('saving');
      setTextStatus(t.st_connecting_switch, true);
    }
    await new Promise(res => setTimeout(res, 500));
  }

  setSaveState('fail');
  setTextStatus(t.st_timeout, false);
}

function openOta(){
  const url = location.origin + "/update";
  window.open(url, "_blank");
}

/* -------- init -------- */
initLanguage();
loadCfg();

// Keepalive
setInterval(()=>{ if(document.visibilityState === 'visible'){ fetch('/ping').catch(()=>{}); } }, 15000);
// Lux poll (1Hz) when visible
setInterval(()=>{ if(document.visibilityState === 'visible'){ pollLux(); } }, 1000);
</script>
</body>
</html>)HTML";

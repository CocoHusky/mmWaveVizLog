/*
 * MR60BHA2 Sensor VisLog
 *
 * This single Arduino sketch provides:
 *   - MR60BHA2 heart, breathing, range, presence, and raw phase readings
 *   - BH1750 ambient-light readings over I2C
 *   - WS2812 RGB control and sensor threshold rules
 *   - A compact dashboard served directly by the XIAO ESP32-C6
 *
 * Connect to WiFi "mmWaveVisLog-MR60BHA2" (password "wirelessphysiology") and open:
 *   http://192.168.4.1/
 */

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <HardwareSerial.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Wire.h>
#include "esp32-hal-rgb-led.h"
#include "Seeed_Arduino_mmWave.h"

static constexpr int RADAR_RX_PIN = 17;
static constexpr int RADAR_TX_PIN = 16;
static constexpr int STATUS_LED_PIN = 1;
static constexpr int I2C_SDA_PIN = 22;
static constexpr int I2C_SCL_PIN = 23;
static constexpr uint8_t AMBIENT_LIGHT_I2C_ADDRESS = 0x23;
static constexpr float MAX_VALID_RADAR_RANGE_M = 6.0f;
static constexpr uint8_t MAX_TRACKED_TARGETS = MAX_TARGET_NUM;

static const char *WIFI_AP_SSID = "mmWaveVisLog-MR60BHA2";
static const char *WIFI_AP_PASSWORD = "wirelessphysiology";
static const char *OTA_HOSTNAME = "mmWaveVisLog-MR60BHA2-OTA";
static const char *OTA_PASSWORD = "wp-ota";
static const char *VisLog_FW_VERSION = "2.1.4";

HardwareSerial radarSerial(1);
SEEED_MR60BHA2 radar;
WebServer server(80);

struct RadarTarget {
  float x = NAN;
  float y = NAN;
  float distance = NAN;
  float angle = NAN;
  float speed = NAN;
  int32_t dopIndex = 0;
  int32_t clusterIndex = 0;
};

struct SensorSnapshot {
  float heartRate = NAN;
  float breathRate = NAN;
  float distance = NAN;
  float rawDistance = NAN;
  float totalPhase = NAN;
  float breathPhase = NAN;
  float heartPhase = NAN;
  float light = NAN;
  float targetX = NAN;
  float targetY = NAN;
  float targetDistance = NAN;
  float targetAngle = NAN;
  float targetSpeed = NAN;
  RadarTarget targets[MAX_TRACKED_TARGETS];
  uint8_t targetCount = 0;
  bool targetValid = false;
  bool targetInfoValid = false;
  bool pointCloudValid = false;
  bool presence = false;
  bool presenceValid = false;
  String presenceSource = "waiting";
  String targetSource = "waiting";
  uint32_t firmwareRaw = 0;
  uint8_t firmwareProject = 0;
  uint8_t firmwareMajor = 0;
  uint8_t firmwareSub = 0;
  uint8_t firmwareModified = 0;
  bool firmwareValid = false;
  uint8_t ledR = 0;
  uint8_t ledG = 0;
  uint8_t ledB = 0;
  uint32_t frame = 0;
  uint32_t lastRadarMs = 0;
  uint32_t lastPresenceMs = 0;
  uint32_t lastTargetMs = 0;
  uint32_t lastDetectionSignalMs = 0;
  uint32_t lastFirmwareMs = 0;
};

enum class LedMode : uint8_t { Off, Solid, Pulse, Rule };

struct StatusLedConfig {
  LedMode mode = LedMode::Solid;
  float brightness = 0.35f;
  uint8_t r = 40;
  uint8_t g = 190;
  uint8_t b = 100;
  String metric = "target_speed";
  float low = -0.05f;
  float high = 0.05f;
  uint8_t lowR = 255, lowG = 0, lowB = 0;
  uint8_t okR = 0, okG = 0, okB = 0;
  uint8_t highR = 0, highG = 80, highB = 255;
};

SensorSnapshot sensorState;
StatusLedConfig statusLed;
bool ambientLightReady = false;
uint32_t lastAmbientLightReadMs = 0;
uint32_t lastSerialReportMs = 0;

const char UI_HTML[] PROGMEM = R"UI(
<!doctype html><html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>MR60BHA2 Sensor VisLog</title>
<style>
:root{color-scheme:dark;--bg:#0d1011;--panel:#171b1d;--line:#30373b;--text:#edf1ef;--muted:#94a09d;--green:#39c678;--yellow:#e3b341;--red:#ef5f5f;--cyan:#54bfe3}.presence-dot{display:inline-block;width:9px;height:9px;border-radius:50%;background:var(--red);box-shadow:0 0 0 1px rgba(0,0,0,.25) inset}.presence-dot.live{background:var(--green);box-shadow:0 0 10px #39c678}*{box-sizing:border-box}body{margin:0;background:#0d1011;color:var(--text);font:13px system-ui,-apple-system,sans-serif}.app{width:min(1180px,calc(100% - 20px));margin:auto;padding:12px 0 22px}header{display:flex;align-items:center;justify-content:space-between;gap:12px;padding:8px 0 12px;border-bottom:1px solid var(--line)}h1{font-size:19px;margin:0;letter-spacing:0}.status{display:flex;align-items:center;gap:7px;color:var(--muted)}.dot{width:8px;height:8px;border-radius:50%;background:var(--red)}.dot.live{background:var(--green);box-shadow:0 0 10px #39c678}.controls{display:grid;grid-template-columns:1fr 1fr auto;gap:8px;align-items:end;padding:10px 0}label{display:grid;gap:4px;color:var(--muted);font-size:11px}select,input,button{height:32px;border:1px solid var(--line);border-radius:6px;background:#121517;color:var(--text);padding:0 8px;font:inherit}button{cursor:pointer;background:#252b2e}.metrics{display:grid;gap:6px}.metric{display:grid;grid-template-columns:185px minmax(0,1fr);min-height:128px;border:1px solid var(--line);border-radius:7px;background:var(--panel);overflow:hidden;position:relative}.metric.collapsed{grid-template-columns:1fr;min-height:34px}.metric.collapsed .chart,.metric.collapsed .note,.metric.collapsed .value,.metric.collapsed .subvalue{display:none}.metric.collapsed .reading{border-right:none;padding:7px 12px 7px 24px;min-height:34px;justify-content:center}.reading{padding:12px 12px 12px 24px;border-right:1px solid var(--line);display:flex;flex-direction:column;justify-content:center;position:relative;min-height:128px}.toggle{position:absolute;left:7px;top:7px;width:12px;height:12px;padding:0;border:none;border-radius:999px;background:transparent;color:var(--muted);font-size:12px;line-height:12px;cursor:pointer}.toggle:hover{color:var(--text)}.name{color:var(--muted);font-size:11px}.value{font-size:28px;font-weight:750;line-height:1.1;margin:4px 0 0}.value.target-value{display:grid;gap:4px;font-size:24px;font-weight:750;line-height:1.08}.value.target-value .unit{display:inline;font-size:11px;color:var(--muted);font-weight:500}.value.target-value .target-line{display:block;white-space:nowrap}.subvalue{font-size:11px;line-height:1.2;color:#aab4b1;margin-top:4px}.unit{font-size:11px;color:var(--muted);font-weight:500}.note{color:#6f7b78;font-size:10px;line-height:1.3;margin-top:3px}.chart{position:relative;height:126px}.chart canvas{width:100%;height:100%;display:block}.radar{display:grid;grid-template-columns:minmax(340px,360px) minmax(250px,280px) minmax(0,1fr);margin-top:6px;border:1px solid var(--line);border-radius:7px;background:var(--panel);overflow:hidden}.radar-info,.target-stack{padding:12px 14px}.radar-info{border-right:1px solid var(--line)}.target-stack{border-right:1px solid var(--line);overflow:auto}.radar-info h2{font-size:12px;margin:0}.radar-top{display:flex;align-items:center;justify-content:space-between;gap:10px;margin-bottom:10px}.radar-top strong{font-size:13px}.target-table{display:grid;grid-template-columns:1fr;gap:6px}.target-row{display:grid;grid-template-columns:96px minmax(0,1fr);gap:8px;align-items:center;padding:5px 0;border-bottom:1px solid rgba(255,255,255,.06)}.target-row:last-child{border-bottom:none}.target-row span{color:var(--muted);font-size:10px}.target-row strong{display:block;color:var(--text);font-size:12px;font-weight:600;line-height:1.25}.target-list{display:grid;gap:8px;align-content:start;min-width:0}.target-card,.target-empty{border:1px solid rgba(255,255,255,.08);border-radius:7px;background:#131619;padding:9px 10px}.target-empty{color:#aab4b1;font:10px/1.4 ui-monospace,monospace}.target-card{display:grid;grid-template-columns:minmax(0,1fr) auto;gap:10px;align-items:center}.target-card-title{font-size:14px;color:#edf1ef;font-weight:700;justify-self:end;white-space:nowrap}.target-card-body{display:grid;gap:2px;color:#dbe0de;font:10px/1.35 ui-monospace,monospace;min-width:0}.target-card-body span{display:block;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}.radar-canvas{height:240px}.radar-canvas canvas{width:100%;height:100%;display:block}.tools{display:grid;grid-template-columns:1fr 1.3fr;gap:6px;margin-top:8px}.box{border:1px solid var(--line);border-radius:7px;background:var(--panel);padding:10px}.box h2{font-size:12px;margin:0 0 8px}.led-grid{display:grid;grid-template-columns:84px 1fr 1fr 1fr auto;gap:7px;align-items:end}.rule-grid{display:grid;grid-template-columns:1.2fr 1fr 1fr 42px 42px 42px auto;gap:7px;align-items:end}.color{padding:2px}.log-controls{display:grid;grid-template-columns:minmax(0,1fr) auto auto;gap:7px;align-items:center;margin-bottom:8px}.log-controls input{width:100%}.log-controls button{white-space:nowrap}.log{height:86px;overflow:auto;background:#090b0c;border:1px solid var(--line);border-radius:6px;padding:7px;color:#aab4b1;font:10px/1.4 ui-monospace,monospace;white-space:pre-wrap}.footer{display:flex;justify-content:flex-end;gap:8px;color:#6f7b78;font-size:10px;margin-top:10px}.small{font-size:10px;color:var(--muted)}@media(max-width:720px){header{align-items:flex-start}.controls,.tools{grid-template-columns:1fr}.metric{grid-template-columns:130px minmax(0,1fr)}.radar{grid-template-columns:1fr}.reading,.radar-info,.target-stack{padding:9px}.metric.collapsed{grid-template-columns:1fr}.value{font-size:22px}.radar-canvas{height:210px}.target-row{grid-template-columns:84px minmax(0,1fr)}.led-grid,.rule-grid{grid-template-columns:1fr 1fr}.rule-grid button,.led-grid button{grid-column:1/-1}}
</style></head><body><main class="app">
<header><div><h1>MR60BHA2 Sensor VisLog</h1><div class="small">Live radar, bio-signal, target tracking, light, LED, and logging tools</div></div><div class="status"><span id="dot" class="dot"></span><span id="connection">Connecting</span></div></header>
<section class="controls"><label>Plot Window<select id="window"><option value="18000">18 seconds</option><option value="36000" selected>36 seconds</option><option value="72000">72 seconds</option></select></label><label>Noise / Stability Band (12-point rolling)<select id="band"><option value="0">Off</option><option value="1">+/- 1 sigma</option><option value="2" selected>+/- 2 sigma</option><option value="3">+/- 3 sigma</option></select></label><button id="clear">Reset plots</button></section>
<section class="radar"><div class="radar-info"><div class="radar-top"><h2>Radar Target Tracking</h2></div><div class="target-table"><div class="target-row"><span>Presence</span><strong><span id="presenceLight" class="presence-dot"></span> <span id="presenceText">--</span></strong></div><div class="target-row"><span>Target Count</span><strong id="targetCount">--</strong></div><div class="target-row"><span>Frame Rate</span><strong id="frameRate">--</strong></div><div class="target-row"><span>Primary Range</span><strong id="targetRange">--</strong></div><div class="target-row"><span>Primary Angle</span><strong id="targetAngle">--</strong></div><div class="target-row"><span>Primary Speed</span><strong id="targetSpeed">--</strong></div><div class="target-row"><span>FW</span><strong id="firmwareVersion">--</strong></div></div></div><div class="target-stack"><div id="targetList" class="target-list"><div class="target-empty">No target records</div></div></div><div class="radar-canvas"><canvas id="radarCanvas"></canvas></div></section>
<section id="metrics" class="metrics" style="margin-top:6px"></section>
<section class="tools"><div class="box"><h2>Status LED Control</h2><div class="led-grid"><label>Mode<select id="ledMode"><option value="solid">Solid</option><option value="pulse">Breathing pulse</option><option value="off">Off</option><option value="rule">Threshold rule</option></select></label><label>Color<input id="ledColor" class="color" type="color" value="#39c678"></label><label>Brightness<input id="brightness" type="range" min="0" max="100" value="35"></label><div></div><button id="sendLed">Apply</button></div></div>
<div class="box"><h2>Sensor-to-LED Rule</h2><div class="rule-grid"><label>Measurement<select id="ruleMetric"></select></label><label>Low<input id="ruleLow" type="number" step="0.01" value="-0.05"></label><label>High<input id="ruleHigh" type="number" step="0.01" value="0.05"></label><label>Below<input id="lowColor" class="color" type="color" value="#ff0000"></label><label>In band<input id="okColor" class="color" type="color" value="#000000"></label><label>Above<input id="highColor" class="color" type="color" value="#0050ff"></label><button id="sendRule">Apply</button></div></div></section>
<section class="box" style="margin-top:6px"><h2>Session Logger</h2><div class="log-controls"><input id="logName" type="text" maxlength="32" placeholder="session name"><button id="logToggle">Start recording</button><button id="logExport JSON">Export JSON</button></div><div id="log" class="log"></div></section>
<footer class="footer"><span>MR60BHA2 Sensor VisLog</span><span>2026-06-23</span><span>License: personal use</span></footer>
</main><script>
const defs=[
{k:'heart_rate',n:'Heart Rate',u:'bpm',c:'#e56a9f',d:1,note:'Radar-estimated heart rate'},
{k:'heart_phase',n:'Heart Motion Phase',u:'raw',c:'#a88cff',d:2,note:'Heartbeat-filtered motion phase'},
{k:'breath_rate',n:'Breathing Rate',u:'rpm',c:'#39c678',d:1,note:'Radar-estimated breathing rate'},
{k:'breath_phase',n:'Breathing Motion Phase',u:'raw',c:'#6aa7ff',d:2,note:'Breathing-filtered motion phase'},
{k:'total_phase',n:'Raw Motion Phase',u:'raw',c:'#54bfe3',d:2,note:'Unfiltered radar motion phase'},
{k:'distance',n:'Primary Range',u:'m',c:'#e3b341',d:2,note:'Best single range estimate'},
{k:'target_distance',n:'Multi-Target Range',u:'m',c:'#f28e2b',d:2,note:'Per-target range derived from X/Y',targets:true,tk:'distance'},
{k:'target_angle',n:'Multi-Target Angle',u:'deg',c:'#edc948',d:1,note:'Per-target angle derived from X/Y',targets:true,tk:'angle'},
{k:'target_speed',n:'Multi-Target Speed',u:'m/s',c:'#b07aa1',d:2,note:'Per-target speed from Doppler index',targets:true,tk:'speed'},
{k:'light',n:'Ambient Light',u:'lux',c:'#f5d76e',d:1,note:'BH1750 light sensor over I2C'}];
let windowMs=36000,mult=2,fail=0;const targetColors=['#ef5f5f','#e3b341','#39c678'];const hist=Object.fromEntries(defs.map(d=>[d.k,d.targets?[[],[],[]]:[]]));
const $=id=>document.getElementById(id);function log(s){$('log').textContent=new Date().toLocaleTimeString()+'  '+s+'\n'+$('log').textContent}
function fmt(v,d){return Number.isFinite(v)?v.toFixed(d):'--'}
function logStamp(d=new Date()){const z=n=>String(n).padStart(2,'0');return `${d.getFullYear()}-${z(d.getMonth()+1)}-${z(d.getDate())}_${z(d.getHours())}-${z(d.getMinutes())}-${z(d.getSeconds())}`}
function cleanName(s){return (s||'mr60bha2').trim().replace(/[^a-z0-9_-]+/gi,'-').replace(/^-+|-+$/g,'').slice(0,32)||'mr60bha2'}
let logActive=false,logRows=[],lastFrame=null,lastFrameAt=0,lastFps=null;
function setup(){ $('metrics').innerHTML=defs.map(d=>`<article class="metric"><div class="reading"><button class="toggle" data-k="${d.k}" aria-expanded="true">−</button><div class="name">${d.n}</div><div class="value${d.targets?' target-value':''}"><span id="v-${d.k}">--</span>${d.targets?'':` <span class="unit">${d.u}</span>`}</div><div class="note">${d.note}</div></div><div class="chart"><canvas id="c-${d.k}"></canvas></div></article>`).join('');$('ruleMetric').innerHTML=defs.map(d=>`<option value="${d.k}">${d.n}</option>`).join('');$('ruleMetric').value='target_speed';document.querySelectorAll('.toggle').forEach(btn=>btn.onclick=()=>toggleMetric(btn))}
function toggleMetric(btn){const card=btn.closest('.metric');const collapsed=card.classList.toggle('collapsed');btn.textContent=collapsed?'+':'−';btn.setAttribute('aria-expanded',String(!collapsed))}
function rolling(a){return a.map((p,i)=>{const v=a.slice(Math.max(0,i-11),i+1).map(z=>z.v).filter(Number.isFinite);if(v.length<2||!mult)return{...p,lo:p.v,hi:p.v};const m=v.reduce((s,n)=>s+n,0)/v.length,sd=Math.sqrt(v.reduce((s,n)=>s+(n-m)**2,0)/v.length);return{...p,lo:p.v-sd*mult,hi:p.v+sd*mult}})}
function draw(d){const cv=$('c-'+d.k),r=cv.getBoundingClientRect(),q=devicePixelRatio||1,w=Math.max(260,r.width*q),h=Math.max(100,r.height*q);if(cv.width!==w||cv.height!==h){cv.width=w;cv.height=h}const x=cv.getContext('2d'),now=Date.now(),left=48*q,right=8*q,top=7*q,bottom=22*q,pw=w-left-right,ph=h-top-bottom,sets=d.targets?hist[d.k]:[hist[d.k]];x.clearRect(0,0,w,h);const rolled=sets.map(rolling),points=rolled.flat(),vals=points.flatMap(p=>[p.lo,p.hi]).filter(Number.isFinite);if(!vals.length)return;let lo=Math.min(...vals),hi=Math.max(...vals),pad=Math.max((hi-lo)*.08,.001);lo-=pad;hi+=pad;const span=Math.max(.001,hi-lo),px=t=>left+Math.max(0,1-(now-t)/windowMs)*pw,py=v=>top+(hi-v)/span*ph;x.font=`${9*q}px system-ui`;x.textAlign='right';x.textBaseline='middle';for(let i=0;i<=4;i++){const v=hi-span*i/4,y=top+ph*i/4;x.strokeStyle='rgba(255,255,255,.09)';x.beginPath();x.moveTo(left,y);x.lineTo(w-right,y);x.stroke();x.fillStyle='#94a09d';x.fillText(v.toFixed(Math.abs(span)<2?2:1),left-5*q,y)}x.textAlign='center';x.textBaseline='top';for(let i=0;i<=4;i++){const sec=windowMs/1000*(1-i/4),xp=left+pw*i/4;x.strokeStyle='rgba(255,255,255,.07)';x.beginPath();x.moveTo(xp,top);x.lineTo(xp,top+ph);x.stroke();x.fillStyle='#94a09d';x.fillText(i===4?'now':'-'+sec.toFixed(0)+'s',xp,top+ph+4*q)}rolled.forEach((b,si)=>{const visible=b.filter(p=>Number.isFinite(p.v)&&p.t>=now-windowMs),color=d.targets?targetColors[si]:d.c;if(!visible.length)return;if(mult&&visible.length>1){x.fillStyle=d.targets?'rgba(255,255,255,.04)':'rgba(84,191,227,.14)';x.beginPath();visible.forEach((p,i)=>i?x.lineTo(px(p.t),py(p.hi)):x.moveTo(px(p.t),py(p.hi)));for(let i=visible.length-1;i>=0;i--)x.lineTo(px(visible[i].t),py(visible[i].lo));x.closePath();x.fill()}x.strokeStyle=color;x.lineWidth=2*q;x.beginPath();visible.forEach((p,i)=>i?x.lineTo(px(p.t),py(p.v)):x.moveTo(px(p.t),py(p.v)));x.stroke()});if(d.targets){x.textAlign='left';x.textBaseline='top';x.font=`${9*q}px system-ui`;[0,1,2].forEach(i=>{x.fillStyle=targetColors[i];x.fillRect(w-right-62*q,top+i*13*q,8*q,2*q);x.fillStyle='#aab4b1';x.fillText('Target '+(i+1),w-right-50*q,top-5*q+i*13*q)})}}
function drawRadar(o){const cv=$('radarCanvas'),r=cv.getBoundingClientRect(),q=devicePixelRatio||1,w=Math.max(300,r.width*q),h=Math.max(180,r.height*q);if(cv.width!==w||cv.height!==h){cv.width=w;cv.height=h}const x=cv.getContext('2d'),cx=w/2,base=h-18*q,beamDeg=60,opR=1.5,maxR=3,scale=(h-34*q)/maxR,targets=Array.isArray(o.targets)?o.targets:[];x.clearRect(0,0,w,h);x.font=`${9*q}px system-ui`;x.textAlign='center';x.textBaseline='middle';for(let m=1;m<=maxR;m++){x.strokeStyle=m<=2?'rgba(255,255,255,.13)':'rgba(255,255,255,.10)';x.beginPath();x.arc(cx,base,m*scale,Math.PI,Math.PI*2);x.stroke();x.fillStyle='rgba(255,255,255,.82)';x.fillText(m+'m',cx+m*scale-10*q,base-4*q)}x.beginPath();x.setLineDash([4*q,4*q]);x.strokeStyle='rgba(200,136,10,.95)';x.lineWidth=1.25*q;x.arc(cx,base,opR*scale,Math.PI,Math.PI*2);x.stroke();x.setLineDash([]);[-beamDeg,0,beamDeg].forEach(a=>{const rad=a*Math.PI/180,len=maxR*scale;x.setLineDash(a===0?[]:[4*q,4*q]);x.strokeStyle=a===0?'rgba(255,255,255,.25)':'rgba(200,136,10,.95)';x.lineWidth=a===0?1*q:1.25*q;x.beginPath();x.moveTo(cx,base);x.lineTo(cx+Math.sin(rad)*len,base-Math.cos(rad)*len);x.stroke()});x.setLineDash([]);for(let deg=-90;deg<=90;deg+=15){if(deg===0)continue;const rad=deg*Math.PI/180,outerX=cx+Math.sin(rad)*(maxR*scale),outerY=base-Math.cos(rad)*(maxR*scale),innerX=cx+Math.sin(rad)*((maxR*scale)-((deg%30===0)?14*q:10*q)),innerY=base-Math.cos(rad)*((maxR*scale)-((deg%30===0)?14*q:10*q));const edge=Math.abs(deg)===60;x.strokeStyle=edge?'rgba(200,136,10,.95)':deg%30===0?'rgba(255,255,255,.18)':'rgba(255,255,255,.12)';x.lineWidth=edge?1.25*q:deg%30===0?1.25*q:1*q;x.beginPath();x.moveTo(innerX,innerY);x.lineTo(outerX,outerY);x.stroke()}for(let deg=-90;deg<=90;deg+=5){const isMajor=deg%30===0,isEdge=Math.abs(deg)===60;if(deg===0)continue;const rad=deg*Math.PI/180,outerX=cx+Math.sin(rad)*(maxR*scale),outerY=base-Math.cos(rad)*(maxR*scale),innerLen=isMajor?14*q:6*q;const innerX=cx+Math.sin(rad)*((maxR*scale)-innerLen),innerY=base-Math.cos(rad)*((maxR*scale)-innerLen);x.strokeStyle=isEdge?'rgba(200,136,10,.95)':isMajor?'rgba(255,255,255,.18)':'rgba(255,255,255,.10)';x.lineWidth=isEdge?1.35*q:isMajor?1.15*q:0.85*q;x.beginPath();x.moveTo(innerX,innerY);x.lineTo(outerX,outerY);x.stroke()}x.font=`${9*q}px system-ui`;[-90,-60,-30,0,30,60,90].forEach(deg=>{const rad=deg*Math.PI/180,rlab=maxR*scale+18*q,px=cx+Math.sin(rad)*rlab,py=base-Math.cos(rad)*rlab,ty=deg===0?Math.max(14*q,py+10*q):py;x.fillStyle='rgba(255,255,255,.82)';x.fillText(`${deg}°`,px + (deg<0?-4*q:deg>0?4*q:0),ty)});x.beginPath();x.strokeStyle='rgba(255,255,255,.12)';x.lineWidth=0.8*q;x.moveTo(cx-3.5*scale,base);x.lineTo(cx+3.5*scale,base);x.stroke();const colors=['#ef5f5f','#e3b341','#39c678'];targets.forEach((t,i)=>{if(!Number.isFinite(t.x)||!Number.isFinite(t.y))return;const tx=cx+t.x*scale,ty=base-t.y*scale;x.fillStyle=colors[i%colors.length];x.strokeStyle='rgba(255,255,255,.28)';x.lineWidth=2*q;x.beginPath();x.arc(tx,ty,7*q,0,Math.PI*2);x.fill();x.stroke();x.fillStyle='#edf1ef';x.font=`${9*q}px monospace`;x.fillText(String(i+1),tx,ty-14*q)});const hasPresence=!!o.presence_valid;const present=!!o.presence;$('presenceLight').classList.toggle('live',hasPresence&&present);$('presenceText').textContent=hasPresence?(present?'Present':'Clear'):'--';$('targetCount').textContent=String(targets.length);$('frameRate').textContent=lastFps?lastFps.toFixed(1)+' fps':'--';$('targetRange').textContent=Number.isFinite(o.target_distance)?o.target_distance.toFixed(2)+' m':(Number.isFinite(o.distance)?o.distance.toFixed(2)+' m':'--');$('targetAngle').textContent=Number.isFinite(o.target_angle)?o.target_angle.toFixed(1)+'°':(Number.isFinite(o.targetAngle)?o.targetAngle.toFixed(1)+'°':'--');$('targetSpeed').textContent=Number.isFinite(o.target_speed)?o.target_speed.toFixed(2)+' m/s':(Number.isFinite(o.targetSpeed)?o.targetSpeed.toFixed(2)+' m/s':'--');$('firmwareVersion').textContent=`${o.console_fw||'2.1.2'}`;$('targetList').innerHTML=targets.slice(0,3).map((z,i)=>`<div class="target-card"><div class="target-card-body"><span>x ${fmt(z.x,2)} m  y ${fmt(z.y,2)} m</span><span>r ${fmt(z.distance,2)} m</span><span>angle ${fmt(z.angle,1)}°</span><span>speed ${fmt(z.speed,2)} m/s</span><span>cluster ${z.cluster_index}</span></div><div class="target-card-title">Target ${i+1}</div></div>`).join('')||'<div class="target-empty">No target records</div>'}function ingest(o){const now=Date.now(),targets=Array.isArray(o.targets)?o.targets:[];if(Number.isFinite(o.frame)&&lastFrame!==null&&o.frame!==lastFrame&&lastFrameAt){lastFps=(o.frame-lastFrame)*1000/Math.max(1,now-lastFrameAt)}if(Number.isFinite(o.frame)){lastFrame=o.frame;lastFrameAt=now}defs.forEach(d=>{if(d.targets){const vals=[0,1,2].map(i=>targets[i]&&Number.isFinite(targets[i][d.tk])?targets[i][d.tk]:null);vals.forEach((v,i)=>{if(Number.isFinite(v)){hist[d.k][i].push({v,t:now});while(hist[d.k][i].length&&hist[d.k][i][0].t<now-windowMs)hist[d.k][i].shift()}});$('v-'+d.k).innerHTML=vals.map((v,i)=>`<span class="target-line">T${i+1}: ${Number.isFinite(v)?v.toFixed(d.d):'n/a'} <span class="unit">${d.u}</span></span>`).join('');draw(d);return}let v=o[d.k];if(Number.isFinite(v)){hist[d.k].push({v,t:now});while(hist[d.k].length&&hist[d.k][0].t<now-windowMs)hist[d.k].shift();$('v-'+d.k).textContent=v.toFixed(d.d)}else{$('v-'+d.k).textContent='--'}draw(d)});drawRadar(o);if(logActive)logRows.push({ts:new Date().toISOString(),data:o});$('dot').classList.add('live');$('connection').textContent='Live';fail=0}
async function poll(){try{const r=await fetch('/api/sample',{cache:'no-store'});if(!r.ok)throw Error('HTTP '+r.status);ingest(await r.json())}catch(e){if(++fail===1)log('Data error: '+e.message);$('dot').classList.remove('live');$('connection').textContent='Offline'}}
function rgb(hex){return{r:parseInt(hex.slice(1,3),16),g:parseInt(hex.slice(3,5),16),b:parseInt(hex.slice(5,7),16)}}async function command(body){try{const r=await fetch('/api/led',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});if(!r.ok)throw Error('HTTP '+r.status);log('LED command applied')}catch(e){log('Command error: '+e.message)}}
function trimHistory(cutoff){Object.values(hist).forEach(a=>{if(Array.isArray(a[0]))a.forEach(s=>{while(s.length&&s[0].t<cutoff)s.shift()});else while(a.length&&a[0].t<cutoff)a.shift()})}function clearHistory(){Object.values(hist).forEach(a=>{if(Array.isArray(a[0]))a.forEach(s=>s.length=0);else a.length=0})}$('window').onchange=e=>{windowMs=+e.target.value;trimHistory(Date.now()-windowMs);defs.forEach(draw)};$('band').onchange=e=>{mult=+e.target.value;defs.forEach(draw)};$('clear').onclick=()=>{clearHistory();defs.forEach(draw);log('Plots reset')};$('sendLed').onclick=()=>{const c=rgb($('ledColor').value);command({mode:$('ledMode').value,brightness:+$('brightness').value/100,...c})};$('sendRule').onclick=()=>{const a=rgb($('lowColor').value),b=rgb($('okColor').value),c=rgb($('highColor').value);$('ledMode').value='rule';command({mode:'rule',metric:$('ruleMetric').value,low:+$('ruleLow').value,high:+$('ruleHigh').value,low_r:a.r,low_g:a.g,low_b:a.b,ok_r:b.r,ok_g:b.g,ok_b:b.b,high_r:c.r,high_g:c.g,high_b:c.b})};$('logToggle').onclick=()=>{logActive=!logActive;if(logActive){logRows=[];$('logToggle').textContent='Stop recording';log('Logging started')}else{$('logToggle').textContent='Start recording';log('Logging stopped')}};$('logExport JSON').onclick=()=>{if(!logRows.length){log('No log data to download');return}const blob=new Blob([JSON.stringify(logRows,null,2)],{type:'application/json'}),url=URL.createObjectURL(blob),a=document.createElement('a');a.href=url;a.download=`${cleanName($('logName').value)}-${logStamp()}.json`;document.body.appendChild(a);a.click();a.remove();URL.revokeObjectURL(url);log('Log downloaded')};window.onresize=()=>defs.forEach(draw);setup();setInterval(poll,300);poll();log('VisLog loaded; waiting for device data');
</script></body></html>
)UI";

String formatJsonNumber(float value, unsigned int digits) {
  if (!isfinite(value)) return "null";
  return String(value, digits);
}

String readJsonTextField(const String &body, const char *key, const String &fallback) {
  String needle = "\"" + String(key) + "\"";
  int keyPos = body.indexOf(needle);
  if (keyPos < 0) return fallback;
  int colonPos = body.indexOf(':', keyPos);
  int start = body.indexOf('"', colonPos + 1);
  int end = body.indexOf('"', start + 1);
  return start >= 0 && end > start ? body.substring(start + 1, end) : fallback;
}

float readJsonFloatField(const String &body, const char *key, float fallback) {
  String needle = "\"" + String(key) + "\"";
  int keyPos = body.indexOf(needle);
  if (keyPos < 0) return fallback;
  int colonPos = body.indexOf(':', keyPos);
  if (colonPos < 0) return fallback;
  return body.substring(colonPos + 1).toFloat();
}

int readJsonIntField(const String &body, const char *key, int fallback) {
  return int(readJsonFloatField(body, key, float(fallback)));
}

void writeStatusLed(uint8_t r, uint8_t g, uint8_t b, float brightness) {
  brightness = constrain(brightness, 0.0f, 1.0f);
  sensorState.ledR = uint8_t(r * brightness);
  sensorState.ledG = uint8_t(g * brightness);
  sensorState.ledB = uint8_t(b * brightness);
  rgbLedWrite(STATUS_LED_PIN, sensorState.ledR, sensorState.ledG, sensorState.ledB);
}

float getMetricValue(const String &metric) {
  if (metric == "heart_rate") return sensorState.heartRate;
  if (metric == "breath_rate") return sensorState.breathRate;
  if (metric == "distance") return sensorState.distance;
  if (metric == "raw_distance") return sensorState.rawDistance;
  if (metric == "presence") return sensorState.presenceValid ? (sensorState.presence ? 1.0f : 0.0f) : NAN;
  if (metric == "people_count") return sensorState.targetInfoValid || sensorState.pointCloudValid ? float(sensorState.targetCount) : NAN;
  if (metric == "target_x") return sensorState.targetX;
  if (metric == "target_y") return sensorState.targetY;
  if (metric == "target_angle") return sensorState.targetAngle;
  if (metric == "target_speed") return sensorState.targetSpeed;
  if (metric == "light") return sensorState.light;
  if (metric == "total_phase") return sensorState.totalPhase;
  if (metric == "breath_phase") return sensorState.breathPhase;
  if (metric == "heart_phase") return sensorState.heartPhase;
  return NAN;
}

void updateStatusLed() {
  if (statusLed.mode == LedMode::Off) {
    writeStatusLed(0, 0, 0, 1.0f);
    return;
  }
  if (statusLed.mode == LedMode::Pulse) {
    float wave = (sinf(float(millis()) * 0.004f) + 1.0f) * 0.5f;
    writeStatusLed(statusLed.r, statusLed.g, statusLed.b, statusLed.brightness * (0.12f + 0.88f * wave));
    return;
  }
  if (statusLed.mode == LedMode::Rule) {
    float value = getMetricValue(statusLed.metric);
    if (!isfinite(value)) {
      writeStatusLed(20, 20, 20, statusLed.brightness);
    } else if (value < statusLed.low) {
      writeStatusLed(statusLed.lowR, statusLed.lowG, statusLed.lowB, statusLed.brightness);
    } else if (value > statusLed.high) {
      writeStatusLed(statusLed.highR, statusLed.highG, statusLed.highB, statusLed.brightness);
    } else {
      writeStatusLed(statusLed.okR, statusLed.okG, statusLed.okB, statusLed.brightness);
    }
    return;
  }
  writeStatusLed(statusLed.r, statusLed.g, statusLed.b, statusLed.brightness);
}

bool beginAmbientLightSensor() {
  Wire.beginTransmission(AMBIENT_LIGHT_I2C_ADDRESS);
  Wire.write(0x01);  // Power on.
  if (Wire.endTransmission() != 0) return false;
  Wire.beginTransmission(AMBIENT_LIGHT_I2C_ADDRESS);
  Wire.write(0x10);  // Continuous high-resolution mode.
  return Wire.endTransmission() == 0;
}

void pollAmbientLight() {
  if (!ambientLightReady || millis() - lastAmbientLightReadMs < 250) return;
  lastAmbientLightReadMs = millis();
  if (Wire.requestFrom(AMBIENT_LIGHT_I2C_ADDRESS, uint8_t(2)) != 2) return;
  uint16_t raw = (uint16_t(Wire.read()) << 8) | uint16_t(Wire.read());
  sensorState.light = float(raw) / 1.2f;
}

float normalizeRadarRange(float raw) {
  if (!isfinite(raw) || raw <= 0.0f) return NAN;
  // Some MR60BHA2 firmware revisions report centimetres despite the API's
  // metre examples. Values beyond the hardware range are treated as cm.
  float metres = raw > MAX_VALID_RADAR_RANGE_M ? raw / 100.0f : raw;
  return metres >= 0.05f && metres <= MAX_VALID_RADAR_RANGE_M ? metres : NAN;
}

float normalizeRadarCoordinate(float raw) {
  if (!isfinite(raw)) return NAN;
  return fabsf(raw) > 10.0f ? raw / 100.0f : raw;
}

void resetRadarTargets() {
  sensorState.targetCount = 0;
  sensorState.targetValid = false;
  sensorState.targetX = NAN;
  sensorState.targetY = NAN;
  sensorState.targetDistance = NAN;
  sensorState.targetAngle = NAN;
  sensorState.targetSpeed = NAN;
  for (uint8_t i = 0; i < MAX_TRACKED_TARGETS; i++) {
    sensorState.targets[i] = RadarTarget();
  }
}

void storeRadarTargets(const PeopleCounting &targets, const char *source) {
  resetRadarTargets();
  sensorState.targetSource = source;
  size_t count = targets.targets.size();
  if (count > MAX_TRACKED_TARGETS) count = MAX_TRACKED_TARGETS;
  sensorState.targetCount = uint8_t(count);
  sensorState.targetValid = sensorState.targetCount > 0;
  for (uint8_t i = 0; i < sensorState.targetCount; i++) {
    const TargetN &target = targets.targets[i];
    RadarTarget &out = sensorState.targets[i];
    out.x = normalizeRadarCoordinate(target.x_point);
    out.y = normalizeRadarCoordinate(target.y_point);
    out.distance = hypotf(out.x, out.y);
    out.angle = atan2f(out.x, out.y) * 180.0f / PI;
    out.speed = float(target.dop_index) * RANGE_STEP / 100.0f;
    out.dopIndex = target.dop_index;
    out.clusterIndex = target.cluster_index;
  }
  if (sensorState.targetValid) {
    sensorState.targetX = sensorState.targets[0].x;
    sensorState.targetY = sensorState.targets[0].y;
    sensorState.targetDistance = sensorState.targets[0].distance;
    sensorState.targetAngle = sensorState.targets[0].angle;
    sensorState.targetSpeed = sensorState.targets[0].speed;
    sensorState.lastDetectionSignalMs = millis();
  }
}

void refreshPresenceState() {
  uint32_t now = millis();
  if (sensorState.lastTargetMs && now - sensorState.lastTargetMs > 1500) {
    resetRadarTargets();
    sensorState.targetSource = "stale";
  }
  if (sensorState.lastPresenceMs && now - sensorState.lastPresenceMs <= 1200) {
    sensorState.presenceValid = true;
  } else {
    sensorState.presence = false;
    sensorState.presenceValid = false;
  }
  sensorState.presenceSource = sensorState.presenceValid ? "sensor" : "waiting";
}

bool refreshRadarFirmwareInfo(bool force = false) {
  uint32_t now = millis();
  if (!force && sensorState.firmwareValid && now - sensorState.lastFirmwareMs < 5000) return false;
  FirmwareInfo firmware;
  if (radar.readFirmwareInfo(firmware) != seeed::mmwave::Status::Ok) return false;
  sensorState.firmwareRaw = firmware.value;
  sensorState.firmwareProject = firmware.firmware_verson.project_name;
  sensorState.firmwareMajor = firmware.firmware_verson.major_version;
  sensorState.firmwareSub = firmware.firmware_verson.sub_version;
  sensorState.firmwareModified = firmware.firmware_verson.modified_version;
  sensorState.firmwareValid = true;
  sensorState.lastFirmwareMs = now;
  return true;
}

void pollRadarSensor() {
  if (!radar.update(20)) return;
  bool changed = false;

  float total, breath, heart;
  if (radar.getHeartBreathPhases(total, breath, heart)) {
    sensorState.totalPhase = total;
    sensorState.breathPhase = breath;
    sensorState.heartPhase = heart;
    changed = true;
  }

  float value;
  if (radar.getBreathRate(value)) {
    sensorState.breathRate = value > 0.0f && value < 80.0f ? value : NAN;
    if (isfinite(sensorState.breathRate)) sensorState.lastDetectionSignalMs = millis();
    changed = true;
  }
  if (radar.getHeartRate(value)) {
    sensorState.heartRate = value > 0.0f && value < 240.0f ? value : NAN;
    if (isfinite(sensorState.heartRate)) sensorState.lastDetectionSignalMs = millis();
    changed = true;
  }
  if (radar.getDistance(value)) {
    sensorState.rawDistance = value;
    sensorState.distance = normalizeRadarRange(value);
    if (isfinite(sensorState.distance)) sensorState.lastDetectionSignalMs = millis();
    changed = true;
  }

  bool detected;
  if (radar.readPresence(detected) == seeed::mmwave::Status::Ok) {
    sensorState.presence = detected;
    sensorState.presenceValid = true;
    sensorState.lastPresenceMs = millis();
    sensorState.presenceSource = "sensor";
    changed = true;
  }

  PeopleCounting targets;
  bool gotTargetInfo = false;
  if (radar.readTargetInfo(targets) == seeed::mmwave::Status::Ok) {
    gotTargetInfo = true;
    sensorState.lastTargetMs = millis();
    sensorState.targetInfoValid = true;
    storeRadarTargets(targets, "target info");
    changed = true;
  }
  if (radar.readPointCloud(targets) == seeed::mmwave::Status::Ok) {
    sensorState.lastTargetMs = millis();
    sensorState.pointCloudValid = true;
    if (!gotTargetInfo) {
      storeRadarTargets(targets, "point cloud");
    }
    changed = true;
  }

  if (refreshRadarFirmwareInfo()) changed = true;

  refreshPresenceState();

  if (changed) {
    sensorState.frame++;
    sensorState.lastRadarMs = millis();
  }
}

String buildSensorStateJson() {
  String json;
  json.reserve(1180);
  json = "{";
  json += "\"heart_rate\":" + formatJsonNumber(sensorState.heartRate, 2) + ",";
  json += "\"breath_rate\":" + formatJsonNumber(sensorState.breathRate, 2) + ",";
  json += "\"distance\":" + formatJsonNumber(sensorState.distance, 3) + ",";
  json += "\"raw_distance\":" + formatJsonNumber(sensorState.rawDistance, 3) + ",";
  json += "\"total_phase\":" + formatJsonNumber(sensorState.totalPhase, 3) + ",";
  json += "\"breath_phase\":" + formatJsonNumber(sensorState.breathPhase, 3) + ",";
  json += "\"heart_phase\":" + formatJsonNumber(sensorState.heartPhase, 3) + ",";
  json += "\"presence\":" + String(sensorState.presence ? "true" : "false") + ",";
  json += "\"presence_valid\":" + String(sensorState.presenceValid ? "true" : "false") + ",";
  json += "\"presence_source\":\"" + sensorState.presenceSource + "\",";
  json += "\"target_valid\":" + String(sensorState.targetValid ? "true" : "false") + ",";
  json += "\"target_source\":\"" + sensorState.targetSource + "\",";
  json += "\"people_count\":" + String(sensorState.targetCount) + ",";
  json += "\"target_count\":" + String(sensorState.targetCount) + ",";
  json += "\"target_x\":" + formatJsonNumber(sensorState.targetX, 3) + ",";
  json += "\"target_y\":" + formatJsonNumber(sensorState.targetY, 3) + ",";
  json += "\"target_distance\":" + formatJsonNumber(sensorState.targetDistance, 3) + ",";
  json += "\"target_angle\":" + formatJsonNumber(sensorState.targetAngle, 2) + ",";
  json += "\"target_speed\":" + formatJsonNumber(sensorState.targetSpeed, 3) + ",";
  json += "\"targets\":[";
  for (uint8_t i = 0; i < sensorState.targetCount; i++) {
    if (i) json += ",";
    json += "{";
    json += "\"x\":" + formatJsonNumber(sensorState.targets[i].x, 3) + ",";
    json += "\"y\":" + formatJsonNumber(sensorState.targets[i].y, 3) + ",";
    json += "\"distance\":" + formatJsonNumber(sensorState.targets[i].distance, 3) + ",";
    json += "\"angle\":" + formatJsonNumber(sensorState.targets[i].angle, 2) + ",";
    json += "\"speed\":" + formatJsonNumber(sensorState.targets[i].speed, 3) + ",";
    json += "\"dop_index\":" + String(sensorState.targets[i].dopIndex) + ",";
    json += "\"cluster_index\":" + String(sensorState.targets[i].clusterIndex);
    json += "}";
  }
  json += "],";
  json += "\"light\":" + formatJsonNumber(sensorState.light, 1) + ",";
  json += "\"light_ready\":" + String(ambientLightReady ? "true" : "false") + ",";
  json += "\"console_fw\":\"" + String(VisLog_FW_VERSION) + "\",";
  json += "\"firmware_valid\":" + String(sensorState.firmwareValid ? "true" : "false") + ",";
  json += "\"firmware_raw\":" + String(sensorState.firmwareRaw) + ",";
  json += "\"firmware_project\":" + String(sensorState.firmwareProject) + ",";
  json += "\"firmware_major\":" + String(sensorState.firmwareMajor) + ",";
  json += "\"firmware_sub\":" + String(sensorState.firmwareSub) + ",";
  json += "\"firmware_modified\":" + String(sensorState.firmwareModified) + ",";
  json += "\"led_r\":" + String(sensorState.ledR) + ",";
  json += "\"led_g\":" + String(sensorState.ledG) + ",";
  json += "\"led_b\":" + String(sensorState.ledB) + ",";
  json += "\"frame\":" + String(sensorState.frame) + ",";
  json += "\"last_radar_ms\":" + String(sensorState.lastRadarMs) + ",";
  json += "\"uptime_ms\":" + String(millis());
  json += "}";
  return json;
}

void handleLedCommand(const String &body) {
  String mode = readJsonTextField(body, "mode", "solid");
  if (mode == "off") statusLed.mode = LedMode::Off;
  else if (mode == "pulse") statusLed.mode = LedMode::Pulse;
  else if (mode == "rule") statusLed.mode = LedMode::Rule;
  else statusLed.mode = LedMode::Solid;

  statusLed.brightness = constrain(readJsonFloatField(body, "brightness", statusLed.brightness), 0.0f, 1.0f);
  statusLed.r = constrain(readJsonIntField(body, "r", statusLed.r), 0, 255);
  statusLed.g = constrain(readJsonIntField(body, "g", statusLed.g), 0, 255);
  statusLed.b = constrain(readJsonIntField(body, "b", statusLed.b), 0, 255);
  statusLed.metric = readJsonTextField(body, "metric", statusLed.metric);
  statusLed.low = readJsonFloatField(body, "low", statusLed.low);
  statusLed.high = readJsonFloatField(body, "high", statusLed.high);
  statusLed.lowR = constrain(readJsonIntField(body, "low_r", statusLed.lowR), 0, 255);
  statusLed.lowG = constrain(readJsonIntField(body, "low_g", statusLed.lowG), 0, 255);
  statusLed.lowB = constrain(readJsonIntField(body, "low_b", statusLed.lowB), 0, 255);
  statusLed.okR = constrain(readJsonIntField(body, "ok_r", statusLed.okR), 0, 255);
  statusLed.okG = constrain(readJsonIntField(body, "ok_g", statusLed.okG), 0, 255);
  statusLed.okB = constrain(readJsonIntField(body, "ok_b", statusLed.okB), 0, 255);
  statusLed.highR = constrain(readJsonIntField(body, "high_r", statusLed.highR), 0, 255);
  statusLed.highG = constrain(readJsonIntField(body, "high_g", statusLed.highG), 0, 255);
  statusLed.highB = constrain(readJsonIntField(body, "high_b", statusLed.highB), 0, 255);
  if (statusLed.high < statusLed.low) {
    float swap = statusLed.low;
    statusLed.low = statusLed.high;
    statusLed.high = swap;
  }
  updateStatusLed();
}

void sendCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.sendHeader("Cache-Control", "no-store");
}

void setupWebServer() {
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
    server.send_P(200, "text/html", UI_HTML);
  });
  server.on("/api/sample", HTTP_GET, []() {
    sendCorsHeaders();
    server.send(200, "application/json", buildSensorStateJson());
  });
  server.on("/api/led", HTTP_POST, []() {
    handleLedCommand(server.arg("plain"));
    sendCorsHeaders();
    server.send(200, "application/json", "{\"ok\":true}");
  });
  server.on("/api/sample", HTTP_OPTIONS, []() { sendCorsHeaders(); server.send(204); });
  server.on("/api/led", HTTP_OPTIONS, []() { sendCorsHeaders(); server.send(204); });
  server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });
  server.begin();
}

void setupOtaUpdates() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.onStart([]() {
    Serial.println("OTA start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA complete");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA progress: %u%%\r", (progress * 100) / total);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("auth failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("begin failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("connect failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("receive failed");
    else if (error == OTA_END_ERROR) Serial.println("end failed");
    else Serial.println("unknown");
  });
  ArduinoOTA.begin();
}

void setup() {
  Serial.begin(115200);
  delay(300);

  radarSerial.begin(115200, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);
  radar.begin(&radarSerial);
  refreshRadarFirmwareInfo(true);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  ambientLightReady = beginAmbientLightSensor();

  rgbLedWrite(STATUS_LED_PIN, 0, 0, 0);
  updateStatusLed();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
  setupWebServer();
  setupOtaUpdates();

  Serial.println();
  Serial.println("MR60BHA2 Sensor VisLog ready");
  Serial.print("WiFi: ");
  Serial.println(WIFI_AP_SSID);
  Serial.print("Open: http://");
  Serial.println(WiFi.softAPIP());
  Serial.print("OTA hostname: ");
  Serial.println(OTA_HOSTNAME);
  Serial.print("OTA password: ");
  Serial.println(OTA_PASSWORD);
  Serial.print("BH1750: ");
  Serial.println(ambientLightReady ? "ready" : "not found");
}

void loop() {
  pollRadarSensor();
  refreshPresenceState();
  pollAmbientLight();
  updateStatusLed();
  server.handleClient();
  ArduinoOTA.handle();

  if (millis() - lastSerialReportMs >= 1000) {
    lastSerialReportMs = millis();
    Serial.println(buildSensorStateJson());
  }
}

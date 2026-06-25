/* SPDX-License-Identifier: MIT */

#include "net_services.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/dhcpv4_server.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/sys/util.h>

#include "app_state.h"
#include "led_control.h"
#include "ota_control.h"
#include "json_stream.h"

LOG_MODULE_REGISTER(vislog_net, LOG_LEVEL_INF);

#define WIFI_AP_SSID "mmWaveVisLog-MR60BHA2"
#define WIFI_AP_PASSWORD "wirelessphysiology"
#define WIFI_AP_CHANNEL WIFI_CHANNEL_ANY
#define WIFI_AP_PORT 80

static struct net_if *ap_iface;
static struct net_mgmt_event_callback wifi_cb;
static uint16_t http_port = WIFI_AP_PORT;

static const char dashboard_html[] = R"HTML(<!doctype html><html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>MR60BHA2 Live Console</title>
<style>
:root{color-scheme:dark;--bg:#0d1011;--panel:#171b1d;--line:#30373b;--text:#edf1ef;--muted:#94a09d;--green:#39c678;--yellow:#e3b341;--red:#ef5f5f;--cyan:#54bfe3}.presence-dot{display:inline-block;width:9px;height:9px;border-radius:50%;background:var(--red);box-shadow:0 0 0 1px rgba(0,0,0,.25) inset}.presence-dot.live{background:var(--green);box-shadow:0 0 10px #39c678}*{box-sizing:border-box}body{margin:0;background:#0d1011;color:var(--text);font:13px system-ui,-apple-system,sans-serif}.app{width:min(1180px,calc(100% - 20px));margin:auto;padding:12px 0 22px}header{display:flex;align-items:center;justify-content:space-between;gap:12px;padding:8px 0 12px;border-bottom:1px solid var(--line)}h1{font-size:19px;margin:0;letter-spacing:0}.status{display:flex;align-items:center;gap:7px;color:var(--muted)}.dot{width:8px;height:8px;border-radius:50%;background:var(--red)}.dot.live{background:var(--green);box-shadow:0 0 10px #39c678}.controls{display:grid;grid-template-columns:1fr 1fr auto;gap:8px;align-items:end;padding:10px 0}label{display:grid;gap:4px;color:var(--muted);font-size:11px}select,input,button{height:32px;border:1px solid var(--line);border-radius:6px;background:#121517;color:var(--text);padding:0 8px;font:inherit}button{cursor:pointer;background:#252b2e}.metrics{display:grid;gap:6px}.metric{display:grid;grid-template-columns:185px minmax(0,1fr);min-height:128px;border:1px solid var(--line);border-radius:7px;background:var(--panel);overflow:hidden;position:relative}.metric.collapsed{grid-template-columns:1fr;min-height:34px}.metric.collapsed .chart,.metric.collapsed .note,.metric.collapsed .value,.metric.collapsed .subvalue{display:none}.metric.collapsed .reading{border-right:none;padding:7px 12px 7px 24px;min-height:34px;justify-content:center}.reading{padding:12px 12px 12px 24px;border-right:1px solid var(--line);display:flex;flex-direction:column;justify-content:center;position:relative;min-height:128px}.toggle{position:absolute;left:7px;top:7px;width:12px;height:12px;padding:0;border:none;border-radius:999px;background:transparent;color:var(--muted);font-size:12px;line-height:12px;cursor:pointer}.toggle:hover{color:var(--text)}.name{color:var(--muted);font-size:11px}.value{font-size:28px;font-weight:750;line-height:1.1;margin:4px 0 0}.value.target-value{display:grid;gap:4px;font-size:24px;font-weight:750;line-height:1.08}.value.target-value .unit{display:inline;font-size:11px;color:var(--muted);font-weight:500}.value.target-value .target-line{display:block;white-space:nowrap}.subvalue{font-size:11px;line-height:1.2;color:#aab4b1;margin-top:4px}.unit{font-size:11px;color:var(--muted);font-weight:500}.note{color:#6f7b78;font-size:10px;line-height:1.3;margin-top:3px}.chart{position:relative;height:126px}.chart canvas{width:100%;height:100%;display:block}.radar{display:grid;grid-template-columns:minmax(340px,360px) minmax(250px,280px) minmax(0,1fr);margin-top:6px;border:1px solid var(--line);border-radius:7px;background:var(--panel);overflow:hidden}.radar-info,.target-stack{padding:12px 14px}.radar-info{border-right:1px solid var(--line)}.target-stack{border-right:1px solid var(--line);overflow:auto}.radar-info h2{font-size:12px;margin:0}.radar-top{display:flex;align-items:center;justify-content:space-between;gap:10px;margin-bottom:10px}.radar-top strong{font-size:13px}.target-table{display:grid;grid-template-columns:1fr;gap:6px}.target-row{display:grid;grid-template-columns:96px minmax(0,1fr);gap:8px;align-items:center;padding:5px 0;border-bottom:1px solid rgba(255,255,255,.06)}.target-row:last-child{border-bottom:none}.target-row span{color:var(--muted);font-size:10px}.target-row strong{display:block;color:var(--text);font-size:12px;font-weight:600;line-height:1.25}.target-list{display:grid;gap:8px;align-content:start;min-width:0}.target-card,.target-empty{border:1px solid rgba(255,255,255,.08);border-radius:7px;background:#131619;padding:9px 10px}.target-empty{color:#aab4b1;font:10px/1.4 ui-monospace,monospace}.target-card{display:grid;grid-template-columns:minmax(0,1fr) auto;gap:10px;align-items:center}.target-card-title{font-size:14px;color:#edf1ef;font-weight:700;justify-self:end;white-space:nowrap}.target-card-body{display:grid;gap:2px;color:#dbe0de;font:10px/1.35 ui-monospace,monospace;min-width:0}.target-card-body span{display:block;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}.radar-canvas{height:240px}.radar-canvas canvas{width:100%;height:100%;display:block}.tools{display:grid;grid-template-columns:1fr 1.3fr;gap:6px;margin-top:8px}.box{border:1px solid var(--line);border-radius:7px;background:var(--panel);padding:10px}.box h2{font-size:12px;margin:0 0 8px}.ota-grid{display:grid;grid-template-columns:minmax(0,1fr) auto;gap:7px;align-items:end}.ota-status{margin-top:6px;color:var(--muted);font-size:10px;line-height:1.35}.led-grid{display:grid;grid-template-columns:84px 1fr 1fr 1fr auto;gap:7px;align-items:end}.rule-grid{display:grid;grid-template-columns:1.2fr 1fr 1fr 42px 42px 42px auto;gap:7px;align-items:end}.color{padding:2px}.log-controls{display:grid;grid-template-columns:minmax(0,1fr) auto auto;gap:7px;align-items:center;margin-bottom:8px}.log-controls input{width:100%}.log-controls button{white-space:nowrap}.log{height:86px;overflow:auto;background:#090b0c;border:1px solid var(--line);border-radius:6px;padding:7px;color:#aab4b1;font:10px/1.4 ui-monospace,monospace;white-space:pre-wrap}.footer{display:flex;justify-content:flex-end;gap:8px;color:#6f7b78;font-size:10px;margin-top:10px}.small{font-size:10px;color:var(--muted)}@media(max-width:720px){header{align-items:flex-start}.controls,.tools{grid-template-columns:1fr}.metric{grid-template-columns:130px minmax(0,1fr)}.radar{grid-template-columns:1fr}.reading,.radar-info,.target-stack{padding:9px}.metric.collapsed{grid-template-columns:1fr}.value{font-size:22px}.radar-canvas{height:210px}.target-row{grid-template-columns:84px minmax(0,1fr)}.led-grid,.rule-grid{grid-template-columns:1fr 1fr}.rule-grid button,.led-grid button{grid-column:1/-1}}
</style></head><body><main class="app">
<header><div><h1>MR60BHA2 live console</h1><div class="small">Realtime data visualization and logging</div></div><div class="status"><span id="dot" class="dot"></span><span id="connection">Connecting</span></div></header>
<section class="controls"><label>History window<select id="window"><option value="18000">18 seconds</option><option value="36000" selected>36 seconds</option><option value="72000">72 seconds</option></select></label><label>Rolling error band (12 points)<select id="band"><option value="0">Off</option><option value="1">+/- 1 sigma</option><option value="2" selected>+/- 2 sigma</option><option value="3">+/- 3 sigma</option></select></label><button id="clear">Clear graphs</button></section>
<section class="radar"><div class="radar-info"><div class="radar-top"><h2>Target position</h2></div><div class="target-table"><div class="target-row"><span>Presence</span><strong><span id="presenceLight" class="presence-dot"></span> <span id="presenceText">--</span></strong></div><div class="target-row"><span>Targets</span><strong id="targetCount">--</strong></div><div class="target-row"><span>Frame rate</span><strong id="frameRate">--</strong></div><div class="target-row"><span>Range</span><strong id="targetRange">--</strong></div><div class="target-row"><span>Angle</span><strong id="targetAngle">--</strong></div><div class="target-row"><span>Speed</span><strong id="targetSpeed">--</strong></div><div class="target-row"><span>FW</span><strong id="firmwareVersion">--</strong></div></div></div><div class="target-stack"><div id="targetList" class="target-list"><div class="target-empty">No target records</div></div></div><div class="radar-canvas"><canvas id="radarCanvas"></canvas></div></section>
<section id="metrics" class="metrics" style="margin-top:6px"></section>
<section class="tools"><div class="box"><h2>Firmware update</h2><div class="ota-grid"><label>Signed image<input id="otaFile" type="file" accept=".bin,.signed.bin,.hex,.signed.hex"></label><button id="otaUpload">Upload and reboot</button></div><div id="otaStatus" class="ota-status">Use a signed MCUboot image from the Zephyr build output.</div></div>
<div class="box"><h2>RGB LED</h2><div class="led-grid"><label>Mode<select id="ledMode"><option value="solid">Solid</option><option value="pulse">Breathing pulse</option><option value="off">Off</option><option value="rule">Threshold rule</option></select></label><label>Color<input id="ledColor" class="color" type="color" value="#39c678"></label><label>Brightness<input id="brightness" type="range" min="0" max="100" value="35"></label><div></div><button id="sendLed">Apply</button></div></div>
<div class="box"><h2>Threshold rule</h2><div class="rule-grid"><label>Measurement<select id="ruleMetric"></select></label><label>Low<input id="ruleLow" type="number" step="0.01" value="-0.05"></label><label>High<input id="ruleHigh" type="number" step="0.01" value="0.05"></label><label>Below<input id="lowColor" class="color" type="color" value="#ff0000"></label><label>In band<input id="okColor" class="color" type="color" value="#000000"></label><label>Above<input id="highColor" class="color" type="color" value="#0050ff"></label><button id="sendRule">Apply</button></div></div></section>
<section class="box" style="margin-top:6px"><h2>Command and data log</h2><div class="log-controls"><input id="logName" type="text" maxlength="32" placeholder="log name"><button id="logToggle">Start log</button><button id="logDownload">Download</button></div><div id="log" class="log"></div></section>
<footer class="footer"><span>Created by @alexburton</span><span>2026-06-23</span><span>License: personal use</span></footer>
</main><script>
const defs=[
{k:'heart_rate',n:'Heart rate',u:'bpm',c:'#e56a9f',d:1,note:'Radar-estimated beats per minute'},
{k:'heart_phase',n:'Heart phase',u:'raw',c:'#a88cff',d:2,note:'Heartbeat-filtered raw phase value'},
{k:'breath_rate',n:'Breathing rate',u:'rpm',c:'#39c678',d:1,note:'Radar-estimated breaths per minute'},
{k:'breath_phase',n:'Breathing phase',u:'raw',c:'#6aa7ff',d:2,note:'Breathing-filtered raw phase value'},
{k:'total_phase',n:'Total phase',u:'raw',c:'#54bfe3',d:2,note:'Raw radar phase value'},
{k:'distance',n:'Subject distance',u:'m',c:'#e3b341',d:2,note:'Validated radar range'},
{k:'target_distance',n:'Target range',u:'m',c:'#f28e2b',d:2,note:'All targets range derived from X/Y',targets:true,tk:'distance'},
{k:'target_angle',n:'Target angle',u:'deg',c:'#edc948',d:1,note:'All targets angle derived from X/Y',targets:true,tk:'angle'},
{k:'target_speed',n:'Target speed',u:'m/s',c:'#b07aa1',d:2,note:'All targets speed derived from Doppler index',targets:true,tk:'speed'},
{k:'light',n:'Ambient light',u:'lux',c:'#f5d76e',d:1,note:'Onboard BH1750 over I2C'}];
let windowMs=36000,mult=2,fail=0,lastGoodAt=0;const targetColors=['#ef5f5f','#e3b341','#39c678'];const hist=Object.fromEntries(defs.map(d=>[d.k,d.targets?[[],[],[]]:[]]));
const $=id=>document.getElementById(id);function log(s){$('log').textContent=new Date().toLocaleTimeString()+'  '+s+'\n'+$('log').textContent}
function fmt(v,d){return Number.isFinite(v)?v.toFixed(d):'--'}
function logStamp(d=new Date()){const z=n=>String(n).padStart(2,'0');return `${d.getFullYear()}-${z(d.getMonth()+1)}-${z(d.getDate())}_${z(d.getHours())}-${z(d.getMinutes())}-${z(d.getSeconds())}`}
function cleanName(s){return (s||'mr60bha2').trim().replace(/[^a-z0-9_-]+/gi,'-').replace(/^-+|-+$/g,'').slice(0,32)||'mr60bha2'}
let logActive=false,logRows=[],lastFrame=null,lastFrameAt=0,lastFps=null;
function setup(){ $('metrics').innerHTML=defs.map(d=>`<article class="metric"><div class="reading"><button class="toggle" data-k="${d.k}" aria-expanded="true">−</button><div class="name">${d.n}</div><div class="value${d.targets?' target-value':''}"><span id="v-${d.k}">--</span>${d.targets?'':` <span class="unit">${d.u}</span>`}</div><div class="note">${d.note}</div></div><div class="chart"><canvas id="c-${d.k}"></canvas></div></article>`).join('');$('ruleMetric').innerHTML=defs.map(d=>`<option value="${d.k}">${d.n}</option>`).join('');$('ruleMetric').value='target_speed';document.querySelectorAll('.toggle').forEach(btn=>btn.onclick=()=>toggleMetric(btn))}
function toggleMetric(btn){const card=btn.closest('.metric');const collapsed=card.classList.toggle('collapsed');btn.textContent=collapsed?'+':'−';btn.setAttribute('aria-expanded',String(!collapsed))}
function rolling(a){return a.map((p,i)=>{const v=a.slice(Math.max(0,i-11),i+1).map(z=>z.v).filter(Number.isFinite);if(v.length<2||!mult)return{...p,lo:p.v,hi:p.v};const m=v.reduce((s,n)=>s+n,0)/v.length,sd=Math.sqrt(v.reduce((s,n)=>s+(n-m)**2,0)/v.length);return{...p,lo:p.v-sd*mult,hi:p.v+sd*mult}})}
function draw(d){const cv=$('c-'+d.k),r=cv.getBoundingClientRect(),q=devicePixelRatio||1,w=Math.max(260,r.width*q),h=Math.max(100,r.height*q);if(cv.width!==w||cv.height!==h){cv.width=w;cv.height=h}const x=cv.getContext('2d'),now=Date.now(),left=48*q,right=8*q,top=7*q,bottom=22*q,pw=w-left-right,ph=h-top-bottom,sets=d.targets?hist[d.k]:[hist[d.k]];x.clearRect(0,0,w,h);const rolled=sets.map(rolling),points=rolled.flat(),vals=points.flatMap(p=>[p.lo,p.hi]).filter(Number.isFinite);if(!vals.length)return;let lo=Math.min(...vals),hi=Math.max(...vals),pad=Math.max((hi-lo)*.08,.001);lo-=pad;hi+=pad;const span=Math.max(.001,hi-lo),px=t=>left+Math.max(0,1-(now-t)/windowMs)*pw,py=v=>top+(hi-v)/span*ph;x.font=`${9*q}px system-ui`;x.textAlign='right';x.textBaseline='middle';for(let i=0;i<=4;i++){const v=hi-span*i/4,y=top+ph*i/4;x.strokeStyle='rgba(255,255,255,.09)';x.beginPath();x.moveTo(left,y);x.lineTo(w-right,y);x.stroke();x.fillStyle='#94a09d';x.fillText(v.toFixed(Math.abs(span)<2?2:1),left-5*q,y)}x.textAlign='center';x.textBaseline='top';for(let i=0;i<=4;i++){const sec=windowMs/1000*(1-i/4),xp=left+pw*i/4;x.strokeStyle='rgba(255,255,255,.07)';x.beginPath();x.moveTo(xp,top);x.lineTo(xp,top+ph);x.stroke();x.fillStyle='#94a09d';x.fillText(i===4?'now':'-'+sec.toFixed(0)+'s',xp,top+ph+4*q)}rolled.forEach((b,si)=>{const visible=b.filter(p=>Number.isFinite(p.v)&&p.t>=now-windowMs),color=d.targets?targetColors[si]:d.c;if(!visible.length)return;if(mult&&visible.length>1){x.fillStyle=d.targets?'rgba(255,255,255,.04)':'rgba(84,191,227,.14)';x.beginPath();visible.forEach((p,i)=>i?x.lineTo(px(p.t),py(p.hi)):x.moveTo(px(p.t),py(p.hi)));for(let i=visible.length-1;i>=0;i--)x.lineTo(px(visible[i].t),py(visible[i].lo));x.closePath();x.fill()}x.strokeStyle=color;x.lineWidth=2*q;x.beginPath();visible.forEach((p,i)=>i?x.lineTo(px(p.t),py(p.v)):x.moveTo(px(p.t),py(p.v)));x.stroke()});if(d.targets){x.textAlign='left';x.textBaseline='top';x.font=`${9*q}px system-ui`;[0,1,2].forEach(i=>{x.fillStyle=targetColors[i];x.fillRect(w-right-62*q,top+i*13*q,8*q,2*q);x.fillStyle='#aab4b1';x.fillText('Target '+(i+1),w-right-50*q,top-5*q+i*13*q)})}}
function drawRadar(o){const cv=$('radarCanvas'),r=cv.getBoundingClientRect(),q=devicePixelRatio||1,w=Math.max(300,r.width*q),h=Math.max(180,r.height*q);if(cv.width!==w||cv.height!==h){cv.width=w;cv.height=h}const x=cv.getContext('2d'),cx=w/2,base=h-18*q,beamDeg=60,opR=1.5,maxR=3,scale=(h-34*q)/maxR,targets=Array.isArray(o.targets)?o.targets:[];x.clearRect(0,0,w,h);x.font=`${9*q}px system-ui`;x.textAlign='center';x.textBaseline='middle';for(let m=1;m<=maxR;m++){x.strokeStyle=m<=2?'rgba(255,255,255,.13)':'rgba(255,255,255,.10)';x.beginPath();x.arc(cx,base,m*scale,Math.PI,Math.PI*2);x.stroke();x.fillStyle='rgba(255,255,255,.82)';x.fillText(m+'m',cx+m*scale-10*q,base-4*q)}x.beginPath();x.setLineDash([4*q,4*q]);x.strokeStyle='rgba(200,136,10,.95)';x.lineWidth=1.25*q;x.arc(cx,base,opR*scale,Math.PI,Math.PI*2);x.stroke();x.setLineDash([]);[-beamDeg,0,beamDeg].forEach(a=>{const rad=a*Math.PI/180,len=maxR*scale;x.setLineDash(a===0?[]:[4*q,4*q]);x.strokeStyle=a===0?'rgba(255,255,255,.25)':'rgba(200,136,10,.95)';x.lineWidth=a===0?1*q:1.25*q;x.beginPath();x.moveTo(cx,base);x.lineTo(cx+Math.sin(rad)*len,base-Math.cos(rad)*len);x.stroke()});x.setLineDash([]);for(let deg=-90;deg<=90;deg+=15){if(deg===0)continue;const rad=deg*Math.PI/180,outerX=cx+Math.sin(rad)*(maxR*scale),outerY=base-Math.cos(rad)*(maxR*scale),innerX=cx+Math.sin(rad)*((maxR*scale)-((deg%30===0)?14*q:10*q)),innerY=base-Math.cos(rad)*((maxR*scale)-((deg%30===0)?14*q:10*q));const edge=Math.abs(deg)===60;x.strokeStyle=edge?'rgba(200,136,10,.95)':deg%30===0?'rgba(255,255,255,.18)':'rgba(255,255,255,.12)';x.lineWidth=edge?1.25*q:deg%30===0?1.25*q:1*q;x.beginPath();x.moveTo(innerX,innerY);x.lineTo(outerX,outerY);x.stroke()}for(let deg=-90;deg<=90;deg+=5){const isMajor=deg%30===0,isEdge=Math.abs(deg)===60;if(deg===0)continue;const rad=deg*Math.PI/180,outerX=cx+Math.sin(rad)*(maxR*scale),outerY=base-Math.cos(rad)*(maxR*scale),innerLen=isMajor?14*q:6*q;const innerX=cx+Math.sin(rad)*((maxR*scale)-innerLen),innerY=base-Math.cos(rad)*((maxR*scale)-innerLen);x.strokeStyle=isEdge?'rgba(200,136,10,.95)':isMajor?'rgba(255,255,255,.18)':'rgba(255,255,255,.10)';x.lineWidth=isEdge?1.35*q:isMajor?1.15*q:0.85*q;x.beginPath();x.moveTo(innerX,innerY);x.lineTo(outerX,outerY);x.stroke()}x.font=`${9*q}px system-ui`;[-90,-60,-30,0,30,60,90].forEach(deg=>{const rad=deg*Math.PI/180,rlab=maxR*scale+18*q,px=cx+Math.sin(rad)*rlab,py=base-Math.cos(rad)*rlab,ty=deg===0?Math.max(14*q,py+10*q):py;x.fillStyle='rgba(255,255,255,.82)';x.fillText(`${deg}°`,px + (deg<0?-4*q:deg>0?4*q:0),ty)});x.beginPath();x.strokeStyle='rgba(255,255,255,.12)';x.lineWidth=0.8*q;x.moveTo(cx-3.5*scale,base);x.lineTo(cx+3.5*scale,base);x.stroke();const colors=['#ef5f5f','#e3b341','#39c678'];targets.forEach((t,i)=>{if(!Number.isFinite(t.x)||!Number.isFinite(t.y))return;const tx=cx+t.x*scale,ty=base-t.y*scale;x.fillStyle=colors[i%colors.length];x.strokeStyle='rgba(255,255,255,.28)';x.lineWidth=2*q;x.beginPath();x.arc(tx,ty,7*q,0,Math.PI*2);x.fill();x.stroke();x.fillStyle='#edf1ef';x.font=`${9*q}px monospace`;x.fillText(String(i+1),tx,ty-14*q)});const hasPresence=!!o.presence_valid;const present=!!o.presence;$('presenceLight').classList.toggle('live',hasPresence&&present);$('presenceText').textContent=hasPresence?(present?'Present':'Clear'):'--';$('targetCount').textContent=String(targets.length);$('frameRate').textContent=lastFps?lastFps.toFixed(1)+' fps':'--';$('targetRange').textContent=Number.isFinite(o.target_distance)?o.target_distance.toFixed(2)+' m':(Number.isFinite(o.distance)?o.distance.toFixed(2)+' m':'--');$('targetAngle').textContent=Number.isFinite(o.target_angle)?o.target_angle.toFixed(1)+'°':(Number.isFinite(o.targetAngle)?o.targetAngle.toFixed(1)+'°':'--');$('targetSpeed').textContent=Number.isFinite(o.target_speed)?o.target_speed.toFixed(2)+' m/s':(Number.isFinite(o.targetSpeed)?o.targetSpeed.toFixed(2)+' m/s':'--');$('firmwareVersion').textContent=`${o.console_fw||'2.1.4'}`;$('targetList').innerHTML=targets.slice(0,3).map((z,i)=>`<div class="target-card"><div class="target-card-body"><span>x ${fmt(z.x,2)} m  y ${fmt(z.y,2)} m</span><span>r ${fmt(z.distance,2)} m</span><span>angle ${fmt(z.angle,1)}°</span><span>speed ${fmt(z.speed,2)} m/s</span><span>cluster ${z.cluster_index}</span></div><div class="target-card-title">Target ${i+1}</div></div>`).join('')||'<div class="target-empty">No target records</div>'}function ingest(o){const now=Date.now(),targets=Array.isArray(o.targets)?o.targets:[];if(Number.isFinite(o.frame)&&lastFrame!==null&&o.frame!==lastFrame&&lastFrameAt){lastFps=(o.frame-lastFrame)*1000/Math.max(1,now-lastFrameAt)}if(Number.isFinite(o.frame)){lastFrame=o.frame;lastFrameAt=now}defs.forEach(d=>{if(d.targets){const vals=[0,1,2].map(i=>targets[i]&&Number.isFinite(targets[i][d.tk])?targets[i][d.tk]:null);vals.forEach((v,i)=>{if(Number.isFinite(v)){hist[d.k][i].push({v,t:now});while(hist[d.k][i].length&&hist[d.k][i][0].t<now-windowMs)hist[d.k][i].shift()}});$('v-'+d.k).innerHTML=vals.map((v,i)=>`<span class="target-line">T${i+1}: ${Number.isFinite(v)?v.toFixed(d.d):'n/a'} <span class="unit">${d.u}</span></span>`).join('');draw(d);return}let v=o[d.k];if(Number.isFinite(v)){hist[d.k].push({v,t:now});while(hist[d.k].length&&hist[d.k][0].t<now-windowMs)hist[d.k].shift();$('v-'+d.k).textContent=v.toFixed(d.d)}else{$('v-'+d.k).textContent='--'}draw(d)});drawRadar(o);if(logActive)logRows.push({ts:new Date().toISOString(),data:o});$('dot').classList.add('live');$('connection').textContent='Live';fail=0;lastGoodAt=now}
async function poll(){const now=Date.now();try{const ac=new AbortController(),timer=setTimeout(()=>ac.abort(),1200);const r=await fetch('/api/sample',{cache:'no-store',signal:ac.signal});clearTimeout(timer);if(!r.ok)throw Error('HTTP '+r.status);ingest(await r.json())}catch(e){if(++fail===1)log('Data error: '+e.message);if(fail>=5&&now-lastGoodAt>=3000){$('dot').classList.remove('live');$('connection').textContent='Offline'}}}
function rgb(hex){return{r:parseInt(hex.slice(1,3),16),g:parseInt(hex.slice(3,5),16),b:parseInt(hex.slice(5,7),16)}}async function command(body){try{const r=await fetch('/api/led',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});if(!r.ok)throw Error('HTTP '+r.status);log('LED command applied')}catch(e){log('Command error: '+e.message)}}
function setOtaStatus(msg){$('otaStatus').textContent=msg}
function uploadOta(){const file=$('otaFile').files[0];if(!file){setOtaStatus('Select a signed firmware image first.');return}const xhr=new XMLHttpRequest();xhr.open('POST','/api/ota');xhr.setRequestHeader('Content-Type','application/octet-stream');$('otaUpload').disabled=true;setOtaStatus(`Uploading ${file.name}...`);xhr.upload.onprogress=e=>{if(e.lengthComputable){setOtaStatus(`Uploading ${Math.round(e.loaded/e.total*100)}%`)}else{setOtaStatus(`Uploading ${Math.round(e.loaded/1024)} KiB...`)}};xhr.onload=async()=>{$('otaUpload').disabled=false;if(xhr.status<200||xhr.status>=300){setOtaStatus(`Upload failed (HTTP ${xhr.status})`);log(`OTA upload failed: HTTP ${xhr.status}`);return}setOtaStatus('Upload complete. Requesting reboot...');log('OTA upload complete');try{const r=await fetch('/api/ota/reboot',{method:'POST'});const j=await r.json();if(!r.ok||!j.ok)throw Error(`HTTP ${r.status}`);setOtaStatus('Reboot scheduled. Reconnect after the device restarts.');log('OTA reboot requested')}catch(e){setOtaStatus(`Upload ok, reboot failed: ${e.message}`);log('Command error: '+e.message)}};xhr.onerror=()=>{$('otaUpload').disabled=false;setOtaStatus('Upload failed.');log('OTA upload failed')};xhr.send(file)}
function trimHistory(cutoff){Object.values(hist).forEach(a=>{if(Array.isArray(a[0]))a.forEach(s=>{while(s.length&&s[0].t<cutoff)s.shift()});else while(a.length&&a[0].t<cutoff)a.shift()})}function clearHistory(){Object.values(hist).forEach(a=>{if(Array.isArray(a[0]))a.forEach(s=>s.length=0);else a.length=0})}$('window').onchange=e=>{windowMs=+e.target.value;trimHistory(Date.now()-windowMs);defs.forEach(draw)};$('band').onchange=e=>{mult=+e.target.value;defs.forEach(draw)};$('clear').onclick=()=>{clearHistory();defs.forEach(draw);log('Graphs cleared')};$('otaUpload').onclick=uploadOta;$('sendLed').onclick=()=>{const c=rgb($('ledColor').value);command({mode:$('ledMode').value,brightness:+$('brightness').value/100,...c})};$('sendRule').onclick=()=>{const a=rgb($('lowColor').value),b=rgb($('okColor').value),c=rgb($('highColor').value);$('ledMode').value='rule';command({mode:'rule',metric:$('ruleMetric').value,low:+$('ruleLow').value,high:+$('ruleHigh').value,low_r:a.r,low_g:a.g,low_b:a.b,ok_r:b.r,ok_g:b.g,ok_b:b.b,high_r:c.r,high_g:c.g,high_b:c.b})};$('logToggle').onclick=()=>{logActive=!logActive;if(logActive){logRows=[];$('logToggle').textContent='Stop log';log('Logging started')}else{$('logToggle').textContent='Start log';log('Logging stopped')}};$('logDownload').onclick=()=>{if(!logRows.length){log('No log data to download');return}const blob=new Blob([JSON.stringify(logRows,null,2)],{type:'application/json'}),url=URL.createObjectURL(blob),a=document.createElement('a');a.href=url;a.download=`${cleanName($('logName').value)}-${logStamp()}.json`;document.body.appendChild(a);a.click();a.remove();URL.revokeObjectURL(url);log('Log downloaded')};window.onresize=()=>defs.forEach(draw);setup();setInterval(poll,300);poll();setOtaStatus('Use a signed MCUboot image from the Zephyr build output.');log('Console loaded; waiting for device data');
</script></body></html>)HTML";

static bool request_path_is(const struct http_client_ctx *client, const char *path)
{
	const char *url = (const char *)client->url_buffer;
	size_t i = 0;

	while (path[i] != '\0') {
		if (url[i] == '\0' || url[i] == '?') {
			return false;
		}
		if (url[i] != path[i]) {
			return false;
		}
		i++;
	}

	return url[i] == '\0' || url[i] == '?';
}

static void set_json_response(struct http_response_ctx *response_ctx, const char *json)
{
	static const struct http_header headers[] = {
		{ .name = "content-type", .value = "application/json" },
		{ .name = "cache-control", .value = "no-store" },
	};

	response_ctx->status = HTTP_200_OK;
	response_ctx->headers = headers;
	response_ctx->header_count = ARRAY_SIZE(headers);
	response_ctx->body = (const uint8_t *)json;
	response_ctx->body_len = strlen(json);
	response_ctx->final_chunk = true;
}

static int root_handler(struct http_client_ctx *client, enum http_transaction_status status,
			const struct http_request_ctx *request_ctx,
			struct http_response_ctx *response_ctx, void *user_data)
{
	ARG_UNUSED(client);
	ARG_UNUSED(request_ctx);
	ARG_UNUSED(user_data);

	if (status == HTTP_SERVER_REQUEST_DATA_FINAL) {
		LOG_INF("HTTP request path: %s", (const char *)client->url_buffer);
		if (request_path_is(client, "/") || request_path_is(client, "/index.html")) {
			static const struct http_header headers[] = {
				{ .name = "content-type", .value = "text/html; charset=utf-8" },
				{ .name = "cache-control", .value = "no-store, no-cache, must-revalidate" },
			};

			response_ctx->status = HTTP_200_OK;
			response_ctx->headers = headers;
			response_ctx->header_count = ARRAY_SIZE(headers);
			response_ctx->body = (const uint8_t *)dashboard_html;
			response_ctx->body_len = strlen(dashboard_html);
			response_ctx->final_chunk = true;
			return 0;
		}

		if (request_path_is(client, "/api/sample")) {
			static char json[1024];
			struct vislog_sample sample;

			vislog_app_state_copy(&sample);
			(void)vislog_sample_to_json(&sample, json, sizeof(json));
			set_json_response(response_ctx, json);
			return 0;
		}

		if (request_path_is(client, "/api/led")) {
			char body[512];
			size_t copy_len = 0;
			int ret = 0;
			size_t request_len = request_ctx != NULL ? request_ctx->data_len : 0;

			if (request_ctx != NULL && request_ctx->data != NULL && request_ctx->data_len > 0) {
				copy_len = MIN(request_ctx->data_len, sizeof(body) - 1);
				memcpy(body, request_ctx->data, copy_len);
				body[copy_len] = '\0';
				ret = vislog_led_apply_json(body);
			} else {
				body[0] = '\0';
				ret = vislog_led_apply_json(body);
			}

			LOG_INF("LED command received (%zu bytes, ret=%d)", request_len, ret);
			set_json_response(response_ctx, "{\"ok\":true}");
			return 0;
		}

		if (request_path_is(client, "/api/ota")) {
			if (request_ctx != NULL && request_ctx->data_len > 0) {
				int ota_ret = vislog_ota_handle_http(client, status, request_ctx,
								    response_ctx);
				if (ota_ret != 0) {
					static char err_json[160];
					snprintf(err_json, sizeof(err_json),
						 "{\"ok\":false,\"error\":%d}", ota_ret);
					set_json_response(response_ctx, err_json);
				}
				return 0;
			}

			int ota_ret = vislog_ota_handle_http(client, status, request_ctx,
							      response_ctx);
			if (ota_ret == 0 && response_ctx->body == NULL) {
				return 0;
			}
			return ota_ret;
		}

		if (request_path_is(client, "/api/ota/reboot")) {
			static char json[160];
			int ota_ret = vislog_ota_schedule_reboot();

			if (ota_ret == 0) {
				snprintf(json, sizeof(json), "{\"ok\":true,\"rebooting\":true}");
			} else {
				snprintf(json, sizeof(json),
					 "{\"ok\":false,\"error\":%d}", ota_ret);
			}
			set_json_response(response_ctx, json);
			return 0;
		}

		response_ctx->status = HTTP_404_NOT_FOUND;
		response_ctx->final_chunk = true;
	}

	return 0;
}
static struct http_resource_detail_dynamic root_detail = {
	.common = {
		.bitmask_of_supported_http_methods = BIT(HTTP_GET) | BIT(HTTP_POST),
		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		.content_type = "text/html; charset=utf-8",
	},
	.cb = root_handler,
};

HTTP_SERVICE_DEFINE_EMPTY(vislog_http_service, NULL, &http_port, 2, 4, NULL, &root_detail.common,
			  NULL);

static void wifi_event_handler(struct net_mgmt_event_callback *cb, uint64_t event,
			       struct net_if *iface)
{
	ARG_UNUSED(cb);
	ARG_UNUSED(iface);

	if (event == NET_EVENT_WIFI_AP_ENABLE_RESULT) {
		LOG_INF("Wi-Fi AP enabled");
	} else if (event == NET_EVENT_WIFI_AP_STA_CONNECTED) {
		const struct wifi_ap_sta_info *sta_info =
			reinterpret_cast<const struct wifi_ap_sta_info *>(cb->info);
		LOG_INF("Station connected: %02x:%02x:%02x:%02x:%02x:%02x", sta_info->mac[0],
			sta_info->mac[1], sta_info->mac[2], sta_info->mac[3], sta_info->mac[4],
			sta_info->mac[5]);
	}
}

static void start_wifi_ap(void)
{
	static struct wifi_connect_req_params ap_config;
	struct in_addr addr;
	struct in_addr netmask;
	int ret;

	ap_iface = net_if_get_wifi_sap();
	if (ap_iface == NULL) {
		LOG_ERR("No Wi-Fi AP interface available");
		return;
	}

	ap_config.ssid = (const uint8_t *)WIFI_AP_SSID;
	ap_config.ssid_length = sizeof(WIFI_AP_SSID) - 1;
	ap_config.psk = (const uint8_t *)WIFI_AP_PASSWORD;
	ap_config.psk_length = sizeof(WIFI_AP_PASSWORD) - 1;
	ap_config.channel = WIFI_AP_CHANNEL;
	ap_config.band = WIFI_FREQ_BAND_2_4_GHZ;
	ap_config.security = WIFI_SECURITY_TYPE_PSK;

	ret = net_mgmt(NET_REQUEST_WIFI_AP_ENABLE, ap_iface, &ap_config,
		       sizeof(ap_config));
	if (ret != 0) {
		LOG_ERR("NET_REQUEST_WIFI_AP_ENABLE failed: %d", ret);
		return;
	}

	if (net_addr_pton(AF_INET, "192.168.4.1", &addr) != 0) {
		LOG_ERR("Invalid AP address");
		return;
	}
	if (net_addr_pton(AF_INET, "255.255.255.0", &netmask) != 0) {
		LOG_ERR("Invalid AP netmask");
		return;
	}

	net_if_ipv4_set_gw(ap_iface, &addr);
	if (net_if_ipv4_addr_add(ap_iface, &addr, NET_ADDR_MANUAL, 0) == NULL) {
		LOG_ERR("Failed to assign AP IPv4 address");
		return;
	}
	if (!net_if_ipv4_set_netmask_by_addr(ap_iface, &addr, &netmask)) {
		LOG_ERR("Failed to assign AP netmask");
		return;
	}

	addr.s4_addr[3] += 10;
	ret = net_dhcpv4_server_start(ap_iface, &addr);
	if (ret != 0) {
		LOG_ERR("Failed to start DHCPv4 server: %d", ret);
	}
}

extern "C" void vislog_net_services_init(void)
{
	net_mgmt_init_event_callback(&wifi_cb, wifi_event_handler,
				     NET_EVENT_WIFI_AP_ENABLE_RESULT |
					     NET_EVENT_WIFI_AP_STA_CONNECTED);
	net_mgmt_add_event_callback(&wifi_cb);

	start_wifi_ap();

	if (http_server_start() != 0) {
		LOG_ERR("HTTP server did not start");
	} else {
		LOG_INF("Dashboard: http://192.168.4.1/");
	}
}

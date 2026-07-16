#include "oled_stream.h"
#include "esphome/core/log.h"

namespace esphome {
namespace oled_stream {

static const char *const TAG = "oled_stream";
static const uint32_t INACTIVITY_TIMEOUT_MS = 5000;
static const uint32_t FRAME_INTERVAL_MS = 33;
static const uint8_t SYNC_BYTE = 0xAA;

static const char EMULATOR_HTML_TEMPLATE[] PROGMEM = R"HTMLDOC(<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>Emulateur OLED</title>
<style>
* { margin: 0; padding: 0; box-sizing: border-box; }
body { background: radial-gradient(circle at center, #2a2a2a 0%, #0a0a0a 100%); display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100vh; font-family: sans-serif; }
.main-row { display: flex; align-items: flex-start; gap: 24px; }
.device { background: linear-gradient(145deg, #3a3a38, #1c1c1a); border-radius: 18px; padding: 22px 22px 40px 22px; box-shadow: 0 10px 30px rgba(0,0,0,0.6), inset 0 1px 1px rgba(255,255,255,0.08), inset 0 -2px 4px rgba(0,0,0,0.5); position: relative; }
.pins { text-align: center; font-size: 9px; letter-spacing: 4px; color: #888; margin-bottom: 6px; font-family: monospace; }
.screen-frame { background: #000; border-radius: 4px; padding: 6px; box-shadow: inset 0 0 8px rgba(0,0,0,0.9), 0 0 0 1px #444; }
#screen { display: block; border-radius: 2px; background: #000; image-rendering: pixelated; }
.bracket { position: absolute; left: -14px; right: -14px; bottom: 12px; height: 10px; background: linear-gradient(180deg, #4a4a48, #222); border-radius: 4px; box-shadow: 0 2px 4px rgba(0,0,0,0.5); }
.label { text-align: center; color: #666; font-size: 11px; margin-top: 14px; letter-spacing: 1px; }
.contrast-box { display: flex; flex-direction: column; align-items: center; }
.side-panel { display: flex; flex-direction: column; gap: 16px; }
.stats-panel, .decode-panel { background: #1a1a1a; border: 1px solid #333; border-radius: 8px; padding: 14px 18px; font-family: monospace; font-size: 12px; color: #ccc; min-width: 240px; }
.stats-panel h3, .decode-panel h3 { color: #29E5FF; font-size: 11px; letter-spacing: 2px; margin-bottom: 10px; font-weight: normal; }
.stat-row { display: flex; justify-content: space-between; padding: 3px 0; border-bottom: 1px solid #2a2a2a; }
.stat-row span:first-child { color: #888; }
.stat-row span:last-child { color: #eee; font-weight: bold; }
select, .decode-panel input { background: #0f0f0f; color: #29E5FF; border: 1px solid #444; border-radius: 4px; padding: 4px 6px; font-family: monospace; font-size: 11px; width: 100%; margin-top: 3px; margin-bottom: 8px; }
.decode-panel label { display: block; color: #888; font-size: 10px; margin-top: 6px; }
.badge { display: inline-block; padding: 2px 8px; border-radius: 10px; font-size: 10px; font-weight: bold; }
.badge-auto { background: #1a4d2e; color: #4ade80; }
.badge-manual { background: #4d3319; color: #fbbf24; }
.badge-low { background: #4d1919; color: #ff6b6b; }
#customFields { display: none; }
</style>
</head>
<body>
<div class="main-row">
  <div class="device">
    <div class="pins">GND &nbsp; VCC &nbsp; SCL &nbsp; SDA</div>
    <div class="screen-frame"><canvas id="screen"></canvas></div>
    <div class="bracket"></div>
  </div>

  <div class="contrast-box">
    <div style="color: #888; font-size: 11px; margin-bottom: 8px; font-family: monospace;">CONTRASTE</div>
    <div style="width: 30px; height: 180px; background: #1a1a1a; border-radius: 6px; border: 1px solid #444; position: relative; overflow: hidden;">
      <div id="contrastFill" style="position: absolute; bottom: 0; width: 100%; background: linear-gradient(180deg, #29E5FF, #1a8bb0); transition: height 0.15s ease;"></div>
    </div>
    <div id="contrastLabel" style="color: #29E5FF; font-size: 16px; font-weight: bold; margin-top: 10px; font-family: monospace;">0%</div>
  </div>

  <div class="side-panel">
    <div class="decode-panel">
      <h3>DÉCODAGE</h3>
      <div style="margin-bottom: 8px;">
        Détecté: <span id="detectedInfo">-</span>
        <span id="confidenceBadge" class="badge badge-auto">AUTO</span>
      </div>
      <label>Mode de décodage</label>
      <select id="decodeMode">
        <option value="auto">Auto (recommandé)</option>
        <option value="mono_page">Mono - Page (SSD1306/SH1106)</option>
        <option value="mono_row">Mono - Ligne (LCD/ePaper)</option>
        <option value="rgb565_row">RGB565 - Ligne (couleur)</option>
        <option value="gray4_row">Gray4 - Ligne (expérimental)</option>
        <option value="custom">Custom...</option>
      </select>

      <div id="customFields">
        <label>Bits par pixel</label>
        <select id="customBpp">
          <option value="1">1 (monochrome)</option>
          <option value="4">4 (16 niveaux gris)</option>
          <option value="8">8 (256 niveaux/couleurs)</option>
          <option value="16">16 (RGB565)</option>
        </select>
        <label>Organisation</label>
        <select id="customOrg">
          <option value="page">Page (8 lignes/octet, vertical)</option>
          <option value="row">Ligne (horizontal)</option>
        </select>
        <label>Ordre des bits</label>
        <select id="customBitOrder">
          <option value="lsb">LSB en premier</option>
          <option value="msb">MSB en premier</option>
        </select>
      </div>
    </div>

    <div class="stats-panel">
      <h3>STATISTIQUES RÉSEAU</h3>
      <div class="stat-row"><span>Type d'afficheur</span><span id="statType">-</span></div>
      <div class="stat-row"><span>Dimensions</span><span id="statDims">-</span></div>
      <div class="stat-row"><span>Taille buffer</span><span id="statBufSize">-</span></div>
      <div class="stat-row"><span>FPS</span><span id="statFps">-</span></div>
      <div class="stat-row"><span>Latence connexion</span><span id="statLatency">-</span></div>
      <div class="stat-row"><span>Jitter</span><span id="statJitter">-</span></div>
      <div class="stat-row"><span>Débit</span><span id="statThroughput">-</span></div>
      <div class="stat-row"><span>Frames reçues</span><span id="statFrames">-</span></div>
      <div class="stat-row"><span>Resyncs</span><span id="statResyncs">-</span></div>
      <div class="stat-row"><span>Reconnexions</span><span id="statReconnects">-</span></div>
    </div>
  </div>
</div>
<div class="label">Emulateur OLED - oled_stream</div>

<script>
const SCALE = 4;
const GAP = 1;
const SYNC_BYTE = 0xAA;
const canvas = document.getElementById('screen');
const ctx = canvas.getContext('2d');
ctx.imageSmoothingEnabled = false;

let DISPLAY_WIDTH = 128;
let DISPLAY_HEIGHT = 64;
let FRAME_SIZE = 1026;

// Ce que l'ESP a détecté automatiquement (la "suggestion")
let detectedLayout = "mono_page";
let detectedBpp = 1;

// Ce qui est réellement utilisé pour décoder (peut être overridé manuellement)
let activeLayout = "mono_page";
let activeBpp = 1;
let activeOrg = "page";
let activeBitOrder = "lsb";

let lastFrameTime = null;
let frameIntervals = [];
let frameCount = 0;
let reconnectCount = 0;
let resyncCount = 0;
let fpsWindow = [];

// ===== Décodeurs par disposition connue =====
function decodeMonoPage(bytes, contrast) {
  for (let page = 0; page < DISPLAY_HEIGHT / 8; page++) {
    for (let x = 0; x < DISPLAY_WIDTH; x++) {
      const byteValue = bytes[page * DISPLAY_WIDTH + x];
      for (let bit = 0; bit < 8; bit++) {
        if ((byteValue >> bit) & 1) {
          const y = page * 8 + bit;
          ctx.fillStyle = (y < 16) ? "#F5C518" : "#29E5FF";
          ctx.globalAlpha = Math.max(0.5, contrast);
          ctx.fillRect(x * SCALE, y * SCALE, SCALE - GAP, SCALE - GAP);
        }
      }
    }
  }
  ctx.globalAlpha = 1.0;
}

function decodeMonoRow(bytes, contrast, bitOrder) {
  const bytesPerRow = Math.ceil(DISPLAY_WIDTH / 8);
  ctx.fillStyle = "#29E5FF";
  ctx.globalAlpha = Math.max(0.5, contrast);
  for (let y = 0; y < DISPLAY_HEIGHT; y++) {
    for (let x = 0; x < DISPLAY_WIDTH; x++) {
      const byteIndex = y * bytesPerRow + Math.floor(x / 8);
      const bitIndex = bitOrder === "msb" ? (7 - (x % 8)) : (x % 8);
      if ((bytes[byteIndex] >> bitIndex) & 1) {
        ctx.fillRect(x * SCALE, y * SCALE, SCALE - GAP, SCALE - GAP);
      }
    }
  }
  ctx.globalAlpha = 1.0;
}

function decodeRgb565Row(bytes, contrast) {
  const imgData = ctx.createImageData(DISPLAY_WIDTH, DISPLAY_HEIGHT);
  for (let i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; i++) {
    const pixel = (bytes[i * 2] << 8) | bytes[i * 2 + 1];
    imgData.data[i * 4] = (((pixel >> 11) & 0x1F) << 3) * contrast;
    imgData.data[i * 4 + 1] = (((pixel >> 5) & 0x3F) << 2) * contrast;
    imgData.data[i * 4 + 2] = ((pixel & 0x1F) << 3) * contrast;
    imgData.data[i * 4 + 3] = 255;
  }
  const tempCanvas = document.createElement('canvas');
  tempCanvas.width = DISPLAY_WIDTH;
  tempCanvas.height = DISPLAY_HEIGHT;
  tempCanvas.getContext('2d').putImageData(imgData, 0, 0);
  ctx.drawImage(tempCanvas, 0, 0, canvas.width, canvas.height);
}

function decodeGray4Row(bytes, contrast) {
  const bytesPerRow = Math.ceil(DISPLAY_WIDTH / 2);
  for (let y = 0; y < DISPLAY_HEIGHT; y++) {
    for (let x = 0; x < DISPLAY_WIDTH; x++) {
      const byteIndex = y * bytesPerRow + Math.floor(x / 2);
      const nibble = (x % 2 === 0) ? (bytes[byteIndex] >> 4) : (bytes[byteIndex] & 0x0F);
      const gray = Math.round((nibble / 15) * 255 * contrast);
      ctx.fillStyle = `rgb(${gray},${gray},${gray})`;
      ctx.fillRect(x * SCALE, y * SCALE, SCALE - GAP, SCALE - GAP);
    }
  }
}

// ===== Décodeur générique CUSTOM — construit à partir des 3 paramètres manuels =====
function decodeCustom(bytes, contrast, bpp, org, bitOrder) {
  if (bpp === 1 && org === "page") {
    decodeMonoPage(bytes, contrast);
  } else if (bpp === 1 && org === "row") {
    decodeMonoRow(bytes, contrast, bitOrder);
  } else if (bpp === 16) {
    decodeRgb565Row(bytes, contrast);
  } else if (bpp === 4) {
    decodeGray4Row(bytes, contrast);
  } else if (bpp === 8) {
    // Grayscale 8 bits générique, ligne par ligne
    for (let y = 0; y < DISPLAY_HEIGHT; y++) {
      for (let x = 0; x < DISPLAY_WIDTH; x++) {
        const gray = Math.round(bytes[y * DISPLAY_WIDTH + x] * contrast);
        ctx.fillStyle = `rgb(${gray},${gray},${gray})`;
        ctx.fillRect(x * SCALE, y * SCALE, SCALE - GAP, SCALE - GAP);
      }
    }
  }
}

function drawFrame(bytes) {
  ctx.fillStyle = "#000000";
  ctx.fillRect(0, 0, canvas.width, canvas.height);
  const contrast = bytes[FRAME_SIZE - 2] / 255.0;

  const mode = document.getElementById('decodeMode').value;

  if (mode === "auto") {
    activeLayout = detectedLayout;
    switch (detectedLayout) {
      case "mono_page": decodeMonoPage(bytes, contrast); break;
      case "mono_row": decodeMonoRow(bytes, contrast, "msb"); break;
      case "rgb565_row": decodeRgb565Row(bytes, contrast); break;
      case "gray4_row": decodeGray4Row(bytes, contrast); break;
      default:
        ctx.fillStyle = "#ff6b6b";
        ctx.font = "14px monospace";
        ctx.fillText(`Disposition "${detectedLayout}" inconnue — essayez le mode Custom`, 10, 20);
    }
  } else if (mode === "custom") {
    const bpp = parseInt(document.getElementById('customBpp').value, 10);
    const org = document.getElementById('customOrg').value;
    const bitOrder = document.getElementById('customBitOrder').value;
    decodeCustom(bytes, contrast, bpp, org, bitOrder);
  } else {
    // Un des modes prédéfinis choisi manuellement (override direct de l'auto)
    switch (mode) {
      case "mono_page": decodeMonoPage(bytes, contrast); break;
      case "mono_row": decodeMonoRow(bytes, contrast, "msb"); break;
      case "rgb565_row": decodeRgb565Row(bytes, contrast); break;
      case "gray4_row": decodeGray4Row(bytes, contrast); break;
    }
  }

  const percent = Math.round(contrast * 100);
  document.getElementById('contrastFill').style.height = percent + '%';
  document.getElementById('contrastLabel').textContent = percent + '%';
}

function updateStats() {
  const now = performance.now();
  frameCount++;
  document.getElementById('statFrames').textContent = frameCount;

  if (lastFrameTime !== null) {
    const interval = now - lastFrameTime;
    frameIntervals.push(interval);
    if (frameIntervals.length > 30) frameIntervals.shift();

    const avg = frameIntervals.reduce((a, b) => a + b, 0) / frameIntervals.length;
    const variance = frameIntervals.reduce((a, b) => a + (b - avg) ** 2, 0) / frameIntervals.length;
    const jitter = Math.sqrt(variance);
    document.getElementById('statJitter').textContent = `${jitter.toFixed(1)} ms`;

    const throughput = (FRAME_SIZE * 1000) / interval;
    document.getElementById('statThroughput').textContent = `${(throughput / 1024).toFixed(2)} Ko/s`;

    fpsWindow.push(now);
    fpsWindow = fpsWindow.filter(t => now - t < 2000);
    document.getElementById('statFps').textContent = (fpsWindow.length / 2).toFixed(1);
  }

  lastFrameTime = now;
}

// ===== Gestion du panneau de décodage =====
document.getElementById('decodeMode').addEventListener('change', function() {
  const isCustom = this.value === "custom";
  document.getElementById('customFields').style.display = isCustom ? "block" : "none";

  const badge = document.getElementById('confidenceBadge');
  if (this.value === "auto") {
    badge.textContent = "AUTO";
    badge.className = "badge badge-auto";
  } else {
    badge.textContent = "MANUEL";
    badge.className = "badge badge-manual";
  }
});

async function streamLoop() {
  const connectStart = performance.now();
  try {
    const res = await fetch('/buffer');
    const latency = performance.now() - connectStart;
    document.getElementById('statLatency').textContent = `${latency.toFixed(1)} ms`;

    const displayType = res.headers.get('X-Display-Type') || 'inconnu';
    const layout = res.headers.get('X-Buffer-Layout') || 'mono_page';
    const bpp = parseInt(res.headers.get('X-Bits-Per-Pixel') || '1', 10);
    const width = parseInt(res.headers.get('X-Display-Width') || '128', 10);
    const height = parseInt(res.headers.get('X-Display-Height') || '64', 10);
    const bufSize = parseInt(res.headers.get('X-Buffer-Size') || '1024', 10);

    DISPLAY_WIDTH = width;
    DISPLAY_HEIGHT = height;
    FRAME_SIZE = bufSize + 2;
    detectedLayout = layout;
    detectedBpp = bpp;

    canvas.width = width * SCALE;
    canvas.height = height * SCALE;

    document.getElementById('statType').textContent = displayType;
    document.getElementById('statDims').textContent = `${width}x${height}`;
    document.getElementById('statBufSize').textContent = `${FRAME_SIZE} octets`;
    document.getElementById('detectedInfo').textContent = `${displayType} (${layout}, ${bpp}bpp)`;

    const reader = res.body.getReader();
    let buffer = new Uint8Array(0);

    while (true) {
      const { done, value } = await reader.read();
      if (done) break;

      const combined = new Uint8Array(buffer.length + value.length);
      combined.set(buffer);
      combined.set(value, buffer.length);
      buffer = combined;

      while (buffer.length >= FRAME_SIZE) {
        if (buffer[0] !== SYNC_BYTE) {
          let syncIndex = -1;
          for (let i = 1; i < buffer.length; i++) {
            if (buffer[i] === SYNC_BYTE) { syncIndex = i; break; }
          }
          if (syncIndex === -1) { buffer = new Uint8Array(0); break; }
          resyncCount++;
          document.getElementById('statResyncs').textContent = resyncCount;
          buffer = buffer.slice(syncIndex);
          continue;
        }

        const frame = buffer.slice(1, FRAME_SIZE);
        drawFrame(frame);
        updateStats();
        buffer = buffer.slice(FRAME_SIZE);
      }
    }
  } catch (e) {
    console.error("Flux interrompu:", e);
  }

  reconnectCount++;
  document.getElementById('statReconnects').textContent = reconnectCount;
  setTimeout(streamLoop, 300);
}

streamLoop();
</script>
</body>
</html>
)HTMLDOC";

void OledStream::setup() {
  this->buffer_size_ = ((uint32_t)this->width_ * this->height_ * this->bits_per_pixel_) / 8;
  this->send_buffer_ = new uint8_t[this->buffer_size_ + 2];
  this->server_ = new WiFiServer(this->port_);
  this->server_->begin();
  ESP_LOGI(TAG, "OledStream démarré: %s (%s, %ubpp), %ux%u, buffer: %u octets",
           this->display_type_.c_str(), this->buffer_layout_.c_str(),
           this->bits_per_pixel_, this->width_, this->height_, this->buffer_size_);
}

void OledStream::loop() {
  if (this->server_ == nullptr) return;
  uint32_t now = millis();

  WiFiClient new_client = this->server_->available();
  if (new_client) {
    unsigned long timeout = millis() + 50;
    String request = "";
    while (new_client.connected() && millis() < timeout) {
      if (new_client.available()) {
        char c = new_client.read();
        request += c;
        if (request.endsWith("\r\n\r\n")) break;
      }
    }

    if (request.indexOf("GET /buffer") >= 0) {
      if (this->stream_client_.connected()) {
        this->stream_client_.stop();
      }
      new_client.setNoDelay(true);
      new_client.print("HTTP/1.1 200 OK\r\n");
      new_client.print("Content-Type: application/octet-stream\r\n");
      new_client.print("Access-Control-Allow-Origin: *\r\n");
      new_client.print("Access-Control-Expose-Headers: X-Display-Type, X-Buffer-Layout, X-Display-Width, X-Display-Height, X-Bits-Per-Pixel, X-Buffer-Size\r\n");
      new_client.printf("X-Display-Type: %s\r\n", this->display_type_.c_str());
      new_client.printf("X-Buffer-Layout: %s\r\n", this->buffer_layout_.c_str());
      new_client.printf("X-Display-Width: %u\r\n", this->width_);
      new_client.printf("X-Display-Height: %u\r\n", this->height_);
      new_client.printf("X-Bits-Per-Pixel: %u\r\n", this->bits_per_pixel_);
      new_client.printf("X-Buffer-Size: %u\r\n", this->buffer_size_);
      new_client.print("Connection: keep-alive\r\n\r\n");
      this->stream_client_ = new_client;
      this->last_frame_sent_ = 0;
      this->last_activity_ = now;
    } else {
      this->send_html_(new_client);
      new_client.stop();
    }
  }

  if (this->stream_client_ && this->stream_client_.connected()) {
    if (now - this->last_frame_sent_ >= FRAME_INTERVAL_MS) {
      this->last_frame_sent_ = now;
      uint8_t *raw_buffer = get_oled_buffer(this->display_);
      this->send_buffer_[0] = SYNC_BYTE;
      memcpy(&this->send_buffer_[1], raw_buffer, this->buffer_size_);
      float contrast = this->contrast_fn_ ? this->contrast_fn_() : 1.0f;
      this->send_buffer_[this->buffer_size_ + 1] = (uint8_t)(contrast * 255.0f);
      size_t total_size = this->buffer_size_ + 2;
      size_t written = this->stream_client_.write(this->send_buffer_, total_size);
      if (written == total_size) this->last_activity_ = now;
    }
    if (now - this->last_activity_ > INACTIVITY_TIMEOUT_MS) {
      this->stream_client_.stop();
    }
  } else if (this->stream_client_) {
    this->stream_client_.stop();
  }
}

void OledStream::send_html_(WiFiClient &client) {
  client.print("HTTP/1.1 200 OK\r\n");
  client.print("Content-Type: text/html\r\n");
  client.print("Connection: close\r\n\r\n");
  for (size_t i = 0; i < sizeof(EMULATOR_HTML_TEMPLATE); i++) {
    client.write(pgm_read_byte(&EMULATOR_HTML_TEMPLATE[i]));
  }
}

}  // namespace oled_stream
}  // namespace esphome

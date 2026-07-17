<img width="1262" height="833" alt="image" src="https://github.com/user-attachments/assets/c469e86f-0618-4c50-aa8b-49440daef55a" />

# oled_stream

[![CI](https://github.com/kaboom748/oled_stream/actions/workflows/ci.yml/badge.svg)](https://github.com/kaboom748/oled_stream/actions/workflows/ci.yml)

An [ESPHome](https://esphome.io) component that mirrors a display's framebuffer
into a web browser, served by the ESP itself.

No cloud, no MQTT, no Home Assistant, no extra hardware. Point a browser at
`http://<device-ip>:8080/` and you get a pixel-exact copy of what is on the glass.

## Why

Small displays end up in awkward places: inside an enclosure, behind a panel, at
the top of a pole. `oled_stream` gives you the screen wherever you are on the
network — for debugging a `lambda:`, for checking a device you cannot reach, or
just for putting the screen on a dashboard.

## How it works

The component reads the display's raw framebuffer and pushes it over a
keep-alive HTTP stream at ~30 fps. The browser page is a small self-contained
viewer that decodes the raw bytes back into pixels — the same work the display
driver does on the device.

| Endpoint | Serves |
|---|---|
| `GET /` | the viewer page (~15 KB, self-contained, no CDN) |
| `GET /buffer` | keep-alive `application/octet-stream` of raw frames |

Each frame is a sync byte, the framebuffer, and the current contrast value. The
stream response carries `X-Display-*` headers so the viewer knows the geometry
and layout without being told.

## Supported hardware

| Platform | Framework | Status |
|---|---|---|
| ESP8266 | Arduino | tested on hardware |
| ESP32 | Arduino | supported |
| ESP32 | ESP-IDF | should work — the ESP32 path has no Arduino dependency, but it has not been tested on hardware |

Any display built on ESPHome's `DisplayBuffer` should work. The buffer layout is
detected from the display platform:

| Layout | Displays |
|---|---|
| `mono_page` | SSD1306, SSD1305, PCD8544 (Nokia 5110) |
| `mono_row` | ST7567, ST7920, Waveshare ePaper, Inkplate, MAX7219 |
| `gray4_row` | SSD1322, SSD1325, SSD1327 |
| `rgb565_row` | ILI9XXX, ST7735, ST7789V, SSD1331, SSD1351 |

If your display is not in the list, the viewer lets you pick the layout by hand.

> **A note on the name.** `oled_stream` also handles RGB565 TFTs, which are not
> OLEDs. The name is historical.

> **Memory.** The component allocates a copy of the framebuffer. That is 1 KB for
> a 128x64 mono display, but 150 KB for a 320x240 RGB565 one — more than an
> ESP8266 has. On a large colour display, use an ESP32. If the allocation fails
> the component marks itself failed and logs it, rather than crashing.

## Installation

```yaml
external_components:
  - source: github://kaboom748/oled_stream
    components: [oled_stream]
    refresh: 1d
```

## Configuration

```yaml
display:
  - platform: ssd1306_i2c
    model: "SSD1306 128x64"
    address: 0x3C
    id: oled_display

oled_stream:
  - display_id: oled_display
    port: 8080
```

| Option | Type | Default | Description |
|---|---|---|---|
| `display_id` | ID | **required** | the display to mirror |
| `port` | port | `8080` | TCP port for the viewer |
| `contrast_lambda` | lambda | — | returns a `float` in `0.0`–`1.0`, applied by the viewer |

`oled_stream` is `MULTI_CONF`: mirror several displays by declaring several
blocks on different ports.

### Why there is no `width` / `height`

There used to be. The display already knows its own geometry, and asking the user
for it again meant a wrong answer was accepted silently — and then the component
read past the end of the framebuffer, thirty times a second. The dimensions now
come from `get_native_width()` / `get_native_height()`, so the two can no longer
disagree.

> **Upgrading?** Delete `width:` and `height:` from your YAML. They are now
> rejected.

### Contrast

ESPHome applies `contrast:` on the panel itself, and it cannot be read back — so
the viewer has no way to know about it. If you change contrast at runtime, feed
the same value to `contrast_lambda` and the browser will match the glass:

```yaml
globals:
  - id: current_contrast
    type: float
    initial_value: "1.0"

oled_stream:
  - display_id: oled_display
    contrast_lambda: |-
      return id(current_contrast);
```

## Limitations

- **Framework.** The ESP8266 path uses Arduino `WiFiServer`; ESP8266 with
  ESP-IDF is not supported (ESPHome does not support that combination anyway).
- **One viewer at a time.** A second `GET /buffer` replaces the first.
- **Bandwidth.** A 128x64 mono display is ~1 KB per frame — around 250 kbit/s at
  30 fps. A large RGB565 panel will saturate the radio; raise the frame interval
  if you try.
- **No authentication.** Anyone on your network can watch the screen. Do not
  expose the port to the internet.

## Known deviation from ESPHome upstream

`display::DisplayBuffer::buffer_` is `protected` and has no public accessor, so
this component reaches it by casting to a derived type the object is not an
instance of. **That is undefined behaviour.** It works in practice, but it is
also why this component is not a candidate for ESPHome upstream as-is: a clean
version needs a small upstream change adding a public `get_buffer()` and
`get_buffer_length()` to `DisplayBuffer`. That conversation belongs on the
ESPHome Discord `#devs` channel before any PR.

If you are here to judge whether to depend on this: the cast is the one thing in
the codebase that is not defensible on its own terms.

## Development

```bash
esphome config tests/test.esp8266.yaml
ruff check components/oled_stream/
```

`tests/test_oled.cpp` is a host-side test that replays a real browser request
against the component and checks the response. It exists because a fixed 256-byte
request buffer once looked perfectly fine and silently broke every request a real
browser made — a browser `GET` is around 500 bytes, and the terminator never
arrived.

## License

Same model as ESPHome: the C++ / runtime code (`.cpp`, `.h`, and the embedded
viewer) is **GPLv3**, the Python code and everything else is **MIT**. See
[LICENSE](LICENSE).

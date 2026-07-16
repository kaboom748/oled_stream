oled_stream2 for ESP32

oled_stream for ESP8266


https://github.com/user-attachments/assets/cfa111de-d13b-4607-85d4-f531fc8dd0be

# ESPHome SSD1306 Remote Display Viewer for ESP8266

This ESPHome component is designed for **ESP8266 devices** and allows remote visualization of an **I²C SSD1306 OLED display** directly from a web browser.

The component captures the exact content of the OLED display buffer and hosts a lightweight web server directly on the ESP8266. No external server, Home Assistant dashboard, or additional hardware is required.

Simply connect to:

```
http://ESP8266_IP_ADDRESS:8080/
```

and the browser will display a remote copy of the OLED screen.

## Features

* Designed specifically for ESP8266
* Supports SSD1306 I²C OLED displays
* Displays an exact replica of the OLED framebuffer
* Optional framebuffer-based rendering support
* Built-in web server hosted directly on the ESP8266
* Access the display remotely from any browser on the network
* No MQTT, Home Assistant, or external services required

## Use cases

This can be useful for:

* Monitoring small OLED displays installed in inaccessible locations
* Debugging ESPHome displays remotely
* Creating remote dashboards for embedded devices
* Sharing an OLED display view over a local network

The goal of this project is simple: **make a physical SSD1306 display viewable remotely with pixel-perfect accuracy through a web browser.**
----------------------------------------------------------------------------------------------------------------------------------------------------------------------------



-Cette library ESPHOME est conçue pour ESP8266.
-Elle permet d'afficher le contenu exact d'un afficheur i2c SSD1306 et/ou avec support de framebuffer (pas MiPiSpi)
-Le ESP8266 va lui même héberger le site web de visualisation sur le port spécifié dans le module.
http://IP_ESP8266:8080/

En gros cela permet d'avoir une copie EXACTE de l'afficheur i2c à distance via son navigateur internet.




<img width="1280" height="860" alt="image" src="https://github.com/user-attachments/assets/f52e8a1a-6cf4-41e4-9fe5-8d62b1ed4a1c" />


````markdown

esphome:
  name: wemo-test
  friendly_name: WEMO-TEST
  platformio_options:
    board_build.f_cpu: 160000000L #Pour les effets led 160mhz plus rapide
      
esp8266:
  board: d1_mini
  framework:
    version: recommended

globals:
  - id: current_contrast   # état interne du contrast
    type: float
    restore_value: no
    initial_value: '1.0'

display:
  - platform: ssd1306_i2c
    model: "SSD1306 128x64"
    address: 0x3C
    id: oled_display
    rotation: 0
    update_interval: 200ms    # ← ajouté
    contrast: 100%    # luminosité de base

external_components:
  - source:
      type: git
      url: https://github.com/kaboom748/oled_stream
      ref: main

oled_stream:
  - display_id: oled_display
    width: 128
    height: 64
    port: 8080
    contrast_lambda: |-
      return id(current_contrast);
````

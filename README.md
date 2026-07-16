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

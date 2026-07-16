````markdown
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

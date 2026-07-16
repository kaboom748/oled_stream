#pragma once

#include "esphome/core/component.h"
#include "esphome/components/display/display_buffer.h"
#include <WiFiServer.h>
#include <WiFiClient.h>
#include <functional>
#include <string>

namespace esphome {
namespace oled_stream {

class DisplayBufferAccessor : public display::DisplayBuffer {
 public:
  uint8_t* reveal_buffer() { return this->buffer_; }
};

inline uint8_t* get_oled_buffer(display::DisplayBuffer *disp) {
  return static_cast<DisplayBufferAccessor*>(disp)->reveal_buffer();
}

class OledStream : public Component {
 public:
  void set_display(display::DisplayBuffer *disp) { this->display_ = disp; }
  void set_port(uint16_t port) { this->port_ = port; }
  void set_contrast_lambda(std::function<float()> f) { this->contrast_fn_ = f; }
  void set_display_type(const std::string &type) { this->display_type_ = type; }
  void set_buffer_layout(const std::string &layout) { this->buffer_layout_ = layout; }
  void set_bits_per_pixel(uint8_t bpp) { this->bits_per_pixel_ = bpp; }
  void set_display_dimensions(uint16_t width, uint16_t height) {
    this->width_ = width;
    this->height_ = height;
  }

  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

 protected:
  void send_html_(WiFiClient &client);

  display::DisplayBuffer *display_{nullptr};
  std::function<float()> contrast_fn_{nullptr};
  std::string display_type_{"unknown"};
  std::string buffer_layout_{"mono_page"};
  uint8_t bits_per_pixel_{1};
  uint16_t width_{128};
  uint16_t height_{64};
  uint32_t buffer_size_{0};
  uint16_t port_{8080};
  WiFiServer *server_{nullptr};

  WiFiClient stream_client_;
  uint32_t last_frame_sent_{0};
  uint32_t last_activity_{0};
  uint8_t *send_buffer_{nullptr};
};

}  // namespace oled_stream
}  // namespace esphome

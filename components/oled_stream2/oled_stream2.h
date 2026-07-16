#pragma once

#include "esphome/core/component.h"
#include "esphome/components/display/display_buffer.h"
#include <functional>
#include <string>

#ifdef USE_ESP8266
#include <WiFiServer.h>
#include <WiFiClient.h>
#elif defined(USE_ESP32)
#include "esphome/components/socket/socket.h"
#include <memory>
#endif

namespace esphome {
namespace oled_stream2 {

class DisplayBufferAccessor : public display::DisplayBuffer {
 public:
  uint8_t* reveal_buffer() { return this->buffer_; }
};

inline uint8_t* get_oled_buffer(display::DisplayBuffer *disp) {
  return static_cast<DisplayBufferAccessor*>(disp)->reveal_buffer();
}

class OledStream2 : public Component {
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
  void send_html_(
#ifdef USE_ESP8266
    WiFiClient &client
#elif defined(USE_ESP32)
    std::unique_ptr<socket::Socket> &client
#endif
  );

  display::DisplayBuffer *display_{nullptr};
  std::function<float()> contrast_fn_{nullptr};
  std::string display_type_{"unknown"};
  std::string buffer_layout_{"mono_page"};
  uint8_t bits_per_pixel_{1};
  uint16_t width_{128};
  uint16_t height_{64};
  uint32_t buffer_size_{0};
  uint16_t port_{8081};

  uint32_t last_frame_sent_{0};
  uint32_t last_activity_{0};
  uint8_t *send_buffer_{nullptr};

#ifdef USE_ESP8266
  WiFiServer *server_{nullptr};
  WiFiClient stream_client_;
#elif defined(USE_ESP32)
  std::unique_ptr<socket::Socket> server_socket_;
  std::unique_ptr<socket::Socket> stream_client_;
#endif
};

}  // namespace oled_stream2
}  // namespace esphome

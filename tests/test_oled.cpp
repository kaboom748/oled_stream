#include <cstdio>
#include <string>

#include "esphome/components/oled_stream/oled_stream.h"
#include "esphome/components/logger/logger.h"

using namespace esphome;
using namespace esphome::oled_stream;

static int fails = 0;
static void check(const char *name, bool ok) {
  printf("  [%s] %s\n", ok ? " OK " : "FAIL", name);
  if (!ok)
    fails++;
}

// Un ecran factice : le composant ne lui demande que sa geometrie.
class FakeDisplay : public display::DisplayBuffer {
 public:
  void alloc(uint32_t n) { this->init_internal_(n); }
  int get_width_internal() override { return 128; }
  int get_height_internal() override { return 64; }
  void draw_absolute_pixel_internal(int x, int y, Color color) override {
    (void) x; (void) y; (void) color;
  }
  display::DisplayType get_display_type() override { return display::DISPLAY_TYPE_BINARY; }
  void update() override {}
};

// La requete GET qu'envoie reellement Chrome : 479 octets.
static const char *CHROME_GET =
    "GET / HTTP/1.1\r\n"
    "Host: 192.168.1.50:8080\r\n"
    "Connection: keep-alive\r\n"
    "Cache-Control: max-age=0\r\n"
    "Upgrade-Insecure-Requests: 1\r\n"
    "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) "
    "Chrome/120.0.0.0 Safari/537.36\r\n"
    "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,"
    "application/signed-exchange;v=b3;q=0.7\r\n"
    "Accept-Encoding: gzip, deflate\r\n"
    "Accept-Language: fr-FR,fr;q=0.9,en-US;q=0.8,en;q=0.7\r\n"
    "\r\n";

static const char *CHROME_BUFFER_GET =
    "GET /buffer HTTP/1.1\r\n"
    "Host: 192.168.1.50:8080\r\n"
    "Connection: keep-alive\r\n"
    "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) "
    "Chrome/120.0.0.0 Safari/537.36\r\n"
    "Accept: */*\r\n"
    "Accept-Encoding: gzip, deflate\r\n"
    "Accept-Language: fr-FR,fr;q=0.9\r\n"
    "\r\n";

int main() {
  logger::global_logger = new logger::Logger(0);
  logger::global_logger->set_log_level(0);

  FakeDisplay disp;
  disp.alloc(1024);

  printf("\nRequete Chrome de reference : %zu octets\n\n", strlen(CHROME_GET));

  // ---- 1. GET / doit renvoyer la page HTML ----
  {
    OledStream comp;
    comp.set_display(&disp);
    comp.set_port(8080);
    comp.set_bits_per_pixel(1);
    comp.setup();

    WiFiClient client;
    client._feed(CHROME_GET);
    g_next_client = &client;

    for (int i = 0; i < 5; i++)
      comp.loop();

    const std::string &out = client._out();
    check("GET / : une reponse est envoyee", !out.empty());
    check("GET / : entete HTTP 200", out.rfind("HTTP/1.1 200 OK", 0) == 0);
    check("GET / : Content-Type text/html", out.find("Content-Type: text/html") != std::string::npos);
    check("GET / : le HTML est bien la", out.find("<!DOCTYPE html>") != std::string::npos);
    printf("       (%zu octets envoyes)\n", out.size());
  }

  // ---- 2. GET /buffer doit ouvrir le flux ----
  {
    OledStream comp;
    comp.set_display(&disp);
    comp.set_port(8080);
    comp.set_bits_per_pixel(1);
    comp.setup();

    WiFiClient client;
    client._feed(CHROME_BUFFER_GET);
    g_next_client = &client;

    for (int i = 0; i < 5; i++)
      comp.loop();

    const std::string &out = client._out();
    check("GET /buffer : une reponse est envoyee", !out.empty());
    check("GET /buffer : flux octet-stream", out.find("application/octet-stream") != std::string::npos);
    check("GET /buffer : en-tete de geometrie", out.find("X-Display-Width: 128") != std::string::npos);
    check("GET /buffer : hauteur lue depuis l'ecran", out.find("X-Display-Height: 64") != std::string::npos);
    check("GET /buffer : le client n'est PAS ferme", !client._stopped());
  }

  // ---- 3. requete arrivant en morceaux (TCP fragmente) ----
  {
    OledStream comp;
    comp.set_display(&disp);
    comp.set_port(8080);
    comp.set_bits_per_pixel(1);
    comp.setup();

    // Aucune donnee au moment de l'acceptation : loop() ne doit pas abandonner.
    WiFiClient client;
    client._feed("");
    g_next_client = &client;
    comp.loop();
    check("fragmentee : pas d'abandon quand rien n'est encore arrive", !client._stopped());

    client._feed(CHROME_GET);
    for (int i = 0; i < 5; i++)
      comp.loop();
    check("fragmentee : la reponse part une fois les octets arrives", !client._out().empty());
  }

  printf("\n%s\n", fails == 0 ? "  TOUS LES TESTS PASSENT" : "  DES TESTS ECHOUENT");
  return fails;
}

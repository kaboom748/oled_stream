import esphome.codegen as cg
from esphome.components import display
import esphome.config_validation as cv
from esphome.const import CONF_DISPLAY_ID, CONF_ID, CONF_PLATFORM, CONF_PORT
from esphome.core import CORE

CODEOWNERS = ["@kaboom748"]
DEPENDENCIES = ["display", "network"]
MULTI_CONF = True

oled_stream_ns = cg.esphome_ns.namespace("oled_stream")
OledStream = oled_stream_ns.class_("OledStream", cg.Component)

CONF_CONTRAST_LAMBDA = "contrast_lambda"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(OledStream),
        # width/height are NOT configurable: the display already knows its own
        # geometry. Asking the user let a wrong value through and made the
        # component memcpy past the end of the framebuffer.
        cv.Required(CONF_DISPLAY_ID): cv.use_id(display.DisplayBuffer),
        cv.Optional(CONF_PORT, default=8080): cv.port,
        cv.Optional(CONF_CONTRAST_LAMBDA): cv.lambda_,
    }
).extend(cv.COMPONENT_SCHEMA)


_PLATFORM_INFO = {
    "ssd1306_i2c": ("SSD1306", "mono_page", 1),
    "ssd1306_spi": ("SSD1306", "mono_page", 1),
    "ssd1305": ("SSD1305", "mono_page", 1),
    "pcd8544": ("PCD8544 (Nokia 5110)", "mono_page", 1),
    "st7567_base": ("ST7567", "mono_row", 1),
    "waveshare_epaper": ("Waveshare ePaper", "mono_row", 1),
    "inkplate": ("Inkplate", "mono_row", 1),
    "ssd1322_base": ("SSD1322", "gray4_row", 4),
    "ssd1325_base": ("SSD1325", "gray4_row", 4),
    "ssd1327_base": ("SSD1327", "gray4_row", 4),
    "ili9xxx": ("ILI9XXX", "rgb565_row", 16),
    "st7735": ("ST7735", "rgb565_row", 16),
    "st7789v": ("ST7789V", "rgb565_row", 16),
    "ssd1331_base": ("SSD1331", "rgb565_row", 16),
    "ssd1351_base": ("SSD1351", "rgb565_row", 16),
    "st7920": ("ST7920", "mono_row", 1),
    "max7219digit": ("MAX7219", "mono_row", 1),
}

_DEFAULT_INFO = ("Unknown", "mono_page", 1)


def _find_display_conf(display_id):
    target_id = str(display_id.id)
    for display_conf in CORE.config.get("display", []):
        conf_id = display_conf.get(CONF_ID)
        if conf_id is not None and str(conf_id.id) == target_id:
            return display_conf
    return None


def _detect_display_info(display_conf):
    if display_conf is None:
        return _DEFAULT_INFO
    platform = display_conf.get(CONF_PLATFORM, "")
    return _PLATFORM_INFO.get(platform, _DEFAULT_INFO)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    display_var = await cg.get_variable(config[CONF_DISPLAY_ID])
    cg.add(var.set_display(display_var))
    cg.add(var.set_port(config[CONF_PORT]))

    display_conf = _find_display_conf(config[CONF_DISPLAY_ID])
    label, layout, bpp = _detect_display_info(display_conf)

    cg.add(var.set_display_type(label))
    cg.add(var.set_buffer_layout(layout))
    cg.add(var.set_bits_per_pixel(bpp))

    if CONF_CONTRAST_LAMBDA in config:
        template_ = await cg.process_lambda(
            config[CONF_CONTRAST_LAMBDA], [], return_type=cg.float_
        )
        cg.add(var.set_contrast_lambda(template_))

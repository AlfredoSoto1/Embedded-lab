import time
import lgpio

# ---- CHANGE THESE GPIOs TO MATCH YOUR WIRING ----
RS = 26
E  = 19
D4 = 13
D5 = 6
D6 = 5
D7 = 16

LCD_WIDTH = 16
LCD_CHR = 1
LCD_CMD = 0

LINE_1 = 0x80
LINE_2 = 0xC0

ENABLE_PULSE = 0.0005
ENABLE_DELAY = 0.0005

h = lgpio.gpiochip_open(0)

for pin in [RS, E, D4, D5, D6, D7]:
    lgpio.gpio_claim_output(h, pin, 0)

def pulse_enable():
    lgpio.gpio_write(h, E, 0)
    time.sleep(ENABLE_DELAY)
    lgpio.gpio_write(h, E, 1)
    time.sleep(ENABLE_PULSE)
    lgpio.gpio_write(h, E, 0)
    time.sleep(ENABLE_DELAY)

def write4bits(bits):
    lgpio.gpio_write(h, D4, 1 if (bits & 0x10) else 0)
    lgpio.gpio_write(h, D5, 1 if (bits & 0x20) else 0)
    lgpio.gpio_write(h, D6, 1 if (bits & 0x40) else 0)
    lgpio.gpio_write(h, D7, 1 if (bits & 0x80) else 0)
    pulse_enable()

def lcd_byte(value, mode):
    lgpio.gpio_write(h, RS, mode)
    # high nibble
    write4bits(value & 0xF0)
    # low nibble
    write4bits((value << 4) & 0xF0)

def lcd_init():
    time.sleep(0.05)
    lgpio.gpio_write(h, RS, 0)
    # Init sequence (4-bit)
    write4bits(0x30); time.sleep(0.005)
    write4bits(0x30); time.sleep(0.0002)
    write4bits(0x30); time.sleep(0.0002)
    write4bits(0x20); time.sleep(0.0002)

    lcd_byte(0x28, LCD_CMD)  # 4-bit, 2 line, 5x8 dots
    lcd_byte(0x0C, LCD_CMD)  # display on, cursor off
    lcd_byte(0x06, LCD_CMD)  # entry mode
    lcd_byte(0x01, LCD_CMD)  # clear
    time.sleep(0.002)

def lcd_string(message, line):
    message = message.ljust(LCD_WIDTH)[:LCD_WIDTH]
    lcd_byte(line, LCD_CMD)
    for ch in message:
        lcd_byte(ord(ch), LCD_CHR)

try:
    lcd_init()
    lcd_string("Hello Alfredo!", LINE_1)
    lcd_string("Raspberry Pi 5", LINE_2)
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    pass
finally:
    lgpio.gpiochip_close(h)

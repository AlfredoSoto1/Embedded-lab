from gpiozero import LED
from time import sleep

led = LED(4)  # BCM GPIO4 (physical pin 7)

while True:
    led.on()
    sleep(0.1)   # 100 ms
    led.off()
    sleep(0.1)   # 100 ms

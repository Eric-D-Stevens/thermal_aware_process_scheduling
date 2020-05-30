import os
import time

while(1):
    os.system("vcgencmd measure_temp")
    os.system("vcgencmd measure_clock arm")

    time.sleep(1)
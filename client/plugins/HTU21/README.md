Есть два способа читать значения с датчика HTU21:
1. С помощью этого плагина
2. С помощью FileIOPligin
  1. Подключить молуь ядра `modprobe sht21` и добавить его автозагрузку `echo "sht21" > /etc/modules-load.d/sht21.conf`
  2. При каждом старте системы инициализировать датчики на шине I2C `echo sht21 0x40 > /sys/bus/i2c/devices/i2c-1/new_device` добавить в конфиг плагина initializer
  3. Читать можно из `/sys/class/hwmon/hwmon1/humidity1_input` и `cat /sys/class/hwmon/hwmon1/temp1_input` через плагин FileIO

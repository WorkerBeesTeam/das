TEMPLATE = subdirs
SUBDIRS += Modbus Random OneWireTherm FileIO Uart Router Watchdog Gatt

!NO_V4L {
    SUBDIRS += Camera
}

!NO_WIRINGPI {
  RESULT = $$find(QMAKE_HOST.arch, "^arm")
  linux-rasp-pi2-g++|linux-rasp-pi3-g++|linux-opi-g++|count(RESULT, 1) {
    CONFIG += WIRINGPI
  }
}

WIRINGPI_EMPTY {
    DEFINES+="NO_WIRINGPI=1"
}

WIRINGPI|WIRINGPI_EMPTY {
    SUBDIRS += WiringPi HTU21 DS18B20 Resistance
}

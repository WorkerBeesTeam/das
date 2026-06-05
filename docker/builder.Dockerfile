FROM debian:bullseye

RUN cat > /etc/apt/sources.list << 'EOF'
deb http://mirror.yandex.ru/debian bullseye main
deb http://mirror.yandex.ru/debian-security bullseye-security main
deb http://mirror.yandex.ru/debian bullseye-updates main
EOF

RUN apt update && apt upgrade -yy
RUN apt install -yy \
	git cmake build-essential qtbase5-dev libqt5websockets5-dev libboost-all-dev libbotan-2-dev libssl-dev libcurl4-openssl-dev libfmt-dev cimg-dev

RUN cd /opt \
	&& git clone --depth 1 https://github.com/MeltwaterArchive/served.git \
	&& mkdir -p build/served \
	&& cd build/served && cmake -DCMAKE_BUILD_TYPE=Release /opt/served \
	&& make -j12 && make install \
	&& rm -fr /opt/build/served

RUN cd /opt \
	&& git clone --depth 1 https://github.com/reo7sp/tgbot-cpp.git \
	&& mkdir build/tg \
	&& cd build/tg && cmake -DCMAKE_BUILD_TYPE=Release /opt/tgbot-cpp \
	&& make -j12 && make install \
	&& rm -fr /opt/build/tg

RUN echo "Build maxbot-cpp...." \
	&& cd /opt \
	&& git clone --depth 1 https://github.com/lirik90/maxbot-cpp.git \
	&& mkdir build/max \
	&& cd build/max && cmake -DCMAKE_BUILD_TYPE=Release /opt/maxbot-cpp \
	&& make -j12 && make install \
	&& rm -fr /opt/build/max

CMD mkdir -p /src/build/dist \
	&& cd /src/build \
	&& qmake /src/full.pro CONFIG+=release CONFIG+=DasServer CONFIG+=ServerOnly CONFIG+=GuiOnly CONFIG+=WIRINGPI_EMPTY CONFIG+=BOT_WEBHOOK \
	&& make -j12 \
	&& make install INSTALL_ROOT=/src/build/dist \
	&& rm -fr /src/build/dist/usr/local/include \
	&& cp -r /usr/local/lib /src/build/dist/usr/local/

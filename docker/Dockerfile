FROM debian:bullseye AS build

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

RUN echo "Build maxbot-cpp" \
	&& cd /opt \
	&& git clone --depth 1  -b feature/maxbot3 https://github.com/lirik90/maxbot-cpp.git \
	&& mkdir build/max \
	&& cd build/max && cmake -DCMAKE_BUILD_TYPE=Release /opt/maxbot-cpp \
	&& make -j12 && make install \
	&& rm -fr /opt/build/max

COPY . /src/

WORKDIR /build
RUN cd /build \
	&& qmake /src/full.pro CONFIG+=release CONFIG+=DasServer CONFIG+=ServerOnly CONFIG+=GuiOnly CONFIG+=WIRINGPI_EMPTY CONFIG+=BOT_WEBHOOK \
	&& make -j12 \
	&& make install INSTALL_ROOT=/dist \
	&& rm -fr /dist/usr/local/include

FROM debian:bullseye

RUN cat > /etc/apt/sources.list << 'EOF'
deb http://mirror.yandex.ru/debian bullseye main
deb http://mirror.yandex.ru/debian-security bullseye-security main
deb http://mirror.yandex.ru/debian bullseye-updates main
EOF

COPY --from=build /usr/local/lib /usr/local/lib
COPY --from=build /dist /dist

RUN apt update && apt upgrade -yy
RUN apt install -yy \
	git libqt5sql5-mysql libqt5websockets5 libbotan-2-17 libssl1.1 libcurl4 libfmt7 libboost-thread1.74.0 libpng16-16

RUN mv /dist/usr/local/lib/* /usr/local/lib/ \
	&& mv /dist/opt/das /opt/ \
	&& ldconfig

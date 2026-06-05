FROM debian:bullseye

RUN cat > /etc/apt/sources.list << 'EOF'
deb http://mirror.yandex.ru/debian bullseye main
deb http://mirror.yandex.ru/debian-security bullseye-security main
deb http://mirror.yandex.ru/debian bullseye-updates main
EOF

RUN apt update && apt upgrade -yy
RUN apt install -yy \
	git libqt5sql5-mysql libqt5websockets5 libbotan-2-17 libssl1.1 libcurl4 libfmt7 libboost-thread1.74.0 libpng16-16

COPY build/dist /dist

RUN mv /dist/usr/local/lib/* /usr/local/lib/ \
	&& mv /dist/opt/das /opt/ \
	&& ldconfig

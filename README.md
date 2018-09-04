# README #

### Для получения репозитория и всех подмодулей можно использовать: ###
> git clone --recursive -j8 git@github.com:lirik90/dai.git  

или  
> git clone git@github.com:lirik90/dai.git  
> cd dai  
> git submodule update --init --recursive

---
### Для Django нужно установить MySQL коннектор: ###
> pip3 install mysqlclient sqlparse

---

### Django dump data: ###
> python3 manage.py dumpdata auth.User auth.Group dai.Dai --indent=2 > dai/fixtures/initial_data.json  
> python3 manage.py dumpdata gh_item.ItemType gh_item.LayoutType gh_item.SignType --indent=2 > gh_item/fixtures/initial_data.json  

### Import data: ###
> python3 manage.py loaddata dai/fixtures/initial_data.json

---
### Пользователь MySQL на сервер должен быть создан примерно так: ###
> CREATE USER 'DaiUser'@'localhost' IDENTIFIED BY '???????';  
> GRANT ALL PRIVILEGES ON dai\_django.* TO 'DaiUser'@'localhost';  
> GRANT ALL PRIVILEGES ON dai\_django\_%.* TO 'DaiUser'@'localhost';  
> GRANT CREATE ON *.* TO 'DaiUser'@'localhost';  
> FLUSH PRIVILEGES;
---

### Для генерации ключа на сервере можно использовать: ###
> ./botan keygen > dtls.key  
> ./botan gen_self_signed dtls.key deviceaccess.ru > dtls.pem

---

### Для создания образа с Raspberry: ###
> find /var/log -type f -delete  
> apt-get autoremove && apt-get autoclean && apt-get clean  
>  
> cat /dev/zero > /root/zeros; sync; rm /root/zeros  
> #zerofree -v /dev/sdd2
>  
> #dd if=/dev/da0 conv=sync,noerror bs=128K | gzip -c | ssh vivek@server1.cyberciti.biz dd of=centos-core-7.gz  
>  
> mount -t cifs -o user=kirill //192.168.202.70/share tmp/  
> dd if=/dev/mmcblk0 conv=sync,noerror bs=128K | gzip -c | dd of=tmp/raspberry.img.gz  
>  
> # write image:  
> gzip -d -c tmp/raspberry.img.gz | dd of=/dev/sdd  

---  

### Для компиляции GDB: ###
> wget https://ftp.gnu.org/gnu/gdb/gdb-7.12.tar.gz
> tar -pxzf gdb-7.12.tar.gz
> cd gdb-7.12
> export PATH=/mnt/second_drive/projects/dai/raspberry/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/bin:$PATH
> ./configure --with-python=yes --target=arm-linux-gnueabihf
> make
> sudo make install

---

### Проброс последовательного порта: ###
На сервере:
> socat tcp-l:54321,reuseaddr,fork file:/dev/ttyUSB0,nonblock,waitlock=/var/run/ttyUSB0.lock

На клиенте:
> socat pty,link=$HOME/vmodem0,waitslave tcp:gh1:54321

---

> export LD_LIBRARY_PATH=/usr/local/lib
> export TSLIB_CONSOLEDEVICE=none
> export TSLIB_FBDEVICE=/dev/fb0
> export TSLIB_TSDEVICE=/dev/input/event0
> export TSLIB_CALIBFILE=/usr/local/etc/pointercal
> export TSLIB_CONFFILE=/usr/local/etc/ts.conf
> export TSLIB_PLUGINDIR=/usr/local/lib/ts

### Калибровка экрана ###
> ts_calibrate

Включить экран если погас:
> echo -ne "\033[9;0]" >/dev/tty1


### Отладка ###
На сервере:
> gdbserver :1234 /opt/Dai/DaiGlobal -e

На клиенте:
> ./arm-linux-gnueabihf-gdb --args /mnt/second_drive/build/dai/Raspberry_Root/DaiModern -e
> set sysroot /mnt/second_drive/projects/dai/raspberry/work/sysroot/
> set solib-search-path /mnt/second_drive/projects/dai/raspberry/work/qt5pi/
> target remote gh3:1234
> continue


---


This README would normally document whatever steps are necessary to get your application up and running.

### What is this repository for? ###

* Quick summary
* Version
* [Learn Markdown](https://bitbucket.org/tutorials/markdowndemo)

### How do I get set up? ###

* Summary of set up
* Configuration
* Dependencies
* Database configuration
* How to run tests
* Deployment instructions

### Contribution guidelines ###

* Writing tests
* Code review
* Other guidelines

### Who do I talk to? ###

* Repo owner or admin
* Other community or team contact

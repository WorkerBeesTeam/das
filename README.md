# Описание
Система предназначена для удалённого управления и сбора информации с устройств, а также создания автоматизации. ПО **DasClient** которое обеспечивает автоматизацию не зависит от сервера, но может быть настроенно на подключение к серверу для удалённого управления объектом и централизованного анализа информации.

## Условные обозначения
- **_Схема_ (Scheme)** - описание всей структуры для взаимодействия с элементами на объекте через API. Схема содержит в себе описание секций и устройств, а также используемых типов, и хранит в себе скрипты автоматизации. Это глобальная единица в системе, т.е. одна схема полностью описывает объект автоматизации (Здание, Цех, Электронный продукт и т.д.)
  - **_Устройство_ (Device)** - описывает физическое устройство которое передаёт/получает данные. Например: Группа реле управляемая черз GPIO. "Устройства (Device)" не обязательно описывают физическое устройство, это может быть виртуальная сущность.
    - **_Элемент устройства_ (Device\_Item)** - описывает еденицу входящих/выходящих данных, например датчик температуры или реле.
    - **_Плагин_ (Plugin\_Type)** - устройству может быть назначен плагин, который будет читать значения из физического устройства и передавать их в _Элемент устройства (Device\_Item)_ и в обратном направлении.
  - **_Секция_ (Section)** - описывает физические отделения на объекте автоматизации (Например: Тамбур, Секция с помидорами, Секция с огурцами)
  - **_Группа элементов_ (Device\_Item\_Group, DIG)** - описывает логическое разделение элементов устройств на гуппы (Например: Освещение, Отопление, Полив)
    - **_Уставки_ (DIG\_Param)** - у каждой группы могут быть задаваемые пользователем параметры
    - **_Состояния_ (DIG\_Status)** - описывает состояния группы элементов о которых нужно информировать пользователя
    - **_Режим_ (DIG\_Mode)** - Для каждой группы можно выбрать режим (Например: Ручной, Автоматический)
- ***Журналы***
  - **_Журнал значений_ (Log\_Value)** - хранит изменения значений элементов устройста.
  - **_Журнал событий_ (Log\_Event)** - хранит текстовые сообщения генерируемые в системе во время работы.
  - **_Журнал уставок_ (Log\_Param)** - хранит изменения значений уставок.
  - **_Журнал состояний_ (Log\_Status)** - хранит изменения состояний групп.
  - **_Журнал режимов_ (Log\_Mode)** - хранит изменения режимов групп.

![Возможный вид страницы "Элементы"](https://github.com/WorkerBeesTeam/das/raw/master/doc/demo.png)

## Части системы:
- **DasClient** - Основное ПО обесспечивающее автоматизацию
  - Язык: С++. Библиотеки: STL, Boost, Qt (Network, DBus, SQL, Script), Botan
  - Используемые внутренние библиотеки: Das, DasPlus, DasDBus, Helpz (Service, DB, DTLS)
  - Описание:
    - База данных (Структура проекта, журнал событий, запись изменения значений)
    - DTLS Клиент (для подключения к серверу)
    - JS движок для отработки автоматизации
    - Плагины для доступа к физическим устройствам (Modbus, OneWireTherm, WiringPi, DS18B20, HTU21, Uart, FileIO, Camera, Random)
- **DasServer** - Серверная часть для централизованного управления
  - Язык: С++. Библиотеки: STL, Boost, Qt (Network, DBus, SQL), Botan
  - Используемые внутренние библиотеки: Das, DasPlus, DasDBus, Helpz (Service, DB, DTLS)
  - Описание:
    - DTLS Сервер (Шифрованный UDP для общения с программой Клиент)
    - База данных
- **DasWebApi**
  - Язык: С++. Библиотеки: STL, Boost, Qt (Network, DBus, SQL, Script), Served
  - Используемые внутренние библиотеки: Das, DasPlus, DasDBus, Helpz (Service, DB, Network)
  - Описание:
    - WebSocket сервер (Для мгновенного отображения изменений и управления)
    - RestfulApi сервер 
    - Stream сервер
- **DasTelegramBot** - Телеграм бот для уведомлений о событиях, и простого управления
  - Язык: С++. Библиотеки: STL, Boost, Qt (Network, DBus, SQL), TgBot
  - Используемые внутренние библиотеки: Das, DasPlus, DasDBus, Helpz (Service, DB)
- **DasModern** - GUI Клиент \ Мобильное приложение
  - Язык: С++, Qt/QML. Библиотеки: STL, Qt (QML, QuickControls2, WebSockets)
- **DasEmulator** - Эмулятор Modbus (Имитации работы физического устройства для тестирования и отладки)
  - Язык: С++. Библиотеки: STL, Qt (Network, DBus, SQL, Gui, SerialBus)
  - Используемые внутренние библиотеки: Das, DasPlus, Helpz (DB)
- **libDas** - Основная библиотека содержащая инструменты для работы с основными частями системы
- **libDasPlus** - Библиотека содержит инструменты для работы со структурой базы данных и другими частями системы
- **libDasDBus** - Содержит описание основного DBus интерфейса для межпроцессорного взаимодействия между частями системы
- **libHelpz** - Вспомогательные базовые библиотеки для функционирования системы
  - **Base** - Логирование, Распаковка настроек, Шаблоны для потоков и десериализации в функцию
  - **DBMeta** - Инстументы для описания базовых классов используемых в libHelpzDB
  - **DB** - Работа с базой данных (Зависит от: DBMeta)
  - **Network** - Работа с протоколом высокого уровня (Зависит от: Base)
  - **DTLS** - Сетевое взаимодействие с использованием протокола высокого уровня поверх DTLS (Зависит от Network)
  - **Service** - Использование служб
- **DasBack** - Backend (Серверная часть сайта)
  - Язык: Python. Библиотеки: Django, PyJWT, DRF, OpenPyXL
- **DasFront** - Frontend (Браузерная часть сайта)
  - Язык: TypeScript. Библиотеки: Angular, RxJS

## Принцип работы
На компьютер объекта автоматизации устанавливается программа DasClient. Если планируется использовать локальное подключение и управлять объектом напрямую без сервера, то так же устанавливаются программы DasWebApi, DasBack и DasFront. Если объект управляется через сервер то на сервере создаётся новая схема или её псевдоним (alias) с уникальным именем для подключения объекта, а также специальная учётная запись с которой объект будет подключаться к серверу. Перед первым запуском для DasClient создаётся база данных, структуру таблиц можно развернуть с помощью программы DasBack. После чего в конфигурационный файл ***/opt/das/DasClient.conf*** заносяться настройки для подключения к базе данных, а также данный для авторизации на сервере (логин, пароль и имя схемы).  
После запуска службы **dasclient** программа загружает схему из базы данных. Схема передаётся в скриптовый движок для предоставления API на основе схемы. Затем движок выполняет скрипты для автоматизации объекта. Параллельно с этим запускается опрос устройств схемы и подключение к серверу. Устройства опрашиваются с установленным пользователем интервалом или в некоторых случаях по алгоритму заложенному в сам плагин опроса. После очередного опроса новые значения передаются в схему. Изменённые значения передаются на сервер и сохраняются там в журнал если настроен соответствующий _алгоритм сохранения_, если передать на сервер не удалось, то значения по такому же алгоритму сохраняются в локальный журнал. Так же значения передаются в скриптовой движок для взаимодействия с ними скриптами автоматизации. Скрипты получая новые значения анализируют их и на основе _уставок_ установленных пользователем могут отправить команду на изменение значения какого либо физического устройства, а так же изменить _состояние_ и тем самым уведомить пользователя. Все изменения в схеме, в том числе сделанные скриптами, отправляются на сервер и там храняться в соответствующих журналах.  
Сервер при подключении к нему запрашивает хэши схемы, отдельно для всех таблиц содержащих общие данные, а также для каждой таблицы с уникальными для объекта данными. Если хэш различается, то в случае с общими данными запрашиваются хэши для каждой таблицы общих данных, а затем если и те различаются, то значения конкретной таблицы. Если различающиеся данные относятся к общим, то сервер передаёт на DasClient актуальные данные, а если уникальные для объекта, то приводит данные в базе к виду как на клиенте. Если синхронизация не удалась, то сервер выставляет состояние подключения "Ошибка синхронизации структур" и повторяет её. Также сервер запрашивает данные журналов для передачи их на сервер если что то накопилось в отсутствии подключения. Также сервер запрашивает текущее время на клиенте, а также версии ПО.  
Соединение с сервером не разрывается. По нему передаются новые данные от клиента, а также управляющие команды от сервера на клиент. В отсутствии активности с заданным интервалом передаётся PING для проверки соединения.  
Используя DBus сервер передаёт все данные приходящие от клиента в приложения **DasWebApi** и **DasTelegramBot**.  
Пользователь зайдя на сайт использует веб приложение **DasFront**. При загрузки оно используя Restful api получает у приложения **DasBack** и **DasWebApi** структуру выбранного пользователем проекта, после чего используя WebSocket устанавливает постоянное соедиенение с приложением **DasWebApi** для мгновенного получения изменений схемы, а также передачи управляющих команд.  
Таким образом новые данные, например с датчика температуры, читаются плагином **DS18B20** загруженнымм в ПО **DasClient** шифруются и передаются по **DTLS** (TLS over UDP) на сервер, затем по DBus передаются в ПО **DasWebApi** которое по WebSocket отправляет их в приложение **DasFront** и отображаются пользователю.  

![Схема работы ПО](https://github.com/WorkerBeesTeam/das/raw/master/doc/work_scheme.png)

# --------------------------------------------------------------------
# Дальше идёт ещё не подготовленная информация

### Для получения репозитория и всех подмодулей можно использовать: ###
> git clone --recursive -j8 git@github.com:WorkerBeesTeam/das.git  

или  
> git clone git@github.com:WorkerBeesTeam/das.git  
> cd das  
> git submodule update --init --recursive

---
### Для Django нужно установить MySQL коннектор: ###
> pip3 install mysqlclient sqlparse

---

### Django dump data: ###
> python3 manage.py dumpdata auth.User auth.Group das.Das --indent=2 > das/fixtures/initial_data.json  
> python3 manage.py dumpdata gh_item.Device_Item_Type gh_item.LayoutType gh_item.SignType --indent=2 > gh_item/fixtures/initial_data.json  

### Import data: ###
> python3 manage.py loaddata das/fixtures/initial_data.json

---
### Пользователь MySQL на сервер должен быть создан примерно так: ###
> CREATE USER 'DasUser'@'localhost' IDENTIFIED BY '???????';  
> GRANT ALL PRIVILEGES ON das\_django.* TO 'DasUser'@'localhost';  
> GRANT ALL PRIVILEGES ON das\_django\_%.* TO 'DasUser'@'localhost';  
> GRANT CREATE ON *.* TO 'DasUser'@'localhost';  
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
> export PATH=/mnt/second_drive/projects/das/raspberry/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/bin:$PATH
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
> gdbserver :1234 /opt/Das/DasGlobal -e

На клиенте:
> ./arm-linux-gnueabihf-gdb --args /mnt/second_drive/build/das/Raspberry_Root/DasModern -e
> set sysroot /mnt/second_drive/projects/das/raspberry/work/sysroot/
> set solib-search-path /mnt/second_drive/projects/das/raspberry/work/qt5pi/
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

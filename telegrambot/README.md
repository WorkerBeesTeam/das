# DasTelegramBot

1. TgBot-cpp: https://github.com/reo7sp/tgbot-cpp.git
2. QtXlsxWriter: https://github.com/WorkerBeesTeam/QtXlsxWriter
3. SMTPClient: https://github.com/bluetiger9/SmtpClient-for-Qt.git
4. ExprTk: https://github.com/ArashPartow/exprtk

## Описание шаблонов меню
- **name**: Текст пункта меню
- **order**: Номер сортировки
- **template**: Текст шаблона который будет передан пользователю.
- **data**: Описание перменных в шаблоне вида "Имя переменной": {}
  - **type**: Тип данных в переменной
    - **connection\_state**: Состояние подключения
    - **dig\_name**: Имя группы элементов устройств
    - **dig\_mode**: Режим группы
    - **dig\_param**: Уставка
      - **with\_name**: Добавить имя уставки
    - **di**: Элемент устройства
      - **with\_name**: Добавить имя элемента
      - **norm**: Выражение в котором вы используя переменную 'x' в качестве значения элемента устройства вычисляете заменяемый текст и складываете его в переменную 'res'. Для вычисления выражения используется [ExprTk: C++ Mathematical Expression Library](https://github.com/ArashPartow/exprtk) 
        - Например: "res := x ? 'Вкл' : 'Выкл'"
        - Например для окончания 1 яблоко, 2 яблока, 5 яблок: "var n := x % 10; var is_not_teen := floor((x % 100) / 10) != 1; res := to_str(x) + ' яблок' + (n == 1 and is_not_teen ? 'о' : n >= 2 and n <= 4 and is_not_teen ? 'а' : '')"
  - **id**: ID в зависимости от типа данных
  - **max\_length**: Ограничение длины


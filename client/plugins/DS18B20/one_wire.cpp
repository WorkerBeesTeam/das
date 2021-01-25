#include <stdexcept>
#include <iostream>

#include <wiringPi.h>

#include "one_wire.h"

namespace Das {

One_Wire::One_Wire(int _pin) :
    pin_(_pin)
{
}

One_Wire::~One_Wire()
{
}

void One_Wire::init()
{
    if (wiringPiSetup() == -1)
        throw std::logic_error("WiringPi Setup error");

    pinMode(pin_, INPUT);
}

/*
 * сброс
 */
int One_Wire::reset()
{
    int response;

    pinMode(pin_, OUTPUT);
    digitalWrite(pin_, LOW);
    delayMicroseconds(480);

    // Когда ONE WIRE устройство обнаруживает положительный перепад, он ждет от 15us до 60us
    pinMode(pin_, INPUT);
    delayMicroseconds(60);

    // и затем передает импульс присутствия, перемещая шину в логический «0» на длительность от 60us до 240us.
    response = digitalRead(pin_);
    delayMicroseconds(410);

    // если 0, значит есть ответ от датчика, если 1 - нет
    return response;
}

/*
 * отправить один бит
 */
void One_Wire::write_bit(uint8_t bit)
{
    if (bit & 1)
    {
        // логический «0» на 10us
        pinMode(pin_, OUTPUT);
        digitalWrite(pin_, LOW);
        delayMicroseconds(10);
        pinMode(pin_, INPUT);
        delayMicroseconds(55);
    }
    else
    {
        // логический «0» на 65us
        pinMode(pin_, OUTPUT);
        digitalWrite(pin_, LOW);
        delayMicroseconds(65);
        pinMode(pin_, INPUT);
        delayMicroseconds(5);
    }
}

/*
 * отправить один байт
 */
void One_Wire::write_byte(uint8_t byte)
{
    uint8_t i = 8;
    while (i--)
    {
        write_bit(byte & 1);
        byte >>= 1;
    }
}

/*
 * получить один бит
 */
uint8_t One_Wire::read_bit()
{
    uint8_t bit = 0;
    // логический «0» на 3us
    pinMode(pin_, OUTPUT);
    digitalWrite(pin_, LOW);
    delayMicroseconds(3);

    // освободить линию и ждать 10us
    pinMode(pin_, INPUT);
    delayMicroseconds(10);

    // прочитать значение
    bit = digitalRead(pin_);

    // ждать 45us и вернуть значение
    delayMicroseconds(45);
    return bit;
}
/*
 * получить один байт
 */
uint8_t One_Wire::read_byte()
{
    uint8_t i = 8, byte = 0;
    while (i--)
    {
        byte >>= 1;
        byte |= (read_bit() << 7);
    }
    return byte;
}

/*
 * читать ROM подчиненного устройства (код 64 бита)
 */
uint64_t One_Wire::read_rom()
{
    uint64_t one_wire_device;
    if (reset() == 0)
    {
        write_byte (CMD_READROM);
        //  код семейства
        one_wire_device = read_byte();
        // серийный номер
        one_wire_device |= (uint16_t) read_byte()
                <<  8 | (uint32_t) read_byte()
                << 16 | (uint32_t) read_byte()
                << 24 | (uint64_t) read_byte()
                << 32 | (uint64_t) read_byte()
                << 40 | (uint64_t) read_byte()
                << 48;
        // CRC
        one_wire_device |= (uint64_t) read_byte() << 56;
    }
    else
        return 1;
    return one_wire_device;
}

/*
 * пропустить ROM
 */
void One_Wire::skip_rom()
{
    reset();
    write_byte (CMD_SKIPROM);
}

/*
 * Команда соответствия ROM, сопровождаемая последовательностью
 * кода ROM на 64 бита позволяет устройству управления шиной
 * обращаться к определенному подчиненному устройству на шине.
 */
void One_Wire::set_device(uint64_t rom)
{
    uint8_t i = 64;
    reset();
    write_byte (CMD_MATCHROM);
    while (i--)
    {
        write_bit(rom & 1);
        rom >>= 1;
    }
}

/*
 * провеска CRC, возвращает "0", если нет ошибок
 * и не "0", если есть ошибки
 */
int One_Wire::crc_check(uint64_t data8x8bit, uint8_t len)
{
    uint8_t dat, crc = 0, fb, stByte = 0;
    do
    {
        dat = (uint8_t)(data8x8bit >> (stByte * 8));
        // счетчик битов в байте
        for (int i = 0; i < 8; i++)
        {
            fb = crc ^ dat;
            fb &= 1;
            crc >>= 1;
            dat >>= 1;
            if (fb == 1)
                crc ^= 0x8c; // полином
        }
        stByte++;
    }
    while (stByte < len);   // счетчик байтов в массиве
    return crc;
}

uint8_t One_Wire::crc8(uint8_t addr[], uint8_t len)
{
    uint8_t crc = 0;
    while (len--)
    {
        uint8_t inbyte = *addr++;
        for (uint8_t i = 8; i; i--)
        {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix)
                crc ^= 0x8c;

            inbyte >>= 1;
        }
    }
    return crc;
}

/*
 * поиск устройств
 */
void One_Wire::search_rom(uint64_t * roms, int & n, const bool &break_flag)
{
    uint64_t lastAddress = 0;
    int lastDiscrepancy = 0;
    int err = 0;
    int i = 0;
    do
    {
        do
        {
            try
            {
                lastAddress = search_next_address(lastAddress, lastDiscrepancy, break_flag);
                int crc = crc_check(lastAddress, 8);
                if (crc == 0)
                {
                    roms[i++] = lastAddress;
                    err = 0;
                }
                else
                    err++;
            }
            catch (std::exception & e)
            {
                err++;
                if (err > 3)
                    throw e;
            }
        }
        while (!break_flag && err != 0);
    }
    while (!break_flag && lastDiscrepancy != 0 && i < n);
    n = i;
}

/*
 * поиск следующего подключенного устройства
 */
uint64_t One_Wire::search_next_address(uint64_t last_address, int & last_discrepancy, const bool& break_flag)
{
    uint64_t newAddress = 0;
    int searchDirection = 0;
    int idBitNumber = 1;
    int lastZero = 0;
    reset();
    write_byte (CMD_SEARCHROM);

    while (idBitNumber < 65)
    {
        int idBit = read_bit();
        int cmpIdBit = read_bit();

        // id_bit = cmp_id_bit = 1
        if (idBit == 1 && cmpIdBit == 1)
        {
            throw std::logic_error("error: id_bit = cmp_id_bit = 1");
        }
        else if (idBit == 0 && cmpIdBit == 0)
        {
            // id_bit = cmp_id_bit = 0
            if (idBitNumber == last_discrepancy)
            {
                searchDirection = 1;
            }
            else if (idBitNumber > last_discrepancy)
            {
                searchDirection = 0;
            }
            else
            {
                if ((uint8_t)(last_address >> (idBitNumber - 1)) & 1)
                    searchDirection = 1;
                else
                    searchDirection = 0;
            }

            if (searchDirection == 0)
                lastZero = idBitNumber;
        }
        else
        {
            // id_bit != cmp_id_bit
            searchDirection = idBit;
        }
        newAddress |= ((uint64_t) searchDirection) << (idBitNumber - 1);
        write_bit(searchDirection);
        idBitNumber++;
    }
    last_discrepancy = lastZero;
    return newAddress;
}

} // namespace Das

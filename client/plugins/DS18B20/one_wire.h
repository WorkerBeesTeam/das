#ifndef ONE_WIRE_H
#define ONE_WIRE_H
 
#define CMD_CONVERTTEMP    0x44
#define CMD_RSCRATCHPAD    0xbe
#define CMD_WSCRATCHPAD    0x4e
#define CMD_CPYSCRATCHPAD  0x48
#define CMD_RECEEPROM      0xb8
#define CMD_RPWRSUPPLY     0xb4
#define CMD_SEARCHROM      0xf0
#define CMD_READROM        0x33
#define CMD_MATCHROM       0x55
#define CMD_SKIPROM        0xcc
#define CMD_ALARMSEARCH    0xec
 
#include <stdint.h>

namespace Das {

class One_Wire
{
 
public:
  One_Wire(int _pin);
  virtual ~One_Wire();

  void init();
  int reset();
 
  void write_bit(uint8_t bit);
  void write_byte(uint8_t byte);
  uint8_t read_bit();
  uint8_t read_byte();

  uint64_t read_rom();
  void skip_rom();

  void set_device(uint64_t rom);

  int crc_check(uint64_t data8x8bit, uint8_t len);
  uint8_t crc8(uint8_t addr[], uint8_t len);

  void search_rom(uint64_t* roms, int& n, const bool &break_flag);

private:
  int pin_;
  uint64_t search_next_address(uint64_t last_address, int& last_discrepancy, const bool &break_flag);
};

} // namespace Das

#endif // ONEWIRE_H

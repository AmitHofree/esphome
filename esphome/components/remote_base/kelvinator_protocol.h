#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "remote_base.h"

#include <cinttypes>

namespace esphome {
namespace remote_base {

const uint16_t kKelvinatorStateLength = 16;

union KelvinatorData {
  uint8_t raw[kKelvinatorStateLength];  ///< The state in IR code form.
  struct {
    // Byte 0
    uint8_t Mode : 3;
    uint8_t Power : 1;
    uint8_t BasicFan : 2;
    uint8_t SwingAuto : 1;
    uint8_t : 1;  // Sleep Modes 1 & 3 (1 = On, 0 = Off)
    // Byte 1
    uint8_t Temp : 4;  // Degrees C.
    uint8_t : 4;
    // Byte 2
    uint8_t : 4;
    uint8_t Turbo : 1;
    uint8_t Light : 1;
    uint8_t IonFilter : 1;
    uint8_t XFan : 1;
    // Byte 3
    uint8_t : 4;
    uint8_t : 2;  // (possibly timer related) (Typically 0b01)
    uint8_t : 2;  // End of command block (B01)
    // (B010 marker and a gap of 20ms)
    // Byte 4
    uint8_t SwingV : 4;
    uint8_t SwingH : 1;
    uint8_t : 3;
    // Byte 5~6
    uint8_t pad0[2];  // Timer related. Typically 0 except when timer in use.
    // Byte 7
    uint8_t : 4;       // (Used in Timer mode)
    uint8_t Sum1 : 4;  // checksum of the previous bytes (0-6)
    // (gap of 40ms)
    // (header mark and space)
    // Byte 8~10
    uint8_t pad1[3];  // Repeat of byte 0~2
    // Byte 11
    uint8_t : 4;
    uint8_t : 2;  // (possibly timer related) (Typically 0b11)
    uint8_t : 2;  // End of command block (B01)
    // (B010 marker and a gap of 20ms)
    // Byte 12
    uint8_t : 1;  // Sleep mode 2 (1 = On, 0=Off)
    uint8_t : 6;  // (Used in Sleep Mode 3, Typically 0b000000)
    uint8_t Quiet : 1;
    // Byte 13
    uint8_t : 8;  // (Sleep Mode 3 related, Typically 0x00)
    // Byte 14
    uint8_t : 4;  // (Sleep Mode 3 related, Typically 0b0000)
    uint8_t Fan : 3;
    // Byte 15
    uint8_t : 4;
    uint8_t Sum2 : 4;  // checksum of the previous bytes (8-14)
  };
};

class KelvinatorProtocol : public RemoteProtocol<KelvinatorData> {
 public:
  void encode(RemoteTransmitData *dst, const KelvinatorData &data) override;
  optional<KelvinatorData> decode(RemoteReceiveData data) override;
  void dump(const KelvinatorData &data) override;
 protected:
  void encode_byte_(RemoteTransmitData *dst, uint8_t item);
  void encode_data_(RemoteTransmitData *dst, const uint8_t *data, uint8_t nbytes);
  uint8_t decode_byte_(RemoteReceiveData src, uint8_t *data);
  uint8_t decode_data_(RemoteReceiveData src, uint8_t *data, uint8_t nbytes);
};

DECLARE_REMOTE_PROTOCOL(Kelvinator)

template<typename... Ts> class KelvinatorAction : public RemoteTransmitterActionBase<Ts...> {
 public:
  TEMPLATABLE_VALUE(uint8_t[kKelvinatorStateLength], raw)

  void encode(RemoteTransmitData *dst, Ts... x) override {
    KelvinatorData data{};
    data.data = this->raw_.value(x...);
    KelvinatorProtocol().encode(dst, data);
  }
};

}  // namespace remote_base
}  // namespace esphome

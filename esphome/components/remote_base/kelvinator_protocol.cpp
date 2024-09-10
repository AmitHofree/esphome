#include "kelvinator_protocol.h"
#include "esphome/core/log.h"

namespace esphome {
namespace remote_base {

static const char *const TAG = "remote.kelvinator";

static const uint16_t kKelvinatorTick = 85;
static const uint16_t kKelvinatorHdrMarkTicks = 106;
static const uint16_t kKelvinatorHdrMark = kKelvinatorHdrMarkTicks * kKelvinatorTick;
static const uint16_t kKelvinatorHdrSpaceTicks = 53;
static const uint16_t kKelvinatorHdrSpace = kKelvinatorHdrSpaceTicks * kKelvinatorTick;
static const uint16_t kKelvinatorBitMarkTicks = 8;
static const uint16_t kKelvinatorBitMark = kKelvinatorBitMarkTicks * kKelvinatorTick;
static const uint16_t kKelvinatorOneSpaceTicks = 18;
static const uint16_t kKelvinatorOneSpace = kKelvinatorOneSpaceTicks * kKelvinatorTick;
static const uint16_t kKelvinatorZeroSpaceTicks = 6;
static const uint16_t kKelvinatorZeroSpace = kKelvinatorZeroSpaceTicks * kKelvinatorTick;
static const uint16_t kKelvinatorGapSpaceTicks = 235;
static const uint16_t kKelvinatorGapSpace = kKelvinatorGapSpaceTicks * kKelvinatorTick;
const uint8_t kKelvinatorCmdFooter = 2;
const uint8_t kKelvinatorCmdFooterBits = 3;
const uint8_t kKelvinatorChecksumStart = 10;

void KelvinatorProtocol::encode(RemoteTransmitData *dst, const KelvinatorData &data) {
  dst->set_carrier_frequency(38000);
  dst->reserve(9871243); // TODO: Calculate what this should be

  // Command block #1 (4 bytes)
  dst->item(kKelvinatorHdrMark, kKelvinatorHdrSpace);
  this->encode_data_(dst, data.raw, 4);

  // Footer for command block (3 bits (b010)) + footer 
  dst->item(kKelvinatorBitMark, kKelvinatorZeroSpace);
  dst->item(kKelvinatorBitMark, kKelvinatorOneSpace);
  dst->item(kKelvinatorBitMark, kKelvinatorZeroSpace);
  dst->item(kKelvinatorBitMark, kKelvinatorGapSpace);

  // Data block #1 (4 bytes)
  this->encode_data_(dst, data.raw + 4, 4);
  dst->item(kKelvinatorBitMark, 2 * kKelvinatorGapSpace);

  // Command block #2 (4 bytes)
  dst->item(kKelvinatorHdrMark, kKelvinatorHdrSpace);
  this->encode_data_(dst, data.raw + 8, 4);

  // Footer for command block (3 bits (b010)) + footer 
  dst->item(kKelvinatorBitMark, kKelvinatorZeroSpace);
  dst->item(kKelvinatorBitMark, kKelvinatorOneSpace);
  dst->item(kKelvinatorBitMark, kKelvinatorZeroSpace);
  dst->item(kKelvinatorBitMark, kKelvinatorGapSpace);

  // Data block #2 (4 bytes)
  this->encode_data_(dst, data.raw + 12, 4);
  dst->item(kKelvinatorBitMark, 2 * kKelvinatorGapSpace);
}

void KelvinatorProtocol::encode_byte_(RemoteTransmitData *dst, uint8_t item) {
    for (uint8_t bit = 0; bit < 8; bit++, item >>= 1) {
      if (item & 1) {
        dst->item(kKelvinatorBitMark, kKelvinatorOneSpace);
      } else {
        dst->item(kKelvinatorBitMark, kKelvinatorZeroSpace);
      }
    }
}

void KelvinatorProtocol::encode_data_(RemoteTransmitData *dst, const uint8_t *data, uint8_t nbytes) {
  for (uint16_t i = 0; i < nbytes; i++) {
      this->encode_byte_(dst, *(data + i));
  }
}

optional<KelvinatorData> KelvinatorProtocol::decode(RemoteReceiveData src) {
  uint8_t out[kKelvinatorStateLength];

  // Command block #1 (4 bytes)
  if (!src.expect_item(kKelvinatorHdrMark, kKelvinatorHdrSpace)) return {};
  if (this->decode_data_(src, out, 4) != 4) return {};

  // Footer for command block (3 bits (b010)) + footer 
  if (!src.expect_item(kKelvinatorBitMark, kKelvinatorZeroSpace)) return {};
  if (!src.expect_item(kKelvinatorBitMark, kKelvinatorOneSpace)) return {};
  if (!src.expect_item(kKelvinatorBitMark, kKelvinatorZeroSpace)) return {};
  if (!src.expect_item(kKelvinatorBitMark, kKelvinatorGapSpace)) return {};

  // Data block #1 (4 bytes)
  if (this->decode_data_(src, out + 4, 4) != 4) return {};
  if (!src.expect_item(kKelvinatorBitMark, 2 * kKelvinatorGapSpace)) return {};

  // Command block #2 (4 bytes)
  if (!src.expect_item(kKelvinatorHdrMark, kKelvinatorHdrSpace)) return {};
  if (this->decode_data_(src, out + 8, 4) != 4) return {};

  // Footer for command block (3 bits (b010)) + footer 
  if (!src.expect_item(kKelvinatorBitMark, kKelvinatorZeroSpace)) return {};
  if (!src.expect_item(kKelvinatorBitMark, kKelvinatorOneSpace)) return {};
  if (!src.expect_item(kKelvinatorBitMark, kKelvinatorZeroSpace)) return {};
  if (!src.expect_item(kKelvinatorBitMark, kKelvinatorGapSpace)) return {};

  // Data block #2 (4 bytes)
  if (this->decode_data_(src, out + 12, 4) != 4) return {};
  if (!src.expect_item(kKelvinatorBitMark, 2 * kKelvinatorGapSpace)) return {};

  return *((KelvinatorData*)(out));
}

uint8_t KelvinatorProtocol::decode_byte_(RemoteReceiveData src, uint8_t *data) {
  uint8_t item = 0;
  for (uint8_t bit = 0; bit < 8; bit++, item >>= 1) {
      if (src.expect_item(kKelvinatorBitMark, kKelvinatorOneSpace)) {
        item = (item << 1) | 1;
      } else if (src.expect_item(kKelvinatorBitMark, kKelvinatorZeroSpace)) {
        item = (item << 1) | 0;
      } else {
        return 0;
      }
  }
  *data = item;
  return 1;
}

uint8_t KelvinatorProtocol::decode_data_(RemoteReceiveData src, uint8_t *data, uint8_t nbytes) {
  uint8_t bytes_read = 0;
  for (uint16_t i = 0; i < nbytes; i++, bytes_read++) {
      if (!this->decode_byte_(src, data + i)) return 0;
  }
  return bytes_read;
}

void KelvinatorProtocol::dump(const KelvinatorData &data) {
  ESP_LOGI(TAG, "Received Kelvinator: data=0x%08" PRIX32 ", nbits=%d", data.data, data.nbits);
}

}  // namespace remote_base
}  // namespace esphome

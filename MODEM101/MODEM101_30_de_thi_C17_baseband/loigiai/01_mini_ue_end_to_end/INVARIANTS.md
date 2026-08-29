# Wire, DSP, security và ownership invariants

- Mọi wire field được encode/decode từng byte little-endian; không serialize
  padding/bit-field của C. Bit điều chế được lấy MSB-first trong mỗi byte.
- IQ16 có header little-endian 16 byte: `"IQ16"`, sample-rate `u32`, count
  `u64`, rồi các cặp `I,Q` int16 two's-complement. `count <= 8192`; phép nhân
  kích thước chỉ xảy ra sau kiểm bound/overflow.
- BMAC bắt đầu bằng `"BMAC"`, version 1, số SDU 1..8 và hai byte reserved bằng
  zero. Mỗi subheader là `LCID:u8,reserved=0:u8,length:u16`; chỉ LCID control/data
  hợp lệ, SDU khác zero và tổng length phải khớp chính xác. Toàn TB được validate
  trước khi bất kỳ child event hay protocol state nào được commit.
- FFT/QAM/NCO dùng Q15. Kết quả thu hẹp đi qua `bb_sat16`; IFFT scale một bit mỗi
  radix-2 stage, forward FFT không scale. LLR dương biểu diễn bit 0, âm biểu diễn
  bit 1; Chase combining cộng bão hòa từng bit của đúng active HARQ process.
- CRC-24A dùng polynomial `0x1864CFB`, init zero, MSB-first và phủ đúng TB
  payload. CRC32C descriptor/IPC dùng reflected polynomial `0x82F63B78`,
  init/final XOR all-ones; DMA phủ byte 0–27, IPC coi field CRC là zero khi tính.
- NIA2 xác thực `COUNT || BEARER || DIRECTION || 0^26 || message`; profile dùng
  bearer 1. Firmware chỉ giữ key-slot handle 1. Protected TB phải qua descriptor
  32 byte đã CRC, publish `fill/clean/release/OWN/clean/release/doorbell`, rồi chỉ
  được tiếp tục sau CRYPTO IRQ 13. Integrity fail không đổi HARQ/RLC/PDCP/state.
- Buffer identity là `(slot,generation,length)`; generation mismatch là stale
  handle. DMA chỉ đi `FREE→CPU_OWNED→DMA_OWNED→DONE→CPU_OWNED/FREE`; cache
  clean/release fence đứng trước device ownership, invalidate/acquire fence đứng
  trước CPU read.
- Tick là `uint64_t` virtual; so sánh dùng signed modular delta với horizon nhỏ
  hơn 2^63. RLC SN modulo 4096; PDCP COUNT modulo 2^32, SN là 18 bit thấp và HFN
  là phần cao. RLC/PDCP giữ tối đa 64 PDU out-of-order, release liên tục theo thứ
  tự và timer có generation để callback cũ không tác động state mới.
- Tám HARQ process có state, active grant association, expiry, generation,
  transmission count và soft LLR buffer riêng. NACK/timeout bounded; accept chỉ
  khớp đúng HARQ id và PDCP COUNT.
- ISR top-half chỉ ACK MMIO, chụp status/timestamp và enqueue bounded handle.
  PHY decoder, parser, crypto continuation và trace output chạy deferred.
- Mười task có storage stack cố định riêng, guard canary, fill pattern, budget và
  high-water scan. Guard/budget fail đặt `supervisor_fault`; runtime không tiếp
  tục dispatch. Domain restart release queue handles và invalidate generation,
  HARQ/reorder/crypto context tương ứng.

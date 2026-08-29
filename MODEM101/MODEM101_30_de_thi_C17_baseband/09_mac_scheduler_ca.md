# MÃ ĐỀ 09 — CARRIER-AGGREGATION NR MODEM

| Thuộc tính | Giá trị |
|---|---|
| Hình thức | Thi lập trình cá nhân gồm đúng 3 câu |
| Quy mô | 1 sinh viên |
| Thời gian | 3 tiếng |
| Tổng điểm | 100 |

## Bối cảnh kỹ thuật

Sinh viên phải tự xây dựng từ đầu **modem NR dual-connectivity/carrier-aggregation thu gọn theo lớp năng lực công khai của Exynos 5400/5410**. Trọng tâm biến thể: **nhiều component carrier, cross-carrier scheduling, band combination, QoS và per-carrier HARQ**. Đây là bài thi engineering tích hợp, không phải ba bài rời: kết quả Câu 1 là nền chạy cho Câu 2, và Câu 3 chỉ đạt điểm khi toàn modem chạy end-to-end qua virtual RF.

Đề lấy cảm hứng từ kiến trúc và tính năng công khai, không yêu cầu và không cho phép dùng firmware độc quyền. Mọi RF là mô phỏng bằng complex-IQ trong tiến trình. Không viết exploit hoặc thử nghiệm thiết bị/mạng thật.

## Hợp đồng bắt buộc toàn bài

- Yêu cầu pass tối thiểu: Bài làm phải dựng từ trắng một modem UE NR-SA end-to-end ở mức firmware C17 freestanding, gồm Armv7-R startup/linker/vector, host simulator, virtual RF/IQ, NR PHY, MAC/RLC/PDCP/RRC/NAS, state/timer/security context, fault handling và trace/output deterministic đúng schema chấm.
- Ngôn ngữ firmware: **ISO C17**; build bằng CMake do sinh viên tự tạo. Không dùng C++, exception, RTTI, STL, template hoặc compiler extension không được đề cho phép.
- Project phải tách `bb_fw_arm.elf`, `libmodem_fw_host.a` và `modem101_host`: cùng C source firmware được cross-build cho target Arm và build lại với platform shim để chạy deterministic trên host.
- Firmware source phải build sạch với `-std=c17 -ffreestanding -fno-builtin -fno-common -Wall -Wextra -Wconversion -Wshadow -Werror`; không `malloc/calloc/realloc/free` sau `bb_modem_init()`.
- Sinh viên tự viết `startup_armv7r.S`, vector table, mode stacks, `.data` copy, `.bss` zero và `linker.ld`; target dùng `arm-none-eabi-gcc -mcpu=cortex-r5 -marm -mfloat-abi=soft -nostdlib`. Assembly chỉ dành cho startup/IRQ/context switch; modem logic phải là C17.
- Sinh viên tự tạo toàn bộ thư mục, header, source, build file, parser, test và executable; **không có starter code hoặc skeleton được cấp**.
- Không dùng GNU Radio, srsRAN, OpenAirInterface, liquid-dsp, FFTW, OpenSSL, ASN.1 generator, modem/DSP stack hoặc code sao chép từ dự án khác. Primitive crypto chuyên biệt phải nằm sau hardware-like ops và chỉ dùng test key.
- Một executable tên `modem101` phải nhận `--config scenario.cfg --events events.txt --iq-in downlink.iq --iq-out uplink.iq --trace trace.jsonl`.
- `*.iq` có header little-endian `magic[4]="IQ16"`, `sample_rate:u32`, `count:u64`, tiếp theo là `count` cặp I/Q `int16`. Mọi phép đọc phải chống overflow/truncation.
- `scenario.cfg` là UTF-8 `key=value`, một khóa mỗi dòng; unknown key là lỗi. `events.txt` là `tick|source|hex_payload` và phải được xử lý đúng thứ tự tick, ổn định theo thứ tự file khi trùng tick.
- Không được truyền payload giữa hai đầu bằng side-channel. Mọi user/control data bắt buộc đi qua waveform và toàn bộ stack mà đề yêu cầu.

API tối thiểu phải tự khai báo và cài đặt:

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct bb_cpx16 { int16_t i, q; };
struct bb_iq_block {
    uint64_t t0;
    uint32_t sample_rate_hz;
    uint32_t count;
    struct bb_cpx16 *samples;
};
struct bb_buf_handle { uint16_t slot, generation; uint32_t length; };
struct bb_event {
    uint64_t tick;
    uint16_t source, type;
    struct bb_buf_handle payload;
};
struct bb_stats {
    uint64_t rx_ok, rx_crc_fail, tx_blocks, dropped, deadline_miss;
};

struct bb_rf_backend {
    int  (*tx_submit)(void *ctx, const struct bb_iq_block *block);
    int  (*rx_acquire)(void *ctx, uint64_t deadline, struct bb_iq_block *out);
    void (*rx_release)(void *ctx, struct bb_iq_block *block);
};
struct bb_platform_ops {
    uint64_t (*ticks)(void *ctx);
    uint32_t (*mmio_read32)(void *ctx, uintptr_t addr);
    void (*mmio_write32)(void *ctx, uintptr_t addr, uint32_t value);
    void (*cache_clean)(void *ctx, const void *addr, size_t len);
    void (*cache_invalidate)(void *ctx, void *addr, size_t len);
};

struct bb_config;
struct bb_modem;
struct bb_soc;
size_t bb_modem_required_memory(const struct bb_config *cfg);
int bb_modem_init(void *arena, size_t arena_len, const struct bb_config *cfg,
                  const struct bb_platform_ops *plat, void *plat_ctx,
                  struct bb_modem **out);
int bb_modem_post(struct bb_modem *m, const struct bb_event *ev);
int bb_modem_irq(struct bb_modem *m, uint16_t irq);
int bb_modem_run_until(struct bb_modem *m, uint64_t tick);
void bb_modem_get_stats(const struct bb_modem *m, struct bb_stats *out);

size_t bb_soc_required_memory(const struct bb_config *cfg);
int bb_soc_init(void *arena, size_t arena_len, const struct bb_config *cfg,
                const struct bb_rf_backend *rf, void *rf_ctx,
                struct bb_soc **out);
int bb_soc_step(struct bb_soc *soc, struct bb_modem *fw, uint64_t tick);
```

### Profile firmware bắt buộc

- Mô hình target là little-endian 32-bit, ABI kiểu Arm AAPCS32/Armv7-R; bài chạy trên Linux x86-64 nhưng mọi persistent/wire/MMIO field phải dùng `uint*_t`, không được phụ thuộc kích thước pointer của host.
- Firmware chia vùng logic `ROM`, `TEXT/RODATA`, `DATA/BSS`, `DMA_NC`, `IPC_SHM`, `KEY`; linker map hoặc báo cáo cuối phải ghi size từng vùng.
- MMIO chỉ qua `bb_mmio_read32/bb_mmio_write32` trên địa chỉ aligned; `volatile` chỉ dùng trong accessor MMIO. SHM/ring dùng C11 atomics với acquire/release.
- Cấm recursion, VLA, unbounded loop theo input, function pointer lấy từ input và cast pointer↔integer không qua `uintptr_t`.
- Mọi object truyền giữa task/DMA/AP dùng handle `{slot,generation}`; raw pointer chỉ có giá trị trong domain đang sở hữu và không xuất hiện trên wire/snapshot.

### SoC baseband tham chiếu bắt buộc

Đây là platform **BBX-R17** dùng chung cho môn học, lấy cảm hứng từ các khối được quan sát công khai trong nghiên cứu Shannon; địa chỉ dưới đây là địa chỉ của đề thi, không phải địa chỉ Samsung:

| Vùng/thiết bị | Địa chỉ | Kích thước | Quyền/ý nghĩa |
|---|---:|---:|---|
| Boot ROM | `0x00000000` | 64 KiB | RX, vector/loader |
| TCM | `0x00100000` | 512 KiB | MPU tách RX hot code và RW hot data; không W+X sau boot |
| SRAM | `0x10000000` | 2 MiB | RW, RTOS/queue/pool |
| DDR | `0x40000000` | 64 MiB | RW, IQ/soft buffer/protocol |
| AP↔CP SHM | `0x50000000` | 4 MiB | non-cacheable descriptor/ring |
| INTC | `0x60000000` | 4 KiB | IRQ mask/pending/ack |
| TIMER/WDT | `0x60001000` | 4 KiB | 64-bit tick, compare, watchdog |
| DMA | `0x60002000` | 8 KiB | 8 channels, descriptor rings |
| RF front-end | `0x60004000` | 4 KiB | RX/TX sample DMA, gain/CFO |
| FFT/FEC DSP | `0x60005000` | 8 KiB | FFT, LDPC/Polar job queues |
| CRYPTO | `0x60007000` | 4 KiB | key slots, cipher/integrity jobs |
| IPC doorbell | `0x60008000` | 4 KiB | AP↔CP notify/reset epoch |

Mỗi accelerator dùng register `CTRL +0x00`, `STATUS +0x04`, `DESC_BASE +0x08`, `DESC_COUNT +0x0c`, `IRQ_STATUS +0x10`, `IRQ_ACK +0x14`, `DOORBELL +0x18`. `CTRL`: ENABLE bit0, RESET bit1, START bit2; `STATUS`: BUSY bit0, DONE bit1, ERROR bit2, QUEUE_FULL bit3. Descriptor 32 byte, little-endian: `src:u32, dst:u32, len:u32, flags:u32, cookie:u16, generation:u16, next:u32, reserved:u32, crc32c:u32`; flags OWN=bit0, EOP=bit1, IRQ=bit2, DIR_TO_DEVICE=bit3. CRC32C phủ 28 byte đầu. Địa chỉ/length phải nằm trọn một vùng được phép; `next` phải bounded và cycle-checked. Cache line là 64 byte; publish descriptor theo thứ tự fill→clean→release fence→OWN→doorbell.

IRQ cố định: timer=1, watchdog=2, RF_RX=8, RF_TX=9, DMA=10, FFT=11, FEC=12, CRYPTO=13, IPC_RX=16. Thứ tự ưu tiên task: `L1_RX 28`, `L1_TX 27`, `MAC 24`, `RLC 20`, `PDCP 18`, `RRC 15`, `NAS 14`, `IPC 12`, `TRACE 4`, `IDLE 0`; số lớn ưu tiên cao. Queue depths lần lượt là 64/64/128/128/128/64/32/64; message inline tối đa 64 byte, payload lớn đi bằng buffer handle. Stack budgets là 16/16/12/16/12/16/16/12/8 KiB, có guard canary và high-water mark; stack overflow tạo supervisor fault, không tiếp tục chạy.

AP↔CP frame có header 28 byte: `magic='BBIP':u32, version:u16, type:u16, total_len:u32, seq:u32, epoch:u32, flags:u32, crc32c:u32`, sau đó là TLV padded 4 byte. Không serialize padding, không tin `total_len`, và response phải giữ đúng `(epoch,seq)`.

## Câu 1 (30 điểm) — MULTI-CARRIER PHY VÀ CLOCK DOMAIN

### Nhiệm vụ

Xây execution/data plane có thể nhận block IQ, chạy đúng theo virtual clock và tạo transport block có kiểm tra lỗi. Code phải tách ít nhất các module `runtime`, `memory`, `phy`, `channel`, `codec` và `trace`.

### Yêu cầu bắt buộc

- Tạo 3 FR1 carrier μ=1 và 1 FR2 carrier μ=2, mỗi carrier có grid/FFT/CFO/channel riêng.
- Cài common timebase, sample-rate conversion nguyên tỉ, slot boundary và timestamp alignment.
- Mỗi carrier có PSS/DMRS/equalizer/QAM; aggregation chỉ được thực hiện sau CRC từng transport block.
- Worker queues bounded và deterministic; carrier mất sync không được làm dừng carrier khác.

### Quy tắc engineering

- Không heap, file I/O, syscall hoặc mutex có thể chặn vô thời hạn trong ISR, symbol/slot loop và protocol fast path; toàn bộ arena/pool/ring được cấp ở init.
- ISR top-half chỉ ACK nguồn ngắt, chụp timestamp/status và enqueue work bằng bounded ring; FFT/decoder/parser chạy ở deferred task, không chạy trong ISR.
- DMA buffer có state `FREE→CPU_OWNED→DMA_OWNED→DONE→FREE`; trước/sau chuyển quyền phải gọi cache clean/invalidate qua platform ops.
- Mọi ring, descriptor, offset, sample count, FFT size và length trên wire phải được kiểm tra trước khi dùng.
- Saturation, endian, bit order, CRC coverage, timestamp và ownership phải được ghi rõ bằng type/invariant. Cấm serialize `struct` bằng `memcpy`, cấm C bit-field cho wire/MMIO và cấm dùng `volatile` thay synchronization.
- PHY phải xuất được các checkpoint: `SYNC`, `FFT`, `CHANNEL_EST`, `DEMAP`, `DECODE`, `TB_CRC`.

### Chấm Câu 1

| Hạng mục | Điểm |
|---|---:|
| Runtime, memory/buffer và virtual-time đúng | 7 |
| TX waveform đúng và deterministic | 7 |
| RX sync/equalize/demap/decode đúng | 10 |
| Validation, bounded resource và trace | 6 |

## Câu 2 (30 điểm) — CA/DC MAC, RLC VÀ CONTROL

### Nhiệm vụ

Xây phần protocol/control còn lại để modem có thể camp/attach hoặc register, tạo bearer/session và chuyển SDU qua PHY của Câu 1. Các layer chạy bằng task/message queue; cấm gọi tắt trực tiếp từ application xuống decoder.

### Yêu cầu bắt buộc

- Cài primary/secondary cell, activation/deactivation timer, per-CC HARQ và cross-carrier grant.
- Scheduler weighted proportional-fair cho eMBB/latency flow, có power/thermal/PRB budget.
- RLC split bearer phân phối segment qua carrier rồi reorder; PDCP duplicate mode cho bearer tin cậy.
- RRC validate band-combination/capability và rollback SCell add failure.

### Hợp đồng state và timer

- Mỗi state transition phải có `from`, `event`, `guard`, `action`, `to`; event sai state không được thay đổi context.
- Timer dùng virtual tick, có generation để callback cũ không tác động context mới; tick wrap phải so bằng signed modular delta.
- SN/COUNT/HARQ id phải xử lý wraparound bằng modular comparison; duplicate/reorder ngoài window bị loại có reason.
- Parser phải giới hạn tổng PDU 65.535 byte, số IE 64, depth 4 và work budget 1.000 operation/PDU.

### Chấm Câu 2

| Hạng mục | Điểm |
|---|---:|
| L2 data path, window/HARQ/bearer chính xác | 10 |
| RRC/NAS/control-state và timer chính xác | 8 |
| Concurrency, ownership, restart/reconfiguration | 6 |
| Security validation và malformed-input handling | 6 |

## Câu 3 (40 điểm) — TÍCH HỢP MODEM END-TO-END TRÊN VIRTUAL RF

### Nhiệm vụ

Ghép Câu 1 và Câu 2 thành một modem duy nhất, đồng thời viết `NetworkPeer` tối thiểu trong chính project để phát/thu waveform phía mạng. `NetworkPeer` chỉ là đối tác kiểm thử; không được dùng nó làm oracle để sửa thẳng state hoặc payload bên UE.

### Scenario bắt buộc

Đăng ký trên PCell, add hai SCell FR1 và một FR2, truyền 64 KiB; FR2 bị blockage, một FR1 thermal throttle, sau đó activate lại; flow tin cậy dùng PDCP duplication.

### Output bắt buộc

Mỗi dòng `trace.jsonl` phải có đúng các trường nền sau (có thể thêm trường):

```json
{"tick":1250,"domain":"RRC","event":"STATE","from":"CAMPED","to":"CONNECTED","id":7}
{"tick":1300,"domain":"PHY","event":"TB","dir":"DL","harq":2,"crc":true,"bytes":384}
{"tick":2400,"domain":"MODEM","event":"FINAL","registered":true,"session":true,"delivered":4096,"drops":1}
```

`stdout` kết thúc bằng đúng một dòng:

```text
MODEM_RESULT status=<PASS|FAIL> state=<STATE> tx_bytes=<N> rx_bytes=<N> crc_fail=<N> recoveries=<N>
```

Điều kiện PASS của mã đề này: **không giao trùng/mất SDU, scheduler tôn trọng budget/fairness, SCell failure không mất PCell session và báo throughput từng carrier**.

### Fault injection và kiểm tra phòng thủ

- Runner phải hỗ trợ tối thiểu: IQ truncation, CFO/delay/noise, PDU length sai, duplicate/reorder, timer trễ, queue full và restart một domain.
- Một lỗi đầu vào chỉ được tạo reject/drop/recovery có trace; không được crash, deadlock, out-of-bounds, use-after-free hoặc uncontrolled allocation.
- Chạy cùng seed/config hai lần phải cho cùng final state, counters và trace hash (bỏ trường wall-clock nếu có).

### Chấm Câu 3

| Hạng mục | Điểm |
|---|---:|
| Scenario end-to-end đi thật qua IQ và toàn stack | 16 |
| Đồng bộ state/data, recovery và fault containment | 10 |
| Correctness payload, counters và deterministic replay | 8 |
| Cấu trúc project, build sạch, test và output đúng schema | 6 |

## Yêu cầu cấu trúc code

Sinh viên phải tự tạo tối thiểu cấu trúc tương đương sau; được đổi tên nhưng không được gộp toàn bộ vào một file:

```text
CMakeLists.txt
cmake/arm-none-eabi-toolchain.cmake
arch/armv7r/startup_armv7r.S
arch/armv7r/linker.ld
include/bb/*.h
src/fw/runtime/*.c
src/fw/phy/*.c
src/fw/l2/*.c
src/fw/control/*.c
src/fw/security/*.c
src/hal/*.c
src/soc/*.c
src/host/*.c
tests/unit_*.c
```

Build chuẩn:

```bash
cmake -S . -B build-host -DCMAKE_BUILD_TYPE=Release
cmake --build build-host --parallel
cmake -S . -B build-arm -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-toolchain.cmake
cmake --build build-arm --target bb_fw_arm
arm-none-eabi-size build-arm/bb_fw_arm.elf
./build-host/modem101 --config scenario.cfg --events events.txt   --iq-in downlink.iq --iq-out uplink.iq --trace trace.jsonl
```

## Lỗi bị trừ nặng hoặc điểm liệt

- Hard-code output, bỏ qua IQ/PHY, truyền payload bằng biến dùng chung giữa UE và peer: **0 điểm Câu 3**.
- Dùng code modem/DSP/crypto bên ngoài hoặc starter của nhóm khác: xử lý theo quy chế học vụ.
- Crash/UB/hang vì malformed input: trừ tối đa 20 điểm toàn bài; sanitizer phát hiện memory error trong đường chấm: điểm correctness của module đó bằng 0.
- Phát RF, truy cập SIM/thiết bị/mạng thương mại, dùng firmware không có quyền hoặc nộp exploit: **điểm liệt toàn bài**.

## Tài liệu kỹ thuật được phép tra cứu

- 3GPP TS 38.211/212/213/214, 38.321/322/323/331; TS 24.501 và 33.501.
- Tài liệu chính thức Samsung Exynos Modem 5400/5410 để hiểu feature class, không để suy đoán chi tiết độc quyền.
- Các paper FirmWire, FirmState, BaseBridge, LORIS và LLFuzz để hiểu runtime/state/testing phòng thủ; không sao chép code.

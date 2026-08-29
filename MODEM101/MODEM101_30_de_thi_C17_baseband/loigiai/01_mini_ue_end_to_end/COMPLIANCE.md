# Đối chiếu yêu cầu Mã đề 01

Tài liệu này ánh xạ trực tiếp từng nhóm yêu cầu của đề sang source và test hiện
hành. Số liệu kiểm thử được đo ngày 2026-08-28.

## Hợp đồng chung và profile firmware

| Yêu cầu | Hiện thực / bằng chứng |
|---|---|
| C17 freestanding, strict warnings | `CMakeLists.txt`: C17, không extension, `-ffreestanding -fno-builtin -fno-common -Wall -Wextra -Wconversion -Wshadow -Werror` |
| Host/Arm dùng cùng firmware source | `modem_fw_host` sinh `libmodem_fw_host.a`; cùng library link vào host và `bb_fw_arm.elf` Cortex-R5 |
| Startup/IRQ/linker | `startup_armv7r.S`: vector, mode stacks, data copy, bss zero, IRQ AAPCS-aligned, CP15 MPU/cache; `linker.ld`: ROM/TCM/SRAM/DDR/SHM/KEY và EHABI index trong TCM_RX; Cortex-R5 boot-smoke |
| Không heap firmware | Không có `malloc/calloc/realloc/free` trong `src/fw`, `src/soc`, `src/hal`; arena/pool/ring cố định |
| Wire fixed-width/canonical | IQ, BMAC, protocol, IPC và descriptor encode từng byte; không serialize struct/padding/bit-field |
| Queue/handle/stack | C11 atomic acquire/release ring; pool `{slot,generation}`; queue depth theo profile; 10 storage stack riêng có guard/high-water/budget |
| MMIO/DMA/cache | MMIO chỉ qua aligned accessor; DMA state machine; clean/invalidate và fence tại ownership boundary; ARM dùng CP15 cache maintenance |
| Descriptor 32 byte | CRC32C byte 0–27, flags/address/length/cookie/generation/next/reserved và exact ring count/cycle validation trong `dma_descriptor.c` |
| AP↔CP BBIP | Header 28 byte, CRC32C, TLV padded 4, version/length/padding/IE bounds và `(epoch,seq)` trong `ipc.c` |
| Memory hardening | ELF32 ARM EABI5 soft-float; không symbol unresolved, không LOAD/GNU_STACK W+X; xem `BUILD_REPORT.md` |

## Câu 1 — 30 điểm

| Rubric | Hiện thực / test |
|---|---|
| Runtime, memory, virtual-time | Slot 0,5 ms; dispatcher chọn deadline nhỏ nhất giữa task/slot/timer; stable same-tick order, modular tick, bounded work; `unit_runtime` |
| SPSC/pool/DMA ownership | Atomic ring, generation-tagged pool, stale-handle rejection, queue-full test, cache calls và `FREE→CPU→DMA→DONE→CPU/FREE` |
| TX waveform | CRC-24A, QPSK/16-QAM Gray, grid/pilot, fixed-point IFFT, CP; `unit_q1` kiểm max TB và waveform deterministic |
| RX waveform | Ba PSS NR dài 127, timing/CFO, AGC, FFT, pilot one-tap equalization/per-symbol phase, soft demap, CRC-24A |
| Virtual channel | Seeded AWGN, CFO và delay; không có đường bit/payload tắt giữa peer và UE |
| IRQ architecture | RF_RX/RF_TX top-half chỉ ACK/chụp handle/enqueue; sync/FFT/decode chạy deferred task |
| Validation | Config unknown key, PCI/modulation/rate; IQ magic/count/overflow/truncated/trailing; sample/ring/length bounds |
| Trace | `SYNC`, `FFT`, `CHANNEL_EST`, `DEMAP`, `DECODE`, `TB_CRC`; JSONL tick đơn điệu |

## Câu 2 — 30 điểm

| Rubric | Hiện thực / test |
|---|---|
| MAC multiplex/BSR | BMAC chứa 1..8 sub-SDU control/data LCID, exact/canonical validation và atomic commit; BSR tăng khi enqueue, giảm khi TX; `unit_mac` |
| 8 HARQ process | Active-grant association, per-process COUNT/state/timer/generation/retry; full-frame signed LLR Chase combining có saturation và tham gia lần decode sau |
| RLC-UM 12-bit | Window 64 modulo 4096, fixed reorder slots, duplicate/out-of-window/collision reason, contiguous release và reorder timeout |
| PDCP 18-bit SN/HFN/COUNT | COUNT modulo 2^32, SN 18 bit/HFN, fixed reorder slots/timer, anti-replay, overlap validation và wrap tests |
| Layer task/message queue | PHY→MAC→RLC→PDCP→control bằng owned handles/queue; application không gọi decoder tắt |
| RRC/NAS | Transition table `{from,event,guard,action,to}`, đầy đủ chuỗi đề bài; wrong-state event không đổi context |
| Timer/restart | Modular due comparison, generation invalidation, HARQ/reorder timeout; PHY/L2 restart dọn queue, handles và domain context |
| NIA2/CRYPTO | AES-CMAC vector, NIA2 COUNT/BEARER/DIRECTION; sign và verify đều submit descriptor SHM qua MMIO, OWN publish, accelerator step, IRQ13 rồi mới continuation |
| Security-before-state | Toàn BMAC validate, protected sub-PDU verify trước HARQ/RLC/PDCP/control mutation; tag sai bị drop |
| Parser phòng thủ | PDU ≤65.535, IE ≤64, depth ≤4, budget 1.000, iterative; sanitizer sweep protocol/IE/IPC/MAC/CMAC/DMA |

`unit_l2` kiểm RLC/PDCP reorder, timeout, 12-bit và 32-bit wrap, replay,
integrity-before-state, conflict overlap, state guard và supervisor stack fault.

## Câu 3 — 40 điểm

Scenario trong `main.c`/`network_peer.c` đi qua `bb_rf_backend` và
`bb_soc_step` cho cả hai chiều:

1. Dò PCI 42 trong waveform tại CFO +1,8 kHz và timing offset.
2. Đi `OFF→SEARCHING→CAMPED→CONNECTING→CONNECTED`.
3. Hoàn tất security bằng crypto descriptor/IRQ; NAS đi
   `DEREGISTERED→REGISTERING→REGISTERED→SESSION_ACTIVE`.
4. Chuyển SDU deterministic 4096 byte qua BMAC/PHY/RLC/PDCP và so sánh
   byte-for-byte.
5. Erase phần còn lại của một codeword sau ba symbol: CRC fail tại tick 2800,
   lưu LLR, HARQ NACK/RETX và combine thành công tại tick 4100.
6. Gửi RLC SN2 trước SN1 và PDCP COUNT lệch để buộc cả hai reorder buffer/release;
   phát lại COUNT cũ để kiểm `PDCP_REPLAY_DROP`.
7. Inject link-loss, tự search/camp/connect lại và trở về DATA.
8. Chạy malformed PDU qua IQ, IQ truncation test, delayed timer, queue full,
   raw IRQ, PHY/L2 domain restart và same-tick ordering.

Điều kiện PASS còn đòi nội bộ: registration, session, 4096 byte khớp, ít nhất
một CRC fail, HARQ RETX và LLR combine, RLC/PDCP reorder, replay drop và hai lần
recovery. Stdout chỉ được in PASS khi tất cả cùng đúng.

## Verification matrix

| Test | Phạm vi |
|---|---|
| `unit_q1` | CRC/FFT/literal PSS fixture/QAM/OFDM/channel/max TB |
| `unit_q2` | CMAC/NIA2/protocol/IE/IPC/DMA descriptor |
| `unit_runtime` | deferred RF, ownership/cache, task stack, virtual-time, queue full |
| `unit_l2` | security-before-state, reorder/wrap/timer/replay, interleaved HARQ, restart guard |
| `unit_defensive` | 2.000 deterministic malformed cases + random-IQ decode |
| `unit_mac` | multi-LCID round-trip, canonical errors, multiplexed stack dispatch |
| `integration_full` | scenario bắt buộc end-to-end |
| `reject_truncated_iq` | IQ reader truncation |
| `deterministic_replay` | hai lượt IQ/JSONL byte-identical |
| `integration_iq_input` | phát file IQ rồi nạp lại qua `--iq-in` |
| `arm_elf_audit` | ABI ELF32 little-endian soft-float, undefined symbol, W+X và vị trí `.ARM.exidx` |
| `arm_qemu_smoke` | cross-link ELF32 Cortex-R5, MPU/startup và vào `bb_firmware_main` |

Project là profile modem virtual-RF thu gọn của đề, không tuyên bố tương thích
bit-for-bit với mạng 3GPP thương mại và không truy cập RF/SIM/mạng thật.

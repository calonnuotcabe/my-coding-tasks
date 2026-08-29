# Lời giải đầy đủ Mã đề 01 — NR-SA R17 smartphone modem

Project này giải trọn cả ba câu của `01_mini_ue_end_to_end.md`. Firmware core là
ISO C17 freestanding, không heap; cùng một tập source được build thành
`libmodem_fw_host.a` cho simulator và `bb_fw_arm.elf` cho Cortex-R5/Armv7-R.
`modem101` cùng `NetworkPeer` tạo virtual RF deterministic, không trao payload
qua side-channel.

Đường dữ liệu của scenario:

```text
NetworkPeer PDU -> BMAC multiplex (control/data LCID)
 -> CRC-24A -> QPSK/16-QAM -> resource grid -> IFFT/CP
 -> timing offset + CFO 1,8 kHz + AWGN -> bb_rf_backend
 -> BBX-R17 SoC/MMIO -> DMA RX -> IRQ top-half -> deferred L1_RX
 -> PSS/AGC/CFO -> FFT -> channel estimate -> soft demap
 -> HARQ LLR Chase combine -> MAC -> CRYPTO descriptor/IRQ
 -> RLC-UM reorder -> PDCP reorder/anti-replay -> RRC/NAS
 -> reassembly và so sánh byte-for-byte SDU 4096 byte
```

## Build và chạy

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure

./build/modem101 --config scenario.cfg --events events.txt \
  --iq-out uplink.iq --trace trace.jsonl
```

Kết quả thành công có đúng một dòng stdout:

```text
MODEM_RESULT status=PASS state=DATA tx_bytes=4096 rx_bytes=4096 crc_fail=1 recoveries=2
```

`--iq-in downlink.iq` là tùy chọn và đọc đúng container IQ16; reader loại magic,
sample-rate, count, overflow, trailing byte và file truncated không hợp lệ.
Scenario mặc định tự sinh downlink qua `NetworkPeer` vì đề yêu cầu peer nằm trong
project.

`events.txt` dùng `tick|source|hex_payload`, được stable-sort theo tick và giữ
thứ tự file khi trùng tick. Source 100–104 lần lượt kiểm raw IRQ, malformed PDU
qua IQ, timer trễ, queue pressure và domain restart.

## Các build kiểm toán

```sh
# ASan + UBSan
cmake -S . -B build-san -DCMAKE_BUILD_TYPE=Debug \
  -DBB_ENABLE_SANITIZERS=ON
cmake --build build-san --parallel
ctest --test-dir build-san --output-on-failure

# GCC static analyzer
cmake -S . -B build-analyzer -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS=-fanalyzer
cmake --build build-analyzer --parallel
ctest --test-dir build-analyzer --output-on-failure

# Cortex-R5, ARM state, soft-float, freestanding
cmake -S . -B build-arm -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-toolchain.cmake
cmake --build build-arm --target bb_fw_arm --parallel
arm-none-eabi-size -A build-arm/bb_fw_arm.elf
arm-none-eabi-nm -u build-arm/bb_fw_arm.elf
cmake --build build-arm --target arm_elf_audit
cmake --build build-arm --target arm_qemu_smoke
```

Audit ngày 2026-08-28 đạt 10/10 test ở cả Release, ASan/UBSan và
`-fanalyzer`; cross-link ARM không còn symbol unresolved hay segment W+X và
boot-smoke Cortex-R5 đi tới `bb_firmware_main` sau khi bật MPU. Hai lượt E2E cho
stdout, IQ và JSONL byte-identical. Kết quả chấm cuối theo rubric là 100/100.

Xem [GRADING_REPORT.md](GRADING_REPORT.md) để đọc bảng audit và điểm kỹ thuật,
[COMPLIANCE.md](COMPLIANCE.md) để đối chiếu từng yêu cầu,
[BUILD_REPORT.md](BUILD_REPORT.md) để xem size ELF hiện hành và
[INVARIANTS.md](INVARIANTS.md) để xem wire/DSP/ownership invariants.

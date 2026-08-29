# MODEM101 — 30 ĐỀ THI LẬP TRÌNH MODEM/BASEBAND PROCESSOR

> Phiên bản 2.0 · 2026-08-27 · Thi cá nhân · 3 giờ · 100 điểm

Đây là 30 mã đề cực nặng. Mỗi mã đề gồm đúng ba câu 30–30–40 và yêu cầu một sinh viên tự tạo từ đầu một modem hoàn chỉnh chạy trên virtual RF. Độ sát thực tế được ưu tiên hơn tính khả thi trong ba giờ.

## Nguyên tắc thiết kế

- Mỗi modem có waveform complex-IQ, PHY, L2, control plane, runtime/task/queue/timer, AP hoặc network interface và observability.
- Không có starter repository, skeleton, helper hoặc package modem. Sinh viên tự tạo toàn bộ project.
- Firmware core viết bằng ISO C17, startup/IRQ tối thiểu bằng Arm assembly, cross-build Cortex-R5-like theo AAPCS32, no-heap sau init, explicit MMIO/DMA/cache/ISR/task ownership; host simulator tách riêng.
- “Shannon-inspired” chỉ có nghĩa lấy cảm hứng từ thông tin công khai về feature class, RTOS/peripheral/message architecture; không khẳng định tái tạo thiết kế độc quyền.
- Security là secure engineering, validation, isolation, replay/fault testing và recovery. Không có bài exploit.
- Chỉ virtual RF/file IQ; không phát sóng và không đụng thiết bị, SIM hoặc mạng thật.

## Cấu trúc điểm thống nhất

| Câu | Trọng tâm | Điểm |
|---|---|---:|
| 1 | Runtime/data plane/PHY chuyên biệt | 30 |
| 2 | L2/L3/control/security chuyên biệt | 30 |
| 3 | Modem end-to-end qua virtual RF và fault recovery | 40 |

## Danh mục

- [01 — NR-SA R17 SMARTPHONE MODEM](01_mini_ue_end_to_end.md)
- [02 — SHANNON-LIKE BASEBAND SoC MODEM](02_rtos_soc_emulator.md)
- [03 — SECURE LTE BASEBAND MODEM WITH A/B FIRMWARE](03_secure_boot_firmware_update.md)
- [04 — AP↔CP SPLIT 5G MODEM](04_ap_bp_ipc_ril.md)
- [05 — MULTI-NUMEROLOGY NR FR1/FR2 MODEM](05_nr_ofdm_transceiver.md)
- [06 — MOBILITY-FIRST 5G UE MODEM](06_cell_search_sync.md)
- [07 — NR CHANNEL-CODING ACCELERATOR MODEM](07_channel_coding_pipeline.md)
- [08 — LOW-LATENCY HARQ 5G MODEM](08_harq_softbuffer.md)
- [09 — CARRIER-AGGREGATION NR MODEM](09_mac_scheduler_ca.md)
- [10 — RELIABLE RLC-AM INDUSTRIAL 5G MODEM](10_rlc_am_engine.md)
- [11 — SECURE USER-PLANE 5G MODEM](11_pdcp_security.md)
- [12 — CAPABILITY-RICH NR RRC MODEM](12_rrc_state_machine.md)
- [13 — 5GS NAS AND SESSION MODEM](13_nas_registration_session.md)
- [14 — LTE VoLTE/IMS VOICE MODEM](14_ims_volte_media.md)
- [15 — LTE/NR DUAL-CONNECTIVITY MODEM](15_multi_rat_controller.md)
- [16 — FULL-SYSTEM SHANNON FIRMWARE MODEM EMULATOR](16_stateful_baseband_emulator.md)
- [17 — STATE-AWARE RESEARCH BASEBAND MODEM](17_state_aware_fuzzer.md)
- [18 — CONFORMANCE-GRADE PROGRAMMABLE MODEM](18_conformance_differential_lab.md)
- [19 — NR/NB-IoT NTN SATELLITE MODEM](19_nr_ntn_modem.md)
- [20 — FAULT-CONTAINED COMMERCIAL BASEBAND MODEM](20_resilient_baseband_platform.md)
- [21 — RELEASE-17 NR REDCAP INDUSTRIAL MODEM](21_nr_redcap_modem.md)
- [22 — TERRESTRIAL NB-IoT RELEASE-17 MODEM](22_nb_iot_terrestrial.md)
- [23 — LTE-M CAT-M1 eMTC MODEM](23_lte_m_emtc.md)
- [24 — NR SIDELINK PC5 V2X MODEM](24_nr_sidelink_v2x.md)
- [25 — DUAL-SIM DUAL-STANDBY BASEBAND MODEM](25_dual_sim_dsds.md)
- [26 — FR2 PHASED-ARRAY BEAM MANAGEMENT MODEM](26_fr2_beam_management.md)
- [27 — NR POSITIONING-CAPABLE UE MODEM](27_nr_positioning.md)
- [28 — PRIVATE-5G URLLC/TSN INDUSTRIAL MODEM](28_private_5g_tsn_urllc.md)
- [29 — RELEASE-17 5G MBS MULTICAST MODEM](29_5g_mbs_broadcast.md)
- [30 — RF CALIBRATION AND DIGITAL PREDISTORTION MODEM](30_rf_calibration_dpd.md)

## Hồ sơ nghiên cứu

Đọc [00_RESEARCH_CHECKLIST.md](00_RESEARCH_CHECKLIST.md) để xem checklist nguồn và phép ánh xạ từ modem thương mại/3GPP sang yêu cầu code.

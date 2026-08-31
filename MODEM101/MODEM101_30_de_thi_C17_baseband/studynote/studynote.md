# MODEM / BASEBAND TỪ SỐ 0 ĐẾN MODEM UE END-TO-END

> Lộ trình thực hành dành cho người **chưa chắc C, chưa hiểu modem/baseband, thường mới chỉ copy code người khác**.
>
> **Mục tiêu cuối của toàn bộ khóa:** không chỉ đọc hiểu từng module, mà có thể **đóng project đáp án/reference lại, mở một thư mục trắng và tự dựng lại toàn bộ mini UE end-to-end** bằng C17: build system, public API, virtual RF/IQ, PHY, runtime/memory, MAC/RLC/PDCP, RRC/NAS, security context, MMIO/DMA/SoC, host `NetworkPeer`, Armv7-R startup/linker và bộ test end-to-end.
>
> “Code lại được nguyên đáp án” ở đây nghĩa là **tự tái tạo được cùng kiến trúc, contract, data flow, state flow, ownership flow và hành vi kiểm thử**, không phải học thuộc rồi chép lại từng dòng source của reference.

---

## -1. Hợp đồng dạy học đã tổng hợp lại từ yêu cầu

Tài liệu này phải phục vụ đúng một kiểu người học cụ thể:

```text
chưa biết modem là hệ thống gì
+ chưa hiểu baseband/IQ/PHY/L2/L3
+ C còn yếu, nhiều lúc chỉ biết copy code
+ chưa biết chia một project lớn thành bài nhỏ
```

Vì vậy khóa học **không được** giả định rằng người học đã biết sẵn DSP, 5G,
firmware hay cách tổ chức project C. Nó cũng không được ném ra một đoạn code,
cho thấy output đúng rồi chuyển sang bài khác.

Mỗi bài chính phải trả lời đủ bảy câu sau:

1. **Bài toán code là gì?** Viết hàm/chương trình nào, input và output cụ thể ra sao?
2. **Cần biết C gì để làm?** Kiểu dữ liệu, vòng lặp, pointer, bit, struct hay API nào?
3. **Tự kiểm tra thế nào?** Test nhỏ, output mong đợi và edge case là gì?
4. **Vừa code khối gì trong modem?** Representation, DSP, runtime, protocol hay control?
5. **Vì sao modem cần khối này?** Nếu bỏ nó hoặc làm sai thì stage nào phía sau hỏng?
6. **Dữ liệu đi đâu tiếp?** Output của bài được bài/module nào tiêu thụ?
7. **Nó nằm ở đâu trong đáp án?** File hoặc nhóm hàm reference nào hiện thực concept đó?

Đây là “hộ chiếu concept” bắt buộc sau mỗi bài:

```text
INPUT
  ↓
CODE VỪA VIẾT
  ↓
OUTPUT
  ↓
KHỐI MODEM DÙNG OUTPUT ĐÓ TIẾP
```

Nếu chỉ chạy ra đúng output nhưng chưa điền được bốn ô trên thì bài **chưa hoàn
thành**.

### Cách đưa lời giải phải tinh tế với người mới

- **Bài 1:** hướng dẫn sát tay và có full code ngay tại chỗ để người chưa chắc C
  biết cách tạo file, compile, chạy và đọc output.
- **Bài 2 trở đi:** đề bài xuất hiện trước, rồi mới tới dự đoán, kiến thức vừa đủ,
  tự code, test và debug. Full implementation nằm trong khối `<details>` đóng mặc
  định ở Bước 7 của chính bài đó.
- **Không phải lật lên xuống:** lời giải ở ngay tại bài, nhưng chỉ hiện khi người học
  chủ động mở sau khi đã tự làm thật.
- **Sau khi xem solution:** đóng lời giải, xóa phần vừa copy nếu có, tự viết lại
  một lần không nhìn.
- **Khi vẫn bí:** làm theo gợi ý debug ở Bước 6 trước; chỉ mở solution sau khi đã
  thử compile, quan sát output/state và sửa ít nhất một lần.

### Hai đường học chạy song song

| Đường học C | Đường học modem/baseband |
|---|---|
| `struct cpx16` | complex baseband sample I/Q |
| array + `count` | một block waveform |
| pointer | DSP in-place và buffer boundary |
| integer width + saturation | fixed-point signal processing |
| bit/endian/parser | wire format, MMIO, descriptor, PDU |
| ring/pool/atomics | IRQ, task queue, ownership |
| enum/state machine/timer | RRC, NAS, HARQ, recovery |
| module/CMake/test | firmware host/Arm và integration |

Không học xong toàn bộ C rồi mới học modem. Mỗi concept C được giới thiệu đúng
lúc nó giúp xây một mảnh modem. Ngược lại, mỗi concept modem phải rơi xuống một
object, function, buffer, state hoặc test cụ thể trong C.

### Chuẩn “hiểu”, không chỉ “đã đọc”

Một bài có bốn mức hoàn thành:

```text
Mức 1 — chạy được
Mức 2 — giải thích được từng input/output và invariant
Mức 3 — sửa được một yêu cầu mà không nhìn solution
Mức 4 — viết lại được từ file trắng và tự tạo test
```

Mục tiêu cuối khóa là Mức 4 cho toàn project `modem101`, không phải chỉ cho các
hàm toy.

---

## 0. Cách dùng tài liệu này

Tài liệu này không được thiết kế để đọc một lèo như sách lý thuyết. Mỗi level dùng vòng lặp:

```text
Đề bài nhỏ
   ↓
Tự đoán chương trình cần làm gì
   ↓
Học đúng lượng kiến thức cần cho bài đó
   ↓
Tự code
   ↓
Chạy test / quan sát output
   ↓
Debug
   ↓
Mở lời giải thu gọn ngay dưới bài
   ↓
Viết lại không nhìn lời giải
```

Quy tắc học:

1. Không copy nguyên lời giải rồi tự coi là đã hiểu.
2. Sau khi xem lời giải, đóng file và viết lại bằng trí nhớ.
3. Mỗi hàm phải trả lời được ba câu:
   - input là gì?
   - output là gì?
   - invariant / điều kiện nào phải luôn đúng?
4. Với DSP, luôn test bằng input nhỏ có thể tính tay trước.
5. Với firmware, ưu tiên code bounded, deterministic, không phụ thuộc heap.
6. Với protocol, vẽ state machine trước khi code `switch`.
7. Với modem, đừng hỏi “file nào quan trọng nhất?”. Hãy hỏi “bit/sample đang đi qua layer nào?”.


### Quy tắc bố cục: một bài là một vòng khép kín

Từ Bài 1 trở đi, **không cần nhảy tới phụ lục lời giải**. Mỗi bài giữ nguyên tám
bước trong cùng một chỗ:

```text
Đề bài nhỏ
   ↓
Tự đoán chương trình cần làm gì
   ↓
Học đúng lượng kiến thức cần cho bài đó
   ↓
Tự code
   ↓
Chạy test / quan sát output
   ↓
Debug
   ↓
Đọc lời giải thu gọn ngay tại bài
   ↓
Viết lại không nhìn lời giải
```

Khối lời giải đóng mặc định. Vì vậy tài liệu vừa không làm lộ đáp án sớm, vừa
không bắt người học cuộn/lật qua hàng nghìn dòng rồi quên mình đang debug gì.

### Tiêu chuẩn “bài code” của tài liệu

Một câu hỏi chọn type, vẽ state hay đọc tool output vẫn chưa được tính là hoàn
thành nếu chưa tạo ra bằng chứng bằng code. Tất cả 41 bài chính và 8 bài kiểu dữ
liệu phải có:

```text
C source hoặc project C cụ thể
→ lệnh compile/build
→ assert hoặc expected output
→ một boundary/failure test
→ lời giải code để đối chiếu
```

Ngoại lệ duy nhất về cách **chạy** là code bare-metal: không chạy trực tiếp như
chương trình host, nhưng vẫn phải tự viết C, build ELF và kiểm tra symbol/section.


### Đích đến của khóa

Project đích có hình dạng gần như:

```text
Application / test peer
        │
        ▼
      NAS
        │
       RRC
        │
      PDCP
        │
       RLC
        │
       MAC
        │
       PHY
        │
 complex IQ samples
        │
 virtual RF / channel
```

Nhưng firmware không chỉ có protocol stack. Nó còn có:

```text
IRQ → queue → scheduler → task
             │
             ├── buffer pool
             ├── DMA ownership
             ├── MMIO
             ├── timer
             ├── trace
             └── watchdog / fault handling
```

Đề tham chiếu yêu cầu C17 freestanding, Armv7-R, virtual RF/IQ, PHY và L2/L3, nên khóa này đi theo đúng thứ tự prerequisite thay vì lao thẳng vào 5G.

---


<!-- MODEM_CONCEPT_PRIMER_V2 -->
<!-- WIZARD_TRACK_FRONT_BEGIN -->
### Giao thức Adjacent Practice Forge — đề và đáp án đan vào từng checkpoint

Các card `Forge F01..F49` được đặt ngay cuối bài tương ứng, trước khi sang bài
kế tiếp. Mỗi card có ba biến thể và một đáp án C17 đóng mặc định bằng
`<details>`. Cách làm bắt buộc:

```text
đọc ba contract
→ tạo file trắng và test trước
→ code cả ba biến thể
→ compile warning-as-error + sanitizer
→ debug ít nhất một lần
→ mở đáp án ngay tại checkpoint
→ đóng đáp án, xóa implementation luyện và viết lại
```

Không tính việc chỉ đổi tên biến/literal là một biến thể. Một biến thể hợp lệ
phải đổi boundary, representation, ownership, time, fault hoặc composition.

### Giao thức Syntax Lens — không bắt người học lật lên/lật xuống

Từ PHẦN 0, lần đầu một syntax xuất hiện, ngay dưới code phải có đủ:

```text
syntax được đọc từ trái sang phải thế nào
type nào đi với format/literal/cast nào
object/pointer/array đang sống ở đâu
failure hoặc UB nào dễ xảy ra
một biến thể phải tự code
đáp án đóng ngay tại chỗ
```

Các ô `Syntax ngay dưới MINI CODE 0A..0R` là lớp học song song với concept modem.
Mục 1B.31A chỉ là bảng tổng hợp về sau, không phải nơi người học lần đầu được
biết `%zu`, `PRIu64` hay default promotion. Khi một bài cần lại syntax quan trọng,
bài đó nhắc lại ngay tại chỗ thay vì bảo “xem phần trước”. Sự lặp này là deliberate
practice, không phải lỗi biên tập.

**Cách đọc dành riêng cho beginner trong tài liệu này:** mỗi Syntax Lens có hai
tầng, nhưng ở lượt đầu chỉ tầng thứ nhất là điều kiện để đi tiếp:

```text
BEGINNER GATE — phải tự gõ, chạy được và giải thích bằng lời thường
ENGINEERING LENS — đọc để nhận diện; chưa phải thuộc lòng thuật ngữ/ABI/UB
```

Nếu một dòng bắt đầu bằng `BEGINNER GATE`, hãy dừng và làm bài nhỏ ngay dưới nó.
Nếu làm được, người học đã đủ prerequisite cho concept modem kế tiếp. Những từ
như *promotion*, *decay*, *variadic*, *storage duration*, *LP64/LLP64* là tên chính
xác của hiện tượng vừa quan sát; ở PHẦN 0 chỉ cần nối được **input → phép biến đổi
→ output**, chưa cần đọc thuộc định nghĩa chuẩn C.

## TRACK PHÙ THỦY — BIẾT NGHỊCH, BIẾT TỰ CHẾ HELPER, VẪN LÀM CORE RẤT CHẮC

Phiên bản này giữ nguyên lộ trình core, nhưng mỗi bài có thêm **Bước 9 — Wizard
Lab**. Bước 9 không có nhiệm vụ dạy thêm một đáp án. Nó luyện khả năng:

> **Làm rõ câu trên:** “không có nhiệm vụ dạy thêm một đáp án” không có nghĩa
> chỉ ném prototype rồi bỏ người học tự bơi. Mỗi Bước 9 **luôn phải có code**:
> helper implementation thật, `main()`/`assert`, lệnh compile và expected
> output. Source nằm trong khối “Phao cứu sinh” đóng mặc định để người học tự
> nghĩ trước, nhưng chắc chắn có đáp án để đối chiếu và học tiếp.

Hợp đồng bắt buộc của mọi Wizard Lab:

```text
ý nghĩ quái dị
→ tự viết prediction
→ prototype/contract để tự code
→ tự nối helper thành experiment hoặc scenario script
→ observer + invariant
→ phao cứu sinh: full C source chạy được
→ đóng lời giải và viết lại
→ tự thay parameter, ghép helper hoặc tạo biến thể chưa có trong đáp án
```

Phao không thay người học bơi. Phao bảo đảm người chưa chắc C không bị mắc vô
hạn ở một lỗi cú pháp, đồng thời cho thấy cách biến một suy nghĩ thành code rõ
type, bounds, ownership và test.

```text
nghĩ một điều chưa có trong đề
→ chỉ ra object C cần thay đổi
→ viết helper nhỏ nhất
→ tạo baseline
→ dự đoán trước khi chạy
→ mutate đúng một biến
→ quan sát bằng dump/metric/digest/property
→ giải thích first divergence
→ tự chế biến thể tiếp theo
```

### Ba mode khi học một bài

```text
MODE HỌC VIÊN
    làm Bước 1–8, hiểu concept và viết lại được

MODE KỸ SƯ
    thêm bounds, ownership, failure atomicity và test xấu

MODE PHÙ THỦY
    dùng Bước 9 để biến helper thành đồ chơi khám phá
```

Không được dùng Wizard Mode để trốn core. Nếu `grid_move_bin()` rất đẹp nhưng
bạn vẫn chưa hiểu array, pointer và logical-bin mapping thì đó chỉ là copy code
trang trí.

### Quy tắc “một lần nghĩ, ba lần code”

Mỗi ý tưởng phải đi qua ba phiên bản:

```text
V1 — code thẳng một case để nhìn thấy hiện tượng
V2 — rút case đó thành helper có parameter
V3 — thêm validation + observer + property test
```

Ví dụ:

```text
V1: tự gán bins[3] sang bins[13]
V2: grid_move_bin(grid, +3, -3)
V3: invalid bin không sửa grid; dump/digest; changed_bins == 2
```

### Sáu câu phải viết trước mỗi trò nghịch

1. Tôi đang sửa **representation** nào: byte, bit, symbol, bin, IQ, event hay state?
2. Helper mới nhận input gì, sửa object nào, trả status gì?
3. Trước khi chạy, tôi dự đoán chính xác sample/bin/field nào thay đổi?
4. Điều gì bắt buộc giữ nguyên?
5. Observer nào chứng minh prediction đúng/sai?
6. Nếu kết quả lạ, experiment nhỏ hơn tiếp theo là gì?

### Phạm vi tuyệt đối

Track này chỉ gồm C và modem/baseband simulator:

```text
IQ / modulation / OFDM bins / channel / synchronization
runtime / queue / timer / ownership / PDU / state machine
```

Mọi thí nghiệm dừng ở C local, virtual IQ và modem simulator; không mở rộng ra
ngoài phạm vi modem/baseband.

<!-- WIZARD_TRACK_FRONT_END -->

---

# PHẦN 0 — MODEM/BASEBAND: HỌC CONCEPT SONG SONG VỚI CODE


Phần này vẫn là phần nhập môn, nhưng không còn bắt bạn đọc một cục lý thuyết dài rồi mới đụng code.

Ta sẽ học theo nhịp:

    CONCEPT RẤT NHỎ
          ↓
    MINI CODE RẤT NHỎ
          ↓
    NHÌN OUTPUT
          ↓
    GIẢI THÍCH CODE ĐANG ĐỨNG Ở ĐÂU TRONG MODEM
          ↓
    NỐI SANG CONCEPT KẾ TIẾP

Các đoạn MINI CODE 0A, 0B, 0C... không tính là Bài 1, Bài 2 của khóa.

Mục đích của chúng là tạo mental model trước khi vào bài tập thật.

Với đúng persona “C còn yếu và hay copy code”, mỗi MINI CODE được học theo hai
nhịp. Nhịp bắt buộc là chạy code và vượt `BEGINNER GATE`; nhịp tăng cường là đọc
phần contract/portability ngay sau đó. Không được tự kết luận mình kém chỉ vì chưa
nhớ tên một quy tắc ABI ở lượt đầu.

Ở PHẦN I:

    Bài 1  → vẫn được hướng dẫn sát.
    Bài 2+ → bắt đầu phải tự code trước.
    Ngay dưới mỗi bài → có full solution thu gọn để đối chiếu, không phải lật file.

Trong PHẦN 0, code cố ý nhỏ và đôi khi là mô hình học tập thu gọn.

Đừng nhầm:

    code toy để hiểu concept

với:

    implementation 5G NR hoàn chỉnh theo 3GPP


## 0.1. Trước hết: modem đang giải quyết gì?


Giả sử có một byte:

    0x41

Đó là chữ:

    'A'

Máy tính hiểu byte.

Nhưng antenna không phát một biến C kiểu:

    uint8_t data = 0x41;

ra ngoài không khí.

Radio cần một tín hiệu thay đổi theo thời gian.

Vì vậy modem phải làm một chuỗi biến đổi.

TX cực kỳ thu gọn:

    byte
      ↓
    bits
      ↓
    symbols
      ↓
    waveform
      ↓
    channel

RX làm gần như ngược lại:

    waveform
      ↓
    estimated symbols
      ↓
    bits
      ↓
    byte

Tên "modem" xuất phát từ:

    modulator
    demodulator

Nhưng modem điện thoại hiện đại lớn hơn hai khối đó rất nhiều.

Nó còn phải:

    tìm cell
    đồng bộ timing
    sửa lệch tần số
    kiểm tra lỗi
    retransmit
    giữ sequence number
    quản lý state radio
    đăng ký mạng
    tạo data session
    quản lý buffer
    xử lý interrupt
    chạy đúng deadline

Vì vậy đừng mental-model modem thành:

    send_packet();

Hãy mental-model modem thành:

    rất nhiều stage nhỏ nối với nhau.


### MINI CODE 0A — Một byte trước khi nó thành signal


Đây là C rất cơ bản:

    #include <stdint.h>
    #include <stdio.h>

    int main(void)
    {
        uint8_t data = 0x41;

        printf("hex = 0x%02x\n", (unsigned)data);
        printf("char = %c\n", (char)data);

        return 0;
    }

Compile:

    gcc -std=c17 -Wall -Wextra -Werror mini0a.c -o mini0a
    ./mini0a

Expected:

    hex = 0x41
    char = A

<!-- SYNTAX_LENS_0A -->
#### Syntax ngay dưới MINI CODE 0A — đọc được từng ký hiệu trước khi đi tiếp

> **BEGINNER GATE 0A:** tự đổi đúng một biến `data`, dự đoán ba cách in rồi mới
> chạy. Chỉ cần nói được “một byte, ba cách quan sát”; thuật ngữ `variadic` ở dưới
> là tên kỹ thuật để học dần.

Đừng chỉ chạy ra đúng output. Hãy đọc từng mảnh C ngay tại đây:

```c
#include <stdint.h>  /* khai báo uint8_t */
#include <stdio.h>   /* khai báo printf */

int main(void)       /* entry point, không nhận argument */
```

```c
uint8_t data = 0x41;
```

- `uint8_t`: số nguyên unsigned đúng 8 bit, phù hợp một binary byte.
- `data`: tên object.
- `=`: khởi tạo object bằng giá trị bên phải.
- `0x41`: integer literal viết ở hệ 16; cùng giá trị số với decimal `65`.
- `;`: kết thúc statement.

Dòng format:

```c
printf("hex = 0x%02x\n", (unsigned)data);
```

được đọc như sau:

| Cú pháp | Nghĩa ngay trong dòng này |
|---|---|
| `printf(...)` | gọi function có format string |
| `%x` | in argument unsigned ở hexadecimal |
| `2` trong `%02x` | field rộng tối thiểu 2 ký tự |
| `0` trong `%02x` | thiếu ký tự thì pad bằng zero |
| `\n` | newline, không phải hai ký tự `\` và `n` khi chạy |
| `(unsigned)data` | convert rõ argument thành type `%x` đang đòi |

Dòng:

```c
printf("char = %c\n", (char)data);
```

dùng `%c` để in một character. Cast `(char)` ở đây nói rõ ta muốn nhìn cùng
bit-pattern như text; binary data trong modem vẫn nên giữ type `uint8_t`.

`printf` là variadic: sau format string, function không nhận metadata runtime
đầy đủ để tự sửa type sai. Format và type argument phải khớp; warning format
phải được coi là build error.

**Tự code ngay:** in cùng `data` ở decimal, hex hai digit và character. Sau đó
đổi thành `0x0A` để thấy `%02x` giữ leading zero.

<details>
<summary>Đáp án syntax 0A — mở sau khi tự gõ</summary>

```c
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    uint8_t data = UINT8_C(0x0A);
    printf("dec=%u hex=0x%02x char=%c\n",
           (unsigned)data,
           (unsigned)data,
           (int)(char)data);
    return 0;
}
```

`%c` cũng nhận promoted `int`, nên cast cuối thành `int` làm contract của lời
gọi lộ rõ. Character `0x0A` chính là newline, vì vậy phần `char=` có thể trông
khác chữ `A`; đó là hành vi của data, không phải lỗi `printf`.

</details>


### Đang học gì?


Chưa có radio ở đây.

Ta chỉ đang xác định điểm bắt đầu của data flow:

    application data
          ↓
        0x41

Sau này chính byte này sẽ phải đi qua nhiều representation khác.

Một tư duy cực quan trọng:

    cùng một thông tin

có thể được biểu diễn dưới nhiều dạng khác nhau.

Ví dụ cùng chữ A:

    ký tự       : 'A'
    integer     : 65
    hexadecimal : 0x41
    bit pattern : 01000001

Sau này tiếp tục có:

    QPSK symbols
    OFDM bins
    IQ samples

Thông tin vẫn là thông tin đó, nhưng representation đã thay đổi.


## 0.2. Byte không tự nhiên biến thành sóng


Muốn truyền data số, trước hết ta phải nhìn byte dưới dạng bit.

0x41 bằng:

    0 1 0 0 0 0 0 1

Nếu chỉ quen copy code, bạn rất dễ thấy:

    x >> n
    x & 1

rồi nhớ công thức mà không hiểu nó làm gì.

Hãy nhìn bằng data flow:

    một byte
       ↓
    chọn vị trí bit
       ↓
    lấy ra 0 hoặc 1


### MINI CODE 0B — Tách một byte thành 8 bit


    #include <stdint.h>
    #include <stdio.h>

    int main(void)
    {
        uint8_t data = 0x41;

        for (int bit = 7; bit >= 0; --bit) {
            uint8_t value = (uint8_t)((data >> (unsigned)bit) & 1U);
            printf("%u", (unsigned)value);
        }

        printf("\n");
        return 0;
    }

Expected:

    01000001

<!-- SYNTAX_LENS_0B -->
#### Syntax ngay dưới MINI CODE 0B — `for`, shift, mask và promotion

> **BEGINNER GATE 0B:** viết bảng tám dòng `bit=7..0`, tự tính riêng
> `(data >> bit) & 1U` cho `data=0x81`, rồi mới chạy loop. Chưa cần tự viết helper
> có validation trước khi đọc xong Lens.

Đọc header vòng lặp:

```c
for (int bit = 7; bit >= 0; --bit)
```

| Mảnh | Nghĩa |
|---|---|
| `int bit = 7` | chạy một lần trước vòng lặp |
| `bit >= 0` | check trước mỗi lượt |
| `--bit` | giảm một sau mỗi lượt |
| `{ ... }` | block được lặp |

Ở đây dùng `int`, không dùng `size_t`, vì biến phải đi qua `0` rồi dừng; một
unsigned loop index không thể biểu diễn `-1` theo cách mental model này.

Biểu thức:

```c
(uint8_t)((data >> (unsigned)bit) & 1U)
```

được đọc từ trong ra ngoài:

1. `(unsigned)bit`: shift count đã biết không âm vì loop guard.
2. `data >> ...`: đưa bit cần đọc xuống vị trí thấp nhất.
3. `& 1U`: xóa mọi bit trừ bit thấp nhất.
4. `(uint8_t)`: thu hẹp kết quả đã chứng minh chỉ có thể là `0` hoặc `1`.

Suffix `U` trong `1U` nói literal là unsigned. Còn `(unsigned)value` trong:

```c
printf("%u", (unsigned)value);
```

làm argument khớp `%u`. Các type hẹp như `uint8_t` chịu integer promotion trong
biểu thức/variadic call; cast ở bài nhập môn giúp contract hiện ra trước mắt.

**Tự code ngay:** viết `get_bit_msb(byte,index)` và `get_bit_lsb(byte,index)`;
test `0x81`, index `0` và `7` để thấy “bit order” là contract chứ không là chân lý
tự nhiên của byte.

<details>
<summary>Đáp án syntax 0B — hai cách đánh số bit</summary>

```c
#include <stdint.h>

static uint8_t get_bit_lsb(uint8_t byte, unsigned index)
{
    return (uint8_t)((byte >> index) & 1U);
}

static uint8_t get_bit_msb(uint8_t byte, unsigned index)
{
    return (uint8_t)((byte >> (7U - index)) & 1U);
}
```

Full file để chạy cả hai convention thay vì copy riêng hai helper:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static uint8_t get_bit_lsb(uint8_t byte, unsigned index)
{
    assert(index < 8U);
    return (uint8_t)((byte >> index) & 1U);
}

static uint8_t get_bit_msb(uint8_t byte, unsigned index)
{
    assert(index < 8U);
    return (uint8_t)((byte >> (7U - index)) & 1U);
}

int main(void)
{
    const uint8_t byte = UINT8_C(0x81);

    for (unsigned index = 0U; index < 8U; ++index) {
        printf("index=%u lsb=%u msb=%u\n", index,
               (unsigned)get_bit_lsb(byte, index),
               (unsigned)get_bit_msb(byte, index));
    }
    assert(get_bit_lsb(byte, 0U) == 1U);
    assert(get_bit_lsb(byte, 7U) == 1U);
    assert(get_bit_msb(byte, 0U) == 1U);
    assert(get_bit_msb(byte, 7U) == 1U);
    return 0;
}
```

Compile file này thành `build/beginner_0b`; sau đó đổi `0x81` thành `0x42` và dự
đoán bốn assert nào phải sửa trước khi chạy lại.

```bash
gcc -std=c17 -Wall -Wextra -Werror -Wformat=2 -pedantic \
    practice/beginner_0b.c -o build/beginner_0b
./build/beginner_0b
```

Hai function chỉ hợp lệ khi `index < 8U`; bản production phải validate trước
shift để tránh shift count ngoài width.

</details>


### Giải thích từng ý


Dòng:

    data >> bit

dịch bit cần xem về vị trí thấp nhất.

Sau đó:

    & 1U

xóa mọi bit khác và chỉ giữ bit cuối.

Ví dụ tưởng tượng:

    data = 01000001

muốn lấy bit số 6:

    01000001 >> 6
    =
    00000001

rồi:

    00000001 & 00000001
    =
    00000001

kết quả là 1.


### Nó nằm ở đâu trong modem?


Bit manipulation xuất hiện gần như khắp modem:

    PHY coding
    MAC header
    RLC header
    PDCP COUNT
    descriptor flags
    MMIO register bits
    protocol parser

Đây chưa phải "code modem 5G".

Nhưng đây là alphabet mà code modem dùng để diễn đạt dữ liệu.


## 0.3. Baseband: bỏ carrier GHz khỏi bài toán


Radio thật có thể phát quanh carrier rất cao:

    hàng trăm MHz
    vài GHz

Nhưng DSP trong modem không muốn mọi thuật toán phải trực tiếp xử lý một sin/cos ở vài GHz.

Một mental model đơn giản:

                     RF / passband
                         GHz
                          │
    antenna → RF front-end / mixer / ADC
                          │
                          ▼
                  complex baseband
                    I[n] + jQ[n]
                          │
                          ▼
                       DSP

Baseband ở đây là representation thuận tiện hơn cho xử lý số.

Ta thường biểu diễn một sample bằng hai thành phần:

    I = in-phase
    Q = quadrature

Một sample:

    x[n] = I[n] + jQ[n]

Đừng vội sợ chữ j.

Trước mắt hãy coi:

    (I, Q)

là một điểm 2D.


### MINI CODE 0C — Complex sample đầu tiên


    #include <stdint.h>
    #include <stdio.h>

    struct cpx16 {
        int16_t i;
        int16_t q;
    };

    int main(void)
    {
        struct cpx16 sample = {1200, -450};

        printf("I=%d Q=%d\n",
               (int)sample.i,
               (int)sample.q);

        return 0;
    }

Expected:

    I=1200 Q=-450

<!-- SYNTAX_LENS_0C -->
#### Syntax ngay dưới MINI CODE 0C — `struct`, field, signed width và `%d`

> **BEGINNER GATE 0C:** tạo hai object `a` và `b`, đổi riêng `a.i`, rồi in cả hai
> để thấy “định nghĩa type” khác “tạo object”.

Declaration:

```c
struct cpx16 {
    int16_t i;
    int16_t q;
};
```

tạo một **type layout** có hai field. Nó chưa tự tạo object nào. Dòng:

```c
struct cpx16 sample = {1200, -450};
```

mới tạo object `sample` và khởi tạo field theo thứ tự declaration. Dấu `-` là
unary minus áp lên literal `450`; vì IQ có thể âm nên storage dùng `int16_t`.

Truy cập field object trực tiếp dùng dấu chấm:

```c
sample.i
sample.q
```

Trong lời gọi:

```c
printf("I=%d Q=%d\n", (int)sample.i, (int)sample.q);
```

`%d` đòi argument type `int`. `int16_t` thường được integer-promote thành `int`,
nhưng cast tường minh ở bài đầu giúp người học nhìn thấy type mà variadic call
nhận. Sang code generic/fixed-width, `<inttypes.h>` còn cho `PRId16`; chưa cần
nhảy tới macro đó trước khi hiểu `%d`.

**Tự code ngay:** tạo sample bằng positional initializer và designated
initializer, rồi chứng minh hai object bằng nhau.

<details>
<summary>Đáp án syntax 0C — designated initializer</summary>

```c
struct cpx16 a = {1200, -450};
struct cpx16 b = {.q = -450, .i = 1200};

printf("same=%d\n", (a.i == b.i) && (a.q == b.q));
```

Full file có cả positive test và mutation test:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

struct cpx16 {
    int16_t i;
    int16_t q;
};

static int cpx16_equal(struct cpx16 a, struct cpx16 b)
{
    return (a.i == b.i) && (a.q == b.q);
}

int main(void)
{
    struct cpx16 a = {1200, -450};
    struct cpx16 b = {.q = -450, .i = 1200};

    assert(cpx16_equal(a, b));
    a.i = 1201;
    assert(!cpx16_equal(a, b));
    printf("a=(%d,%d) b=(%d,%d) same=%d\n",
           (int)a.i, (int)a.q, (int)b.i, (int)b.q,
           cpx16_equal(a, b));
    return 0;
}
```

```bash
gcc -std=c17 -Wall -Wextra -Werror -Wformat=2 -pedantic \
    practice/beginner_0c.c -o build/beginner_0c
./build/beginner_0c
```

Designated initializer gắn giá trị theo tên field nên không phụ thuộc thứ tự viết
trong initializer; output predicate là `1` khi hai field cùng bằng nhau.

</details>


### Tại sao lại là struct?


Ta có hai giá trị luôn đi cùng nhau:

    I
    Q

Nếu viết rời:

    int16_t i;
    int16_t q;

thì vẫn chạy.

Nhưng:

    struct cpx16

nói rõ với người đọc code:

    "đây là một complex sample"

Thay vì:

    "đây là hai số nguyên chẳng biết liên quan gì nhau"


### Một sample có phải là cả sóng không?


Không.

Một sample chỉ giống một ảnh chụp tại một thời điểm.

Waveform là:

    sample[0]
    sample[1]
    sample[2]
    sample[3]
    ...

Tức là một chuỗi rất nhiều điểm I/Q.


## 0.4. Waveform trong code thực ra là buffer


Khi nói:

    receiver nhận waveform

trong code thường nghĩa là:

    receiver có một buffer complex samples

Ví dụ:

    struct cpx16 samples[1024];

Đây là một thay đổi mental model quan trọng.

Từ ngữ vật lý:

    tín hiệu
    waveform
    radio samples

khi rơi xuống software thường trở thành:

    struct
    array
    pointer
    count
    buffer


### MINI CODE 0D — Một waveform cực nhỏ


    #include <stdint.h>
    #include <stdio.h>

    struct cpx16 {
        int16_t i;
        int16_t q;
    };

    int main(void)
    {
        struct cpx16 samples[4] = {
            {100,   0},
            {200,  20},
            {300, -30},
            {400,  40}
        };

        for (size_t n = 0; n < 4; ++n) {
            printf("sample[%zu] = (%d,%d)\n",
                   n,
                   (int)samples[n].i,
                   (int)samples[n].q);
        }

        return 0;
    }

<!-- SYNTAX_LENS_0D -->
#### Syntax ngay dưới MINI CODE 0D — array, `size_t`, index và `%zu`

> **BEGINNER GATE 0D:** trước hết chỉ đổi array từ bốn thành ba phần tử, dùng
> `sizeof samples / sizeof samples[0]`, và kiểm output có đúng ba dòng. `%zu` phải
> đi cùng `size_t`; phần array decay bên dưới chỉ cần nhận diện ở lượt đầu.

Đây là lần đầu `size_t` xuất hiện, nên phải hiểu ngay tại chỗ:

```c
for (size_t n = 0; n < 4; ++n)
```

- `size_t` là unsigned integer type dành cho kích thước object, capacity và array index.
- Width của nó theo target: thường 64 bit trên host x86-64, 32 bit trên Armv7-R.
- Nó không đồng nghĩa portable với `unsigned int` hay `unsigned long`.
- Vì thế `printf` dùng `%zu`, không dùng `%u` hay đoán `%lu`.

```c
printf("sample[%zu] = ...", n);
```

`%zu` có hai phần: `z` nói argument có size type (`size_t`), `u` nói biểu diễn
decimal unsigned. Tương tự `%zx` in `size_t` ở hexadecimal.

Array:

```c
struct cpx16 samples[4];
```

tạo storage cho đúng bốn object. `samples[n]` là phần tử thứ `n`, index hợp lệ
chỉ khi `0 <= n < 4`. Trong function parameter, array thường decay thành pointer
và function không tự biết count; đó là lý do API DSP dùng `(samples,count)`.

Đừng lặp magic number `4`. Cách C tự suy count trong chính scope có array thật:

```c
size_t count = sizeof samples / sizeof samples[0];
for (size_t n = 0U; n < count; ++n) {
    printf("sample[%zu]=(%d,%d)\n", n,
           (int)samples[n].i, (int)samples[n].q);
}
```

`sizeof` trả về `size_t`, nên chính kết quả `sizeof` cũng in bằng `%zu`.

**Tự code ngay:** đổi array thành 1, 4 và 8 sample mà không sửa loop bound bằng tay.

<details>
<summary>Đáp án syntax 0D — count theo storage thật</summary>

```c
size_t count = sizeof samples / sizeof samples[0];
printf("count=%zu element_bytes=%zu total_bytes=%zu\n",
       count, sizeof samples[0], sizeof samples);
```

Full file để đổi 1/4/8 phần tử mà không chạm loop bound:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

struct cpx16 {
    int16_t i;
    int16_t q;
};

int main(void)
{
    const struct cpx16 samples[] = {
        {100, 0}, {200, 20}, {300, -30}, {400, 40}
    };
    const size_t count = sizeof samples / sizeof samples[0];

    assert(count == 4U);
    for (size_t n = 0U; n < count; ++n) {
        printf("sample[%zu]=(%d,%d)\n", n,
               (int)samples[n].i, (int)samples[n].q);
    }
    printf("count=%zu element_bytes=%zu total_bytes=%zu\n",
           count, sizeof samples[0], sizeof samples);
    return 0;
}
```

```bash
gcc -std=c17 -Wall -Wextra -Werror -Wformat=2 -pedantic \
    practice/beginner_0d.c -o build/beginner_0d
./build/beginner_0d
```

Phép chia chỉ dùng được nơi `samples` vẫn là array object. Nếu `samples` đã là
function parameter/pointer, `sizeof samples` chỉ đo kích thước pointer.

</details>


### Code này đang mô phỏng cái gì?


Không phải antenna.

Không phải packet.

Nó đang mô phỏng representation software của:

    một đoạn waveform complex-baseband rất nhỏ

Input:

    bốn cặp I,Q

Output:

    chương trình duyệt được bốn sample theo thời gian


### Tại sao phải học mảng sớm như vậy?


Bởi vì gần như mọi DSP primitive sau này đều có dạng tư duy:

    nhận pointer tới samples
    nhận count
    duyệt samples
    biến đổi samples

Ví dụ sau này:

    AGC(samples, count)
    correct_cfo(samples, count)
    fft(samples, nfft)
    correlate(samples, count, reference)

Nếu chưa quen array/pointer/count, bạn sẽ nhìn DSP code như phép thuật.


## 0.5. I/Q giữ amplitude và phase như thế nào?


Hãy tưởng tượng sample là vector:

          Q
          ^
          |
      x • |
          |
    ------+----------> I

Nếu:

    I = 1
    Q = 0

vector hướng sang phải.

Nếu:

    I = 0
    Q = 1

vector hướng lên.

Nếu:

    I = -1
    Q = 0

vector hướng sang trái.

Nếu:

    I = 0
    Q = -1

vector hướng xuống.

Do đó I/Q cho ta thông tin về:

    amplitude
    phase

Đây là lý do modulation như QPSK/QAM có thể encode bit bằng các điểm khác nhau trên mặt phẳng.


### MINI CODE 0E — Nhìn bốn quadrant


    #include <stdint.h>
    #include <stdio.h>

    struct cpx16 {
        int16_t i;
        int16_t q;
    };

    static const char *quadrant(struct cpx16 x)
    {
        if (x.i >= 0 && x.q >= 0) {
            return "I+, Q+";
        }

        if (x.i >= 0 && x.q < 0) {
            return "I+, Q-";
        }

        if (x.i < 0 && x.q >= 0) {
            return "I-, Q+";
        }

        return "I-, Q-";
    }

    int main(void)
    {
        struct cpx16 x = {-1000, 800};

        printf("%s\n", quadrant(x));
        return 0;
    }

Expected:

    I-, Q+

<!-- SYNTAX_LENS_0E -->
#### Syntax ngay dưới MINI CODE 0E — return string, `if` và logical operators

> **BEGINNER GATE 0E:** với năm điểm `(1,1)`, `(-1,1)`, `(-1,-1)`, `(1,-1)`,
> `(0,0)`, hãy dự đoán chuỗi trước khi chạy. Trục `I==0`/`Q==0` phải có policy
> viết thành code, không được phụ thuộc tình cờ vào thứ tự `if`.

```c
static const char *quadrant(struct cpx16 x)
```

đọc từ tên function ra ngoài:

```text
quadrant → function nhận một struct by-value
         → trả pointer tới char read-only qua pointer đó
```

String literal như `"I+, Q+"` có static storage duration; trả pointer tới nó hợp
lệ. Không được sửa string literal. `const char *` làm ý định read-only lộ ra.

Condition:

```c
x.i >= 0 && x.q >= 0
```

dùng `&&`: cả hai vế phải đúng. C có short-circuit: nếu vế trái sai, vế phải
không cần evaluate. `%s` trong `printf` đòi pointer tới null-terminated char
sequence; `quadrant(x)` trả đúng representation đó.

**Tự code ngay:** thêm case trục `I==0` hoặc `Q==0` theo policy riêng; đừng để
policy ẩn trong thứ tự `if`.

Ba nấc làm quen, làm lần lượt và compile sau từng nấc:

1. In quadrant của bốn điểm không nằm trên trục.
2. Thêm policy riêng cho `I==0`, `Q==0` và origin.
3. Đưa bảy điểm vào array, loop qua mọi điểm và assert chuỗi trả về.

<details>
<summary>Đáp án syntax 0E — full file có policy cho trục</summary>

Lưu thành `practice/beginner_0e.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct cpx16 {
    int16_t i;
    int16_t q;
};

static const char *quadrant(struct cpx16 x)
{
    if ((x.i == 0) && (x.q == 0)) return "ORIGIN";
    if (x.i == 0) return "Q_AXIS";
    if (x.q == 0) return "I_AXIS";
    if ((x.i > 0) && (x.q > 0)) return "I+, Q+";
    if ((x.i < 0) && (x.q > 0)) return "I-, Q+";
    if ((x.i < 0) && (x.q < 0)) return "I-, Q-";
    return "I+, Q-";
}

int main(void)
{
    const struct cpx16 points[] = {
        {1, 1}, {-1, 1}, {-1, -1}, {1, -1},
        {0, 0}, {0, 7}, {7, 0}
    };
    const char *expected[] = {
        "I+, Q+", "I-, Q+", "I-, Q-", "I+, Q-",
        "ORIGIN", "Q_AXIS", "I_AXIS"
    };
    const size_t count = sizeof points / sizeof points[0];

    for (size_t n = 0U; n < count; ++n) {
        const char *actual = quadrant(points[n]);
        printf("point[%zu]=(%d,%d) -> %s\n", n,
               (int)points[n].i, (int)points[n].q, actual);
        assert(strcmp(actual, expected[n]) == 0);
    }
    return 0;
}
```

```bash
gcc -std=c17 -Wall -Wextra -Werror -pedantic \
    practice/beginner_0e.c -o build/beginner_0e
./build/beginner_0e
```

Boundary đã được biến thành policy nhìn thấy trong code: origin được check trước
hai trục, hai trục được check trước bốn quadrant.

</details>


### Ý nghĩa modem/baseband


Bạn vừa viết một detector cực kỳ thô dựa trên dấu I/Q.

Một QPSK hard-demapper đơn giản sau này cũng dùng trực giác tương tự:

    dấu I
    dấu Q
       ↓
    quyết định bit

Tất nhiên modem thật còn noise, equalization và soft information.

Nhưng concept bắt đầu từ đây.


## 0.6. Modulation: bit bắt đầu trở thành signal point


Ta đã có:

    bits

và đã có:

    complex sample / complex point

Giờ cần nối hai thế giới.

Một mapping QPSK học tập rất đơn giản:

    00 → (+A, +A)
    01 → (+A, -A)
    10 → (-A, +A)
    11 → (-A, -A)

Đây chưa phải toàn bộ NR PHY.

Nó chỉ dạy một transformation:

    2 bits
       ↓
    1 complex symbol


### MINI CODE 0F — Toy QPSK mapper


    #include <stdint.h>
    #include <stdio.h>

    struct cpx16 {
        int16_t i;
        int16_t q;
    };

    static struct cpx16 toy_qpsk_map(uint8_t b0, uint8_t b1)
    {
        const int16_t a = 10000;

        struct cpx16 out;

        out.i = b0 ? (int16_t)-a : a;
        out.q = b1 ? (int16_t)-a : a;

        return out;
    }

    int main(void)
    {
        struct cpx16 x = toy_qpsk_map(1, 0);

        printf("symbol = (%d,%d)\n",
               (int)x.i,
               (int)x.q);

        return 0;
    }

Expected:

    symbol = (-10000,10000)

<!-- SYNTAX_LENS_0F -->
#### Syntax ngay dưới MINI CODE 0F — return struct by-value, `const` local và `?:`

> **BEGINNER GATE 0F:** map đủ bốn cặp bit `00,01,10,11`, in bốn symbol và kiểm
> chỉ có hai mức biên độ `-10000/+10000`. Bài out-parameter ở cuối Lens là tầng
> tăng cường; Gate cơ bản không đòi pointer trước MINI CODE 0G.

```c
static struct cpx16 toy_qpsk_map(uint8_t b0, uint8_t b1)
```

trả cả `struct cpx16` by-value. Caller nhận một object kết quả, không phải pointer
tới local variable đã chết. `const int16_t a = 10000;` nói object local `a` không
được gán lại sau initialization.

```c
out.i = b0 ? (int16_t)-a : a;
```

conditional operator `condition ? when_true : when_false` chọn amplitude theo
bit. Nó là expression có value, nên dùng được bên phải assignment. Input phải
được validate/mask về `0/1`; type `uint8_t` một mình vẫn cho phép `2..255`.

**Tự code ngay:** reject `b0>1 || b1>1`, rồi viết mapper trả status qua
out-parameter để failure không tạo symbol giả hợp lệ.

<details>
<summary>Đáp án syntax 0F — mapper có validation</summary>

```c
static int toy_qpsk_map_checked(uint8_t b0, uint8_t b1,
                                struct cpx16 *out)
{
    const int16_t a = INT16_C(10000);
    if ((out == NULL) || (b0 > 1U) || (b1 > 1U)) return -1;
    out->i = b0 ? (int16_t)-a : a;
    out->q = b1 ? (int16_t)-a : a;
    return 0;
}
```

Full file code đủ bốn symbol và failure không sửa output:

```c
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

struct cpx16 {
    int16_t i;
    int16_t q;
};

static int toy_qpsk_map_checked(uint8_t b0, uint8_t b1,
                                struct cpx16 *out)
{
    const int16_t amplitude = INT16_C(10000);
    struct cpx16 candidate;

    if ((out == NULL) || (b0 > 1U) || (b1 > 1U)) return -1;
    candidate.i = b0 ? (int16_t)-amplitude : amplitude;
    candidate.q = b1 ? (int16_t)-amplitude : amplitude;
    *out = candidate;
    return 0;
}

int main(void)
{
    for (uint8_t b0 = 0U; b0 <= 1U; ++b0) {
        for (uint8_t b1 = 0U; b1 <= 1U; ++b1) {
            struct cpx16 symbol = {0, 0};
            assert(toy_qpsk_map_checked(b0, b1, &symbol) == 0);
            printf("%u%u -> (%d,%d)\n", (unsigned)b0, (unsigned)b1,
                   (int)symbol.i, (int)symbol.q);
        }
    }

    struct cpx16 unchanged = {7, 9};
    assert(toy_qpsk_map_checked(2U, 0U, &unchanged) == -1);
    assert((unchanged.i == 7) && (unchanged.q == 9));
    return 0;
}
```

```bash
gcc -std=c17 -Wall -Wextra -Werror -Wformat=2 -pedantic \
    practice/beginner_0f.c -o build/beginner_0f
./build/beginner_0f
```

</details>


### Đây là modulator chưa?


Đây là một phần rất nhỏ của digital modulation.

Ta vừa đổi representation:

    logic domain
        bits

thành:

    signal domain
        complex symbol

Nhưng complex symbol vẫn chưa phải waveform OFDM hoàn chỉnh.

Còn nhiều bước:

    symbols
       ↓
    resource grid
       ↓
    IFFT
       ↓
    cyclic prefix
       ↓
    IQ samples theo thời gian


### Câu phải tự nói được


Bài code vừa rồi không "gửi bit qua radio".

Nó chỉ map bit thành điểm constellation.


## 0.7. Receiver phải làm chiều ngược, nhưng có noise


Nếu TX map:

    10 → (-A,+A)

receiver lý tưởng có thể nhìn:

    I < 0
    Q >= 0

và đoán lại:

    10

Nhưng channel có thể biến:

    (-10000, +10000)

thành:

    (-9230, +11010)

hoặc:

    (-7000, +6100)

Điểm không còn hoàn hảo.

Receiver phải quyết định:

    điểm nhận gần vùng nào nhất?


### MINI CODE 0G — Toy hard demapper


    #include <stdint.h>
    #include <stdio.h>

    struct cpx16 {
        int16_t i;
        int16_t q;
    };

    static void toy_qpsk_demap(struct cpx16 x,
                               uint8_t *b0,
                               uint8_t *b1)
    {
        *b0 = x.i < 0 ? 1U : 0U;
        *b1 = x.q < 0 ? 1U : 0U;
    }

    int main(void)
    {
        struct cpx16 received = {-9230, 11010};
        uint8_t b0 = 0;
        uint8_t b1 = 0;

        toy_qpsk_demap(received, &b0, &b1);

        printf("%u%u\n",
               (unsigned)b0,
               (unsigned)b1);

        return 0;
    }

Expected:

    10

<!-- SYNTAX_LENS_0G -->
#### Syntax ngay dưới MINI CODE 0G — `void`, out-parameter, `&` và `*`

> **BEGINNER GATE 0G:** trên giấy vẽ `b0 → &b0 → uint8_t *b0 → *b0`; sau đó
> đổi dấu `received.i` và quan sát chỉ bit thứ nhất đổi. Đây là bài pointer đầu
> tiên, chưa cần tối ưu hay viết API generic.

```c
static void toy_qpsk_demap(struct cpx16 x,
                           uint8_t *b0,
                           uint8_t *b1)
```

`void` nói function không return value trực tiếp. Hai pointer `b0/b1` là nơi
caller cho phép function ghi output:

```c
toy_qpsk_demap(received, &b0, &b1);
```

- `&b0`: lấy địa chỉ object `b0`.
- parameter `uint8_t *b0`: nhận địa chỉ đó.
- `*b0 = ...`: dereference pointer để ghi vào object caller.

Pointer có thể NULL; toy code cố ý ngắn nhưng API production phải check trước
dereference. Cách tốt hơn là return `int` status và chỉ commit output sau khi mọi
input hợp lệ.

**Tự code ngay:** gọi với `NULL` trong test; implementation phải reject thay vì
crash. Sau đó bảo đảm nếu một out-pointer NULL thì out-pointer còn lại không bị
ghi nửa chừng.

<details>
<summary>Đáp án syntax 0G — out-parameter transactional</summary>

```c
static int toy_qpsk_demap_checked(struct cpx16 x,
                                  uint8_t *b0, uint8_t *b1)
{
    uint8_t first, second;
    if ((b0 == NULL) || (b1 == NULL)) return -1;
    first = x.i < 0 ? 1U : 0U;
    second = x.q < 0 ? 1U : 0U;
    *b0 = first;
    *b1 = second;
    return 0;
}
```

Full file để nhìn object caller trước/sau lời gọi:

```c
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

struct cpx16 {
    int16_t i;
    int16_t q;
};

static int toy_qpsk_demap_checked(struct cpx16 x,
                                  uint8_t *b0, uint8_t *b1)
{
    uint8_t first;
    uint8_t second;

    if ((b0 == NULL) || (b1 == NULL)) return -1;
    first = x.i < 0 ? 1U : 0U;
    second = x.q < 0 ? 1U : 0U;
    *b0 = first;
    *b1 = second;
    return 0;
}

int main(void)
{
    uint8_t b0 = 9U;
    uint8_t b1 = 9U;

    assert(toy_qpsk_demap_checked((struct cpx16){-100, 100},
                                   &b0, &b1) == 0);
    assert((b0 == 1U) && (b1 == 0U));
    printf("bits=%u%u\n", (unsigned)b0, (unsigned)b1);

    b1 = 7U;
    assert(toy_qpsk_demap_checked((struct cpx16){100, 100},
                                   NULL, &b1) == -1);
    assert(b1 == 7U);
    return 0;
}
```

```bash
gcc -std=c17 -Wall -Wextra -Werror -Wformat=2 -pedantic \
    practice/beginner_0g.c -o build/beginner_0g
./build/beginner_0g
```

</details>


### Concept cực quan trọng


TX và RX thường tạo thành cặp:

    map
    demap

    encode
    decode

    add CP
    remove CP

    IFFT
    FFT

    serialize
    parse

Một cách test rất mạnh là:

    RX(TX(x)) == x

trong điều kiện lý tưởng.

Sau này ta gọi nhiều test kiểu này là:

    round-trip test


## 0.8. Channel là thứ phá demo hoàn hảo của ta


Nếu TX và RX nối trực tiếp:

    TX output → RX input

thì mọi thứ quá dễ.

Radio thật có:

    noise
    delay
    multipath
    frequency offset
    phase offset
    fading

Khóa này dùng virtual RF để mô phỏng các impairment đó bằng IQ trong tiến trình.


### Một channel toy


Tưởng tượng TX có:

    x0 x1 x2 x3

delay 2 sample tạo:

    0 0 x0 x1 x2 x3

Noise có thể biến:

    I' = I + noise_i
    Q' = Q + noise_q

CFO làm mỗi sample bị xoay phase thêm một chút theo thời gian.


### MINI CODE 0H — Noise toy deterministic


Ta chưa dùng random.

Chỉ cộng một lỗi cố định để nhìn representation thay đổi:

    #include <stdint.h>
    #include <stdio.h>

    struct cpx16 {
        int16_t i;
        int16_t q;
    };

    static struct cpx16 add_toy_error(struct cpx16 x)
    {
        x.i = (int16_t)(x.i + 300);
        x.q = (int16_t)(x.q - 200);
        return x;
    }

    int main(void)
    {
        struct cpx16 tx = {10000, 10000};
        struct cpx16 rx = add_toy_error(tx);

        printf("TX=(%d,%d)\n", (int)tx.i, (int)tx.q);
        printf("RX=(%d,%d)\n", (int)rx.i, (int)rx.q);

        return 0;
    }

Expected:

    TX=(10000,10000)
    RX=(10300,9800)

<!-- SYNTAX_LENS_0H -->
#### Syntax ngay dưới MINI CODE 0H — parameter by-value và narrowing cast

> **BEGINNER GATE 0H:** chạy một case an toàn `100+300` và một case biên
> `32760+300`; in giá trị wide trước khi thu hẹp. Mục tiêu là nhìn thấy vì sao
> cast không đồng nghĩa với clamp.

`add_toy_error(struct cpx16 x)` nhận **bản sao** của sample. Assignment sửa local
copy `x`; object `tx` của caller không đổi. Return by-value tạo object `rx` mới.

```c
x.i = (int16_t)(x.i + 300);
```

Trong expression, `x.i` được integer-promote rồi cộng như `int`; cast cuối thu
hẹp về `int16_t`. Toy input còn trong range, nhưng cast không phải saturation.
Nếu tổng vượt `INT16_MAX`, numeric meaning có thể wrap/implementation-defined.

**Tự code ngay:** thay phép thu hẹp mù bằng `sat16`, test `x.i=32760` cộng `300`.

<details>
<summary>Đáp án syntax 0H — widen rồi saturation</summary>

```c
#include <limits.h>
#include <stdint.h>

static int16_t sat16(int32_t value)
{
    if (value > INT16_MAX) return INT16_MAX;
    if (value < INT16_MIN) return INT16_MIN;
    return (int16_t)value;
}

x.i = sat16((int32_t)x.i + INT32_C(300));
x.q = sat16((int32_t)x.q - INT32_C(200));
```

Bản trên là phần thay vào MINI CODE. Beginner không phải tự đoán nó nằm trong
function nào; đây là full file chạy độc lập:

```c
#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

struct cpx16 {
    int16_t i;
    int16_t q;
};

static int16_t sat16(int32_t value)
{
    if (value > INT16_MAX) return INT16_MAX;
    if (value < INT16_MIN) return INT16_MIN;
    return (int16_t)value;
}

static struct cpx16 add_toy_error(struct cpx16 x,
                                  int32_t delta_i, int32_t delta_q)
{
    int32_t wide_i = (int32_t)x.i + delta_i;
    int32_t wide_q = (int32_t)x.q + delta_q;

    printf("wide=(%" PRId32 ",%" PRId32 ")\n", wide_i, wide_q);
    x.i = sat16(wide_i);
    x.q = sat16(wide_q);
    return x;
}

int main(void)
{
    struct cpx16 safe = add_toy_error((struct cpx16){100, 0}, 300, -200);
    struct cpx16 edge = add_toy_error((struct cpx16){32760, -32760}, 300, -300);

    assert((safe.i == 400) && (safe.q == -200));
    assert((edge.i == INT16_MAX) && (edge.q == INT16_MIN));
    printf("edge=(%" PRId16 ",%" PRId16 ")\n", edge.i, edge.q);
    return 0;
}
```

```bash
gcc -std=c17 -Wall -Wextra -Werror -Wformat=2 -pedantic \
    practice/beginner_0h.c -o build/beginner_0h
./build/beginner_0h
```

Expected biên: `32760 + 300` clamp thành `32767`, không đổi dấu.

</details>


### Ý nghĩa


TX symbol và RX symbol không nhất thiết giống hệt nhau về numeric value.

Nhiệm vụ receiver không phải lúc nào cũng là:

    received == transmitted ?

Mà thường là:

    từ received noisy signal
    suy ra transmitted information hợp lý nhất

Đây là gốc của:

    detection
    equalization
    decoding


## 0.9. Sampling: waveform cũng có trục thời gian


Một array:

    samples[0]
    samples[1]
    samples[2]

chỉ có ý nghĩa vật lý khi ta biết tốc độ lấy mẫu.

Ví dụ:

    Fs = 7,680,000 samples/s

thì một sample đại diện một bước thời gian:

    Ts = 1 / Fs

Firmware không nhất thiết lưu thời gian floating-point cho từng sample.

Thường một block có metadata:

    timestamp đầu block
    sample rate
    số lượng sample
    pointer tới sample


### MINI CODE 0I — IQ block


    #include <stdint.h>
    #include <stdio.h>

    struct cpx16 {
        int16_t i;
        int16_t q;
    };

    struct iq_block {
        uint64_t t0;
        uint32_t sample_rate_hz;
        uint32_t count;
        struct cpx16 *samples;
    };

    int main(void)
    {
        struct cpx16 storage[4] = {
            {100, 0},
            {200, 0},
            {300, 0},
            {400, 0}
        };

        struct iq_block block = {
            .t0 = 5000,
            .sample_rate_hz = 7680000U,
            .count = 4U,
            .samples = storage
        };

        printf("t0=%llu rate=%u count=%u first_I=%d\n",
               (unsigned long long)block.t0,
               (unsigned)block.sample_rate_hz,
               (unsigned)block.count,
               (int)block.samples[0].i);

        return 0;
    }

<!-- SYNTAX_LENS_0I -->
#### Syntax ngay dưới MINI CODE 0I — designated initializer, pointer field và integer format portable

> **BEGINNER GATE 0I:** chỉ cần tự chỉ đúng bốn field `WHEN/RATE/HOW MANY/WHERE`
> và in chúng không warning. `PRIu64` là cách compiler giữ type contract cho bạn,
> không phải công thức cần học thuộc.

Initializer:

```c
struct iq_block block = {
    .t0 = 5000,
    .sample_rate_hz = 7680000U,
    .count = 4U,
    .samples = storage
};
```

dùng `.field = value`, nên người đọc thấy meaning của từng giá trị và việc đổi
thứ tự field trong initializer không đổi field được gán. `storage` là array;
trong biểu thức gán này nó decay thành pointer tới phần tử đầu, đúng type
`struct cpx16 *`.

Ba width ở đây có ba contract:

```text
uint64_t t0          → timestamp/counter dài, fixed 64 bit
uint32_t sample_rate → wire/config field fixed 32 bit
uint32_t count       → profile block count fixed 32 bit
```

Dòng gốc dùng `%llu` sau cast `unsigned long long` để người mới chạy được trên
nhiều compiler. Cách portable theo chính fixed-width type là:

```c
#include <inttypes.h>

printf("t0=%" PRIu64 " rate=%" PRIu32 " count=%" PRIu32 " first_I=%" PRId16 "\n",
       block.t0,
       block.sample_rate_hz,
       block.count,
       block.samples[0].i);
```

Các string literal đứng cạnh nhau được C ghép lúc compile:

```c
"t0=%" PRIu64 " rate=%" PRIu32
```

không phải phép cộng string. Macro `PRIu64`/`PRIu32` từ `<inttypes.h>` chọn
specifier đúng với typedef thực trên ABI hiện tại. Không tự đoán `uint64_t` luôn
là `unsigned long long`: Linux LP64 và Windows LLP64 có thể alias khác nhau.

**Tự code ngay:** in `UINT64_MAX`, `UINT32_MAX`, `INT16_MIN` bằng macro `PRI*`;
build với `-Wformat=2 -Werror`.

<details>
<summary>Đáp án syntax 0I — format matrix nhỏ</summary>

```c
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

printf("u64=%" PRIu64 " u32=%" PRIu32 " i16=%" PRId16 "\n",
       UINT64_MAX, UINT32_MAX, INT16_MIN);
```

Đoạn trên chỉ là statement minh họa. Full translation unit để beginner gõ và
compile mà không phải tự bọc vào `main`:

```c
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    uint64_t tick = UINT64_MAX;
    uint32_t count = UINT32_MAX;
    int16_t llr = INT16_MIN;

    printf("tick=%" PRIu64 " count=%" PRIu32 " llr=%" PRId16 "\n",
           tick, count, llr);
    printf("tick_hex=0x%016" PRIx64 " count_hex=0x%08" PRIx32 "\n",
           tick, count);
    return 0;
}
```

```bash
gcc -std=c17 -Wall -Wextra -Werror -Wformat=2 -pedantic \
    practice/beginner_0i.c -o build/beginner_0i
./build/beginner_0i
```

Không cast để “làm warning im”. Chọn đúng macro theo type mới là sửa contract.

</details>


### Tại sao struct này quan trọng?


Nó gom:

    WHEN   → t0
    RATE   → sample_rate_hz
    HOW MANY → count
    WHERE  → samples

Sau này virtual RF, DMA và PHY không chỉ chuyền "một pointer".

Chúng phải biết contract của cả block.


## 0.10. OFDM: symbol không được ném thẳng ra antenna


QPSK/QAM tạo complex symbols.

OFDM tổ chức nhiều symbol trên các subcarrier.

Mental model:

    frequency bins
        │
        │ đặt QAM symbols vào các bin
        ▼
       IFFT
        ▼
    time-domain IQ
        ▼
    add cyclic prefix
        ▼
      channel

Receiver:

      channel
        ▼
    remove CP
        ▼
       FFT
        ▼
    frequency bins

Điều quan trọng:

    QPSK/QAM symbol

và:

    IQ sample theo thời gian

đều là số phức, nhưng chúng không nhất thiết đại diện cùng một stage.


### MINI CODE 0J — Resource bins trước IFFT


Ta chưa code FFT.

Chỉ nhìn cách một frequency-domain symbol có thể được lưu:

    #include <stdint.h>
    #include <stdio.h>

    struct cpx16 {
        int16_t i;
        int16_t q;
    };

    int main(void)
    {
        struct cpx16 bins[8] = {0};

        bins[1].i = 10000;
        bins[1].q = 10000;

        bins[2].i = -10000;
        bins[2].q = 10000;

        for (size_t k = 0; k < 8; ++k) {
            printf("bin[%zu] = (%d,%d)\n",
                   k,
                   (int)bins[k].i,
                   (int)bins[k].q);
        }

        return 0;
    }

<!-- SYNTAX_LENS_0J -->
#### Syntax ngay dưới MINI CODE 0J — zero-initialization và nhắc lại `%zu`

> **BEGINNER GATE 0J:** zero tám bin, chỉ đặt bin vật lý `0` và `7`, rồi in cả
> tám dòng. Chỉ sau khi pass mới thêm mapping logical bin âm.

```c
struct cpx16 bins[8] = {0};
```

khởi tạo toàn bộ field của toàn bộ phần tử về zero. Đây là initialization lúc
tạo object, không phải phép xóa buffer tùy thời điểm và không thay thế ownership
protocol của DMA.

Trong loop, `k` là `size_t`, nên ngay tại dòng in phải giữ:

```c
printf("bin[%zu]", k);
```

Không đổi thành `%u` chỉ vì `k` hiện nhỏ hơn 8. Format khớp **type**, không khớp
giá trị tình cờ nhỏ. `bins[k].i` đọc theo chuỗi:

```text
bins      array
bins[k]   object thứ k
.i        field i của object đó
```

**Tự code ngay:** zero toàn grid, đặt bin `0`, `+1`, `-1` qua helper logical-bin
rồi in cả logical bin và storage index bằng `%td`/`%zu` tương ứng.

Ba nấc làm quen:

1. Chỉ set storage index `0` và `7`, in bằng `%zu`.
2. Viết mapping logical `-1→7`, `0→0`, `+1→1` và in logical bin bằng `%td`.
3. Reject logical bin ngoài `[-4,3]`; failure không được ghi output index.

<details>
<summary>Đáp án syntax 0J — full file logical bin tám phần tử</summary>

Lưu thành `practice/beginner_0j.c`:

```c
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

struct cpx16 {
    int16_t i;
    int16_t q;
};

static int logical_bin_to_index(ptrdiff_t bin, size_t *index)
{
    size_t candidate;

    if ((index == NULL) || (bin < -4) || (bin > 3)) return -1;
    candidate = bin < 0 ? (size_t)(bin + 8) : (size_t)bin;
    *index = candidate;
    return 0;
}

static void set_bin(struct cpx16 bins[8], ptrdiff_t bin, int16_t i)
{
    size_t index = 0U;
    int status = logical_bin_to_index(bin, &index);

    assert(status == 0);
    bins[index].i = i;
    printf("logical=%td storage=%zu I=%d\n", bin, index, (int)i);
}

int main(void)
{
    struct cpx16 bins[8] = {0};
    size_t unchanged = 99U;

    set_bin(bins, 0, 100);
    set_bin(bins, 1, 200);
    set_bin(bins, -1, 300);

    assert(bins[0].i == 100);
    assert(bins[1].i == 200);
    assert(bins[7].i == 300);
    assert(logical_bin_to_index(4, &unchanged) == -1);
    assert(unchanged == 99U);
    return 0;
}
```

```bash
gcc -std=c17 -Wall -Wextra -Werror -pedantic \
    practice/beginner_0j.c -o build/beginner_0j
./build/beginner_0j
```

`ptrdiff_t` mô tả logical index có dấu nên in bằng `%td`; storage index có type
`size_t` nên in bằng `%zu`.

</details>


### Code này chưa làm gì?


Chưa tạo OFDM waveform.

Nó chỉ tạo:

    frequency-domain grid/bins

IFFT ở phần sau mới biến bins thành time samples.

Đây là lý do không được thấy cùng type:

    struct cpx16

rồi kết luận mọi cpx16 đều là cùng một loại dữ liệu về mặt semantic.

Type giống nhau.

Stage khác nhau.

Representation meaning khác nhau.


## 0.11. Synchronization: RX không biết bắt đầu ở đâu


Một lỗi mental model thường gặp:

    "TX phát frame, vậy RX biết sample đầu frame chứ?"

Không.

Receiver thường chỉ thấy stream:

    ... x x x x x x x x x x x ...

Nó phải tự tìm:

    signal có tồn tại không?
    frame/symbol bắt đầu đâu?
    reference sequence ở vị trí nào?
    carrier lệch bao nhiêu?

Một kỹ thuật nền:

    correlation

Ta có một pattern biết trước:

    reference

và trượt nó qua input.


### MINI CODE 0K — Correlation 1D toy


Chưa dùng complex để tránh quá tải.

    #include <stddef.h>
    #include <stdint.h>
    #include <stdio.h>

    static int32_t dot4(const int16_t *x,
                        const int16_t reference[4])
    {
        int32_t sum = 0;

        for (size_t i = 0; i < 4; ++i) {
            sum += (int32_t)x[i] * (int32_t)reference[i];
        }

        return sum;
    }

    int main(void)
    {
        int16_t input[8] = {
            0, 0,
            1, -1, 1, -1,
            0, 0
        };

        int16_t reference[4] = {
            1, -1, 1, -1
        };

        int32_t best_score = -2147483647;
        size_t best_offset = 0;

        for (size_t offset = 0; offset <= 4; ++offset) {
            int32_t score = dot4(&input[offset], reference);

            printf("offset=%zu score=%ld\n",
                   offset,
                   (long)score);

            if (score > best_score) {
                best_score = score;
                best_offset = offset;
            }
        }

        printf("best_offset=%zu\n", best_offset);
        return 0;
    }

<!-- SYNTAX_LENS_0K -->
#### Syntax ngay dưới MINI CODE 0K — function parameter, `const`, pointer arithmetic và `PRId32`

> **BEGINNER GATE 0K:** tính tay score tại offset `0`, `1`, `2`; chạy code và
> khoanh offset có score lớn nhất. Chưa cần chứng minh bound `int64_t` trước khi
> hiểu dot product nhỏ này.

Prototype:

```c
static int32_t dot4(const int16_t *x,
                    const int16_t reference[4]);
```

được đọc ngay tại chỗ:

- `static`: function chỉ visible trong translation unit này.
- return `int32_t`: correlation score có dấu, rộng hơn sample.
- `const int16_t *x`: function đọc sample qua pointer nhưng không sửa qua pointer đó.
- `reference[4]` trong parameter vẫn được điều chỉnh gần như pointer; số `4`
  giúp diễn đạt intent nhưng không tự mang runtime length.

Lời gọi:

```c
dot4(&input[offset], reference)
```

dùng `&` lấy địa chỉ phần tử tại offset. Chỉ hợp lệ vì loop bảo đảm còn đủ bốn
sample: `offset <= 4` với input tám phần tử.

Dòng gốc cast score sang `long` rồi dùng `%ld`. Với fixed-width type, cách không
phụ thuộc `long` 32 hay 64 bit là:

```c
#include <inttypes.h>
printf("offset=%zu score=%" PRId32 "\n", offset, score);
```

`offset` là `size_t`→`%zu`; `score` là `int32_t`→`PRId32`. Mỗi argument có format
riêng, không có một specifier “số nguyên chung chung”.

**Tự code ngay:** đổi accumulator thành `int64_t`, thêm `PRId64`, và tính bound
để giải thích khi nào `int32_t` không còn đủ.

<details>
<summary>Đáp án syntax 0K — accumulator rộng</summary>

```c
#include <inttypes.h>

int64_t score = 0;
/* score += (int64_t)x[i] * reference[i]; */
printf("offset=%zu score=%" PRId64 "\n", offset, score);
```

Snippet trên chỉ chỉ ra ba dòng phải đổi. Đây là full file để không còn biến
`offset` hoặc array ẩn ngoài code block:

```c
#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

static int64_t dot4_wide(const int16_t *x, const int16_t reference[4])
{
    int64_t sum = 0;

    for (size_t i = 0U; i < 4U; ++i) {
        sum += (int64_t)x[i] * (int64_t)reference[i];
    }
    return sum;
}

int main(void)
{
    const int16_t input[8] = {0, 0, 1, -1, 1, -1, 0, 0};
    const int16_t reference[4] = {1, -1, 1, -1};
    int64_t best_score = INT64_MIN;
    size_t best_offset = 0U;

    for (size_t offset = 0U; offset <= 4U; ++offset) {
        int64_t score = dot4_wide(&input[offset], reference);
        printf("offset=%zu score=%" PRId64 "\n", offset, score);
        if (score > best_score) {
            best_score = score;
            best_offset = offset;
        }
    }

    assert(best_offset == 2U);
    assert(best_score == INT64_C(4));
    return 0;
}
```

```bash
gcc -std=c17 -Wall -Wextra -Werror -Wformat=2 -pedantic \
    practice/beginner_0k.c -o build/beginner_0k
./build/beginner_0k
```

Cast phải xảy ra **trước phép nhân**; cast kết quả đã overflow không cứu được dữ liệu.

</details>


### Đây là phiên bản đồ chơi của cái gì?


Concept:

    known sequence
         +
    received stream
         ↓
    similarity theo offset
         ↓
    peak
         ↓
    timing estimate

Sau này PSS/cell search dùng cùng loại tư duy, nhưng signal, sequence và xử lý phức tạp hơn.


## 0.12. CRC: nhận được bit chưa có nghĩa là tin nó


Channel có thể làm sai bit.

Decoder có thể khôi phục được một candidate block.

Nhưng layer trên cần biết:

    block này có đáng tin không?

CRC là một error-detection mechanism.

Mental model:

    payload
      ↓
    append CRC
      ↓
    transmit
      ↓
    receive/decode
      ↓
    recompute CRC
      ↓
      | matches? |
       /        \
     yes         no
      |           |
    accept      drop/retry


### MINI CODE 0L — Chỉ học API contract, chưa cướp Bài CRC


Ở đây ta chưa implement CRC-24A.

Chỉ nhìn cách code modem có thể gọi nó:

    uint32_t crc24a(const uint8_t *data, size_t len);

    int verify_block(const uint8_t *data,
                     size_t len,
                     uint32_t expected_crc)
    {
        uint32_t actual_crc = crc24a(data, len);

        return actual_crc == expected_crc;
    }

<!-- SYNTAX_LENS_0L -->
#### Syntax ngay dưới MINI CODE 0L — đọc prototype như contract

> **BEGINNER GATE 0L:** chưa cài CRC polynomial. Hãy viết một checker nhận
> `actual_crc`, `expected_crc`, trả status và chỉ ghi `matches` khi input hợp lệ;
> mục tiêu là học API contract, không giả vờ đã viết CRC24A.

```c
uint32_t crc24a(const uint8_t *data, size_t len);
```

Đọc từng vai trò ngay tại đây:

```text
uint32_t             return CRC fixed 32-bit container
const uint8_t *data  borrowed input bytes, function không sửa qua pointer
size_t len           số byte hợp lệ kể từ data
;                    chỉ declaration/prototype, chưa có body implementation
```

Pointer không tự chứa length. Cặp `(data,len)` mới là slice contract. Nếu
`data==NULL`, chỉ `len==0` mới có thể hợp lệ khi API công bố như vậy.

```c
return actual_crc == expected_crc;
```

operator `==` tạo kết quả logic `0` hoặc `1`, rồi function return qua `int`.
Production API thường dùng `bool` cho predicate hoặc enum/status rõ hơn để phân
biệt “CRC mismatch” với “input invalid”.

**Tự code ngay:** đổi thành API trả status và out-parameter `bool *matches`;
failure không được ghi `matches`.

Ba nấc làm quen:

1. Viết predicate đơn giản `actual_crc == expected_crc`.
2. Thêm `bool *matches` và reject `NULL`.
3. Gọi failure khi `matches` đang là `true`; assert nó vẫn là `true` để chứng minh
   failure không ghi output.

<details>
<summary>Đáp án syntax 0L — full file chỉ luyện CRC check contract</summary>

Lưu thành `practice/beginner_0l.c`:

```c
#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static int crc_check(uint32_t actual_crc, uint32_t expected_crc,
                     bool *matches)
{
    bool candidate;

    if (matches == NULL) return -1;
    candidate = actual_crc == expected_crc;
    *matches = candidate;
    return 0;
}

int main(void)
{
    bool matches = false;

    assert(crc_check(UINT32_C(0x00ABCDEF),
                     UINT32_C(0x00ABCDEF), &matches) == 0);
    assert(matches);
    printf("actual=0x%08" PRIx32 " expected=0x%08" PRIx32
           " matches=%d\n",
           UINT32_C(0x00ABCDEF), UINT32_C(0x00ABCDEF), (int)matches);

    matches = true;
    assert(crc_check(1U, 2U, NULL) == -1);
    assert(matches);
    return 0;
}
```

```bash
gcc -std=c17 -Wall -Wextra -Werror -pedantic \
    practice/beginner_0l.c -o build/beginner_0l
./build/beginner_0l
```

File này **không phải CRC24A implementation**. Nó cô lập đúng bài học hiện tại:
status, predicate output và failure atomicity của API checker.

</details>


### Tại sao không cho luôn implementation?


Vì mục đích PHẦN 0 là hiểu:

    CRC đứng ở đâu
    input là gì
    output là gì
    PASS/FAIL dùng để làm gì

Bài CRC sau mới bắt bạn quan tâm:

    polynomial
    bit order
    initial value
    coverage
    test vector


### Ý nghĩa modem


CRC FAIL có thể dẫn đến:

    drop
    HARQ retransmission
    counter tăng
    trace event

Nó không nên dẫn đến:

    "cứ đẩy block hỏng lên application"


## 0.13. PHY gửi được bit vẫn chưa tạo ra mạng di động


Giả sử PHY có thể làm:

    bits ↔ waveform

Vẫn chưa đủ.

Ta còn các layer như:

    MAC
    RLC
    PDCP
    RRC
    NAS

Một mental model rất thô:

    NAS   → quan hệ UE với core network
    RRC   → trạng thái radio/control
    PDCP  → COUNT/security/reordering
    RLC   → segmentation/reassembly/reordering
    MAC   → multiplex/scheduling/HARQ
    PHY   → bit ↔ radio waveform

Đừng cố học hết chi tiết từng layer ở đây.

Chỉ cần hiểu:

    PHY trả lời:
        "transport block radio này có decode được không?"

Nó không tự trả lời mọi câu như:

    đây là bearer nào?
    packet duplicate không?
    packet đến sai thứ tự không?
    đang connected hay camped?
    đã registered với core chưa?
    PDU session đã active chưa?


### MINI CODE 0M — Một PDU header toy


Đây KHÔNG phải header chuẩn 3GPP.

Chỉ để thấy protocol layer thêm metadata quanh payload.

    #include <stdint.h>
    #include <stdio.h>

    struct toy_pdu {
        uint8_t bearer_id;
        uint16_t sequence_number;
        uint8_t payload[4];
    };

    int main(void)
    {
        struct toy_pdu pdu = {
            .bearer_id = 3,
            .sequence_number = 17,
            .payload = {0x41, 0x42, 0x43, 0x44}
        };

        printf("bearer=%u sn=%u first=0x%02x\n",
               (unsigned)pdu.bearer_id,
               (unsigned)pdu.sequence_number,
               (unsigned)pdu.payload[0]);

        return 0;
    }

<!-- SYNTAX_LENS_0M -->
#### Syntax ngay dưới MINI CODE 0M — nested array initializer và promotion khi in

> **BEGINNER GATE 0M:** encode số `0x1234` thành hai byte little-endian, rồi ghép
> lại và assert nhận đúng `0x1234`. Chỉ đổi từng byte sau khi đã dự đoán value.

```c
.payload = {0x41, 0x42, 0x43, 0x44}
```

khởi tạo fixed array nằm **bên trong** struct; bytes thuộc storage của object
`pdu`, không phải một pointer tới nơi khác. Khi in `uint8_t/uint16_t` bằng `%u`,
mini code cast từng value sang `unsigned` để format contract rõ.

Struct này là object nội bộ, không tự là wire packet. Padding, endian và enum/
bool layout không được serialize bằng:

```c
memcpy(wire, &pdu, sizeof pdu); /* sai contract wire */
```

Wire codec phải put/get từng fixed-width field và test exact bytes.

**Tự code ngay:** encode `bearer_id` một byte, `sequence_number` little-endian
hai byte và bốn payload byte thành đúng bảy byte; đặt đáp án byte ngay dưới test.

<details>
<summary>Đáp án syntax 0M — wire bytes canonical</summary>

```c
uint8_t wire[7];
wire[0] = pdu.bearer_id;
wire[1] = (uint8_t)pdu.sequence_number;
wire[2] = (uint8_t)(pdu.sequence_number >> 8U);
for (size_t k = 0U; k < 4U; ++k) wire[3U + k] = pdu.payload[k];
/* expected: 03 11 00 41 42 43 44 */
```

Full file với encode, byte-by-byte expected và round-trip nhỏ:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

struct tiny_pdu {
    uint8_t bearer_id;
    uint16_t sequence_number;
    uint8_t payload[4];
};

static void encode_tiny_pdu(const struct tiny_pdu *pdu, uint8_t wire[7])
{
    wire[0] = pdu->bearer_id;
    wire[1] = (uint8_t)pdu->sequence_number;
    wire[2] = (uint8_t)(pdu->sequence_number >> 8U);
    for (size_t k = 0U; k < 4U; ++k) wire[3U + k] = pdu->payload[k];
}

static uint16_t decode_u16_le(const uint8_t bytes[2])
{
    return (uint16_t)((uint16_t)bytes[0] |
                      ((uint16_t)bytes[1] << 8U));
}

int main(void)
{
    const struct tiny_pdu pdu = {
        .bearer_id = 3U,
        .sequence_number = UINT16_C(0x0011),
        .payload = {0x41U, 0x42U, 0x43U, 0x44U}
    };
    const uint8_t expected[7] = {3U, 0x11U, 0U, 0x41U, 0x42U, 0x43U, 0x44U};
    uint8_t wire[7] = {0};

    encode_tiny_pdu(&pdu, wire);
    for (size_t k = 0U; k < 7U; ++k) {
        printf("%02x%s", (unsigned)wire[k], k + 1U == 7U ? "\n" : " ");
        assert(wire[k] == expected[k]);
    }
    assert(decode_u16_le(&wire[1]) == pdu.sequence_number);
    return 0;
}
```

```bash
gcc -std=c17 -Wall -Wextra -Werror -Wformat=2 -pedantic \
    practice/beginner_0m.c -o build/beginner_0m
./build/beginner_0m
```

</details>


### Concept


Payload một mình chưa đủ.

Protocol thường thêm context:

    identity
    sequence
    length
    flags
    type
    integrity information

Sau này bạn sẽ học serialize/parse đúng wire format.

Đừng memcpy một struct toy như trên rồi coi đó là protocol thật.


## 0.14. RRC và NAS là hai state machine khác nhau


Một biến:

    connected = true;

quá nghèo để mô tả modem.

RRC có thể có state:

    OFF
      ↓
    SEARCHING
      ↓
    CAMPED
      ↓
    CONNECTING
      ↓
    CONNECTED

NAS lại có state khác:

    DEREGISTERED
        ↓
    REGISTERING
        ↓
    REGISTERED
        ↓
    SESSION_ACTIVE

Có thể:

    RRC = CONNECTED

nhưng:

    NAS = REGISTERING

Tức là radio link đã connected nhưng UE chưa có data session hoàn chỉnh.


### MINI CODE 0N — State machine toy


    #include <stdio.h>

    enum rrc_state {
        RRC_OFF,
        RRC_SEARCHING,
        RRC_CAMPED,
        RRC_CONNECTED
    };

    enum rrc_event {
        EV_POWER_ON,
        EV_CELL_FOUND,
        EV_SETUP_OK
    };

    static enum rrc_state rrc_step(enum rrc_state state,
                                   enum rrc_event event)
    {
        switch (state) {
        case RRC_OFF:
            if (event == EV_POWER_ON) {
                return RRC_SEARCHING;
            }
            break;

        case RRC_SEARCHING:
            if (event == EV_CELL_FOUND) {
                return RRC_CAMPED;
            }
            break;

        case RRC_CAMPED:
            if (event == EV_SETUP_OK) {
                return RRC_CONNECTED;
            }
            break;

        case RRC_CONNECTED:
            break;
        }

        return state;
    }

    int main(void)
    {
        enum rrc_state state = RRC_OFF;

        state = rrc_step(state, EV_POWER_ON);
        state = rrc_step(state, EV_CELL_FOUND);
        state = rrc_step(state, EV_SETUP_OK);

        printf("state=%d\n", (int)state);
        return 0;
    }

<!-- SYNTAX_LENS_0N -->
#### Syntax ngay dưới MINI CODE 0N — `enum`, `switch`, `case`, `break`

> **BEGINNER GATE 0N:** viết bảng ba cột `state/event/next state` trước code;
> chạy đúng một transition hợp lệ và một wrong-state event phải giữ state cũ.

`enum rrc_state` và `enum rrc_event` là hai type logic khác vai trò dù compiler
có thể biểu diễn cả hai bằng integer. Đừng truyền event vào chỗ state chỉ vì cast
làm compiler im.

Function `rrc_step(state,event)` nhận cả hai by-value. Nếu transition không khớp,
dòng cuối `return state;` trả state cũ, nên wrong-state event là no-op.

`switch (state)` chọn case theo current state:

```text
case LABEL:  điểm vào khi value khớp
break;       thoát switch, tránh chạy xuyên case kế
return ...;  thoát cả function nên không cần break sau nó
```

In enum local bằng `(int)state` + `%d` chỉ để debug. Không serialize enum trực
tiếp; encode qua fixed-width integer sau range validation.

**Tự code ngay:** thêm `RRC_STATE_COUNT`, reject enum ngoài range và viết
`rrc_state_name` có `default: "UNKNOWN"` ngay dưới test.

Ba nấc làm quen:

1. Code một transition `IDLE + START → CONNECTING`.
2. Thêm `CONNECTED + RELEASE → IDLE`; wrong-state event giữ state cũ.
3. Thêm range validation và observer `rrc_state_name()`.

<details>
<summary>Đáp án syntax 0N — full file state/event có validation</summary>

Lưu thành `practice/beginner_0n.c`:

```c
#include <assert.h>
#include <stdio.h>

enum rrc_state {
    RRC_IDLE,
    RRC_CONNECTING,
    RRC_CONNECTED,
    RRC_STATE_COUNT
};

enum rrc_event {
    RRC_EV_START,
    RRC_EV_SETUP_OK,
    RRC_EV_RELEASE,
    RRC_EVENT_COUNT
};

static const char *rrc_state_name(enum rrc_state state)
{
    switch (state) {
    case RRC_IDLE:       return "IDLE";
    case RRC_CONNECTING: return "CONNECTING";
    case RRC_CONNECTED:  return "CONNECTED";
    case RRC_STATE_COUNT:
    default:             return "UNKNOWN";
    }
}

static int rrc_step(enum rrc_state state, enum rrc_event event,
                    enum rrc_state *next)
{
    enum rrc_state candidate;

    if ((next == NULL) || (state < RRC_IDLE) || (state >= RRC_STATE_COUNT) ||
        (event < RRC_EV_START) || (event >= RRC_EVENT_COUNT)) return -1;

    candidate = state;
    if ((state == RRC_IDLE) && (event == RRC_EV_START)) {
        candidate = RRC_CONNECTING;
    } else if ((state == RRC_CONNECTING) && (event == RRC_EV_SETUP_OK)) {
        candidate = RRC_CONNECTED;
    } else if ((state == RRC_CONNECTED) && (event == RRC_EV_RELEASE)) {
        candidate = RRC_IDLE;
    }
    *next = candidate;
    return 0;
}

int main(void)
{
    enum rrc_state next = RRC_STATE_COUNT;

    assert(rrc_step(RRC_IDLE, RRC_EV_START, &next) == 0);
    assert(next == RRC_CONNECTING);
    printf("next=%s\n", rrc_state_name(next));

    assert(rrc_step(RRC_IDLE, RRC_EV_RELEASE, &next) == 0);
    assert(next == RRC_IDLE);
    assert(rrc_step((enum rrc_state)99, RRC_EV_START, &next) == -1);
    assert(next == RRC_IDLE);
    return 0;
}
```

```bash
gcc -std=c17 -Wall -Wextra -Werror -pedantic \
    practice/beginner_0n.c -o build/beginner_0n
./build/beginner_0n
```

Expected line: `next=CONNECTING`. Hai assert sau chứng minh wrong-state event là
no-op còn enum ngoài range là invalid input; hai case không bị trộn làm một.

</details>


### Đang học gì?


Không phải RRC chuẩn hoàn chỉnh.

Ta đang học pattern:

    current state
        +
    event
        ↓
    guard / rule
        ↓
    next state

Một event không hợp lệ không được tùy tiện sửa context.

Sau này mỗi transition phải nghĩ:

    FROM
    EVENT
    GUARD
    ACTION
    TO


## 0.15. Modem còn là một hệ thống real-time firmware


Đây là phần người mới thường bỏ qua vì nghĩ:

    "modem = DSP"

Không đủ.

Giả sử RF hardware vừa nhận xong một block IQ.

Một flow có thể là:

    RF DMA complete
          ↓
         IRQ
          ↓
    ISR ACK interrupt
          ↓
    enqueue event
          ↓
      L1_RX task
          ↓
    sync / FFT / decode
          ↓
       MAC task

Nếu ISR làm FFT mất quá lâu:

    deadline miss

Nếu queue đầy:

    drop

Nếu buffer bị CPU sửa trong lúc DMA đang đọc:

    corruption

Nếu callback timer cũ chạy sau khi state đã reset:

    stale event bug

Vì vậy runtime không phải "phần phụ".

Nó là thứ làm DSP/protocol chạy đúng thời gian và đúng ownership.


### MINI CODE 0O — Event thay vì gọi mọi thứ lung tung


    #include <stdint.h>
    #include <stdio.h>

    enum event_type {
        EV_RF_RX_DONE,
        EV_TIMER,
        EV_IPC
    };

    struct event {
        uint64_t tick;
        enum event_type type;
    };

    static void handle_event(const struct event *ev)
    {
        switch (ev->type) {
        case EV_RF_RX_DONE:
            printf("tick=%llu: schedule RX work\n",
                   (unsigned long long)ev->tick);
            break;

        case EV_TIMER:
            printf("tick=%llu: process timer\n",
                   (unsigned long long)ev->tick);
            break;

        case EV_IPC:
            printf("tick=%llu: process IPC\n",
                   (unsigned long long)ev->tick);
            break;
        }
    }

    int main(void)
    {
        struct event ev = {
            .tick = 1250,
            .type = EV_RF_RX_DONE
        };

        handle_event(&ev);
        return 0;
    }

<!-- SYNTAX_LENS_0O -->
#### Syntax ngay dưới MINI CODE 0O — `enum`, `switch`, `->` và tick format

> **BEGINNER GATE 0O:** tạo ba event, mỗi event đi vào một `case`, rồi thêm một
> value ngoài enum để observer in `UNKNOWN`. Mục tiêu là thấy event mang data và
> handler chọn action.

```c
enum event_type {
    EV_RF_RX_DONE,
    EV_TIMER,
    EV_IPC
};
```

tạo tập tên integer cho state/event local. Không serialize raw `enum` lên wire
vì width/layout underlying thuộc compiler/ABI; wire dùng `uint*_t` rồi validate.

Trong:

```c
static void handle_event(const struct event *ev)
```

`ev` là pointer tới object read-only qua pointer đó. Vì là pointer, truy cập field
dùng `ev->type`, tương đương `(*ev).type`. `switch` chọn đúng một `case`; `break`
ngăn fall-through sang case tiếp theo.

Tick là `uint64_t`, vì thế bản portable đặt ngay cạnh code là:

```c
#include <inttypes.h>
printf("tick=%" PRIu64 ": schedule RX work\n", ev->tick);
```

Dòng `%llu` + cast trong mini code là cầu nhập môn; project strict dùng `PRIu64`
để cùng source đúng trên Linux host, Windows tool và Arm target.

**Tự code ngay:** thêm `default` reject enum ngoài range và function
`event_type_name()` trả string cho trace.

<details>
<summary>Đáp án syntax 0O — observer enum an toàn</summary>

```c
static const char *event_type_name(enum event_type type)
{
    switch (type) {
    case EV_RF_RX_DONE: return "RF_RX_DONE";
    case EV_TIMER:      return "TIMER";
    case EV_IPC:        return "IPC";
    default:            return "UNKNOWN";
    }
}
```

Full file để chạy đủ ba `case` và một unknown value:

```c
#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum event_type {
    EV_RF_RX_DONE,
    EV_TIMER,
    EV_IPC
};

struct event {
    uint64_t tick;
    enum event_type type;
};

static const char *event_type_name(enum event_type type)
{
    switch (type) {
    case EV_RF_RX_DONE: return "RF_RX_DONE";
    case EV_TIMER:      return "TIMER";
    case EV_IPC:        return "IPC";
    default:            return "UNKNOWN";
    }
}

static void handle_event(const struct event *event)
{
    printf("tick=%" PRIu64 " type=%s\n",
           event->tick, event_type_name(event->type));
}

int main(void)
{
    const struct event events[] = {
        {1U, EV_RF_RX_DONE}, {2U, EV_TIMER}, {3U, EV_IPC},
        {4U, (enum event_type)99}
    };

    for (size_t n = 0U; n < sizeof events / sizeof events[0]; ++n) {
        handle_event(&events[n]);
    }
    assert(strcmp(event_type_name((enum event_type)99), "UNKNOWN") == 0);
    return 0;
}
```

```bash
gcc -std=c17 -Wall -Wextra -Werror -Wformat=2 -pedantic \
    practice/beginner_0o.c -o build/beginner_0o
./build/beginner_0o
```

</details>


### Concept


Code này dạy boundary:

    interrupt/source
        ↓
      event
        ↓
    deferred work

Sau này ta mới thêm:

    bounded ring
    atomic indices
    priority
    queue depth
    generation
    deadline


## 0.16. Buffer có ownership, không chỉ có pointer


Một pointer nói:

    memory ở đâu

nhưng không tự nói:

    ai được phép sửa nó lúc này?

Trong modem, buffer có thể đi qua:

    CPU
    DMA
    accelerator
    AP/CP shared memory

Một model ownership có thể là:

    FREE
      ↓
    CPU_OWNED
      ↓
    DMA_OWNED
      ↓
    DONE
      ↓
    FREE

Nếu CPU ghi khi:

    DMA_OWNED

thì bug không còn là bài toán toán học.

Đó là concurrency/ownership bug.


### MINI CODE 0P — Ownership state toy


    #include <stdio.h>

    enum buffer_state {
        BUF_FREE,
        BUF_CPU_OWNED,
        BUF_DMA_OWNED,
        BUF_DONE
    };

    static int submit_to_dma(enum buffer_state *state)
    {
        if (*state != BUF_CPU_OWNED) {
            return -1;
        }

        *state = BUF_DMA_OWNED;
        return 0;
    }

    int main(void)
    {
        enum buffer_state state = BUF_CPU_OWNED;

        if (submit_to_dma(&state) == 0) {
            printf("DMA now owns the buffer\n");
        }

        return 0;
    }

<!-- SYNTAX_LENS_0P -->
#### Syntax ngay dưới MINI CODE 0P — pointer tới state và dereference có kiểm tra

> **BEGINNER GATE 0P:** gọi submit hai lần trên cùng state: lần đầu thành công,
> lần hai phải fail và state không đổi. Sau đó mới test `NULL`.

```c
static int submit_to_dma(enum buffer_state *state)
```

nhận địa chỉ state để function có thể mutate object caller. Caller truyền
`&state`; callee đọc/ghi `*state`. Hai operator là hai chiều của cùng boundary:

```text
&object  → lấy địa chỉ
*pointer → truy cập object tại địa chỉ
```

Toy code chưa check NULL. Bản an toàn phải check trước `*state`; success đổi
`BUF_CPU_OWNED→BUF_DMA_OWNED`, failure giữ state nguyên. Return `int` ở đây là
status (`0` success, `-1` failure), không phải data payload.

<details>
<summary>Đáp án syntax 0P — failure không dereference NULL</summary>

```c
static int submit_to_dma(enum buffer_state *state)
{
    if ((state == NULL) || (*state != BUF_CPU_OWNED)) return -1;
    *state = BUF_DMA_OWNED;
    return 0;
}
```

Full file test success, repeated submit và `NULL`:

```c
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

enum buffer_state {
    BUF_CPU_OWNED,
    BUF_DMA_OWNED
};

static int submit_to_dma(enum buffer_state *state)
{
    if ((state == NULL) || (*state != BUF_CPU_OWNED)) return -1;
    *state = BUF_DMA_OWNED;
    return 0;
}

int main(void)
{
    enum buffer_state state = BUF_CPU_OWNED;

    assert(submit_to_dma(&state) == 0);
    assert(state == BUF_DMA_OWNED);
    assert(submit_to_dma(&state) == -1);
    assert(state == BUF_DMA_OWNED);
    assert(submit_to_dma(NULL) == -1);
    puts("CPU_OWNED -> DMA_OWNED; repeated submit rejected");
    return 0;
}
```

```bash
gcc -std=c17 -Wall -Wextra -Werror -Wformat=2 -pedantic \
    practice/beginner_0p.c -o build/beginner_0p
./build/beginner_0p
```

</details>


### Tại sao học cái này trong khóa modem?


Vì một modem có thể cho output sai dù:

    FFT đúng
    QPSK đúng
    CRC đúng

chỉ vì:

    buffer bị reuse quá sớm

Đó là lý do khi đọc firmware phải theo dõi không chỉ data flow.


## 0.17. Ba flow phải theo dõi cùng lúc


Khi đọc modem firmware, luôn hỏi ba câu song song:

    DATA FLOW
        sample / bit / PDU đang đi đâu?

    STATE FLOW
        modem / bearer / HARQ / protocol đang ở state nào?

    OWNERSHIP FLOW
        ai đang sở hữu buffer / descriptor / key slot?


Ví dụ:

    RF DMA
       │
       │ IQ buffer
       ▼
    L1_RX task
       │
       │ decoded TB
       ▼
      MAC
       │
       │ MAC SDU
       ▼
      RLC

Đó là DATA FLOW.

Song song:

    RRC_SEARCHING
        ↓
    RRC_CAMPED
        ↓
    RRC_CONNECTED

Đó là STATE FLOW.

Song song nữa:

    FREE
      ↓
    DMA_OWNED
      ↓
    CPU_OWNED
      ↓
    FREE

Đó là OWNERSHIP FLOW.

Nếu chỉ nhìn một trong ba, nhiều bug modem sẽ rất khó hiểu.


## 0.18. AP và baseband processor không phải một thứ


Trong smartphone, mental model rất thô:

    | Application Processor — AP          |
    | Android / apps / networking         |
                    │
                    │ IPC / shared memory
                    ▼
    | Communication Processor — CP        |
    | baseband / modem real-time firmware |
                    │
                    ▼
               RF / antenna

AP có thể yêu cầu:

    "hãy tạo data connection"

Nhưng CP mới phải làm:

    cell search
    RRC
    NAS
    PHY
    HARQ
    security context
    timing

Trong project học, các khái niệm như:

    AP↔CP SHM
    IPC doorbell
    buffer handle
    MMIO

mô phỏng những boundary kiểu này.


### MINI CODE 0Q — API boundary


Tư duy tốt hơn việc cho AP chọc thẳng vào state nội bộ:

    enum cp_command {
        CP_CMD_START,
        CP_CMD_STOP
    };

    int cp_post_command(enum cp_command command);

<!-- SYNTAX_LENS_0Q -->
#### Syntax ngay dưới MINI CODE 0Q — declaration là boundary trước implementation

> **BEGINNER GATE 0Q:** trước function pointer, hãy tự viết bản trực tiếp
> `cp_post_command(command)` có hai command hợp lệ và một unknown command bị
> reject. `ops + ctx` ở cuối Lens là tầng engineering, không phải prerequisite.

Dòng trên kết thúc bằng `;`, nên nó chỉ công bố function contract:

```text
name      cp_post_command
input     một enum cp_command by-value
return    int status
body      chưa xuất hiện ở đây
```

Header public chứa declaration; một source file chứa definition có `{...}`.
Compiler dùng prototype để type-check caller, linker nối caller với đúng symbol.

Ba biến thể phải code ngay:

1. unknown command reject, state không đổi;
2. API async trả “accepted” nhưng completion đi qua event;
3. thêm `void *ctx` để hai CP instance không dùng global state.

<details>
<summary>Đáp án syntax 0Q — ops + context tối thiểu</summary>

```c
struct cp_ops {
    int (*post_command)(void *ctx, enum cp_command command);
};

static int cp_post(const struct cp_ops *ops, void *ctx,
                   enum cp_command command)
{
    if ((ops == NULL) || (ops->post_command == NULL)) return -1;
    return ops->post_command(ctx, command);
}
```

Full file cho tầng engineering; trước khi đọc function pointer, hãy tự thay ba
lời gọi `cp_post(...)` bằng lời gọi trực tiếp `cp_post_impl(&context, command)`:

```c
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

enum cp_command {
    CP_CMD_START,
    CP_CMD_STOP
};

struct cp_context {
    bool running;
};

struct cp_ops {
    int (*post_command)(void *ctx, enum cp_command command);
};

static int cp_post_impl(void *opaque, enum cp_command command)
{
    struct cp_context *context = opaque;

    if (context == NULL) return -1;
    switch (command) {
    case CP_CMD_START: context->running = true; return 0;
    case CP_CMD_STOP:  context->running = false; return 0;
    default:           return -1;
    }
}

static int cp_post(const struct cp_ops *ops, void *ctx,
                   enum cp_command command)
{
    if ((ops == NULL) || (ops->post_command == NULL)) return -1;
    return ops->post_command(ctx, command);
}

int main(void)
{
    const struct cp_ops ops = {.post_command = cp_post_impl};
    struct cp_context context = {.running = false};

    assert(cp_post(&ops, &context, CP_CMD_START) == 0);
    assert(context.running);
    assert(cp_post(&ops, &context, (enum cp_command)99) == -1);
    assert(context.running);
    assert(cp_post(&ops, &context, CP_CMD_STOP) == 0);
    assert(!context.running);
    puts("START -> reject UNKNOWN -> STOP");
    return 0;
}
```

```bash
gcc -std=c17 -Wall -Wextra -Werror -Wformat=2 -pedantic \
    practice/beginner_0q.c -o build/beginner_0q
./build/beginner_0q
```

Function pointer cung cấp behavior; `ctx` cung cấp state của instance.

</details>

AP gửi command qua interface.

CP tự quyết định state transition hợp lệ.

Boundary rõ ràng giúp tránh:

    raw pointer leak
    state mutation tùy tiện
    side-channel giữa test peer và UE


## 0.19. Fixed-point: tại sao modem không chỉ dùng double?


Trong chương trình host, dùng:

    double

rất tiện để học DSP.

Nhưng firmware/DSP thực tế thường quan tâm fixed-point vì:

    predictable bit width
    saturation rõ ràng
    hardware primitive nhanh
    memory nhỏ
    pipeline accelerator
    tránh phụ thuộc floating-point ABI trong một số target

Ví dụ Q15 có thể hiểu gần đúng:

    32767   ≈ +1.0
    16384   ≈ +0.5
   -16384   ≈ -0.5

Nếu nhân hai int16_t, ta thường widen trước:

    int32_t product =
        (int32_t)a * (int32_t)b;

Nếu kết quả vượt miền int16_t, cần nghĩ đến:

    scaling
    rounding
    saturation

Nếu cứ cast mù:

    int16_t y = (int16_t)huge_value;

có thể wrap thành giá trị hoàn toàn khác.


### MINI CODE 0R — Widen trước khi nhân


    #include <stdint.h>
    #include <stdio.h>

    int main(void)
    {
        int16_t a = 20000;
        int16_t b = 2;

        int32_t wide = (int32_t)a * (int32_t)b;

        printf("%ld\n", (long)wide);
        return 0;
    }

Expected:

    40000

<!-- SYNTAX_LENS_0R -->
#### Syntax ngay dưới MINI CODE 0R — cast đúng chỗ và format theo result type

> **BEGINNER GATE 0R:** viết ba biến `a`, `b`, `wide`; in `wide` trước. Sau đó
> mới thêm `sat16(wide)` và so sánh hai output. Phải thấy rõ compute type khác
> storage type.

```c
int32_t wide = (int32_t)a * (int32_t)b;
```

Hai cast xảy ra trước `*`, nên phép nhân được thực hiện trong `int32_t`. Pattern
phải thuộc bằng tay:

```text
storage hẹp → widen operands → compute → scale/check → narrow có saturation
```

Cast mù ở cuối là sai hướng:

```c
int16_t wrong = (int16_t)(a * b);
```

vì nó vừa không công bố overflow policy, vừa có thể thu hẹp giá trị 40000 thành
bit-pattern mang nghĩa khác.

Dòng gốc dùng `(long)wide` + `%ld`. Cách theo fixed-width contract:

```c
#include <inttypes.h>
printf("%" PRId32 "\n", wide);
```

**Tự code ngay:** thử `a=INT16_MIN`, `b=-1`; giữ result ở `int32_t`, rồi gọi
`sat16` để thấy compute đúng nhưng storage đích cần policy.

<details>
<summary>Đáp án syntax 0R — narrow có saturation</summary>

```c
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>

static int16_t sat16(int32_t value)
{
    if (value > INT16_MAX) return INT16_MAX;
    if (value < INT16_MIN) return INT16_MIN;
    return (int16_t)value;
}

int32_t wide = (int32_t)INT16_MIN * INT32_C(-1);
int16_t narrowed = sat16(wide);
printf("wide=%" PRId32 " narrow=%" PRId16 "\n", wide, narrowed);
```

Ba statement trên chưa phải một chương trình. Full file để test cả positive và
negative boundary:

```c
#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

static int16_t sat16(int32_t value)
{
    if (value > INT16_MAX) return INT16_MAX;
    if (value < INT16_MIN) return INT16_MIN;
    return (int16_t)value;
}

static int32_t mul_wide(int16_t a, int16_t b)
{
    return (int32_t)a * (int32_t)b;
}

int main(void)
{
    int32_t positive = mul_wide(INT16_MIN, INT16_C(-1));
    int32_t negative = mul_wide(INT16_MIN, INT16_C(2));
    int16_t positive_sat = sat16(positive);
    int16_t negative_sat = sat16(negative);

    printf("positive=%" PRId32 " -> %" PRId16 "\n",
           positive, positive_sat);
    printf("negative=%" PRId32 " -> %" PRId16 "\n",
           negative, negative_sat);
    assert((positive == INT32_C(32768)) && (positive_sat == INT16_MAX));
    assert((negative == INT32_C(-65536)) && (negative_sat == INT16_MIN));
    return 0;
}
```

```bash
gcc -std=c17 -Wall -Wextra -Werror -Wformat=2 -pedantic \
    practice/beginner_0r.c -o build/beginner_0r
./build/beginner_0r
```

Expected logic: `wide=32768`, `narrow=32767` nếu `sat16` clamp đúng.

</details>


### Ý nghĩa


40000 không fit trong int16_t dương.

Bài sat16 sau này sẽ buộc bạn quyết định:

    clamp về 32767

thay vì để overflow phá constellation/DSP.


## 0.20. Toàn bộ byte → waveform → byte, nhìn lại lần nữa


Bây giờ quay lại chữ:

    'A'

TX high-level:

    'A'
     │
     │ application/protocol data
     ▼
    PDU / SDU handling
     ▼
    transport block
     ▼
    CRC / coding
     ▼
    bits
     ▼
    QPSK / QAM mapping
     ▼
    complex symbols
     ▼
    resource grid
     ▼
    IFFT
     ▼
    cyclic prefix
     ▼
    time-domain IQ samples
     ▼
    virtual RF/channel

RX:

    IQ samples
     ▼
    synchronization
     ▼
    CFO correction / AGC
     ▼
    remove CP
     ▼
    FFT
     ▼
    resource extraction
     ▼
    equalization
     ▼
    demapper
     ▼
    bits / soft information
     ▼
    decode
     ▼
    CRC check
     ▼
    MAC
     ▼
    RLC
     ▼
    PDCP
     ▼
    RRC/NAS hoặc user data
     ▼
    'A'


### Đừng học thuộc sơ đồ như thơ


Hãy dùng năm câu sau tại MỌI stage:

    1. Input representation là gì?
    2. Output representation là gì?
    3. Hàm có đổi data hay chỉ đổi metadata/state?
    4. Ai sở hữu buffer?
    5. Nếu stage sai, symptom sẽ xuất hiện ở đâu?


## 0.21. Năm loại code bạn sẽ viết trong khóa


1. REPRESENTATION CODE

Ví dụ:

    struct cpx16
    endian helpers
    packet fields
    handles

Câu hỏi:

    dữ liệu được biểu diễn thế nào?


2. DSP CODE

Ví dụ:

    QPSK/QAM
    FFT/IFFT
    correlation
    CFO correction
    equalization

Câu hỏi:

    signal được biến đổi thế nào?


3. PROTOCOL CODE

Ví dụ:

    MAC PDU
    RLC SN
    PDCP COUNT
    RRC/NAS state machine

Câu hỏi:

    data/control được tổ chức và theo dõi thế nào?


4. RUNTIME CODE

Ví dụ:

    ring
    timer
    IRQ
    scheduler
    pool
    DMA ownership

Câu hỏi:

    công việc chạy khi nào và ai sở hữu resource?


5. INTEGRATION CODE

Ví dụ:

    NetworkPeer
    virtual RF
    deterministic runner
    trace
    end-to-end test

Câu hỏi:

    mọi khối có thực sự nối với nhau không?


## 0.22. Cách học từ đây: concept luôn đi cùng code


Từ Bài 1 trở đi, mỗi bài phải được đọc theo bốn lớp.


LỚP A — CODE TASK

    Tôi phải viết hàm/chương trình gì?


LỚP B — C CONCEPT

    Tôi đang học syntax/semantics C gì?

Ví dụ:

    struct
    array
    pointer
    integer width
    enum
    function
    atomic


LỚP C — MODEM CONCEPT

    Code này đại diện cho khối nào?

Ví dụ:

    IQ sample
    modulation
    buffer
    timer
    state transition


LỚP D — SYSTEM CONNECTION

    Output của bài này sẽ được ai dùng tiếp?

Ví dụ:

    qpsk_map
       ↓
    resource mapper
       ↓
    IFFT


## 0.23. Checklist bắt buộc sau mỗi bài code


Sau khi chương trình chạy đúng, CHƯA được coi là xong.

Tự trả lời:

    INPUT
        Khối vừa code nhận gì?

    OUTPUT
        Nó tạo ra gì?

    REPRESENTATION
        Nó đổi từ dạng dữ liệu nào sang dạng nào?

    LAYER
        Nó thuộc:
            RF?
            PHY?
            L2/L3?
            runtime?
            integration?

    STATE
        Nó có thay đổi state nào không?

    OWNERSHIP
        Ai sở hữu input/output buffer?

    FAILURE
        Nếu code sai, symptom sẽ là gì?

    NEXT
        Khối nào dùng output tiếp?


Nếu không trả lời được tám mục trên:

    chưa hiểu bài

dù:

    gcc compile sạch
    test pass
    output đúng


## 0.24. Một ví dụ cách suy nghĩ đúng


Giả sử sau này bạn code:

    void qpsk_map(...);

Đừng chỉ ghi:

    "hàm này map QPSK"

Hãy tự giải thích:

    INPUT
        2 bit

    OUTPUT
        1 complex symbol

    REPRESENTATION
        bit domain → constellation domain

    LAYER
        PHY TX

    STATE
        không nhất thiết đổi RRC/NAS state

    OWNERSHIP
        caller đưa output pointer; function fill output

    FAILURE
        mapping sai → receiver demap sai dù channel perfect

    NEXT
        resource mapper đặt symbol vào OFDM grid

Đó mới là hiểu code modem.


## 0.25. Điểm xuất phát trước Bài 1


Sau PHẦN 0, bạn CHƯA cần:

    biết FFT bằng đầu
    thuộc 3GPP
    hiểu hết RRC
    biết DMA descriptor chi tiết
    biết Armv7-R startup

Nhưng bạn phải bắt đầu thấy được bức tranh:

    modem không phải một hàm lớn

mà là:

    pipeline data
    +
    state machines
    +
    real-time runtime
    +
    resource ownership

và mọi thứ cuối cùng đều rơi xuống những thứ C rất cụ thể:

    integer
    struct
    array
    pointer
    function
    enum
    buffer
    loop
    bit operation

PHẦN I bắt đầu từ đúng chỗ đó.

Bài 1 sẽ quay lại:

    struct cpx16

nhưng lần này bạn đã biết:

    nó là gì
    nó đứng ở đâu
    tại sao modem cần nó
    và sau này nó sẽ đi vào pipeline nào

---

---

# PHẦN I — C CẦN THIẾT ĐỂ KHÔNG CÒN COPY CODE MÙ

## Level 1 — Biến, kiểu dữ liệu, hàm, mảng

### Bài 1 — Sample IQ đầu tiên

#### 1. Đề bài nhỏ

Ở PHẦN 0 ta đã có mental model đầu tiên về modem. Bây giờ bắt đầu học C bằng
đúng object nhỏ nhất của đường IQ: một sample radio số được biểu diễn bởi hai số:

- `I`: in-phase
- `Q`: quadrature

Hãy viết chương trình tạo một sample có `I = 1200`, `Q = -450`, rồi in ra.

#### 2. Tự đoán chương trình cần làm gì

Chương trình cần tạo đúng một object chứa hai thành phần I/Q rồi in đúng hai giá trị đã gán. Chưa có modulation hay RF ở đây.

Trước khi nhìn code, hãy tự viết một dòng dự đoán output vào giấy hoặc comment.

#### 3. Học đúng lượng kiến thức cần cho bài

Trong C, một giá trị 16-bit có dấu dùng `int16_t`.

```c
#include <stdint.h>
```

Ta gom hai giá trị liên quan vào một `struct`:

```c
struct cpx16 {
    int16_t i;
    int16_t q;
};
```

Tạo biến:

```c
struct cpx16 x;
x.i = 1200;
x.q = -450;
```

Hoặc initializer:

```c
struct cpx16 x = {1200, -450};
```

In bằng `printf` trong chương trình host để học:

```c
printf("I=%d Q=%d\n", (int)x.i, (int)x.q);
```

#### 4. Tự code — Bài 1 được cầm tay chỉ việc

```c
#include <stdint.h>
#include <stdio.h>

struct cpx16 {
    int16_t i;
    int16_t q;
};

int main(void)
{
    struct cpx16 x = {1200, -450};

    printf("I=%d Q=%d\n", (int)x.i, (int)x.q);
    return 0;
}
```

##### Syntax checkpoint ngay dưới Bài 1 — phải tự đọc được full file

```text
#include <stdint.h> → lấy fixed-width integer types
#include <stdio.h>  → lấy printf
struct ... { ... }; → định nghĩa type
struct cpx16 x      → tạo object có type đó
{1200,-450}         → initializer theo thứ tự field
x.i / x.q           → truy cập field bằng dấu chấm
(int)x.i             → convert rõ sang type %d đòi
return 0             → báo process thành công
```

Không quay lên MINI CODE 0C để tra. Ngay tại đây, nếu chưa giải thích được một
dòng, comment dòng đó, compile, quan sát diagnostic/output thay đổi, rồi ghi câu
trả lời cạnh code.

Ba biến thể và đáp án tại chỗ:

1. Đổi sang designated initializer `.q=-450,.i=1200`.
2. In thêm `sizeof x` bằng `%zu`.
3. In `int16_t` bằng `PRId16` mà không cast.

> **Thứ tự beginner bắt buộc:** trước tiên compile nguyên Bài 1 và nhìn thấy
> `I=1200 Q=-450`. Sau đó làm biến thể 1, compile; làm biến thể 2, compile; cuối
> cùng mới làm biến thể 3. Không sửa cả ba rồi mới build, vì khi warning xuất hiện
> bạn sẽ không biết thay đổi nào gây ra nó.

<details>
<summary>Đáp án syntax checkpoint Bài 1</summary>

```c
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

struct cpx16 { int16_t i, q; };

int main(void)
{
    struct cpx16 x = {.q = INT16_C(-450), .i = INT16_C(1200)};
    printf("I=%" PRId16 " Q=%" PRId16 " object_bytes=%zu\n",
           x.i, x.q, sizeof x);
    return 0;
}
```

`sizeof x` trả `size_t`→`%zu`; field fixed-width dùng macro `PRI*` tương ứng.

```bash
gcc -std=c17 -Wall -Wextra -Werror -Wformat=2 -pedantic \
    practice/task_01_formats.c -o build/task_01_formats
./build/task_01_formats
```

Expected phải chứa `I=1200 Q=-450`; `object_bytes` phải bằng `sizeof x` trên chính
target đang chạy, không học thuộc một con số layout từ máy khác.

</details>

Đây là bài duy nhất full code được đặt lộ thiên. Hãy tự gõ từng dòng, không copy cả block.

#### 5. Chạy test / quan sát output

Output chính xác phải là `I=1200 Q=-450`.

```bash
mkdir -p practice build
gcc -std=c17 -Wall -Wextra -Werror practice/task_01.c -o build/task_01
./build/task_01
```

Expected:

```text
I=1200 Q=-450
```

#### 6. Debug

Nếu compiler không biết `int16_t`, kiểm tra `<stdint.h>`; nếu `%d` warning, giữ cast `(int)`; nếu output sai dấu, kiểm tra initializer.

#### Sau khi code: ý nghĩa modem/baseband

`struct cpx16` không phải “sóng”. Nó chỉ là một object C chứa hai số nguyên.

Radio waveform số sẽ là **một dãy rất nhiều sample**:

```text
sample[0] sample[1] sample[2] ...
```

mỗi sample có:

```text
(I, Q)
```

---

**Bài này thuộc nhóm:** Representation / complex baseband.

**Bạn vừa code cái gì trong modem?** `struct cpx16` là đơn vị dữ liệu nhỏ nhất của waveform số trong khóa: một complex sample I/Q.

**Nó nằm ở đâu?** RF/PHY boundary.

```text
INPUT  → hai số I,Q từ ADC hoặc simulator
KHỐI   → code của Bài 1
OUTPUT → một object complex sample có kiểu rõ ràng
```

**Tại sao modem cần nó?** Các bài sau sẽ biến một sample thành mảng sample, rồi thành cả waveform. Nếu chưa hiểu rằng `I,Q` chỉ là representation của một điểm tín hiệu theo thời gian, mọi phần QPSK/FFT phía sau sẽ trở thành công thức vô nghĩa.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 1 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải

Bài 1 là ngoại lệ: implementation có hướng dẫn chính là block ở Bước 4. Bây giờ đọc lại từng dòng và tự trả lời: dòng này tạo type, object, hay observable output?

#### 8. Viết lại không nhìn lời giải

Đóng Bước 4 khỏi màn hình, tạo `practice/task_01_rebuild.c`, tự viết lại từ file trắng và chỉ dùng expected output. Chỉ sang Bài 2 khi bản rebuild tự compile và in đúng.


<!-- WIZARD_TASK_1_BEGIN -->
#### 9. Wizard Lab — Tự chế vocabulary cho complex sample

**Ý nghĩ quái dị:** Nếu coi một sample như viên gạch, mình muốn tự tạo, đổi dấu, hoán đổi I/Q và xoay nó mà không viết lại field assignment ở mọi nơi thì sao?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
struct cpx16 cpx16_make(int16_t i, int16_t q);
struct cpx16 cpx16_conjugate(struct cpx16 x);
struct cpx16 cpx16_swap_iq(struct cpx16 x);
struct cpx16 cpx16_rotate_90(struct cpx16 x);
```

1. **Bậc 1:** Tạo bốn sample ở bốn quadrant và dự đoán output của từng helper bằng tay.
2. **Bậc 2:** Gọi `rotate_90` bốn lần; kiểm tra sample quay về giá trị ban đầu, trừ trường hợp saturation ở biên.
3. **Bậc 3:** Thử `I/Q = INT16_MIN`; ghi lại vì sao phép đổi dấu cần widen + saturation.

**Observer bắt buộc:** In input/output cùng quadrant; thêm `cpx16_equal()` làm predicate.

**Invariant:** Helper by-value không được sửa object caller; rotate bốn lần là identity với giá trị không chạm biên.

**Tự chế spell tiếp theo:** Tự viết `cpx16_rotate_quadrant(x, turns)` với `turns` từ -3 tới +3.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 1</strong></summary>

Lưu thành `practice/wizard_task_01.c`:

```c
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

struct cpx16 { int16_t i; int16_t q; };

static int16_t neg_sat(int16_t value)
{
    return value == INT16_MIN ? INT16_MAX : (int16_t)-value;
}

static struct cpx16 cpx16_make(int16_t i, int16_t q)
{
    return (struct cpx16){ .i = i, .q = q };
}

static struct cpx16 cpx16_conjugate(struct cpx16 x)
{
    return cpx16_make(x.i, neg_sat(x.q));
}

static struct cpx16 cpx16_swap_iq(struct cpx16 x)
{
    return cpx16_make(x.q, x.i);
}

static struct cpx16 cpx16_rotate_90(struct cpx16 x)
{
    return cpx16_make(neg_sat(x.q), x.i);
}

int main(void)
{
    const struct cpx16 x = cpx16_make(INT16_C(3), INT16_C(-4));
    struct cpx16 y = x;
    assert(cpx16_conjugate(x).q == 4);
    assert(cpx16_swap_iq(x).i == -4);
    for (size_t k = 0; k < 4U; ++k) y = cpx16_rotate_90(y);
    assert(y.i == x.i && y.q == x.q);
    puts("wizard task 01 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_01.c -o wizard_task_01
./wizard_task_01
```

Expected cuối output:

```text
wizard task 01 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_1_END -->

---

#### Forge F01 — Bài 1: một sample IQ, ba cách nói suy nghĩ bằng code

Tự code:

1. `iq_power` trả `I²+Q²` không overflow với mọi `int16_t`.
2. `iq_conjugate` đổi dấu Q có saturation tại `INT16_MIN`.
3. `iq_rotate90` biến `(I,Q)` thành `(-Q,I)`.

<details>
<summary>Đáp án F01 + test trọng điểm</summary>

```c
#include <assert.h>
#include <limits.h>
#include <stdint.h>

struct cpx16 { int16_t i, q; };
static int16_t f01_sat(int32_t x)
{
    if (x > INT16_MAX) return INT16_MAX;
    if (x < INT16_MIN) return INT16_MIN;
    return (int16_t)x;
}
static uint64_t iq_power(struct cpx16 x)
{
    int64_t i = x.i, q = x.q;
    return (uint64_t)(i * i + q * q);
}
static struct cpx16 iq_conjugate(struct cpx16 x)
{
    x.q = f01_sat(-(int32_t)x.q); return x;
}
static struct cpx16 iq_rotate90(struct cpx16 x)
{
    return (struct cpx16){f01_sat(-(int32_t)x.q), x.i};
}
/* assert(iq_power((struct cpx16){INT16_MIN,INT16_MIN}) == 2147483648ULL); */
/* assert(iq_conjugate((struct cpx16){0,INT16_MIN}).q == INT16_MAX); */
```

</details>

### Bài 2 — Một block IQ

#### 1. Đề bài nhỏ

Chưa mở Bước 7 ở ngay dưới bài này.

Tạo mảng 8 sample:

```text
(100, 0)
(200, 10)
(300, 20)
(400, 30)
(500, 40)
(600, 50)
(700, 60)
(800, 70)
```

Yêu cầu:

1. Dùng `struct cpx16 samples[8]`.
2. Dùng vòng `for` để in toàn bộ.
3. Viết hàm:

```c
int32_t energy_l1(struct cpx16 x);
```

trả về:

```text
abs(I) + abs(Q)
```

4. In energy của từng sample.

Expected dạng:

```text
0: I=100 Q=0 E=100
1: I=200 Q=10 E=210
...
```

#### 2. Tự đoán chương trình cần làm gì

Chương trình phải duyệt đủ 8 sample, giữ đúng thứ tự và tính một số đo đơn giản cho từng sample. Output là 8 dòng, không phải một giá trị tổng.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- array cố định và `size_t`
- vòng `for`
- hàm nhận `struct` by value và trung gian `int32_t`

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

Kiến thức đã có sẵn trong đề cũ:

**Kiến thức cần tự dùng**

```c
for (size_t i = 0; i < 8; ++i) {
    ...
}
```

Một hàm:

```c
int32_t f(struct cpx16 x)
{
    ...
}
```

Không dùng `abs()` nếu chưa muốn học library. Tự viết:

```c
int32_t abs32(int32_t x)
{
    return x < 0 ? -x : x;
}
```

##### Syntax checkpoint ngay dưới Bài 2 — function, by-value và toán tử `?:`

```c
int32_t abs32(int32_t x)
```

nghĩa là function nhận một bản sao `int32_t x` và trả `int32_t`. Nó không sửa
sample caller. Expression:

```c
x < 0 ? -x : x
```

đọc là “nếu condition đúng lấy expression giữa, nếu sai lấy expression cuối”.
Đây là conditional operator, không phải cú pháp riêng của DSP.

Loop:

```c
for (size_t i = 0; i < 8; ++i)
```

dùng `size_t` vì `i` là array index; in `i` bằng `%zu`. Nhưng `abs32(INT32_MIN)`
không biểu diễn được số dương tương ứng trong `int32_t`, nên helper hiện tại cần
contract hoặc accumulator rộng hơn.

**Biến thể bắt buộc:** viết `magnitude_i16(int16_t)` trả `uint32_t`, test
`INT16_MIN`, rồi dùng nó trong `energy_l1` để không có unary-negation overflow.

<details>
<summary>Đáp án syntax checkpoint Bài 2</summary>

```c
static uint32_t magnitude_i16(int16_t value)
{
    int32_t wide = value;
    return (uint32_t)((wide < 0) ? -wide : wide);
}

static uint32_t energy_l1(struct cpx16 x)
{
    return magnitude_i16(x.i) + magnitude_i16(x.q);
}
```

Hai helper trên là patch cho bài. Đây là full file boundary để beginner không
phải tự đoán include, type hoặc nơi đặt `main`:

```c
#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

struct cpx16 {
    int16_t i;
    int16_t q;
};

static uint32_t magnitude_i16(int16_t value)
{
    int32_t wide = value;
    return (uint32_t)((wide < 0) ? -wide : wide);
}

static uint32_t energy_l1(struct cpx16 x)
{
    return magnitude_i16(x.i) + magnitude_i16(x.q);
}

int main(void)
{
    const struct cpx16 normal = {100, -20};
    const struct cpx16 edge = {INT16_MIN, INT16_MIN};

    assert(magnitude_i16(0) == 0U);
    assert(magnitude_i16(INT16_C(-7)) == 7U);
    assert(magnitude_i16(INT16_MIN) == UINT32_C(32768));
    assert(energy_l1(normal) == UINT32_C(120));
    assert(energy_l1(edge) == UINT32_C(65536));
    printf("normal=%" PRIu32 " edge=%" PRIu32 "\n",
           energy_l1(normal), energy_l1(edge));
    return 0;
}
```

```bash
gcc -std=c17 -Wall -Wextra -Werror -Wformat=2 -pedantic \
    practice/task_02_boundary.c -o build/task_02_boundary
./build/task_02_boundary
```

Expected: `normal=120 edge=65536`. Nếu bản dùng `int16_t` cho magnitude in số âm
hoặc sanitizer báo negate overflow, widen đã xảy ra quá muộn.

Widen xảy ra trước negate; với hai component `INT16_MIN`, tổng tối đa `65536`
vẫn fit `uint32_t`.

</details>

#### 4. Tự code

Làm trong `practice/task_02.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Phải có đúng 8 dòng; dòng đầu có `E=100`, dòng cuối có `E=870`. Thêm một sample âm để kiểm tra `abs32`.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_02.c \
    -o build/task_02
./build/task_02
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

Nếu thiếu/thừa dòng, in `i` trước. Nếu energy âm, kiểm tra widen sang `int32_t` trước khi lấy trị tuyệt đối.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** IQ stream / sample buffer.

**Bạn vừa code cái gì trong modem?** Modem không xử lý một sample đơn lẻ mà xử lý block hàng trăm/hàng nghìn sample. Mảng `samples[]` chính là phiên bản thu nhỏ của RX/TX DMA buffer.

**Nó nằm ở đâu?** PHY data plane.

```text
INPUT  → một block các complex sample
KHỐI   → code của Bài 2
OUTPUT → duyệt/đo một đặc trưng đơn giản trên từng sample
```

**Tại sao modem cần nó?** `energy_l1` chưa phải AGC thật; nó tập cho bạn nhìn waveform như dữ liệu số có thể đo. Sau này AGC, sync và detector cũng duyệt block IQ theo kiểu tương tự.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 2 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

```c
#include <stdint.h>
#include <stdio.h>

struct cpx16 {
    int16_t i;
    int16_t q;
};

static int32_t abs32(int32_t x)
{
    return x < 0 ? -x : x;
}

static int32_t energy_l1(struct cpx16 x)
{
    return abs32((int32_t)x.i) + abs32((int32_t)x.q);
}

int main(void)
{
    struct cpx16 samples[8] = {
        {100, 0}, {200, 10}, {300, 20}, {400, 30},
        {500, 40}, {600, 50}, {700, 60}, {800, 70}
    };

    for (size_t i = 0U; i < 8U; ++i) {
        printf("%zu: I=%d Q=%d E=%d\n",
               i,
               (int)samples[i].i,
               (int)samples[i].q,
               (int)energy_l1(samples[i]));
    }

    return 0;
}
```

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_2_BEGIN -->
#### 9. Wizard Lab — Biến block IQ thành canvas

**Ý nghĩ quái dị:** Nếu không muốn nhập từng sample, mình có thể sinh ramp, impulse, checkerboard I/Q hoặc pattern chỉ bằng vài helper không?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
void iq_fill(struct cpx16 *x, size_t n, struct cpx16 value);
void iq_make_ramp(struct cpx16 *x, size_t n, int16_t step_i, int16_t step_q);
int iq_paint_impulse(struct cpx16 *x, size_t n, size_t at, struct cpx16 value);
```

1. **Bậc 1:** Sinh ramp 8 sample rồi so energy từng phần tử với tính tay.
2. **Bậc 2:** Paint impulse tại index 0, giữa, cuối; invalid index phải giữ block nguyên vẹn.
3. **Bậc 3:** Viết `iq_paint_stride()` để đặt sample đặc biệt mỗi N vị trí.

**Observer bắt buộc:** `iq_dump`, energy min/max và digest toàn block.

**Invariant:** Generator không ghi ngoài `count`; failure không đổi block.

**Tự chế spell tiếp theo:** Tạo pattern mà I tăng, Q giảm rồi đo thời điểm saturation đầu tiên.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 2</strong></summary>

Lưu thành `practice/wizard_task_02.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

struct cpx16 { int16_t i; int16_t q; };

static void iq_fill(struct cpx16 *x, size_t n, struct cpx16 value)
{
    if (x == NULL) return;
    for (size_t k = 0; k < n; ++k) x[k] = value;
}

static void iq_make_ramp(struct cpx16 *x, size_t n,
                         int16_t step_i, int16_t step_q)
{
    if (x == NULL) return;
    for (size_t k = 0; k < n; ++k) {
        x[k].i = (int16_t)((int32_t)step_i * (int32_t)k);
        x[k].q = (int16_t)((int32_t)step_q * (int32_t)k);
    }
}

static int iq_paint_impulse(struct cpx16 *x, size_t n, size_t at,
                            struct cpx16 value)
{
    if (x == NULL || at >= n) return -1;
    x[at] = value;
    return 0;
}

int main(void)
{
    struct cpx16 x[8];
    iq_fill(x, 8U, (struct cpx16){0, 0});
    iq_make_ramp(x, 8U, 10, -2);
    assert(x[3].i == 30 && x[3].q == -6);
    assert(iq_paint_impulse(x, 8U, 5U, (struct cpx16){99, 7}) == 0);
    assert(iq_paint_impulse(x, 8U, 8U, (struct cpx16){1, 1}) == -1);
    assert(x[5].i == 99);
    puts("wizard task 02 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_02.c -o wizard_task_02
./wizard_task_02
```

Expected cuối output:

```text
wizard task 02 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_2_END -->

---

#### Forge F02 — Bài 2: block IQ không chỉ là một vòng `for`

Tự code:

1. Tính tổng energy bằng accumulator đủ rộng.
2. Tìm index sample có power lớn nhất, first-win khi hòa.
3. Zero toàn bộ Q in-place nhưng reject `(samples=NULL,count>0)`.

<details>
<summary>Đáp án F02 + invariants</summary>

```c
#include <stddef.h>
#include <stdint.h>
struct cpx16 { int16_t i, q; };
static uint64_t f02_p(struct cpx16 x)
{ int64_t i=x.i,q=x.q; return (uint64_t)(i*i+q*q); }
int iq_energy(const struct cpx16 *x, size_t n, uint64_t *out)
{
    size_t k; uint64_t sum=0U;
    if ((out==NULL)||((x==NULL)&&(n!=0U))) return -1;
    for (k=0U;k<n;++k) {
        uint64_t p=f02_p(x[k]); if (sum>UINT64_MAX-p) return -1; sum+=p;
    }
    *out=sum; return 0;
}
int iq_peak_index(const struct cpx16 *x,size_t n,size_t *out)
{
    size_t k,best=0U; uint64_t peak;
    if ((x==NULL)||(out==NULL)||(n==0U)) return -1;
    peak=f02_p(x[0]);
    for(k=1U;k<n;++k) if(f02_p(x[k])>peak){peak=f02_p(x[k]);best=k;}
    *out=best; return 0;
}
int iq_zero_q(struct cpx16 *x,size_t n)
{
    size_t k; if((x==NULL)&&(n!=0U)) return -1;
    for(k=0U;k<n;++k) x[k].q=0; return 0;
}
```

Test `n=0`, một sample và hai peak bằng nhau. Failure không được ghi out-param.

</details>

## Level 2 — Pointer không còn là phép thuật

Nếu chưa hiểu pointer, gần như không thể đọc firmware modem.

### Concept

```c
int x = 10;
int *p = &x;
```

- `x` = object chứa 10.
- `&x` = địa chỉ của object.
- `p` = biến chứa địa chỉ đó.
- `*p` = object nằm tại địa chỉ mà `p` trỏ tới.

```text
x
┌──────┐
│  10  │
└──────┘
  ▲
  │
  p
```

Đây là nền của:

- buffer
- DMA
- MMIO accessor
- parser
- API kiểu `out` parameter
- arena allocator

### Bài 3 — Sửa sample qua pointer

#### 1. Đề bài nhỏ

Viết:

```c
void scale_half(struct cpx16 *x);
```

Sau:

```c
struct cpx16 s = {1000, -600};
scale_half(&s);
```

thì:

```text
I=500 Q=-300
```

Chưa mở Bước 7 ở ngay dưới bài này.

#### 2. Tự đoán chương trình cần làm gì

Caller đưa địa chỉ của một sample; hàm sửa chính sample đó. Sau khi return, caller nhìn thấy I và Q đã giảm một nửa.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- địa chỉ `&`, dereference `*`, member access `->`
- NULL guard
- in-place update

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_03.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Test `{1000,-600}` → `{500,-300}`; gọi với `NULL` không crash; test số lẻ để tự xác định quy tắc chia integer.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_03.c \
    -o build/task_03
./build/task_03
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

Nếu caller không đổi, có thể bạn truyền `s` thay vì `&s` hoặc sửa một bản copy. Dùng GDB `print x` và `print *x`.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** In-place DSP.

**Bạn vừa code cái gì trong modem?** Rất nhiều DSP primitive sửa trực tiếp buffer để tránh copy: scale gain, correct CFO, windowing, equalization... Pointer cho phép hàm thao tác đúng object của caller.

**Nó nằm ở đâu?** PHY / DSP primitive.

```text
INPUT  → địa chỉ một sample
KHỐI   → code của Bài 3
OUTPUT → sample tại chính địa chỉ đó được cập nhật
```

**Tại sao modem cần nó?** Nếu chỉ hiểu pointer như cú pháp `*`/`&` mà không hiểu ownership, bạn sẽ rất dễ tạo use-after-free hoặc sửa nhầm DMA buffer khi học runtime.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 3 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

```c
void scale_half(struct cpx16 *x)
{
    if (x == NULL) {
        return;
    }

    x->i = (int16_t)(x->i / 2);
    x->q = (int16_t)(x->q / 2);
}
```

Điểm cần hiểu:

```c
x->i
```

tương đương:

```c
(*x).i
```

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_3_BEGIN -->
#### 9. Wizard Lab — Callback biến pointer thành cỗ máy biến hình

**Ý nghĩ quái dị:** Nếu muốn truyền một phép biến đổi sample như data rồi thay scale bằng conjugate/rotate mà vòng lặp không đổi thì sao?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
typedef struct cpx16 (*cpx_transform_fn)(struct cpx16 x, void *ctx);
void iq_apply(struct cpx16 *x, size_t n, cpx_transform_fn fn, void *ctx);
```

1. **Bậc 1:** Viết callback scale 1/2 dùng `ctx` chứa gain.
2. **Bậc 2:** Thay callback bằng đổi dấu Q mà `iq_apply` không đổi.
3. **Bậc 3:** Compose hai pass rồi so với một callback combined.

**Observer bắt buộc:** Dump trước/sau và count số sample thực sự đổi.

**Invariant:** Callback NULL hoặc buffer NULL không được crash; vòng lặp chỉ chạm `n` sample.

**Tự chế spell tiếp theo:** Viết `iq_apply_if` nhận thêm predicate theo index/sample.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 3</strong></summary>

Lưu thành `practice/wizard_task_03.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

struct cpx16 { int16_t i; int16_t q; };
typedef struct cpx16 (*cpx_transform_fn)(struct cpx16 x, void *ctx);

static void iq_apply(struct cpx16 *x, size_t n,
                     cpx_transform_fn fn, void *ctx)
{
    if (x == NULL || fn == NULL) return;
    for (size_t k = 0; k < n; ++k) x[k] = fn(x[k], ctx);
}

static struct cpx16 add_bias(struct cpx16 x, void *ctx)
{
    const struct cpx16 *bias = ctx;
    if (bias == NULL) return x;
    x.i = (int16_t)(x.i + bias->i);
    x.q = (int16_t)(x.q + bias->q);
    return x;
}

int main(void)
{
    struct cpx16 x[2] = {{1, 2}, {3, 4}};
    const struct cpx16 bias = {10, -1};
    iq_apply(x, 2U, add_bias, (void *)&bias);
    assert(x[0].i == 11 && x[0].q == 1);
    assert(x[1].i == 13 && x[1].q == 3);
    puts("wizard task 03 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_03.c -o wizard_task_03
./wizard_task_03
```

Expected cuối output:

```text
wizard task 03 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_3_END -->

---

#### Forge F03 — Bài 3: pointer có boundary và failure contract

Tự code:

1. Add `(di,dq)` có saturation qua pointer.
2. Swap hai sample, cho phép hai pointer trỏ cùng object.
3. Trả pointer tới phần tử thứ `index` chỉ khi `index<count`.

<details>
<summary>Đáp án F03</summary>

```c
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
struct cpx16 { int16_t i,q; };
static int16_t f03_sat(int32_t x)
{ return x>INT16_MAX?INT16_MAX:(x<INT16_MIN?INT16_MIN:(int16_t)x); }
int iq_add(struct cpx16 *x,int16_t di,int16_t dq)
{
    if(x==NULL)return -1;
    x->i=f03_sat((int32_t)x->i+di); x->q=f03_sat((int32_t)x->q+dq); return 0;
}
int iq_swap(struct cpx16 *a,struct cpx16 *b)
{
    struct cpx16 t; if((a==NULL)||(b==NULL))return -1;
    t=*a;*a=*b;*b=t;return 0;
}
int iq_at(struct cpx16 *base,size_t count,size_t index,struct cpx16 **out)
{
    if((out==NULL)||(base==NULL)||(index>=count))return -1;
    *out=&base[index];return 0;
}
```

Không làm `base + index` trước khi chứng minh index nằm trong array object.

</details>

### Bài 4 — Xử lý cả buffer

#### 1. Đề bài nhỏ

Viết:

```c
void zero_iq(struct cpx16 *samples, size_t count);
```

Hàm phải đặt toàn bộ `i/q = 0`.

Sau đó trả lời bằng lời:

- Tại sao hàm cần `count`?
- Nếu truyền `count` lớn hơn kích thước thật thì chuyện gì xảy ra?
- Tại sao firmware parser luôn phải kiểm tra length?

#### 2. Tự đoán chương trình cần làm gì

Hàm nhận địa chỉ phần tử đầu và số phần tử hợp lệ, rồi chỉ ghi trong đúng vùng đó. Không có `count` thì hàm không biết điểm dừng.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- contract `(pointer, count)`
- index `samples[i]`
- bounds và out-of-bounds write

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_04.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Test count 0, count 1 và count bằng toàn mảng. Dùng ASan để bắt trường hợp cố ý truyền count quá lớn trong một test riêng.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_04.c \
    -o build/task_04
./build/task_04
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

Nếu ASan báo heap/stack-buffer-overflow, so `count` với kích thước thật tại call site; pointer tự nó không chứa length.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** Buffer contract.

**Bạn vừa code cái gì trong modem?** Một pointer không mang theo kích thước. Firmware luôn phải biết buffer bắt đầu ở đâu và có bao nhiêu phần tử hợp lệ.

**Nó nằm ở đâu?** mọi data plane.

```text
INPUT  → pointer + count
KHỐI   → code của Bài 4
OUTPUT → toàn bộ vùng hợp lệ được xử lý
```

**Tại sao modem cần nó?** Đây là nền của mọi API kiểu `(samples, count)` và của defensive bounds checking. Modem nhận input liên tục; một `count` sai có thể biến lỗi radio thành memory corruption.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 4 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

```c
void zero_iq(struct cpx16 *samples, size_t count)
{
    if (samples == NULL) {
        return;
    }

    for (size_t i = 0U; i < count; ++i) {
        samples[i].i = 0;
        samples[i].q = 0;
    }
}
```

`count` sai lớn hơn allocation thật → out-of-bounds write. Đây là lý do length/capacity là contract bắt buộc.

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_4_BEGIN -->
#### 9. Wizard Lab — Sửa có chọn lọc thay vì quét mù

**Ý nghĩ quái dị:** Nếu chỉ muốn zero sample có energy vượt ngưỡng hoặc chỉ một cửa sổ thời gian thì helper nên trông thế nào?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
size_t iq_zero_range(struct cpx16 *x, size_t n, size_t first, size_t count);
size_t iq_zero_if(struct cpx16 *x, size_t n,
                  int (*predicate)(struct cpx16, void *), void *ctx);
```

1. **Bậc 1:** Zero cửa sổ [2,5) và đặt sentinel trước/sau buffer.
2. **Bậc 2:** Zero mọi sample có `abs(I)+abs(Q)>T`.
3. **Bậc 3:** So một pass predicate với hai pass range + threshold.

**Observer bắt buộc:** Changed-count, sentinel và first/last changed index.

**Invariant:** Mọi sample ngoài selection byte-identical.

**Tự chế spell tiếp theo:** Viết helper trả danh sách range liên tục thỏa predicate.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 4</strong></summary>

Lưu thành `practice/wizard_task_04.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

struct cpx16 { int16_t i; int16_t q; };

static size_t iq_zero_range(struct cpx16 *x, size_t n,
                            size_t first, size_t count)
{
    if (x == NULL || first >= n) return 0U;
    if (count > n - first) count = n - first;
    for (size_t k = first; k < first + count; ++k) x[k] = (struct cpx16){0, 0};
    return count;
}

static size_t iq_zero_if(struct cpx16 *x, size_t n,
                         int (*predicate)(struct cpx16, void *), void *ctx)
{
    size_t changed = 0U;
    if (x == NULL || predicate == NULL) return 0U;
    for (size_t k = 0; k < n; ++k) {
        if (predicate(x[k], ctx) != 0) { x[k] = (struct cpx16){0, 0}; ++changed; }
    }
    return changed;
}

static int i_above(struct cpx16 x, void *ctx)
{
    const int16_t *threshold = ctx;
    return threshold != NULL && x.i > *threshold;
}

int main(void)
{
    struct cpx16 x[4] = {{1,1},{5,1},{9,1},{2,1}};
    const int16_t threshold = 4;
    assert(iq_zero_range(x, 4U, 3U, 99U) == 1U);
    assert(iq_zero_if(x, 4U, i_above, (void *)&threshold) == 2U);
    assert(x[1].i == 0 && x[2].i == 0);
    puts("wizard task 04 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_04.c -o wizard_task_04
./wizard_task_04
```

Expected cuối output:

```text
wizard task 04 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_4_END -->

---

#### Forge F04 — Bài 4: biến đổi cả buffer với ba policy

Tự code:

1. Scale Q15 in-place và saturation.
2. Zero mỗi sample thứ `step`, reject `step==0` để tránh loop vô hạn.
3. Reverse-copy sang output khác; reject nếu hai range overlap.

<details>
<summary>Đáp án F04</summary>

```c
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
struct cpx16{int16_t i,q;};
static int16_t f04_sat(int64_t x)
{return x>INT16_MAX?INT16_MAX:(x<INT16_MIN?INT16_MIN:(int16_t)x);}
int iq_scale_q15(struct cpx16*x,size_t n,int16_t gain)
{
 size_t k;if((x==NULL)&&(n!=0U))return -1;
 for(k=0U;k<n;++k){x[k].i=f04_sat(((int64_t)x[k].i*gain)>>15);
                       x[k].q=f04_sat(((int64_t)x[k].q*gain)>>15);}return 0;
}
int iq_zero_every(struct cpx16*x,size_t n,size_t first,size_t step)
{
 size_t k;if((x==NULL)||(step==0U)||(first>=n))return -1;
 for(k=first;k<n;){x[k]=(struct cpx16){0,0};if(step>SIZE_MAX-k)break;k+=step;}
 return 0;
}
int iq_reverse_copy(const struct cpx16*in,struct cpx16*out,size_t n)
{
 size_t k,bytes;uintptr_t ia,oa;
 if(((in==NULL)||(out==NULL))&&(n!=0U))return -1;
 if(n>SIZE_MAX/sizeof *in)return -1;bytes=n*sizeof *in;
 ia=(uintptr_t)in;oa=(uintptr_t)out;
 if((bytes>UINTPTR_MAX-ia)||(bytes>UINTPTR_MAX-oa))return -1;
 if((n!=0U)&&(ia<oa+(uintptr_t)bytes)&&(oa<ia+(uintptr_t)bytes))return -1;
 for(k=0U;k<n;++k)out[k]=in[n-1U-k];return 0;
}
```

`uintptr_t` tồn tại trên profile của project và cho phép audit range theo địa chỉ.
Một API portable hơn có thể công bố non-overlap contract thay vì cố suy ra quan
hệ giữa pointer của hai object C không liên quan.

</details>

## Level 3 — Integer width, overflow, saturation

DSP firmware rất ghét kiểu `int` mơ hồ.

Dùng:

```c
uint8_t
uint16_t
uint32_t
uint64_t
int16_t
int32_t
int64_t
```

Ví dụ IQ dùng `int16_t` vì sample storage nhỏ, nhưng nhân hai sample phải widen trước:

```c
int32_t product = (int32_t)a.i * (int32_t)b.i;
```

Nếu nhân ngay trong type quá nhỏ, overflow có thể phá kết quả DSP.

### Saturation

Trong signal processing, đôi khi thay vì wrap:

```text
32767 + 1000 -> số âm
```

ta muốn clamp:

```text
> 32767  -> 32767
< -32768 -> -32768
```

### Bài 5 — `sat16`

#### 1. Đề bài nhỏ

Viết:

```c
int16_t sat16(int32_t value);
```

Test:

```text
sat16(100)       = 100
sat16(40000)     = 32767
sat16(-50000)    = -32768
```

Không hard-code chỉ ba input trên.

#### 2. Tự đoán chương trình cần làm gì

Giá trị nằm trong miền `int16_t` được giữ nguyên; giá trị vượt biên bị kẹp ở biên gần nhất, tuyệt đối không wrap sang dấu ngược lại.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- `INT16_MIN/MAX`
- so sánh trước cast
- saturation khác wraparound

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_05.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Test `INT16_MIN-1`, `INT16_MIN`, `-1`, `0`, `INT16_MAX`, `INT16_MAX+1` bằng input `int32_t`.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_05.c \
    -o build/task_05
./build/task_05
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

Nếu 40000 thành số âm, bạn đã cast xuống `int16_t` trước khi so biên. Mọi compare phải xảy ra ở `int32_t`.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** Saturation / fixed-point.

**Bạn vừa code cái gì trong modem?** DSP integer không được để overflow wrap ngẫu nhiên. `sat16` biến kết quả rộng hơn về miền `int16_t` bằng saturation.

**Nó nằm ở đâu?** PHY fixed-point DSP.

```text
INPUT  → giá trị trung gian 32-bit
KHỐI   → code của Bài 5
OUTPUT → sample 16-bit nằm trong miền an toàn
```

**Tại sao modem cần nó?** AGC, FFT, complex multiply và filter đều có thể tăng biên độ. Nếu wrap thay vì saturate, constellation có thể lật sang giá trị hoàn toàn khác dù code vẫn compile.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 5 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

```c
#include <limits.h>
#include <stdint.h>

int16_t sat16(int32_t value)
{
    if (value > INT16_MAX) {
        return INT16_MAX;
    }
    if (value < INT16_MIN) {
        return INT16_MIN;
    }
    return (int16_t)value;
}
```

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_5_BEGIN -->
#### 9. Wizard Lab — Tự thiết kế luật clipping

**Ý nghĩ quái dị:** Saturation không nhất thiết đối xứng; nếu muốn clamp I và Q bằng hai miền khác nhau để mô phỏng front-end méo thì sao?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
int16_t sat_range(int32_t value, int16_t low, int16_t high);
void iq_clip_axes(struct cpx16 *x, size_t n,
                  int16_t i_low, int16_t i_high,
                  int16_t q_low, int16_t q_high);
```

1. **Bậc 1:** Clip I ở ±1000 nhưng Q ở ±3000; đếm clip mỗi axis.
2. **Bậc 2:** Sweep threshold và vẽ bảng threshold→clipped_count.
3. **Bậc 3:** Cố ý truyền low>high; helper phải reject trước khi sửa.

**Observer bắt buộc:** Clip counter I/Q, peak trước/sau và constellation dump.

**Invariant:** Sau success mọi sample nằm trong miền; invalid range giữ block nguyên.

**Tự chế spell tiếp theo:** Tạo soft-knee toy: vùng gần biên giảm gain trước khi hard clamp.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 5</strong></summary>

Lưu thành `practice/wizard_task_05.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

struct cpx16 { int16_t i; int16_t q; };

static int16_t sat_range(int32_t value, int16_t low, int16_t high)
{
    if (low > high) return low;
    if (value < low) return low;
    if (value > high) return high;
    return (int16_t)value;
}

static void iq_clip_axes(struct cpx16 *x, size_t n,
                         int16_t i_low, int16_t i_high,
                         int16_t q_low, int16_t q_high)
{
    if (x == NULL) return;
    for (size_t k = 0; k < n; ++k) {
        x[k].i = sat_range(x[k].i, i_low, i_high);
        x[k].q = sat_range(x[k].q, q_low, q_high);
    }
}

int main(void)
{
    struct cpx16 x[2] = {{-100, 80}, {30, -90}};
    iq_clip_axes(x, 2U, -50, 50, -20, 20);
    assert(x[0].i == -50 && x[0].q == 20);
    assert(x[1].i == 30 && x[1].q == -20);
    puts("wizard task 05 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_05.c -o wizard_task_05
./wizard_task_05
```

Expected cuối output:

```text
wizard task 05 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_5_END -->

---

#### Forge F05 — Bài 5: saturation thành một họ primitive

Tự code `sat8(int32_t)`, `add_sat16(int16_t,int16_t)` và
`mul_q15(int16_t,int16_t)` có rounding đối xứng do bạn công bố.

<details>
<summary>Đáp án F05</summary>

```c
#include <limits.h>
#include <stdint.h>
int8_t sat8(int32_t x)
{if(x>INT8_MAX)return INT8_MAX;if(x<INT8_MIN)return INT8_MIN;return(int8_t)x;}
int16_t add_sat16(int16_t a,int16_t b)
{int32_t x=(int32_t)a+b;if(x>INT16_MAX)return INT16_MAX;
 if(x<INT16_MIN)return INT16_MIN;return(int16_t)x;}
int16_t mul_q15(int16_t a,int16_t b)
{int32_t p=(int32_t)a*(int32_t)b;
 p+=(p>=0)?(1<<14):-(1<<14);
 p>>=15;if(p>INT16_MAX)return INT16_MAX;if(p<INT16_MIN)return INT16_MIN;
 return(int16_t)p;}
```

Test toàn bộ lân cận biên: `MIN-1` ở accumulator, `MIN`, `MIN+1`, `MAX-1`,
`MAX`, `MAX+1`; Q15 test `0`, gần `+1`, `-1` và hai số âm.

</details>

## Level 4 — Bit, byte, endian và packet

Modem xử lý rất nhiều field không byte-aligned hoặc phải serialize chính xác.

### Bit operations tối thiểu

```c
x & mask
x | mask
x ^ mask
x << n
x >> n
```

Lấy bit 3:

```c
uint32_t bit = (x >> 3U) & 1U;
```

Set bit 3:

```c
x |= (1U << 3U);
```

Clear bit 3:

```c
x &= ~(1U << 3U);
```

### Little-endian serialization

Muốn ghi `uint32_t 0x12345678` thành wire little-endian:

```text
78 56 34 12
```

Đừng làm:

```c
memcpy(buf, &value, 4);
```

nếu wire format bắt buộc portable.

Hãy serialize tường minh:

```c
buf[0] = (uint8_t)(value);
buf[1] = (uint8_t)(value >> 8U);
buf[2] = (uint8_t)(value >> 16U);
buf[3] = (uint8_t)(value >> 24U);
```

### Bài 6 — `put_u32_le` và `get_u32_le`

#### 1. Đề bài nhỏ

Tự viết:

```c
void put_u32_le(uint8_t out[4], uint32_t value);
uint32_t get_u32_le(const uint8_t in[4]);
```

Test round trip với:

```text
0x00000000
0x12345678
0xffffffff
```

#### 2. Tự đoán chương trình cần làm gì

Một hàm tách integer thành đúng 4 byte little-endian; hàm kia ghép 4 byte về integer. Round trip phải trả lại đúng input.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- shift/mask unsigned
- cast từng byte
- endianness và round-trip property

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_06.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Với `0x12345678`, bytes phải là `78 56 34 12`; test round trip ba vector trong đề.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_06.c \
    -o build/task_06
./build/task_06
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

In từng byte bằng hex. Nếu ra `12 34 56 78`, thứ tự shift/index đang là big-endian.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** Wire format / endian.

**Bạn vừa code cái gì trong modem?** Firmware giao tiếp với file IQ, descriptor, IPC và MMIO bằng byte layout xác định. `put/get_u32_le` buộc bạn serialize field bằng quy tắc, không bằng cách memcpy struct host.

**Nó nằm ở đâu?** runtime / IPC / DMA boundary.

```text
INPUT  → integer hoặc byte buffer
KHỐI   → code của Bài 6
OUTPUT → byte sequence little-endian hoặc integer đã parse
```

**Tại sao modem cần nó?** Đề yêu cầu persistent/wire/MMIO field độc lập kích thước pointer host. Đây là bài đầu tiên dạy rằng representation trong RAM của C không đồng nghĩa representation trên wire.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 6 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

```c
#include <stdint.h>

void put_u32_le(uint8_t out[4], uint32_t value)
{
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8U);
    out[2] = (uint8_t)(value >> 16U);
    out[3] = (uint8_t)(value >> 24U);
}

uint32_t get_u32_le(const uint8_t in[4])
{
    return ((uint32_t)in[0]) |
           ((uint32_t)in[1] << 8U) |
           ((uint32_t)in[2] << 16U) |
           ((uint32_t)in[3] << 24U);
}
```

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_6_BEGIN -->
#### 9. Wizard Lab — Tự chế wire-field painter

**Ý nghĩ quái dị:** Nếu muốn thay đúng một field trong buffer descriptor/PDU mà không memcpy struct, mình cần helper byte-level nào?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
int patch_u16_le(uint8_t *buf, size_t len, size_t off, uint16_t value);
int patch_u32_le(uint8_t *buf, size_t len, size_t off, uint32_t value);
void hex_diff(const uint8_t *a, const uint8_t *b, size_t len);
```

1. **Bậc 1:** Patch value tại offset 0, giữa và `len-4`.
2. **Bậc 2:** Patch vượt biên; digest buffer phải giữ nguyên.
3. **Bậc 3:** Tạo baseline descriptor rồi thay từng field, mỗi lần chỉ một mutation.

**Observer bắt buộc:** Hexdump, changed-byte offsets và round-trip getter.

**Invariant:** Chỉ đúng width byte tại offset được phép đổi.

**Tự chế spell tiếp theo:** Viết `patch_bits` cho field không byte-aligned với mask rõ ràng.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 6</strong></summary>

Lưu thành `practice/wizard_task_06.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static int patch_u16_le(uint8_t *buf, size_t len, size_t off, uint16_t value)
{
    if (buf == NULL || off > len || len - off < 2U) return -1;
    buf[off] = (uint8_t)value;
    buf[off + 1U] = (uint8_t)(value >> 8U);
    return 0;
}

static int patch_u32_le(uint8_t *buf, size_t len, size_t off, uint32_t value)
{
    if (buf == NULL || off > len || len - off < 4U) return -1;
    for (size_t k = 0; k < 4U; ++k) buf[off + k] = (uint8_t)(value >> (8U * k));
    return 0;
}

static void hex_diff(const uint8_t *a, const uint8_t *b, size_t len)
{
    if (a == NULL || b == NULL) return;
    for (size_t k = 0; k < len; ++k)
        if (a[k] != b[k]) printf("off=%zu %02x->%02x\n", k, a[k], b[k]);
}

int main(void)
{
    uint8_t before[8] = {0};
    uint8_t after[8] = {0};
    assert(patch_u16_le(after, 8U, 1U, UINT16_C(0x1234)) == 0);
    assert(patch_u32_le(after, 8U, 4U, UINT32_C(0x89abcdef)) == 0);
    assert(patch_u32_le(after, 8U, 6U, 1U) == -1);
    assert(after[1] == 0x34U && after[7] == 0x89U);
    hex_diff(before, after, 8U);
    puts("wizard task 06 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_06.c -o wizard_task_06
./wizard_task_06
```

Expected cuối output:

```text
wizard task 06 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_6_END -->

---

#### Forge F06 — Bài 6: endian codec phải transactional

Tự code:

1. `put_u16_be`/`get_u16_be` để buộc tách endian khỏi host.
2. `put_u64_le`/`get_u64_le` cho tick/IQ count.
3. Cursor writer chỉ advance offset khi đủ capacity.

<details>
<summary>Đáp án F06</summary>

```c
#include <stddef.h>
#include <stdint.h>
struct writer{uint8_t*p;size_t cap,off;};
void put_u16_be(uint8_t out[2],uint16_t v)
{out[0]=(uint8_t)(v>>8);out[1]=(uint8_t)v;}
uint16_t get_u16_be(const uint8_t in[2])
{return(uint16_t)(((uint16_t)in[0]<<8)|(uint16_t)in[1]);}
void put_u64_le(uint8_t out[8],uint64_t v)
{size_t k;for(k=0U;k<8U;++k)out[k]=(uint8_t)(v>>(8U*k));}
uint64_t get_u64_le(const uint8_t in[8])
{size_t k;uint64_t v=0U;for(k=0U;k<8U;++k)v|=(uint64_t)in[k]<<(8U*k);return v;}
int writer_u16_be(struct writer*w,uint16_t v)
{if((w==NULL)||(w->p==NULL)||(w->off>w->cap)||((w->cap-w->off)<2U))return-1;
 put_u16_be(&w->p[w->off],v);w->off+=2U;return 0;}
```

Fixture bắt buộc: `0x0123456789ABCDEF` thành `EF CD AB 89 67 45 23 01` ở LE.

</details>

# PHẦN I-B — TỪ ĐIỂN KIỂU DỮ LIỆU C DÙNG TRONG MODEM

Phần này tồn tại vì source modem không chỉ có `int`, `char` và pointer đơn giản.
Bạn sẽ gặp những dòng như:

```c
int bb_modem_init(void *arena, size_t arena_len,
                  const struct bb_config *cfg,
                  const struct bb_platform_ops *plat,
                  void *plat_ctx,
                  struct bb_modem **out);
```

Nếu đọc cả dòng một lần, nó giống mật mã. Ta sẽ tách từng kiểu và học cách dùng
trước khi quay lại prototype này.

## 1B.1. Kiểu dữ liệu là hợp đồng, không chỉ là “cái hộp”

Khi chọn kiểu, bạn đang trả lời:

```text
Giá trị là số hay địa chỉ?
Có thể âm không?
Cần chính xác bao nhiêu bit?
Đây là byte trên wire hay số để tính toán?
Caller có được sửa object không?
Pointer có thể NULL không?
Ai sở hữu object?
Object sống bao lâu?
Có được truy cập giữa IRQ/task không?
Có yêu cầu alignment/cache line không?
```

Ví dụ:

```c
uint16_t generation;
```

không chỉ nghĩa là “một số 16-bit”. Trong buffer pool, nó nói:

```text
không âm
miền 0..65535
được dùng làm identity version
wire/handle cần kích thước ổn định
wraparound phải được nghĩ tới
```

## 1B.2. Cheat sheet dùng xuyên suốt project

| Kiểu | Dùng cho | Ví dụ modem | Không nên dùng cho |
|---|---|---|---|
| `uint8_t` | byte, bit field đã mask, ID nhỏ | PDU type, LCID, modulation | length lớn, phép nhân trung gian |
| `int8_t` | số nhỏ có dấu | hiếm trong project này | IQ sample chính |
| `uint16_t` | ID/slot/SN nhỏ | pool slot, generation, RLC SN | byte count có thể vượt 65535 |
| `int16_t` | fixed-point/sample có dấu | I/Q, LLR, oscillator step | tích correlation lớn |
| `uint32_t` | count/register/CRC/address vật lý 32-bit | sample rate, MMIO value, length bounded | host pointer |
| `int32_t` | trung gian signed/status/value có thể âm | CFO Hz, DSP product, trace value | tick dài hạn |
| `uint64_t` | tick/counter/file count dài | virtual time, IQ count, stats | array index nhỏ nếu không cần |
| `int64_t` | accumulator signed lớn | correlation, phase/CFO math | storage sample |
| `size_t` | kích thước object trong memory của C | buffer length, capacity, loop index | field wire có width cố định |
| `ptrdiff_t` | hiệu hai pointer cùng mảng | khoảng cách pointer | file/API POSIX result |
| `ssize_t` | host POSIX: byte count hoặc `-1` | kết quả `read()` | firmware ISO C freestanding |
| `uintptr_t` | integer đủ chứa địa chỉ | MMIO address | độ dài buffer |
| `bool` | đúng/sai trong context local | `security_active`, `valid` | field wire nếu layout yêu cầu `u8` |
| `enum` | tập state/event có tên | RRC/NAS/DMA state | serialize trực tiếp lên wire |
| `struct` | gom object có quan hệ | IQ block, event, handle | wire format bằng `memcpy` |
| `void *` | địa chỉ chưa gắn kiểu cụ thể | arena, callback context | arithmetic trực tiếp |

Quy tắc ngắn:

```text
wire/register/protocol field → fixed-width integer
memory size/capacity/index   → size_t
địa chỉ chuyển qua integer   → uintptr_t
POSIX byte result            → ssize_t
sample signed                → int16_t
DSP intermediate             → int32_t/int64_t
clock/counter dài            → uint64_t
```

## 1B.2A. Vậy `int`, `char`, `float`, `double` dùng lúc nào?

### `int`

`int` vẫn hữu ích cho:

```text
return status 0/-1
file descriptor trên host
giá trị local không đi lên wire và range nhỏ, rõ
```

Ví dụ:

```c
int rc = bb_phy_receive(...);
if (rc != 0) {
    /* failure */
}
```

Không dùng `int` khi width là contract:

```c
int crc;          /* không rõ wire width */
int sample_rate;  /* không rõ signedness/width */
```

### `char`

`char` dùng cho text/C string:

```c
const char *checkpoint = "TB_CRC";
char line[256];
```

Đừng dùng plain `char` cho binary sample/PDU khi cần signedness rõ, vì `char` có
thể signed hoặc unsigned tùy target/compiler.

```text
text       → char
binary byte → uint8_t
signed 8-bit numeric → int8_t
```

### `float` và `double`

Host reference/test có thể dùng `double` để tạo oracle dễ hiểu:

```c
double angle = 2.0 * pi * k / n;
```

Nhưng firmware Cortex-R5 profile trong đề dùng soft-float và fast path cần
deterministic/bounded behavior, nên DSP chính dùng fixed-point:

```text
host DFT/reference → double có thể chấp nhận
firmware FFT/NCO   → int16_t Q15 + int32_t intermediate
```

Không phải floating-point “sai”. Nó chỉ có trade-off về hardware, latency,
determinism, ABI và power.

## 1B.2B. Cùng C source nhưng size có thể khác giữa host và Arm

Một data model thường gặp:

| Type | x86-64 Linux thường gặp | Armv7-R 32-bit thường gặp |
|---|---:|---:|
| `char` | 1 byte | 1 byte |
| `short` | 2 | 2 |
| `int` | 4 | 4 |
| `long` | 8 | 4 |
| pointer | 8 | 4 |
| `size_t` | 8 | 4 |
| `uintptr_t` | 8 | 4 |
| `uint32_t` | 4 | 4 |
| `uint64_t` | 8 | 8 |

Đây là lý do project yêu cầu:

```text
persistent/wire/MMIO field → uint*_t cố định
pointer/memory size        → type native pointer/size
```

Tự quan sát trên host:

```c
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    printf("sizeof(int)=%zu\n", sizeof(int));
    printf("sizeof(long)=%zu\n", sizeof(long));
    printf("sizeof(void*)=%zu\n", sizeof(void *));
    printf("sizeof(size_t)=%zu\n", sizeof(size_t));
    printf("sizeof(uint32_t)=%zu\n", sizeof(uint32_t));
    printf("sizeof(uint64_t)=%zu\n", sizeof(uint64_t));
    return 0;
}
```

Compile host rồi cross-compile/inspect target. Không hard-code giả định
`sizeof(pointer)==4` vào source dùng chung.

## 1B.3. `uint8_t`: byte thật, không phải “số nào cũng được”

```c
uint8_t byte = 0x41U;
uint8_t pdu_type = 7U;
uint8_t harq_id = 2U;
```

Miền giá trị:

```text
0 .. 255
```

Nó phù hợp với:

- byte trong packet;
- message ID;
- flag sau khi mask;
- modulation selector;
- mảng raw data.

Ví dụ buffer protocol:

```c
uint8_t frame[512];
```

nghĩa là 512 byte, không phải 512 số nguyên chung chung.

### Bẫy integer promotion

Trong biểu thức, `uint8_t` thường được promote thành `int`:

```c
uint8_t a = 200U;
uint8_t b = 100U;
int sum = a + b;        /* sum là 300 */
uint8_t wrapped = a + b; /* thu hẹp về 44 */
```

Vì vậy đừng dùng `uint8_t` làm accumulator.

```c
uint32_t sum = (uint32_t)a + (uint32_t)b;
```

### Modem concept

`uint8_t` là representation tự nhiên của packet/wire. Nhưng ngay khi tính toán,
ta thường widen sang `uint32_t` hoặc `int32_t`.

## 1B.4. `int16_t`: tại sao IQ có dấu?

```c
struct bb_cpx16 {
    int16_t i;
    int16_t q;
};
```

Miền:

```text
-32768 .. 32767
```

I/Q phải có dấu vì vector complex có thể nằm ở cả bốn quadrant:

```text
I+,Q+   I+,Q-
I-,Q+   I-,Q-
```

### Không nhân trực tiếp rồi nhét lại `int16_t`

Sai về tư duy:

```c
int16_t product = a.i * b.i;
```

Đúng hơn:

```c
int32_t product = (int32_t)a.i * (int32_t)b.i;
```

Sau khi scale/shift mới saturation về `int16_t`:

```c
int16_t result = bb_sat16(product >> 15);
```

Data flow kiểu:

```text
int16 storage
→ int32 computation
→ scale/saturation
→ int16 storage
```

Đây là pattern fixed-point xuất hiện trong FFT, NCO và equalizer.

## 1B.5. `uint16_t`: slot, generation và sequence number

Ví dụ:

```c
struct bb_buf_handle {
    uint16_t slot;
    uint16_t generation;
    uint32_t length;
};
```

Tại sao không dùng `int`?

```text
slot/generation không âm
handle cần layout ổn định
16 bit là đủ cho pool bounded
```

RLC SN cũng dùng miền modulo:

```c
uint16_t rlc_sn;
```

nhưng invariant thật là:

```text
0 .. 4095
```

Type chỉ giới hạn tới 65535; code vẫn phải check:

```c
if (rlc_sn >= 4096U) {
    return -1;
}
```

**Bài học:** type không thay thế validation theo protocol.

## 1B.6. `uint32_t` và `int32_t`: cùng 32 bit nhưng khác ý nghĩa

Không âm:

```c
uint32_t sample_rate_hz = 7680000U;
uint32_t sample_count = 1024U;
uint32_t crc = bb_crc24a(data, length);
```

Có thể âm:

```c
int32_t cfo_hz = -1800;
int32_t trace_value = -1;
int32_t product = (int32_t)i * (int32_t)gain;
```

Sai lầm nguy hiểm:

```c
uint32_t cfo_hz = -1800;
```

Giá trị sẽ biến thành một số unsigned rất lớn.

### Quy tắc literal

Khi so với unsigned, dùng suffix `U`:

```c
if (count > 8192U) {
    return -1;
}
```

Nhưng đừng thêm `U` mù quáng cho số cần âm.

## 1B.7. `uint64_t` và `int64_t`: thời gian dài và accumulator lớn

Virtual tick:

```c
uint64_t now;
uint64_t deadline;
```

Stats:

```c
uint64_t rx_ok;
uint64_t dropped;
```

Correlation:

```c
int64_t correlation_i = 0;
int64_t correlation_q = 0;
```

Tại sao correlation cần signed 64-bit?

```text
mỗi tích int16 × int16 có thể gần 2^30
cộng hàng trăm sample có thể vượt int32
kết quả có thể âm
```

Constant 64-bit rõ ràng:

```c
uint64_t timeout = UINT64_C(1500);
```

## 1B.8. `size_t`: kiểu đúng cho size, capacity và index memory

```c
size_t length;
size_t capacity;
size_t index;
```

`size_t` là kiểu unsigned đủ biểu diễn kích thước object lớn nhất mà C có thể
quản lý trên target.

Ví dụ:

```c
int parse(const uint8_t *input, size_t length);
```

nghĩa là:

```text
input  → địa chỉ byte đầu
length → số byte hợp lệ kể từ input
```

### Tại sao không dùng `uint32_t` cho mọi length?

Host x86-64 thường có `size_t` 64-bit; Arm 32-bit thường có `size_t` 32-bit.
Memory API phải dùng đúng kiểu native của target.

Wire field vẫn dùng fixed width:

```c
uint32_t total_len_wire;
```

Sau validate mới convert:

```c
if ((uint64_t)total_len_wire > (uint64_t)capacity) {
    return -1;
}

size_t total_len = (size_t)total_len_wire;
```

### Bẫy vòng lặp đi ngược

Sai vì `size_t` không âm:

```c
for (size_t i = count - 1U; i >= 0U; --i) {
    /* loop không dừng đúng */
}
```

Cách an toàn:

```c
for (size_t i = count; i-- > 0U; ) {
    /* i đi count-1 ... 0 */
}
```

hoặc dùng `ptrdiff_t`/kiểu signed khi thực sự cần chỉ số âm và đã kiểm range.

## 1B.9. `ssize_t`: chỉ dùng ở host POSIX

`read()` trả về:

```c
ssize_t read(int fd, void *buf, size_t count);
```

Vì kết quả cần biểu diễn:

```text
> 0  số byte đọc được
0    EOF
-1   lỗi
```

`size_t` không biểu diễn `-1`, nên POSIX dùng `ssize_t`.

```c
ssize_t n = read(fd, buffer, sizeof buffer);
if (n < 0) {
    /* error */
} else if (n == 0) {
    /* EOF */
} else {
    size_t received = (size_t)n; /* cast sau khi đã chứng minh n > 0 */
}
```

Firmware freestanding không nên phụ thuộc `ssize_t`, vì đây không phải kiểu ISO C
chuẩn. Host runner có thể dùng nó.

## 1B.10. `uintptr_t`: integer dành cho địa chỉ

MMIO API:

```c
uint32_t bb_mmio_read32(uintptr_t address);
```

`uintptr_t` là unsigned integer có thể chứa giá trị pointer mà không bị mất bit
trên implementation có hỗ trợ nó.

```c
uintptr_t base = UINT32_C(0x60002000);
uintptr_t status = base + UINT32_C(0x04);
```

Sau đó accessor có thể đổi sang pointer:

```c
volatile uint32_t *reg = (volatile uint32_t *)address;
```

Không dùng:

```c
uint32_t pointer_value = (uint32_t)ptr;
```

trên host 64-bit vì có thể cắt mất nửa địa chỉ.

### Phân biệt ba thứ

```text
uint32_t  → register value 32-bit
size_t    → kích thước vùng memory
uintptr_t → địa chỉ biểu diễn dưới dạng integer
```

## 1B.11. `bool`: state rõ hơn magic number

```c
bool security_active;
bool pending_rx_valid;
bool supervisor_fault;
```

Gán:

```c
security_active = true;
security_active = false;
```

Check:

```c
if (!security_active) {
    return -1;
}
```

Không nên dùng một `bool` cho state có ba giá trị:

```text
IDLE
SUBMITTED
COMPLETED
```

Trường hợp đó dùng `enum`.

Wire format nên encode `0/1` bằng `uint8_t` rõ ràng thay vì serialize trực tiếp
`bool`, vì kích thước/layout của `bool` không phải contract wire của bạn.

## 1B.12. `enum`: đặt tên cho state, nhưng đừng serialize thẳng

```c
enum bb_dma_state {
    BB_DMA_FREE = 0,
    BB_DMA_CPU_OWNED,
    BB_DMA_DMA_OWNED,
    BB_DMA_DONE
};
```

So với magic integer:

```c
slot->state = 2; /* 2 là gì? */
```

enum rõ hơn:

```c
slot->state = BB_DMA_DMA_OWNED;
```

Nhưng kích thước underlying của enum là chuyện của compiler/ABI. Trên wire:

```c
output[0] = (uint8_t)state;
```

và phải validate khi decode:

```c
if (input[0] > (uint8_t)BB_DMA_DONE) {
    return -1;
}
```

## 1B.13. `struct`: object có ý nghĩa, không phải wire packet tự động

```c
struct bb_event {
    uint64_t tick;
    uint16_t source;
    uint16_t type;
    struct bb_buf_handle payload;
};
```

Struct này gom:

```text
khi nào event xảy ra
ai phát event
event loại gì
payload object nào
```

### Dùng `.` và `->`

Object trực tiếp:

```c
struct bb_event ev;
ev.tick = 100U;
```

Pointer tới object:

```c
struct bb_event *p = &ev;
p->tick = 100U;
```

Hai dòng tương đương về ý nghĩa:

```c
p->tick
(*p).tick
```

### Padding

Compiler có thể chèn padding để alignment. Vì vậy cấm:

```c
memcpy(wire, &ev, sizeof ev);
```

nếu wire format yêu cầu layout cố định.

Hãy encode từng field bằng `put_u16/put_u32/put_u64`.

## 1B.14. Array: storage thật, nhưng khi truyền hàm thường thành pointer

```c
struct bb_cpx16 samples[256];
```

Đây là storage cho đúng 256 object.

Trong cùng scope:

```c
size_t count = sizeof samples / sizeof samples[0];
```

Nhưng parameter:

```c
void process(struct bb_cpx16 samples[256]);
```

trong function gần như được điều chỉnh thành pointer:

```c
void process(struct bb_cpx16 *samples);
```

Function không tự biết có 256 phần tử. Vì vậy API phải nhận `count`:

```c
void process(struct bb_cpx16 *samples, size_t count);
```

### Fixed array nằm trong struct

```c
struct bb_pool_slot {
    uint8_t data[1024];
};
```

Storage nằm ngay trong mỗi slot, không phải pointer tới vùng khác. Điều này giúp
firmware no-heap và lifetime rõ.

## 1B.15. Pointer cơ bản: địa chỉ + type của object được trỏ tới

```c
uint8_t *data;
```

Đọc:

```text
data là pointer tới uint8_t
```

```c
struct bb_cpx16 *samples;
```

Đọc:

```text
samples là pointer tới bb_cpx16 đầu tiên
```

Pointer không chứa:

```text
length
capacity
ownership
lifetime
```

Những thứ đó phải nằm trong contract hoặc type đi kèm.

## 1B.16. Bốn dạng `const` phải đọc được

### Pointer tới data read-only qua pointer đó

```c
const uint8_t *input;
```

Bạn được đổi `input` trỏ nơi khác, nhưng không được viết `input[0]` qua pointer
này.

```c
input = another_buffer; /* được */
input[0] = 1U;          /* không được */
```

### Pointer const tới data có thể sửa

```c
uint8_t * const output = buffer;
```

Bạn được sửa data, nhưng không đổi pointer:

```c
output[0] = 1U;       /* được */
output = other;       /* không được */
```

### Cả pointer và data đều const

```c
const uint8_t * const fixed_input = buffer;
```

Không đổi pointer, không sửa data qua pointer.

### Trong prototype modem

```c
int bb_phy_transmit(const struct bb_config *cfg,
                    const uint8_t *tb,
                    size_t tb_len,
                    struct bb_iq_block *out);
```

Đọc contract:

```text
cfg     input, function không sửa config
tb      input bytes, function không sửa payload
tb_len  số byte hợp lệ
out     output object, function được sửa
```

`const` không tự nói object sống bao lâu. Lifetime vẫn phải được thiết kế.

## 1B.17. Out-parameter: function trả nhiều kết quả

```c
int decode(const uint8_t *input, size_t input_len,
           uint8_t *output, size_t capacity,
           size_t *output_len);
```

Return `int` dùng cho status:

```text
0  success
-1 failure
```

Data trả qua pointer:

```text
output      buffer do caller cấp
capacity    số byte có thể ghi
output_len  nơi function ghi số byte thực tế
```

Caller:

```c
uint8_t output[512];
size_t output_len = 0U;

int rc = decode(input, input_len,
                output, sizeof output,
                &output_len);
```

`&output_len` nghĩa là đưa địa chỉ biến cho function sửa.

## 1B.18. Double pointer `T **out`: trả về một pointer

```c
struct bb_modem **out;
```

Đọc từ tên biến ra ngoài:

```text
out
→ là pointer
→ tới một pointer
→ pointer trong đó trỏ tới struct bb_modem
```

Caller:

```c
struct bb_modem *modem = NULL;

int rc = bb_modem_init(arena, sizeof arena,
                       &cfg, &ops, &platform,
                       &modem);
```

Memory picture:

```text
out ──────► biến modem ──────► object struct bb_modem trong arena
```

Tại sao không return pointer trực tiếp?

Vì function cần trả riêng:

```text
status lỗi/thành công
pointer kết quả
```

Quy tắc an toàn:

```c
if (out == NULL) {
    return -1;
}

*out = NULL;
/* validate mọi thứ, tạo object */
*out = modem;
return 0;
```

Đặt `*out = NULL` sớm giúp caller không giữ kết quả rác khi init fail.

## 1B.19. `void *`: pointer chưa biết type, không phải pointer ma thuật

Arena:

```c
void *arena;
```

Callback context:

```c
void *ctx;
```

`void *` có thể giữ địa chỉ object bất kỳ, nhưng phải cast về đúng type trước
khi dereference:

```c
struct host_platform *platform = ctx;
platform->tick += 1U;
```

### Không arithmetic trực tiếp trên `void *` trong ISO C

Đổi sang byte pointer:

```c
uint8_t *bytes = arena;
void *object = &bytes[offset];
```

### `ctx` giải quyết vấn đề gì?

Function pointer không giữ state. Ta truyền thêm context:

```text
callback code + ctx object = một backend có state
```

## 1B.20. Function pointer: đọc từ tên biến ra ngoài

```c
uint64_t (*ticks)(void *ctx);
```

Đọc:

```text
ticks
→ là pointer tới function
→ function nhận void *ctx
→ trả uint64_t
```

Ví dụ implementation:

```c
struct fake_clock {
    uint64_t now;
};

static uint64_t fake_ticks(void *ctx)
{
    struct fake_clock *clock = ctx;
    return clock->now;
}
```

Gắn callback:

```c
struct bb_platform_ops ops = {
    .ticks = fake_ticks
};
```

Gọi:

```c
uint64_t now = ops.ticks(&clock);
```

Function pointer phức tạp hơn:

```c
int (*tx_submit)(void *ctx,
                 const struct bb_iq_block *block);
```

Contract:

```text
callback trả status int
nhận backend state qua ctx
nhận IQ block read-only
```

Đây là cách cùng firmware core chạy được với host virtual RF hoặc backend target.

## 1B.21. Opaque/incomplete struct: cố ý giấu implementation

Header public chỉ khai báo:

```c
struct bb_modem;
struct bb_soc;
```

Đây là incomplete type. Caller được phép giữ pointer:

```c
struct bb_modem *modem;
```

nhưng không được:

```c
sizeof(struct bb_modem)
modem->internal_field
```

vì layout chỉ tồn tại trong private header/source.

Lợi ích:

```text
giữ encapsulation
caller không phá state nội bộ
implementation có thể thay đổi mà public API giữ nguyên
```

API `required_memory()` + `init(arena,...,**out)` cho caller biết cần bao nhiêu
memory mà không phải thấy layout private.

## 1B.22. Handle khác pointer như thế nào?

```c
struct bb_buf_handle {
    uint16_t slot;
    uint16_t generation;
    uint32_t length;
};
```

Pointer:

```text
địa chỉ trực tiếp
chỉ an toàn khi lifetime và owner còn đúng
```

Handle:

```text
identity gián tiếp
pool kiểm slot + generation trước khi trả pointer
có thể reject stale reference
```

Flow:

```text
task A alloc → nhận handle
task A enqueue handle
task B dequeue handle
pool_get(handle) → pointer tạm thời
task B xử lý
task B release(handle)
```

Không gửi raw pointer qua wire/snapshot/AP↔CP.

## 1B.23. `_Atomic uint32_t`: type có semantics đồng bộ

```c
_Atomic uint32_t write_index;
_Atomic uint32_t read_index;
```

Đây không chỉ là `uint32_t` thường. Access nên dùng atomic operation:

```c
uint32_t read = atomic_load_explicit(&ring->read_index,
                                     memory_order_acquire);

atomic_store_explicit(&ring->write_index,
                      next,
                      memory_order_release);
```

Mental model SPSC:

```text
producer fill entry
→ release-store write_index
→ consumer acquire-load write_index
→ consumer mới được đọc entry đã publish
```

Atomic giải quyết data race và ordering giữa execution contexts. Nó không tự sửa
logic full/empty sai.

## 1B.24. `volatile`: dùng cho MMIO, không thay atomic

MMIO:

```c
volatile uint32_t *reg = (volatile uint32_t *)address;
uint32_t value = *reg;
```

`volatile` nói với compiler rằng mỗi read/write là observable và không được tự
ý bỏ đi như memory thường.

Nó **không bảo đảm**:

```text
atomicity
thread synchronization
memory ordering giữa cores
race freedom
```

Sai:

```c
volatile uint32_t queue_index; /* không biến queue thành thread-safe */
```

Đúng vai trò:

```text
MMIO accessor       → volatile
IRQ/task shared ring → C11 atomic
DMA ownership        → atomic fence + cache operations
```

## 1B.25. `_Alignas(64)`: ép storage theo cache line/DMA requirement

```c
_Alignas(64) uint8_t descriptor[32];
_Alignas(64) struct bb_cpx16 samples[8192];
```

Nó yêu cầu địa chỉ object chia hết cho 64.

Tại sao cần?

```text
cache line = 64 byte
DMA descriptor publish theo cache line
tránh descriptor chia đôi qua hai cache line
đáp ứng hardware alignment
```

Kiểm tra host:

```c
#include <stdint.h>
#include <assert.h>

assert(((uintptr_t)&object % 64U) == 0U);
```

Alignment không tự làm cache coherent. Vẫn cần clean/invalidate và ownership
fence.

## 1B.26. `static`, `extern`, `typedef` không phải cùng loại vấn đề

### `static` function trong source

```c
static uint8_t get_bit(...);
```

Function chỉ visible trong translation unit hiện tại. Đây là linkage, không phải
kiểu dữ liệu mới.

### `extern`

```c
extern const uint8_t test_key[16];
```

Object được định nghĩa ở translation unit khác.

### `typedef`

```c
typedef uint16_t bb_slot_t;
```

Tạo tên mới cho type hiện có. Nó không tự tạo type runtime mới.

Project dùng `struct bb_event` trực tiếp khá nhiều để người đọc thấy rõ object
kind. Đừng thêm `typedef` chỉ để che mọi `struct` nếu không làm meaning rõ hơn.

## 1B.27. Cách đọc một prototype dài từ trái sang phải theo vai trò

Quay lại:

```c
int bb_modem_init(void *arena, size_t arena_len,
                  const struct bb_config *cfg,
                  const struct bb_platform_ops *plat,
                  void *plat_ctx,
                  struct bb_modem **out);
```

Tách:

| Thành phần | Nghĩa |
|---|---|
| `int` | return status |
| `void *arena` | vùng memory caller cấp |
| `size_t arena_len` | số byte hợp lệ của arena |
| `const struct bb_config *cfg` | config input read-only |
| `const struct bb_platform_ops *plat` | bảng callback read-only |
| `void *plat_ctx` | state đưa lại cho callback |
| `struct bb_modem **out` | nơi trả pointer modem đã init |

Data/ownership flow:

```text
caller owns arena
→ init đặt modem object vào arena
→ out nhận pointer tới object đó
→ modem giữ bản copy/config callback theo contract
→ caller phải giữ arena sống lâu hơn modem
```

## 1B.28. Cách đọc declaration khó bằng quy tắc “bắt đầu từ tên”

Ví dụ:

```c
const struct bb_iq_block *block;
```

```text
block → pointer → tới bb_iq_block const
```

```c
void (*rx_release)(void *ctx, struct bb_iq_block *block);
```

```text
rx_release
→ pointer tới function
→ nhận ctx và pointer IQ block có thể sửa
→ không trả value
```

```c
struct bb_cpx16 (*buffers)[256];
```

```text
buffers
→ pointer
→ tới array 256 bb_cpx16
```

Khác với:

```c
struct bb_cpx16 *buffers[256];
```

```text
buffers
→ array 256 phần tử
→ mỗi phần tử là pointer tới bb_cpx16
```

Dấu ngoặc thay đổi hoàn toàn type.

## 1B.29. Safe conversion: cast chỉ sau khi đã chứng minh range

Sai:

```c
uint16_t small = (uint16_t)length; /* warning biến mất nhưng có thể truncate */
```

Đúng:

```c
if (length > UINT16_MAX) {
    return -1;
}

uint16_t small = (uint16_t)length;
```

Signed sang unsigned:

```c
if (result < 0) {
    return -1;
}

size_t length = (size_t)result;
```

Pointer/integer:

```c
uintptr_t address = (uintptr_t)ptr;
```

Không cast chỉ để làm `-Wconversion` im lặng.

## 1B.30. Kiểm tra cộng/nhân size trước allocation/copy

Nguy hiểm:

```c
size_t bytes = count * sizeof(struct bb_cpx16);
```

Nếu `count` từ input và quá lớn, phép nhân có thể wrap.

Check:

```c
if (count > SIZE_MAX / sizeof(struct bb_cpx16)) {
    return -1;
}

size_t bytes = count * sizeof(struct bb_cpx16);
```

Check cộng:

```c
if (payload_len > SIZE_MAX - header_len) {
    return -1;
}

size_t total = header_len + payload_len;
```

Đây là lý do kiểu dữ liệu liên quan trực tiếp tới defensive engineering.

## 1B.31. In số đúng kiểu

Cách portable với `<inttypes.h>`:

```c
#include <inttypes.h>

uint32_t count = 100U;
uint64_t tick = UINT64_C(5000);

printf("count=%" PRIu32 " tick=%" PRIu64 "\n",
       count, tick);
```

`size_t` dùng `%zu`:

```c
printf("length=%zu\n", length);
```

`uintptr_t`:

```c
printf("address=0x%" PRIxPTR "\n", address);
```

Trong bài đầu có thể cast `int16_t` sang `int` để dùng `%d`, nhưng khi code
project nên học format macro đúng.

### 1B.31A. Bản tổng hợp sau các Syntax Lens — format specifier là một phần của type contract

Phần này không còn là lần đầu syntax xuất hiện. `%d`, `%u`, `%zu`, `PRId32` và
`PRIu64` đã được giải thích ngay dưới MINI CODE/Bài đầu tiên dùng chúng. Đây là
bảng hợp nhất để audit source lớn và làm gate trước parser/runtime; người đọc
không cần quay về đây khi đang làm bài trước đó.

`printf` là variadic function. Sau format string, compiler/runtime không nhận
thêm metadata nói argument thật có type gì. Nếu format đòi một type nhưng bạn
truyền type khác, đây không chỉ là “in hơi sai”; hành vi có thể là undefined.

```c
size_t length = 100U;
printf("%u\n", length); /* sai: %u đòi unsigned int */
printf("%zu\n", length); /* đúng */
```

Trên một máy hai type có thể tình cờ cùng width và dòng sai có vẻ chạy được.
Sang ABI khác, warning khác hoặc giá trị lớn hơn, bug mới lộ. Modem source dùng
chung cho host và Arm không được dựa vào may mắn đó.

| Giá trị | Header | Cách in portable | Ghi chú |
|---|---|---|---|
| `int` | `<stdio.h>` | `%d`, `%i`, `%x` sau khi logic đúng signedness | status/local integer |
| `unsigned int` | `<stdio.h>` | `%u`, `%x` | không thay cho mọi unsigned type |
| `size_t` | `<stddef.h>` | `%zu`, `%zx` | `sizeof` cũng trả `size_t` |
| `ptrdiff_t` | `<stddef.h>` | `%td`, `%tx` | hiệu hai pointer trong cùng array |
| `intmax_t` | `<stdint.h>` | `%jd` | integer signed rộng nhất |
| `uintmax_t` | `<stdint.h>` | `%ju`, `%jx` | integer unsigned rộng nhất |
| `uint8_t` | `<inttypes.h>` | `"%" PRIu8` | macro ghép với string literal |
| `int16_t` | `<inttypes.h>` | `"%" PRId16` | sample/LLR có dấu |
| `uint32_t` | `<inttypes.h>` | `"%" PRIu32`, `"%08" PRIx32` | count/register/CRC |
| `int32_t` | `<inttypes.h>` | `"%" PRId32` | CFO/DSP intermediate |
| `uint64_t` | `<inttypes.h>` | `"%" PRIu64`, `"%016" PRIx64` | tick/counter |
| `uintptr_t` | `<inttypes.h>` | `"%" PRIuPTR`, `"%" PRIxPTR` | integer address |
| pointer | `<stdio.h>` | `%p` với `(void *)pointer` | chỉ debug địa chỉ, không phải wire |
| `double` | `<stdio.h>` | `%f`, `%e`, `%g` | `float` được promote thành `double` |

Cú pháp macro trông lạ nhưng chỉ là string-literal concatenation của C:

```c
printf("tick=%" PRIu64 " crc=0x%06" PRIx32 "\n", tick, crc);
```

Compiler nhìn nó giống:

```c
printf("tick=%llu crc=0x%06x\n", tick, crc); /* ví dụ expansion trên một ABI */
```

Bạn không được tự hard-code expansion ví dụ. `<inttypes.h>` chọn expansion đúng
cho implementation hiện tại.

### 1B.31B. Vì sao không đoán `%lu` hoặc `%llu` cho `uint64_t`?

Data model khác nhau:

```text
LP64  (Linux x86-64): long và pointer thường 64 bit
LLP64 (Windows 64-bit): long 32 bit, long long và pointer 64 bit
ILP32 (Armv7-R): int, long và pointer thường 32 bit
```

`uint64_t` có thể là alias của `unsigned long` hoặc `unsigned long long`. Vì thế:

```c
printf("%lu", value);  /* chỉ đúng nếu value thật là unsigned long */
printf("%llu", value); /* chỉ đúng nếu value thật là unsigned long long */
printf("%" PRIu64, value); /* đúng theo implementation */
```

Tương tự, `size_t` không đồng nghĩa với `unsigned long`; `%zu` mới biểu diễn
contract của chính `size_t`.

### 1B.31C. Default argument promotions phải hiểu

Trong lời gọi variadic như `printf`:

```text
char, signed char, unsigned char, short, unsigned short
→ integer promotions (thường thành int, đôi khi unsigned int)

float → double
```

Do đó đoạn nhập môn này hợp lệ khi `int` chứa được toàn miền `int16_t`:

```c
int16_t sample = -123;
printf("%d\n", (int)sample);
```

Nhưng promotion không biến `size_t` thành `unsigned int`, cũng không biến
`uint64_t` thành type mà `%u` đòi. Đừng dùng “mọi số nguyên đều là số” làm mental
model cho variadic API.

Width và precision động cũng có type riêng:

```c
int width = 8;
size_t length = 12U;
printf("%*zu\n", width, length); /* width là int, value là size_t */
```

Không truyền `size_t width` trực tiếp cho `*`; `printf` đọc dynamic width như
`int`. Nếu width đến từ input, range-check rồi convert.

### 1B.31D. `scanf` không dùng y hệt `printf`

Với input, argument là pointer và macro dùng tiền tố `SCN`:

```c
#include <inttypes.h>

uint64_t tick = 0U;
size_t length = 0U;

if (scanf("%" SCNu64 " %zu", &tick, &length) != 2) {
    /* reject input */
}
```

Trong code phòng thủ, thường nên đọc cả line bằng `fgets`, rồi parse có bound
với `strto*`/parser riêng. Ví dụ trên chỉ để hiểu type contract; không phải giấy
phép dùng `scanf` cho mọi config/PDU.

### 1B.31E. Checkpoint bắt buộc — type/format laboratory

Không sang bài pointer/parser nếu chưa làm đủ ba biến thể này:

1. **Biến thể A — record modem:** viết `format_record` in `size_t`,
   `ptrdiff_t`, `uint64_t`, `int32_t`, `uint16_t`, `uintptr_t` và `double`.
2. **Biến thể B — boundary:** test `SIZE_MAX`, `UINT64_MAX`, `INT32_MIN`, địa
   chỉ zero và buffer output thiếu đúng một byte.
3. **Biến thể C — bug injection:** cố thay `%zu` bằng `%u`, `PRIu64` bằng `%lu`
   và `%td` bằng `%zu`; build phải đỏ với `-Wformat=2 -Werror`. Sau đó sửa lại.

Contract của `format_record`:

```text
success → return 0, output là một C string canonical
failure → return -1, output của caller không đổi
không dùng global, không allocation, không cast để bịt warning format
```

<details>
<summary>Đáp án kề bên — <code>type_format_lab.c</code> đầy đủ</summary>

```c
#include <assert.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct modem_record {
    size_t length;
    ptrdiff_t delta;
    uint64_t tick;
    int32_t cfo_hz;
    uint16_t rlc_sn;
    uintptr_t address;
    double snr_db;
};

int format_record(char *out, size_t capacity,
                  const struct modem_record *record);

int format_record(char *out, size_t capacity,
                  const struct modem_record *record)
{
    char temporary[256];
    int written;

    if ((out == NULL) || (record == NULL) || (capacity == 0U)) {
        return -1;
    }
    written = snprintf(temporary, sizeof temporary,
                       "len=%zu delta=%td tick=%" PRIu64
                       " cfo=%" PRId32 " sn=%" PRIu16
                       " addr=0x%" PRIxPTR " snr=%.2f",
                       record->length,
                       record->delta,
                       record->tick,
                       record->cfo_hz,
                       record->rlc_sn,
                       record->address,
                       record->snr_db);
    if ((written < 0) || ((size_t)written >= sizeof temporary) ||
        ((size_t)written >= capacity)) {
        return -1;
    }
    (void)memcpy(out, temporary, (size_t)written + 1U);
    return 0;
}

static void test_normal(void)
{
    const struct modem_record record = {
        4096U, -3, UINT64_C(5000), INT32_C(-1800),
        UINT16_C(4095), (uintptr_t)UINT32_C(0x60002000), 12.25
    };
    char output[256] = "unchanged";

    assert(format_record(output, sizeof output, &record) == 0);
    assert(strcmp(output,
                  "len=4096 delta=-3 tick=5000 cfo=-1800 sn=4095 "
                  "addr=0x60002000 snr=12.25") == 0);
}

static void test_failure_is_transactional(void)
{
    const struct modem_record record = {
        1U, 0, UINT64_C(2), INT32_C(3), UINT16_C(4),
        (uintptr_t)0U, 5.0
    };
    char output[8] = "KEEP";

    assert(format_record(output, sizeof output, &record) == -1);
    assert(strcmp(output, "KEEP") == 0);
    assert(format_record(NULL, 0U, &record) == -1);
    assert(format_record(output, sizeof output, NULL) == -1);
}

int main(void)
{
    test_normal();
    test_failure_is_transactional();
    puts("type_format_lab passed");
    return 0;
}
```

Build:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -Wformat=2 -Wstrict-prototypes -Wmissing-prototypes \
    -fsanitize=address,undefined -g \
    type_format_lab.c -o type_format_lab
./type_format_lab
```

Output:

```text
type_format_lab passed
```

</details>

### 1B.31F. Gate tự chấm: phải trả lời và code được

- `sizeof` trả type gì, in bằng format nào?
- Vì sao `%u` cho `size_t` có thể là UB dù cùng là unsigned?
- Vì sao `%llu` không phải đáp án portable mặc định cho `uint64_t`?
- Khi nào dùng `%td`, khi nào dùng `%zu`?
- Vì sao `%p` cần argument `void *`?
- `float` biến thành type gì khi đi qua `...`?
- Dynamic width `*` đòi type gì?
- `SCNu64` khác `PRIu64` ở vai trò nào?
- Tại sao warning format phải là build error trong host tools?
- Viết được một test chứng minh output buffer không đổi khi format fail chưa?

Không trả lời miệng. Tạo `format_matrix.c` in ít nhất 12 type/value, rồi cố làm
sai bốn format để nhìn compiler bắt. Phần type chỉ được coi là vững khi bạn có
thể chọn type, literal, conversion và format như một contract thống nhất.

## 1B.32. Decision tree: gặp field mới thì chọn type gì?

```text
Field lên wire/MMIO/file không?
├── Có → width được spec quy định?
│        ├── Có → uint8_t/16_t/32_t/64_t tương ứng
│        └── Không → định nghĩa format trước, không dùng int tùy tiện
└── Không
    ├── Là memory size/capacity/index? → size_t
    ├── Là POSIX result có -1?         → ssize_t
    ├── Là địa chỉ integer?            → uintptr_t
    ├── Là true/false?                 → bool
    ├── Là state hữu hạn?              → enum
    ├── Là sample signed?              → int16_t
    ├── Là DSP intermediate?           → int32_t/int64_t
    └── Là clock/counter dài?          → uint64_t
```

Sau đó hỏi tiếp:

```text
pointer có cần const không?
caller hay callee cấp storage?
capacity ở đâu?
out-parameter có thể NULL không?
object có đi qua task/DMA không?
nếu có, dùng handle hay pointer?
```

## 1B.33. Các lỗi kiểu dữ liệu thường gặp trong modem C

### Lỗi 1 — Dùng `int` cho wire field

```c
int length;
```

Không nói được width/layout. Dùng fixed-width theo format.

### Lỗi 2 — Dùng unsigned cho giá trị có thể âm

```c
uint32_t cfo_hz;
```

CFO có thể âm; dùng `int32_t`.

### Lỗi 3 — So signed/unsigned

```c
int index = -1;
size_t count = 10U;

if (index < count) { ... }
```

`index` có thể bị convert thành unsigned lớn. Validate `index >= 0` trước rồi
convert.

### Lỗi 4 — Pointer có data nhưng không có length

```c
parse(input); /* parser biết đọc bao nhiêu byte bằng cách nào? */
```

API phải là `(input,length)`.

### Lỗi 5 — Out-parameter không check NULL

```c
*out_len = decoded;
```

nếu `out_len == NULL` sẽ crash.

### Lỗi 6 — Trả pointer tới local variable

```c
uint8_t *bad(void)
{
    uint8_t local[16];
    return local;
}
```

`local` chết khi function return.

### Lỗi 7 — Giữ pointer sau khi pool release

Handle đã release thì pointer tạm thời lấy từ handle cũng không còn hợp lệ.

### Lỗi 8 — Dùng `volatile` cho queue

Không tạo synchronization. Dùng atomic + memory order.

### Lỗi 9 — `memcpy` struct lên wire

Padding/endian/enum/bool có thể khác.

### Lỗi 10 — Cast warning đi thay vì check range

Cast không sửa dữ liệu bị truncate.

## 1B.33A. Cách tạo và chạy các bài kiểu dữ liệu

Tạo thư mục riêng để không trộn với project lớn:

```bash
mkdir -p datatype_labs
cd datatype_labs
```

Mỗi bài để trong một file:

```text
type01_read_prototype.c
type02_choose_types.md
type03_const.c
type04_safe_conversion.c
type05_out_parameter.c
type06_callback.c
type07_pointer_or_handle.md
type08_wire_struct.c
```

Với file C, compile bằng:

```bash
gcc -std=c17 \
    -Wall -Wextra -Wconversion -Wshadow -Werror \
    -fsanitize=address,undefined \
    -g -O0 \
    type04_safe_conversion.c \
    -o type04_safe_conversion

./type04_safe_conversion
```

Nếu test dùng `assert`, chương trình không in gì khi pass là bình thường. Có thể
thêm:

```c
puts("type04 passed");
```

Khi compile fail, đọc error theo thứ tự:

```text
file:line:column
loại error/warning
expression nào có type gì
compiler đang convert từ type nào sang type nào
```

Ví dụ:

```text
conversion from 'size_t' to 'uint16_t' may change value
```

Không chữa ngay bằng cast. Hỏi:

```text
size_t có thể lớn hơn UINT16_MAX không?
nếu có thì behavior mong muốn là reject hay truncate?
```

Mỗi bài phải có ít nhất:

```text
1 happy-path test
1 boundary test
1 invalid-input test
```

Sau khi pass, chạy GDB và in type/value:

```gdb
break main
run
ptype length
print length
print/x address
next
```

Mục tiêu là nhìn type trong compiler/debugger, không chỉ đọc lý thuyết.

## 1B.34. Bài kiểu dữ liệu 1 — Đọc prototype có hướng dẫn

### 1. Đề bài nhỏ

Đọc prototype sau và tự gắn vai trò cho từng thành phần trước khi xem phần hướng dẫn:

```c
int bb_phy_receive(const struct bb_config *cfg,
                   const struct bb_iq_block *in,
                   uint8_t *tb,
                   size_t capacity,
                   size_t *tb_len);
```

### 2. Tự đoán declaration/chương trình cần biểu diễn gì

Prototype mô tả status, hai input chỉ đọc, một output buffer, capacity và output length.

### 3. Học đúng lượng kiến thức cần cho bài

- return status
- `const` input pointer
- output buffer + capacity + out-length

### 4. Tự làm / tự code

Chưa mở Bước 7. Viết câu trả lời hoặc file `practice/type_01.c`; với bài code, thêm test `main()` nhỏ nhất.

### 5. Chạy test / quan sát compiler và output

Tự che phần diễn giải rồi gắn vai trò cho từng parameter; viết caller tối thiểu và kiểm tra `tb_len <= sizeof tb`.

Với bài có C source, dùng warning nghiêm ngặt và sanitizer như hướng dẫn ở mục 1B.33A.

### 6. Debug

Nếu lẫn `capacity` với `tb_len`, hỏi: giá trị nào caller đưa vào trước call, giá trị nào callee ghi ra sau call?

### Sau khi code: ý nghĩa trong modem/baseband

Đây là public API PHY: IQ block đi vào và transport-block bytes đi ra.

### 7. Đọc lời giải — ngay tại bài

<details>
<summary><strong>Mở lời giải Bài kiểu dữ liệu 1</strong></summary>

Cho:

```c
int bb_phy_receive(const struct bb_config *cfg,
                   const struct bb_iq_block *in,
                   uint8_t *tb,
                   size_t capacity,
                   size_t *tb_len);
```

Ta đọc cùng nhau:

```text
return int           → status
cfg const pointer    → config input, không sửa
in const pointer     → IQ input, không sửa block qua pointer
tb uint8_t pointer   → output byte buffer
capacity size_t      → sức chứa tb
tb_len size_t *      → output số byte thực tế
```

Caller tối thiểu:

```c
uint8_t tb[512];
size_t tb_len = 0U;

int rc = bb_phy_receive(&cfg, &input,
                        tb, sizeof tb,
                        &tb_len);
```

Đây là bài kiểu dữ liệu đầu tiên nên được hướng dẫn full.

</details>

### 8. Viết lại không nhìn lời giải

Đóng khối lời giải, viết lại câu trả lời/code từ trắng, rồi tự giải thích **type → miền giá trị → ownership/lifetime → representation modem**.


<!-- WIZARD_TYPE_1_BEGIN -->
### 9. Wizard Lab kiểu dữ liệu — Prototype role annotator

**Câu hỏi mở:** Có thể viết helper/debug macro in vai trò input/output/capacity để đọc API dài nhanh hơn không?

```c
struct param_note { const char *name; const char *role; const char *owner; };
void dump_param_notes(const struct param_note *notes, size_t count);
```

1. Annotate PHY receive.
2. Annotate pool_alloc.
3. So hai API và tìm pattern chung.

**Observer:** Bảng name/type/role/owner/lifetime.

**Invariant:** Annotation không thay contract thật; mọi out-parameter phải có owner/storage rõ.

**Tự chế:** Tự tạo template note từ prototype mới trước khi đọc implementation.



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TYPE 1</strong></summary>

Lưu thành `practice/wizard_type_01.c`:

```c
#include <assert.h>
#include <stdio.h>
#include <string.h>

struct param_note { const char *name; const char *role; const char *owner; };

static void dump_param_notes(const struct param_note *notes, size_t count)
{
    if (notes == NULL) return;
    for (size_t k = 0; k < count; ++k)
        printf("%-10s role=%-8s owner=%s\n",notes[k].name,notes[k].role,notes[k].owner);
}

int main(void)
{
    const struct param_note note[] = {{"cfg","input","caller"},{"tb","output","caller"},{"tb_len","out-len","caller"}};
    dump_param_notes(note,3U);
    assert(strcmp(note[1].role,"output") == 0);
    puts("wizard type 01 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_type_01.c -o wizard_type_01
./wizard_type_01
```

Expected cuối output:

```text
wizard type 01 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TYPE_1_END -->

---

#### Forge F07 — Bài kiểu dữ liệu 1: prototype phải biến thành executable contract

Tự code `slice_take(input,length,wanted,&slice,&slice_length)`: output mượn
memory input, không copy; failure giữ hai out-param nguyên vẹn. Thêm biến thể
take-prefix và take-tail.

<details>
<summary>Đáp án F07</summary>

```c
#include <stddef.h>
#include <stdint.h>
int slice_take(const uint8_t*in,size_t len,size_t offset,size_t wanted,
               const uint8_t**slice,size_t*slice_len)
{
 const uint8_t*tmp;
 if((slice==NULL)||(slice_len==NULL)||((in==NULL)&&(len!=0U))||
    (offset>len)||(wanted>len-offset))return-1;
 tmp=(wanted==0U)?in:&in[offset];*slice=tmp;*slice_len=wanted;return 0;
}
```

Bạn phải nói được: `in` read-only borrowed; `len/offset/wanted` là memory size;
`slice` là out-pointer; lifetime output không vượt lifetime input.

</details>

## 1B.35. Bài kiểu dữ liệu 2 — Chọn đúng kiểu

### 1. Đề bài nhỏ

Chưa mở Bước 7 ở ngay dưới bài này.

Chọn type cho:

1. Một byte PDU.
2. I/Q sample.
3. Sample rate.
4. CFO có thể âm.
5. Virtual tick.
6. Kích thước buffer host.
7. Kết quả `read()` có thể là `-1`.
8. Địa chỉ MMIO.
9. DMA state.
10. Flag `security_active`.

Với mỗi câu phải giải thích **vì sao**, không chỉ ghi type.

Sau phần chọn type, viết chương trình `practice/type_02.c`:

1. Tạo `enum bb_dma_state`.
2. Tạo `struct type_demo` chứa đủ 10 loại dữ liệu trên.
3. Dùng `_Static_assert` kiểm tra width của các integer fixed-width.
4. Khởi tạo một object bằng designated initializer.
5. Dùng `assert` kiểm tra các giá trị và `printf` in chúng bằng format đúng type.

Không `memcpy` cả `struct type_demo` lên wire. Struct này chỉ dùng để luyện chọn
type trong RAM.

### 2. Tự đoán declaration/chương trình cần biểu diễn gì

Mỗi field cần type thể hiện đúng signedness, width, lifetime hoặc platform role; không có một type dùng cho tất cả.

### 3. Học đúng lượng kiến thức cần cho bài

- wire width khác memory size
- signedness theo miền giá trị
- enum/bool/uintptr_t/ssize_t có vai trò riêng

### 4. Tự làm / tự code

Chưa mở Bước 7. Làm theo thứ tự:

1. Viết bảng 10 lựa chọn type bằng lời.
2. Tạo `practice/type_02.c` với đầy đủ `#include`.
3. Định nghĩa `enum bb_dma_state` và `struct type_demo`.
4. Viết các `_Static_assert` trước `main()`.
5. Khởi tạo object `demo`, kiểm tra bằng `assert`, rồi in bằng `printf`.

Nếu chưa nhớ format của `uint32_t`, `uint64_t` hoặc `uintptr_t`, xem lại mục
1B.31 rồi quay lại bài; không đổi tất cả sang `%lu` cho nhanh.

### 5. Chạy test / quan sát compiler và output

Điền đủ 10 dòng rồi thêm cột miền giá trị và lý do. Sau đó compile chương trình:

```bash
mkdir -p practice build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/type_02.c \
    -o build/type_02
./build/type_02
```

Expected cuối output:

```text
type02 passed
```

Compiler phải không có warning. `_Static_assert` phải pass ngay lúc compile.

### 6. Debug

Nếu chỉ trả lời theo số lớn/nhỏ, kiểm tra lại field đó nằm trên wire, trong RAM, là address, state hay status.

- Lỗi `unknown type name 'ssize_t'`: thiếu `<sys/types.h>` trên host POSIX.
- Lỗi format `printf`: dùng macro trong `<inttypes.h>` cho fixed-width integer.
- Nếu cast hết mọi warning: dừng lại và kiểm tra miền giá trị trước cast.

### Sau khi code: ý nghĩa trong modem/baseband

Chọn type đúng làm contract data/wire/ownership của modem rõ từ declaration.

### 7. Đọc lời giải — ngay tại bài

<details>
<summary><strong>Mở lời giải Bài kiểu dữ liệu 2</strong></summary>

| Dữ liệu | Type phù hợp | Vì sao |
|---|---|---|
| Một byte PDU | `uint8_t` | đúng một byte unsigned trên wire |
| I/Q sample | `int16_t` | fixed-point 16-bit và cần giá trị âm |
| Sample rate | `uint32_t` | Hz không âm, profile dùng field 32-bit |
| CFO | `int32_t` | có thể lệch âm hoặc dương |
| Virtual tick | `uint64_t` | counter dài và không âm |
| Kích thước buffer host | `size_t` | đúng type của `sizeof` và memory API |
| Kết quả `read()` | `ssize_t` | cần số byte không âm hoặc `-1` |
| Địa chỉ MMIO | `uintptr_t` | integer đủ giữ giá trị pointer/address |
| DMA state | `enum bb_dma_state` | tập state hữu hạn có tên |
| `security_active` | `bool` | đúng hai trạng thái local true/false |

Chú ý:

```text
type storage không luôn bằng protocol range
```

Ví dụ `harq_id` có thể là `uint8_t`, nhưng code vẫn phải check `< 8`.

Full chương trình đối chiếu:

```c
#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

enum bb_dma_state {
    BB_DMA_FREE = 0,
    BB_DMA_CPU_OWNED,
    BB_DMA_DEVICE_OWNED
};

struct type_demo {
    uint8_t pdu_byte;
    int16_t iq_sample;
    uint32_t sample_rate_hz;
    int32_t cfo_hz;
    uint64_t virtual_tick;
    size_t buffer_size;
    ssize_t io_result;
    uintptr_t mmio_address;
    enum bb_dma_state dma_state;
    bool security_active;
};

_Static_assert(sizeof(uint8_t) == 1U, "uint8_t must be one byte");
_Static_assert(sizeof(int16_t) == 2U, "int16_t must be two bytes");
_Static_assert(sizeof(uint32_t) == 4U, "uint32_t must be four bytes");
_Static_assert(sizeof(uint64_t) == 8U, "uint64_t must be eight bytes");
_Static_assert(sizeof(uintptr_t) == sizeof(void *),
               "uintptr_t must hold a pointer");

int main(void)
{
    const struct type_demo demo = {
        .pdu_byte = UINT8_C(0x41),
        .iq_sample = INT16_C(-1200),
        .sample_rate_hz = UINT32_C(30720000),
        .cfo_hz = INT32_C(-1500),
        .virtual_tick = UINT64_C(9000000000),
        .buffer_size = (size_t)4096U,
        .io_result = (ssize_t)-1,
        .mmio_address = (uintptr_t)UINT32_C(0x40000000),
        .dma_state = BB_DMA_CPU_OWNED,
        .security_active = true
    };

    assert(demo.pdu_byte == UINT8_C(0x41));
    assert(demo.iq_sample < 0);
    assert(demo.sample_rate_hz == UINT32_C(30720000));
    assert(demo.cfo_hz == INT32_C(-1500));
    assert(demo.io_result == (ssize_t)-1);
    assert(demo.dma_state == BB_DMA_CPU_OWNED);
    assert(demo.security_active);

    printf("pdu=0x%02x iq=%d rate=%" PRIu32 " cfo=%" PRId32 "\n",
           (unsigned)demo.pdu_byte,
           (int)demo.iq_sample,
           demo.sample_rate_hz,
           demo.cfo_hz);
    printf("tick=%" PRIu64 " size=%zu io=%zd mmio=0x%" PRIxPTR "\n",
           demo.virtual_tick,
           demo.buffer_size,
           demo.io_result,
           demo.mmio_address);

    puts("type02 passed");
    return 0;
}
```

> Chú ý: type đúng là bước đầu; protocol range vẫn phải được validate riêng.

</details>

### 8. Viết lại không nhìn lời giải

Đóng khối lời giải, viết lại câu trả lời/code từ trắng, rồi tự giải thích **type → miền giá trị → ownership/lifetime → representation modem**.


<!-- WIZARD_TYPE_2_BEGIN -->
### 9. Wizard Lab kiểu dữ liệu — Type-domain experiment

**Câu hỏi mở:** Nếu đổi một field từ signed sang unsigned hoặc width nhỏ hơn, compiler/test giúp thấy bug gì?

```c
int range_fits_u16(size_t value);
int range_fits_i16(int32_t value);
void dump_type_limits(void);
```

1. Sweep biên mỗi type.
2. So signed/unsigned warning.
3. Đổi CFO âm sang uint32 và quan sát representation.

**Observer:** Compiler warnings, limit table và round-trip cast.

**Invariant:** Cast chỉ sau range check; storage type không thay protocol range.

**Tự chế:** Viết helper checked-conversion cho ba cặp type bạn dùng nhiều nhất.



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TYPE 2</strong></summary>

Lưu thành `practice/wizard_type_02.c`:

```c
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

static int range_fits_u16(size_t value) { return value <= UINT16_MAX; }
static int range_fits_i16(int32_t value) { return value >= INT16_MIN && value <= INT16_MAX; }
static void dump_type_limits(void)
{
    printf("u16=0..%u i16=%d..%d size_max=%zu\n",UINT16_MAX,INT16_MIN,INT16_MAX,(size_t)-1);
}

int main(void)
{
    assert(range_fits_u16(65535U) != 0 && range_fits_u16(65536U) == 0);
    assert(range_fits_i16(-32768) != 0 && range_fits_i16(40000) == 0);
    dump_type_limits();
    puts("wizard type 02 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_type_02.c -o wizard_type_02
./wizard_type_02
```

Expected cuối output:

```text
wizard type 02 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TYPE_2_END -->

---

#### Forge F08 — Bài kiểu dữ liệu 2: chọn type rồi chứng minh bằng code

Tự code:

1. `sn12_valid(uint16_t)`.
2. `tick_due(uint64_t now,uint64_t deadline)` bằng signed modular delta.
3. Convert wire `uint64_t count` sang `size_t` sau range check.

<details>
<summary>Đáp án F08</summary>

```c
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
int sn12_valid(uint16_t sn){return sn<UINT16_C(4096);}
int tick_due(uint64_t now,uint64_t deadline)
{return ((now-deadline)&(UINT64_C(1)<<63U))==0U;}
int u64_to_size(uint64_t wire,size_t*out)
{
 if((out==NULL)||(wire>(uint64_t)SIZE_MAX))return-1;
 *out=(size_t)wire;return 0;
}
```

Gate: giải thích vì sao type `uint16_t` chưa đủ enforce SN12, và horizon của
`tick_due` phải nhỏ hơn `2^63`.

</details>

## 1B.36. Bài kiểu dữ liệu 3 — `const` nào đúng?

### 1. Đề bài nhỏ

Viết prototype cho hàm:

```text
nhận config chỉ đọc
nhận input bytes chỉ đọc
ghi output bytes
không thay đổi địa chỉ output bên trong function
```

Hint:

```c
int transform(/* cfg */, /* input */, /* output */, size_t count);
```

Phân biệt:

```c
const uint8_t *input
uint8_t * const output
```

Lưu ý `const` top-level ở parameter pointer truyền by value thường không phải
phần public contract quan trọng, nhưng vẫn hữu ích bên trong implementation.

### 2. Tự đoán declaration/chương trình cần biểu diễn gì

Config và input không được sửa; output bytes được ghi. `const` phải đặt đúng phía của dấu `*`.

### 3. Học đúng lượng kiến thức cần cho bài

- `const T *`
- `T * const`
- top-level const parameter

### 4. Tự làm / tự code

Chưa mở Bước 7. Viết câu trả lời hoặc file `practice/type_03.c`; với bài code, thêm test `main()` nhỏ nhất.

### 5. Chạy test / quan sát compiler và output

Viết prototype, tạo implementation thử sửa từng thứ và dùng compiler để xem phép nào bị chặn.

Với bài có C source, dùng warning nghiêm ngặt và sanitizer như hướng dẫn ở mục 1B.33A.

### 6. Debug

Đọc declaration từ tên ra ngoài; phân biệt 'pointer const' với 'data const'.

### Sau khi code: ý nghĩa trong modem/baseband

Const-correct API ngăn DSP/parser vô tình sửa config hoặc RX input.

### 7. Đọc lời giải — ngay tại bài

<details>
<summary><strong>Mở lời giải Bài kiểu dữ liệu 3</strong></summary>

Public prototype đơn giản:

```c
struct bb_config;

int transform(const struct bb_config *cfg,
              const uint8_t *input,
              uint8_t *output,
              size_t count);
```

Ý nghĩa:

```text
cfg/input không được sửa qua các pointer đó
output data được phép sửa
```

Trong implementation, nếu muốn nhắc compiler rằng biến pointer `output` không
được trỏ sang buffer khác:

```c
int transform(const struct bb_config *cfg,
              const uint8_t *input,
              uint8_t * const output,
              size_t count)
{
    /* ... */
}
```

Nhưng qualifier top-level `const` trên parameter pointer truyền by value không
làm thay đổi function type đối với caller. Contract quan trọng nhất vẫn là:

```text
const data input
mutable data output
```

</details>

### 8. Viết lại không nhìn lời giải

Đóng khối lời giải, viết lại câu trả lời/code từ trắng, rồi tự giải thích **type → miền giá trị → ownership/lifetime → representation modem**.


<!-- WIZARD_TYPE_3_BEGIN -->
### 9. Wizard Lab kiểu dữ liệu — Const compiler laboratory

**Câu hỏi mở:** Thay từng vị trí const rồi cố ý sửa data/pointer để compiler dạy mình điều gì?

```c
void inspect_read_only(const uint8_t *data, size_t n);
void fill_output(uint8_t * const out, size_t n);
```

1. Bỏ/thêm const data.
2. Thử reassign pointer const.
3. Tạo view read-only của IQ block.

**Observer:** Compile success/failure matrix.

**Invariant:** Public contract const-data không bị lách bằng cast.

**Tự chế:** Tạo ba snippet compile-fail làm regression documentation.



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TYPE 3</strong></summary>

Lưu thành `practice/wizard_type_03.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void inspect_read_only(const uint8_t *data, size_t n)
{
    uint32_t sum = 0U;
    if (data == NULL) return;
    for (size_t k = 0; k < n; ++k) sum += data[k];
    printf("sum=%u\n",sum);
}

static void fill_output(uint8_t * const out, size_t n)
{
    if (out == NULL) return;
    for (size_t k = 0; k < n; ++k) out[k] = (uint8_t)k;
}

int main(void)
{
    const uint8_t input[3] = {1U,2U,3U};
    const uint8_t before[3] = {1U,2U,3U};
    uint8_t output[3];
    inspect_read_only(input,3U); assert(memcmp(input,before,sizeof input) == 0);
    fill_output(output,3U); assert(output[2] == 2U);
    puts("wizard type 03 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_type_03.c -o wizard_type_03
./wizard_type_03
```

Expected cuối output:

```text
wizard type 03 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TYPE_3_END -->

---

#### Forge F09 — Bài kiểu dữ liệu 3: `const` qua ba vai trò

Tự code checksum read-only, invert mutable và chọn sample trả borrowed
`const` pointer. Cố viết qua pointer ở từng API để compiler bắt.

<details>
<summary>Đáp án F09</summary>

```c
#include <stddef.h>
#include <stdint.h>
uint32_t bytes_sum(const uint8_t*p,size_t n)
{size_t k;uint32_t s=0U;if((p==NULL)&&(n!=0U))return 0U;
 for(k=0U;k<n;++k)s+=p[k];return s;}
int bytes_invert(uint8_t*p,size_t n)
{size_t k;if((p==NULL)&&(n!=0U))return-1;
 for(k=0U;k<n;++k)p[k]=(uint8_t)~p[k];return 0;}
int byte_choose(const uint8_t*p,size_t n,size_t i,const uint8_t**out)
{if((p==NULL)||(out==NULL)||(i>=n))return-1;*out=&p[i];return 0;}
```

`const` không nói ownership hay lifetime; note của bài phải ghi output của
`byte_choose` chết cùng storage đầu vào.

</details>

## 1B.37. Bài kiểu dữ liệu 4 — Safe conversion

### 1. Đề bài nhỏ

Viết:

```c
int size_to_u16(size_t input, uint16_t *output);
```

Yêu cầu:

```text
NULL output → fail
input > UINT16_MAX → fail
success → ghi output và return 0
failure → không ghi giá trị rác
```

### 2. Tự đoán declaration/chương trình cần biểu diễn gì

Chỉ cast `size_t` xuống `uint16_t` sau khi đã chứng minh không vượt `UINT16_MAX`; fail không được ghi output.

### 3. Học đúng lượng kiến thức cần cho bài

- range check
- NULL guard
- commit output chỉ trên success

### 4. Tự làm / tự code

Chưa mở Bước 7. Viết câu trả lời hoặc file `practice/type_04.c`; với bài code, thêm test `main()` nhỏ nhất.

### 5. Chạy test / quan sát compiler và output

Test NULL, 0, UINT16_MAX và UINT16_MAX+1.

Với bài có C source, dùng warning nghiêm ngặt và sanitizer như hướng dẫn ở mục 1B.33A.

### 6. Debug

Đặt sentinel trong output trước call fail; nếu sentinel đổi, bạn ghi quá sớm.

### Sau khi code: ý nghĩa trong modem/baseband

Đây là kiểu conversion xuất hiện khi memory length host đi vào field wire 16-bit.

### 7. Đọc lời giải — ngay tại bài

<details>
<summary><strong>Mở lời giải Bài kiểu dữ liệu 4</strong></summary>

```c
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static int size_to_u16(size_t input, uint16_t *output)
{
    if (output == NULL) {
        return -1;
    }

    if (input > UINT16_MAX) {
        return -1;
    }

    *output = (uint16_t)input;
    return 0;
}

int main(void)
{
    uint16_t output = 1234U;

    assert(size_to_u16(0U, &output) == 0);
    assert(output == 0U);

    assert(size_to_u16(UINT16_MAX, &output) == 0);
    assert(output == UINT16_MAX);

    output = 1234U;
    assert(size_to_u16((size_t)UINT16_MAX + 1U, &output) == -1);
    assert(output == 1234U); /* failure không sửa output */

    assert(size_to_u16(10U, NULL) == -1);

    puts("type04 passed");
    return 0;
}
```

Điểm chính:

```text
check range
→ mới cast
```

Không phải:

```text
cast
→ hy vọng dữ liệu vẫn đúng
```

</details>

### 8. Viết lại không nhìn lời giải

Đóng khối lời giải, viết lại câu trả lời/code từ trắng, rồi tự giải thích **type → miền giá trị → ownership/lifetime → representation modem**.


<!-- WIZARD_TYPE_4_BEGIN -->
### 9. Wizard Lab kiểu dữ liệu — Checked-conversion matrix

**Câu hỏi mở:** Có thể dùng một bảng vector để test hàng loạt conversion chứ không chỉ ba case không?

```c
struct u16_case { size_t input; int expected_rc; uint16_t expected; };
int run_u16_cases(const struct u16_case *cases, size_t count);
```

1. Biên 0/MAX/MAX+1.
2. Output sentinel khi fail.
3. Sinh 100 giá trị quanh biên.

**Observer:** Case index và expected/actual.

**Invariant:** Failure không sửa output; success round-trip đúng.

**Tự chế:** Generalize pattern runner cho size→u32 và i32→i16.



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TYPE 4</strong></summary>

Lưu thành `practice/wizard_type_04.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

struct u16_case { size_t input; int expected_rc; uint16_t expected; };
static int checked_u16(size_t input, uint16_t *out)
{
    if (out == NULL || input > UINT16_MAX) return -1;
    *out = (uint16_t)input; return 0;
}

static int run_u16_cases(const struct u16_case *cases, size_t count)
{
    if (cases == NULL) return -1;
    for (size_t k = 0; k < count; ++k) {
        uint16_t out = UINT16_C(0xaaaa);
        const int rc = checked_u16(cases[k].input,&out);
        if (rc != cases[k].expected_rc || (rc == 0 && out != cases[k].expected)) return -1;
        if (rc != 0 && out != UINT16_C(0xaaaa)) return -1;
    }
    return 0;
}

int main(void)
{
    const struct u16_case c[] = {{0U,0,0U},{65535U,0,65535U},{65536U,-1,0U}};
    assert(run_u16_cases(c,3U) == 0);
    puts("wizard type 04 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_type_04.c -o wizard_type_04
./wizard_type_04
```

Expected cuối output:

```text
wizard type 04 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TYPE_4_END -->

---

#### Forge F10 — Bài kiểu dữ liệu 4: safe conversion không dùng cast làm thuốc mê

Tự code `size_to_u16`, `i64_to_size` và `size_add`. Mỗi failure phải giữ output.

<details>
<summary>Đáp án F10</summary>

```c
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
int size_to_u16(size_t x,uint16_t*out)
{if((out==NULL)||(x>UINT16_MAX))return-1;*out=(uint16_t)x;return 0;}
int i64_to_size(int64_t x,size_t*out)
{if((out==NULL)||(x<0)||((uint64_t)x>(uint64_t)SIZE_MAX))return-1;
 *out=(size_t)x;return 0;}
int size_add(size_t a,size_t b,size_t*out)
{if((out==NULL)||(b>SIZE_MAX-a))return-1;*out=a+b;return 0;}
```

Thêm `size_mul` không nhìn đáp án: check `a != 0 && b > SIZE_MAX/a`.

</details>

## 1B.38. Bài kiểu dữ liệu 5 — Out-parameter

### 1. Đề bài nhỏ

Viết hàm:

```c
int find_peak(const int16_t *samples, size_t count,
              size_t *peak_index, int16_t *peak_value);
```

Yêu cầu:

```text
không sửa samples
reject NULL
reject count=0
trả cả index và value
```

Modem concept: detector/synchronizer thường trả nhiều metadata bằng
out-parameter.

### 2. Tự đoán declaration/chương trình cần biểu diễn gì

Hàm quét mảng và trả đồng thời index lẫn value qua hai out-parameter.

### 3. Học đúng lượng kiến thức cần cho bài

- input const pointer + count
- nhiều out-parameter
- khởi tạo best từ phần tử đầu

### 4. Tự làm / tự code

Chưa mở Bước 7. Viết câu trả lời hoặc file `practice/type_05.c`; với bài code, thêm test `main()` nhỏ nhất.

### 5. Chạy test / quan sát compiler và output

Test peak đầu/giữa/cuối, một phần tử, count 0 và NULL.

Với bài có C source, dùng warning nghiêm ngặt và sanitizer như hướng dẫn ở mục 1B.33A.

### 6. Debug

Nếu peak âm sai, đừng khởi tạo best bằng 0; dùng `samples[0]` sau validation.

### Sau khi code: ý nghĩa trong modem/baseband

Detector/synchronizer thường trả score, index và metadata cùng lúc.

### 7. Đọc lời giải — ngay tại bài

<details>
<summary><strong>Mở lời giải Bài kiểu dữ liệu 5</strong></summary>

Solution này định nghĩa “peak” là sample có **giá trị signed lớn nhất**, không
phải trị tuyệt đối lớn nhất.

```c
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static int find_peak(const int16_t *samples, size_t count,
                     size_t *peak_index, int16_t *peak_value)
{
    size_t best_index;
    int16_t best_value;

    if ((samples == NULL) || (peak_index == NULL) ||
        (peak_value == NULL) || (count == 0U)) {
        return -1;
    }

    best_index = 0U;
    best_value = samples[0];

    for (size_t index = 1U; index < count; ++index) {
        if (samples[index] > best_value) {
            best_value = samples[index];
            best_index = index;
        }
    }

    *peak_index = best_index;
    *peak_value = best_value;
    return 0;
}

int main(void)
{
    const int16_t samples[] = {-20, 100, 50, 99};
    size_t index = SIZE_MAX;
    int16_t value = 0;

    assert(find_peak(samples,
                     sizeof samples / sizeof samples[0],
                     &index, &value) == 0);
    assert(index == 1U);
    assert(value == 100);

    assert(find_peak(NULL, 4U, &index, &value) == -1);
    assert(find_peak(samples, 0U, &index, &value) == -1);
    assert(find_peak(samples, 4U, NULL, &value) == -1);

    puts("type05 passed");
    return 0;
}
```

Nếu muốn peak magnitude, cần contract khác và phải cẩn thận với `INT16_MIN` vì
trị tuyệt đối của `-32768` không chứa được trong `int16_t`.

</details>

### 8. Viết lại không nhìn lời giải

Đóng khối lời giải, viết lại câu trả lời/code từ trắng, rồi tự giải thích **type → miền giá trị → ownership/lifetime → representation modem**.


<!-- WIZARD_TYPE_5_BEGIN -->
### 9. Wizard Lab kiểu dữ liệu — Reducer callback

**Câu hỏi mở:** Peak chỉ là một loại reduce; làm sao tái dùng vòng quét cho min/max/energy/first-above?

```c
typedef void (*sample_reduce_fn)(int16_t value, size_t index, void *ctx);
void samples_reduce(const int16_t *x, size_t n, sample_reduce_fn fn, void *ctx);
```

1. Max signed.
2. Max magnitude.
3. First threshold crossing.

**Observer:** Reducer state và final metadata.

**Invariant:** Input const; callback state nằm trong ctx; count 0 policy rõ.

**Tự chế:** Viết reducer trả top-3 peak không sort toàn array.



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TYPE 5</strong></summary>

Lưu thành `practice/wizard_type_05.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

typedef void (*sample_reduce_fn)(int16_t value, size_t index, void *ctx);
static void samples_reduce(const int16_t *x, size_t n, sample_reduce_fn fn, void *ctx)
{
    if (x == NULL || fn == NULL) return;
    for (size_t k = 0; k < n; ++k) fn(x[k],k,ctx);
}
struct reduce_ctx { int32_t sum; int16_t max; size_t max_at; };
static void collect(int16_t value, size_t index, void *ctx)
{
    struct reduce_ctx *r = ctx; if (r == NULL) return;
    r->sum += value; if (index == 0U || value > r->max) { r->max=value; r->max_at=index; }
}

int main(void)
{
    const int16_t x[4] = {1,9,-2,3}; struct reduce_ctx r = {0};
    samples_reduce(x,4U,collect,&r);
    assert(r.sum == 11 && r.max == 9 && r.max_at == 1U);
    puts("wizard type 05 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_type_05.c -o wizard_type_05
./wizard_type_05
```

Expected cuối output:

```text
wizard type 05 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TYPE_5_END -->

---

#### Forge F11 — Bài kiểu dữ liệu 5: out-parameter phải commit một lần

Tự code min/max byte, split `uint32_t` thành high/low `uint16_t`, và decoder
trả cả value lẫn consumed length. Không ghi một output rồi mới phát hiện lỗi.

<details>
<summary>Đáp án F11</summary>

```c
#include <stddef.h>
#include <stdint.h>
int byte_minmax(const uint8_t*p,size_t n,uint8_t*lo,uint8_t*hi)
{size_t k;uint8_t l,h;if((p==NULL)||(lo==NULL)||(hi==NULL)||(n==0U))return-1;
 l=h=p[0];for(k=1U;k<n;++k){if(p[k]<l)l=p[k];if(p[k]>h)h=p[k];}
 *lo=l;*hi=h;return 0;}
int split_u32(uint32_t v,uint16_t*high,uint16_t*low)
{if((high==NULL)||(low==NULL))return-1;
 *high=(uint16_t)(v>>16);*low=(uint16_t)v;return 0;}
int decode_u16_le(const uint8_t*p,size_t n,uint16_t*v,size_t*used)
{uint16_t t;if((p==NULL)||(v==NULL)||(used==NULL)||(n<2U))return-1;
 t=(uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8));*v=t;*used=2U;return 0;}
```

</details>

## 1B.39. Bài kiểu dữ liệu 6 — Function pointer + context

### 1. Đề bài nhỏ

Tạo:

```c
struct clock_ops {
    uint64_t (*ticks)(void *ctx);
};
```

Viết fake clock có field `now`, gắn callback, gọi qua ops và kiểm kết quả.

Modem concept: platform abstraction cho phép cùng firmware chạy host và ARM.

### 2. Tự đoán declaration/chương trình cần biểu diễn gì

Ops table gọi clock implementation qua function pointer; `ctx` mang state của instance fake.

### 3. Học đúng lượng kiến thức cần cho bài

- function pointer signature
- `void *ctx` cast lại đúng type
- dependency injection

### 4. Tự làm / tự code

Chưa mở Bước 7. Viết câu trả lời hoặc file `practice/type_06.c`; với bài code, thêm test `main()` nhỏ nhất.

### 5. Chạy test / quan sát compiler và output

Tạo hai fake clock có `now` khác nhau và gọi chung một ops table.

Với bài có C source, dùng warning nghiêm ngặt và sanitizer như hướng dẫn ở mục 1B.33A.

### 6. Debug

Nếu hai clock lẫn nhau, bạn đã dùng global thay vì `ctx`.

### Sau khi code: ý nghĩa trong modem/baseband

Cùng firmware core có thể dùng fake time trên host và timer phần cứng trên Arm.

### 7. Đọc lời giải — ngay tại bài

<details>
<summary><strong>Mở lời giải Bài kiểu dữ liệu 6</strong></summary>

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

struct clock_ops {
    uint64_t (*ticks)(void *ctx);
};

struct fake_clock {
    uint64_t now;
};

static uint64_t fake_ticks(void *ctx)
{
    struct fake_clock *clock = ctx;

    if (clock == NULL) {
        return 0U;
    }

    return clock->now;
}

int main(void)
{
    struct fake_clock clock = { .now = UINT64_C(1250) };
    const struct clock_ops ops = { .ticks = fake_ticks };

    assert(ops.ticks != NULL);
    assert(ops.ticks(&clock) == UINT64_C(1250));

    clock.now = UINT64_C(2000);
    assert(ops.ticks(&clock) == UINT64_C(2000));

    puts("type06 passed");
    return 0;
}
```

Tách vai trò:

```text
fake_ticks → code sẽ chạy
&clock     → object state mà code sử dụng
ops        → interface mà modem nhìn thấy
```

Trên target khác, `ticks` có thể đọc timer MMIO nhưng caller không cần đổi logic.

</details>

### 8. Viết lại không nhìn lời giải

Đóng khối lời giải, viết lại câu trả lời/code từ trắng, rồi tự giải thích **type → miền giá trị → ownership/lifetime → representation modem**.


<!-- WIZARD_TYPE_6_BEGIN -->
### 9. Wizard Lab kiểu dữ liệu — Ops-table decorator

**Câu hỏi mở:** Có thể bọc callback clock để đếm/log call mà caller không biết không?

```c
struct clock_counter_ctx { const struct clock_ops *inner; void *inner_ctx; size_t calls; };
uint64_t counting_ticks(void *ctx);
```

1. Hai fake clock độc lập.
2. Decorator đếm call.
3. Decorator thêm offset deterministic.

**Observer:** Returned tick + calls.

**Invariant:** Decorator không dùng global; lifetime ctx rõ.

**Tự chế:** Chain hai decorator: count rồi clamp monotonic.



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TYPE 6</strong></summary>

Lưu thành `practice/wizard_type_06.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

struct clock_ops { uint64_t (*ticks)(void *ctx); };
struct clock_counter_ctx { const struct clock_ops *inner; void *inner_ctx; size_t calls; };
struct fake_clock { uint64_t now; };

static uint64_t fake_ticks(void *ctx)
{
    const struct fake_clock *clock = ctx; return clock == NULL ? 0U : clock->now;
}
static uint64_t counting_ticks(void *ctx)
{
    struct clock_counter_ctx *counter = ctx;
    if (counter == NULL || counter->inner == NULL || counter->inner->ticks == NULL) return 0U;
    ++counter->calls; return counter->inner->ticks(counter->inner_ctx);
}

int main(void)
{
    struct fake_clock clock = {42U}; const struct clock_ops inner = {fake_ticks};
    struct clock_counter_ctx ctx = {&inner,&clock,0U}; const struct clock_ops decorated = {counting_ticks};
    assert(decorated.ticks(&ctx) == 42U && decorated.ticks(&ctx) == 42U && ctx.calls == 2U);
    puts("wizard type 06 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_type_06.c -o wizard_type_06
./wizard_type_06
```

Expected cuối output:

```text
wizard type 06 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TYPE_6_END -->

---

#### Forge F12 — Bài kiểu dữ liệu 6: callback luôn đi cùng context

Tự code map bytes, reduce sum và predicate count. Dùng hai context khác nhau
trong cùng test để chứng minh không có hidden global state.

<details>
<summary>Đáp án F12</summary>

```c
#include <stddef.h>
#include <stdint.h>
typedef uint8_t(*map_fn)(void*,uint8_t);
typedef int(*pred_fn)(void*,uint8_t);
int bytes_map(uint8_t*p,size_t n,map_fn fn,void*ctx)
{size_t k;if(((p==NULL)&&(n!=0U))||(fn==NULL))return-1;
 for(k=0U;k<n;++k)p[k]=fn(ctx,p[k]);return 0;}
size_t bytes_count(const uint8_t*p,size_t n,pred_fn fn,void*ctx)
{size_t k,c=0U;if(((p==NULL)&&(n!=0U))||(fn==NULL))return 0U;
 for(k=0U;k<n;++k)if(fn(ctx,p[k]))++c;return c;}
```

Biến thể tự hoàn tất: reducer nhận accumulator `uint64_t *` và callback trả
status để có thể abort mà không giấu lỗi.

</details>

## 1B.40. Bài kiểu dữ liệu 7 — Pointer hay handle?

### 1. Đề bài nhỏ

Với từng tình huống, chọn raw pointer hay handle và giải thích:

1. Hàm FFT sửa buffer trong cùng task.
2. IRQ enqueue RX buffer cho deferred task.
3. Encode descriptor lên wire.
4. Local parser nhìn payload trong thời gian function call.
5. AP gửi buffer identity cho CP.

Sau khi trả lời bằng lời, viết `practice/type_07.c` để chứng minh hai trường hợp:

1. Hàm DSP nhận raw pointer và sửa mảng IQ ngay trong cùng lời gọi.
2. Pool hai slot trả `struct buf_handle { slot, generation }`; sau khi release và
   allocate lại, handle cũ phải bị reject.

### 2. Tự đoán declaration/chương trình cần biểu diễn gì

Pointer phù hợp khi cùng lifetime/context; handle phù hợp khi vượt task/IRQ/IPC hoặc cần phát hiện stale identity.

### 3. Học đúng lượng kiến thức cần cho bài

- lifetime
- ownership transfer
- serialization và generation

### 4. Tự làm / tự code

Chưa mở Bước 7. Tạo `practice/type_07.c`, rồi code theo từng nấc:

1. Viết `scale_iq_half(int16_t *samples, size_t count)` và test trước.
2. Định nghĩa `struct buf_handle` chỉ gồm `slot` và `generation`.
3. Tạo pool đúng hai slot, mỗi slot tám byte.
4. Viết `pool_alloc`, `pool_resolve`, `pool_release`.
5. Test handle hợp lệ.
6. Release, allocate lại cùng slot và test handle cũ bị reject.

Không truyền raw pointer qua “queue giả”. Object đi qua boundary phải được đại
diện bằng handle.

### 5. Chạy test / quan sát compiler và output

Lập bảng 5 tình huống: lựa chọn, owner hiện tại, lifetime và lỗi cần tránh. Sau
đó chạy code:

```bash
mkdir -p practice build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/type_07.c \
    -o build/type_07
./build/type_07
```

Expected:

```text
raw pointer: I=500 Q=-300
handle: old generation rejected
type07 passed
```

### 6. Debug

Nếu định encode raw pointer lên wire, dừng lại: address chỉ có nghĩa trong address space hiện tại.

- Handle cũ vẫn resolve được: bạn chỉ so `slot`, chưa so `generation`.
- Allocate lại nhưng generation không đổi: tăng generation lúc slot được cấp lại.
- Hai API trả pointer nhưng không có owner/lifetime: ghi lại ai đang giữ slot.
- ASan báo lỗi: kiểm tra `slot < SLOT_COUNT` trước khi index pool.

### Sau khi code: ý nghĩa trong modem/baseband

Modem dùng handle để chuyển ownership an toàn qua queue/DMA/AP↔CP.

### 7. Đọc lời giải — ngay tại bài

<details>
<summary><strong>Mở lời giải Bài kiểu dữ liệu 7</strong></summary>

1. **FFT sửa buffer trong cùng task:** raw pointer + `count`. Lifetime và owner
   nằm trong cùng call/domain.
2. **IRQ chuyển RX buffer sang deferred task:** handle generation-tagged. Hai
   execution context cần chuyển identity/ownership có kiểm tra stale.
3. **Descriptor trên wire:** fixed-width address/handle field theo contract,
   không serialize raw host pointer.
4. **Parser nhìn payload trong thời gian function call:** `const uint8_t *` +
   `size_t length`; đây là borrowed read-only view ngắn hạn.
5. **AP gửi identity cho CP:** handle/offset/physical aperture ID fixed-width;
   không gửi process virtual pointer.

Quy tắc:

```text
local + lifetime rõ + cùng owner → pointer
qua queue/domain/wire/restart    → handle hoặc fixed-width identity
```

Full chương trình đối chiếu:

```c
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define SLOT_COUNT 2U
#define SLOT_CAPACITY 8U

struct buf_handle {
    uint16_t slot;
    uint16_t generation;
};

struct buffer_slot {
    bool used;
    uint16_t generation;
    uint8_t data[SLOT_CAPACITY];
};

struct buffer_pool {
    struct buffer_slot slots[SLOT_COUNT];
};

static void scale_iq_half(int16_t *samples, size_t count)
{
    if (samples == NULL) {
        return;
    }

    for (size_t index = 0U; index < count; ++index) {
        samples[index] = (int16_t)(samples[index] / 2);
    }
}

static uint16_t next_generation(uint16_t current)
{
    if (current == UINT16_MAX) {
        return UINT16_C(1);
    }

    return (uint16_t)(current + UINT16_C(1));
}

static int pool_alloc(struct buffer_pool *pool,
                      struct buf_handle *out_handle,
                      uint8_t **out_data)
{
    if ((pool == NULL) || (out_handle == NULL) || (out_data == NULL)) {
        return -1;
    }

    for (size_t index = 0U; index < (size_t)SLOT_COUNT; ++index) {
        struct buffer_slot *slot = &pool->slots[index];

        if (!slot->used) {
            slot->generation = next_generation(slot->generation);
            slot->used = true;

            for (size_t byte = 0U; byte < (size_t)SLOT_CAPACITY; ++byte) {
                slot->data[byte] = UINT8_C(0);
            }

            out_handle->slot = (uint16_t)index;
            out_handle->generation = slot->generation;
            *out_data = slot->data;
            return 0;
        }
    }

    return -1;
}

static int pool_resolve(struct buffer_pool *pool,
                        struct buf_handle handle,
                        uint8_t **out_data)
{
    struct buffer_slot *slot;

    if ((pool == NULL) || (out_data == NULL)) {
        return -1;
    }
    if ((size_t)handle.slot >= (size_t)SLOT_COUNT) {
        return -1;
    }

    slot = &pool->slots[handle.slot];
    if ((!slot->used) || (slot->generation != handle.generation)) {
        return -1;
    }

    *out_data = slot->data;
    return 0;
}

static int pool_release(struct buffer_pool *pool, struct buf_handle handle)
{
    struct buffer_slot *slot;

    if (pool == NULL) {
        return -1;
    }
    if ((size_t)handle.slot >= (size_t)SLOT_COUNT) {
        return -1;
    }

    slot = &pool->slots[handle.slot];
    if ((!slot->used) || (slot->generation != handle.generation)) {
        return -1;
    }

    slot->used = false;
    return 0;
}

int main(void)
{
    int16_t iq[] = {INT16_C(1000), INT16_C(-600)};
    struct buffer_pool pool = {0};
    struct buf_handle old_handle;
    struct buf_handle new_handle;
    uint8_t *data = NULL;
    uint8_t *resolved = NULL;

    scale_iq_half(iq, sizeof iq / sizeof iq[0]);
    assert(iq[0] == INT16_C(500));
    assert(iq[1] == INT16_C(-300));
    printf("raw pointer: I=%d Q=%d\n", (int)iq[0], (int)iq[1]);

    assert(pool_alloc(&pool, &old_handle, &data) == 0);
    data[0] = UINT8_C(0x41);
    assert(pool_resolve(&pool, old_handle, &resolved) == 0);
    assert(resolved[0] == UINT8_C(0x41));

    assert(pool_release(&pool, old_handle) == 0);
    assert(pool_resolve(&pool, old_handle, &resolved) == -1);

    assert(pool_alloc(&pool, &new_handle, &data) == 0);
    assert(new_handle.slot == old_handle.slot);
    assert(new_handle.generation != old_handle.generation);
    assert(pool_resolve(&pool, old_handle, &resolved) == -1);

    puts("handle: old generation rejected");
    puts("type07 passed");
    return 0;
}
```

Điểm quan trọng: handle không phải “pointer được đổi tên”. `slot` tìm object;
`generation` chứng minh identity đó vẫn thuộc đúng lifetime hiện tại.

</details>

### 8. Viết lại không nhìn lời giải

Đóng khối lời giải, viết lại câu trả lời/code từ trắng, rồi tự giải thích **type → miền giá trị → ownership/lifetime → representation modem**.


<!-- WIZARD_TYPE_7_BEGIN -->
### 9. Wizard Lab kiểu dữ liệu — Handle mutation workbench

**Câu hỏi mở:** Nếu tự thay slot/generation trong handle copy, validator phân loại stale/out-of-range/live thế nào?

```c
enum handle_status handle_check(const struct pool *p, struct buf_handle h);
struct buf_handle handle_with_generation(struct buf_handle h, uint16_t gen);
```

1. Flip slot.
2. Generation cũ/mới.
3. Churn slot rồi replay handle history.

**Observer:** Status reason và pool snapshot.

**Invariant:** Handle là value; mutate bản copy không sửa pool; raw pointer không qua boundary.

**Tự chế:** Tạo history recorder của mọi handle được phát/release.



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TYPE 7</strong></summary>

Lưu thành `practice/wizard_type_07.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define HANDLE_CAP 4U
struct buf_handle { uint16_t slot; uint16_t generation; };
struct pool { uint16_t generation[HANDLE_CAP]; uint8_t live[HANDLE_CAP]; };
enum handle_status { HANDLE_OK, HANDLE_RANGE, HANDLE_FREE, HANDLE_STALE };

static enum handle_status handle_check(const struct pool *p, struct buf_handle h)
{
    if (p == NULL || h.slot >= HANDLE_CAP) return HANDLE_RANGE;
    if (p->live[h.slot] == 0U) return HANDLE_FREE;
    if (p->generation[h.slot] != h.generation) return HANDLE_STALE;
    return HANDLE_OK;
}
static struct buf_handle handle_with_generation(struct buf_handle h, uint16_t gen)
{
    h.generation = gen; return h;
}

int main(void)
{
    struct pool p = {.generation={3U,0U,0U,0U},.live={1U,0U,0U,0U}};
    const struct buf_handle good = {0U,3U};
    assert(handle_check(&p,good) == HANDLE_OK);
    assert(handle_check(&p,handle_with_generation(good,2U)) == HANDLE_STALE);
    puts("wizard type 07 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_type_07.c -o wizard_type_07
./wizard_type_07
```

Expected cuối output:

```text
wizard type 07 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TYPE_7_END -->

---

#### Forge F13 — Bài kiểu dữ liệu 7: pointer tạm thời, handle bền qua boundary

Tự code validate handle, next-generation bỏ qua zero và resolver chỉ trả pointer
sau đủ slot/generation/length/ownership check.

<details>
<summary>Đáp án F13</summary>

```c
#include <stddef.h>
#include <stdint.h>
#define HC 4U
struct handle{uint16_t slot,generation;uint32_t length;};
struct hslot{uint16_t generation;int used;uint8_t data[64];};
uint16_t next_gen16(uint16_t x){++x;return x==0U?1U:x;}
void*resolve(struct hslot*s,size_t n,struct handle h)
{if((s==NULL)||((size_t)h.slot>=n)||(h.length>64U)||(h.generation==0U))return NULL;
 if(!s[h.slot].used||(s[h.slot].generation!=h.generation))return NULL;
 return s[h.slot].data;}
```

Fuzz `slot`, `generation`, `length`; không dereference nếu resolver trả NULL.

</details>

## 1B.41. Bài kiểu dữ liệu 8 — Struct không phải wire

### 1. Đề bài nhỏ

Cho:

```c
struct toy_header {
    uint8_t type;
    uint32_t length;
    bool valid;
};
```

Yêu cầu:

1. In `sizeof(struct toy_header)`.
2. Giải thích vì sao có thể lớn hơn 6.
3. Tự định nghĩa wire format đúng 6 byte.
4. Viết encode/decode từng field.
5. Reject `valid` khác `0/1`.

### 2. Tự đoán declaration/chương trình cần biểu diễn gì

C struct có padding; wire format 6 byte phải encode/decode field-by-field và validate bool.

### 3. Học đúng lượng kiến thức cần cho bài

- padding/alignment
- fixed wire offsets
- manual endian encode/decode

### 4. Tự làm / tự code

Chưa mở Bước 7. Viết câu trả lời hoặc file `practice/type_08.c`; với bài code, thêm test `main()` nhỏ nhất.

### 5. Chạy test / quan sát compiler và output

In `sizeof`, encode vector cố định, dump 6 byte, decode lại và test invalid valid-byte.

Với bài có C source, dùng warning nghiêm ngặt và sanitizer như hướng dẫn ở mục 1B.33A.

### 6. Debug

Nếu dùng `memcpy(&struct)`, output phụ thuộc ABI/padding/endianness.

### Sau khi code: ý nghĩa trong modem/baseband

Descriptor/PDU/file IQ phải có wire layout độc lập layout C trong RAM.

### 7. Đọc lời giải — ngay tại bài

<details>
<summary><strong>Mở lời giải Bài kiểu dữ liệu 8</strong></summary>

Wire format chọn đúng 6 byte:

```text
byte 0      type:u8
byte 1..4   length:u32 little-endian
byte 5      valid:u8, chỉ nhận 0 hoặc 1
```

Full program:

```c
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define TOY_WIRE_SIZE 6U

struct toy_header {
    uint8_t type;
    uint32_t length;
    bool valid;
};

static void put_u32_le(uint8_t output[4], uint32_t value)
{
    output[0] = (uint8_t)value;
    output[1] = (uint8_t)(value >> 8U);
    output[2] = (uint8_t)(value >> 16U);
    output[3] = (uint8_t)(value >> 24U);
}

static uint32_t get_u32_le(const uint8_t input[4])
{
    return ((uint32_t)input[0]) |
           ((uint32_t)input[1] << 8U) |
           ((uint32_t)input[2] << 16U) |
           ((uint32_t)input[3] << 24U);
}

static int toy_encode(const struct toy_header *header,
                      uint8_t output[TOY_WIRE_SIZE])
{
    if ((header == NULL) || (output == NULL)) {
        return -1;
    }

    output[0] = header->type;
    put_u32_le(&output[1], header->length);
    output[5] = header->valid ? 1U : 0U;
    return 0;
}

static int toy_decode(const uint8_t input[TOY_WIRE_SIZE],
                      size_t input_len,
                      struct toy_header *header)
{
    struct toy_header temporary;

    if ((input == NULL) || (header == NULL) ||
        (input_len != TOY_WIRE_SIZE)) {
        return -1;
    }

    if (input[5] > 1U) {
        return -1;
    }

    temporary.type = input[0];
    temporary.length = get_u32_le(&input[1]);
    temporary.valid = input[5] != 0U;

    *header = temporary; /* commit chỉ sau khi validate toàn frame */
    return 0;
}

int main(void)
{
    const struct toy_header original = {
        .type = 7U,
        .length = UINT32_C(0x12345678),
        .valid = true
    };
    struct toy_header decoded = {0U, 0U, false};
    uint8_t wire[TOY_WIRE_SIZE];

    printf("sizeof(struct toy_header)=%zu\n",
           sizeof(struct toy_header));

    assert(toy_encode(&original, wire) == 0);
    assert(wire[0] == 7U);
    assert(wire[1] == 0x78U);
    assert(wire[2] == 0x56U);
    assert(wire[3] == 0x34U);
    assert(wire[4] == 0x12U);
    assert(wire[5] == 1U);

    assert(toy_decode(wire, sizeof wire, &decoded) == 0);
    assert(decoded.type == original.type);
    assert(decoded.length == original.length);
    assert(decoded.valid == original.valid);

    wire[5] = 2U;
    assert(toy_decode(wire, sizeof wire, &decoded) == -1);

    puts("type08 passed");
    return 0;
}
```

`sizeof(struct toy_header)` có thể là 12 do alignment/padding, trong khi wire
format vẫn luôn đúng 6 byte. Đây chính là bằng chứng không được serialize struct
bằng `memcpy`.

#### Forge F14 — Bài kiểu dữ liệu 8: struct nội bộ không phải wire layout

Định nghĩa logical header `{type:u16,length:u32,tick:u64}` nhưng wire phải đúng
14 byte little-endian. Viết encode, decode, test truncated và trailing policy.

<details>
<summary>Đáp án F14</summary>

```c
#include <stddef.h>
#include <stdint.h>
struct hdr{uint16_t type;uint32_t length;uint64_t tick;};
static void putle(uint8_t*p,uint64_t v,size_t n)
{size_t k;for(k=0U;k<n;++k)p[k]=(uint8_t)(v>>(8U*k));}
static uint64_t getle(const uint8_t*p,size_t n)
{size_t k;uint64_t v=0U;for(k=0U;k<n;++k)v|=(uint64_t)p[k]<<(8U*k);return v;}
int hdr_encode(uint8_t*out,size_t cap,const struct hdr*h)
{if((out==NULL)||(h==NULL)||(cap<14U))return-1;
 putle(out,h->type,2U);putle(out+2U,h->length,4U);putle(out+6U,h->tick,8U);return 0;}
int hdr_decode(const uint8_t*in,size_t n,struct hdr*h)
{struct hdr t;if((in==NULL)||(h==NULL)||(n!=14U))return-1;
 t.type=(uint16_t)getle(in,2U);t.length=(uint32_t)getle(in+2U,4U);
 t.tick=getle(in+6U,8U);*h=t;return 0;}
```

`sizeof(struct hdr)` không được xuất hiện trong wire test; padding là chuyện ABI.

</details>

## Checklist hoàn thành chương kiểu dữ liệu

```text
[ ] Bài 1 đọc được prototype không đoán mò.
[ ] Bài 2 chọn type và giải thích được signedness/width.
[ ] Bài 3 phân biệt const data và const pointer.
[ ] Bài 4 chỉ cast sau range check.
[ ] Bài 5 dùng out-parameter và reject NULL/count=0.
[ ] Bài 6 tự gắn/gọi function pointer + context.
[ ] Bài 7 phân biệt borrowed pointer và transferable handle.
[ ] Bài 8 chứng minh struct memory layout khác wire layout.
[ ] Compile toàn bộ với -Wconversion -Werror và sanitizer.
[ ] Đóng solution rồi viết lại Bài 4–6 không nhìn.
```

</details>

### 8. Viết lại không nhìn lời giải

Đóng khối lời giải, viết lại câu trả lời/code từ trắng, rồi tự giải thích **type → miền giá trị → ownership/lifetime → representation modem**.


<!-- WIZARD_TYPE_8_BEGIN -->
### 9. Wizard Lab kiểu dữ liệu — Wire-layout painter

**Câu hỏi mở:** Nếu muốn thêm field/version vào wire format mà vẫn backward-parse, helper encode/decode nên tiến hóa ra sao?

```c
int toy_header_encode_v2(uint8_t *out, size_t cap,
                         const struct toy_header_v2 *h, size_t *written);
int toy_header_decode_any(const uint8_t *in, size_t len,
                          struct toy_header_v2 *out);
```

1. Encode v1/v2.
2. Unknown version.
3. Truncate mỗi field boundary.

**Observer:** Hex diff, consumed bytes và status.

**Invariant:** Không serialize padding; decode fail không commit partial output.

**Tự chế:** Thêm TLV extension toy nhưng giữ parser budget.



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TYPE 8</strong></summary>

Lưu thành `practice/wizard_type_08.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define V2_SIZE 8U
struct toy_header_v2 { uint8_t version, type; uint32_t length; uint16_t flags; };
static void put16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8U);}
static void put32(uint8_t *p,uint32_t v){for(size_t k=0;k<4U;++k)p[k]=(uint8_t)(v>>(8U*k));}
static uint16_t get16(const uint8_t *p){return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8U));}
static uint32_t get32(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8U)|((uint32_t)p[2]<<16U)|((uint32_t)p[3]<<24U);}

static int toy_header_encode_v2(uint8_t *out, size_t cap,
                                const struct toy_header_v2 *h, size_t *written)
{
    if (out == NULL || h == NULL || written == NULL || cap < V2_SIZE || h->version != 2U) return -1;
    out[0]=h->version;out[1]=h->type;put32(&out[2],h->length);put16(&out[6],h->flags);*written=V2_SIZE;return 0;
}
static int toy_header_decode_any(const uint8_t *in, size_t len, struct toy_header_v2 *out)
{
    struct toy_header_v2 tmp;
    if (in == NULL || out == NULL || len < 6U) return -1;
    if (in[0] == 1U) tmp=(struct toy_header_v2){2U,in[1],get32(&in[2]),0U};
    else if (in[0] == 2U && len >= V2_SIZE) tmp=(struct toy_header_v2){2U,in[1],get32(&in[2]),get16(&in[6])};
    else return -1;
    *out=tmp;return 0;
}

int main(void)
{
    const struct toy_header_v2 h={2U,7U,0x12345678U,0x55aaU};uint8_t wire[V2_SIZE];size_t n;struct toy_header_v2 decoded;
    assert(toy_header_encode_v2(wire,sizeof wire,&h,&n)==0&&n==V2_SIZE);
    assert(toy_header_decode_any(wire,n,&decoded)==0&&decoded.flags==0x55aaU&&decoded.length==h.length);
    puts("wizard type 08 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_type_08.c -o wizard_type_08
./wizard_type_08
```

Expected cuối output:

```text
wizard type 08 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TYPE_8_END -->

---

## 1B.42. Checklist trước khi dùng một type trong source thật

```text
[ ] Tôi biết miền giá trị thực, không chỉ miền của type.
[ ] Tôi biết field có thể âm hay không.
[ ] Tôi biết đây là wire width hay memory size.
[ ] Tôi biết pointer có thể NULL không.
[ ] Tôi biết caller/callee ai cấp storage.
[ ] Tôi biết pointer/data có cần const không.
[ ] Tôi biết lifetime kéo dài tới đâu.
[ ] Tôi biết object đi qua task/DMA/IPC bằng pointer hay handle.
[ ] Tôi đã check conversion trước cast.
[ ] Tôi đã check cộng/nhân size overflow.
[ ] Tôi không serialize padding/enum/bool bằng memcpy.
[ ] Tôi dùng atomic chứ không dùng volatile để sync.
[ ] Tôi hiểu alignment/cache requirement nếu là DMA object.
```

Nếu trả lời được checklist này, các prototype dài trong project sẽ không còn là
“cú pháp khó” mà trở thành bản mô tả ownership và data flow.


# PHẦN II — TỪ CONCEPT MODEM SANG TÍN HIỆU IQ

## Level 5 — Nhìn lại signal chain bằng code

### Modem là gì?

Trong ngữ cảnh điện thoại, modem/cellular baseband là subsystem xử lý communication với mạng di động.

Một cách cực kỳ giản lược:

```text
bits của protocol
    ↓
channel coding / modulation
    ↓
digital baseband waveform
    ↓
RF transceiver
    ↓
antenna
```

Ở chiều nhận:

```text
antenna
   ↓
RF front-end
   ↓
ADC tạo digital samples
   ↓
baseband DSP
   ↓
bit / PDU
   ↓
protocol stack
```

### Baseband là gì?

Baseband là representation của signal sau khi carrier RF cao tần đã được chuyển xuống gần 0 Hz.

Thay vì xử lý trực tiếp một carrier vài GHz trong phần mềm, DSP thường làm việc với complex signal:

```text
x[n] = I[n] + j Q[n]
```

Trong C:

```c
struct cpx16 {
    int16_t i;
    int16_t q;
};
```

Đây là lý do bài tập đầu tiên dùng I/Q.

### I và Q trực giác

Hãy tưởng tượng một vector quay trên mặt phẳng:

```text
          Q
          ^
          |
      *   |  sample = (I,Q)
          |
----------+----------> I
          |
```

Magnitude liên quan đến độ lớn; angle liên quan đến phase.

Không cần học số phức sâu ngay. Trước mắt cần hiểu:

```text
(I,Q) = một điểm/vector 2D
một waveform = chuỗi các vector theo thời gian
```

---

## Level 6 — Sampling và sample rate

Nếu sample rate là:

```text
Fs = 7,680,000 sample/s
```

thì mỗi sample cách nhau:

```text
Ts = 1/Fs
```

Firmware thường không lưu floating-point time cho từng sample. Nó giữ:

- `t0`: timestamp block
- `sample_rate_hz`
- `count`
- pointer tới samples

Ví dụ:

```c
struct iq_block {
    uint64_t t0;
    uint32_t sample_rate_hz;
    uint32_t count;
    struct cpx16 *samples;
};
```

### Bài 7 — Duration của IQ block

#### 1. Đề bài nhỏ

Viết hàm host:

```c
double duration_seconds(const struct iq_block *block);
```

trả về:

```text
count / sample_rate_hz
```

Nhớ check:

- `block == NULL`
- `sample_rate_hz == 0`

Sau đó tự trả lời:

> Vì sao code firmware thật có thể tránh `double`, nhưng simulator host vẫn có thể dùng `double` cho diagnostic?

#### 2. Tự đoán chương trình cần làm gì

Duration bằng số sample chia sample rate. Hàm phải từ chối object rỗng hoặc sample rate bằng 0 thay vì chia bừa.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- pointer tới struct chỉ đọc
- check NULL/zero
- ép sang `double` cho diagnostic host

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_07.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Test sample rate bình thường, rate 0 và pointer NULL. Ví dụ 480 samples/48000 Hz = 0.01 s.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_07.c \
    -o build/task_07
./build/task_07
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

Nếu bị division by zero, guard đặt quá muộn. Nếu kết quả 0, có thể integer division đã xảy ra trước cast.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** Sampling time.

**Bạn vừa code cái gì trong modem?** Một IQ block không chỉ có sample; nó còn tương ứng một khoảng thời gian vật lý. Duration nối `sample_rate` với số sample.

**Nó nằm ở đâu?** PHY timing.

```text
INPUT  → count + sample rate
KHỐI   → code của Bài 7
OUTPUT → thời lượng block
```

**Tại sao modem cần nó?** Modem là real-time. Sau này CFO phase advance, slot scheduling và deadline đều cần hiểu sample index tương ứng thời gian bao nhiêu.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 7 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

```c
double duration_seconds(const struct iq_block *block)
{
    if ((block == NULL) || (block->sample_rate_hz == 0U)) {
        return 0.0;
    }

    return (double)block->count / (double)block->sample_rate_hz;
}
```

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_7_BEGIN -->
#### 9. Wizard Lab — Time calculator thành công cụ sweep

**Ý nghĩ quái dị:** Nếu cùng một block được diễn giải bằng nhiều sample rate, deadline/tick thay đổi thế nào?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
int iq_duration_ns(size_t count, uint32_t rate_hz, uint64_t *out_ns);
int iq_slice_ticks(size_t first, size_t count, uint32_t rate_hz,
                   uint64_t tick_hz, uint64_t *start, uint64_t *duration);
```

1. **Bậc 1:** Sweep rate 15.36/30.72/61.44 MHz cho cùng count.
2. **Bậc 2:** Đổi count quanh một OFDM symbol và quan sát rounding.
3. **Bậc 3:** Tìm input gây overflow nếu nhân trước chia; sửa bằng thứ tự arithmetic an toàn.

**Observer bắt buộc:** Bảng count/rate/ns/ticks và sai số rounding.

**Invariant:** Rate zero fail; output không đổi khi fail; arithmetic không overflow.

**Tự chế spell tiếp theo:** Tạo helper chọn block size gần nhất với duration mong muốn.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 7</strong></summary>

Lưu thành `practice/wizard_task_07.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static int scale_ratio(size_t value, uint64_t numerator,
                       uint64_t denominator, uint64_t *out)
{
    if (denominator == 0U || out == NULL) return -1;
    if ((uint64_t)value > UINT64_MAX / numerator) return -1;
    *out = ((uint64_t)value * numerator) / denominator;
    return 0;
}

static int iq_duration_ns(size_t count, uint32_t rate_hz, uint64_t *out_ns)
{
    return scale_ratio(count, UINT64_C(1000000000), rate_hz, out_ns);
}

static int iq_slice_ticks(size_t first, size_t count, uint32_t rate_hz,
                          uint64_t tick_hz, uint64_t *start, uint64_t *duration)
{
    if (rate_hz == 0U || start == NULL || duration == NULL) return -1;
    if (scale_ratio(first, tick_hz, rate_hz, start) != 0) return -1;
    return scale_ratio(count, tick_hz, rate_hz, duration);
}

int main(void)
{
    uint64_t ns = 0U, start = 0U, duration = 0U;
    assert(iq_duration_ns(7680U, 7680000U, &ns) == 0 && ns == 1000000U);
    assert(iq_slice_ticks(100U, 50U, 1000U, 1000000U, &start, &duration) == 0);
    assert(start == 100000U && duration == 50000U);
    puts("wizard task 07 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_07.c -o wizard_task_07
./wizard_task_07
```

Expected cuối output:

```text
wizard task 07 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_7_END -->

---

#### Forge F15 — Bài 7: duration phải có đơn vị và overflow contract

Tự code duration nanosecond từ `(sample_count,sample_rate_hz)`, cộng vào `t0`
có check overflow, và phép ngược tính số sample tối thiểu để phủ duration.

<details>
<summary>Đáp án F15</summary>

```c
#include <stdint.h>
int duration_ns(uint64_t count,uint32_t rate,uint64_t*out)
{uint64_t sec,rem,ns;if((out==NULL)||(rate==0U))return-1;
 sec=count/rate;rem=count%rate;
 if(sec>UINT64_MAX/UINT64_C(1000000000))return-1;
 ns=sec*UINT64_C(1000000000)+(rem*UINT64_C(1000000000))/rate;
 *out=ns;return 0;}
int block_end(uint64_t t0,uint64_t duration,uint64_t*out)
{if((out==NULL)||(duration>UINT64_MAX-t0))return-1;*out=t0+duration;return 0;}
int samples_to_cover(uint64_t ns,uint32_t rate,uint64_t*out)
{uint64_t q,r,base;if((out==NULL)||(rate==0U))return-1;
 q=ns/UINT64_C(1000000000);r=ns%UINT64_C(1000000000);
 if(q>UINT64_MAX/rate)return-1;base=q*rate;
 if(r!=0U){uint64_t extra=(r*rate+UINT64_C(999999999))/UINT64_C(1000000000);
  if(extra>UINT64_MAX-base)return-1;base+=extra;}*out=base;return 0;}
```

Với profile IQ block nhỏ, `rem*1e9` an toàn vì `rem<rate<=UINT32_MAX`; vẫn phải
ghi invariant này. Test `rate=0`, exact second và fraction không chia hết.

</details>

# PHẦN III — DIGITAL COMMUNICATION TỪ BIT ĐẾN IQ

## Level 7 — BPSK/QPSK trước, đừng nhảy FFT

### QPSK

QPSK map 2 bit thành một complex symbol.

Một mapping học tập đơn giản:

```text
00 -> (+A,+A)
01 -> (+A,-A)
10 -> (-A,+A)
11 -> (-A,-A)
```

Trong fixed-point có thể chọn:

```c
A = 11585
```

### Bài 8 — QPSK mapper

#### 1. Đề bài nhỏ

Viết:

```c
void qpsk_map(uint8_t b0, uint8_t b1, struct cpx16 *out);
```

Không cần normalize bằng floating point.

#### 2. Tự đoán chương trình cần làm gì

Hai bit chọn dấu của I và Q để tạo một điểm constellation. Hàm ghi symbol qua out-parameter.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- mask bit
- constellation mapping
- out-parameter và NULL guard

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_08.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Map đủ `00,01,10,11`; kiểm tra mỗi output chỉ có hai mức biên độ và đúng dấu.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_08.c \
    -o build/task_08
./build/task_08
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

Nếu hai input ra cùng symbol, in `b0 & 1U`, `b1 & 1U`; kiểm tra bạn gán cả I lẫn Q.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** Digital modulation TX.

**Bạn vừa code cái gì trong modem?** QPSK mapper gom bit thành complex symbol. Đây là bước đầu tiên biến data logic thành điểm trên constellation.

**Nó nằm ở đâu?** PHY transmitter.

```text
INPUT  → 2 bit mỗi symbol
KHỐI   → code của Bài 8
OUTPUT → một complex symbol
```

**Tại sao modem cần nó?** Nếu bit→symbol mapping không nhất quán, receiver có thể thu waveform hoàn hảo nhưng vẫn khôi phục sai bit. Gray mapping giảm số bit sai khi nhầm sang điểm lân cận.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 8 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

```c
void qpsk_map(uint8_t b0, uint8_t b1, struct cpx16 *out)
{
    const int16_t a = 11585;

    if (out == NULL) {
        return;
    }

    out->i = ((b0 & 1U) == 0U) ? a : (int16_t)-a;
    out->q = ((b1 & 1U) == 0U) ? a : (int16_t)-a;
}

void qpsk_demap(struct cpx16 x, uint8_t *b0, uint8_t *b1)
{
    if ((b0 == NULL) || (b1 == NULL)) {
        return;
    }

    *b0 = x.i < 0 ? 1U : 0U;
    *b1 = x.q < 0 ? 1U : 0U;
}
```

Unit test:

```c
#include <assert.h>

for (uint8_t b0 = 0U; b0 < 2U; ++b0) {
    for (uint8_t b1 = 0U; b1 < 2U; ++b1) {
        struct cpx16 s;
        uint8_t r0;
        uint8_t r1;

        qpsk_map(b0, b1, &s);
        qpsk_demap(s, &r0, &r1);

        assert(r0 == b0);
        assert(r1 == b1);
    }
}
```

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_8_BEGIN -->
#### 9. Wizard Lab — Constellation do chính bạn định nghĩa

**Ý nghĩ quái dị:** Nếu mapping QPSK không hard-code mà nằm trong table có thể xoay/đổi quadrant thì sao?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
struct qpsk_table { struct cpx16 point[4]; };
int qpsk_map_table(const struct qpsk_table *t, uint8_t bits, struct cpx16 *out);
void qpsk_table_rotate_90(struct qpsk_table *t);
```

1. **Bậc 1:** Sinh table chuẩn rồi rotate toàn constellation.
2. **Bậc 2:** Hoán đổi label 01 và 11; dự đoán round-trip với demapper cũ.
3. **Bậc 3:** Tự viết validator phát hiện hai label trùng point.

**Observer bắt buộc:** Print label→I/Q, minimum distance và exhaustive round-trip.

**Invariant:** Table hợp lệ có bốn point phân biệt; mapper không đọc bit ngoài 2 LSB.

**Tự chế spell tiếp theo:** Tạo table từ hai axis amplitude thay vì nhập bốn point.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 8</strong></summary>

Lưu thành `practice/wizard_task_08.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

struct cpx16 { int16_t i; int16_t q; };
struct qpsk_table { struct cpx16 point[4]; };

static int qpsk_map_table(const struct qpsk_table *t, uint8_t bits,
                          struct cpx16 *out)
{
    if (t == NULL || out == NULL || bits > 3U) return -1;
    *out = t->point[bits];
    return 0;
}

static void qpsk_table_rotate_90(struct qpsk_table *t)
{
    if (t == NULL) return;
    for (size_t k = 0; k < 4U; ++k) {
        const int16_t old_i = t->point[k].i;
        t->point[k].i = (int16_t)-t->point[k].q;
        t->point[k].q = old_i;
    }
}

int main(void)
{
    struct qpsk_table t = {{{1,1},{-1,1},{-1,-1},{1,-1}}};
    struct cpx16 out;
    assert(qpsk_map_table(&t, 2U, &out) == 0 && out.i == -1 && out.q == -1);
    qpsk_table_rotate_90(&t);
    assert(qpsk_map_table(&t, 0U, &out) == 0 && out.i == -1 && out.q == 1);
    puts("wizard task 08 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_08.c -o wizard_task_08
./wizard_task_08
```

Expected cuối output:

```text
wizard task 08 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_8_END -->

---

#### Forge F16 — Bài 8: QPSK mapper đổi bit-order và amplitude

Tự code mapper một dibit, mapper byte MSB-first thành bốn symbol, và biến thể
LSB-first. Hai biến thể phải khác output với fixture `0x81`.

<details>
<summary>Đáp án F16</summary>

```c
#include <stddef.h>
#include <stdint.h>
struct cpx16{int16_t i,q;};
struct cpx16 qpsk(uint8_t b0,uint8_t b1,int16_t a)
{return(struct cpx16){b0?-a:a,b1?-a:a};}
int qpsk_byte(uint8_t byte,int msb_first,int16_t a,struct cpx16 out[4])
{size_t k;if((out==NULL)||(a<=0))return-1;
 for(k=0U;k<4U;++k){
  unsigned si=msb_first?(unsigned)(7U-2U*k):(unsigned)(2U*k);
  unsigned sq=msb_first?(unsigned)(6U-2U*k):(unsigned)(2U*k+1U);
  out[k]=qpsk((uint8_t)((byte>>si)&1U),(uint8_t)((byte>>sq)&1U),a);}return 0;}
```

Chọn và ghi rõ thứ tự bit trong dibit. Nếu contract dùng `(MSB,MSB-1)`, sửa
hai shift cho nhất quán; test round-trip mới là nguồn chân lý.

</details>

### Bài 9 — QPSK demapper

#### 1. Đề bài nhỏ

Viết:

```c
void qpsk_demap(struct cpx16 x, uint8_t *b0, uint8_t *b1);
```

Decision rule đơn giản:

```text
I < 0 -> bit 1
I >=0 -> bit 0
Q < 0 -> bit 1
Q >=0 -> bit 0
```

#### 2. Tự đoán chương trình cần làm gì

Receiver nhìn dấu I/Q để khôi phục hai bit. Với channel sạch, `demap(map(bits))` phải trả lại đúng bits.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- hard decision theo dấu
- hai out-parameter
- property test đủ bốn cặp bit

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_09.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Exhaustive 4 cặp bit: `demap(map(bits)) == bits`; thêm điểm nằm đúng trục để xác nhận quy ước `>= 0`.

Chi tiết test đã có trong đề:

**Test bắt buộc**

Với mọi cặp bit:

```text
00, 01, 10, 11
```

phải có:

```text
demap(map(bits)) == bits
```

Đây là pattern test cực quan trọng: **round-trip test**.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_09.c \
    -o build/task_09
./build/task_09
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

Nếu chỉ một cặp fail, in bits→I/Q→bits trên cùng dòng để xác định trục I hay Q bị đảo.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** Digital demodulation RX.

**Bạn vừa code cái gì trong modem?** Demapper thực hiện hướng ngược: quan sát điểm constellation và quyết định bit gần nhất.

**Nó nằm ở đâu?** PHY receiver.

```text
INPUT  → complex symbol nhận được
KHỐI   → code của Bài 9
OUTPUT → ước lượng bit
```

**Tại sao modem cần nó?** Trong modem thật thường dùng soft decision/LLR thay vì quyết định cứng đơn giản. Bài này cố ý làm hard decision trước để thấy bản chất detection.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 9 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

```c
void qpsk_map(uint8_t b0, uint8_t b1, struct cpx16 *out)
{
    const int16_t a = 11585;

    if (out == NULL) {
        return;
    }

    out->i = ((b0 & 1U) == 0U) ? a : (int16_t)-a;
    out->q = ((b1 & 1U) == 0U) ? a : (int16_t)-a;
}

void qpsk_demap(struct cpx16 x, uint8_t *b0, uint8_t *b1)
{
    if ((b0 == NULL) || (b1 == NULL)) {
        return;
    }

    *b0 = x.i < 0 ? 1U : 0U;
    *b1 = x.q < 0 ? 1U : 0U;
}
```

Unit test:

```c
#include <assert.h>

for (uint8_t b0 = 0U; b0 < 2U; ++b0) {
    for (uint8_t b1 = 0U; b1 < 2U; ++b1) {
        struct cpx16 s;
        uint8_t r0;
        uint8_t r1;

        qpsk_map(b0, b1, &s);
        qpsk_demap(s, &r0, &r1);

        assert(r0 == b0);
        assert(r1 == b1);
    }
}
```

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_9_BEGIN -->
#### 9. Wizard Lab — Demapper có vùng không chắc

**Ý nghĩ quái dị:** Hard decision ép mọi point thành bit; nếu point quá gần trục thì trả erasure/độ tin cậy ra sao?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
struct bit_decision { uint8_t bit; uint16_t confidence; int valid; };
struct bit_decision decide_axis(int16_t value, uint16_t dead_zone);
```

1. **Bậc 1:** Sweep value từ -5 tới +5 với dead_zone 0,2,4.
2. **Bậc 2:** Thêm noise toy vào bốn point và đếm invalid decision.
3. **Bậc 3:** So hard bits với decision có confidence.

**Observer bắt buộc:** Decision table và histogram confidence.

**Invariant:** Ngoài dead zone, sign quyết định bit; tăng dead zone không được giảm invalid count.

**Tự chế spell tiếp theo:** Đổi confidence thành khoảng cách tới threshold và feed vào LLR toy.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 9</strong></summary>

Lưu thành `practice/wizard_task_09.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

struct bit_decision { uint8_t bit; uint16_t confidence; int valid; };

static struct bit_decision decide_axis(int16_t value, uint16_t dead_zone)
{
    const int32_t magnitude = value < 0 ? -(int32_t)value : (int32_t)value;
    struct bit_decision out = { .bit = value < 0 ? 1U : 0U,
                                .confidence = (uint16_t)magnitude,
                                .valid = magnitude > dead_zone };
    return out;
}

int main(void)
{
    const struct bit_decision weak = decide_axis(3, 5U);
    const struct bit_decision strong = decide_axis(-20, 5U);
    assert(weak.valid == 0);
    assert(strong.valid != 0 && strong.bit == 1U && strong.confidence == 20U);
    puts("wizard task 09 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_09.c -o wizard_task_09
./wizard_task_09
```

Expected cuối output:

```text
wizard task 09 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_9_END -->

---

#### Forge F17 — Bài 9: demapper phải biểu diễn cả “không chắc”

Tự code hard bit, hard bit có erasure threshold và soft LLR signed. Quy ước:
LLR dương là bit 0, âm là bit 1.

<details>
<summary>Đáp án F17</summary>

```c
#include <limits.h>
#include <stdint.h>
int hard_bit(int16_t axis){return axis<0?1:0;}
int hard_or_erasure(int16_t axis,uint16_t threshold,int*out)
{int32_t w=axis;uint32_t a=(uint32_t)(w<0?-w:w);
 if(out==NULL)return-1;if(a<threshold)return 1;*out=hard_bit(axis);return 0;}
int16_t llr_axis(int16_t axis,int16_t gain)
{int32_t x=(int32_t)axis*gain/INT16_MAX;
 if(x>INT16_MAX)return INT16_MAX;if(x<INT16_MIN)return INT16_MIN;return(int16_t)x;}
```

Test `axis=-1,0,+1`, đúng tại threshold và `INT16_MIN`. Đừng gọi `abs(int16_t)`.

</details>

## Level 8 — 16-QAM và constellation

16-QAM map 4 bit/symbol.

Thay vì chỉ dấu `+/-`, mỗi axis có nhiều mức amplitude.

Ví dụ Gray-like levels:

```text
00 -> +3
01 -> +1
11 -> -1
10 -> -3
```

Trong fixed-point, scale thành integer.

### Bài 10 — 16-QAM round trip

#### 1. Đề bài nhỏ

Tạo API:

```c
void qam16_map(uint8_t nibble, struct cpx16 *out);
uint8_t qam16_demap(struct cpx16 x);
```

Test toàn bộ:

```c
for (uint8_t x = 0; x < 16; ++x) {
    map(x)
    assert(demap(...) == x)
}
```

#### 2. Tự đoán chương trình cần làm gì

Bốn bit chọn một trong 16 điểm. Mapper và demapper phải dùng cùng quy ước để toàn bộ 16 giá trị round trip được.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- nibble và lookup table
- ngưỡng quyết định
- exhaustive test 0..15

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_10.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Loop từ 0 đến 15 và assert round trip; in constellation để phát hiện table đảo trục.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_10.c \
    -o build/task_10
./build/task_10
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

Nếu round trip fail hàng loạt, mapper/demapper đang dùng table/ngưỡng khác nhau. In nibble và hai axis index.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Điều đang học thật sự**

Không phải thuộc table.

Bạn đang học pipeline:

```text
bits → symbol → channel → estimated symbol → bits
```

OFDM chỉ đặt rất nhiều symbol này lên nhiều subcarrier.

**Bài này thuộc nhóm:** Higher-order QAM.

**Bạn vừa code cái gì trong modem?** 16-QAM mang nhiều bit trên một symbol hơn QPSK bằng cách dùng nhiều mức amplitude/phase hơn.

**Nó nằm ở đâu?** PHY modulation.

```text
INPUT  → 4 bit
KHỐI   → code của Bài 10
OUTPUT → một trong 16 constellation points rồi round-trip về bit
```

**Tại sao modem cần nó?** Trade-off chính: spectral efficiency cao hơn nhưng khoảng cách constellation nhỏ hơn nên nhạy noise hơn. Đây là trực giác nền của adaptive modulation.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 10 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

```c
static int16_t qam_axis(uint8_t pair)
{
    static const int16_t level[4] = {
        9486, 3162, -9486, -3162
    };

    return level[pair & 3U];
}

void qam16_map(uint8_t bits, struct cpx16 *out)
{
    if (out == NULL) {
        return;
    }

    out->i = qam_axis((uint8_t)((bits >> 2U) & 3U));
    out->q = qam_axis((uint8_t)(bits & 3U));
}

static uint8_t qam_axis_demap(int16_t x)
{
    if (x >= 6324) return 0U;
    if (x >= 0) return 1U;
    if (x <= -6324) return 2U;
    return 3U;
}

uint8_t qam16_demap(struct cpx16 x)
{
    return (uint8_t)((qam_axis_demap(x.i) << 2U) |
                     qam_axis_demap(x.q));
}
```

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_10_BEGIN -->
#### 9. Wizard Lab — Constellation sculptor 16-QAM

**Ý nghĩ quái dị:** Nếu muốn mirror, rotate, compress riêng outer points hoặc đổi label table theo ý thì sao?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
void qam16_build_table(struct cpx16 out[16], int16_t inner, int16_t outer);
void constellation_mirror_i(struct cpx16 *points, size_t count);
int constellation_validate_unique(const struct cpx16 *points, size_t count);
```

1. **Bậc 1:** Đổi tỉ lệ inner/outer rồi đo minimum Manhattan distance.
2. **Bậc 2:** Mirror I nhưng giữ demapper cũ; liệt kê nibble sai.
3. **Bậc 3:** Tạo table có hai point trùng và bắt validator reject.

**Observer bắt buộc:** Bảng 16 label, unique-count, minimum distance và round-trip failures.

**Invariant:** Transform hình học không được làm mất point nếu muốn invertible.

**Tự chế spell tiếp theo:** Viết nearest-point demapper dùng table thay vì branch threshold cố định.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 10</strong></summary>

Lưu thành `practice/wizard_task_10.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

struct cpx16 { int16_t i; int16_t q; };

static void qam16_build_table(struct cpx16 out[16], int16_t inner, int16_t outer)
{
    const int16_t level[4] = { (int16_t)-outer, (int16_t)-inner, inner, outer };
    if (out == NULL) return;
    for (size_t q = 0; q < 4U; ++q)
        for (size_t i = 0; i < 4U; ++i)
            out[q * 4U + i] = (struct cpx16){level[i], level[q]};
}

static void constellation_mirror_i(struct cpx16 *points, size_t count)
{
    if (points == NULL) return;
    for (size_t k = 0; k < count; ++k) points[k].i = (int16_t)-points[k].i;
}

static int constellation_validate_unique(const struct cpx16 *p, size_t n)
{
    if (p == NULL) return -1;
    for (size_t a = 0; a < n; ++a)
        for (size_t b = a + 1U; b < n; ++b)
            if (p[a].i == p[b].i && p[a].q == p[b].q) return -1;
    return 0;
}

int main(void)
{
    struct cpx16 table[16];
    qam16_build_table(table, 1, 3);
    assert(constellation_validate_unique(table, 16U) == 0);
    constellation_mirror_i(table, 16U);
    assert(constellation_validate_unique(table, 16U) == 0);
    puts("wizard task 10 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_10.c -o wizard_task_10
./wizard_task_10
```

Expected cuối output:

```text
wizard task 10 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_10_END -->

---

#### Forge F18 — Bài 10: 16-QAM Gray phải round-trip đủ 16 điểm

Tự code map hai bit trên một axis, inverse nearest-level, rồi map/demap đủ
`I/Q`. Sweep mọi nibble `0..15`, không chỉ bốn góc.

<details>
<summary>Đáp án F18</summary>

```c
#include <stdint.h>
struct cpx16{int16_t i,q;};
static int16_t axis_map(uint8_t bits,int16_t a)
{switch(bits&3U){case 0U:return(int16_t)(3*a);case 1U:return a;
 case 3U:return(int16_t)-a;default:return(int16_t)(-3*a);}}
static uint8_t axis_demap(int16_t x,int16_t a)
{if(x>=2*a)return 0U;if(x>=0)return 1U;if(x>=-2*a)return 3U;return 2U;}
struct cpx16 qam16_map(uint8_t nibble,int16_t a)
{return(struct cpx16){axis_map((uint8_t)(nibble>>2),a),axis_map(nibble,a)};}
uint8_t qam16_demap(struct cpx16 x,int16_t a)
{return(uint8_t)((axis_demap(x.i,a)<<2)|axis_demap(x.q,a));}
```

Giới hạn `a<=10922` để `3*a` nằm trong `int16_t`; API production phải reject
amplitude ngoài range thay vì tin caller như snippet tập trung này.

</details>

## Level 9 — CRC: “Tôi nhận đúng chưa?”

CRC không sửa lỗi. Nó giúp phát hiện nhiều dạng corruption.

Pipeline cơ bản:

```text
payload
  ↓
CRC(payload)
  ↓
[payload | crc]
  ↓
channel
  ↓
recompute CRC
  ↓
match ? accept : drop/retry
```

### Bài 11 — CRC API trước khi implementation

#### 1. Đề bài nhỏ

Không cần tự phát minh polynomial.

Thiết kế API:

```c
uint32_t crc24a(const uint8_t *data, size_t len);
```

Viết trước các test property:

1. Cùng bytes → cùng CRC.
2. Flip một bit → thường CRC khác.
3. `len=0` không đọc memory ngoài.
4. `data=NULL, len>0` phải có behavior xác định.

Sau khi đã tự chạy test và debug ít nhất một lần, mới mở Bước 7 ngay dưới bài này.

#### 2. Tự đoán chương trình cần làm gì

CRC đi tuần tự qua từng byte/bit và tạo fingerprint 24-bit. Cùng input phải deterministic; đổi một bit thường đổi kết quả.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- shift/XOR theo polynomial
- loop byte rồi loop bit
- API behavior với NULL/length

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_11.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Cùng buffer gọi hai lần phải giống; flip một bit phải đổi CRC; test `len=0` và `NULL,len>0`.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_11.c \
    -o build/task_11
./build/task_11
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

Trace một byte: in CRC trước XOR và sau từng bit. Kiểm tra mask 24-bit và dùng unsigned shift.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** Error detection boundary.

**Bạn vừa code cái gì trong modem?** CRC gắn một dấu kiểm vào transport block để RX biết block sau decode có đáng tin hay không.

**Nó nằm ở đâu?** PHY coding/check.

```text
INPUT  → payload bytes/bits
KHỐI   → code của Bài 11
OUTPUT → CRC value hoặc PASS/FAIL
```

**Tại sao modem cần nó?** Thiết kế API trước implementation buộc bạn xác định coverage, initial value, bit order và output contract. Trong modem, CRC FAIL thường kích hoạt drop/HARQ chứ không được đẩy dữ liệu hỏng lên stack.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 11 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

```c
uint32_t crc24a(const uint8_t *data, size_t len)
{
    uint32_t crc = 0U;

    if ((data == NULL) && (len != 0U)) {
        return 0U;
    }

    for (size_t i = 0U; i < len; ++i) {
        crc ^= (uint32_t)data[i] << 16U;

        for (uint32_t bit = 0U; bit < 8U; ++bit) {
            crc <<= 1U;
            if ((crc & 0x1000000U) != 0U) {
                crc ^= 0x1864CFBU;
            }
        }
    }

    return crc & 0xFFFFFFU;
}
```

Đừng chỉ nhớ polynomial. Trace một byte bằng tay để hiểu shift/XOR process.

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_11_BEGIN -->
#### 9. Wizard Lab — CRC microscope

**Ý nghĩ quái dị:** Thay vì chỉ nhận CRC cuối, mình có thể nhìn CRC state sau từng byte/bit để tự khám phá avalanche không?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
typedef void (*crc_trace_fn)(size_t byte, unsigned bit, uint32_t state, void *ctx);
uint32_t crc24a_trace(const uint8_t *data, size_t len,
                      crc_trace_fn trace, void *ctx);
```

1. **Bậc 1:** Trace một byte 0x00 và 0x01; tìm bit đầu tiên state khác.
2. **Bậc 2:** Flip từng bit payload và đếm Hamming distance của CRC.
3. **Bậc 3:** Tắt trace bằng NULL; CRC cuối phải giống implementation thường.

**Observer bắt buộc:** State table, XOR diff và Hamming distance.

**Invariant:** Observer callback không được thay đổi thuật toán; trace on/off cho cùng CRC.

**Tự chế spell tiếp theo:** Viết helper tìm bit flip tạo CRC diff nhỏ nhất trong vector toy.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 11</strong></summary>

Lưu thành `practice/wizard_task_11.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

typedef void (*crc_trace_fn)(size_t byte, unsigned bit,
                             uint32_t state, void *ctx);

static uint32_t crc24a_trace(const uint8_t *data, size_t len,
                             crc_trace_fn trace, void *ctx)
{
    uint32_t crc = 0U;
    if (data == NULL && len != 0U) return 0U;
    for (size_t byte = 0; byte < len; ++byte) {
        for (unsigned bit = 0; bit < 8U; ++bit) {
            const uint32_t input = (uint32_t)((data[byte] >> (7U - bit)) & 1U);
            const uint32_t feedback = ((crc >> 23U) & 1U) ^ input;
            crc = (crc << 1U) & UINT32_C(0x00ffffff);
            if (feedback != 0U) crc ^= UINT32_C(0x00864cfb);
            if (trace != NULL) trace(byte, bit, crc, ctx);
        }
    }
    return crc;
}

static void count_trace(size_t byte, unsigned bit, uint32_t state, void *ctx)
{
    size_t *calls = ctx;
    (void)byte; (void)bit; (void)state;
    if (calls != NULL) ++*calls;
}

int main(void)
{
    const uint8_t data[2] = {0x12U, 0x34U};
    size_t calls = 0U;
    const uint32_t a = crc24a_trace(data, 2U, count_trace, &calls);
    const uint32_t b = crc24a_trace(data, 2U, NULL, NULL);
    assert(a == b && calls == 16U);
    puts("wizard task 11 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_11.c -o wizard_task_11
./wizard_task_11
```

Expected cuối output:

```text
wizard task 11 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_11_END -->

---

#### Forge F19 — Bài 11: CRC là contract bit-order, coverage và verify

Tự code CRC-24A MSB-first, API incremental nhiều chunk và verify payload + ba
byte CRC. Test split tại mọi offset phải giống one-shot.

<details>
<summary>Đáp án F19</summary>

```c
#include <stddef.h>
#include <stdint.h>
#define CRC24_MASK UINT32_C(0xFFFFFF)
#define CRC24_POLY UINT32_C(0x864CFB)
uint32_t crc24a_update(uint32_t crc,const uint8_t*p,size_t n)
{size_t k;unsigned b;crc&=CRC24_MASK;
 for(k=0U;k<n;++k)for(b=0U;b<8U;++b){
  uint32_t bit=(p[k]>>(7U-b))&1U;uint32_t feedback=((crc>>23)^bit)&1U;
  crc=(crc<<1)&CRC24_MASK;if(feedback)crc^=CRC24_POLY;}return crc;}
uint32_t crc24a(const uint8_t*p,size_t n){return crc24a_update(0U,p,n);}
int crc24a_verify(const uint8_t*p,size_t n,const uint8_t tag[3])
{uint32_t c;if(((p==NULL)&&(n!=0U))||(tag==NULL))return-1;c=crc24a(p,n);
 return(tag[0]==(uint8_t)(c>>16)&&tag[1]==(uint8_t)(c>>8)&&tag[2]==(uint8_t)c)?1:0;}
```

Bug injection: LSB-first, init all-ones, phủ cả tag, đổi polynomial. Mỗi bug cần
một fixture độc lập, không sinh expected bằng chính hàm đang test.

</details>

# PHẦN IV — OFDM TỪ TRỰC GIÁC ĐẾN CODE

## Level 10 — Tại sao OFDM?

Giả sử ta có nhiều QPSK symbol:

```text
S[-3] S[-2] S[-1] DC S[1] S[2] S[3]
```

Ta muốn mỗi symbol nằm trên một subcarrier khác nhau.

Frequency-domain resource bins:

```text
bin 0  1  2  3 ... N-1
      ↑ đặt complex symbols
```

IFFT biến frequency bins thành time-domain samples:

```text
frequency grid --IFFT--> time waveform
```

Receiver:

```text
time waveform --FFT--> frequency bins
```

Vậy flow tối thiểu:

```text
bits
 ↓
QPSK/QAM
 ↓
map subcarriers
 ↓
IFFT
 ↓
cyclic prefix
 ↓
IQ samples
```

Receiver đảo lại.

---

## Level 11 — DFT bằng code chậm trước FFT

Đừng học FFT bằng cách copy radix-2 ngay.

Trước tiên viết DFT size 4/8 bằng floating point trên host để thấy concept.

### Bài 12 — DFT host mini

#### 1. Đề bài nhỏ

Dùng type riêng:

```c
struct cpxf {
    double re;
    double im;
};
```

Viết DFT forward cho `N=4`.

Input đầu tiên:

```text
[1, 0, 0, 0]
```

Expected magnitude frequency bins bằng nhau.

Input thứ hai:

```text
[1, 1, 1, 1]
```

Expected năng lượng chủ yếu ở DC.

Mục tiêu là hiểu transform, không phải performance.

#### 2. Tự đoán chương trình cần làm gì

DFT nhận N sample time-domain và tạo N bin frequency-domain. Impulse cho phổ phẳng; tín hiệu hằng tập trung ở DC.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- số phức `double`
- góc `-2πkn/N`
- `sin`, `cos`, accumulation và tolerance

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_12.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Test impulse `[1,0,0,0]` và constant `[1,1,1,1]`; so real/imag bằng tolerance, không dùng `==` cho `double`.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_12.c \
    -o build/task_12 -lm
./build/task_12
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

Nếu sign imag ngược, kiểm tra dấu trong exponent forward. Nếu gần 0 nhưng không bằng 0, dùng tolerance.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** Frequency-domain thinking.

**Bạn vừa code cái gì trong modem?** DFT cho bạn thấy một block time-domain có thể được biểu diễn thành các frequency bin.

**Nó nằm ở đâu?** PHY DSP foundation.

```text
INPUT  → N complex samples
KHỐI   → code của Bài 12
OUTPUT → N complex frequency coefficients
```

**Tại sao modem cần nó?** OFDM được xây trên đúng cặp biến đổi này. DFT O(N²) dùng để học/reference; FFT sau đó chỉ là cách tính cùng kết quả hiệu quả hơn.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 12 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

```c
#include <math.h>
#include <stddef.h>

#define PI 3.14159265358979323846

struct cpxf {
    double re;
    double im;
};

void dft4(const struct cpxf in[4], struct cpxf out[4])
{
    for (size_t k = 0U; k < 4U; ++k) {
        double re = 0.0;
        double im = 0.0;

        for (size_t n = 0U; n < 4U; ++n) {
            double a = -2.0 * PI * (double)k * (double)n / 4.0;
            double c = cos(a);
            double s = sin(a);

            re += in[n].re * c - in[n].im * s;
            im += in[n].re * s + in[n].im * c;
        }

        out[k].re = re;
        out[k].im = im;
    }
}
```

Test 1:

```c
struct cpxf x[4] = {
    {1.0, 0.0},
    {0.0, 0.0},
    {0.0, 0.0},
    {0.0, 0.0}
};
```

Kết quả lý tưởng:

```text
X[0] = 1 + j0
X[1] = 1 + j0
X[2] = 1 + j0
X[3] = 1 + j0
```

Test 2 với `[1,1,1,1]` cho `X[0]≈4`, các bin còn lại gần 0.

Điểm cần hiểu: DFT này là **reference oracle** để kiểm FFT. Nó chậm nhưng dễ tin cậy hơn khi debug.

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_12_BEGIN -->
#### 9. Wizard Lab — Tone-to-bin laboratory

**Ý nghĩ quái dị:** Nếu tự sinh tone cho từng k rồi chạy DFT, mình có thể 'ra lệnh' năng lượng xuất hiện ở bin nào không?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
void make_complex_tone(struct cpxf *x, size_t n, int bin);
size_t dft_peak_bin(const struct cpxf *freq, size_t n);
void spectrum_dump(const struct cpxf *freq, size_t n);
```

1. **Bậc 1:** Sinh tone k=0,1,-1 và dự đoán peak.
2. **Bậc 2:** Cộng hai tone rồi tìm hai peak lớn nhất.
3. **Bậc 3:** Cắt tone còn nửa block và quan sát leakage.

**Observer bắt buộc:** Magnitude per bin và ranked peak list.

**Invariant:** Tone nguyên chu kỳ trên cửa sổ lý tưởng tập trung ở bin tương ứng trong tolerance.

**Tự chế spell tiếp theo:** Viết helper chọn phase ban đầu và kiểm tra magnitude không đổi.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 12</strong></summary>

Lưu thành `practice/wizard_task_12.c`:

```c
#include <assert.h>
#include <math.h>
#include <stdio.h>

#define PI 3.14159265358979323846
struct cpxf { double i; double q; };

static void make_complex_tone(struct cpxf *x, size_t n, int bin)
{
    if (x == NULL || n == 0U) return;
    for (size_t k = 0; k < n; ++k) {
        const double phase = 2.0 * PI * (double)bin * (double)k / (double)n;
        x[k] = (struct cpxf){cos(phase), sin(phase)};
    }
}

static void dft(const struct cpxf *time, struct cpxf *freq, size_t n)
{
    for (size_t k = 0; k < n; ++k) {
        freq[k] = (struct cpxf){0.0, 0.0};
        for (size_t m = 0; m < n; ++m) {
            const double p = -2.0 * PI * (double)k * (double)m / (double)n;
            freq[k].i += time[m].i * cos(p) - time[m].q * sin(p);
            freq[k].q += time[m].i * sin(p) + time[m].q * cos(p);
        }
    }
}

static size_t dft_peak_bin(const struct cpxf *freq, size_t n)
{
    size_t best = 0U;
    double energy = -1.0;
    if (freq == NULL) return 0U;
    for (size_t k = 0; k < n; ++k) {
        const double e = freq[k].i * freq[k].i + freq[k].q * freq[k].q;
        if (e > energy) { energy = e; best = k; }
    }
    return best;
}

static void spectrum_dump(const struct cpxf *freq, size_t n)
{
    if (freq == NULL) return;
    for (size_t k = 0; k < n; ++k)
        printf("bin=%zu mag2=%.3f\n", k, freq[k].i * freq[k].i + freq[k].q * freq[k].q);
}

int main(void)
{
    struct cpxf time[8], freq[8];
    make_complex_tone(time, 8U, 3);
    dft(time, freq, 8U);
    assert(dft_peak_bin(freq, 8U) == 3U);
    spectrum_dump(freq, 8U);
    puts("wizard task 12 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_12.c -o wizard_task_12 -lm
./wizard_task_12
```

Expected cuối output:

```text
wizard task 12 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_12_END -->

---

#### Forge F20 — Bài 12: DFT host oracle phải chỉ đúng bin

Tự code tone generator, một DFT bin và peak search first-win. Chạy tone ở DC,
bin dương, bin âm và hai tone bằng biên độ.

<details>
<summary>Đáp án F20</summary>

```c
#include <math.h>
#include <stddef.h>
struct cpx64{double i,q;};
void tone(struct cpx64*x,size_t n,int bin)
{const double t=6.28318530717958647692;size_t k;
 for(k=0U;k<n;++k){double a=t*(double)bin*(double)k/(double)n;x[k]=(struct cpx64){cos(a),sin(a)};}}
struct cpx64 dft_bin(const struct cpx64*x,size_t n,size_t bin)
{const double t=6.28318530717958647692;size_t k;struct cpx64 y={0,0};
 for(k=0U;k<n;++k){double a=-t*(double)bin*(double)k/(double)n;
  y.i+=x[k].i*cos(a)-x[k].q*sin(a);y.q+=x[k].i*sin(a)+x[k].q*cos(a);}return y;}
size_t dft_peak(const struct cpx64*x,size_t n)
{size_t k,b=0U;double best=-1.0;for(k=0U;k<n;++k){struct cpx64 y=dft_bin(x,n,k);
 double p=y.i*y.i+y.q*y.q;if(p>best){best=p;b=k;}}return b;}
```

Đây là oracle chậm `O(N²)`, không phải lý do dùng nó trong symbol loop.

</details>

## Level 12 — FFT radix-2

Khi đã hiểu DFT:

- `N` là power-of-two.
- FFT giảm complexity từ khoảng `O(N²)` xuống `O(N log N)`.
- radix-2 thường gồm:
  - bit reversal
  - butterfly stages
  - twiddle factors

Butterfly concept:

```text
u = even
v = odd * W

out_even = u + v
out_odd  = u - v
```

### Fixed point Q15

Ta biểu diễn khoảng `[-1,1)` gần như:

```text
1.0 ≈ 32767
0.5 ≈ 16384
```

Nhân Q15:

```c
real = ((int32_t)a.i * b.i - (int32_t)a.q * b.q) / 32768;
```

rồi saturation về `int16_t`.

### Bài 13 — `complex_mul_q15`

#### 1. Đề bài nhỏ

Viết:

```c
struct cpx16 complex_mul_q15(struct cpx16 a, struct cpx16 b);
```

Test identity gần đúng:

```text
x * (32767, 0) ≈ x
```

#### 2. Tự đoán chương trình cần làm gì

Nhân hai số phức theo công thức real/imag, giữ trung gian rộng, scale Q15 rồi saturation về `int16_t`.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- công thức nhân complex
- Q15 scale 32768
- widen trước multiply và saturation

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_13.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Test identity Q15, zero và hai giá trị biên; so với reference floating-point trong sai số 1–2 LSB.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_13.c \
    -o build/task_13
./build/task_13
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

Nếu biên độ lớn bất thường, thiếu shift Q15; nếu overflow trước shift, accumulator chưa đủ rộng.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** Complex arithmetic fixed-point.

**Bạn vừa code cái gì trong modem?** Channel, oscillator, equalizer và FFT đều cần nhân số phức. Phiên bản Q15 buộc bạn xử lý scale và saturation đúng.

**Nó nằm ở đâu?** PHY DSP primitive.

```text
INPUT  → hai complex Q15
KHỐI   → code của Bài 13
OUTPUT → tích complex Q15
```

**Tại sao modem cần nó?** Sai dấu ở `(ac-bd, ad+bc)` hoặc quên shift Q15 sẽ làm CFO correction/equalization hỏng hoàn toàn nhưng lỗi nhìn ngoài chỉ giống 'sync kém'.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 13 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

```c
struct cpx16 complex_mul_q15(struct cpx16 a, struct cpx16 b)
{
    int32_t real;
    int32_t imag;
    struct cpx16 out;

    real = ((int32_t)a.i * (int32_t)b.i -
            (int32_t)a.q * (int32_t)b.q) / 32768;

    imag = ((int32_t)a.i * (int32_t)b.q +
            (int32_t)a.q * (int32_t)b.i) / 32768;

    out.i = sat16(real);
    out.q = sat16(imag);
    return out;
}
```

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_13_BEGIN -->
#### 9. Wizard Lab — Complex-operation composer Q15

**Ý nghĩ quái dị:** Nếu gain, rotate và conjugate đều là complex multiply/transform, mình có thể compose chúng thành chuỗi helper không?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
struct cpx16 cpx_mul_q15(struct cpx16 a, struct cpx16 b);
struct cpx16 cpx_gain_q15(struct cpx16 x, int16_t gain);
void iq_mix_q15(struct cpx16 *x, size_t n, const struct cpx16 *osc);
```

1. **Bậc 1:** Nhân với 1, j, -1, -j và viết expected bằng tay.
2. **Bậc 2:** So `(x*a)*b` với `x*(a*b)` trong sai số fixed-point.
3. **Bậc 3:** Tìm input làm thứ tự operation khác nhau nhiều nhất do quantization.

**Observer bắt buộc:** Error LSB so với double reference và saturation counter.

**Invariant:** Intermediate phải widen; output luôn nằm int16; identity gần đúng trong tolerance.

**Tự chế spell tiếp theo:** Tạo `struct dsp_chain` chứa array operation callback Q15.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 13</strong></summary>

Lưu thành `practice/wizard_task_13.c`:

```c
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

struct cpx16 { int16_t i; int16_t q; };

static int16_t sat16(int32_t value)
{
    if (value > INT16_MAX) return INT16_MAX;
    if (value < INT16_MIN) return INT16_MIN;
    return (int16_t)value;
}

static struct cpx16 cpx_mul_q15(struct cpx16 a, struct cpx16 b)
{
    const int32_t real = (int32_t)a.i * b.i - (int32_t)a.q * b.q;
    const int32_t imag = (int32_t)a.i * b.q + (int32_t)a.q * b.i;
    return (struct cpx16){sat16(real / INT32_C(32768)),
                          sat16(imag / INT32_C(32768))};
}

static struct cpx16 cpx_gain_q15(struct cpx16 x, int16_t gain)
{
    return (struct cpx16){sat16(((int32_t)x.i * gain) / INT32_C(32768)),
                          sat16(((int32_t)x.q * gain) / INT32_C(32768))};
}

static void iq_mix_q15(struct cpx16 *x, size_t n, const struct cpx16 *osc)
{
    if (x == NULL || osc == NULL) return;
    for (size_t k = 0; k < n; ++k) x[k] = cpx_mul_q15(x[k], osc[k]);
}

int main(void)
{
    struct cpx16 x[2] = {{1000, 0}, {0, 1000}};
    const struct cpx16 osc[2] = {{32767, 0}, {0, 32767}};
    assert(cpx_gain_q15(x[0], 16384).i == 500);
    iq_mix_q15(x, 2U, osc);
    assert(x[0].i == 999 && x[1].i < 0);
    puts("wizard task 13 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_13.c -o wizard_task_13
./wizard_task_13
```

Expected cuối output:

```text
wizard task 13 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_13_END -->

---

#### Forge F21 — Bài 13: complex Q15 cần widen, scale, narrow

Tự code multiply, multiply-conjugate và complex add saturation. Test bốn
quadrant, gần full-scale và `INT16_MIN`.

<details>
<summary>Đáp án F21</summary>

```c
#include <limits.h>
#include <stdint.h>
struct cpx16{int16_t i,q;};
static int16_t f21s(int64_t x){return x>INT16_MAX?INT16_MAX:(x<INT16_MIN?INT16_MIN:(int16_t)x);}
struct cpx16 cmul_q15(struct cpx16 a,struct cpx16 b)
{int64_t i=(int64_t)a.i*b.i-(int64_t)a.q*b.q;
 int64_t q=(int64_t)a.i*b.q+(int64_t)a.q*b.i;
 return(struct cpx16){f21s(i>>15),f21s(q>>15)};}
struct cpx16 cmul_conj_q15(struct cpx16 a,struct cpx16 b)
{b.q=f21s(-(int32_t)b.q);return cmul_q15(a,b);}
struct cpx16 cadd_sat(struct cpx16 a,struct cpx16 b)
{return(struct cpx16){f21s((int32_t)a.i+b.i),f21s((int32_t)a.q+b.q)};}
```

</details>

### Bài 14 — FFT validation

#### 1. Đề bài nhỏ

Mục tiêu là có một FFT radix-2 `N=8` cho kết quả gần DFT reference.

Trước khi mở lời giải:

1. Vẽ permutation bit-reversal của các index `0..7`.
2. Viết test impulse và constant signal.
3. Viết skeleton có ba stage `length=2,4,8`; phần butterfly chưa chắc có thể để TODO.
4. Compile/chạy ít nhất một lần và ghi lại stage đầu tiên sai.

Sau lần thử đó mới được mở Bước 7, trace implementation rồi đóng lại và tự viết.

#### 2. Tự đoán chương trình cần làm gì

FFT phải cho kết quả gần DFT reference nhưng ít phép toán hơn. Bit-reversal và từng butterfly stage phải biến đổi đúng vị trí.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- power-of-two
- bit reversal
- butterfly, twiddle và so sánh với DFT

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_14.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

So từng bin FFT N=8 với DFT reference; test impulse, constant và một tone duy nhất.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_14.c \
    -o build/task_14 -lm
./build/task_14
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

So FFT với DFT sau từng stage, không chỉ output cuối. Bug thường nằm ở bit-reversal, twiddle sign hoặc scale.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** FFT accelerator-equivalent primitive.

**Bạn vừa code cái gì trong modem?** FFT là engine chuyển cả symbol OFDM time-domain sang frequency-domain; IFFT làm ngược lại.

**Nó nằm ở đâu?** PHY OFDM.

```text
INPUT  → N complex samples
KHỐI   → code của Bài 14
OUTPUT → N bins
```

**Tại sao modem cần nó?** Validation với DFT/reference quan trọng hơn việc chỉ thấy FFT 'chạy'. Một modem cần numeric correctness có sai số bounded, không chỉ compile success.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 14 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

Phiên bản này cố ý scale `1/2` ở mỗi stage để giảm nguy cơ overflow. Vì có `log2(N)` stage, output cuối bị scale tổng cộng `1/N`.

```c
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PI 3.14159265358979323846

struct cpx16 {
    int16_t i;
    int16_t q;
};

static int16_t sat16_local(int32_t x)
{
    if (x > 32767) return 32767;
    if (x < -32768) return -32768;
    return (int16_t)x;
}

static struct cpx16 mul_q15(struct cpx16 a, struct cpx16 b)
{
    int32_t re = (int32_t)a.i * b.i - (int32_t)a.q * b.q;
    int32_t im = (int32_t)a.i * b.q + (int32_t)a.q * b.i;

    re >>= 15;
    im >>= 15;

    return (struct cpx16){sat16_local(re), sat16_local(im)};
}

static bool is_pow2(size_t n)
{
    return (n != 0U) && ((n & (n - 1U)) == 0U);
}

static size_t reverse_bits(size_t x, unsigned bits)
{
    size_t r = 0U;

    for (unsigned i = 0U; i < bits; ++i) {
        r = (r << 1U) | (x & 1U);
        x >>= 1U;
    }

    return r;
}

int fft_q15(struct cpx16 *x, size_t n)
{
    unsigned bits = 0U;

    if ((x == NULL) || !is_pow2(n)) {
        return -1;
    }

    for (size_t t = n; t > 1U; t >>= 1U) {
        ++bits;
    }

    for (size_t i = 0U; i < n; ++i) {
        size_t j = reverse_bits(i, bits);
        if (j > i) {
            struct cpx16 tmp = x[i];
            x[i] = x[j];
            x[j] = tmp;
        }
    }

    for (size_t len = 2U; len <= n; len <<= 1U) {
        size_t half = len / 2U;

        for (size_t base = 0U; base < n; base += len) {
            for (size_t j = 0U; j < half; ++j) {
                double theta = -2.0 * PI * (double)j / (double)len;
                struct cpx16 w = {
                    (int16_t)lrint(cos(theta) * 32767.0),
                    (int16_t)lrint(sin(theta) * 32767.0)
                };

                struct cpx16 u = x[base + j];
                struct cpx16 v = mul_q15(x[base + j + half], w);

                int32_t ar = ((int32_t)u.i + v.i) / 2;
                int32_t ai = ((int32_t)u.q + v.q) / 2;
                int32_t br = ((int32_t)u.i - v.i) / 2;
                int32_t bi = ((int32_t)u.q - v.q) / 2;

                x[base + j].i = sat16_local(ar);
                x[base + j].q = sat16_local(ai);
                x[base + j + half].i = sat16_local(br);
                x[base + j + half].q = sat16_local(bi);
            }
        }
    }

    return 0;
}
```

Khi validate, đừng so thẳng với DFT chưa scale. Hãy normalize DFT cùng hệ số `1/N` rồi cho tolerance vài LSB.

Production target thường không gọi `sin/cos` trong FFT fast path; twiddle có thể precompute hoặc nằm trong accelerator. Bài này ưu tiên hiểu radix-2 và fixed-point behavior.

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_14_BEGIN -->
#### 9. Wizard Lab — Hook vào từng stage FFT

**Ý nghĩ quái dị:** Nếu FFT sai, thay vì nhìn output cuối, mình có thể cắm observer sau bit-reversal và mỗi butterfly stage không?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
typedef void (*fft_stage_fn)(size_t length, const struct cpx16 *x,
                             size_t n, void *ctx);
int fft_q15_observed(struct cpx16 *x, size_t n,
                     fft_stage_fn observer, void *ctx);
```

1. **Bậc 1:** Dump stage 2/4/8 cho impulse.
2. **Bậc 2:** Cố ý đổi sign một twiddle rồi tìm first divergent stage.
3. **Bậc 3:** Observer NULL phải không đổi result/determinism.

**Observer bắt buộc:** Stage digest + diff với DFT/reference fixture.

**Invariant:** Observer read-only; cùng input cho cùng stage trace.

**Tự chế spell tiếp theo:** Viết observer chỉ dump khi energy stage vượt ngưỡng.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 14</strong></summary>

Lưu thành `practice/wizard_task_14.c`:

```c
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#define PI 3.14159265358979323846
struct cpx16 { int16_t i; int16_t q; };
typedef void (*fft_stage_fn)(size_t length, const struct cpx16 *x,
                             size_t n, void *ctx);

static int16_t clip(double x)
{
    if (x > (double)INT16_MAX) return INT16_MAX;
    if (x < (double)INT16_MIN) return INT16_MIN;
    return (int16_t)lround(x);
}

static size_t reverse_bits(size_t value, unsigned bits)
{
    size_t out = 0U;
    for (unsigned k = 0; k < bits; ++k) {
        out = (out << 1U) | (value & 1U);
        value >>= 1U;
    }
    return out;
}

static int fft_q15_observed(struct cpx16 *x, size_t n,
                            fft_stage_fn observer, void *ctx)
{
    if (x == NULL || n == 0U || n > 16U) return -1;
    unsigned bits = 0U;
    for (size_t size = n; size > 1U; size >>= 1U) ++bits;
    if (((size_t)1U << bits) != n) return -1;

    for (size_t k = 0; k < n; ++k) {
        const size_t r = reverse_bits(k,bits);
        if (r > k) { const struct cpx16 tmp=x[k]; x[k]=x[r]; x[r]=tmp; }
    }

    for (size_t length = 2U; length <= n; length *= 2U) {
        const size_t half = length / 2U;
        for (size_t base = 0U; base < n; base += length) {
            for (size_t j = 0U; j < half; ++j) {
                const double p = -2.0 * PI * (double)j / (double)length;
                const double c = cos(p), s = sin(p);
                const struct cpx16 u = x[base+j];
                const struct cpx16 v = x[base+j+half];
                const double tr = (double)v.i*c - (double)v.q*s;
                const double ti = (double)v.i*s + (double)v.q*c;
                x[base+j] = (struct cpx16){clip((double)u.i+tr),clip((double)u.q+ti)};
                x[base+j+half] = (struct cpx16){clip((double)u.i-tr),clip((double)u.q-ti)};
            }
        }
        if (observer != NULL) observer(length,x,n,ctx);
    }
    return 0;
}

static void count_stage(size_t length, const struct cpx16 *x,
                        size_t n, void *ctx)
{
    size_t *calls = ctx;
    assert(length >= 2U && length <= n && (length & (length - 1U)) == 0U);
    assert(x != NULL);
    ++*calls;
}

int main(void)
{
    struct cpx16 x[4] = {{1,0},{1,0},{1,0},{1,0}};
    size_t calls = 0U;
    assert(fft_q15_observed(x, 4U, count_stage, &calls) == 0);
    assert(calls == 2U && x[0].i == 4 && x[1].i == 0);
    puts("wizard task 14 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_14.c -o wizard_task_14 -lm
./wizard_task_14
```

Expected cuối output:

```text
wizard task 14 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_14_END -->

---

#### Forge F22 — Bài 14: FFT validation là code, không phải nhìn plot

Tự code power-of-two validator, bit-reversal index và comparator trả first bad
bin. Biến thể tolerance phải gồm absolute + relative term.

<details>
<summary>Đáp án F22</summary>

```c
#include <math.h>
#include <stddef.h>
int is_pow2(size_t n){return n!=0U&&(n&(n-1U))==0U;}
size_t bit_reverse(size_t x,unsigned bits)
{size_t r=0U;unsigned k;for(k=0U;k<bits;++k){r=(r<<1)|(x&1U);x>>=1;}return r;}
int close_vec(const double*a,const double*b,size_t n,double abs,double rel,size_t*bad)
{size_t k;if((a==NULL)||(b==NULL)||(bad==NULL))return-1;
 for(k=0U;k<n;++k){double scale=fmax(fabs(a[k]),fabs(b[k]));
  if(fabs(a[k]-b[k])>abs+rel*scale){*bad=k;return 0;}}return 1;}
```

Sweep `N=1,2,4,...`; reject `0,3,6`. Với complex spectrum, compare cả I và Q.

</details>

## Level 13 — Cyclic Prefix (CP)

Sau IFFT, ta lấy một đoạn cuối symbol copy lên đầu:

```text
IFFT output:
[A B C D E F G H]

CP length 2:
[G H | A B C D E F G H]
```

Receiver bỏ CP:

```text
[G H | A B C D E F G H]
       ^ FFT bắt đầu đây
```

Trong hệ thật, CP hỗ trợ chống multipath/ISI trong giới hạn nhất định.

### Bài 15 — Add/remove CP

#### 1. Đề bài nhỏ

Viết:

```c
int add_cp(const struct cpx16 *symbol,
           size_t nfft,
           size_t cp_len,
           struct cpx16 *out,
           size_t out_cap);
```

Check bắt buộc:

```text
cp_len <= nfft
out_cap >= nfft + cp_len
pointer != NULL
```

Sau đó viết `remove_cp`.

#### 2. Tự đoán chương trình cần làm gì

`add_cp` chép đuôi symbol lên đầu; `remove_cp` bỏ đúng phần prefix. Cả hai phải kiểm tra pointer, length và capacity trước khi copy.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- copy vùng array
- capacity arithmetic an toàn
- invariant `cp_len <= nfft`

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_15.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Test CP 0, CP bằng nfft, capacity thiếu 1 và round trip `remove_cp(add_cp(x)) == x`.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_15.c \
    -o build/task_15
./build/task_15
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

Vẽ indices của prefix và symbol. ASan lỗi thường do tính `nfft+cp_len` hoặc capacity check sau copy.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** Cyclic prefix.

**Bạn vừa code cái gì trong modem?** CP copy phần cuối OFDM symbol lên đầu để receiver có khoảng bảo vệ trước multipath và giúp convolution của channel gần circular trong cửa sổ FFT.

**Nó nằm ở đâu?** PHY OFDM framing.

```text
INPUT  → IFFT symbol
KHỐI   → code của Bài 15
OUTPUT → time-domain symbol có CP / symbol đã remove CP
```

**Tại sao modem cần nó?** Nếu CP length hoặc indexing sai, FFT bins bị nhiễu xuyên symbol. Bài này nối toán FFT với cấu trúc waveform thực tế.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 15 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

##### Solution Bài 15 — cyclic prefix

```c
int add_cp(const struct cpx16 *symbol,
           size_t nfft,
           size_t cp_len,
           struct cpx16 *out,
           size_t out_cap)
{
    if ((symbol == NULL) || (out == NULL) ||
        (nfft == 0U) || (cp_len > nfft) ||
        (out_cap < nfft + cp_len)) {
        return -1;
    }

    for (size_t i = 0U; i < cp_len; ++i) {
        out[i] = symbol[nfft - cp_len + i];
    }

    for (size_t i = 0U; i < nfft; ++i) {
        out[cp_len + i] = symbol[i];
    }

    return 0;
}
```

##### Solution Bài 15 bổ sung — `remove_cp`

```c
int remove_cp(const struct cpx16 *in,
              size_t in_len,
              size_t cp_len,
              struct cpx16 *out,
              size_t nfft)
{
    if ((in == NULL) || (out == NULL) ||
        (cp_len > in_len) ||
        (nfft > in_len - cp_len)) {
        return -1;
    }

    for (size_t i = 0U; i < nfft; ++i) {
        out[i] = in[cp_len + i];
    }

    return 0;
}
```

Round-trip test:

```text
original symbol → add_cp → remove_cp → phải giống original byte-for-byte
```

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_15_BEGIN -->
#### 9. Wizard Lab — Cyclic-prefix vandalism có kiểm soát

**Ý nghĩ quái dị:** CP chịu được delay vì lặp đuôi symbol; nếu phá đúng một sample CP hoặc đổi cp_len thì failure xuất hiện ở đâu?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
int cp_patch(struct cpx16 *with_cp, size_t total,
             size_t cp_index, struct cpx16 value);
size_t cp_mismatch_count(const struct cpx16 *with_cp,
                         size_t nfft, size_t cp_len);
```

1. **Bậc 1:** Patch sample đầu/cuối CP rồi đo mismatch.
2. **Bậc 2:** Sweep channel delay từ 0 tới cp_len+2.
3. **Bậc 3:** So lỗi khi phá CP với phá sample data cùng biên độ.

**Observer bắt buộc:** CP mismatch list, FFT-bin diff và payload/CRC status.

**Invariant:** Add/remove CP round-trip khi không mutation; patch không ghi vào symbol ngoài index chọn.

**Tự chế spell tiếp theo:** Tạo CP pattern marker chỉ cho debug rồi validator tự loại trước TX thật.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 15</strong></summary>

Lưu thành `practice/wizard_task_15.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

struct cpx16 { int16_t i; int16_t q; };

static int cp_patch(struct cpx16 *with_cp, size_t total,
                    size_t cp_index, struct cpx16 value)
{
    if (with_cp == NULL || cp_index >= total) return -1;
    with_cp[cp_index] = value;
    return 0;
}

static size_t cp_mismatch_count(const struct cpx16 *x,
                                size_t nfft, size_t cp_len)
{
    size_t bad = 0U;
    if (x == NULL || cp_len > nfft) return 0U;
    for (size_t k = 0; k < cp_len; ++k) {
        const struct cpx16 a = x[k];
        const struct cpx16 b = x[cp_len + nfft - cp_len + k];
        if (a.i != b.i || a.q != b.q) ++bad;
    }
    return bad;
}

int main(void)
{
    struct cpx16 x[6] = {{3,0},{4,0},{1,0},{2,0},{3,0},{4,0}};
    assert(cp_mismatch_count(x, 4U, 2U) == 0U);
    assert(cp_patch(x, 6U, 0U, (struct cpx16){9,0}) == 0);
    assert(cp_mismatch_count(x, 4U, 2U) == 1U);
    puts("wizard task 15 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_15.c -o wizard_task_15
./wizard_task_15
```

Expected cuối output:

```text
wizard task 15 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_15_END -->

---

#### Forge F23 — Bài 15: CP API phải nói rõ capacity và aliasing

Tự code add CP, remove CP và verify CP bằng compare. Output được caller cấp,
input/output không overlap theo contract.

<details>
<summary>Đáp án F23</summary>

```c
#include <stddef.h>
#include <stdint.h>
struct cpx16{int16_t i,q;};
int add_cp(const struct cpx16*in,size_t n,size_t cp,struct cpx16*out,size_t cap)
{size_t k;if((in==NULL)||(out==NULL)||(cp>n)||(n>SIZE_MAX-cp)||(cap<n+cp))return-1;
 for(k=0U;k<cp;++k)out[k]=in[n-cp+k];for(k=0U;k<n;++k)out[cp+k]=in[k];return 0;}
int remove_cp(const struct cpx16*in,size_t total,size_t cp,struct cpx16*out,size_t cap)
{size_t k,n;if((in==NULL)||(out==NULL)||(cp>total))return-1;n=total-cp;if(cap<n)return-1;
 for(k=0U;k<n;++k)out[k]=in[cp+k];return 0;}
int cp_matches(const struct cpx16*x,size_t total,size_t cp)
{size_t k,n;if((x==NULL)||(cp>total))return 0;n=total-cp;if(cp>n)return 0;
 for(k=0U;k<cp;++k)if(x[k].i!=x[n+k].i||x[k].q!=x[n+k].q)return 0;return 1;}
```

Test compile cùng source trên host và Arm; wire/sample width không được dựa vào
`short` dù ABI hiện tại tình cờ cho `sizeof(short)==2`.

</details>

## Level 14 — Resource grid

Resource grid là cách tổ chức symbol theo time/frequency resource.

Model học tập:

```text
          subcarrier →
OFDM 0    P P P P P P      PSS/sync
OFDM 1    R R R R R R      pilot/reference
OFDM 2    D D D D D D      data
OFDM 3    D D D D D D
...
```

Không cần ngay lập tức tái tạo toàn 38.211.

Trong mini modem ta có thể dùng:

- symbol 0: synchronization sequence
- symbol 1: pilot
- symbol 2+: data

Đây là simplification để học pipeline.

### Bài 16 — Map data lên carrier

#### 1. Đề bài nhỏ

Cho `NFFT=16`, bỏ DC bin, dùng 8 carrier quanh DC.

Tạo:

```c
void clear_symbol(struct cpx16 bins[16]);
```

và function map 4 QPSK symbols vào 4 carrier xác định.

In toàn bộ bin để nhìn layout.

#### 2. Tự đoán chương trình cần làm gì

Chương trình xóa toàn bộ grid, chừa DC và đặt từng symbol đúng carrier đã chọn. In bins để nhìn thấy vị trí nào được dùng.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- frequency bins và index
- zero initialization
- DC bin và carrier map

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_16.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

In đủ 16 bin; assert DC và bin không dùng bằng 0, bốn carrier đích đúng symbol.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_16.c \
    -o build/task_16
./build/task_16
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

In `bin_index` ngay lúc map; nếu DC bị ghi, kiểm tra mapping quanh zero/FFT indexing.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** Resource grid.

**Bạn vừa code cái gì trong modem?** PHY không ném QAM symbol ngẫu nhiên vào FFT. Nó đặt data/reference/sync vào những subcarrier và symbol time xác định.

**Nó nằm ở đâu?** PHY resource mapping.

```text
INPUT  → QAM symbols + carrier indices
KHỐI   → code của Bài 16
OUTPUT → frequency-domain grid
```

**Tại sao modem cần nó?** Đây là mental model để sau này hiểu PSS, DMRS, PDSCH/PUSCH: chúng khác nhau phần lớn ở 'cái gì được map vào resource nào và theo rule nào'.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 16 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

Ví dụ chọn bốn data carrier `1,2,14,15`, bỏ DC bin `0`.

```c
#include <stddef.h>
#include <stdint.h>

struct cpx16 {
    int16_t i;
    int16_t q;
};

void clear_symbol(struct cpx16 bins[16])
{
    for (size_t i = 0U; i < 16U; ++i) {
        bins[i].i = 0;
        bins[i].q = 0;
    }
}

int map_four_qpsk(struct cpx16 bins[16],
                  const struct cpx16 data[4])
{
    static const uint8_t carrier[4] = {1U, 2U, 14U, 15U};

    if ((bins == NULL) || (data == NULL)) {
        return -1;
    }

    for (size_t i = 0U; i < 4U; ++i) {
        bins[carrier[i]] = data[i];
    }

    return 0;
}
```

Ở đây carrier list là rule học tập, chưa phải mapping PDSCH/PUSCH NR thật. Điều cần nắm là **grid được clear rồi từng loại resource được map có chủ đích**.

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_16_BEGIN -->
#### 9. Wizard Lab — Resource-grid painter — thay đổi bin theo ý

**Ý nghĩ quái dị:** Làm sao để set/get/move/copy/swap/notch/shift/mirror bin bằng logical coordinate mà không còn sợ index âm?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
int grid_set_bin(struct grid *g, int bin, struct cpx16 value);
int grid_move_bin(struct grid *g, int from, int to);
int grid_notch(struct grid *g, int first, int last);
int grid_shift(struct grid *g, int delta);
int grid_mirror(struct grid *g);
```

1. **Bậc 1:** Move +3→-3 trên baseline bốn carrier; changed-bin count phải đúng.
2. **Bậc 2:** Paint comb mỗi 4 bin rồi shift +1.
3. **Bậc 3:** Làm bẩn guard đúng một bin và viết predicate bắt nó.

**Observer bắt buộc:** Grid dump logical order, occupancy bitmap, energy/digest/diff.

**Invariant:** Invalid logical bin không sửa grid; mọi bin ngoài vùng mutation giữ nguyên.

**Tự chế spell tiếp theo:** Viết script local gồm lệnh `SET/MOVE/NOTCH/SHIFT/DUMP` chạy qua command struct.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 16</strong></summary>

Lưu thành `practice/wizard_task_16.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define GRID_N 16U
struct cpx16 { int16_t i; int16_t q; };
struct grid { struct cpx16 bins[GRID_N]; };

static int index_of(int bin, size_t *index)
{
    if (index == NULL || bin < -8 || bin > 7) return -1;
    *index = (size_t)(bin + 8);
    return 0;
}

static int grid_set_bin(struct grid *g, int bin, struct cpx16 value)
{
    size_t k;
    if (g == NULL || index_of(bin, &k) != 0) return -1;
    g->bins[k] = value;
    return 0;
}

static int grid_move_bin(struct grid *g, int from, int to)
{
    size_t a, b;
    if (g == NULL || index_of(from, &a) != 0 || index_of(to, &b) != 0) return -1;
    g->bins[b] = g->bins[a]; g->bins[a] = (struct cpx16){0,0};
    return 0;
}

static int grid_notch(struct grid *g, int first, int last)
{
    if (g == NULL || first > last || first < -8 || last > 7) return -1;
    for (int bin = first; bin <= last; ++bin)
        assert(grid_set_bin(g, bin, (struct cpx16){0,0}) == 0);
    return 0;
}

static int grid_shift(struct grid *g, int delta)
{
    struct grid copy = {0};
    if (g == NULL) return -1;
    for (int bin = -8; bin <= 7; ++bin) {
        const int to = ((bin + 8 + delta) % 16 + 16) % 16;
        copy.bins[(size_t)to] = g->bins[(size_t)(bin + 8)];
    }
    *g = copy; return 0;
}

static int grid_mirror(struct grid *g)
{
    if (g == NULL) return -1;
    for (size_t a = 1U; a < GRID_N; ++a) {
        const size_t b = GRID_N - a;
        if (a >= b) break;
        const struct cpx16 tmp = g->bins[a]; g->bins[a] = g->bins[b]; g->bins[b] = tmp;
    }
    return 0;
}

int main(void)
{
    struct grid g = {0};
    assert(grid_set_bin(&g, 3, (struct cpx16){30,3}) == 0);
    assert(grid_move_bin(&g, 3, -3) == 0 && g.bins[5].i == 30);
    assert(grid_notch(&g, -1, 1) == 0);
    assert(grid_shift(&g, 2) == 0 && grid_mirror(&g) == 0);
    puts("wizard task 16 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_16.c -o wizard_task_16
./wizard_task_16
```

Expected cuối output:

```text
wizard task 16 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_16_END -->

---

#### Forge F24 — Bài 16: resource grid phải tách logical bin và storage index

Tự code mapper bin `[-N/2,N/2-1]`, pilot writer reject DC, và zero guard bands.

<details>
<summary>Đáp án F24</summary>

```c
#include <stddef.h>
#include <stdint.h>
struct cpx16{int16_t i,q;};
int bin_index(int32_t bin,size_t n,size_t*out)
{int64_t half;if((out==NULL)||(n==0U)||((n&1U)!=0U)||(n>(size_t)INT32_MAX))return-1;
 half=(int64_t)n/2;if((int64_t)bin< -half||(int64_t)bin>=half)return-1;
 *out=bin<0?n-(size_t)(-(int64_t)bin):(size_t)bin;return 0;}
int grid_put(struct cpx16*g,size_t n,int32_t bin,struct cpx16 v,int allow_dc)
{size_t k;if((g==NULL)||(!allow_dc&&bin==0)||bin_index(bin,n,&k)!=0)return-1;g[k]=v;return 0;}
int zero_guards(struct cpx16*g,size_t n,size_t width)
{size_t k;if((g==NULL)||(n==0U)||((n&1U)!=0U)||(width>n/2U))return-1;
 for(k=0U;k<width;++k){g[n/2U+k]=(struct cpx16){0,0};
  g[n/2U-width+k]=(struct cpx16){0,0};}return 0;}
```

Vẽ bảng logical→storage cho `N=8` bằng tay rồi mới tin code.

</details>

# PHẦN V — CHANNEL: THẾ GIỚI THẬT PHÁ SIGNAL CỦA TA RA SAO

## Level 15 — Noise, delay, CFO

Một virtual RF học tập có thể làm ba việc:

```text
TX IQ
  ↓
delay
  ↓
CFO rotation
  ↓
noise
  ↓
RX IQ
```

### Delay

Đơn giản prepend zero samples:

```text
0 0 0 x0 x1 x2 ...
```

### CFO

Carrier Frequency Offset làm phase xoay dần theo sample:

```text
x[n] * exp(j * 2π Δf n / Fs)
```

Trực giác:

```text
sample vector quay thêm một góc nhỏ ở mỗi bước
```

### Noise

Noise cộng trực tiếp vào I/Q:

```text
I' = I + noise_i
Q' = Q + noise_q
```

Sau cùng saturation về sample range.

---

### Bài 17 — Channel chỉ có delay

#### 1. Đề bài nhỏ

Viết:

```c
int apply_delay(const struct iq_block *in,
                struct iq_block *out,
                uint32_t delay);
```

Không allocate trong hàm. Caller phải cấp `out->samples` và capacity trước.

Đây là pattern firmware quan trọng:

```text
caller owns memory
callee fills memory
```

#### 2. Tự đoán chương trình cần làm gì

Output có một đoạn zero/delay trước dữ liệu gốc. Callee không cấp phát; chỉ ghi vào storage do caller cung cấp khi capacity đủ.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- caller-owned output
- count/capacity
- copy có offset và overflow check

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_17.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Test delay 0, 1, 3; output thiếu capacity phải fail và không ghi ngoài vùng.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_17.c \
    -o build/task_17
./build/task_17
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

Đặt sentinel sau output capacity. Nếu sentinel đổi, loop bound hoặc offset bị sai.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** Channel delay / timing offset.

**Bạn vừa code cái gì trong modem?** Receiver thường không bắt đầu đúng tại sample 0. Delay toy model mô phỏng propagation + alignment offset.

**Nó nằm ở đâu?** virtual RF/channel.

```text
INPUT  → TX IQ + delay
KHỐI   → code của Bài 17
OUTPUT → RX IQ bị dịch thời gian
```

**Tại sao modem cần nó?** Đây là lý do sync phải tồn tại. Nếu channel luôn perfect-aligned, ta sẽ vô tình thiết kế receiver chỉ chạy trong demo.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 17 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

Giả sử `out->count` ban đầu được caller dùng như capacity.

```c
int apply_delay(const struct iq_block *in,
                struct iq_block *out,
                uint32_t delay)
{
    if ((in == NULL) || (out == NULL) ||
        (in->samples == NULL) || (out->samples == NULL)) {
        return -1;
    }

    if ((size_t)delay + (size_t)in->count > (size_t)out->count) {
        return -2;
    }

    for (uint32_t i = 0U; i < delay; ++i) {
        out->samples[i].i = 0;
        out->samples[i].q = 0;
    }

    for (uint32_t i = 0U; i < in->count; ++i) {
        out->samples[delay + i] = in->samples[i];
    }

    out->t0 = in->t0;
    out->sample_rate_hz = in->sample_rate_hz;
    out->count = in->count + delay;
    return 0;
}
```

Lưu ý API dùng cùng field `count` làm capacity trước call và length sau call hơi dễ nhầm. Production API có thể tách capacity rõ ràng hơn.

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_17_BEGIN -->
#### 9. Wizard Lab — Channel delay không còn là một con số

**Ý nghĩ quái dị:** Nếu delay chỉ áp dụng cho một cửa sổ, đổi giữa frame hoặc có sample dropout thì helper channel cần tách thế nào?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
int channel_delay_range(const struct iq_block *in, struct iq_block *out,
                        size_t first, size_t count, uint32_t delay);
int channel_drop_every(struct iq_block *block, size_t stride);
```

1. **Bậc 1:** Delay chỉ nửa sau block và nhìn discontinuity.
2. **Bậc 2:** Drop mỗi sample thứ 8 bằng zero.
3. **Bậc 3:** Compose delay→drop và drop→delay; giải thích vì sao khác.

**Observer bắt buộc:** First non-zero, gap ranges, correlation heatmap.

**Invariant:** Output capacity/bounds đúng; fixed input/params cho deterministic output.

**Tự chế spell tiếp theo:** Tạo channel script gồm nhiều segment `{start,end,delay}`.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 17</strong></summary>

Lưu thành `practice/wizard_task_17.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define IQ_CAP 32U
struct cpx16 { int16_t i; int16_t q; };
struct iq_block { struct cpx16 sample[IQ_CAP]; size_t count; };

static int channel_delay_range(const struct iq_block *in, struct iq_block *out,
                               size_t first, size_t count, uint32_t delay)
{
    if (in == NULL || out == NULL || first > in->count || count > in->count - first) return -1;
    if ((size_t)delay > IQ_CAP - in->count) return -1;
    *out = (struct iq_block){0}; out->count = in->count + delay;
    for (size_t k = 0; k < first; ++k) out->sample[k] = in->sample[k];
    for (size_t k = first; k < first + count; ++k) out->sample[k + delay] = in->sample[k];
    for (size_t k = first + count; k < in->count; ++k) out->sample[k + delay] = in->sample[k];
    return 0;
}

static int channel_drop_every(struct iq_block *block, size_t stride)
{
    if (block == NULL || stride == 0U) return -1;
    for (size_t k = stride - 1U; k < block->count; k += stride)
        block->sample[k] = (struct cpx16){0,0};
    return 0;
}

int main(void)
{
    struct iq_block in = {.sample={{1,0},{2,0},{3,0},{4,0}}, .count=4U};
    struct iq_block out;
    assert(channel_delay_range(&in, &out, 1U, 2U, 2U) == 0);
    assert(out.count == 6U && out.sample[3].i == 2);
    assert(channel_drop_every(&out, 2U) == 0 && out.sample[1].i == 0);
    puts("wizard task 17 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_17.c -o wizard_task_17
./wizard_task_17
```

Expected cuối output:

```text
wizard task 17 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_17_END -->

---

#### Forge F25 — Bài 17: delay channel có linear, circular và observer

Tự code linear delay zero-fill, circular delay và detector vị trí impulse peak.

<details>
<summary>Đáp án F25</summary>

```c
#include <stddef.h>
#include <stdint.h>
struct cpx16{int16_t i,q;};
int delay_linear(const struct cpx16*in,struct cpx16*out,size_t n,size_t d)
{size_t k;if((in==NULL)||(out==NULL)||(d>n))return-1;
 for(k=0U;k<n;++k)out[k]=k<d?(struct cpx16){0,0}:in[k-d];return 0;}
int delay_circular(const struct cpx16*in,struct cpx16*out,size_t n,size_t d)
{size_t k;if((in==NULL)||(out==NULL)||(n==0U))return-1;d%=n;
 for(k=0U;k<n;++k)out[(k+d)%n]=in[k];return 0;}
size_t impulse_peak(const struct cpx16*x,size_t n)
{size_t k,b=0U;uint64_t best=0U;for(k=0U;k<n;++k){int64_t i=x[k].i,q=x[k].q;
 uint64_t p=(uint64_t)(i*i+q*q);if(p>best){best=p;b=k;}}return b;}
```

Không gọi in-place với hai delay function này; thêm ping-pong buffer hoặc viết
biến thể in-place riêng có chứng minh.

</details>

### Bài 18 — Oscillator complex

#### 1. Đề bài nhỏ

Trước khi CFO, viết oscillator đơn giản trên host dùng `sin/cos` để tạo:

```text
cos(theta) + j sin(theta)
```

Sau khi đã tự chạy test và debug ít nhất một lần, mới mở Bước 7 ngay dưới bài này.

#### 2. Tự đoán chương trình cần làm gì

Mỗi sample index tạo một phase; `cos` là I và `sin` là Q. Phase tiến đều sẽ tạo rotation đều trên complex plane.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- radian và phase increment
- `cos`/`sin`
- sample rate, frequency và deterministic host math

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_18.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Test phase 0, quarter-cycle, half-cycle và magnitude gần 1; gọi lại phải cho cùng kết quả.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_18.c \
    -o build/task_18 -lm
./build/task_18
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

In phase trước `sin/cos`; nếu phase tăng quá nhanh, kiểm tra đơn vị Hz, sample/s và `2π`.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** Carrier frequency offset.

**Bạn vừa code cái gì trong modem?** Sai lệch oscillator/relative frequency tạo rotation phase liên tục trên IQ samples. Complex oscillator là cách tạo/correct rotation đó.

**Nó nằm ở đâu?** RF impairment / PHY sync.

```text
INPUT  → sample index, frequency, sample rate
KHỐI   → code của Bài 18
OUTPUT → complex phasor theo thời gian
```

**Tại sao modem cần nó?** CFO nhỏ vẫn tích lũy phase qua nhiều sample và phá OFDM orthogonality. Bài này chuẩn bị trực tiếp cho CFO injection và correction.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 18 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

```c
#include <math.h>
#include <stddef.h>

#define PI 3.14159265358979323846

struct cpxf {
    double re;
    double im;
};

struct cpxf oscillator(double freq_hz,
                       double sample_rate_hz,
                       size_t n)
{
    double theta = 2.0 * PI * freq_hz * (double)n / sample_rate_hz;

    return (struct cpxf){
        cos(theta),
        sin(theta)
    };
}
```

Apply CFO:

```c
struct cpxf cmul(struct cpxf a, struct cpxf b)
{
    return (struct cpxf){
        a.re * b.re - a.im * b.im,
        a.re * b.im + a.im * b.re
    };
}
```

TX/channel xoay `+Δf`; receiver correction dùng oscillator `-Δf`.

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_18_BEGIN -->
#### 9. Wizard Lab — Phase/CFO sculptor

**Ý nghĩ quái dị:** Nếu oscillator đổi frequency hoặc phase giữa block, mình có thể tạo phase jump/CFO window bằng helper stateful không?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
struct osc { double phase; double step; };
void osc_set_frequency(struct osc *o, double hz, double sample_rate);
void osc_jump_phase(struct osc *o, double radians);
int iq_mix_window(struct cpxf *x, size_t n, size_t first, size_t count,
                  struct osc *o);
```

1. **Bậc 1:** Bật CFO chỉ sample 32..63.
2. **Bậc 2:** Chèn phase jump π/2 tại sample 64.
3. **Bậc 3:** Bù bằng helper ngược dấu và đo residual.

**Observer bắt buộc:** Phase difference liên tiếp, spectrum leakage và residual error.

**Invariant:** Ngoài window sample không đổi; oscillator phase tiến đúng count đã xử lý.

**Tự chế spell tiếp theo:** Tạo frequency schedule theo array segment thay vì một step cố định.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 18</strong></summary>

Lưu thành `practice/wizard_task_18.c`:

```c
#include <assert.h>
#include <math.h>
#include <stdio.h>

#define PI 3.14159265358979323846
struct cpxf { double i; double q; };
struct osc { double phase; double step; };

static void osc_set_frequency(struct osc *o, double hz, double sample_rate)
{
    if (o != NULL && sample_rate > 0.0) o->step = 2.0 * PI * hz / sample_rate;
}

static void osc_jump_phase(struct osc *o, double radians)
{
    if (o != NULL) o->phase += radians;
}

static int iq_mix_window(struct cpxf *x, size_t n, size_t first,
                         size_t count, struct osc *o)
{
    if (x == NULL || o == NULL || first > n || count > n - first) return -1;
    for (size_t k = first; k < first + count; ++k) {
        const double c = cos(o->phase), s = sin(o->phase);
        const struct cpxf old = x[k];
        x[k].i = old.i * c - old.q * s;
        x[k].q = old.i * s + old.q * c;
        o->phase += o->step;
    }
    return 0;
}

int main(void)
{
    struct cpxf x[4] = {{1,0},{1,0},{1,0},{1,0}};
    struct osc o = {0.0, 0.0};
    osc_set_frequency(&o, 1.0, 4.0);
    assert(iq_mix_window(x, 4U, 0U, 4U, &o) == 0);
    assert(fabs(x[1].q - 1.0) < 1e-9);
    osc_jump_phase(&o, PI);
    puts("wizard task 18 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_18.c -o wizard_task_18 -lm
./wizard_task_18
```

Expected cuối output:

```text
wizard task 18 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_18_END -->

---

#### Forge F26 — Bài 18: oscillator phải giữ state qua block

Tự code NCO host reference, rotate block và reset có chủ đích. Test hai block
liên tiếp bằng một block ghép.

<details>
<summary>Đáp án F26</summary>

```c
#include <math.h>
#include <stddef.h>
struct cpx64{double i,q;};struct nco{double phase,step;};
void nco_reset(struct nco*n,double phase){if(n!=NULL)n->phase=phase;}
int nco_rotate(struct nco*n,struct cpx64*x,size_t count)
{const double tau=6.28318530717958647692;size_t k;if((n==NULL)||((x==NULL)&&(count!=0U)))return-1;
 for(k=0U;k<count;++k){double c=cos(n->phase),s=sin(n->phase),i=x[k].i,q=x[k].q;
  x[k].i=i*c-q*s;x[k].q=i*s+q*c;n->phase+=n->step;
  if(n->phase>tau||n->phase< -tau)n->phase=fmod(n->phase,tau);}return 0;}
```

Biến thể firmware: thay `sin/cos` bằng LUT Q15 và test sai số với oracle này.

</details>

# PHẦN VI — SYNCHRONIZATION: RECEIVER KHÔNG BIẾT FRAME BẮT ĐẦU Ở ĐÂU

## Level 16 — Correlation

Receiver có một chuỗi tham chiếu đã biết.

Nó trượt reference qua input và tính similarity:

```text
input:      .......ABCDEFG.......
reference:         ABCDEFG
```

vị trí metric lớn nhất có thể là timing estimate.

Với complex sample, correlation phải tính cả I/Q.

### Bài 19 — Correlation thật đơn giản

#### 1. Đề bài nhỏ

Bắt đầu bằng số nguyên 1D, không complex:

```text
input     = [0,0,1,-1,1,-1,0,0]
reference = [1,-1,1,-1]
```

Viết sliding dot product và tìm offset tốt nhất.

Sau khi pass mới mở rộng sang complex IQ.

#### 2. Tự đoán chương trình cần làm gì

Trượt reference qua input, tính score ở mỗi offset và chọn score lớn nhất. Offset đúng phải trùng nơi pattern bắt đầu.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- sliding window
- dot product/correlation
- best score + best offset

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_19.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Vector trong đề phải cho best offset 2; thêm tie case để tự quy định chọn offset đầu tiên hay cuối cùng.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_19.c \
    -o build/task_19
./build/task_19
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

In bảng `offset,score`. Nếu peak lệch một, kiểm tra giới hạn cuối `offset + ref_len <= input_len`.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Điều phải hiểu**

Cell search/synchronization không phải parser tìm byte magic.

Receiver tìm pattern trong **waveform**.

Đây là ranh giới cực quan trọng giữa:

```text
protocol data
```

và:

```text
physical signal
```

**Bài này thuộc nhóm:** Correlation detector.

**Bạn vừa code cái gì trong modem?** Correlation hỏi: 'đoạn tín hiệu hiện tại giống reference sequence đến mức nào?'. Receiver dùng ý tưởng này để tìm sync sequence trong stream IQ.

**Nó nằm ở đâu?** PHY synchronization.

```text
INPUT  → received window + known reference
KHỐI   → code của Bài 19
OUTPUT → correlation score theo offset
```

**Tại sao modem cần nó?** Bài này là phiên bản nhỏ của cell search. Khi peak rõ, receiver suy ra vị trí timing thay vì được hard-code offset.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 19 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

##### Solution Bài 19 — correlation 1D

```c
#include <stddef.h>
#include <stdint.h>

int find_best_offset(const int16_t *input,
                     size_t input_len,
                     const int16_t *reference,
                     size_t ref_len,
                     size_t *best_offset)
{
    int64_t best = INT64_MIN;

    if ((input == NULL) || (reference == NULL) ||
        (best_offset == NULL) || (ref_len == 0U) ||
        (input_len < ref_len)) {
        return -1;
    }

    for (size_t off = 0U; off <= input_len - ref_len; ++off) {
        int64_t metric = 0;

        for (size_t i = 0U; i < ref_len; ++i) {
            metric += (int32_t)input[off + i] * reference[i];
        }

        if (metric > best) {
            best = metric;
            *best_offset = off;
        }
    }

    return 0;
}
```

##### Solution Bài 19 bổ sung — Complex correlation

```c
#include <stddef.h>

struct cpxf {
    double re;
    double im;
};

double corr_metric(const struct cpxf *input,
                   const struct cpxf *ref,
                   size_t n)
{
    double acc_re = 0.0;
    double acc_im = 0.0;

    for (size_t i = 0U; i < n; ++i) {
        /* input[i] * conj(ref[i]) */
        acc_re += input[i].re * ref[i].re + input[i].im * ref[i].im;
        acc_im += input[i].im * ref[i].re - input[i].re * ref[i].im;
    }

    return acc_re * acc_re + acc_im * acc_im;
}
```

Ta dùng magnitude-squared của correlation để tránh cần `sqrt()` khi chỉ so peak.

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_19_BEGIN -->
#### 9. Wizard Lab — Correlation heatmap thay vì một peak

**Ý nghĩ quái dị:** Nếu có hai pattern giống nhau hoặc peak gần bằng nhau, chỉ trả best offset sẽ che mất ambiguity không?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
int corr_scan_i32(const int16_t *x, size_t nx,
                  const int16_t *ref, size_t nr,
                  int32_t *scores, size_t score_cap);
size_t corr_top_k(const int32_t *scores, size_t n,
                  size_t *offsets, size_t k);
```

1. **Bậc 1:** Overlay reference hai lần với amplitude bằng nhau.
2. **Bậc 2:** Giảm amplitude peak thứ hai từng bước.
3. **Bậc 3:** Corrupt mỗi chip thứ N và đo peak/second-peak ratio.

**Observer bắt buộc:** Toàn bộ score curve, top-K và ambiguity ratio.

**Invariant:** Score array length đúng `nx-nr+1`; best helper không đọc ngoài.

**Tự chế spell tiếp theo:** Viết detector yêu cầu peak cao hơn second peak một margin.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 19</strong></summary>

Lưu thành `practice/wizard_task_19.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static int corr_scan_i32(const int16_t *x, size_t nx,
                         const int16_t *ref, size_t nr,
                         int32_t *scores, size_t cap)
{
    if (x == NULL || ref == NULL || scores == NULL || nr == 0U || nx < nr) return -1;
    const size_t needed = nx - nr + 1U;
    if (cap < needed) return -1;
    for (size_t off = 0; off < needed; ++off) {
        int32_t sum = 0;
        for (size_t k = 0; k < nr; ++k) sum += (int32_t)x[off+k] * ref[k];
        scores[off] = sum;
    }
    return (int)needed;
}

static size_t corr_top_k(const int32_t *scores, size_t n,
                         size_t *offsets, size_t k)
{
    if (scores == NULL || offsets == NULL) return 0U;
    if (k > n) k = n;
    for (size_t rank = 0; rank < k; ++rank) {
        size_t best = n;
        for (size_t i = 0U; i < n; ++i) {
            int used = 0;
            for (size_t r = 0; r < rank; ++r) if (offsets[r] == i) used = 1;
            if (used == 0 && (best == n || scores[i] > scores[best])) best = i;
        }
        offsets[rank] = best;
    }
    return k;
}

int main(void)
{
    const int16_t x[6] = {1,-1,1,1,-1,1};
    const int16_t ref[3] = {1,-1,1};
    int32_t score[4]; size_t top[2];
    assert(corr_scan_i32(x, 6U, ref, 3U, score, 4U) == 4);
    assert(corr_top_k(score, 4U, top, 2U) == 2U && score[top[0]] == 3);
    puts("wizard task 19 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_19.c -o wizard_task_19
./wizard_task_19
```

Expected cuối output:

```text
wizard task 19 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_19_END -->

---

#### Forge F27 — Bài 19: correlation phải trả cả metric lẫn vị trí

Tự code complex correlation tại một lag, sweep lag bounded và tie first-win.

<details>
<summary>Đáp án F27</summary>

```c
#include <stddef.h>
#include <stdint.h>
struct cpx16{int16_t i,q;};struct corr{int64_t i,q;};
struct corr correlate(const struct cpx16*x,const struct cpx16*r,size_t n)
{size_t k;struct corr c={0,0};for(k=0U;k<n;++k){
 c.i+=(int64_t)x[k].i*r[k].i+(int64_t)x[k].q*r[k].q;
 c.q+=(int64_t)x[k].q*r[k].i-(int64_t)x[k].i*r[k].q;}return c;}
static uint64_t cmag1(struct corr c)
{uint64_t i=(uint64_t)(c.i<0?-c.i:c.i),q=(uint64_t)(c.q<0?-c.q:c.q);return i+q;}
int corr_peak(const struct cpx16*x,size_t nx,const struct cpx16*r,size_t nr,size_t*out)
{size_t lag,b=0U;uint64_t best=0U;if((x==NULL)||(r==NULL)||(out==NULL)||(nr==0U)||(nr>nx))return-1;
 for(lag=0U;lag<=nx-nr;++lag){uint64_t m=cmag1(correlate(&x[lag],r,nr));if(m>best){best=m;b=lag;}}
 *out=b;return 0;}
```

Bound accumulator theo `n`; production phải reject length khiến `int64_t` có
thể overflow. `INT64_MIN` không thể xuất hiện nếu bound đã được chứng minh.

</details>

## Level 17 — PSS NR ở mức học tập

Trong NR có ba PSS sequence ứng với `N_ID2 = 0,1,2`, độ dài 127.

Mục tiêu ở khóa này:

1. hiểu sequence deterministic;
2. generate ba reference;
3. map lên subcarriers;
4. IFFT thành waveform;
5. correlation để tìm timing + identity.

### Bài 20 — PSS generator

#### 1. Đề bài nhỏ

Viết function:

```c
void make_pss(uint32_t identity, int16_t out[127]);
```

Test:

- identity 0/1/2 cho output deterministic.
- chỉ chứa `+A/-A` trong implementation học tập.
- cùng identity gọi lại phải byte-identical.

Không cần nhớ polynomial bằng đầu; hiểu LFSR/sequence generation trước.

#### 2. Tự đoán chương trình cần làm gì

Cùng identity phải sinh đúng cùng 127 chip; identity khác tạo sequence khác. Output chỉ dùng hai mức biên độ đã quy định.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- LFSR/sequence rule
- identity
- determinism và fixed output length

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_20.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Identity 0/1/2: length 127, deterministic, chỉ `+A/-A`; so hai lần gọi bằng `memcmp`.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_20.c \
    -o build/task_20
./build/task_20
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

In state LFSR vài bước đầu; lỗi thường là tap/index, shift direction hoặc ánh xạ bit sang ±A.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** NR cell-search primitive.

**Bạn vừa code cái gì trong modem?** PSS là known synchronization sequence mà UE có thể dò khi chưa biết payload. Sinh đúng sequence là prerequisite để correlation có reference đúng.

**Nó nằm ở đâu?** NR PHY synchronization.

```text
INPUT  → NID2 / sequence rule
KHỐI   → code của Bài 20
OUTPUT → 127-chip PSS sequence
```

**Tại sao modem cần nó?** Đây là bước chuyển từ DSP toy sang NR-specific concept. UE mới bật máy phải tìm cell trước khi RRC/NAS có thể làm gì.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 20 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

Đây là công thức PSS NR theo TS 38.211: có ba sequence tương ứng `N_ID2 = 0,1,2`.

```c
#include <stdint.h>

int make_pss(uint32_t nid2, int16_t out[127])
{
    uint8_t x[127];

    if ((out == 0) || (nid2 > 2U)) {
        return -1;
    }

    x[0] = 0U;
    x[1] = 1U;
    x[2] = 1U;
    x[3] = 0U;
    x[4] = 1U;
    x[5] = 1U;
    x[6] = 1U;

    for (uint32_t i = 0U; i < 120U; ++i) {
        x[i + 7U] = (uint8_t)((x[i + 4U] + x[i]) & 1U);
    }

    for (uint32_t n = 0U; n < 127U; ++n) {
        uint32_t m = (n + 43U * nid2) % 127U;
        out[n] = x[m] ? (int16_t)-32767 : (int16_t)32767;
    }

    return 0;
}
```

`±32767` ở đây chỉ là amplitude fixed-point của lab. Sequence sign mới là phần quan trọng.

Test bắt buộc:

```text
nid2=0,1,2 đều deterministic
nid2=3 bị reject
output chỉ có +32767 hoặc -32767
```

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_20_BEGIN -->
#### 9. Wizard Lab — Known-sequence mutator

**Ý nghĩ quái dị:** PSS là known pattern; nếu flip chip theo stride, cyclic shift hoặc ghép identity khác thì detector suy giảm thế nào?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
void sequence_flip_stride(int16_t *seq, size_t n, size_t stride);
void sequence_cyclic_shift(int16_t *seq, size_t n, size_t delta);
size_t sequence_diff_count(const int16_t *a, const int16_t *b, size_t n);
```

1. **Bậc 1:** Flip mỗi chip thứ 7/11/17 và lập bảng score.
2. **Bậc 2:** Scan identity 0/1/2 trên cùng waveform.
3. **Bậc 3:** Overlay hai PSS lệch offset và xếp hạng peak.

**Observer bắt buộc:** Diff count, correlation score table và detected identity.

**Invariant:** Mutation deterministic; baseline generator không bị sửa vì luôn clone trước.

**Tự chế spell tiếp theo:** Tạo `sequence_mutator_fn` và chạy matrix mutation×identity.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 20</strong></summary>

Lưu thành `practice/wizard_task_20.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static void sequence_flip_stride(int16_t *seq, size_t n, size_t stride)
{
    if (seq == NULL || stride == 0U) return;
    for (size_t k = stride - 1U; k < n; k += stride) seq[k] = (int16_t)-seq[k];
}

static void sequence_cyclic_shift(int16_t *seq, size_t n, size_t delta)
{
    int16_t tmp[32];
    if (seq == NULL || n == 0U || n > 32U) return;
    delta %= n;
    for (size_t k = 0; k < n; ++k) tmp[(k + delta) % n] = seq[k];
    for (size_t k = 0; k < n; ++k) seq[k] = tmp[k];
}

static size_t sequence_diff_count(const int16_t *a, const int16_t *b, size_t n)
{
    size_t count = 0U;
    if (a == NULL || b == NULL) return 0U;
    for (size_t k = 0; k < n; ++k) if (a[k] != b[k]) ++count;
    return count;
}

int main(void)
{
    const int16_t baseline[4] = {1,-1,1,-1};
    int16_t x[4] = {1,-1,1,-1};
    sequence_flip_stride(x, 4U, 2U);
    assert(sequence_diff_count(baseline, x, 4U) == 2U);
    sequence_cyclic_shift(x, 4U, 1U);
    puts("wizard task 20 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_20.c -o wizard_task_20
./wizard_task_20
```

Expected cuối output:

```text
wizard task 20 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_20_END -->

---

#### Forge F28 — Bài 20: PSS generator và detector đủ ba `N_ID2`

Tự code sequence 127 chip, ba cyclic shift và detector delay bounded.

<details>
<summary>Đáp án F28</summary>

```c
#include <stddef.h>
#include <stdint.h>
#define PSS_N 127U
int pss_generate(unsigned nid2,int8_t out[PSS_N])
{uint8_t x[PSS_N];size_t n;if((out==NULL)||(nid2>2U))return-1;
 x[0]=0U;x[1]=1U;x[2]=1U;x[3]=0U;x[4]=1U;x[5]=1U;x[6]=1U;
 for(n=0U;n<120U;++n)x[n+7U]=(uint8_t)((x[n+4U]+x[n])&1U);
 for(n=0U;n<PSS_N;++n){size_t m=(n+43U*nid2)%PSS_N;out[n]=x[m]?-1:1;}return 0;}
int pss_detect(const int16_t*x,size_t nx,size_t max_delay,unsigned*nid2,size_t*delay)
{unsigned id;size_t d;int64_t best=-1;if((x==NULL)||(nid2==NULL)||(delay==NULL)||(nx<PSS_N))return-1;
 if(max_delay>nx-PSS_N)max_delay=nx-PSS_N;
 for(id=0U;id<3U;++id){int8_t p[PSS_N];(void)pss_generate(id,p);
  for(d=0U;d<=max_delay;++d){size_t n;int64_t c=0;for(n=0U;n<PSS_N;++n)c+=(int64_t)x[d+n]*p[n];
   if(c<0)c=-c;if(c>best){best=c;*nid2=id;*delay=d;}}}return 0;}
```

Đối chiếu với literal fixture độc lập như lời giải 100/100; không tự generate
fixture bằng cùng implementation.

</details>

# PHẦN VII — RECEIVER PIPELINE ĐẦU TIÊN

## Level 18 — TX/RX mini OFDM end-to-end

Ta đã có đủ mảnh:

```text
payload
 ↓
CRC
 ↓
QPSK
 ↓
resource bins
 ↓
IFFT + CP
 ↓
channel
 ↓
sync
 ↓
remove CP
 ↓
FFT
 ↓
demap
 ↓
CRC check
```

### Bài 21 — Mini link 16 byte

#### 1. Đề bài nhỏ

Mục tiêu:

```text
input payload == recovered payload
```

Constraints:

- fixed seed
- QPSK
- noise ban đầu = 0
- CFO ban đầu = 0
- delay = 0

Chỉ khi pass mới tăng difficulty:

1. delay 3 samples;
2. noise nhỏ;
3. CFO nhỏ;
4. corruption để CRC fail.

#### 2. Tự đoán chương trình cần làm gì

Payload đi qua từng stage TX, channel và RX rồi quay lại bytes. Lúc đầu mọi impairment bằng 0 để cô lập lỗi logic.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- pipeline theo stage
- round-trip invariant
- trace checkpoint và fixed seed

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_21.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Giai đoạn đầu payload 16 byte phải round trip với delay/noise/CFO bằng 0; sau đó bật từng impairment một, không bật cùng lúc.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_21.c \
    -o build/task_21 -lm
./build/task_21
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

Dừng ở stage đầu tiên khác expected. Không debug CRC khi bytes đã sai từ mapper/OFDM/channel.

Checkpoint debug đã có trong đề:

**Debug checkpoint**

TX/RX phải trace từng stage:

```text
TX_FRAME
TX_MAP
TX_IFFT
RX_SYNC
RX_FFT
RX_DEMAP
RX_CRC_OK / RX_CRC_FAIL
```

Không debug modem bằng một dòng “failed”.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** First end-to-end PHY link.

**Bạn vừa code cái gì trong modem?** Mini link buộc tất cả primitive PHY hợp tác: bytes→CRC→bits→modulation→OFDM→channel→RX→CRC.

**Nó nằm ở đâu?** PHY integration.

```text
INPUT  → 16-byte transport block
KHỐI   → code của Bài 21
OUTPUT → waveform rồi payload recovered
```

**Tại sao modem cần nó?** Giá trị của bài này là data phải thật sự đi qua representation trung gian. Nếu bạn lén copy payload TX sang RX, bạn không còn test modem nữa.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 21 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

Đây là reference học tập hoàn chỉnh cho case đầu tiên `delay=noise=CFO=0`. Nó dùng DFT/IFFT trực tiếp để code dễ đọc; khi đã hiểu pipeline thì thay bằng FFT fixed-point của Bài 14.

```c
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define PI 3.14159265358979323846
#define NFFT 128U
#define CP 16U
#define PAYLOAD 16U
#define CRC_BYTES 3U
#define FRAME_BYTES (PAYLOAD + CRC_BYTES)
#define FRAME_BITS (FRAME_BYTES * 8U)
#define NSYM ((FRAME_BITS + 1U) / 2U)

struct cpxf {
    double re;
    double im;
};

static uint32_t crc24a(const uint8_t *p, size_t n)
{
    uint32_t crc = 0U;

    for (size_t i = 0U; i < n; ++i) {
        crc ^= (uint32_t)p[i] << 16U;

        for (unsigned b = 0U; b < 8U; ++b) {
            crc <<= 1U;
            if ((crc & 0x1000000U) != 0U) {
                crc ^= 0x1864CFBU;
            }
        }
    }

    return crc & 0xFFFFFFU;
}

static unsigned get_bit(const uint8_t *p, size_t bit)
{
    return (p[bit / 8U] >> (7U - (bit % 8U))) & 1U;
}

static void set_bit(uint8_t *p, size_t bit, unsigned value)
{
    uint8_t mask = (uint8_t)(1U << (7U - (bit % 8U)));

    if (value != 0U) {
        p[bit / 8U] |= mask;
    } else {
        p[bit / 8U] &= (uint8_t)~mask;
    }
}

static struct cpxf qpsk(unsigned b0, unsigned b1)
{
    const double a = 0.7071067811865475;

    return (struct cpxf){
        b0 ? -a : a,
        b1 ? -a : a
    };
}

static void ifft128(const struct cpxf *X, struct cpxf *x)
{
    for (size_t n = 0U; n < NFFT; ++n) {
        double re = 0.0;
        double im = 0.0;

        for (size_t k = 0U; k < NFFT; ++k) {
            double a = 2.0 * PI * (double)k * (double)n / (double)NFFT;
            double c = cos(a);
            double s = sin(a);

            re += X[k].re * c - X[k].im * s;
            im += X[k].re * s + X[k].im * c;
        }

        x[n].re = re / (double)NFFT;
        x[n].im = im / (double)NFFT;
    }
}

static void dft128(const struct cpxf *x, struct cpxf *X)
{
    for (size_t k = 0U; k < NFFT; ++k) {
        double re = 0.0;
        double im = 0.0;

        for (size_t n = 0U; n < NFFT; ++n) {
            double a = -2.0 * PI * (double)k * (double)n / (double)NFFT;
            double c = cos(a);
            double s = sin(a);

            re += x[n].re * c - x[n].im * s;
            im += x[n].re * s + x[n].im * c;
        }

        X[k].re = re;
        X[k].im = im;
    }
}

int main(void)
{
    uint8_t tx[FRAME_BYTES] = {0};
    uint8_t rx[FRAME_BYTES] = {0};
    struct cpxf bins[NFFT] = {{0}};
    struct cpxf time[NFFT] = {{0}};
    struct cpxf wave[NFFT + CP] = {{0}};
    struct cpxf rx_bins[NFFT] = {{0}};
    const uint8_t payload[PAYLOAD] = "Hello modem lab";

    memcpy(tx, payload, PAYLOAD);

    uint32_t crc = crc24a(tx, PAYLOAD);
    tx[PAYLOAD + 0U] = (uint8_t)(crc >> 16U);
    tx[PAYLOAD + 1U] = (uint8_t)(crc >> 8U);
    tx[PAYLOAD + 2U] = (uint8_t)crc;

    /* TX_MAP: bỏ DC bin 0, dùng bins 1..NSYM. */
    for (size_t s = 0U; s < NSYM; ++s) {
        unsigned b0 = get_bit(tx, 2U * s);
        unsigned b1 = (2U * s + 1U < FRAME_BITS)
                    ? get_bit(tx, 2U * s + 1U)
                    : 0U;

        bins[1U + s] = qpsk(b0, b1);
    }

    /* TX_IFFT */
    ifft128(bins, time);

    /* add CP */
    for (size_t i = 0U; i < CP; ++i) {
        wave[i] = time[NFFT - CP + i];
    }
    for (size_t i = 0U; i < NFFT; ++i) {
        wave[CP + i] = time[i];
    }

    /* Channel ở bước đầu là identity. RX biết timing=CP. */

    /* RX_FFT */
    dft128(&wave[CP], rx_bins);

    /* RX_DEMAP */
    for (size_t s = 0U; s < NSYM; ++s) {
        struct cpxf z = rx_bins[1U + s];

        set_bit(rx, 2U * s, z.re < 0.0);
        if (2U * s + 1U < FRAME_BITS) {
            set_bit(rx, 2U * s + 1U, z.im < 0.0);
        }
    }

    uint32_t received_crc =
        ((uint32_t)rx[PAYLOAD + 0U] << 16U) |
        ((uint32_t)rx[PAYLOAD + 1U] << 8U) |
        (uint32_t)rx[PAYLOAD + 2U];

    uint32_t expected_crc = crc24a(rx, PAYLOAD);

    printf("payload_equal=%d crc_ok=%d\n",
           memcmp(tx, rx, PAYLOAD) == 0,
           received_crc == expected_crc);

    return ((memcmp(tx, rx, PAYLOAD) == 0) &&
            (received_crc == expected_crc)) ? 0 : 1;
}
```

Compile:

```bash
gcc -std=c17 -Wall -Wextra -Werror task21.c -lm -o task21
./task21
```

Expected:

```text
payload_equal=1 crc_ok=1
```

Sau khi baseline pass, **không sửa TX/RX bằng shortcut**. Hãy chèn `channel()` thật ở giữa `wave` và RX, rồi lần lượt thêm delay, noise, CFO và sync.

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_21_BEGIN -->
#### 9. Wizard Lab — Pipeline tap và first-divergence hunter

**Ý nghĩ quái dị:** Nếu có thể hook sau mapper, grid, IFFT, channel, FFT, demapper thì bug sẽ tự lộ ở stage đầu tiên nào?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
enum phy_stage { ST_BITS, ST_SYMBOLS, ST_GRID, ST_TIME_IQ, ST_RX_GRID, ST_BITS_OUT };
typedef void (*phy_tap_fn)(enum phy_stage stage, const void *data,
                           size_t count, void *ctx);
```

1. **Bậc 1:** Chạy baseline và lưu digest mỗi stage.
2. **Bậc 2:** Inject move-bin mutation đúng sau resource mapping.
3. **Bậc 3:** So trace baseline/mutated và báo first divergent stage.

**Observer bắt buộc:** Stage digest, typed dump và final CRC/payload diff.

**Invariant:** Tap read-only không đổi output; mutation hook chỉ bật ở stage đã chọn.

**Tự chế spell tiếp theo:** Viết `pipeline_breakpoint(stage,predicate)` chỉ dump khi property fail.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 21</strong></summary>

Lưu thành `practice/wizard_task_21.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

enum phy_stage { ST_BITS, ST_SYMBOLS, ST_GRID, ST_TIME_IQ, ST_RX_GRID, ST_BITS_OUT };
typedef void (*phy_tap_fn)(enum phy_stage stage, const void *data,
                           size_t count, void *ctx);
struct tap_log { uint32_t digest[6]; size_t calls; };

static uint32_t digest_bytes(const void *data, size_t count)
{
    const uint8_t *p = data;
    uint32_t h = UINT32_C(2166136261);
    for (size_t k = 0; k < count; ++k) h = (h ^ p[k]) * UINT32_C(16777619);
    return h;
}

static void digest_tap(enum phy_stage stage, const void *data,
                       size_t count, void *ctx)
{
    struct tap_log *log = ctx;
    if (log == NULL || stage < ST_BITS || stage > ST_BITS_OUT) return;
    log->digest[(size_t)stage] = digest_bytes(data, count);
    ++log->calls;
}

static void phy_emit_tap(phy_tap_fn tap, enum phy_stage stage,
                         const void *data, size_t count, void *ctx)
{
    if (tap != NULL) tap(stage, data, count, ctx);
}

int main(void)
{
    const uint8_t bits[4] = {0U,1U,1U,0U};
    uint8_t changed[4] = {0U,1U,0U,0U};
    struct tap_log a = {0}, b = {0};
    phy_emit_tap(digest_tap, ST_BITS, bits, sizeof bits, &a);
    phy_emit_tap(digest_tap, ST_BITS, changed, sizeof changed, &b);
    assert(a.calls == 1U && a.digest[ST_BITS] != b.digest[ST_BITS]);
    puts("wizard task 21 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_21.c -o wizard_task_21
./wizard_task_21
```

Expected cuối output:

```text
wizard task 21 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_21_END -->

---

#### Forge F29 — Bài 21: mini link phải có first-divergence và BER

Tự code compare byte, BER MSB/LSB independent, và fault injector flip mỗi bit
thứ `step`. Nối mapper→channel→demapper của các card trước.

<details>
<summary>Đáp án F29</summary>

```c
#include <stddef.h>
#include <stdint.h>
size_t first_byte_diff(const uint8_t*a,const uint8_t*b,size_t n)
{size_t k;for(k=0U;k<n;++k)if(a[k]!=b[k])return k;return n;}
size_t bit_errors(const uint8_t*a,const uint8_t*b,size_t n)
{size_t k,c=0U;for(k=0U;k<n;++k){uint8_t v=(uint8_t)(a[k]^b[k]);
 while(v!=0U){c+=(size_t)(v&1U);v>>=1;}}return c;}
int flip_every(uint8_t*p,size_t n,size_t first,size_t step)
{size_t total,pos;if((p==NULL)||(step==0U)||(n>SIZE_MAX/8U))return-1;total=n*8U;
 if(first>=total)return-1;for(pos=first;;){p[pos/8U]^=(uint8_t)(1U<<(7U-pos%8U));
  if(step>SIZE_MAX-pos||pos+step>=total)break;pos+=step;}return 0;}
```

Gate: 16 byte no-channel round-trip `BER=0`; một symbol fault phải tạo expected
bit error và CRC fail, không được “sửa” expected payload cho xanh.

</details>

# PHẦN VIII — FIRMWARE RUNTIME: PHY KHÔNG ĐƯỢC CHẠY TỪ `main()` MỘT CÁCH TÙY TIỆN

## Level 19 — Event-driven firmware

Một firmware baseband thường phản ứng theo event:

```text
RF RX interrupt
Timer interrupt
DMA completion
IPC from application processor
Crypto completion
```

Không nên để ISR làm FFT vài nghìn operation.

Pattern:

```text
IRQ top half
    │ ACK interrupt
    │ capture status/timestamp
    │ enqueue event
    ▼
deferred task
    │
    └── heavy processing
```

### Bài 22 — Event struct

#### 1. Đề bài nhỏ

Tạo:

```c
struct event {
    uint64_t tick;
    uint16_t source;
    uint16_t type;
    uint32_t value;
};
```

Tạo array event và in theo thứ tự.

Sau đó thay `value` bằng handle ở level sau.

#### 2. Tự đoán chương trình cần làm gì

Mỗi event gom thời điểm, nguồn, loại và payload nhỏ. Array giữ nhiều event để runtime xử lý theo thứ tự xác định.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- event `struct`
- fixed-width fields
- array và deterministic ordering

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_22.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Tạo ít nhất bốn event, in đủ field, xác nhận thứ tự và `sizeof` chỉ để quan sát chứ không serialize struct.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_22.c \
    -o build/task_22
./build/task_22
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

In một event ngay sau khởi tạo. Warning uninitialized cho biết bạn chưa gán đủ field.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** Event-driven firmware.

**Bạn vừa code cái gì trong modem?** Firmware modem không chạy một hàm lớn từ đầu tới cuối; nó phản ứng với timer, IRQ, RX completion, protocol message. `event` là representation chuẩn của một việc vừa xảy ra.

**Nó nằm ở đâu?** runtime/control.

```text
INPUT  → tick + source + type + payload handle
KHỐI   → code của Bài 22
OUTPUT → work item cho scheduler/task
```

**Tại sao modem cần nó?** Event tách producer khỏi consumer. Điều này cho phép ISR chỉ enqueue và task xử lý nặng sau đó.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 22 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

```c
#include <stdint.h>

struct buf_handle {
    uint16_t slot;
    uint16_t generation;
    uint32_t length;
};

struct event {
    uint64_t tick;
    uint16_t source;
    uint16_t type;
    struct buf_handle payload;
};
```

Ví dụ producer:

```c
struct event ev = {
    .tick = 1250U,
    .source = 8U,
    .type = 1U,
    .payload = {3U, 7U, 384U}
};
```

Điểm chính: event chứa **handle**, không chứa raw pointer đi xuyên domain.

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_22_BEGIN -->
#### 9. Wizard Lab — Event constructor và event cloning

**Ý nghĩ quái dị:** Nếu event là data, mình có thể clone rồi đổi tick/type/value để tạo scenario mà không viết lại initializer dài không?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
struct event event_make(uint64_t tick, uint16_t source,
                        uint16_t type, uint32_t value);
struct event event_with_tick(struct event base, uint64_t tick);
struct event event_with_value(struct event base, uint32_t value);
```

1. **Bậc 1:** Tạo baseline rồi clone thành early/late event.
2. **Bậc 2:** Duplicate cùng tick nhưng serial khác để test stable order.
3. **Bậc 3:** Tạo invalid type và bắt validator reject.

**Observer bắt buộc:** Event dump và field-diff mask.

**Invariant:** With-helper chỉ đổi đúng field được đặt tên.

**Tự chế spell tiếp theo:** Tạo builder nhỏ có validation thay vì designated initializer rải rác.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 22</strong></summary>

Lưu thành `practice/wizard_task_22.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

struct event { uint64_t tick; uint16_t source; uint16_t type; uint32_t value; };

static struct event event_make(uint64_t tick, uint16_t source,
                               uint16_t type, uint32_t value)
{
    return (struct event){tick, source, type, value};
}

static struct event event_with_tick(struct event base, uint64_t tick)
{
    base.tick = tick; return base;
}

static struct event event_with_value(struct event base, uint32_t value)
{
    base.value = value; return base;
}

int main(void)
{
    const struct event base = event_make(10U, 1U, 2U, 3U);
    const struct event delayed = event_with_tick(base, 99U);
    const struct event changed = event_with_value(base, 77U);
    assert(base.tick == 10U && base.value == 3U);
    assert(delayed.tick == 99U && delayed.value == 3U);
    assert(changed.tick == 10U && changed.value == 77U);
    puts("wizard task 22 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_22.c -o wizard_task_22
./wizard_task_22
```

Expected cuối output:

```text
wizard task 22 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_22_END -->

---

## Level 20 — SPSC ring buffer

SPSC = single producer, single consumer.

Concept:

```text
write_index -> producer
read_index  -> consumer
```

Ring capacity 8:

```text
index physical = logical_index & (capacity - 1)
```

chỉ thuận tiện khi capacity là power of two.

### Empty

```text
read_index == write_index
```

### Full

```text
write_index - read_index >= capacity
```

#### Forge F30 — Bài 22: event phải có constructor, order và clone contract

Tự code constructor validate type/handle, comparator theo `(tick,sequence)` và
clone chỉ copy handle identity, không copy payload bytes.

<details>
<summary>Đáp án F30</summary>

```c
#include <stdint.h>
struct handle{uint16_t slot,generation;uint32_t length;};
struct event{uint64_t tick;uint16_t source,type,sequence;struct handle payload;};
int event_make(uint64_t tick,uint16_t src,uint16_t type,uint16_t seq,
               struct handle h,struct event*out)
{struct event t;if((out==NULL)||(type==0U)||(h.generation==0U))return-1;
 t=(struct event){tick,src,type,seq,h};*out=t;return 0;}
int event_before(const struct event*a,const struct event*b)
{if(a->tick!=b->tick)return a->tick<b->tick;return a->sequence<b->sequence;}
int event_clone(const struct event*in,struct event*out)
{if((in==NULL)||(out==NULL))return-1;*out=*in;return 0;}
```

Clone làm tăng số reference logic; project thật phải quyết định handle là move,
borrow hay ref-count. Không được giả định copy struct tạo ownership mới.

</details>

### Bài 23 — Ring không atomic trước

#### 1. Đề bài nhỏ

Viết bản single-thread:

```c
int ring_push(struct ring *r, const struct event *ev);
int ring_pop(struct ring *r, struct event *out);
```

Test:

1. empty pop fail;
2. push 8 item success;
3. push item thứ 9 fail;
4. pop đúng FIFO;
5. wraparound vẫn đúng.

#### 2. Tự đoán chương trình cần làm gì

Push ghi vào tail, pop đọc từ head; full/empty không được lẫn. Sau khi index vòng qua cuối mảng, FIFO vẫn phải đúng.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- circular index
- head/tail và full/empty
- FIFO + wraparound tests

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_23.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Chạy đủ năm test trong đề, đặc biệt fill→pop vài item→push tiếp để ép wraparound.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_23.c \
    -o build/task_23
./build/task_23
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

In head/tail/count sau mỗi operation. Đừng sửa nhiều điều kiện cùng lúc; trace một chuỗi wrap ngắn.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** Bounded queue.

**Bạn vừa code cái gì trong modem?** Ring buffer là đường vận chuyển event/sample descriptor giữa context. Bản non-atomic giúp hiểu head/tail trước khi thêm concurrency.

**Nó nằm ở đâu?** runtime.

```text
INPUT  → producer push / consumer pop
KHỐI   → code của Bài 23
OUTPUT → FIFO bounded
```

**Tại sao modem cần nó?** Queue full là trạng thái hợp lệ phải xử lý, không phải lý do malloc thêm vô hạn. Modem cần bounded resource vì deadline và memory budget cố định.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 23 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

```c
#define RING_CAP 8U

struct ring {
    uint32_t write_index;
    uint32_t read_index;
    struct event entries[RING_CAP];
};

int ring_push(struct ring *r, const struct event *ev)
{
    if ((r == NULL) || (ev == NULL)) {
        return -1;
    }

    if ((r->write_index - r->read_index) >= RING_CAP) {
        return -2;
    }

    r->entries[r->write_index & (RING_CAP - 1U)] = *ev;
    r->write_index++;
    return 0;
}

int ring_pop(struct ring *r, struct event *out)
{
    if ((r == NULL) || (out == NULL)) {
        return -1;
    }

    if (r->read_index == r->write_index) {
        return -2;
    }

    *out = r->entries[r->read_index & (RING_CAP - 1U)];
    r->read_index++;
    return 0;
}
```

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_23_BEGIN -->
#### 9. Wizard Lab — Ring-pressure pattern generator

**Ý nghĩ quái dị:** Thay vì push tuần tự, nếu producer tạo burst 3-1-4-1-5 và consumer chạy chậm thì high-water mark ra sao?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
int ring_push_n(struct ring *r, const struct event *events, size_t n,
                size_t *pushed);
void ring_snapshot(const struct ring *r, struct ring_stats *out);
int ring_run_pattern(struct ring *r, const int *steps, size_t nsteps);
```

1. **Bậc 1:** Sinh pattern burst và log depth sau từng step.
2. **Bậc 2:** Ép full rồi pop một, push một qua nhiều vòng.
3. **Bậc 3:** So drop-newest với reject-all policy ở helper wrapper.

**Observer bắt buộc:** Head/tail/depth/high-water/drop count.

**Invariant:** FIFO, depth≤capacity và failure không ghi payload.

**Tự chế spell tiếp theo:** Viết model ring đơn giản rồi differential-test với implementation.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 23</strong></summary>

Lưu thành `practice/wizard_task_23.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define RING_CAP 4U
struct event { uint32_t value; };
struct ring { struct event item[RING_CAP]; size_t head, tail, count, drops; };
struct ring_stats { size_t count, free_slots, drops; };

static int ring_push(struct ring *r, struct event e)
{
    if (r == NULL || r->count == RING_CAP) { if (r != NULL) ++r->drops; return -1; }
    r->item[r->tail] = e; r->tail = (r->tail + 1U) % RING_CAP; ++r->count; return 0;
}

static int ring_pop(struct ring *r, struct event *out)
{
    if (r == NULL || out == NULL || r->count == 0U) return -1;
    *out = r->item[r->head]; r->head = (r->head + 1U) % RING_CAP; --r->count; return 0;
}

static int ring_push_n(struct ring *r, const struct event *e, size_t n, size_t *pushed)
{
    if (r == NULL || e == NULL || pushed == NULL) return -1;
    *pushed = 0U;
    while (*pushed < n && ring_push(r, e[*pushed]) == 0) ++*pushed;
    return *pushed == n ? 0 : -1;
}

static void ring_snapshot(const struct ring *r, struct ring_stats *out)
{
    if (r != NULL && out != NULL) *out = (struct ring_stats){r->count, RING_CAP-r->count, r->drops};
}

static int ring_run_pattern(struct ring *r, const int *steps, size_t nsteps)
{
    struct event e; uint32_t serial = 0U;
    if (r == NULL || steps == NULL) return -1;
    for (size_t s = 0; s < nsteps; ++s) {
        if (steps[s] > 0) for (int k = 0; k < steps[s]; ++k)
            if (ring_push(r, (struct event){serial++}) != 0) return -1;
        if (steps[s] < 0) for (int k = 0; k > steps[s]; --k)
            if (ring_pop(r, &e) != 0) return -1;
    }
    return 0;
}

int main(void)
{
    struct ring r = {0}; struct ring_stats stats; size_t pushed;
    const struct event e[3] = {{1U},{2U},{3U}};
    const int pattern[2] = {-2, 3};
    assert(ring_push_n(&r, e, 3U, &pushed) == 0 && pushed == 3U);
    assert(ring_run_pattern(&r, pattern, 2U) == 0);
    ring_snapshot(&r, &stats); assert(stats.count == 4U);
    puts("wizard task 23 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_23.c -o wizard_task_23
./wizard_task_23
```

Expected cuối output:

```text
wizard task 23 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_23_END -->

---

#### Forge F31 — Bài 23: ring thường để học invariant trước atomic

Tự code push/pop/peek cho capacity 4. Sau mỗi operation assert
`r<cap,w<cap,count<=cap`; failure không đổi indices.

<details>
<summary>Đáp án F31</summary>

```c
#include <stddef.h>
#define RCAP 4U
struct ring{int e[RCAP];size_t r,w,count;};
static int rv(const struct ring*q)
{return q!=NULL&&q->r<RCAP&&q->w<RCAP&&q->count<=RCAP;}
int ring_push(struct ring*q,int v)
{if(!rv(q)||q->count==RCAP)return-1;q->e[q->w]=v;q->w=(q->w+1U)%RCAP;++q->count;return 0;}
int ring_pop(struct ring*q,int*out)
{if(!rv(q)||(out==NULL)||q->count==0U)return-1;*out=q->e[q->r];q->r=(q->r+1U)%RCAP;--q->count;return 0;}
int ring_peek(const struct ring*q,int*out)
{if(!rv(q)||(out==NULL)||q->count==0U)return-1;*out=q->e[q->r];return 0;}
```

Test wrap ít nhất hai vòng. Card này tuyệt đối chưa được gọi giữa hai thread.

</details>

### Bài 24 — Atomic SPSC

#### 1. Đề bài nhỏ

Sau khi bản thường pass, đổi index thành:

```c
_Atomic uint32_t write_index;
_Atomic uint32_t read_index;
```

Học semantics tối thiểu:

- producer viết payload trước;
- release publish `write_index`;
- consumer acquire `write_index` rồi mới đọc payload.

Không dùng `volatile` để thay synchronization.

#### 2. Tự đoán chương trình cần làm gì

Producer ghi payload rồi publish index bằng release; consumer acquire index trước khi đọc. `volatile` không tạo ra contract này.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- C17 `_Atomic`
- release/acquire
- single-producer/single-consumer ownership

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_24.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Ngoài unit test full/empty/FIFO, chạy producer/consumer nhiều vòng; dùng ThreadSanitizer trên host khi có thể.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_24.c \
    -o build/task_24 -pthread
./build/task_24
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

Nếu thỉnh thoảng sai, kiểm tra thứ tự payload store và index publish; `volatile` hay relaxed bừa không sửa race.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** SPSC concurrency.

**Bạn vừa code cái gì trong modem?** Atomic SPSC ring cho phép một producer và một consumer trao đổi dữ liệu mà không dùng mutex blocking.

**Nó nằm ở đâu?** ISR↔task / task↔task.

```text
INPUT  → atomic head/tail
KHỐI   → code của Bài 24
OUTPUT → thread-safe bounded FIFO dưới contract SPSC
```

**Tại sao modem cần nó?** Acquire/release ở đây là synchronization thật; `volatile` không thay thế được. Đây là pattern rất gần firmware dataplane.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 24 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

##### Solution Bài 24 — Atomic SPSC core

```c
struct atomic_ring {
    _Atomic uint32_t write_index;
    _Atomic uint32_t read_index;
    uint32_t capacity;
    struct event entries[128];
};

int atomic_ring_push(struct atomic_ring *r, const struct event *ev)
{
    uint32_t w;
    uint32_t rd;

    if ((r == NULL) || (ev == NULL)) {
        return -1;
    }

    w = atomic_load_explicit(&r->write_index, memory_order_relaxed);
    rd = atomic_load_explicit(&r->read_index, memory_order_acquire);

    if ((r->capacity == 0U) ||
        ((r->capacity & (r->capacity - 1U)) != 0U) ||
        (r->capacity > 128U) ||
        ((w - rd) >= r->capacity)) {
        return -2;
    }

    r->entries[w & (r->capacity - 1U)] = *ev;

    atomic_store_explicit(&r->write_index,
                          w + 1U,
                          memory_order_release);
    return 0;
}
```

Consumer làm đối xứng: load `read` relaxed, load `write` acquire, đọc entry, store `read` release.

##### Solution Bài 24 bổ sung — `atomic_ring_pop`

```c
int atomic_ring_pop(struct atomic_ring *r, struct event *out)
{
    uint32_t rd;
    uint32_t w;

    if ((r == NULL) || (out == NULL)) {
        return -1;
    }

    rd = atomic_load_explicit(&r->read_index, memory_order_relaxed);
    w = atomic_load_explicit(&r->write_index, memory_order_acquire);

    if ((r->capacity == 0U) ||
        ((r->capacity & (r->capacity - 1U)) != 0U) ||
        (r->capacity > 128U) ||
        (rd == w)) {
        return -2;
    }

    *out = r->entries[rd & (r->capacity - 1U)];

    atomic_store_explicit(&r->read_index,
                          rd + 1U,
                          memory_order_release);
    return 0;
}
```

SPSC nghĩa là **đúng một producer và đúng một consumer**. Đừng dùng implementation này cho MPMC rồi mong atomic tự cứu.

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_24_BEGIN -->
#### 9. Wizard Lab — Interleaving model cho SPSC

**Ý nghĩ quái dị:** Không thể nhìn race bằng một run; mình có thể mô hình hóa từng bước producer/consumer và enumerate interleaving nhỏ không?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
enum spsc_step { P_WRITE_PAYLOAD, P_PUBLISH, C_ACQUIRE, C_READ };
int spsc_model_run(const enum spsc_step *steps, size_t n,
                   struct spsc_model_result *out);
```

1. **Bậc 1:** Enumerate permutation bốn bước và đánh dấu sequence hợp lệ.
2. **Bậc 2:** Cố ý cho consumer read trước acquire/publish.
3. **Bậc 3:** So model với threaded stress test host.

**Observer bắt buộc:** Trace happens-before và bad-read counter.

**Invariant:** Consumer chỉ thấy payload sau publish/acquire; mỗi item đọc tối đa một lần.

**Tự chế spell tiếp theo:** Viết generator interleaving cho hai item nhưng vẫn bounded.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 24</strong></summary>

Lưu thành `practice/wizard_task_24.c`:

```c
#include <assert.h>
#include <stdio.h>

enum spsc_step { P_WRITE_PAYLOAD, P_PUBLISH, C_ACQUIRE, C_READ };
struct spsc_model_result { int valid; int published; int acquired; int read; size_t failed_at; };

static int spsc_model_run(const enum spsc_step *steps, size_t n,
                          struct spsc_model_result *out)
{
    struct spsc_model_result r = {.valid=1, .failed_at=n};
    if (steps == NULL || out == NULL) return -1;
    int payload = 0;
    for (size_t k = 0; k < n; ++k) {
        switch (steps[k]) {
        case P_WRITE_PAYLOAD: payload = 1; break;
        case P_PUBLISH: if (payload == 0) r.valid = 0; else r.published = 1; break;
        case C_ACQUIRE: if (r.published == 0) r.valid = 0; else r.acquired = 1; break;
        case C_READ: if (r.acquired == 0) r.valid = 0; else r.read = 1; break;
        default: r.valid = 0; break;
        }
        if (r.valid == 0) { r.failed_at = k; break; }
    }
    *out = r; return r.valid != 0 ? 0 : -1;
}

int main(void)
{
    const enum spsc_step good[] = {P_WRITE_PAYLOAD,P_PUBLISH,C_ACQUIRE,C_READ};
    const enum spsc_step bad[] = {P_PUBLISH,P_WRITE_PAYLOAD,C_READ};
    struct spsc_model_result r;
    assert(spsc_model_run(good, 4U, &r) == 0 && r.read != 0);
    assert(spsc_model_run(bad, 3U, &r) == -1 && r.failed_at == 0U);
    puts("wizard task 24 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_24.c -o wizard_task_24
./wizard_task_24
```

Expected cuối output:

```text
wizard task 24 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_24_END -->

---

#### Forge F32 — Bài 24: SPSC atomic publish data trước index

Tự code push release-store, pop acquire-load và test 1 triệu item bằng đúng một
producer/một consumer. Sau đó cố đổi release/acquire thành relaxed và giải thích
vì sao test xanh vẫn chưa chứng minh code đúng.

<details>
<summary>Đáp án F32 — SPSC core</summary>

```c
#include <stdatomic.h>
#include <stdint.h>
#define ACAP 8U
struct aring{_Atomic uint32_t r,w;int e[ACAP];};
int aring_push(struct aring*q,int v)
{uint32_t w,r,next;if(q==NULL)return-1;
 w=atomic_load_explicit(&q->w,memory_order_relaxed);
 r=atomic_load_explicit(&q->r,memory_order_acquire);next=(w+1U)%ACAP;
 if(next==r)return 1;q->e[w]=v;
 atomic_store_explicit(&q->w,next,memory_order_release);return 0;}
int aring_pop(struct aring*q,int*out)
{uint32_t r,w;if((q==NULL)||(out==NULL))return-1;
 r=atomic_load_explicit(&q->r,memory_order_relaxed);
 w=atomic_load_explicit(&q->w,memory_order_acquire);if(r==w)return 1;
 *out=q->e[r];atomic_store_explicit(&q->r,(r+1U)%ACAP,memory_order_release);return 0;}
```

Một slot bị hy sinh để phân biệt full/empty, usable capacity là 7. Đây chỉ đúng
SPSC; thêm producer thứ hai làm contract sai dù type vẫn compile.

</details>

# PHẦN IX — MEMORY OWNERSHIP: TRÁNH UAF VÀ CON TRỎ LANG THANG

## Level 21 — Buffer pool

Firmware realtime thường tránh `malloc()` ở fast path.

Ta cấp trước pool:

```text
slot 0 [1024 bytes]
slot 1 [1024 bytes]
...
```

Một handle:

```c
struct buf_handle {
    uint16_t slot;
    uint16_t generation;
    uint32_t length;
};
```

Tại sao có `generation`?

Ví dụ:

```text
slot 3 gen=10 allocate -> handle A
release
slot 3 gen=11 allocate -> handle B
```

Nếu code cũ còn A rồi cố dùng:

```text
slot matches nhưng generation không match → reject stale handle
```

### Bài 25 — Pool 4 slot

#### 1. Đề bài nhỏ

Mỗi slot 64 byte.

API:

```c
int pool_alloc(struct pool *p,
               uint32_t length,
               struct buf_handle *h,
               uint8_t **data);

int pool_get(struct pool *p,
             struct buf_handle h,
             uint8_t **data);

int pool_release(struct pool *p,
                 struct buf_handle h);
```

Test stale handle bắt buộc.

#### 2. Tự đoán chương trình cần làm gì

Allocate trả slot và generation; release làm handle cũ hết hiệu lực. `pool_get` phải từ chối stale handle dù index trùng.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- slot state
- generation-tagged handle
- lifetime và stale-handle rejection

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_25.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Allocate hết 4 slot, slot thứ 5 fail; release rồi allocate lại; handle cũ phải bị reject.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_25.c \
    -o build/task_25
./build/task_25
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

In `{slot,generation,state}` ở alloc/get/release. Nếu handle cũ pass, generation chưa tăng hoặc chưa được so.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** Buffer ownership + generation.

**Bạn vừa code cái gì trong modem?** Pool tái sử dụng slot cố định; generation tag giúp phát hiện handle cũ trỏ nhầm vào slot đã được tái cấp phát.

**Nó nằm ở đâu?** memory/runtime.

```text
INPUT  → allocate/free handle
KHỐI   → code của Bài 25
OUTPUT → bounded reusable buffer slot
```

**Tại sao modem cần nó?** Trong modem, payload lớn đi bằng handle vì copy tốn thời gian. Generation là một lớp phòng thủ chống stale reference/use-after-recycle.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 25 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

```c
#define POOL_SLOTS 4U
#define SLOT_BYTES 64U

struct pool_slot {
    uint16_t generation;
    bool used;
    uint32_t length;
    uint8_t data[SLOT_BYTES];
};

struct pool {
    struct pool_slot slots[POOL_SLOTS];
};

struct buf_handle {
    uint16_t slot;
    uint16_t generation;
    uint32_t length;
};

void pool_init(struct pool *p)
{
    if (p == NULL) return;

    for (uint16_t i = 0U; i < POOL_SLOTS; ++i) {
        p->slots[i].generation = 1U;
        p->slots[i].used = false;
        p->slots[i].length = 0U;
    }
}

int pool_alloc(struct pool *p,
               uint32_t length,
               struct buf_handle *h,
               uint8_t **data)
{
    if ((p == NULL) || (h == NULL) || (data == NULL) ||
        (length > SLOT_BYTES)) {
        return -1;
    }

    for (uint16_t i = 0U; i < POOL_SLOTS; ++i) {
        if (!p->slots[i].used) {
            p->slots[i].used = true;
            p->slots[i].length = length;

            h->slot = i;
            h->generation = p->slots[i].generation;
            h->length = length;
            *data = p->slots[i].data;
            return 0;
        }
    }

    return -2;
}

int pool_get(struct pool *p,
             struct buf_handle h,
             uint8_t **data)
{
    struct pool_slot *s;

    if ((p == NULL) || (data == NULL) || (h.slot >= POOL_SLOTS)) {
        return -1;
    }

    s = &p->slots[h.slot];

    if ((!s->used) ||
        (s->generation != h.generation) ||
        (s->length != h.length)) {
        return -2;
    }

    *data = s->data;
    return 0;
}

int pool_release(struct pool *p, struct buf_handle h)
{
    struct pool_slot *s;

    if ((p == NULL) || (h.slot >= POOL_SLOTS)) {
        return -1;
    }

    s = &p->slots[h.slot];

    if ((!s->used) || (s->generation != h.generation)) {
        return -2;
    }

    s->used = false;
    s->length = 0U;
    s->generation++;
    if (s->generation == 0U) {
        s->generation = 1U;
    }

    return 0;
}
```

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_25_BEGIN -->
#### 9. Wizard Lab — Pool churn và ownership microscope

**Ý nghĩ quái dị:** Nếu allocate/release cùng slot hàng nghìn lần, generation/owner/stale reject có còn đúng không?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
int pool_churn(struct pool *p, size_t rounds, struct churn_stats *out);
void pool_dump(const struct pool *p);
int handle_is_live(const struct pool *p, struct buf_handle h);
```

1. **Bậc 1:** Giữ một handle cũ rồi churn 100 vòng.
2. **Bậc 2:** Gắn owner tag CPU/DMA/TASK và thử release sai owner.
3. **Bậc 3:** Đẩy generation gần wrap bằng fixture rồi quan sát policy.

**Observer bắt buộc:** Slot/generation/owner trace và stale-reject count.

**Invariant:** Một slot chỉ có một owner; handle cũ không trở nên live ngoài policy wrap đã định.

**Tự chế spell tiếp theo:** Tạo `pool_assert_consistent()` quét toàn pool sau mỗi operation.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 25</strong></summary>

Lưu thành `practice/wizard_task_25.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define POOL_CAP 4U
struct buf_handle { uint16_t slot; uint16_t generation; };
struct pool_slot { uint16_t generation; int live; };
struct pool { struct pool_slot slot[POOL_CAP]; };
struct churn_stats { size_t allocs, frees, stale_rejects; };

static int handle_is_live(const struct pool *p, struct buf_handle h)
{
    return p != NULL && h.slot < POOL_CAP && p->slot[h.slot].live != 0 &&
           p->slot[h.slot].generation == h.generation;
}

static int alloc_one(struct pool *p, struct buf_handle *out)
{
    for (size_t k = 0; k < POOL_CAP; ++k) if (p->slot[k].live == 0) {
        p->slot[k].live = 1; *out = (struct buf_handle){(uint16_t)k,p->slot[k].generation}; return 0;
    }
    return -1;
}

static int free_one(struct pool *p, struct buf_handle h)
{
    if (handle_is_live(p,h) == 0) return -1;
    p->slot[h.slot].live = 0; ++p->slot[h.slot].generation; return 0;
}

static int pool_churn(struct pool *p, size_t rounds, struct churn_stats *out)
{
    if (p == NULL || out == NULL) return -1;
    *out = (struct churn_stats){0};
    for (size_t k = 0; k < rounds; ++k) {
        struct buf_handle h;
        if (alloc_one(p,&h) != 0) return -1; ++out->allocs;
        if (free_one(p,h) != 0) return -1; ++out->frees;
        if (handle_is_live(p,h) == 0) ++out->stale_rejects;
    }
    return 0;
}

static void pool_dump(const struct pool *p)
{
    if (p == NULL) return;
    for (size_t k = 0; k < POOL_CAP; ++k)
        printf("slot=%zu gen=%u live=%d\n",k,p->slot[k].generation,p->slot[k].live);
}

int main(void)
{
    struct pool p = {0}; struct churn_stats s;
    assert(pool_churn(&p, 8U, &s) == 0 && s.stale_rejects == 8U);
    pool_dump(&p);
    puts("wizard task 25 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_25.c -o wizard_task_25
./wizard_task_25
```

Expected cuối output:

```text
wizard task 25 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_25_END -->

---

#### Forge F33 — Bài 25: pool acquire/release và stale-handle regression

Tự code acquire first-free, resolve và release. Sau release rồi reacquire cùng
slot, handle cũ phải fail; double-release phải fail mà không tăng generation lần hai.

<details>
<summary>Đáp án F33</summary>

```c
#include <stddef.h>
#include <stdint.h>
#define PCAP 4U
struct ph{uint16_t slot,generation;uint32_t length;};
struct ps{uint16_t generation;int used;uint8_t data[64];};
struct pool{struct ps s[PCAP];};
static uint16_t ng(uint16_t x){++x;return x==0U?1U:x;}
int pool_acquire(struct pool*p,uint32_t len,struct ph*out)
{size_t k;if((p==NULL)||(out==NULL)||(len>64U))return-1;
 for(k=0U;k<PCAP;++k)if(!p->s[k].used){if(p->s[k].generation==0U)p->s[k].generation=1U;
  p->s[k].used=1;*out=(struct ph){(uint16_t)k,p->s[k].generation,len};return 0;}return 1;}
void*pool_resolve(struct pool*p,struct ph h)
{if((p==NULL)||(h.slot>=PCAP)||(h.length>64U)||!p->s[h.slot].used||
 p->s[h.slot].generation!=h.generation)return NULL;return p->s[h.slot].data;}
int pool_release(struct pool*p,struct ph h)
{if(pool_resolve(p,h)==NULL)return-1;p->s[h.slot].used=0;
 p->s[h.slot].generation=ng(p->s[h.slot].generation);return 0;}
```

</details>

## Level 22 — Arena allocation

Project firmware có thể nhận một vùng memory lớn từ caller:

```text
arena
┌──────────────────────────────┐
│ modem object                 │
│ pool                         │
│ queues                       │
│ DSP workspace                │
└──────────────────────────────┘
```

API kiểu:

```c
size_t modem_required_memory(const struct config *cfg);

int modem_init(void *arena,
               size_t arena_len,
               ...);
```

Lợi ích:

- deterministic memory usage;
- không heap sau init;
- dễ audit;
- phù hợp firmware.

### Bài 26 — Aligned arena object

#### 1. Đề bài nhỏ

Cho một `uint8_t arena[4096]`, align địa chỉ cho `struct modem` bằng `_Alignof` và `uintptr_t`.

Không cast pointer sang `uint32_t`; host có thể 64-bit.

#### 2. Tự đoán chương trình cần làm gì

Chương trình tính địa chỉ aligned nằm bên trong arena, kiểm tra còn đủ chỗ rồi mới cast thành object pointer.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- `uintptr_t`
- `_Alignof` và round-up
- arena bounds

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_26.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Assert địa chỉ `% _Alignof(struct modem) == 0`; test arena quá nhỏ và base bị lệch.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_26.c \
    -o build/task_26
./build/task_26
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

In base, aligned, end ở hex bằng `uintptr_t`; đừng truncate pointer host về `uint32_t`.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** Arena allocation.

**Bạn vừa code cái gì trong modem?** Firmware freestanding thường cấp một arena lớn lúc init rồi carve thành object/pool có alignment rõ ràng thay vì malloc trong fast path.

**Nó nằm ở đâu?** firmware memory.

```text
INPUT  → arena base + size + alignment
KHỐI   → code của Bài 26
OUTPUT → object placement deterministic
```

**Tại sao modem cần nó?** Alignment quan trọng với DMA/cache/accelerator. Bài này giải thích vì sao `bb_modem_required_memory()` tồn tại trong API đích.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 26 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

```c
#include <stddef.h>
#include <stdint.h>

struct modem {
    uint32_t state;
    uint64_t tick;
    uint8_t scratch[96];
};

void *arena_place(void *arena,
                  size_t arena_len,
                  size_t object_size,
                  size_t alignment)
{
    uintptr_t base;
    uintptr_t aligned;
    size_t skip;

    if ((arena == NULL) ||
        (alignment == 0U) ||
        ((alignment & (alignment - 1U)) != 0U)) {
        return NULL;
    }

    base = (uintptr_t)arena;
    aligned = (base + (uintptr_t)alignment - 1U) &
              ~((uintptr_t)alignment - 1U);
    skip = (size_t)(aligned - base);

    if ((skip > arena_len) ||
        (object_size > arena_len - skip)) {
        return NULL;
    }

    return (void *)aligned;
}

int example(void)
{
    uint8_t arena[4096];
    struct modem *m = arena_place(arena,
                                  sizeof(arena),
                                  sizeof(struct modem),
                                  _Alignof(struct modem));

    if (m == NULL) {
        return -1;
    }

    m->state = 0U;
    m->tick = 0U;
    return 0;
}
```

Dùng `uintptr_t`, không cắt pointer thành `uint32_t` khi chạy host x86-64.

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_26_BEGIN -->
#### 9. Wizard Lab — Arena layout planner

**Ý nghĩ quái dị:** Nếu tự thay số ring/pool/FFT buffer, mình có thể tính và vẽ memory layout trước khi init không?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
int arena_plan_add(struct arena_plan *p, const char *name,
                   size_t size, size_t alignment);
void arena_plan_dump(const struct arena_plan *p);
size_t arena_plan_waste(const struct arena_plan *p);
```

1. **Bậc 1:** Đổi thứ tự object để giảm padding.
2. **Bậc 2:** Sweep alignment 4/16/64 cho DMA object.
3. **Bậc 3:** Tăng pool slot đến khi vượt budget và tìm first failing config.

**Observer bắt buộc:** Offset/size/alignment/padding table và total budget.

**Invariant:** Mọi object aligned, không overlap, end≤arena size.

**Tự chế spell tiếp theo:** Viết planner thử nhiều thứ tự cho 6 object và chọn waste nhỏ nhất.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 26</strong></summary>

Lưu thành `practice/wizard_task_26.c`:

```c
#include <assert.h>
#include <stdio.h>

#define PLAN_CAP 8U
struct arena_item { const char *name; size_t offset, size, padding; };
struct arena_plan { struct arena_item item[PLAN_CAP]; size_t count, cursor, waste; };

static int arena_plan_add(struct arena_plan *p, const char *name,
                          size_t size, size_t alignment)
{
    if (p == NULL || name == NULL || alignment == 0U ||
        (alignment & (alignment - 1U)) != 0U || p->count == PLAN_CAP) return -1;
    const size_t aligned = (p->cursor + alignment - 1U) & ~(alignment - 1U);
    if (aligned < p->cursor || size > (size_t)-1 - aligned) return -1;
    p->item[p->count++] = (struct arena_item){name,aligned,size,aligned-p->cursor};
    p->waste += aligned - p->cursor; p->cursor = aligned + size; return 0;
}

static void arena_plan_dump(const struct arena_plan *p)
{
    if (p == NULL) return;
    for (size_t k = 0; k < p->count; ++k)
        printf("%s off=%zu size=%zu pad=%zu\n",p->item[k].name,p->item[k].offset,p->item[k].size,p->item[k].padding);
}

static size_t arena_plan_waste(const struct arena_plan *p)
{
    return p == NULL ? 0U : p->waste;
}

int main(void)
{
    struct arena_plan p = {0};
    assert(arena_plan_add(&p,"iq",3U,1U) == 0);
    assert(arena_plan_add(&p,"dma",8U,8U) == 0);
    assert(arena_plan_waste(&p) == 5U);
    arena_plan_dump(&p);
    puts("wizard task 26 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_26.c -o wizard_task_26
./wizard_task_26
```

Expected cuối output:

```text
wizard task 26 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_26_END -->

---

#### Forge F34 — Bài 26: arena phải xử lý alignment và arithmetic overflow

Tự code `align_up`, allocation transactional và required-size calculator cho
ba object alignment khác nhau.

<details>
<summary>Đáp án F34</summary>

```c
#include <stddef.h>
#include <stdint.h>
struct arena{uint8_t*base;size_t cap,off;};
int align_up_size(size_t x,size_t a,size_t*out)
{size_t mask;if((out==NULL)||(a==0U)||((a&(a-1U))!=0U))return-1;mask=a-1U;
 if(x>SIZE_MAX-mask)return-1;*out=(x+mask)&~mask;return 0;}
int arena_alloc(struct arena*a,size_t bytes,size_t alignment,void**out)
{size_t start;if((a==NULL)||(out==NULL)||(a->base==NULL)||(a->off>a->cap)||
 align_up_size(a->off,alignment,&start)!=0||bytes>a->cap-start)return-1;
 *out=&a->base[start];a->off=start+bytes;return 0;}
int required3(size_t s1,size_t a1,size_t s2,size_t a2,size_t s3,size_t a3,size_t*out)
{size_t x=0U;if(out==NULL)return-1;
 if(align_up_size(x,a1,&x)||s1>SIZE_MAX-x)return-1;x+=s1;
 if(align_up_size(x,a2,&x)||s2>SIZE_MAX-x)return-1;x+=s2;
 if(align_up_size(x,a3,&x)||s3>SIZE_MAX-x)return-1;x+=s3;*out=x;return 0;}
```

Failure phải giữ `arena.off` và `*out`. Snippet chỉ gán sau mọi validation.

</details>

# PHẦN X — MMIO, DMA VÀ CACHE OWNERSHIP

## Level 23 — MMIO concept

Memory-mapped I/O nghĩa là device register được truy cập giống address.

Firmware API tốt thường che hardware qua accessor:

```c
uint32_t mmio_read32(uintptr_t addr);
void mmio_write32(uintptr_t addr, uint32_t value);
```

Ở host simulator, function này không thực sự chạm hardware. Nó có thể index vào array register mô phỏng.

Đây là sức mạnh của abstraction:

```text
same firmware logic
       │
       ├── real/target MMIO
       └── host simulated MMIO
```

### Bài 27 — Fake register bank

#### 1. Đề bài nhỏ

Tạo:

```c
uint32_t regs[16];
```

Map base giả:

```text
BASE + 0x00 -> regs[0]
BASE + 0x04 -> regs[1]
...
```

Reject:

- unaligned address;
- outside range.

#### 2. Tự đoán chương trình cần làm gì

Address được đổi thành offset/register index sau khi kiểm tra range và alignment. Address sai bị reject, không index mảng bừa.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- MMIO address/offset
- alignment và range
- fake register array trước hardware accessor

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_27.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Test address đầu/cuối hợp lệ, unaligned và ngoài range; invalid access không làm đổi register nào.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_27.c \
    -o build/task_27
./build/task_27
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

Tính và in offset trước index. Validate `address >= BASE`, `(offset % 4)==0`, rồi mới chia 4.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** MMIO mental model.

**Bạn vừa code cái gì trong modem?** Fake register bank mô phỏng cách CPU điều khiển peripheral bằng load/store vào địa chỉ register xác định.

**Nó nằm ở đâu?** HAL/SoC.

```text
INPUT  → register offset + value
KHỐI   → code của Bài 27
OUTPUT → device state thay đổi hoặc status đọc ra
```

**Tại sao modem cần nó?** Bạn chưa điều khiển RF thật; bạn đang học contract CPU↔accelerator. Sau này DMA/FFT/CRYPTO được kick bằng cùng kiểu register/doorbell.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 27 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

##### Solution Bài 27 — fake MMIO bank

```c
#define MMIO_BASE ((uintptr_t)0x60000000U)
#define REG_COUNT 16U

struct fake_mmio {
    uint32_t regs[REG_COUNT];
};

int fake_mmio_read32(struct fake_mmio *m,
                     uintptr_t addr,
                     uint32_t *value)
{
    uintptr_t offset;

    if ((m == NULL) || (value == NULL) ||
        (addr < MMIO_BASE) || ((addr & 3U) != 0U)) {
        return -1;
    }

    offset = addr - MMIO_BASE;
    if ((offset / 4U) >= REG_COUNT) {
        return -2;
    }

    *value = m->regs[offset / 4U];
    return 0;
}
```

Write tương tự.

##### Solution Bài 27 bổ sung — Fake MMIO write

```c
int fake_mmio_write32(struct fake_mmio *m,
                      uintptr_t addr,
                      uint32_t value)
{
    uintptr_t offset;

    if ((m == NULL) ||
        (addr < MMIO_BASE) ||
        ((addr & 3U) != 0U)) {
        return -1;
    }

    offset = addr - MMIO_BASE;
    if ((offset / 4U) >= REG_COUNT) {
        return -2;
    }

    m->regs[offset / 4U] = value;
    return 0;
}
```

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_27_BEGIN -->
#### 9. Wizard Lab — Register model có side-effect hook

**Ý nghĩ quái dị:** Fake MMIO không chỉ là array; nếu write doorbell phải tạo event còn read status phải clear bit thì mô hình thế nào?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
typedef int (*reg_write_hook)(uint32_t offset, uint32_t value, void *ctx);
int reg_bank_write(struct reg_bank *b, uint32_t off, uint32_t value);
void reg_bank_set_hook(struct reg_bank *b, reg_write_hook hook, void *ctx);
```

1. **Bậc 1:** Hook doorbell tăng IRQ-pending counter.
2. **Bậc 2:** Tạo write-one-to-clear status register.
3. **Bậc 3:** Ghi unaligned/out-of-range; bank và side effect phải giữ nguyên.

**Observer bắt buộc:** Register diff + side-effect/event trace.

**Invariant:** Validation trước hook; invalid access không tạo side effect.

**Tự chế spell tiếp theo:** Tạo descriptor table mô tả permission RO/RW/W1C thay switch dài.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 27</strong></summary>

Lưu thành `practice/wizard_task_27.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define REG_COUNT 8U
typedef int (*reg_write_hook)(uint32_t offset, uint32_t value, void *ctx);
struct reg_bank { uint32_t reg[REG_COUNT]; reg_write_hook hook; void *ctx; };

static void reg_bank_set_hook(struct reg_bank *b, reg_write_hook hook, void *ctx)
{
    if (b != NULL) { b->hook = hook; b->ctx = ctx; }
}

static int reg_bank_write(struct reg_bank *b, uint32_t off, uint32_t value)
{
    if (b == NULL || (off % 4U) != 0U || off / 4U >= REG_COUNT) return -1;
    if (b->hook != NULL && b->hook(off,value,b->ctx) != 0) return -1;
    b->reg[off/4U] = value; return 0;
}

static int reject_start_zero(uint32_t off, uint32_t value, void *ctx)
{
    size_t *calls = ctx; ++*calls;
    return off == 0U && value == 0U ? -1 : 0;
}

int main(void)
{
    struct reg_bank b = {0}; size_t calls = 0U;
    reg_bank_set_hook(&b,reject_start_zero,&calls);
    assert(reg_bank_write(&b,0U,0U) == -1 && b.reg[0] == 0U);
    assert(reg_bank_write(&b,4U,7U) == 0 && b.reg[1] == 7U && calls == 2U);
    puts("wizard task 27 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_27.c -o wizard_task_27
./wizard_task_27
```

Expected cuối output:

```text
wizard task 27 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_27_END -->

---

#### Forge F35 — Bài 27: fake MMIO bank phải mô hình hóa side effect

Tự code aligned read/write, bounds theo register index và W1C cho IRQ status.
Unknown address phải reject, không modulo vào bank.

<details>
<summary>Đáp án F35</summary>

```c
#include <stddef.h>
#include <stdint.h>
#define MBASE ((uintptr_t)UINT32_C(0x60002000))
#define MREGS 8U
#define IRQ_STATUS 4U
#define IRQ_ACK 5U
struct mmio{uint32_t r[MREGS];};
static int midx(uintptr_t a,size_t*out)
{uintptr_t d;if((out==NULL)||(a<MBASE))return-1;d=a-MBASE;
 if((d&3U)!=0U||(d/4U)>=MREGS)return-1;*out=(size_t)(d/4U);return 0;}
int mmio_read(struct mmio*m,uintptr_t a,uint32_t*out)
{size_t k;if((m==NULL)||(out==NULL)||midx(a,&k))return-1;*out=m->r[k];return 0;}
int mmio_write(struct mmio*m,uintptr_t a,uint32_t v)
{size_t k;if((m==NULL)||midx(a,&k))return-1;
 if(k==IRQ_ACK)m->r[IRQ_STATUS]&=~v;else m->r[k]=v;return 0;}
```

Biến thể: STATUS read-only, CTRL RESET tự clear, DOORBELL tăng counter observer.

</details>

## Level 24 — DMA ownership state machine

DMA cho device đọc/ghi memory mà CPU không copy từng byte.

Đừng nghĩ “DMA = memcpy nhanh”. Quan trọng hơn là ownership.

State học tập:

```text
FREE
 ↓ CPU reserve
CPU_OWNED
 ↓ descriptor published
DMA_OWNED
 ↓ completion IRQ
DONE
 ↓ CPU consume/recycle
FREE
```

### Bài 28 — DMA slot state validation

#### 1. Đề bài nhỏ

Viết functions chỉ cho phép transition hợp lệ.

Ví dụ:

```text
FREE -> DMA_OWNED
```

phải reject vì thiếu bước CPU prepare.

#### 2. Tự đoán chương trình cần làm gì

Mỗi API chỉ nhận một số transition ownership hợp lệ. Transition nhảy cóc phải fail và state cũ phải được giữ nguyên.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- `enum` state
- transition validation
- DMA/cache ownership protocol

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

Kiến thức đã có sẵn trong đề cũ:

**Cache concept**

Nếu CPU cache và DMA nhìn memory khác thời điểm, firmware cần protocol kiểu:

```text
CPU fills descriptor
cache clean
release fence
OWN=1
ring doorbell
```

RX có thể cần invalidate trước CPU đọc data DMA vừa ghi.

Không cần hardware cache thật ở host; simulator vẫn nên ghi trace để enforce contract.

#### 4. Tự code

Làm trong `practice/task_28.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Duyệt từng transition hợp lệ và ít nhất ba transition sai; sai phải trả lỗi và giữ nguyên state.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_28.c \
    -o build/task_28
./build/task_28
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

Log `old,event,new/REJECT`. Nếu state đổi khi fail, update đang xảy ra trước validation.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** DMA ownership state machine.

**Bạn vừa code cái gì trong modem?** DMA và CPU không được cùng tùy ý sửa một buffer. State FREE→CPU_OWNED→DMA_OWNED→DONE→FREE định nghĩa quyền truy cập.

**Nó nằm ở đâu?** SoC data movement.

```text
INPUT  → buffer descriptor + state transition
KHỐI   → code của Bài 28
OUTPUT → ownership hợp lệ
```

**Tại sao modem cần nó?** Nếu CPU sửa buffer khi device đang đọc, lỗi có thể nondeterministic. Đây là một trong những concept firmware quan trọng hơn cả cú pháp descriptor.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 28 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

```c
enum dma_state {
    DMA_FREE = 0,
    DMA_CPU_OWNED,
    DMA_DMA_OWNED,
    DMA_DONE
};

bool dma_transition_allowed(enum dma_state from,
                            enum dma_state to)
{
    switch (from) {
    case DMA_FREE:
        return to == DMA_CPU_OWNED;
    case DMA_CPU_OWNED:
        return to == DMA_DMA_OWNED;
    case DMA_DMA_OWNED:
        return to == DMA_DONE;
    case DMA_DONE:
        return to == DMA_FREE;
    default:
        return false;
    }
}
```

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_28_BEGIN -->
#### 9. Wizard Lab — DMA transition script

**Ý nghĩ quái dị:** Nếu ownership là state machine, mình có thể feed cả script transition và tự tìm bước đầu tiên invalid không?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
struct dma_step { enum dma_event event; const char *label; };
int dma_run_script(struct dma_slot *slot, const struct dma_step *steps,
                   size_t count, size_t *failed_at);
```

1. **Bậc 1:** Run happy path CPU fill→clean→publish→DMA done→invalidate→CPU.
2. **Bậc 2:** Bỏ cache clean và xác nhận fail đúng step.
3. **Bậc 3:** Duplicate completion hoặc ring doorbell hai lần.

**Observer bắt buộc:** From/event/to trace, failed index và owner.

**Invariant:** Invalid step giữ state; một transition chỉ có một commit point.

**Tự chế spell tiếp theo:** Generate mọi sequence dài≤5 và liệt kê sequence hợp lệ.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 28</strong></summary>

Lưu thành `practice/wizard_task_28.c`:

```c
#include <assert.h>
#include <stdio.h>

enum dma_event { DMA_PREPARE, DMA_SUBMIT, DMA_IRQ_DONE, DMA_RELEASE };
enum dma_state { DMA_FREE, DMA_READY, DMA_IN_FLIGHT, DMA_COMPLETE };
struct dma_step { enum dma_event event; const char *label; };
struct dma_slot { enum dma_state state; };

static int dma_apply(struct dma_slot *s, enum dma_event e)
{
    if (s == NULL) return -1;
    if (s->state == DMA_FREE && e == DMA_PREPARE) s->state = DMA_READY;
    else if (s->state == DMA_READY && e == DMA_SUBMIT) s->state = DMA_IN_FLIGHT;
    else if (s->state == DMA_IN_FLIGHT && e == DMA_IRQ_DONE) s->state = DMA_COMPLETE;
    else if (s->state == DMA_COMPLETE && e == DMA_RELEASE) s->state = DMA_FREE;
    else return -1;
    return 0;
}

static int dma_run_script(struct dma_slot *slot, const struct dma_step *steps,
                          size_t count, size_t *failed_at)
{
    if (slot == NULL || steps == NULL || failed_at == NULL) return -1;
    *failed_at = count;
    for (size_t k = 0; k < count; ++k) if (dma_apply(slot,steps[k].event) != 0) {
        printf("failed: %s\n",steps[k].label); *failed_at = k; return -1;
    }
    return 0;
}

int main(void)
{
    struct dma_slot slot = {DMA_FREE}; size_t failed;
    const struct dma_step good[] = {{DMA_PREPARE,"prepare"},{DMA_SUBMIT,"submit"},{DMA_IRQ_DONE,"done"},{DMA_RELEASE,"release"}};
    const struct dma_step bad[] = {{DMA_SUBMIT,"submit-too-early"}};
    assert(dma_run_script(&slot,good,4U,&failed) == 0 && slot.state == DMA_FREE);
    assert(dma_run_script(&slot,bad,1U,&failed) == -1 && failed == 0U);
    puts("wizard task 28 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_28.c -o wizard_task_28
./wizard_task_28
```

Expected cuối output:

```text
wizard task 28 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_28_END -->

---

#### Forge F36 — Bài 28: DMA ownership là state machine, không phải vài assignment

Tự code transition validator, submit và completion. Cache clean phải xảy ra
trước publish device ownership; invalidate trước CPU đọc DONE.

<details>
<summary>Đáp án F36</summary>

```c
#include <stddef.h>
enum ds{D_FREE,D_CPU,D_DEVICE,D_DONE};
struct cache_ops{void(*clean)(void*,const void*,size_t);void(*invalidate)(void*,void*,size_t);};
struct dma_slot{enum ds state;void*p;size_t n;};
int dma_acquire(struct dma_slot*s)
{if((s==NULL)||(s->state!=D_FREE))return-1;s->state=D_CPU;return 0;}
int dma_submit(struct dma_slot*s,const struct cache_ops*o,void*ctx)
{if((s==NULL)||(o==NULL)||(o->clean==NULL)||(s->state!=D_CPU)||
 (s->p==NULL&&s->n!=0U))return-1;o->clean(ctx,s->p,s->n);s->state=D_DEVICE;return 0;}
int dma_complete_irq(struct dma_slot*s)
{if((s==NULL)||(s->state!=D_DEVICE))return-1;s->state=D_DONE;return 0;}
int dma_reclaim(struct dma_slot*s,const struct cache_ops*o,void*ctx)
{if((s==NULL)||(o==NULL)||(o->invalidate==NULL)||(s->state!=D_DONE))return-1;
 o->invalidate(ctx,s->p,s->n);s->state=D_CPU;return 0;}
int dma_release(struct dma_slot*s)
{if((s==NULL)||(s->state!=D_CPU))return-1;s->state=D_FREE;return 0;}
```

Trong target thật thêm release/acquire fence và descriptor OWN bit; callback
counter trong test phải chứng minh đúng thứ tự, không chỉ final state.

</details>

# PHẦN XI — L2: TỪ TRANSPORT BLOCK ĐẾN DỮ LIỆU

## Level 25 — MAC multiplexing

MAC có thể gom nhiều logical payload vào một transport block.

Mini format học tập:

```text
[LCID][LEN_LO][LEN_HI][payload...]
[LCID][LEN_LO][LEN_HI][payload...]
...
```

Parser phải kiểm tra trước khi đọc.

### Bài 29 — MAC multiplex

#### 1. Đề bài nhỏ

Định nghĩa:

```c
struct mac_sdu {
    uint8_t lcid;
    const uint8_t *data;
    uint16_t length;
};
```

Viết serialize vào output buffer có capacity.

#### 2. Tự đoán chương trình cần làm gì

Nhiều SDU được serialize nối tiếp vào một PDU có header/length rõ ràng. Trước mọi write phải chứng minh output còn đủ capacity.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- wire header
- cursor/capacity
- serialize từng field thay vì `memcpy(struct)`

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_29.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Test 0/1/nhiều SDU, payload rỗng nếu contract cho phép, và output thiếu đúng 1 byte.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_29.c \
    -o build/task_29
./build/task_29
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

In cursor, needed, capacity trước mỗi field. Nếu fail muộn, capacity check đang đặt sau write.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** MAC multiplexing.

**Bạn vừa code cái gì trong modem?** MAC gom nhiều logical payload/control element vào một transport unit để PHY truyền trong cơ hội radio hiện tại.

**Nó nằm ở đâu?** Layer 2 MAC.

```text
INPUT  → nhiều SDU/control elements
KHỐI   → code của Bài 29
OUTPUT → một MAC PDU
```

**Tại sao modem cần nó?** PHY không cần biết từng application message. MAC là lớp đóng gói gần scheduler/radio nhất, nối protocol data với transport block.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 29 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

Wire format học tập của mỗi SDU:

```text
1 byte  LCID
2 byte  length little-endian
N byte  payload
```

##### Multiplex

```c
#include <stddef.h>
#include <stdint.h>

struct mac_sdu {
    uint8_t lcid;
    const uint8_t *data;
    uint16_t length;
};

int mac_mux(const struct mac_sdu *sdus,
            size_t count,
            uint8_t *out,
            size_t cap,
            size_t *written)
{
    size_t pos = 0U;

    if ((sdus == NULL) || (out == NULL) || (written == NULL)) {
        return -1;
    }

    for (size_t i = 0U; i < count; ++i) {
        size_t need = 3U + (size_t)sdus[i].length;

        if ((sdus[i].length != 0U) && (sdus[i].data == NULL)) {
            return -2;
        }

        if ((need > cap) || (pos > cap - need)) {
            return -3;
        }

        out[pos++] = sdus[i].lcid;
        out[pos++] = (uint8_t)sdus[i].length;
        out[pos++] = (uint8_t)(sdus[i].length >> 8U);

        for (uint16_t j = 0U; j < sdus[i].length; ++j) {
            out[pos++] = sdus[i].data[j];
        }
    }

    *written = pos;
    return 0;
}
```

##### Demultiplex

Parser này trả từng view vào input buffer; caller không được giữ pointer sau khi input buffer bị recycle.

```c
struct mac_view {
    uint8_t lcid;
    const uint8_t *data;
    uint16_t length;
};

int mac_demux(const uint8_t *pdu,
              size_t pdu_len,
              struct mac_view *out,
              size_t out_cap,
              size_t *out_count)
{
    size_t pos = 0U;
    size_t n = 0U;

    if ((pdu == NULL) || (out == NULL) || (out_count == NULL)) {
        return -1;
    }

    while (pos < pdu_len) {
        uint16_t len;

        if (pdu_len - pos < 3U) {
            return -2;
        }
        if (n >= out_cap) {
            return -3;
        }

        out[n].lcid = pdu[pos++];
        len = (uint16_t)pdu[pos] |
              (uint16_t)((uint16_t)pdu[pos + 1U] << 8U);
        pos += 2U;

        if ((size_t)len > pdu_len - pos) {
            return -4;
        }

        out[n].length = len;
        out[n].data = &pdu[pos];
        pos += (size_t)len;
        ++n;
    }

    *out_count = n;
    return 0;
}
```

Case `header length=1000` nhưng chỉ còn 10 byte sẽ fail tại bounds check trước khi tạo view.

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_29_BEGIN -->
#### 9. Wizard Lab — MAC layout composer

**Ý nghĩ quái dị:** Nếu muốn tự 'vẽ' PDU từ recipe SDU/control element thay vì hard-code buffer, helper nào giúp compose an toàn?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
int mac_builder_begin(struct mac_builder *b, uint8_t *out, size_t cap);
int mac_builder_add_sdu(struct mac_builder *b, uint8_t lcid,
                        const uint8_t *data, uint16_t len);
int mac_builder_finish(struct mac_builder *b, size_t *out_len);
```

1. **Bậc 1:** Build cùng SDU theo ba thứ tự và dump wire layout.
2. **Bậc 2:** Fill capacity đúng biên và thiếu một byte.
3. **Bậc 3:** Tạo recipe array rồi loop builder.

**Observer bắt buộc:** Cursor/capacity, hex layout và parsed child list.

**Invariant:** Builder fail không ghi PDU dở được coi là complete; length field đúng payload.

**Tự chế spell tiếp theo:** Thêm dry-run mode chỉ tính required bytes mà không ghi output.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 29</strong></summary>

Lưu thành `practice/wizard_task_29.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct mac_builder { uint8_t *out; size_t cap, used; int begun; };

static int mac_builder_begin(struct mac_builder *b, uint8_t *out, size_t cap)
{
    if (b == NULL || out == NULL) return -1;
    *b = (struct mac_builder){out,cap,0U,1}; return 0;
}

static int mac_builder_add_sdu(struct mac_builder *b, uint8_t lcid,
                               const uint8_t *data, uint16_t len)
{
    if (b == NULL || data == NULL || b->begun == 0 || b->used > b->cap ||
        (size_t)len > b->cap - b->used || b->cap - b->used - len < 3U) return -1;
    b->out[b->used++] = lcid;
    b->out[b->used++] = (uint8_t)(len >> 8U);
    b->out[b->used++] = (uint8_t)len;
    memcpy(&b->out[b->used],data,len); b->used += len; return 0;
}

static int mac_builder_finish(struct mac_builder *b, size_t *out_len)
{
    if (b == NULL || out_len == NULL || b->begun == 0) return -1;
    *out_len = b->used; b->begun = 0; return 0;
}

int main(void)
{
    uint8_t out[16]; const uint8_t sdu[3] = {1U,2U,3U};
    struct mac_builder b; size_t len;
    assert(mac_builder_begin(&b,out,sizeof out) == 0);
    assert(mac_builder_add_sdu(&b,4U,sdu,3U) == 0);
    assert(mac_builder_finish(&b,&len) == 0 && len == 6U && out[0] == 4U);
    puts("wizard task 29 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_29.c -o wizard_task_29
./wizard_task_29
```

Expected cuối output:

```text
wizard task 29 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_29_END -->

---

#### Forge F37 — Bài 29: MAC encoder phải canonical và BSR có invariant

Tự code một sub-SDU header `{lcid,reserved=0,length:u16 LE}`, append payload và
BSR update chống overflow/underflow.

<details>
<summary>Đáp án F37</summary>

```c
#include <stddef.h>
#include <stdint.h>
int mac_one(uint8_t*out,size_t cap,uint8_t lcid,const uint8_t*p,size_t n,size_t*used)
{size_t k;if((out==NULL)||(used==NULL)||((p==NULL)&&(n!=0U))||n==0U||
 n>UINT16_MAX||cap<4U||n>cap-4U)return-1;
 out[0]=lcid;out[1]=0U;out[2]=(uint8_t)n;out[3]=(uint8_t)(n>>8);
 for(k=0U;k<n;++k)out[4U+k]=p[k];*used=4U+n;return 0;}
int bsr_enqueue(uint32_t*b,size_t n)
{if((b==NULL)||(n>UINT32_MAX)||(uint32_t)n>UINT32_MAX-*b)return-1;*b+=(uint32_t)n;return 0;}
int bsr_transmit(uint32_t*b,size_t n)
{if((b==NULL)||(n>*b))return-1;*b-=(uint32_t)n;return 0;}
```

Production BMAC có magic/version/count và 1..8 SDU; card này chỉ luyện atomic
field encoding. Encoder fail không được thay BSR.

</details>

### Bài 30 — MAC demultiplex defensive

#### 1. Đề bài nhỏ

Nếu header nói length=1000 nhưng buffer còn 10 byte:

```text
reject
```

Không crash, không đọc quá buffer.

Đây là mindset parser quan trọng hơn thuộc format.

#### 2. Tự đoán chương trình cần làm gì

Parser giữ cursor và số byte còn lại, validate header/length trước khi đọc payload. Input hỏng trả lỗi chứ không đọc vượt biên.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- untrusted length
- remaining-bytes check
- fail-closed parser

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_30.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Test valid PDU, truncated header, declared length lớn hơn remaining, unknown LCID và trailing bytes theo contract.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_30.c \
    -o build/task_30
./build/task_30
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

Với mỗi read, ghi lại `cursor` và `remaining`. Parser không được advance trước khi chứng minh còn đủ byte.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** Defensive MAC parsing.

**Bạn vừa code cái gì trong modem?** RX phải tách MAC PDU từ input không đáng tin. Length, count và boundary đều phải được validate trước khi advance pointer.

**Nó nằm ở đâu?** Layer 2 RX.

```text
INPUT  → MAC PDU bytes
KHỐI   → code của Bài 30
OUTPUT → các sub-PDU hợp lệ hoặc reject
```

**Tại sao modem cần nó?** Một lỗi radio/corruption không được biến thành out-of-bounds. Protocol parser trong modem là security boundary, không chỉ parser tiện lợi.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 30 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

Wire format học tập của mỗi SDU:

```text
1 byte  LCID
2 byte  length little-endian
N byte  payload
```

##### Multiplex

```c
#include <stddef.h>
#include <stdint.h>

struct mac_sdu {
    uint8_t lcid;
    const uint8_t *data;
    uint16_t length;
};

int mac_mux(const struct mac_sdu *sdus,
            size_t count,
            uint8_t *out,
            size_t cap,
            size_t *written)
{
    size_t pos = 0U;

    if ((sdus == NULL) || (out == NULL) || (written == NULL)) {
        return -1;
    }

    for (size_t i = 0U; i < count; ++i) {
        size_t need = 3U + (size_t)sdus[i].length;

        if ((sdus[i].length != 0U) && (sdus[i].data == NULL)) {
            return -2;
        }

        if ((need > cap) || (pos > cap - need)) {
            return -3;
        }

        out[pos++] = sdus[i].lcid;
        out[pos++] = (uint8_t)sdus[i].length;
        out[pos++] = (uint8_t)(sdus[i].length >> 8U);

        for (uint16_t j = 0U; j < sdus[i].length; ++j) {
            out[pos++] = sdus[i].data[j];
        }
    }

    *written = pos;
    return 0;
}
```

##### Demultiplex

Parser này trả từng view vào input buffer; caller không được giữ pointer sau khi input buffer bị recycle.

```c
struct mac_view {
    uint8_t lcid;
    const uint8_t *data;
    uint16_t length;
};

int mac_demux(const uint8_t *pdu,
              size_t pdu_len,
              struct mac_view *out,
              size_t out_cap,
              size_t *out_count)
{
    size_t pos = 0U;
    size_t n = 0U;

    if ((pdu == NULL) || (out == NULL) || (out_count == NULL)) {
        return -1;
    }

    while (pos < pdu_len) {
        uint16_t len;

        if (pdu_len - pos < 3U) {
            return -2;
        }
        if (n >= out_cap) {
            return -3;
        }

        out[n].lcid = pdu[pos++];
        len = (uint16_t)pdu[pos] |
              (uint16_t)((uint16_t)pdu[pos + 1U] << 8U);
        pos += 2U;

        if ((size_t)len > pdu_len - pos) {
            return -4;
        }

        out[n].length = len;
        out[n].data = &pdu[pos];
        pos += (size_t)len;
        ++n;
    }

    *out_count = n;
    return 0;
}
```

Case `header length=1000` nhưng chỉ còn 10 byte sẽ fail tại bounds check trước khi tạo view.

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_30_BEGIN -->
#### 9. Wizard Lab — Parser boundary explorer

**Ý nghĩ quái dị:** Nếu truncate cùng một PDU ở mọi byte offset, parser trả status nào và có commit partial child không?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
struct byte_view { const uint8_t *data; size_t len; };
struct byte_view prefix_view(const uint8_t *data, size_t len, size_t prefix);
void parser_sweep_truncation(const uint8_t *pdu, size_t len);
```

1. **Bậc 1:** Parse prefix length 0..full và lập histogram status.
2. **Bậc 2:** Patch length field lớn/nhỏ hơn payload.
3. **Bậc 3:** Thêm trailing byte và kiểm tra canonical policy.

**Observer bắt buộc:** Status, consumed bytes, child count và output digest.

**Invariant:** Failure không commit child/output state; không read ngoài view.

**Tự chế spell tiếp theo:** Tạo `parser_probe` trả first offset nơi behavior đổi từ truncated sang valid.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 30</strong></summary>

Lưu thành `practice/wizard_task_30.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

struct byte_view { const uint8_t *data; size_t len; };

static struct byte_view prefix_view(const uint8_t *data, size_t len, size_t prefix)
{
    if (prefix > len) prefix = len;
    return (struct byte_view){data,prefix};
}

static int toy_parse(struct byte_view v)
{
    if (v.data == NULL || v.len < 2U) return -1;
    const size_t payload = v.data[1];
    return payload <= v.len - 2U ? 0 : -1;
}

static void parser_sweep_truncation(const uint8_t *pdu, size_t len)
{
    for (size_t prefix = 0; prefix <= len; ++prefix) {
        const int rc = toy_parse(prefix_view(pdu,len,prefix));
        printf("prefix=%zu rc=%d\n",prefix,rc);
    }
}

int main(void)
{
    const uint8_t pdu[5] = {7U,3U,1U,2U,3U};
    assert(toy_parse(prefix_view(pdu,5U,4U)) == -1);
    assert(toy_parse(prefix_view(pdu,5U,5U)) == 0);
    parser_sweep_truncation(pdu,5U);
    puts("wizard task 30 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_30.c -o wizard_task_30
./wizard_task_30
```

Expected cuối output:

```text
wizard task 30 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_30_END -->

---

#### Forge F38 — Bài 30: demux phải validate toàn frame rồi mới dispatch

Tự code parser tối đa 8 sub-SDU vào descriptor tạm; reject reserved, zero length,
unknown LCID và trailing byte. Chỉ caller mới dispatch sau return success.

<details>
<summary>Đáp án F38</summary>

```c
#include <stddef.h>
#include <stdint.h>
struct sdu{uint8_t lcid;const uint8_t*p;uint16_t n;};
int mac_parse(const uint8_t*p,size_t n,struct sdu out[8],size_t*count)
{struct sdu tmp[8];size_t off=0U,c=0U;if((p==NULL)||(out==NULL)||(count==NULL))return-1;
 while(off<n){uint16_t len;if(c==8U||n-off<4U)return-1;
  if(p[off+1U]!=0U||p[off]>2U)return-1;
  len=(uint16_t)((uint16_t)p[off+2U]|((uint16_t)p[off+3U]<<8));off+=4U;
  if(len==0U||(size_t)len>n-off)return-1;tmp[c++]=(struct sdu){p[off-4U],&p[off],len};off+=len;}
 for(off=0U;off<c;++off)out[off]=tmp[off];*count=c;return c==0U?-1:0;}
```

Output descriptor borrow input; giữ input sống tới khi consumer xong hoặc đổi
sang owned handle trước khi enqueue task.

</details>

## Level 26 — HARQ intuition

HARQ = khi transmission lỗi, receiver có thể yêu cầu/rely on retransmission và kết hợp soft information.

Mini mental model:

```text
TX attempt 1 → CRC fail
                 │ keep soft LLR
TX attempt 2 → combine LLR
                 │
                 └→ CRC pass
```

`hard bit` chỉ nói:

```text
0 hoặc 1
```

LLR còn thể hiện confidence/sign.

Trong simplified implementation có thể cộng LLR có saturation.

### Bài 31 — Soft combine

#### 1. Đề bài nhỏ

Cho:

```text
attempt1 = [+10, -3, +2, -20]
attempt2 = [+8,  -9, -5, -12]
```

combine element-wise.

Sau combine, hard decision dựa vào sign.

Sau đó thêm saturating add `int16_t`.

#### 2. Tự đoán chương trình cần làm gì

Các LLR ở cùng vị trí được cộng có saturation; dấu của tổng quyết định bit. Retransmission bổ sung evidence thay vì thay thế nó.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- LLR sign
- element-wise combine
- saturating add

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_31.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Test vector trong đề, giá trị gần biên dương/âm và saturation; hard decision phải theo dấu tổng.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_31.c \
    -o build/task_31
./build/task_31
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

In hai LLR và tổng rộng trước saturation. Nếu decision đảo, quy ước sign TX/RX không thống nhất.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** HARQ soft combining intuition.

**Bạn vừa code cái gì trong modem?** Khi một transmission không đủ tốt, receiver có thể giữ soft information và kết hợp với retransmission thay vì vứt mọi thứ.

**Nó nằm ở đâu?** MAC/PHY HARQ boundary.

```text
INPUT  → LLR lần trước + LLR lần mới
KHỐI   → code của Bài 31
OUTPUT → LLR tích lũy mạnh hơn
```

**Tại sao modem cần nó?** Soft combine giải thích tại sao HARQ process phải giữ state per process. Đây là cầu nối giữa PHY reliability và MAC retransmission logic.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 31 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

```c
void combine_llr(const int16_t *a,
                 const int16_t *b,
                 int16_t *out,
                 size_t count)
{
    if ((a == NULL) || (b == NULL) || (out == NULL)) {
        return;
    }

    for (size_t i = 0U; i < count; ++i) {
        out[i] = sat16((int32_t)a[i] + (int32_t)b[i]);
    }
}
```

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_31_BEGIN -->
#### 9. Wizard Lab — LLR painter cho HARQ

**Ý nghĩ quái dị:** Nếu tự paint vùng uncertain, đảo dấu một window hoặc saturate attempt, combine sẽ cứu được bit nào?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
void llr_fill_range(int16_t *llr, size_t n, size_t first,
                    size_t count, int16_t value);
void llr_flip_sign_range(int16_t *llr, size_t n, size_t first, size_t count);
size_t llr_hard_diff(const int16_t *a, const int16_t *b, size_t n);
```

1. **Bậc 1:** Zero LLR bit 4..7 ở attempt1, mạnh ở attempt2.
2. **Bậc 2:** Đảo dấu cùng window ở một attempt.
3. **Bậc 3:** Sweep số attempt và đếm bit recovered.

**Observer bắt buộc:** Per-bit LLR before/after, saturation và hard-bit diff.

**Invariant:** Combine element-wise đúng identity/process; saturation không wrap.

**Tự chế spell tiếp theo:** Tạo heatmap attempt×bit dưới dạng CSV local.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 31</strong></summary>

Lưu thành `practice/wizard_task_31.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static void llr_fill_range(int16_t *llr, size_t n, size_t first,
                           size_t count, int16_t value)
{
    if (llr == NULL || first >= n) return;
    if (count > n - first) count = n - first;
    for (size_t k = first; k < first + count; ++k) llr[k] = value;
}

static void llr_flip_sign_range(int16_t *llr, size_t n,
                                size_t first, size_t count)
{
    if (llr == NULL || first >= n) return;
    if (count > n - first) count = n - first;
    for (size_t k = first; k < first + count; ++k)
        llr[k] = llr[k] == INT16_MIN ? INT16_MAX : (int16_t)-llr[k];
}

static size_t llr_hard_diff(const int16_t *a, const int16_t *b, size_t n)
{
    size_t changed = 0U;
    if (a == NULL || b == NULL) return 0U;
    for (size_t k = 0; k < n; ++k) if ((a[k] < 0) != (b[k] < 0)) ++changed;
    return changed;
}

int main(void)
{
    const int16_t baseline[6] = {4,-4,4,-4,4,-4};
    int16_t probe[6] = {4,-4,4,-4,4,-4};
    llr_fill_range(probe,6U,1U,2U,1);
    assert(llr_hard_diff(baseline,probe,6U) == 1U);
    llr_flip_sign_range(probe,6U,0U,3U);
    assert(llr_hard_diff(baseline,probe,6U) == 2U);
    puts("wizard task 31 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_31.c -o wizard_task_31
./wizard_task_31
```

Expected cuối output:

```text
wizard task 31 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_31_END -->

---

#### Forge F39 — Bài 31: HARQ soft combine phải gắn đúng process

Tự code eight-process begin/combine/reset. Reject wrong id, length mismatch và
combine khi inactive; LLR cộng saturation.

<details>
<summary>Đáp án F39</summary>

```c
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#define HN 8U
#define HSOFT 64U
struct hp{int active;uint32_t count;size_t n;int16_t llr[HSOFT];unsigned tx;};
struct harq{struct hp p[HN];};
static int16_t hs(int32_t x){return x>INT16_MAX?INT16_MAX:(x<INT16_MIN?INT16_MIN:(int16_t)x);}
int harq_begin(struct harq*h,unsigned id,uint32_t count,size_t n)
{size_t k;if((h==NULL)||(id>=HN)||(n==0U)||(n>HSOFT)||h->p[id].active)return-1;
 h->p[id]=(struct hp){1,count,n,{0},1U};for(k=0U;k<n;++k)h->p[id].llr[k]=0;return 0;}
int harq_combine(struct harq*h,unsigned id,uint32_t count,const int16_t*x,size_t n)
{size_t k;struct hp*p;if((h==NULL)||(x==NULL)||(id>=HN))return-1;p=&h->p[id];
 if(!p->active||p->count!=count||p->n!=n)return-1;
 for(k=0U;k<n;++k)p->llr[k]=hs((int32_t)p->llr[k]+x[k]);++p->tx;return 0;}
int harq_reset(struct harq*h,unsigned id)
{if((h==NULL)||(id>=HN))return-1;h->p[id].active=0;h->p[id].n=0U;return 0;}
```

Thêm test interleaved id 2/id 5 để bắt shared-soft-buffer bug.

</details>

## Level 27 — RLC-UM reorder

Packets có sequence number.

Có thể đến:

```text
SN 10
SN 12
SN 11
```

Receiver giữ window/reorder slots và giao lên trên đúng policy.

### Wraparound

Nếu SN có 12 bit:

```text
4094, 4095, 0, 1
```

không thể compare naïve bằng `a > b`.

### Bài 32 — 4-bit sequence toy model

#### 1. Đề bài nhỏ

Đừng làm 12-bit ngay.

Sequence modulo 16.

Viết function xác định khoảng cách forward:

```c
uint8_t sn_distance4(uint8_t from, uint8_t to);
```

Test:

```text
14 -> 15 = 1
15 -> 0  = 1
0  -> 1  = 1
14 -> 1  = 3
```

Khi hiểu mới nâng lên 12 bit.

#### 2. Tự đoán chương trình cần làm gì

Khoảng cách sequence đi theo chiều forward trên vòng modulo 16, nên `15 → 0` là 1 chứ không phải số âm hay 15.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- modulo arithmetic
- mask 4 bit
- forward-distance invariant

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_32.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Bốn vector trong đề cùng mọi cặp `from,to` từ 0..15; output luôn nằm 0..15.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_32.c \
    -o build/task_32
./build/task_32
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

Mask input về 4 bit rồi trace phép trừ unsigned. Không dùng `abs(to-from)` vì vòng có hướng.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** Modulo sequence numbers.

**Bạn vừa code cái gì trong modem?** Sequence number hữu hạn sẽ wrap. So sánh integer thông thường thất bại quanh điểm wrap, nên protocol dùng modular distance/window.

**Nó nằm ở đâu?** RLC/PDCP/HARQ.

```text
INPUT  → SN nhỏ có wrap
KHỐI   → code của Bài 32
OUTPUT → order relation trong cửa sổ
```

**Tại sao modem cần nó?** Bản 4-bit làm bạn thấy trực giác trước khi lên SN 12/18-bit. Nếu logic wrap sai, packet mới sau wrap có thể bị coi là packet cũ.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 32 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

```c
uint8_t sn_distance4(uint8_t from, uint8_t to)
{
    from &= 0x0FU;
    to &= 0x0FU;
    return (uint8_t)((to - from) & 0x0FU);
}
```

Đây mới là distance primitive. Policy “newer/older” còn phụ thuộc window size.

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_32_BEGIN -->
#### 9. Wizard Lab — Sequence-number script generator

**Ý nghĩ quái dị:** Thay vì nghĩ wrap bằng vài case, mình có thể generate cả walk quanh modulo và reorder/drop/duplicate theo script không?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
size_t sn_walk4(uint8_t start, size_t steps, uint8_t *out, size_t cap);
void sn_script_swap(uint8_t *sn, size_t n, size_t a, size_t b);
void sn_script_duplicate(uint8_t *sn, size_t *n, size_t cap, size_t at);
```

1. **Bậc 1:** Walk từ 14 qua 15,0,1 rồi swap hai packet.
2. **Bậc 2:** Duplicate SN đúng trước/đúng sau wrap.
3. **Bậc 3:** Feed script vào reorder toy và ghi delivery timeline.

**Observer bắt buộc:** Expected/delivered SN, window state và pending slots.

**Invariant:** Distance nằm trong modulo; duplicate không deliver hai lần.

**Tự chế spell tiếp theo:** Parameter hóa bit width 4/6/12 bằng mask và test property.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 32</strong></summary>

Lưu thành `practice/wizard_task_32.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static size_t sn_walk4(uint8_t start, size_t steps, uint8_t *out, size_t cap)
{
    if (out == NULL) return 0U;
    if (steps > cap) steps = cap;
    for (size_t k = 0; k < steps; ++k) out[k] = (uint8_t)((start + k) & 0x0fU);
    return steps;
}

static void sn_script_swap(uint8_t *sn, size_t n, size_t a, size_t b)
{
    if (sn == NULL || a >= n || b >= n) return;
    const uint8_t tmp = sn[a]; sn[a] = sn[b]; sn[b] = tmp;
}

static void sn_script_duplicate(uint8_t *sn, size_t *n, size_t cap, size_t at)
{
    if (sn == NULL || n == NULL || *n >= cap || at >= *n) return;
    for (size_t k = *n; k > at + 1U; --k) sn[k] = sn[k-1U];
    sn[at+1U] = sn[at]; ++*n;
}

int main(void)
{
    uint8_t sn[8]; size_t n = sn_walk4(14U,5U,sn,8U);
    assert(n == 5U && sn[0] == 14U && sn[2] == 0U);
    sn_script_swap(sn,n,0U,4U);
    sn_script_duplicate(sn,&n,8U,2U);
    assert(n == 6U && sn[2] == sn[3]);
    puts("wizard task 32 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_32.c -o wizard_task_32
./wizard_task_32
```

Expected cuối output:

```text
wizard task 32 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_32_END -->

---

#### Forge F40 — Bài 32: modular SN phải dùng distance, không dùng `<` thường

Tự code normalize SN4, forward distance và window membership. Sau đó đổi modulus
từ 16 sang 4096 cho RLC SN12.

<details>
<summary>Đáp án F40</summary>

```c
#include <stdint.h>
uint16_t sn_distance(uint16_t from,uint16_t to,uint16_t modulus)
{return(uint16_t)((to+modulus-from)%modulus);}
int sn_in_forward_window(uint16_t base,uint16_t candidate,uint16_t modulus,uint16_t window)
{uint16_t d;if(modulus==0U||window==0U||window>modulus/2U||base>=modulus||candidate>=modulus)return 0;
 d=sn_distance(base,candidate,modulus);return d<window;}
int sn_before(uint16_t a,uint16_t b,uint16_t modulus)
{uint16_t d;if(modulus==0U||a>=modulus||b>=modulus)return 0;
 d=sn_distance(a,b,modulus);return d!=0U&&d<modulus/2U;}
```

Test `14,15,0,1` quanh wrap; quan hệ ở đúng half-modulus phải được contract xử
lý riêng, không tự coi là before.

</details>

## Level 28 — PDCP COUNT, replay và reorder

PDCP có sequence number và higher-order counter logic.

Concept cần nắm:

```text
COUNT = HFN || SN
```

Receiver phải phân biệt:

- packet mới;
- duplicate;
- packet cũ ngoài window;
- reorder hợp lệ;
- wraparound.

### Bài 33 — Anti-replay bitmap toy

#### 1. Đề bài nhỏ

Dùng window 8 packet.

Giữ:

```text
highest_count
bitmap 8 bit
```

Process sequence:

```text
100 accept
101 accept
101 reject duplicate
99  accept nếu còn window và chưa thấy
90  reject quá cũ
```

Không cần đúng PDCP spec ngay. Mục tiêu học sliding replay window.

#### 2. Tự đoán chương trình cần làm gì

Packet mới đẩy cửa sổ; packet cũ trong window chỉ được nhận một lần; packet quá cũ hoặc duplicate bị reject.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- sliding window
- bitmap shift/set/test
- duplicate và too-old classification

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_33.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Chạy sequence `100,101,101,99,90`; thêm jump xa làm bitmap dịch quá window.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_33.c \
    -o build/task_33
./build/task_33
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

In highest/bitmap dạng binary trước và sau packet. Tách ba case: newer, inside-window, too-old.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** Anti-replay window.

**Bạn vừa code cái gì trong modem?** Receiver cần nhớ packet nào trong một cửa sổ COUNT/SN đã thấy để loại duplicate/replay mà vẫn chấp nhận reorder hợp lệ.

**Nó nằm ở đâu?** PDCP/security.

```text
INPUT  → sequence/count
KHỐI   → code của Bài 33
OUTPUT → accept hoặc reject + bitmap state
```

**Tại sao modem cần nó?** Đây không chỉ là 'security feature'; nó là stateful protocol logic. Bitmap/window tránh lưu mọi sequence từng thấy từ đầu phiên.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 33 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

```c
#include <stdbool.h>
#include <stdint.h>

struct replay8 {
    uint32_t highest;
    uint8_t bitmap;
    bool initialized;
};

bool replay8_accept(struct replay8 *r, uint32_t count)
{
    if (r == NULL) {
        return false;
    }

    if (!r->initialized) {
        r->initialized = true;
        r->highest = count;
        r->bitmap = 1U;
        return true;
    }

    if (count > r->highest) {
        uint32_t delta = count - r->highest;

        if (delta >= 8U) {
            r->bitmap = 1U;
        } else {
            r->bitmap = (uint8_t)((r->bitmap << delta) | 1U);
        }

        r->highest = count;
        return true;
    }

    {
        uint32_t age = r->highest - count;

        if (age >= 8U) {
            return false;
        }

        uint8_t mask = (uint8_t)(1U << age);
        if ((r->bitmap & mask) != 0U) {
            return false;
        }

        r->bitmap |= mask;
        return true;
    }
}
```

Sequence kiểm tra:

```text
100 → accept
101 → accept
101 → reject duplicate
99  → accept
90  → reject old
```

Đây là toy `uint32_t COUNT` chưa xử lý wrap kiểu PDCP production; wrap được học riêng bằng modular comparison.

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_33_BEGIN -->
#### 9. Wizard Lab — Replay-window command language

**Ý nghĩ quái dị:** Nếu muốn mô tả `NEW 100, NEW 101, DUP 101, OLD 99, FAR 120` thành script C, helper nào giúp chạy/replay?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
struct count_step { uint32_t count; int expected_accept; };
int replay_run_script(struct replay_window *w,
                      const struct count_step *steps, size_t n);
```

1. **Bậc 1:** Script baseline trong bài rồi clone window.
2. **Bậc 2:** Jump xa đúng window size và lớn hơn window.
3. **Bậc 3:** Permute packet trong window và kiểm bitmap.

**Observer bắt buộc:** Highest, bitmap binary, decision reason.

**Invariant:** Duplicate/too-old reject không đổi window; accepted new packet update đúng một lần.

**Tự chế spell tiếp theo:** Viết `replay_explain()` trả enum reason thay vì chỉ bool.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 33</strong></summary>

Lưu thành `practice/wizard_task_33.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

struct replay_window { uint32_t highest; uint32_t bitmap; int initialized; };
struct count_step { uint32_t count; int expected_accept; };

static int replay_accept(struct replay_window *w, uint32_t count)
{
    if (w == NULL) return 0;
    if (w->initialized == 0) { *w = (struct replay_window){count,1U,1}; return 1; }
    if (count > w->highest) {
        const uint32_t delta = count - w->highest;
        w->bitmap = delta >= 32U ? 1U : (w->bitmap << delta) | 1U;
        w->highest = count; return 1;
    }
    const uint32_t age = w->highest - count;
    if (age >= 32U) return 0;
    const uint32_t mask = UINT32_C(1) << age;
    if ((w->bitmap & mask) != 0U) return 0;
    w->bitmap |= mask; return 1;
}

static int replay_run_script(struct replay_window *w,
                             const struct count_step *steps, size_t n)
{
    if (w == NULL || steps == NULL) return -1;
    for (size_t k = 0; k < n; ++k)
        if (replay_accept(w,steps[k].count) != steps[k].expected_accept) return -1;
    return 0;
}

int main(void)
{
    struct replay_window w = {0};
    const struct count_step script[] = {{10U,1},{11U,1},{10U,0},{9U,1},{9U,0},{50U,1},{10U,0}};
    assert(replay_run_script(&w,script,sizeof script/sizeof script[0]) == 0);
    puts("wizard task 33 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_33.c -o wizard_task_33
./wizard_task_33
```

Expected cuối output:

```text
wizard task 33 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_33_END -->

---

#### Forge F41 — Bài 33: anti-replay bitmap phải xử lý ahead/behind/duplicate

Tự code cửa sổ 64 COUNT, first packet, jump lớn và packet cũ trong window.

<details>
<summary>Đáp án F41</summary>

```c
#include <stdint.h>
struct replay{uint32_t highest;uint64_t seen;int initialized;};
int replay_accept(struct replay*r,uint32_t count)
{uint32_t d;uint64_t bit;if(r==NULL)return-1;
 if(!r->initialized){r->initialized=1;r->highest=count;r->seen=1U;return 1;}
 if(count>r->highest){d=count-r->highest;r->seen=d>=64U?1U:(r->seen<<d)|1U;
  r->highest=count;return 1;}
 d=r->highest-count;if(d>=64U)return 0;bit=UINT64_C(1)<<d;
 if((r->seen&bit)!=0U)return 0;r->seen|=bit;return 1;}
```

PDCP thật dùng modular COUNT và HFN derivation; card này dùng monotonic `uint32_t`
để tách riêng bitmap mechanics trước.

</details>

# PHẦN XII — RRC/NAS: MODEM LÀ STATE MACHINE LỚN

## Level 29 — State machine trước protocol chi tiết

Mini RRC:

```text
OFF
 ↓ beacon/search
SEARCHING
 ↓ cell found
CAMPED
 ↓ setup
CONNECTING
 ↓ complete
CONNECTED
```

Mini NAS:

```text
DEREGISTERED
 ↓ registration start
REGISTERING
 ↓ registration accept
REGISTERED
 ↓ session accept
SESSION_ACTIVE
```

### Sai lầm phổ biến

Code kiểu:

```c
if (packet.type == SESSION_ACCEPT)
    nas = SESSION_ACTIVE;
```

là sai về state discipline.

Phải check:

```text
from
+ event
+ guard
+ action
→ to
```

### Bài 34 — RRC toy machine

#### 1. Đề bài nhỏ

Events:

```text
EV_BOOT
EV_CELL_FOUND
EV_SETUP
EV_SETUP_COMPLETE
EV_LINK_LOSS
```

Yêu cầu:

- invalid event không đổi state;
- mọi transition log `from/event/to`;
- link loss từ CONNECTED quay về SEARCHING.

#### 2. Tự đoán chương trình cần làm gì

State + event quyết định state kế tiếp. Event không hợp lệ giữ nguyên state và mọi transition hợp lệ đều được log.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- enum state/event
- explicit transition function
- log `from/event/to`

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_34.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Test happy path tới CONNECTED, invalid event ở mỗi state và LINK_LOSS quay SEARCHING.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_34.c \
    -o build/task_34
./build/task_34
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

Log cả transition bị reject. Nếu event hợp lệ ở sai state vẫn chạy, switch đang chỉ xét event mà bỏ state.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** RRC control state.

**Bạn vừa code cái gì trong modem?** RRC không xử lý mọi event ở mọi thời điểm. State machine định nghĩa event nào hợp lệ khi SEARCHING/CAMPED/CONNECTED.

**Nó nằm ở đâu?** control plane RRC.

```text
INPUT  → current state + event
KHỐI   → code của Bài 34
OUTPUT → new state + actions
```

**Tại sao modem cần nó?** Một biến boolean `connected` không đủ để biểu diễn quá trình cell search/connect/recovery. Guard/action/to giúp state transition auditable.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 34 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

```c
enum rrc_state {
    RRC_OFF = 0,
    RRC_SEARCHING,
    RRC_CAMPED,
    RRC_CONNECTING,
    RRC_CONNECTED
};

enum rrc_event {
    EV_BOOT = 0,
    EV_CELL_FOUND,
    EV_SETUP,
    EV_SETUP_COMPLETE,
    EV_LINK_LOSS
};

int rrc_handle(enum rrc_state *state, enum rrc_event ev)
{
    if (state == NULL) return -1;

    switch (*state) {
    case RRC_OFF:
        if (ev == EV_BOOT) {
            *state = RRC_SEARCHING;
            return 0;
        }
        break;

    case RRC_SEARCHING:
        if (ev == EV_CELL_FOUND) {
            *state = RRC_CAMPED;
            return 0;
        }
        break;

    case RRC_CAMPED:
        if (ev == EV_SETUP) {
            *state = RRC_CONNECTING;
            return 0;
        }
        break;

    case RRC_CONNECTING:
        if (ev == EV_SETUP_COMPLETE) {
            *state = RRC_CONNECTED;
            return 0;
        }
        break;

    case RRC_CONNECTED:
        if (ev == EV_LINK_LOSS) {
            *state = RRC_SEARCHING;
            return 0;
        }
        break;
    }

    return -2;
}
```

Production code nên tách transition table/guard/action khi machine lớn.

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_34_BEGIN -->
#### 9. Wizard Lab — State-machine scenario composer

**Ý nghĩ quái dị:** Nếu event sequence là data, mình có thể tự generate link flap, setup retry và invalid ordering rồi replay deterministic không?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
struct state_step { enum event event; enum state expected; };
int state_run_script(struct machine *m, const struct state_step *steps,
                     size_t n, size_t *failed_at);
```

1. **Bậc 1:** Happy path rồi LINK_LOSS/FOUND lặp ba lần.
2. **Bậc 2:** SETUP_COMPLETE trước SETUP.
3. **Bậc 3:** Duplicate EV_SETUP và xác nhận idempotence/reject policy.

**Observer bắt buộc:** From/event/to/reason log và state digest.

**Invariant:** Invalid event không mutate state; same script gives identical trace.

**Tự chế spell tiếp theo:** Generate mọi event pair cho mỗi state và xuất transition coverage table.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 34</strong></summary>

Lưu thành `practice/wizard_task_34.c`:

```c
#include <assert.h>
#include <stdio.h>

enum state { ST_IDLE, ST_SEARCHING, ST_CONNECTED };
enum event { EV_START, EV_SYNC, EV_RELEASE, EV_FAIL };
struct state_step { enum event event; enum state expected; };
struct machine { enum state state; };

static int machine_handle(struct machine *m, enum event e)
{
    if (m == NULL) return -1;
    if (m->state == ST_IDLE && e == EV_START) m->state = ST_SEARCHING;
    else if (m->state == ST_SEARCHING && e == EV_SYNC) m->state = ST_CONNECTED;
    else if (m->state == ST_SEARCHING && e == EV_FAIL) m->state = ST_IDLE;
    else if (m->state == ST_CONNECTED && e == EV_RELEASE) m->state = ST_IDLE;
    else return -1;
    return 0;
}

static int state_run_script(struct machine *m, const struct state_step *steps,
                            size_t n, size_t *failed_at)
{
    if (m == NULL || steps == NULL || failed_at == NULL) return -1;
    *failed_at = n;
    for (size_t k = 0; k < n; ++k) {
        if (machine_handle(m,steps[k].event) != 0 || m->state != steps[k].expected) {
            *failed_at = k; return -1;
        }
    }
    return 0;
}

int main(void)
{
    struct machine m = {ST_IDLE}; size_t failed;
    const struct state_step script[] = {{EV_START,ST_SEARCHING},{EV_SYNC,ST_CONNECTED},{EV_RELEASE,ST_IDLE}};
    assert(state_run_script(&m,script,3U,&failed) == 0);
    const struct state_step bad[] = {{EV_SYNC,ST_CONNECTED}};
    assert(state_run_script(&m,bad,1U,&failed) == -1 && failed == 0U);
    puts("wizard task 34 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_34.c -o wizard_task_34
./wizard_task_34
```

Expected cuối output:

```text
wizard task 34 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_34_END -->

---

#### Forge F42 — Bài 34: RRC table phải deterministic và inspectable

Tự code table validation duplicate `(from,event)`, dispatch wrong-state no-op và
state-name observer. Guard/action là function pointer từ static table, không từ input.

<details>
<summary>Đáp án F42</summary>

```c
#include <stddef.h>
enum rs{R_OFF,R_SEARCH,R_CAMP,R_CONNECTING,R_CONNECTED,R_COUNT};
struct rt{enum rs from;unsigned event;enum rs to;};
int rt_validate(const struct rt*t,size_t n)
{size_t i,j;if((t==NULL)&&(n!=0U))return-1;for(i=0U;i<n;++i){
 if(t[i].from>=R_COUNT||t[i].to>=R_COUNT||t[i].event==0U)return-1;
 for(j=0U;j<i;++j)if(t[j].from==t[i].from&&t[j].event==t[i].event)return-1;}return 0;}
int rt_dispatch(enum rs*state,unsigned ev,const struct rt*t,size_t n)
{size_t i;if((state==NULL)||(t==NULL)||*state<R_OFF||*state>=R_COUNT)return-1;
 for(i=0U;i<n;++i)if(t[i].from==*state&&t[i].event==ev){*state=t[i].to;return 0;}return 1;}
const char*rs_name(enum rs s)
{static const char*const names[]={"OFF","SEARCHING","CAMPED","CONNECTING","CONNECTED"};
 return s<R_COUNT?names[s]:"UNKNOWN";}
```

Biến thể thêm guard/action transactional: action fail không đổi state.

</details>

## Level 30 — Timer có generation

Timer callback cũ là bug kinh điển.

Ví dụ:

```text
Timer A gen=5 scheduled
state reset
Timer A gen=6 scheduled
callback gen=5 đến trễ
```

Nếu không check generation, callback cũ phá state mới.

### Bài 35 — Timer generation

#### 1. Đề bài nhỏ

Struct:

```c
struct timer {
    uint64_t deadline;
    uint16_t generation;
    bool armed;
};
```

Khi arm lại:

```text
generation++
```

Event timeout phải mang generation snapshot.

Handler chỉ chạy nếu:

```text
event_generation == current_generation
```

#### 2. Tự đoán chương trình cần làm gì

Mỗi lần arm tăng generation. Timeout cũ có deadline đúng nhưng generation sai vẫn phải bị bỏ qua.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- deadline
- generation snapshot
- stale timeout và wrap-safe comparison

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_35.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Arm gen1, arm lại gen2, phát timeout gen1 rồi gen2; chỉ gen2 được xử lý.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_35.c \
    -o build/task_35
./build/task_35
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

In deadline + generation của timer và event. Đừng chỉ so deadline vì event cũ có thể đến muộn.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** Timer lifetime / stale callback.

**Bạn vừa code cái gì trong modem?** Protocol thường arm timer rồi đổi state trước khi callback cũ chạy. Generation tag giúp callback biết mình còn thuộc 'đời' timer hiện tại hay đã stale.

**Nó nằm ở đâu?** runtime/control.

```text
INPUT  → timer id + generation + deadline
KHỐI   → code của Bài 35
OUTPUT → callback chỉ tác động context hợp lệ
```

**Tại sao modem cần nó?** Đây là cùng tư duy stale-handle ở buffer pool áp dụng cho thời gian. Không có generation, timeout cũ có thể phá state mới sau reconnect.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 35 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

##### Solution Bài 35 — timer generation

```c
struct timer {
    uint64_t deadline;
    uint16_t generation;
    bool armed;
};

uint16_t timer_arm(struct timer *t, uint64_t deadline)
{
    t->generation++;
    if (t->generation == 0U) {
        t->generation = 1U;
    }

    t->deadline = deadline;
    t->armed = true;
    return t->generation;
}

bool timer_fire(struct timer *t,
                uint64_t now,
                uint16_t event_generation)
{
    if ((t == NULL) || !t->armed) {
        return false;
    }

    if (event_generation != t->generation) {
        return false;
    }

    if (now < t->deadline) {
        return false;
    }

    t->armed = false;
    return true;
}
```

Lưu ý: compare `now < deadline` ở đây là toy code. Firmware có tick wrap cần modular comparison.

##### Solution Bài 35 bổ sung — Tick comparison có wrap

Với counter unsigned, có thể so deadline theo signed modular delta khi khoảng thời gian không vượt nửa miền:

```c
#include <stdbool.h>
#include <stdint.h>

static bool tick_reached32(uint32_t now, uint32_t deadline)
{
    return (int32_t)(now - deadline) >= 0;
}
```

Không dùng kiểu này nếu deadline có thể cách `now` hơn `2^31-1` ticks; contract phải giới hạn horizon.

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_35_BEGIN -->
#### 9. Wizard Lab — Timer time-warp laboratory

**Ý nghĩ quái dị:** Nếu arm/rearm, kéo tick tới trước/sau deadline và inject timeout generation cũ thì handler phản ứng thế nào?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
void fake_time_set(struct fake_time *t, uint64_t now);
int timer_emit_snapshot(const struct timer *timer, struct timeout_event *out);
int timer_handle(const struct timer *timer, const struct timeout_event *event);
```

1. **Bậc 1:** Snapshot gen1, rearm gen2, deliver gen1 late.
2. **Bậc 2:** Set time deadline-1, deadline, deadline+1.
3. **Bậc 3:** Đưa tick gần UINT64 wrap nếu contract hỗ trợ wrap-safe compare.

**Observer bắt buộc:** Now/deadline/generation/decision reason.

**Invariant:** Stale event không mutate context; due boundary được định nghĩa đúng một lần.

**Tự chế spell tiếp theo:** Tạo timer script và ranked list deadline sắp tới.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 35</strong></summary>

Lưu thành `practice/wizard_task_35.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

struct fake_time { uint64_t now; };
struct timer { uint64_t deadline; uint32_t generation; int armed; };
struct timeout_event { uint64_t deadline; uint32_t generation; };

static void fake_time_set(struct fake_time *t, uint64_t now)
{
    if (t != NULL) t->now = now;
}

static int timer_emit_snapshot(const struct timer *timer, struct timeout_event *out)
{
    if (timer == NULL || out == NULL || timer->armed == 0) return -1;
    *out = (struct timeout_event){timer->deadline,timer->generation}; return 0;
}

static int timer_handle(const struct timer *timer, const struct timeout_event *event)
{
    if (timer == NULL || event == NULL || timer->armed == 0) return -1;
    return timer->generation == event->generation && timer->deadline == event->deadline ? 0 : -1;
}

int main(void)
{
    struct fake_time time; struct timer timer = {100U,7U,1}; struct timeout_event e;
    fake_time_set(&time,100U); assert(time.now == 100U);
    assert(timer_emit_snapshot(&timer,&e) == 0 && timer_handle(&timer,&e) == 0);
    ++timer.generation;
    assert(timer_handle(&timer,&e) == -1);
    puts("wizard task 35 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_35.c -o wizard_task_35
./wizard_task_35
```

Expected cuối output:

```text
wizard task 35 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_35_END -->

---

#### Forge F43 — Bài 35: timer generation giết callback cũ

Tự code arm/cancel/fire; due compare modular; event mang generation snapshot.

<details>
<summary>Đáp án F43</summary>

```c
#include <stdint.h>
struct timer{uint64_t deadline;uint32_t generation;int active;};
static uint32_t tg(uint32_t x){++x;return x==0U?1U:x;}
uint32_t timer_arm(struct timer*t,uint64_t deadline)
{if(t==NULL)return 0U;t->generation=tg(t->generation);t->deadline=deadline;
 t->active=1;return t->generation;}
int timer_cancel(struct timer*t)
{if(t==NULL)return-1;t->active=0;t->generation=tg(t->generation);return 0;}
int timer_fire(struct timer*t,uint64_t now,uint32_t event_generation)
{if((t==NULL)||!t->active||t->generation!=event_generation)return 0;
 if(((now-t->deadline)&(UINT64_C(1)<<63U))!=0U)return 0;
 t->active=0;return 1;}
```

Test: arm A, arm B, callback A đến trễ phải no-op; callback B trước/equal/after
deadline; deadline quanh `UINT64_MAX` trong horizon nhỏ hơn `2^63`.

</details>

# PHẦN XIII — SECURITY CONTEXT Ở MỨC KIẾN TRÚC

## Level 31 — Key handle, không kéo raw key khắp firmware

Một design tốt có thể để firmware giữ:

```text
key_slot = 1
```

thay vì raw AES key ở mọi layer.

Firmware submit crypto job:

```text
operation
key_slot
COUNT
BEARER
DIRECTION
input buffer
output buffer
```

Crypto engine hoàn tất rồi IRQ báo completion.

Đây là architectural separation.

### Bài 36 — Fake crypto engine

#### 1. Đề bài nhỏ

Không cần AES ở bài đầu.

Fake engine nhận descriptor và tính checksum đơn giản chỉ để luyện async flow:

```text
submit job
  ↓
BUSY
  ↓ simulator step
DONE + IRQ
  ↓
firmware completion handler
```

Sau khi flow đúng mới thay primitive test bằng implementation phù hợp project.

#### 2. Tự đoán chương trình cần làm gì

Submit chỉ tạo job BUSY; simulator step hoàn tất job và phát completion; handler mới tiêu thụ kết quả. Không biến async thành lời gọi đồng bộ giả.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- job descriptor/state
- submit/step/completion
- key handle thay raw key

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_36.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Submit khi IDLE thành công, submit khi BUSY fail, step tạo DONE/IRQ, completion đưa engine về IDLE.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_36.c \
    -o build/task_36
./build/task_36
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

Log `IDLE→BUSY→DONE→IDLE`. Nếu completion xuất hiện ngay trong submit, bạn đã làm mất semantics async.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** Hardware-like crypto offload.

**Bạn vừa code cái gì trong modem?** Modem firmware thường không muốn expose raw key khắp stack; nó submit job dùng key-slot handle rồi nhận completion.

**Nó nằm ở đâu?** security/SoC.

```text
INPUT  → descriptor + key slot + COUNT/bearer/direction
KHỐI   → code của Bài 36
OUTPUT → completion + tag/result
```

**Tại sao modem cần nó?** Fake engine dạy boundary đúng: protocol yêu cầu integrity, hardware-like accelerator thực hiện primitive. Key material và control flow được tách.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 36 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

Mục tiêu là luyện submit→busy→step→done→IRQ, không phải mô phỏng AES.

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct crypto_job {
    const uint8_t *src;
    size_t len;
    uint16_t key_slot;
    uint32_t count;
};

struct fake_crypto {
    bool busy;
    bool done;
    bool irq_pending;
    struct crypto_job job;
    uint32_t tag;
};

int crypto_submit(struct fake_crypto *c,
                  const struct crypto_job *job)
{
    if ((c == NULL) || (job == NULL) ||
        ((job->len != 0U) && (job->src == NULL))) {
        return -1;
    }

    if (c->busy) {
        return -2;
    }

    c->job = *job;
    c->busy = true;
    c->done = false;
    c->irq_pending = false;
    return 0;
}

void crypto_step(struct fake_crypto *c)
{
    uint32_t acc;

    if ((c == NULL) || !c->busy) {
        return;
    }

    acc = 0x9E3779B9U ^ c->job.count ^ c->job.key_slot;

    for (size_t i = 0U; i < c->job.len; ++i) {
        acc = (acc << 5U) | (acc >> 27U);
        acc ^= c->job.src[i];
    }

    c->tag = acc;
    c->busy = false;
    c->done = true;
    c->irq_pending = true;
}

bool crypto_irq_take(struct fake_crypto *c, uint32_t *tag)
{
    if ((c == NULL) || (tag == NULL) || !c->irq_pending) {
        return false;
    }

    *tag = c->tag;
    c->irq_pending = false;
    return true;
}
```

Firmware side không cần biết fake checksum được tính thế nào. Nó chỉ biết contract accelerator.

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_36_BEGIN -->
#### 9. Wizard Lab — Async-job disturbance lab

**Ý nghĩ quái dị:** Nếu completion bị delay/duplicate hoặc job mới submit khi BUSY, async engine và key handle có giữ contract không?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
int crypto_engine_delay_completion(struct fake_crypto *e, uint64_t ticks);
int crypto_engine_duplicate_irq(struct fake_crypto *e);
void crypto_engine_dump(const struct fake_crypto *e);
```

1. **Bậc 1:** Submit hai job liên tiếp khi engine một slot.
2. **Bậc 2:** Delay completion qua timeout.
3. **Bậc 3:** Duplicate IRQ sau DONE và kiểm completion chỉ commit một lần.

**Observer bắt buộc:** IDLE/BUSY/DONE trace, job id, key handle và completion count.

**Invariant:** Không expose raw key; mỗi accepted job có tối đa một completion commit.

**Tự chế spell tiếp theo:** Tạo job queue bounded trước engine và pressure-test.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 36</strong></summary>

Lưu thành `practice/wizard_task_36.c`:

```c
#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

struct fake_crypto { uint64_t completion_tick; size_t pending_irqs; int busy; };

static int crypto_engine_delay_completion(struct fake_crypto *e, uint64_t ticks)
{
    if (e == NULL || e->busy == 0 || UINT64_MAX - e->completion_tick < ticks) return -1;
    e->completion_tick += ticks; return 0;
}

static int crypto_engine_duplicate_irq(struct fake_crypto *e)
{
    if (e == NULL || e->pending_irqs == 0U) return -1;
    ++e->pending_irqs; return 0;
}

static void crypto_engine_dump(const struct fake_crypto *e)
{
    if (e != NULL) printf("busy=%d completion=%" PRIu64 " irqs=%zu\n",e->busy,e->completion_tick,e->pending_irqs);
}

int main(void)
{
    struct fake_crypto e = {50U,1U,1};
    assert(crypto_engine_delay_completion(&e,25U) == 0 && e.completion_tick == 75U);
    assert(crypto_engine_duplicate_irq(&e) == 0 && e.pending_irqs == 2U);
    crypto_engine_dump(&e);
    puts("wizard task 36 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_36.c -o wizard_task_36
./wizard_task_36
```

Expected cuối output:

```text
wizard task 36 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_36_END -->

---

#### Forge F44 — Bài 36: crypto engine là async job, không phải call tắt

Tự code submit descriptor dùng key-slot handle, mark device done và complete qua
cookie/generation. Raw key không được nằm trong job.

<details>
<summary>Đáp án F44</summary>

```c
#include <stddef.h>
#include <stdint.h>
enum js{J_FREE,J_CPU,J_DEVICE,J_DONE};
struct key_handle{uint16_t slot,generation;};
struct job{enum js state;struct key_handle key;uint16_t cookie,generation;
 uint32_t count;const uint8_t*msg;size_t n;uint8_t tag[4];};
int crypto_submit(struct job*j,struct key_handle key,uint16_t cookie,
                  uint32_t count,const uint8_t*msg,size_t n)
{if((j==NULL)||(j->state!=J_FREE)||(key.generation==0U)||
 ((msg==NULL)&&(n!=0U))||n>65535U)return-1;
 j->key=key;j->cookie=cookie;++j->generation;if(j->generation==0U)j->generation=1U;
 j->count=count;j->msg=msg;j->n=n;j->state=J_DEVICE;return 0;}
int crypto_device_done(struct job*j,const uint8_t tag[4])
{size_t k;if((j==NULL)||(tag==NULL)||(j->state!=J_DEVICE))return-1;
 for(k=0U;k<4U;++k)j->tag[k]=tag[k];j->state=J_DONE;return 0;}
int crypto_complete(struct job*j,uint16_t cookie,uint16_t generation,uint8_t out[4])
{size_t k;if((j==NULL)||(out==NULL)||(j->state!=J_DONE)||j->cookie!=cookie||
 j->generation!=generation)return-1;for(k=0U;k<4U;++k)out[k]=j->tag[k];j->state=J_FREE;return 0;}
```

Project 100/100 còn cần descriptor CRC, physical aperture, cache/fence, OWN,
doorbell, MMIO và IRQ13; card này luyện identity/lifetime trước khi nối hardware.

</details>

# PHẦN XIV — HOST SIMULATOR VÀ VIRTUAL RF

## Level 32 — Tại sao cùng firmware source build hai nơi?

Mục tiêu architecture:

```text
               shared C firmware core
                  /           \
                 /             \
        host simulator       Arm target
          x86-64             Cortex-R5
```

Hardware dependency đi qua ops:

```c
struct platform_ops {
    uint64_t (*ticks)(void *ctx);
    uint32_t (*mmio_read32)(void *ctx, uintptr_t addr);
    void (*mmio_write32)(void *ctx, uintptr_t addr, uint32_t value);
    ...
};
```

Đây là dependency injection theo phong cách C.

### Bài 37 — Platform clock

#### 1. Đề bài nhỏ

Viết fake clock:

```c
struct fake_platform {
    uint64_t now;
};
```

callback:

```c
uint64_t fake_ticks(void *ctx);
```

Test:

```text
now=100 -> returns 100
now=250 -> returns 250
```

Sau đó dùng callback trong scheduler thay vì gọi `clock_gettime()` trực tiếp từ firmware.

#### 2. Tự đoán chương trình cần làm gì

Firmware gọi clock qua function pointer và `ctx`; thay field `now` của fake platform thì callback trả đúng giá trị mới.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- function pointer
- `void *ctx`
- dependency injection và fake time

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_37.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

`now=100` trả 100, đổi thành 250 trả 250; hai fake context độc lập không lẫn state.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_37.c \
    -o build/task_37
./build/task_37
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

Nếu callback crash, kiểm tra `ctx` có đúng type/lifetime và function pointer đã được gán trước khi gọi.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** Virtual time.

**Bạn vừa code cái gì trong modem?** Test modem cần clock deterministic để cùng input luôn tạo cùng event order và trace.

**Nó nằm ở đâu?** platform/runtime.

```text
INPUT  → platform tick source
KHỐI   → code của Bài 37
OUTPUT → monotonic virtual time
```

**Tại sao modem cần nó?** Dùng wall-clock thật làm test dễ flaky. Virtual clock còn cho phép inject timer late, deadline miss và replay chính xác.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 37 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

```c
struct fake_platform {
    uint64_t now;
};

uint64_t fake_ticks(void *ctx)
{
    struct fake_platform *p = ctx;

    if (p == NULL) {
        return 0U;
    }

    return p->now;
}
```

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_37_BEGIN -->
#### 9. Wizard Lab — Platform personality swapping

**Ý nghĩ quái dị:** Cùng firmware core có thể đổi clock/MMIO/cache personality bằng ops table trong runtime không?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
struct platform_fixture { struct bb_platform_ops ops; void *ctx; };
struct platform_fixture make_fast_clock(void);
struct platform_fixture make_jitter_script_clock(const uint64_t *ticks, size_t n);
```

1. **Bậc 1:** Chạy scheduler với clock tăng 1 và tăng 10.
2. **Bậc 2:** Script clock lặp cùng tick để test stable order.
3. **Bậc 3:** Thay cache hook bằng logger và kiểm call order DMA.

**Observer bắt buộc:** Ops-call trace và final modem digest.

**Invariant:** Core không gọi host API trực tiếp; đổi fixture không đổi contract public.

**Tự chế spell tiếp theo:** Viết platform decorator đếm mọi callback mà không sửa implementation gốc.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 37</strong></summary>

Lưu thành `practice/wizard_task_37.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct bb_platform_ops { uint64_t (*ticks)(void *ctx); };
struct platform_fixture { struct bb_platform_ops ops; void *ctx; };
struct script_clock { const uint64_t *ticks; size_t n, at; };

static uint64_t fast_ticks(void *ctx) { (void)ctx; return UINT64_C(1000); }
static uint64_t script_ticks(void *ctx)
{
    struct script_clock *c = ctx;
    if (c == NULL || c->n == 0U) return 0U;
    const size_t at = c->at < c->n ? c->at++ : c->n - 1U;
    return c->ticks[at];
}

static struct platform_fixture make_fast_clock(void)
{
    return (struct platform_fixture){{fast_ticks},NULL};
}

static struct platform_fixture make_jitter_script_clock(const uint64_t *ticks, size_t n)
{
    struct script_clock *c = malloc(sizeof *c);
    if (c == NULL || ticks == NULL || n == 0U) { free(c); return (struct platform_fixture){0}; }
    *c = (struct script_clock){ticks,n,0U};
    return (struct platform_fixture){{script_ticks},c};
}

static void platform_fixture_destroy(struct platform_fixture *f)
{
    if (f != NULL && f->ops.ticks == script_ticks) free(f->ctx);
    if (f != NULL) *f = (struct platform_fixture){0};
}

int main(void)
{
    const uint64_t script[3] = {10U,12U,11U};
    struct platform_fixture fast = make_fast_clock();
    struct platform_fixture jitter = make_jitter_script_clock(script,3U);
    assert(fast.ops.ticks(fast.ctx) == 1000U);
    assert(jitter.ops.ticks != NULL);
    assert(jitter.ops.ticks(jitter.ctx) == 10U && jitter.ops.ticks(jitter.ctx) == 12U);
    platform_fixture_destroy(&jitter);
    puts("wizard task 37 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_37.c -o wizard_task_37
./wizard_task_37
```

Expected cuối output:

```text
wizard task 37 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_37_END -->

---

## Level 33 — Virtual time

Deterministic simulator không chạy theo wall clock.

Thay vì:

```text
sleep(5 ms)
```

runner làm:

```text
tick = 0
step modem
step soc
step peer

tick = 500
...
```

Lợi ích:

- test nhanh;
- reproducible;
- timeout deterministic;
- dễ replay bug.

#### Forge F45 — Bài 37: platform clock phải chứng minh được hai personality

Tự code:

1. `clock_read` qua ops + context, reject callback NULL.
2. `monotonic_read` reject thời gian đi lùi nhưng cho phép bằng nhau.
3. Offset clock bọc một clock khác, không dùng global.

<details>
<summary>Đáp án F45</summary>

```c
#include <stdint.h>
struct clock_ops{uint64_t(*ticks)(void*);};
struct monotonic{const struct clock_ops*ops;void*ctx;uint64_t last;int initialized;};
struct offset_clock{const struct clock_ops*inner;void*inner_ctx;uint64_t offset;};
int clock_read(const struct clock_ops*ops,void*ctx,uint64_t*out)
{if((ops==NULL)||(ops->ticks==NULL)||(out==NULL))return-1;*out=ops->ticks(ctx);return 0;}
int monotonic_read(struct monotonic*m,uint64_t*out)
{uint64_t now;if((m==NULL)||(out==NULL)||clock_read(m->ops,m->ctx,&now)!=0)return-1;
 if(m->initialized&&now!=m->last&&
    (((now-m->last)&(UINT64_C(1)<<63U))!=0U))return-1;
 m->initialized=1;m->last=now;*out=now;return 0;}
uint64_t offset_ticks(void*ctx)
{struct offset_clock*c=ctx;return c->inner->ticks(c->inner_ctx)+c->offset;}
```

Test hai fake clock cùng code callback nhưng context `now=100` và `now=900`;
đổi một context không được ảnh hưởng clock kia.

</details>

### Bài 38 — Scheduler theo tick

#### 1. Đề bài nhỏ

Cho events:

```text
100 A
50  B
100 C
20  D
```

Yêu cầu stable ordering:

```text
20 D
50 B
100 A
100 C
```

Hai event cùng tick giữ thứ tự file.

#### 2. Tự đoán chương trình cần làm gì

Scheduler sắp theo tick tăng dần; nếu tick bằng nhau thì giữ thứ tự input. Cùng input luôn cho cùng output.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- tick + insertion order
- stable ordering
- deterministic scheduler

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_38.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Input `100A,50B,100C,20D` phải ra `20D,50B,100A,100C`; chạy lại byte-identical.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_38.c \
    -o build/task_38
./build/task_38
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

In `{tick,original_order}`; comparator không được trả tie tùy ý khi tick bằng nhau.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** Real-time scheduler.

**Bạn vừa code cái gì trong modem?** Modem có nhiều task nhưng L1 RX/TX deadline gắt hơn trace hay idle. Scheduler theo tick/priority quyết định work nào chạy trước.

**Nó nằm ở đâu?** runtime.

```text
INPUT  → ready work + virtual time + priority
KHỐI   → code của Bài 38
OUTPUT → bounded execution order
```

**Tại sao modem cần nó?** Đây là lúc thấy 'correct algorithm' chưa đủ. Nếu MAC/trace chiếm CPU khiến L1_RX trễ slot, radio pipeline vẫn fail.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 38 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

Dataset nhỏ có thể dùng insertion sort, vốn stable nếu chỉ dịch khi `>` chứ không dịch khi `==`.

```c
#include <stddef.h>
#include <stdint.h>

struct sched_event {
    uint64_t tick;
    char tag;
};

void stable_sort_events(struct sched_event *a, size_t n)
{
    for (size_t i = 1U; i < n; ++i) {
        struct sched_event key = a[i];
        size_t j = i;

        while ((j > 0U) && (a[j - 1U].tick > key.tick)) {
            a[j] = a[j - 1U];
            --j;
        }

        a[j] = key;
    }
}
```

Input:

```text
100 A
50  B
100 C
20  D
```

Output:

```text
20 D
50 B
100 A
100 C
```

`A` vẫn đứng trước `C` vì sort stable.

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_38_BEGIN -->
#### 9. Wizard Lab — Scheduler mutation engine

**Ý nghĩ quái dị:** Nếu timeline là data, mình có thể delay/drop/duplicate/relabel event rồi vẫn giữ deterministic replay không?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
int timeline_delay_type(struct timeline *t, enum event_type type, uint64_t delta);
size_t timeline_drop_if(struct timeline *t, event_predicate_fn fn, void *ctx);
int timeline_duplicate_first(struct timeline *t, enum event_type type);
```

1. **Bậc 1:** Delay RX 75 tick, duplicate timer, drop trace.
2. **Bậc 2:** Sort stable và so serial khi tick bằng nhau.
3. **Bậc 3:** Run mutation hai lần trên clone baseline và `cmp` trace.

**Observer bắt buộc:** Timeline dump, digest, first order difference.

**Invariant:** Baseline immutable; same mutation+seed gives byte-identical trace.

**Tự chế spell tiếp theo:** Tạo callback mutator list và experiment runner như grid lab.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 38</strong></summary>

Lưu thành `practice/wizard_task_38.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define TL_CAP 8U
enum event_type { EV_RX, EV_TIMER, EV_TASK };
struct event { uint64_t tick; enum event_type type; uint32_t value; };
struct timeline { struct event e[TL_CAP]; size_t count; };
typedef int (*event_predicate_fn)(const struct event *e, void *ctx);

static int timeline_delay_type(struct timeline *t, enum event_type type, uint64_t delta)
{
    if (t == NULL) return -1;
    for (size_t k = 0; k < t->count; ++k) if (t->e[k].type == type) {
        if (UINT64_MAX - t->e[k].tick < delta) return -1;
        t->e[k].tick += delta;
    }
    return 0;
}

static size_t timeline_drop_if(struct timeline *t, event_predicate_fn fn, void *ctx)
{
    size_t write = 0U, dropped = 0U;
    if (t == NULL || fn == NULL) return 0U;
    for (size_t read = 0; read < t->count; ++read)
        if (fn(&t->e[read],ctx) != 0) ++dropped; else t->e[write++] = t->e[read];
    t->count = write; return dropped;
}

static int timeline_duplicate_first(struct timeline *t, enum event_type type)
{
    if (t == NULL || t->count == TL_CAP) return -1;
    for (size_t k = 0; k < t->count; ++k) if (t->e[k].type == type) {
        t->e[t->count++] = t->e[k]; return 0;
    }
    return -1;
}

static int value_is(const struct event *e, void *ctx)
{
    const uint32_t *value = ctx; return e != NULL && value != NULL && e->value == *value;
}

int main(void)
{
    struct timeline t = {.e={{10U,EV_RX,1U},{20U,EV_TIMER,2U},{30U,EV_TASK,3U}},.count=3U};
    const uint32_t drop = 2U;
    assert(timeline_delay_type(&t,EV_RX,5U) == 0 && t.e[0].tick == 15U);
    assert(timeline_duplicate_first(&t,EV_RX) == 0 && t.count == 4U);
    assert(timeline_drop_if(&t,value_is,(void *)&drop) == 1U && t.count == 3U);
    puts("wizard task 38 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_38.c -o wizard_task_38
./wizard_task_38
```

Expected cuối output:

```text
wizard task 38 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_38_END -->

---

# PHẦN XV — FILE FORMAT VÀ DEFENSIVE PARSING

## Level 34 — IQ16 container

Một format IQ học tập:

```text
magic[4] = "IQ16"
sample_rate : u32 little-endian
count       : u64 little-endian
samples     : count × { int16 I, int16 Q }
```

Parser phải check trước:

```text
magic đúng?
sample_rate != 0?
count có overflow khi nhân sizeof(sample)?
file đủ bytes?
trailing policy?
```

### Integer overflow check

Không làm mù:

```c
bytes = count * 4;
```

Nếu count attacker-controlled, multiplication có thể overflow.

Pattern:

```c
if (count > SIZE_MAX / 4U) reject;
bytes = count * 4U;
```

#### Forge F46 — Bài 38: scheduler phải stable, bounded và virtual-time only

Tự code stable insert cùng deadline, pop-due và `run_until` có work budget. Khi
budget cạn, event chưa chạy phải còn nguyên trong queue.

<details>
<summary>Đáp án F46</summary>

```c
#include <stddef.h>
#include <stdint.h>
#define ECAP 16U
struct se{uint64_t tick;uint32_t order;uint16_t type;};
struct sched{struct se e[ECAP];size_t n;uint32_t next_order;};
int sched_post(struct sched*s,uint64_t tick,uint16_t type)
{size_t k;struct se x;if((s==NULL)||(type==0U)||(s->n==ECAP))return-1;
 x=(struct se){tick,s->next_order++,type};k=s->n;
 while(k>0U&&s->e[k-1U].tick>tick){s->e[k]=s->e[k-1U];--k;}
 s->e[k]=x;++s->n;return 0;}
int sched_pop_due(struct sched*s,uint64_t now,struct se*out)
{size_t k;if((s==NULL)||(out==NULL)||(s->n==0U)||
 (((now-s->e[0].tick)&(UINT64_C(1)<<63U))!=0U))return 0;
 *out=s->e[0];for(k=1U;k<s->n;++k)s->e[k-1U]=s->e[k];--s->n;return 1;}
typedef int(*dispatch_fn)(void*,const struct se*);
int sched_run(struct sched*s,uint64_t until,size_t budget,dispatch_fn fn,void*ctx,size_t*ran)
{struct se e;size_t n=0U;int rc;if((s==NULL)||(fn==NULL)||(ran==NULL))return-1;
 while(n<budget&&(rc=sched_pop_due(s,until,&e))==1){if(fn(ctx,&e)!=0)return-1;++n;}
 *ran=n;return 0;}
```

`order` giải thích source order nhưng stable insertion đã giữ thứ tự bằng điều
kiện `>` thay vì `>=`. Thêm test cùng tick 100 cho sequence A,B,C.

</details>

### Bài 39 — IQ16 reader host

#### 1. Đề bài nhỏ

Viết reader chỉ host-side.

Test files:

1. valid 4 samples;
2. bad magic;
3. truncated header;
4. count quá lớn;
5. truncated sample payload.

#### 2. Tự đoán chương trình cần làm gì

Reader validate header/count/size trước khi đọc sample. File hỏng phải fail sạch, không allocation/copy vượt giới hạn.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- binary file I/O
- manual endian decode
- size overflow/truncation checks

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_39.c`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Dùng đủ năm file test trong đề; chạy ASan/UBSan; file lỗi phải trả status rõ và giải phóng tài nguyên host.

```bash
mkdir -p build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined practice/task_39.c \
    -o build/task_39
./build/task_39
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

Tách lỗi thành open/header/count/payload. Với count lớn, kiểm tra multiplication overflow trước tính expected bytes.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** IQ file as virtual ADC/DAC.

**Bạn vừa code cái gì trong modem?** `IQ16` reader biến file thành cùng representation mà RF backend sẽ cấp: metadata + complex samples.

**Nó nằm ở đâu?** host↔RF simulation.

```text
INPUT  → serialized IQ16 file
KHỐI   → code của Bài 39
OUTPUT → validated IQ block
```

**Tại sao modem cần nó?** Header truncation/count overflow phải bị reject. File này chính là 'antenna giả' của lab; user/control data không được đi đường tắt ngoài waveform.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 39 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

API này dùng buffer do caller cấp để tránh allocation không bounded.

```c
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct cpx16 {
    int16_t i;
    int16_t q;
};

static uint32_t u32le(const uint8_t b[4])
{
    return (uint32_t)b[0] |
           ((uint32_t)b[1] << 8U) |
           ((uint32_t)b[2] << 16U) |
           ((uint32_t)b[3] << 24U);
}

static uint64_t u64le(const uint8_t b[8])
{
    uint64_t v = 0U;

    for (unsigned i = 0U; i < 8U; ++i) {
        v |= (uint64_t)b[i] << (8U * i);
    }

    return v;
}

static int16_t s16le(const uint8_t b[2])
{
    uint16_t u = (uint16_t)b[0] |
                 (uint16_t)((uint16_t)b[1] << 8U);
    return (int16_t)u;
}

int read_iq16(const char *path,
              struct cpx16 *samples,
              size_t capacity,
              uint32_t *sample_rate,
              size_t *count)
{
    uint8_t header[16];
    FILE *f;
    uint64_t count64;

    if ((path == NULL) || (samples == NULL) ||
        (sample_rate == NULL) || (count == NULL)) {
        return -1;
    }

    f = fopen(path, "rb");
    if (f == NULL) {
        return -2;
    }

    if (fread(header, 1U, sizeof(header), f) != sizeof(header)) {
        fclose(f);
        return -3;
    }

    if (memcmp(header, "IQ16", 4U) != 0) {
        fclose(f);
        return -4;
    }

    *sample_rate = u32le(&header[4]);
    count64 = u64le(&header[8]);

    if (*sample_rate == 0U) {
        fclose(f);
        return -5;
    }

    if (count64 > (uint64_t)SIZE_MAX) {
        fclose(f);
        return -6;
    }

    if ((size_t)count64 > capacity) {
        fclose(f);
        return -7;
    }

    for (size_t i = 0U; i < (size_t)count64; ++i) {
        uint8_t raw[4];

        if (fread(raw, 1U, sizeof(raw), f) != sizeof(raw)) {
            fclose(f);
            return -8;
        }

        samples[i].i = s16le(&raw[0]);
        samples[i].q = s16le(&raw[2]);
    }

    if (fgetc(f) != EOF) {
        fclose(f);
        return -9; /* policy: reject trailing data */
    }

    fclose(f);
    *count = (size_t)count64;
    return 0;
}
```

Các testcase yêu cầu của bài map trực tiếp vào return code khác nhau. Quan trọng hơn code cụ thể là parser **không bao giờ tin `count` trước khi kiểm capacity/truncation**.

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_39_BEGIN -->
#### 9. Wizard Lab — IQ-file workbench an toàn

**Ý nghĩ quái dị:** Nếu muốn tự tạo/patch/truncate IQ16 local để khám phá parser và waveform mà không sửa binary bằng tay thì helper nào cần?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
int iq16_write(const char *path, const struct iq_block *block);
int iq16_patch_sample(const char *in, const char *out,
                      size_t index, struct cpx16 value);
int iq16_truncate_copy(const char *in, const char *out, size_t bytes);
```

1. **Bậc 1:** Generate 4-sample valid file từ code.
2. **Bậc 2:** Patch sample 2 rồi diff hexdump và parsed block.
3. **Bậc 3:** Tạo bản truncate ở mỗi boundary header/payload.

**Observer bắt buộc:** File size, parser status, sample diff và deterministic hash.

**Invariant:** Không sửa file input; mọi output path riêng; parser không trust count.

**Tự chế spell tiếp theo:** Tạo local recipe file mô tả pattern IQ rồi generator C đọc recipe đơn giản.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 39</strong></summary>

Lưu thành `practice/wizard_task_39.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct cpx16 { int16_t i; int16_t q; };
struct iq_block { const struct cpx16 *sample; size_t count; };

static int iq16_write(const char *path, const struct iq_block *block)
{
    if (path == NULL || block == NULL || (block->sample == NULL && block->count != 0U)) return -1;
    FILE *f = fopen(path,"wb"); if (f == NULL) return -1;
    const size_t wrote = fwrite(block->sample,sizeof *block->sample,block->count,f);
    const int close_rc = fclose(f);
    return wrote == block->count && close_rc == 0 ? 0 : -1;
}

static int iq16_patch_sample(const char *in, const char *out,
                             size_t index, struct cpx16 value)
{
    FILE *src = fopen(in,"rb"), *dst = fopen(out,"wb");
    if (src == NULL || dst == NULL) { if (src) fclose(src); if (dst) fclose(dst); return -1; }
    struct cpx16 x; size_t at = 0U; int found = 0;
    while (fread(&x,sizeof x,1U,src) == 1U) {
        if (at++ == index) { x = value; found = 1; }
        if (fwrite(&x,sizeof x,1U,dst) != 1U) { found = 0; break; }
    }
    const int read_ok = ferror(src) == 0;
    const int close_src = fclose(src);
    const int close_dst = fclose(dst);
    const int ok = found && read_ok && close_src == 0 && close_dst == 0;
    return ok ? 0 : -1;
}

static int iq16_truncate_copy(const char *in, const char *out, size_t bytes)
{
    FILE *src = fopen(in,"rb"), *dst = fopen(out,"wb");
    if (src == NULL || dst == NULL) { if (src) fclose(src); if (dst) fclose(dst); return -1; }
    uint8_t buf[32]; size_t left = bytes;
    while (left != 0U) {
        const size_t want = left < sizeof buf ? left : sizeof buf;
        const size_t got = fread(buf,1U,want,src);
        if (got == 0U || fwrite(buf,1U,got,dst) != got) break;
        left -= got;
    }
    const int close_src = fclose(src);
    const int close_dst = fclose(dst);
    const int ok = left == 0U && close_src == 0 && close_dst == 0;
    return ok ? 0 : -1;
}

int main(void)
{
    const struct cpx16 sample[3] = {{1,2},{3,4},{5,6}};
    const struct iq_block block = {sample,3U}; struct cpx16 readback[3];
    assert(iq16_write("wizard_iq_in.bin",&block) == 0);
    assert(iq16_patch_sample("wizard_iq_in.bin","wizard_iq_patch.bin",1U,(struct cpx16){99,88}) == 0);
    FILE *f = fopen("wizard_iq_patch.bin","rb"); assert(f != NULL);
    assert(fread(readback,sizeof readback,1U,f) == 1U && fclose(f) == 0 && readback[1].i == 99);
    assert(iq16_truncate_copy("wizard_iq_in.bin","wizard_iq_short.bin",sizeof(struct cpx16)) == 0);
    remove("wizard_iq_in.bin"); remove("wizard_iq_patch.bin"); remove("wizard_iq_short.bin");
    puts("wizard task 39 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_39.c -o wizard_task_39
./wizard_task_39
```

Expected cuối output:

```text
wizard task 39 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_39_END -->

---

#### Forge F47 — Bài 39: IQ16 parser phải reject trước khi cấp/đọc sample

Tự code parser memory-view, exact-size validator và sample accessor. Biến thể
phải bắt bad magic, rate zero, count >8192, multiplication overflow, truncated
và trailing bytes.

<details>
<summary>Đáp án F47</summary>

```c
#include <stddef.h>
#include <stdint.h>
#include <string.h>
struct iq_view{uint32_t rate;uint64_t count;const uint8_t*samples;};
static uint32_t g32(const uint8_t*p)
{return(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static uint64_t g64(const uint8_t*p)
{size_t k;uint64_t v=0U;for(k=0U;k<8U;++k)v|=(uint64_t)p[k]<<(8U*k);return v;}
int iq16_view(const uint8_t*p,size_t n,struct iq_view*out)
{struct iq_view v;size_t bytes;if((p==NULL)||(out==NULL)||(n<16U)||memcmp(p,"IQ16",4U)!=0)return-1;
 v.rate=g32(p+4U);v.count=g64(p+8U);if(v.rate==0U||v.count>8192U||v.count>SIZE_MAX/4U)return-1;
 bytes=(size_t)v.count*4U;if(bytes!=n-16U)return-1;v.samples=p+16U;*out=v;return 0;}
int iq16_sample(const struct iq_view*v,size_t index,int16_t*i,int16_t*q)
{const uint8_t*p;if((v==NULL)||(i==NULL)||(q==NULL)||index>=v->count)return-1;p=v->samples+4U*index;
 *i=(int16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8));
 *q=(int16_t)((uint16_t)p[2]|((uint16_t)p[3]<<8));return 0;}
```

File reader thật đọc header trước, validate count/size rồi mới đọc body; không
tin file metadata để allocation không bounded.

</details>

# PHẦN XVI — ARMv7-R STARTUP: TỪ RESET ĐẾN `main`/FIRMWARE ENTRY

## Level 35 — Tại sao firmware cần startup assembly?

Trên Linux program bình thường, loader/runtime làm rất nhiều thứ trước `main`.

Bare metal thì bạn phải chuẩn bị:

```text
reset vector
 ↓
stack pointer
 ↓
copy .data ROM → RAM
 ↓
zero .bss
 ↓
optional MPU/FPU/etc
 ↓
call C firmware entry
```

### Linker script concept

Linker script nói section nằm ở đâu:

```text
.text   executable code
.rodata constants
.data   initialized mutable data
.bss    zero-initialized data
.stack  stack region
```

### Bài 40 — Tự viết C rồi đọc linker map

#### 1. Đề bài nhỏ

Viết `practice/task_40.c` là một firmware image cực nhỏ có:

- `reset_handler()` trong `.text`;
- `boot_counter` zero-init để xuất hiện trong `.bss`;
- `boot_attempts = 1` để xuất hiện trong `.data`;
- `boot_magic` read-only để quan sát `.rodata`;
- một vector table nhỏ đặt trong section `.vectors`.

Sau đó build ELF freestanding và dùng:

```bash
arm-none-eabi-size -A firmware.elf
arm-none-eabi-nm firmware.elf
arm-none-eabi-objdump -h firmware.elf
```

Trả lời:

- `.text` size bao nhiêu?
- `.bss` có chiếm bytes trong ELF file giống `.data` không?
- reset symbol ở address nào?

Chưa cần tự viết linker script. Mục tiêu là **tự tạo object C trước**, rồi nhìn
thấy từng object rơi vào section nào.

#### 2. Tự đoán chương trình cần làm gì

Source C tạo function/global object; compiler và linker biến chúng thành các
section/symbol trong ELF. Output của bài không phải dòng text từ chương trình
đang chạy mà là `task40.elf`, `task40.map` và báo cáo từ các tool.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- ELF sections/symbols
- linker map
- `size`, `nm`, `objdump`
- global zero-init, initialized data và read-only data
- `-ffreestanding`, `-nostdlib` và entry symbol

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task_40.c`. Chưa mở Bước 7.

1. Include `<stdint.h>`.
2. Tạo ba global object đại diện `.bss`, `.data`, `.rodata`.
3. Viết helper `add_one()` và `reset_handler()`.
4. Trong `reset_handler`, cập nhật hai biến rồi đi vào vòng lặp vô hạn.
5. Tạo một mảng function pointer chứa `reset_handler` và đặt nó vào `.vectors`.
6. Build ELF trước; chỉ khi link thành công mới chạy `size`, `nm`, `objdump`.

Attribute đặt section là glue phụ thuộc toolchain, được chấp nhận trong riêng
bài startup/linker; firmware core còn lại vẫn giữ C17 portable.

#### 5. Chạy test / quan sát output

Lưu output ba tool vào note; tự trả lời địa chỉ reset, size `.text/.data/.bss` và kiểm tra câu trả lời bằng map file.

```bash
mkdir -p practice build
arm-none-eabi-gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -ffreestanding -fno-builtin -nostdlib \
    -Wl,-e,reset_handler -Wl,-Map=build/task40.map \
    practice/task_40.c -o build/task40.elf
arm-none-eabi-size -A build/task40.elf
arm-none-eabi-nm -n build/task40.elf
arm-none-eabi-objdump -h build/task40.elf
```

Nếu máy chưa có Arm toolchain, vẫn làm cùng bài bằng ELF host để học section:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -ffreestanding -fno-builtin -nostdlib -no-pie \
    -Wl,-e,reset_handler -Wl,-Map=build/task40.map \
    practice/task_40.c -o build/task40.elf
size -A build/task40.elf
nm -n build/task40.elf
objdump -h build/task40.elf
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

Nếu không thấy reset symbol, kiểm tra `-Wl,-e,reset_handler`, symbol có bị khai
báo `static` không và bạn có đang đọc đúng ELF vừa build không.

Nếu không thấy `.rodata`, compiler có thể đã fold constant; kiểm tra bằng
`nm -n` và build ở mức tối ưu mặc định/`-O0`. Nếu `.vectors` mất, kiểm tra
attribute `used` và section name.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** Firmware memory image.

**Bạn vừa code cái gì trong modem?** Linker map cho biết code/data/stack/buffer thực sự nằm ở vùng nào và tốn bao nhiêu byte trên target.

**Nó nằm ở đâu?** Arm firmware / memory architecture.

```text
INPUT  → ELF/map symbols and sections
KHỐI   → code của Bài 40
OUTPUT → mental map ROM/TEXT/DATA/BSS/TCM/SRAM/...
```

**Tại sao modem cần nó?** Đọc map trước khi tự viết linker giúp hiểu linker script không phải nghi thức build. Nó quyết định địa chỉ startup, permissions và nơi các object tồn tại.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 40 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

Full C source đối chiếu:

```c
#include <stdint.h>

volatile uint32_t boot_counter;
uint32_t boot_attempts = UINT32_C(1);
const uint32_t boot_magic = UINT32_C(0x4D4F444D);

static uint32_t add_one(uint32_t value)
{
    return value + UINT32_C(1);
}

void reset_handler(void);

__attribute__((used, section(".vectors")))
void (* const vector_table[])(void) = {
    reset_handler
};

void reset_handler(void)
{
    boot_counter = add_one(boot_magic);
    boot_attempts = boot_attempts + UINT32_C(1);

    for (;;) {
        /* Firmware chờ event/IRQ; bài này không chạy ELF trực tiếp. */
    }
}
```

Build rồi quan sát:

```bash
arm-none-eabi-size -A firmware.elf
arm-none-eabi-nm -n firmware.elf
arm-none-eabi-objdump -h firmware.elf
arm-none-eabi-objdump -t firmware.elf
```

Tên file trong bài thực hành là `build/task40.elf`; nếu copy command trên thì
thay `firmware.elf` bằng tên đó.

Cách đọc:

```text
.text   : machine code, thường RX
.rodata : constant, thường R
.data   : có initial bytes trong image, copy sang RAM khi boot
.bss    : zero-init; chiếm RAM runtime nhưng không cần chứa toàn bộ zero bytes trong image
```

Tìm reset symbol:

```bash
arm-none-eabi-nm -n firmware.elf | grep -E 'reset|Reset|_start'
```

Điểm phải tự trả lời được:

```text
VMA của section là gì?
LMA có khác VMA của .data không?
startup lấy source/destination để copy .data từ symbol linker nào?
.bss start/end symbol nào được startup zero?
stack top nằm ở đâu?
```

Nếu chưa trả lời được năm câu này, chưa nên tự viết `linker.ld` lớn.

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_40_BEGIN -->
#### 9. Wizard Lab — Baseband memory-budget alchemy

**Ý nghĩ quái dị:** Nếu đổi NFFT, số HARQ process, ring capacity và pool slot, RAM budget của riêng modem biến đổi thế nào?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
struct bb_memory_config { size_t nfft, harq_processes, ring_capacity, pool_slots; };
int bb_memory_budget(const struct bb_memory_config *cfg,
                     struct bb_memory_report *out);
void bb_memory_report_dump(const struct bb_memory_report *r);
```

1. **Bậc 1:** Tăng từng parameter một và đo delta bytes theo module.
2. **Bậc 2:** Tìm config lớn nhất dưới budget 256 KiB.
3. **Bậc 3:** Đổi alignment DMA 16→64 và đo padding.

**Observer bắt buộc:** Bytes PHY/runtime/HARQ/pool, padding và total.

**Invariant:** Mọi multiplication/addition check overflow; invalid config không tạo report rác.

**Tự chế spell tiếp theo:** Viết search helper chọn config tốt nhất theo objective tự định nghĩa.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 40</strong></summary>

Lưu thành `practice/wizard_task_40.c`:

```c
#include <assert.h>
#include <stdio.h>

struct bb_memory_config { size_t nfft, harq_processes, ring_capacity, pool_slots; };
struct bb_memory_report { size_t phy, harq, ring, pool, total; };

static int add_mul(size_t *total, size_t count, size_t size, size_t *part)
{
    if (part == NULL || total == NULL || (count != 0U && size > (size_t)-1 / count)) return -1;
    *part = count * size;
    if (*part > (size_t)-1 - *total) return -1;
    *total += *part; return 0;
}

static int bb_memory_budget(const struct bb_memory_config *cfg,
                            struct bb_memory_report *out)
{
    struct bb_memory_report r = {0};
    if (cfg == NULL || out == NULL || cfg->nfft == 0U) return -1;
    if (add_mul(&r.total,cfg->nfft,8U,&r.phy) != 0 ||
        add_mul(&r.total,cfg->harq_processes,4096U,&r.harq) != 0 ||
        add_mul(&r.total,cfg->ring_capacity,32U,&r.ring) != 0 ||
        add_mul(&r.total,cfg->pool_slots,2048U,&r.pool) != 0) return -1;
    *out = r; return 0;
}

static void bb_memory_report_dump(const struct bb_memory_report *r)
{
    if (r != NULL) printf("phy=%zu harq=%zu ring=%zu pool=%zu total=%zu\n",r->phy,r->harq,r->ring,r->pool,r->total);
}

int main(void)
{
    const struct bb_memory_config c = {1024U,8U,64U,16U}; struct bb_memory_report r;
    assert(bb_memory_budget(&c,&r) == 0 && r.total == 75776U);
    bb_memory_report_dump(&r);
    puts("wizard task 40 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_40.c -o wizard_task_40
./wizard_task_40
```

Expected cuối output:

```text
wizard task 40 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_40_END -->

---

#### Forge F48 — Bài 40: linker map phải biến thành invariant executable

Tự code region containment chống wrap, overlap validator và W^X audit. Tạo
fixtures ROM/TCM/SRAM/DDR/SHM/KEY của đề.

<details>
<summary>Đáp án F48</summary>

```c
#include <stddef.h>
#include <stdint.h>
enum perm{P_R=1U,P_W=2U,P_X=4U};
struct region{uintptr_t base;size_t size;unsigned perm;};
int region_end(const struct region*r,uintptr_t*out)
{if((r==NULL)||(out==NULL)||(r->size>(size_t)(UINTPTR_MAX-r->base)))return-1;
 *out=r->base+(uintptr_t)r->size;return 0;}
int region_contains(const struct region*r,uintptr_t addr,size_t n)
{uintptr_t end,re;if(region_end(r,&re)!=0||addr<r->base||n>(size_t)(UINTPTR_MAX-addr))return 0;
 end=addr+(uintptr_t)n;return end<=re;}
int regions_overlap(const struct region*a,const struct region*b)
{uintptr_t ae,be;if(region_end(a,&ae)||region_end(b,&be))return-1;
 return a->base<be&&b->base<ae;}
int map_audit(const struct region*r,size_t n)
{size_t i,j;if((r==NULL)&&(n!=0U))return-1;for(i=0U;i<n;++i){uintptr_t end;
 if(region_end(&r[i],&end)!=0||((r[i].perm&(P_W|P_X))==(P_W|P_X)))return-1;
 for(j=0U;j<i;++j)if(regions_overlap(&r[i],&r[j])!=0)return-1;}return 0;}
```

Sau đó đối chiếu parser/tool output thật: ELF32 little-endian EABI5 soft-float,
undefined symbol rỗng, `.ARM.exidx` ở TCM_RX, không LOAD/GNU_STACK W+X.

</details>

# PHẦN XVII — PROJECT KIẾN TRÚC

## Level 36 — Tách module trước khi project thành 5.000 dòng spaghetti

Cấu trúc đề xuất:

```text
include/bb/
    bb.h
    mac.h
    protocol.h
    soc.h

src/fw/
    runtime/
    memory/
    codec/
    phy/
    channel/
    l2/
    control/
    security/
    trace/

src/hal/
src/soc/
src/host/
tests/
arch/armv7r/
```

Rule:

```text
header = contract
source = implementation
```

Không nhét toàn bộ struct internal vào public API.

### Bài 41 — Tách mini project

#### 1. Đề bài nhỏ

Tách QPSK project thành:

```text
include/dsp.h
src/dsp.c
tests/test_dsp.c
CMakeLists.txt
```

Build bằng CMake.

#### 2. Tự đoán chương trình cần làm gì

Cùng QPSK primitive được tách thành public header, implementation, test và build target. Test chỉ phụ thuộc API công khai.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- header guard và public API
- source/test separation
- CMake target + warning flags

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong `practice/task41/`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

`cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure`; test phải link qua library target.

```bash
cmake -S practice/task41 -B build/task41
cmake --build build/task41
ctest --test-dir build/task41 --output-on-failure
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

Nếu link lỗi, kiểm tra declaration/definition trùng prototype và target test đã link library; bật verbose build để xem command.

Nếu vẫn bí, thêm trace nhỏ tại nơi invariant có thể vỡ rồi chạy lại. Chỉ sau ít nhất một lần compile và một lần quan sát state mới mở lời giải.

#### Sau khi code: ý nghĩa modem/baseband

**Bài này thuộc nhóm:** Software architecture / module boundaries.

**Bạn vừa code cái gì trong modem?** Tách project theo runtime/phy/l2/control/hal/soc/host khiến dependency và ownership rõ, đồng thời cho cùng firmware source chạy host-test và cross-build target.

**Nó nằm ở đâu?** integration/engineering.

```text
INPUT  → các module đã học
KHỐI   → code của Bài 41
OUTPUT → mini project có API rõ
```

**Tại sao modem cần nó?** Đây là bài chống quay lại thói quen copy một file khổng lồ. Khi module có contract, bạn có thể unit-test từng tầng và lần data flow end-to-end.

**Không được sang bài tiếp theo nếu chưa tự nói được:** “Bài 41 nhận cái gì, biến nó thành cái gì, và output đó sẽ được khối nào dùng tiếp.”

#### 7. Đọc lời giải — đặt ngay đây để không phải lật tài liệu

<details>
<summary><strong>Chỉ mở sau khi đã tự code, chạy test và debug</strong></summary>

Cấu trúc:

```text
qpsk101/
├── CMakeLists.txt
├── include/
│   └── dsp.h
├── src/
│   └── dsp.c
└── tests/
    └── test_dsp.c
```

##### `include/dsp.h`

```c
#ifndef DSP_H
#define DSP_H

#include <stdint.h>

struct cpx16 {
    int16_t i;
    int16_t q;
};

struct cpx16 qpsk_map(uint8_t b0, uint8_t b1);
void qpsk_demap(struct cpx16 x, uint8_t *b0, uint8_t *b1);

#endif
```

##### `src/dsp.c`

```c
#include "dsp.h"

#define QPSK_A ((int16_t)23170)

struct cpx16 qpsk_map(uint8_t b0, uint8_t b1)
{
    struct cpx16 x;

    x.i = (b0 & 1U) ? (int16_t)-QPSK_A : QPSK_A;
    x.q = (b1 & 1U) ? (int16_t)-QPSK_A : QPSK_A;
    return x;
}

void qpsk_demap(struct cpx16 x, uint8_t *b0, uint8_t *b1)
{
    if (b0 != 0) {
        *b0 = x.i < 0 ? 1U : 0U;
    }
    if (b1 != 0) {
        *b1 = x.q < 0 ? 1U : 0U;
    }
}
```

##### `tests/test_dsp.c`

```c
#include "dsp.h"
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    for (uint8_t b0 = 0U; b0 <= 1U; ++b0) {
        for (uint8_t b1 = 0U; b1 <= 1U; ++b1) {
            uint8_t r0 = 0U;
            uint8_t r1 = 0U;
            struct cpx16 x = qpsk_map(b0, b1);

            qpsk_demap(x, &r0, &r1);

            if ((r0 != b0) || (r1 != b1)) {
                return 1;
            }
        }
    }

    puts("PASS");
    return 0;
}
```

##### `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.16)
project(qpsk101 C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(dsp STATIC
    src/dsp.c
)

target_include_directories(dsp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(dsp PRIVATE
    -Wall -Wextra -Wconversion -Wshadow -Werror
)

add_executable(test_dsp
    tests/test_dsp.c
)

target_link_libraries(test_dsp PRIVATE dsp)

target_compile_options(test_dsp PRIVATE
    -Wall -Wextra -Wconversion -Wshadow -Werror
)

enable_testing()
add_test(NAME qpsk_roundtrip COMMAND test_dsp)
```

Build/test:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Ý nghĩa cuối cùng của Bài 41: `dsp.h` là **contract**, `dsp.c` là **implementation**, `test_dsp.c` là **evidence**. Đây là pattern phải giữ khi project modem tăng từ vài chục lên hàng nghìn dòng.

---

#### Forge F49 — Bài 41: một ý nghĩ phải đi qua module boundary và test

Tự dựng mini project ba module:

1. `codec` biến byte thành signed symbol.
2. `channel` nhận impairment qua config, không global.
3. `pipeline` chỉ nối public API; test thay channel thật bằng fake qua ops + ctx.

<details>
<summary>Đáp án F49 — contract tối thiểu của project tách module</summary>

```c
/* codec.h */
#include <stddef.h>
#include <stdint.h>
int codec_map(const uint8_t *in, size_t n, int16_t *out, size_t capacity);

/* channel.h */
struct channel_ops {
    int (*apply)(void *ctx, int16_t *symbols, size_t count);
};

/* pipeline.h */
int pipeline_run(const uint8_t *bytes, size_t length,
                 int16_t *symbols, size_t capacity,
                 const struct channel_ops *channel, void *channel_ctx);
```

```c
/* codec.c */
#include "codec.h"
int codec_map(const uint8_t *in,size_t n,int16_t *out,size_t cap)
{size_t k;if((((in==NULL)||(out==NULL))&&(n!=0U))||(cap<n))return-1;
 for(k=0U;k<n;++k)out[k]=(in[k]&1U)!=0U?INT16_C(-16384):INT16_C(16384);return 0;}
```

```c
/* pipeline.c */
#include "codec.h"
#include "channel.h"
int pipeline_run(const uint8_t *bytes,size_t length,int16_t *symbols,size_t cap,
                 const struct channel_ops *ops,void *ctx)
{if((ops==NULL)||(ops->apply==NULL))return-1;
 if(codec_map(bytes,length,symbols,cap)!=0)return-1;
 return ops->apply(ctx,symbols,length);}
```

```c
/* test_pipeline.c: fake chứng minh composition và context */
#include <assert.h>
struct fake_channel { int16_t delta; size_t calls; };
static int fake_apply(void *ctx,int16_t *x,size_t n)
{struct fake_channel *f=ctx;size_t k;++f->calls;
 for(k=0U;k<n;++k)x[k]=(int16_t)(x[k]+f->delta);return 0;}
/* Hai fake context delta khác nhau phải cho output khác và calls độc lập. */
```

Biến thể cuối: thêm observer callback vào pipeline mà không để `codec.c` biết
trace/file/host. Đây là rehearsal nhỏ cho kiến trúc cùng core chạy host và Arm.

</details>

## Checklist lời giải đã được phủ sau phần bổ sung

```text
Bài 1      full code ngay tại bài vì đây là bài cầm tay chỉ việc
Bài 2–41   có solution thu gọn ở Bước 7 ngay tại bài
```

Một vài bài có solution ghép chung vì chúng là hai chiều của cùng primitive, ví dụ QPSK map/demap và MAC mux/demux.

</details>

Khi đối chiếu, tìm ba thứ: contract nào bạn bỏ sót, invariant nào lời giải giữ, và test nào bắt được khác biệt. Không cần ép code giống từng dòng nếu hành vi và invariant đúng.

#### 8. Viết lại không nhìn lời giải

1. Đóng khối `<details>`.
2. Đổi tên implementation vừa đối chiếu hoặc xóa phần đã copy.
3. Tạo bản `*_rebuild.c`/module trắng.
4. Chỉ nhìn đề, prototype, expected output và note bug của chính bạn.
5. Chạy lại đúng bộ test ở Bước 5.
6. Tự nói được `INPUT → CODE → OUTPUT → KHỐI MODEM DÙNG TIẾP`.

**Gate qua bài:** bản viết lại pass test và bạn giải thích được vì sao từng type/bounds/state check tồn tại.


<!-- WIZARD_TASK_41_BEGIN -->
#### 9. Wizard Lab — Tự sinh module từ một ý nghĩ

**Ý nghĩ quái dị:** Nếu nghĩ ra helper mới, làm sao biến nó thành module có header/source/test/CMake mà không quay lại file spaghetti?

Trước khi code, viết ba prediction:

```text
P1 — field/sample/bin/state nào sẽ đổi?
P2 — điều gì bắt buộc không đổi?
P3 — observer nào sẽ là first evidence?
```

**Vocabulary helper cần tự tạo:**

```c
/* Tự tạo contract cho module mới: */
int bb_grid_tools_init(struct bb_grid_tools *tools, size_t nfft);
int bb_grid_tools_apply(struct bb_grid_tools *tools,
                        const struct bb_grid_command *command);
```

1. **Bậc 1:** Tách grid helper khỏi experiment main thành library.
2. **Bậc 2:** Thêm fake observer qua callback mà public API không lộ internals.
3. **Bậc 3:** Tạo hai executable dùng cùng library với recipe khác nhau.

**Observer bắt buộc:** Dependency graph, public-symbol list, unit tests và integration output.

**Invariant:** Header chỉ chứa contract; test không include private source; core không phụ thuộc experiment.

**Tự chế spell tiếp theo:** Tạo checklist/scaffold script local cho module kế tiếp, nhưng tự viết contract trước khi generate file.

Kết thúc Wizard Lab bằng một note ngắn:

```text
Ý nghĩ → representation → helper → actual → first divergence → spell kế tiếp
```



#### Phao cứu sinh — source helper đầy đủ và test nhỏ nhất

Chỉ mở sau khi đã tự viết ít nhất một bản. Đây là **mini implementation
chạy được để học contract**, không phải khối production của modem thương mại.

<details>
<summary><strong>Mở source Wizard Lab TASK 41</strong></summary>

Lưu thành `practice/wizard_task_41.c`:

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define GRID_MAX 32U
struct cpx16 { int16_t i; int16_t q; };
enum command_type { CMD_SET, CMD_ZERO, CMD_SHIFT };
struct bb_grid_command { enum command_type type; int bin, delta; struct cpx16 value; };
struct bb_grid_tools { struct cpx16 bin[GRID_MAX]; size_t nfft; int initialized; };

static int bb_grid_tools_init(struct bb_grid_tools *tools, size_t nfft)
{
    if (tools == NULL || nfft == 0U || nfft > GRID_MAX || (nfft & 1U) != 0U) return -1;
    *tools = (struct bb_grid_tools){0}; tools->nfft = nfft; tools->initialized = 1; return 0;
}

static int logical_index(const struct bb_grid_tools *t, int bin, size_t *out)
{
    if (t == NULL || out == NULL || bin < -(int)(t->nfft/2U) || bin >= (int)(t->nfft/2U)) return -1;
    *out = (size_t)(bin + (int)(t->nfft/2U)); return 0;
}

static int bb_grid_tools_apply(struct bb_grid_tools *t,
                               const struct bb_grid_command *c)
{
    size_t index;
    if (t == NULL || c == NULL || t->initialized == 0) return -1;
    if (c->type == CMD_SET || c->type == CMD_ZERO) {
        if (logical_index(t,c->bin,&index) != 0) return -1;
        t->bin[index] = c->type == CMD_ZERO ? (struct cpx16){0,0} : c->value; return 0;
    }
    if (c->type == CMD_SHIFT) {
        struct cpx16 tmp[GRID_MAX] = {0};
        for (size_t k = 0; k < t->nfft; ++k) {
            const int to = ((int)k + c->delta % (int)t->nfft + (int)t->nfft) % (int)t->nfft;
            tmp[(size_t)to] = t->bin[k];
        }
        for (size_t k = 0; k < t->nfft; ++k) t->bin[k] = tmp[k]; return 0;
    }
    return -1;
}

int main(void)
{
    struct bb_grid_tools t; size_t index;
    const struct bb_grid_command set = {CMD_SET,3,0,{7,8}};
    const struct bb_grid_command shift = {CMD_SHIFT,0,2,{0,0}};
    assert(bb_grid_tools_init(&t,16U) == 0 && bb_grid_tools_apply(&t,&set) == 0);
    assert(logical_index(&t,3,&index) == 0 && t.bin[index].i == 7);
    assert(bb_grid_tools_apply(&t,&shift) == 0);
    puts("wizard task 41 passed");
    return 0;
}
```

Compile và chạy:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror practice/wizard_task_41.c -o wizard_task_41
./wizard_task_41
```

Expected cuối output:

```text
wizard task 41 passed
```

Sau khi chạy pass, đóng lời giải, tạo file trắng và viết lại. Tiếp đó
bắt buộc đổi ít nhất một parameter hoặc tự thêm một helper mới.

</details>

<!-- WIZARD_TASK_41_END -->

---

<!-- BASEBAND_WIZARD_LAB_BEGIN -->
# PHẦN XVII-W — BASEBAND WIZARD LAB: BIẾN Ý NGHĨ QUÁI DỊ THÀNH HELPER C

Phần này không thay thế 41 bài chính. Nó là track thứ ba chạy song song:

```text
C FOUNDATION       → viết đúng và an toàn
MODEM FOUNDATION   → hiểu signal/data/state/ownership
WIZARD TRACK       → tự nghĩ thí nghiệm, tự tạo helper, tự quan sát và tự rút quy luật
```

Một “phù thủy baseband” trong tài liệu này không phải người thuộc nhiều API
nhất. Đó là người có thể nhìn một ý nghĩ còn mơ hồ như:

```text
“Nếu tao chuyển bin +3 sang bin -3 thì waveform sẽ đổi thế nào?”
```

rồi tự biến nó thành:

```text
quy ước bin
→ type C
→ helper có contract
→ baseline
→ mutation
→ observer
→ invariant
→ test
→ một câu hỏi mới khó hơn
```

Toàn bộ phần này chỉ làm việc với modem/baseband trong simulator:

- complex IQ;
- constellation;
- resource grid và OFDM bin;
- virtual channel;
- synchronization;
- event/queue/timer;
- buffer ownership;
- PDU/parser/state machine;
- host test và virtual RF.

Mọi input đều là virtual IQ, local file hoặc object C trong simulator. Tinh thần
“nghịch” ở đây chỉ là **tạo baseline, thay một biến và quan sát** bên trong
modem/baseband.

---

## W.1. Khác biệt giữa “biết làm bài” và “biết tạo phép”

Người biết làm bài thường làm được đường đã có sẵn:

```text
đề cho prototype
→ điền implementation
→ expected output pass
```

Người biết tạo phép có thể tự tạo đường mới:

```text
nhìn thấy một pattern
→ hỏi “nếu đổi nó thì sao?”
→ chọn representation cần sửa
→ viết helper nhỏ nhất để sửa
→ viết observer để nhìn hậu quả
→ đặt property để biết kết quả có hợp lý không
→ ghép helper thành thí nghiệm lớn hơn
```

Không được nhảy thẳng tới “framework tổng quát”. Phép thuật thật bắt đầu bằng
một helper hơi vụng nhưng nhìn thấy được:

```c
int grid_move_bin(struct grid *grid, int from_bin, int to_bin);
```

Sau khi helper này chạy đúng, ta mới nghĩ tiếp:

```c
int grid_move_band(struct grid *grid,
                   int first_bin,
                   size_t count,
                   int delta);
```

Rồi mới tới callback/pipeline:

```c
typedef int (*grid_mutator_fn)(struct grid *grid, void *ctx);
```

Đây là quy luật:

```text
case cụ thể
→ helper cụ thể
→ nhìn ra phần thay đổi
→ parameter hóa đúng phần đó
→ composition
```

Không parameter hóa mọi thứ từ đầu. Làm vậy thường tạo abstraction đẹp nhưng
không giải được bài nào.

---

## W.2. “Compiler trong đầu”: dịch suy nghĩ sang C qua chín pass

Giả sử ý nghĩ ban đầu là:

> “Tao muốn đục một lỗ ở giữa resource grid rồi xem receiver phản ứng.”

### Pass 1 — Viết lại ý nghĩ bằng động từ có thể đo

“Đục lỗ” phải được dịch thành:

```text
đặt I=0,Q=0 cho một dải bin xác định
```

Nếu chưa có động từ như `set`, `move`, `drop`, `delay`, `duplicate`, `rotate`,
`compare`, `count`, bạn chưa sẵn sàng code.

### Pass 2 — Chỉ ra object bị thay đổi

```text
object = frequency-domain grid
không phải time-domain IQ block
không phải payload bytes
```

Sai representation thì helper vẫn compile nhưng thí nghiệm không có nghĩa.

### Pass 3 — Chọn coordinate system

Với `NFFT=16`, tài liệu dùng logical bin:

```text
-8 -7 -6 -5 -4 -3 -2 -1 0 +1 +2 +3 +4 +5 +6 +7
```

Trong array C, negative bin phải được map về index. Viết helper mapping một lần,
không rải công thức `nfft + bin` khắp source.

### Pass 4 — Viết contract trước implementation

```c
int grid_notch(struct grid *grid, int first_bin, int last_bin);
```

Hỏi:

- `first_bin > last_bin` thì sao?
- bin ngoài miền thì sao?
- failure có được sửa một nửa grid không?
- range inclusive hay exclusive?

### Pass 5 — Viết baseline nhỏ tính tay được

```text
bin -3 = (100, 10)
bin -1 = (200, 20)
bin +1 = (300, 30)
bin +3 = (400, 40)
```

Đừng bắt đầu bằng grid 4096 bin và 14 OFDM symbols.

### Pass 6 — Viết observer trước mutation lớn

Ít nhất cần:

```c
void grid_dump(const struct grid *grid);
int64_t grid_energy_l1(const struct grid *grid);
uint32_t grid_digest(const struct grid *grid);
size_t grid_diff_count(const struct grid *a, const struct grid *b);
```

Không có observer thì bạn chỉ “cảm giác code đã chạy”.

### Pass 7 — Viết property

Ví dụ notch:

```text
mọi bin ngoài vùng notch phải byte-identical
mọi bin trong vùng notch phải bằng zero
energy sau notch không được lớn hơn baseline
```

### Pass 8 — Chạy một biến mỗi lần

```text
baseline
→ notch bin +1
→ reset baseline
→ notch bin -1..+1
→ reset baseline
→ notch toàn band
```

Không vừa notch, vừa shift, vừa inject noise rồi hỏi bug ở đâu.

### Pass 9 — Tự sinh câu hỏi kế tiếp

Sau notch, hỏi tiếp:

```text
Nếu không zero mà giảm gain 1/2 thì sao?
Nếu notch chỉ xuất hiện ở OFDM symbol lẻ thì sao?
Nếu pilot bị notch nhưng data không bị thì observer nào bắt được?
```

Đây là chỗ “học” biến thành “nghiên cứu”.

---

## W.3. Chín loại helper phải biết tự nghĩ ra

| Loại helper | Câu hỏi nó trả lời | Ví dụ baseband |
|---|---|---|
| Constructor | Tạo object hợp lệ nhỏ nhất thế nào? | `cpx16_make`, `event_make` |
| Generator | Tạo cả pattern thế nào? | `iq_make_impulse`, `grid_make_comb` |
| Transformer | Đổi object hợp lệ thành object khác | `iq_conjugate`, `grid_shift` |
| Mutator | Sửa tại chỗ có chủ đích | `grid_set_bin`, `event_delay` |
| Inspector | Nhìn state/data mà không sửa | `grid_dump`, `ring_snapshot` |
| Metric | Nén object thành số đo | `iq_energy_l1`, `corr_peak` |
| Predicate | Property có đúng không? | `grid_is_guard_clean` |
| Adapter | Nối hai representation/API | `logical_bin_to_index` |
| Oracle | Kết quả có khớp reference? | `fft_matches_dft` |

Mỗi lần bí code, hỏi:

```text
Mình đang thiếu loại helper nào?
```

Ví dụ bạn muốn “thay đổi bin theo ý” nhưng cứ viết trực tiếp:

```c
grid.bin[15] = value;
```

Bạn đang thiếu adapter:

```c
int logical_bin_to_index(int bin, size_t *out_index);
```

và mutator:

```c
int grid_set_bin(struct grid *grid, int bin, struct cpx16 value);
```

Khi hai helper này chắc, mọi trò `move/swap/notch/paint/shift/mirror` trở nên dễ
viết hơn nhiều.

---

## W.4. Helper tốt phải có “mép” rõ

Một helper baseband không chỉ có happy path. Trước khi code, điền bảng:

| Mép | Câu phải trả lời |
|---|---|
| Input domain | bin/sample/state nào hợp lệ? |
| Output | return value và object nào thay đổi? |
| Atomicity | fail có để object bị sửa một nửa không? |
| Ownership | helper mượn hay nhận ownership? |
| Aliasing | input/output có được trùng nhau không? |
| Capacity | caller cấp bao nhiêu storage? |
| Numeric | có widen/saturation/overflow không? |
| Determinism | cùng input/seed có ra cùng output không? |
| Observability | test nhìn hậu quả bằng gì? |

Ví dụ contract đầy đủ hơn:

```c
/*
 * Move one logical OFDM bin.
 *
 * Success:
 *   - destination receives the source value;
 *   - source becomes zero;
 *   - all other bins remain unchanged.
 *
 * Failure:
 *   - returns -1;
 *   - grid remains byte-identical.
 */
int grid_move_bin(struct grid *grid, int from_bin, int to_bin);
```

Comment này không phải trang trí. Nó là blueprint của test.

---

## W.4A. Tương tác C — một helper không sống một mình

Biết viết từng hàm riêng lẻ vẫn chưa đủ. Modem là chuỗi object và stage tương
tác với nhau. Ta phải nhìn được **ai gọi ai, object nào đi qua ranh giới nào,
ai được sửa object đó và lỗi được truyền ngược về đâu**.

Một experiment đúng thường có hình dạng:

```text
constructor/generator
        ↓ tạo baseline hợp lệ
adapter
        ↓ đổi cách gọi tên hoặc representation
transformer/mutator
        ↓ tạo hiện tượng cần nghiên cứu
inspector/metric
        ↓ biến hậu quả thành evidence
predicate/oracle
        ↓ quyết định property pass hay fail
test/experiment runner
        ↓ giữ toàn bộ vòng lặp chạy lại được
```

### W.4A.1. Năm kiểu tương tác C phải luyện

| Kiểu tương tác | Khi dùng | Ví dụ modem/baseband | Bẫy cần tránh |
|---|---|---|---|
| Call trực tiếp | Một phép biến đổi nhỏ, luồng rõ | `grid_set_bin()` gọi `logical_bin_to_index()` | Helper dưới âm thầm sửa global |
| Pipeline dữ liệu | Output stage trước là input stage sau | mapper → grid → IFFT → channel → FFT | Mất contract về length/scale giữa hai stage |
| Callback + `void *ctx` | Muốn đổi thuật toán nhưng giữ runner | mutator của experiment, fake clock | Cast sai type hoặc `ctx` chết trước callback |
| Ops table | Cùng interface, nhiều backend | host clock và Arm timer; virtual RF backend | Function pointer NULL hoặc signature lệch |
| Queue/handle | Vượt task/ISR/owner | RX buffer từ ISR sang deferred task | Gửi raw pointer qua domain, double-release |

Mỗi khi nối hai helper, phải viết ra **interaction contract**:

```text
producer trả gì?
consumer đòi gì?
storage do ai cấp?
owner trước và sau call là ai?
length/capacity dùng đơn vị byte, sample hay element?
fail ở consumer thì producer/object còn hợp lệ không?
observer đứng ở mép nào để thấy first divergence?
```

### W.4A.2. Ví dụ 1 — helper gọi helper, không lặp lại mapping

Sai về thiết kế:

```c
int grid_zero_bin(struct grid *grid, int bin)
{
    /* Tự chép lại công thức bin → index tại đây. */
    size_t index = (size_t)(bin + (int)(GRID_N / 2U));
    grid->bins[index].i = 0;
    grid->bins[index].q = 0;
    return 0;
}
```

Vấn đề không chỉ là code dài. Hai helper có thể dần dùng hai quy ước bin khác
nhau. Tốt hơn là composition:

```c
int grid_zero_bin(struct grid *grid, int bin)
{
    const struct cpx16 zero = cpx16_make(INT16_C(0), INT16_C(0));

    return grid_set_bin(grid, bin, zero);
}
```

Ở đây:

```text
cpx16_make  chịu trách nhiệm tạo sample
grid_set_bin chịu trách nhiệm validate grid/bin và mapping
grid_zero_bin chỉ diễn tả policy “ghi zero”
```

Nếu test fail, ta đã biết nên kiểm tra layer nào trước.

### W.4A.3. Ví dụ 2 — truyền lỗi xuyên chuỗi helper

Đừng bỏ qua status chỉ vì experiment đang nhỏ:

```c
int grid_move_then_notch(struct grid *grid,
                         int from_bin,
                         int to_bin,
                         int first_notch,
                         int last_notch)
{
    struct grid temporary;

    if (grid == NULL) {
        return -1;
    }

    temporary = *grid;

    if (grid_move_bin(&temporary, from_bin, to_bin) != 0) {
        return -1;
    }

    if (grid_notch(&temporary, first_notch, last_notch) != 0) {
        return -1;
    }

    *grid = temporary;
    return 0;
}
```

`temporary` tạo **failure atomicity**: stage thứ hai fail thì caller không nhận
một grid đã move dở. Đây là pattern rất hữu ích cho parser, config update và
state transition trong modem.

Tự test ba đường:

```text
move fail  → original byte-identical
notch fail → original byte-identical
cả hai pass → chỉ những bin trong contract được đổi
```

### W.4A.4. Ví dụ 3 — callback biến suy nghĩ thành plug-in nhỏ

Ta không muốn mỗi experiment copy lại đoạn clone/dump/diff/test. Runner nhận
một hành vi qua function pointer:

```c
typedef int (*grid_mutator_fn)(struct grid *grid, void *ctx);

int run_grid_experiment(const char *name,
                        const struct grid *baseline,
                        grid_mutator_fn mutate,
                        void *ctx);
```

Ba thứ tương tác nhưng giữ vai trò tách biệt:

```text
mutate → code hành vi: move/notch/shift
ctx    → parameter/state riêng của đúng hành vi đó
runner → clone baseline, gọi mutate, đo diff và in evidence
```

Ví dụ context:

```c
struct move_ctx {
    int from_bin;
    int to_bin;
};

static int mutate_move(struct grid *grid, void *ctx)
{
    const struct move_ctx *move = ctx;

    if (move == NULL) {
        return -1;
    }

    return grid_move_bin(grid, move->from_bin, move->to_bin);
}
```

Không dùng global `current_from_bin`. Hai experiment phải có thể tồn tại cùng
lúc mà không lẫn parameter.

### W.4A.5. Ví dụ 4 — ops table nối core modem với backend

```c
struct bb_clock_ops {
    uint64_t (*ticks)(void *ctx);
};

struct bb_clock {
    const struct bb_clock_ops *ops;
    void *ctx;
};

uint64_t bb_clock_ticks(const struct bb_clock *clock)
{
    if ((clock == NULL) || (clock->ops == NULL) ||
        (clock->ops->ticks == NULL)) {
        return UINT64_C(0);
    }

    return clock->ops->ticks(clock->ctx);
}
```

Core chỉ gọi `bb_clock_ticks()`. Host có thể gắn fake clock; target có thể gắn
timer platform. Interaction này đáng luyện vì nó xuất hiện ở time, virtual RF,
trace sink, DMA backend và entropy provider.

Test bắt buộc:

```text
hai fake clock có ctx khác nhau → đọc ra hai giá trị khác nhau
ops NULL                         → fail an toàn
ticks NULL                       → fail an toàn
đổi now của clock A              → clock B không đổi
```

### W.4A.6. Helper ladder — từ code thẳng tới vocabulary tự chế

Với mỗi ý tưởng mới, đi qua năm nấc:

```text
Nấc 1 — viết thẳng đúng một case trong main
Nấc 2 — khoanh phần thay đổi thành helper
Nấc 3 — tách parameter thành struct context
Nấc 4 — viết observer + invariant + failure test
Nấc 5 — đưa helper vào runner để compose với helper khác
```

Ví dụ ý tưởng “bin pilot đôi lúc biến mất”:

| Nấc | Thứ phải code |
|---|---|
| 1 | `grid.bins[index] = zero` cho đúng một symbol |
| 2 | `grid_zero_bin(grid, pilot_bin)` |
| 3 | `struct intermittent_notch_ctx { int bin; size_t every; }` |
| 4 | `grid_diff_count`, `grid_is_guard_clean`, test `every == 0` |
| 5 | callback chạy qua nhiều OFDM symbol và ghi timeline evidence |

Không cần nhảy thẳng lên Nấc 5. Khả năng “code được thứ lạ” lớn lên từ việc
biết nâng một đoạn code cụ thể lên từng nấc, không phải cố phát minh abstraction
hoàn hảo ngay lần đầu.

### W.4A.7. Sáu bài tương tác C bắt buộc

1. Viết `iq_transform_in_place()` nhận callback biến đổi từng sample và `ctx`.
2. Dùng cùng runner cho conjugate, rotate 90°, scale Q15 và inject impulse.
3. Viết `grid_pipeline_run()` nhận mảng ba mutator; dừng ở lỗi đầu tiên và trả
   index stage fail.
4. Viết bản transactional: bất kỳ stage nào fail thì output bằng baseline.
5. Gắn fake clock qua ops table vào timeline runner; hai clock không được dùng
   chung state ngầm.
6. Nối `PDU generator → mutator → parser → state machine → trace observer`; sau
   mỗi ranh giới ghi digest để tìm first divergence.

Gate hoàn thành:

```text
[ ] Không helper nào cần global mutable state.
[ ] Mỗi callback có ctx với lifetime giải thích được.
[ ] Mỗi chain propagate lỗi; không nuốt status.
[ ] Có test fail ở stage đầu, giữa và cuối.
[ ] Có observer sau từng boundary quan trọng.
[ ] Tự thêm được mutator mới mà không sửa runner.
```

---

## W.5. Repo riêng cho trò nghịch

Đừng nhét experiment bừa vào `phy.c`. Tạo lab riêng:

```text
wizard_baseband/
├── CMakeLists.txt
├── include/
│   └── wizard/
│       ├── iq_tools.h
│       ├── grid_tools.h
│       ├── experiment.h
│       └── timeline_tools.h
├── src/
│   ├── iq_tools.c
│   ├── grid_tools.c
│   ├── experiment.c
│   └── timeline_tools.c
├── experiments/
│   ├── exp_move_bin.c
│   ├── exp_notch_pilot.c
│   ├── exp_phase_jump.c
│   └── exp_delay_event.c
├── tests/
│   ├── test_iq_tools.c
│   ├── test_grid_tools.c
│   └── test_timeline_tools.c
└── notes/
    └── experiment_log.md
```

Quy tắc dependency:

```text
tools không biết experiment cụ thể
experiment được phép gọi tools
observer không sửa object
test không phụ thuộc thứ tự chạy của experiment khác
```

---

## W.6. Full Lab 1 — IQ Spellbook và OFDM Bin Forge

File: `wizard_baseband/wizard_iq_grid_lab.c`

Lab này dạy bốn năng lực cùng lúc:

1. tạo helper cho complex IQ;
2. dùng logical bin thay vì index khó nhớ;
3. clone baseline rồi mutate bản sao;
4. dùng callback để biến một mutation thành experiment có thể chạy lại.

```c
#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define NFFT 16

struct cpx16 {
    int16_t i;
    int16_t q;
};

struct grid {
    struct cpx16 bins[NFFT];
};

typedef int (*grid_mutator_fn)(struct grid *grid, void *ctx);

struct move_args {
    int from_bin;
    int to_bin;
};

struct notch_args {
    int first_bin;
    int last_bin;
};

struct shift_args {
    int delta;
};

static int16_t sat16(int32_t value)
{
    if (value > INT16_MAX) {
        return INT16_MAX;
    }
    if (value < INT16_MIN) {
        return INT16_MIN;
    }
    return (int16_t)value;
}

static int32_t abs32(int32_t value)
{
    return value < 0 ? -value : value;
}

static struct cpx16 cpx16_make(int16_t i, int16_t q)
{
    const struct cpx16 value = {i, q};
    return value;
}

static int cpx16_equal(struct cpx16 a, struct cpx16 b)
{
    return (a.i == b.i) && (a.q == b.q);
}

static void iq_conjugate(struct cpx16 *samples, size_t count)
{
    if (samples == NULL) {
        return;
    }

    for (size_t index = 0U; index < count; ++index) {
        samples[index].q = sat16(-(int32_t)samples[index].q);
    }
}

static void iq_rotate_90(struct cpx16 *samples, size_t count)
{
    if (samples == NULL) {
        return;
    }

    for (size_t index = 0U; index < count; ++index) {
        const int16_t old_i = samples[index].i;
        const int16_t old_q = samples[index].q;

        samples[index].i = sat16(-(int32_t)old_q);
        samples[index].q = old_i;
    }
}

static void iq_scale_q15(struct cpx16 *samples,
                         size_t count,
                         int16_t gain_q15)
{
    if (samples == NULL) {
        return;
    }

    for (size_t index = 0U; index < count; ++index) {
        const int32_t i_product =
            (int32_t)samples[index].i * (int32_t)gain_q15;
        const int32_t q_product =
            (int32_t)samples[index].q * (int32_t)gain_q15;

        samples[index].i = sat16(i_product / INT32_C(32768));
        samples[index].q = sat16(q_product / INT32_C(32768));
    }
}

static void iq_reverse(struct cpx16 *samples, size_t count)
{
    if (samples == NULL) {
        return;
    }

    for (size_t left = 0U; left < count / 2U; ++left) {
        const size_t right = count - 1U - left;
        const struct cpx16 temporary = samples[left];

        samples[left] = samples[right];
        samples[right] = temporary;
    }
}

static void iq_dump(const char *name,
                    const struct cpx16 *samples,
                    size_t count)
{
    if ((name == NULL) || ((samples == NULL) && (count != 0U))) {
        return;
    }

    printf("IQ %s count=%zu\n", name, count);
    for (size_t index = 0U; index < count; ++index) {
        printf("  [%zu] I=%d Q=%d\n",
               index, (int)samples[index].i, (int)samples[index].q);
    }
}

static int logical_bin_to_index(int bin, size_t *out_index)
{
    if (out_index == NULL) {
        return -1;
    }
    if ((bin < -(NFFT / 2)) || (bin >= (NFFT / 2))) {
        return -1;
    }

    *out_index = bin < 0 ? (size_t)(bin + NFFT) : (size_t)bin;
    return 0;
}

static int normalize_bin(int bin, int *out_bin)
{
    if (out_bin == NULL) {
        return -1;
    }
    if ((bin < -(NFFT + NFFT / 2)) ||
        (bin >= (NFFT + NFFT / 2))) {
        return -1;
    }

    while (bin < -(NFFT / 2)) {
        bin += NFFT;
    }
    while (bin >= (NFFT / 2)) {
        bin -= NFFT;
    }

    *out_bin = bin;
    return 0;
}

static void grid_clear(struct grid *grid)
{
    if (grid == NULL) {
        return;
    }

    for (size_t index = 0U; index < (size_t)NFFT; ++index) {
        grid->bins[index] = cpx16_make(INT16_C(0), INT16_C(0));
    }
}

static int grid_get_bin(const struct grid *grid,
                        int bin,
                        struct cpx16 *out_value)
{
    size_t index;

    if ((grid == NULL) || (out_value == NULL)) {
        return -1;
    }
    if (logical_bin_to_index(bin, &index) != 0) {
        return -1;
    }

    *out_value = grid->bins[index];
    return 0;
}

static int grid_set_bin(struct grid *grid,
                        int bin,
                        struct cpx16 value)
{
    size_t index;

    if (grid == NULL) {
        return -1;
    }
    if (logical_bin_to_index(bin, &index) != 0) {
        return -1;
    }

    grid->bins[index] = value;
    return 0;
}

static int grid_move_bin(struct grid *grid, int from_bin, int to_bin)
{
    struct cpx16 value;

    if (grid == NULL) {
        return -1;
    }
    if ((logical_bin_to_index(from_bin, &(size_t){0U}) != 0) ||
        (logical_bin_to_index(to_bin, &(size_t){0U}) != 0)) {
        return -1;
    }
    if (from_bin == to_bin) {
        return 0;
    }
    if (grid_get_bin(grid, from_bin, &value) != 0) {
        return -1;
    }
    if (grid_set_bin(grid, to_bin, value) != 0) {
        return -1;
    }

    return grid_set_bin(grid, from_bin,
                        cpx16_make(INT16_C(0), INT16_C(0)));
}

static int grid_swap_bins(struct grid *grid, int first_bin, int second_bin)
{
    struct cpx16 first;
    struct cpx16 second;

    if (grid == NULL) {
        return -1;
    }
    if ((grid_get_bin(grid, first_bin, &first) != 0) ||
        (grid_get_bin(grid, second_bin, &second) != 0)) {
        return -1;
    }
    if ((grid_set_bin(grid, first_bin, second) != 0) ||
        (grid_set_bin(grid, second_bin, first) != 0)) {
        return -1;
    }

    return 0;
}

static int grid_notch(struct grid *grid, int first_bin, int last_bin)
{
    if ((grid == NULL) || (first_bin > last_bin)) {
        return -1;
    }
    if ((first_bin < -(NFFT / 2)) || (last_bin >= (NFFT / 2))) {
        return -1;
    }

    for (int bin = first_bin; bin <= last_bin; ++bin) {
        if (grid_set_bin(grid, bin,
                         cpx16_make(INT16_C(0), INT16_C(0))) != 0) {
            return -1;
        }
    }

    return 0;
}

static int grid_shift(struct grid *grid, int delta)
{
    struct grid shifted;

    if ((grid == NULL) || (delta < -NFFT) || (delta > NFFT)) {
        return -1;
    }

    grid_clear(&shifted);
    for (int bin = -(NFFT / 2); bin < (NFFT / 2); ++bin) {
        struct cpx16 value;
        int destination_bin;

        if ((grid_get_bin(grid, bin, &value) != 0) ||
            (normalize_bin(bin + delta, &destination_bin) != 0) ||
            (grid_set_bin(&shifted, destination_bin, value) != 0)) {
            return -1;
        }
    }

    *grid = shifted;
    return 0;
}

static int64_t grid_energy_l1(const struct grid *grid)
{
    int64_t energy = 0;

    if (grid == NULL) {
        return 0;
    }

    for (size_t index = 0U; index < (size_t)NFFT; ++index) {
        energy += (int64_t)abs32((int32_t)grid->bins[index].i);
        energy += (int64_t)abs32((int32_t)grid->bins[index].q);
    }

    return energy;
}

static uint32_t grid_digest(const struct grid *grid)
{
    uint32_t hash = UINT32_C(2166136261);

    if (grid == NULL) {
        return 0U;
    }

    for (size_t index = 0U; index < (size_t)NFFT; ++index) {
        hash ^= (uint32_t)(uint16_t)grid->bins[index].i;
        hash *= UINT32_C(16777619);
        hash ^= (uint32_t)(uint16_t)grid->bins[index].q;
        hash *= UINT32_C(16777619);
    }

    return hash;
}

static size_t grid_diff_count(const struct grid *a, const struct grid *b)
{
    size_t count = 0U;

    if ((a == NULL) || (b == NULL)) {
        return 0U;
    }

    for (size_t index = 0U; index < (size_t)NFFT; ++index) {
        if (!cpx16_equal(a->bins[index], b->bins[index])) {
            ++count;
        }
    }

    return count;
}

static void grid_dump(const char *name, const struct grid *grid)
{
    if ((name == NULL) || (grid == NULL)) {
        return;
    }

    printf("GRID %s energy=%" PRId64 " digest=%08" PRIx32 "\n",
           name, grid_energy_l1(grid), grid_digest(grid));

    for (int bin = -(NFFT / 2); bin < (NFFT / 2); ++bin) {
        struct cpx16 value;

        assert(grid_get_bin(grid, bin, &value) == 0);
        printf("  bin=%+3d I=%6d Q=%6d\n",
               bin, (int)value.i, (int)value.q);
    }
}

static int mutate_move(struct grid *grid, void *ctx)
{
    const struct move_args *args = ctx;

    if (args == NULL) {
        return -1;
    }
    return grid_move_bin(grid, args->from_bin, args->to_bin);
}

static int mutate_notch(struct grid *grid, void *ctx)
{
    const struct notch_args *args = ctx;

    if (args == NULL) {
        return -1;
    }
    return grid_notch(grid, args->first_bin, args->last_bin);
}

static int mutate_shift(struct grid *grid, void *ctx)
{
    const struct shift_args *args = ctx;

    if (args == NULL) {
        return -1;
    }
    return grid_shift(grid, args->delta);
}

static int run_grid_experiment(const char *name,
                               const struct grid *baseline,
                               grid_mutator_fn mutate,
                               void *ctx)
{
    struct grid candidate;
    int rc;

    if ((name == NULL) || (baseline == NULL) || (mutate == NULL)) {
        return -1;
    }

    candidate = *baseline;
    rc = mutate(&candidate, ctx);
    if (rc != 0) {
        printf("EXPERIMENT %s status=REJECTED\n", name);
        return rc;
    }

    printf("EXPERIMENT %s changed_bins=%zu energy_before=%" PRId64
           " energy_after=%" PRId64 "\n",
           name,
           grid_diff_count(baseline, &candidate),
           grid_energy_l1(baseline),
           grid_energy_l1(&candidate));
    grid_dump(name, &candidate);
    return 0;
}

int main(void)
{
    struct cpx16 iq[] = {
        {INT16_C(1000), INT16_C(100)},
        {INT16_C(2000), INT16_C(-200)},
        {INT16_C(-3000), INT16_C(300)}
    };
    struct grid baseline;
    struct grid probe;
    struct cpx16 value;
    const struct move_args move = {.from_bin = 3, .to_bin = -3};
    const struct notch_args notch = {.first_bin = -1, .last_bin = 1};
    const struct shift_args shift = {.delta = 2};
    uint32_t baseline_digest;

    iq_dump("baseline", iq, sizeof iq / sizeof iq[0]);
    iq_conjugate(iq, sizeof iq / sizeof iq[0]);
    iq_rotate_90(iq, sizeof iq / sizeof iq[0]);
    iq_scale_q15(iq, sizeof iq / sizeof iq[0], INT16_C(16384));
    iq_reverse(iq, sizeof iq / sizeof iq[0]);
    iq_dump("mutated", iq, sizeof iq / sizeof iq[0]);

    grid_clear(&baseline);
    assert(grid_set_bin(&baseline, -3,
                        cpx16_make(INT16_C(100), INT16_C(10))) == 0);
    assert(grid_set_bin(&baseline, -1,
                        cpx16_make(INT16_C(200), INT16_C(20))) == 0);
    assert(grid_set_bin(&baseline, 1,
                        cpx16_make(INT16_C(300), INT16_C(30))) == 0);
    assert(grid_set_bin(&baseline, 3,
                        cpx16_make(INT16_C(400), INT16_C(40))) == 0);

    baseline_digest = grid_digest(&baseline);
    grid_dump("baseline", &baseline);

    probe = baseline;
    assert(grid_swap_bins(&probe, -3, 3) == 0);
    assert(grid_swap_bins(&probe, -3, 3) == 0);
    assert(grid_digest(&probe) == baseline_digest);

    assert(run_grid_experiment("move +3 to -3", &baseline,
                               mutate_move, (void *)&move) == 0);
    assert(run_grid_experiment("notch -1..+1", &baseline,
                               mutate_notch, (void *)&notch) == 0);
    assert(run_grid_experiment("cyclic shift +2", &baseline,
                               mutate_shift, (void *)&shift) == 0);

    assert(grid_digest(&baseline) == baseline_digest);
    assert(grid_get_bin(&baseline, 3, &value) == 0);
    assert(cpx16_equal(value,
                       cpx16_make(INT16_C(400), INT16_C(40))));

    puts("wizard iq/grid lab passed");
    return 0;
}
```

Compile:

```bash
mkdir -p wizard_baseband build
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined \
    wizard_baseband/wizard_iq_grid_lab.c \
    -o build/wizard_iq_grid_lab
ASAN_OPTIONS=detect_leaks=0 ./build/wizard_iq_grid_lab
```

Expected cuối output:

```text
wizard iq/grid lab passed
```

### W.6.1. Đọc code theo helper graph

```text
logical_bin_to_index
        ↓
grid_get_bin / grid_set_bin
        ↓
move / swap / notch / shift
        ↓
mutator callback
        ↓
run_grid_experiment
        ↓
dump / energy / digest / diff
```

Đây là hình dạng của khả năng “hiện thực hóa ý nghĩ”:

```text
không viết một main khổng lồ
mà xây vocabulary helper đủ tốt
để câu hỏi mới chỉ còn là composition
```

### W.6.2. Mười lần nghịch bắt buộc với Bin Forge

1. Đổi `move +3 → -3` thành `-1 → +6`; dự đoán hai bin nào đổi.
2. Move từ bin zero; giải thích DC bị thay đổi có ý nghĩa gì.
3. Move một bin zero sang bin đang có data; data cũ ở destination đi đâu?
4. Sửa contract để reject destination không-zero.
5. Viết `grid_copy_bin`: source không bị xóa.
6. Viết `grid_scale_bin_q15` để giảm gain đúng một carrier.
7. Viết `grid_paint_comb(first, step, value)`.
8. Viết `grid_mirror`; quyết định bin Nyquist `-N/2` xử lý thế nào.
9. Viết `grid_is_guard_clean` và cố ý làm bẩn guard.
10. Compose `shift → notch → move`; sau mỗi stage phải dump/digest riêng.

Sau mỗi trò, ghi:

```text
Tôi đã đổi representation nào?
Helper nào thực hiện mutation?
Observer nào chứng minh nó đã đổi?
Phần nào phải giữ nguyên?
Nếu kết quả khác dự đoán, giả thuyết mới là gì?
```

---

## W.7. Full Lab 2 — Runtime Timeline Sorcery

Baseband không chỉ có signal. Một ý nghĩ như:

> “Nếu RX completion đến muộn 75 tick và timer bị duplicate thì state có còn
> deterministic không?”

cũng phải biến được thành helper C.

File: `wizard_baseband/wizard_runtime_lab.c`

```c
#include <assert.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define MAX_EVENTS 16U

enum event_type {
    EV_RX = 1,
    EV_TIMER,
    EV_TASK,
    EV_TRACE
};

struct event {
    uint64_t tick;
    enum event_type type;
    uint32_t value;
    uint32_t serial;
};

struct timeline {
    struct event events[MAX_EVENTS];
    size_t count;
    uint32_t next_serial;
};

static const char *event_name(enum event_type type)
{
    switch (type) {
    case EV_RX: return "RX";
    case EV_TIMER: return "TIMER";
    case EV_TASK: return "TASK";
    case EV_TRACE: return "TRACE";
    default: return "UNKNOWN";
    }
}

static void timeline_init(struct timeline *timeline)
{
    if (timeline == NULL) {
        return;
    }

    timeline->count = 0U;
    timeline->next_serial = 0U;
}

static int timeline_push(struct timeline *timeline,
                         uint64_t tick,
                         enum event_type type,
                         uint32_t value)
{
    struct event *event;

    if ((timeline == NULL) || (timeline->count >= (size_t)MAX_EVENTS)) {
        return -1;
    }

    event = &timeline->events[timeline->count];
    event->tick = tick;
    event->type = type;
    event->value = value;
    event->serial = timeline->next_serial;

    ++timeline->count;
    ++timeline->next_serial;
    return 0;
}

static int timeline_delay_type(struct timeline *timeline,
                               enum event_type type,
                               uint64_t delta)
{
    if (timeline == NULL) {
        return -1;
    }

    for (size_t index = 0U; index < timeline->count; ++index) {
        if (timeline->events[index].type == type) {
            if (UINT64_MAX - timeline->events[index].tick < delta) {
                return -1;
            }
            timeline->events[index].tick += delta;
        }
    }

    return 0;
}

static int timeline_duplicate_first(struct timeline *timeline,
                                    enum event_type type,
                                    uint64_t extra_delay)
{
    if ((timeline == NULL) || (timeline->count >= (size_t)MAX_EVENTS)) {
        return -1;
    }

    for (size_t index = 0U; index < timeline->count; ++index) {
        if (timeline->events[index].type == type) {
            const struct event source = timeline->events[index];

            if (UINT64_MAX - source.tick < extra_delay) {
                return -1;
            }

            return timeline_push(timeline,
                                 source.tick + extra_delay,
                                 source.type,
                                 source.value);
        }
    }

    return -1;
}

static size_t timeline_drop_type(struct timeline *timeline,
                                 enum event_type type)
{
    size_t write_index = 0U;
    size_t dropped = 0U;

    if (timeline == NULL) {
        return 0U;
    }

    for (size_t read_index = 0U;
         read_index < timeline->count;
         ++read_index) {
        if (timeline->events[read_index].type == type) {
            ++dropped;
            continue;
        }

        timeline->events[write_index] = timeline->events[read_index];
        ++write_index;
    }

    timeline->count = write_index;
    return dropped;
}

static int event_before(struct event a, struct event b)
{
    if (a.tick != b.tick) {
        return a.tick < b.tick;
    }
    return a.serial < b.serial;
}

static void timeline_sort_stable(struct timeline *timeline)
{
    if (timeline == NULL) {
        return;
    }

    for (size_t index = 1U; index < timeline->count; ++index) {
        const struct event key = timeline->events[index];
        size_t position = index;

        while ((position > 0U) &&
               event_before(key, timeline->events[position - 1U])) {
            timeline->events[position] = timeline->events[position - 1U];
            --position;
        }
        timeline->events[position] = key;
    }
}

static size_t timeline_count_type(const struct timeline *timeline,
                                  enum event_type type)
{
    size_t count = 0U;

    if (timeline == NULL) {
        return 0U;
    }

    for (size_t index = 0U; index < timeline->count; ++index) {
        if (timeline->events[index].type == type) {
            ++count;
        }
    }

    return count;
}

static uint32_t timeline_digest(const struct timeline *timeline)
{
    uint32_t hash = UINT32_C(2166136261);

    if (timeline == NULL) {
        return 0U;
    }

    for (size_t index = 0U; index < timeline->count; ++index) {
        const struct event *event = &timeline->events[index];

        hash ^= (uint32_t)(event->tick & UINT64_C(0xffffffff));
        hash *= UINT32_C(16777619);
        hash ^= (uint32_t)event->type;
        hash *= UINT32_C(16777619);
        hash ^= event->value;
        hash *= UINT32_C(16777619);
        hash ^= event->serial;
        hash *= UINT32_C(16777619);
    }

    return hash;
}

static void timeline_dump(const char *name, const struct timeline *timeline)
{
    if ((name == NULL) || (timeline == NULL)) {
        return;
    }

    printf("TIMELINE %s count=%zu digest=%08" PRIx32 "\n",
           name, timeline->count, timeline_digest(timeline));

    for (size_t index = 0U; index < timeline->count; ++index) {
        const struct event *event = &timeline->events[index];

        printf("  tick=%" PRIu64 " serial=%" PRIu32
               " type=%s value=%" PRIu32 "\n",
               event->tick,
               event->serial,
               event_name(event->type),
               event->value);
    }
}

int main(void)
{
    struct timeline baseline;
    struct timeline candidate;
    uint32_t before_digest;

    timeline_init(&baseline);
    assert(timeline_push(&baseline, UINT64_C(100), EV_RX,
                         UINT32_C(1)) == 0);
    assert(timeline_push(&baseline, UINT64_C(50), EV_TIMER,
                         UINT32_C(7)) == 0);
    assert(timeline_push(&baseline, UINT64_C(100), EV_TASK,
                         UINT32_C(2)) == 0);
    assert(timeline_push(&baseline, UINT64_C(20), EV_TRACE,
                         UINT32_C(9)) == 0);

    before_digest = timeline_digest(&baseline);
    timeline_dump("baseline insertion order", &baseline);

    candidate = baseline;
    assert(timeline_delay_type(&candidate, EV_RX, UINT64_C(75)) == 0);
    assert(timeline_duplicate_first(&candidate, EV_TIMER,
                                    UINT64_C(1)) == 0);
    assert(timeline_drop_type(&candidate, EV_TRACE) == 1U);
    timeline_sort_stable(&candidate);

    assert(timeline_count_type(&candidate, EV_RX) == 1U);
    assert(timeline_count_type(&candidate, EV_TIMER) == 2U);
    assert(timeline_count_type(&candidate, EV_TRACE) == 0U);
    assert(timeline_digest(&baseline) == before_digest);

    timeline_dump("delay RX + duplicate timer + drop trace", &candidate);
    puts("wizard runtime lab passed");
    return 0;
}
```

Compile:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -g -fsanitize=address,undefined \
    wizard_baseband/wizard_runtime_lab.c \
    -o build/wizard_runtime_lab
ASAN_OPTIONS=detect_leaks=0 ./build/wizard_runtime_lab
```

Expected cuối output:

```text
wizard runtime lab passed
```

### W.7.1. Những helper mới tự chế

Không sửa source chính ngay. Từ baseline trên, tự thêm từng helper:

```c
int timeline_move_one(struct timeline *t,
                      uint32_t serial,
                      uint64_t new_tick);

int timeline_duplicate_n(struct timeline *t,
                         enum event_type type,
                         size_t copies,
                         uint64_t spacing);

size_t timeline_drop_if(struct timeline *t,
                        int (*predicate)(const struct event *, void *),
                        void *ctx);

int timeline_assert_monotonic(const struct timeline *t);
```

Mỗi helper phải có failure atomicity: capacity thiếu hoặc tick overflow thì
timeline không được bị sửa một nửa.

---

## W.8. Bảng 36 trò nghịch thuần modem/baseband

Không làm hết một lúc. Chọn một trò, viết prediction, code một helper, chạy một
observer, rồi đóng experiment.

| # | Miền | Ý nghĩ quái dị | Helper nên tự viết | Observer |
|---:|---|---|---|---|
| 1 | IQ | Conjugate đúng nửa sau block | `iq_conjugate_range` | dump + energy |
| 2 | IQ | Rotate 90° chỉ sample index nguyên tố | `iq_rotate_if` | quadrant counter |
| 3 | IQ | Đảo thứ tự block nhưng giữ timestamp | `iq_reverse` | correlation offset |
| 4 | IQ | Clip riêng Q, giữ nguyên I | `iq_clip_axis` | saturation counter |
| 5 | IQ | Chèn impulse mỗi 17 sample | `iq_paint_impulses` | peak list |
| 6 | IQ | Zero một cửa sổ giữa waveform | `iq_zero_window` | gap detector |
| 7 | Constellation | Hoán đổi quadrant II và IV | `qam_remap_table` | exhaustive round trip |
| 8 | Constellation | Tạo vùng “không chắc” quanh trục | `qpsk_demap_soft` | erasure count |
| 9 | Constellation | Scale symbol theo bit pattern | `symbol_gain_by_label` | per-label energy |
| 10 | OFDM | Move bin `+3` sang `-3` | `grid_move_bin` | grid diff |
| 11 | OFDM | Copy một bin ra ba vị trí | `grid_fanout_bin` | occupied-bin count |
| 12 | OFDM | Notch đúng pilot | `grid_notch` | pilot-missing predicate |
| 13 | OFDM | Shift toàn band +2 | `grid_shift` | peak bin |
| 14 | OFDM | Mirror quanh DC | `grid_mirror` | conjugate-symmetry check |
| 15 | OFDM | Tạo comb mỗi 4 bin | `grid_paint_comb` | occupancy bitmap |
| 16 | OFDM | Làm bẩn guard đúng một bin | `grid_set_bin` | guard validator |
| 17 | OFDM | Gain dốc từ trái sang phải | `grid_apply_ramp` | per-bin magnitude |
| 18 | Channel | Delay thay đổi giữa hai frame | `channel_piecewise_delay` | sync peak |
| 19 | Channel | Phase jump tại sample 64 | `channel_phase_jump` | phase-difference trace |
| 20 | Channel | CFO chỉ bật trong một cửa sổ | `channel_cfo_window` | bin leakage metric |
| 21 | Channel | Burst noise deterministic | `channel_noise_window` | CRC + error positions |
| 22 | Channel | I gain khác Q gain | `channel_iq_imbalance` | constellation ellipse |
| 23 | Sync | Tạo hai PSS peak bằng nhau | `iq_overlay_pattern` | ranked peaks |
| 24 | Sync | Corrupt mỗi chip thứ 7 | `sequence_flip_stride` | correlation loss |
| 25 | Sync | Dò sai identity có chủ đích | `scan_all_pss` | score table |
| 26 | Runtime | Delay RX completion | `timeline_delay_type` | event order |
| 27 | Runtime | Duplicate timer event | `timeline_duplicate_first` | generation rejection |
| 28 | Runtime | Drop một IRQ nhưng giữ DMA done | `timeline_drop_if` | ownership leak check |
| 29 | Runtime | Fill ring theo burst 3-1-4 | `ring_push_pattern` | depth high-water mark |
| 30 | Memory | Reuse slot liên tục tới wrap generation | `pool_churn` | stale-handle rejects |
| 31 | MAC | Truncate PDU ở mọi offset | `buffer_prefix_view` | parser status histogram |
| 32 | MAC | Đổi length nhưng không đổi payload | `pdu_patch_u16` | atomic reject |
| 33 | HARQ | Làm LLR 0 ở một dải bit | `llr_zero_range` | combine recovery |
| 34 | RLC/PDCP | Tạo sequence quanh wrap | `sn_script_generate` | delivery order |
| 35 | Control | Link flapping đúng mỗi N tick | `event_repeat_pattern` | transition log |
| 36 | Integration | Hook mutation giữa hai stage | `pipeline_tap` | first-divergence stage |

---

## W.9. Công thức sinh vô hạn trò mới

Chọn một từ mỗi cột:

| Object | Động từ | Phạm vi | Điều kiện | Observer |
|---|---|---|---|---|
| IQ sample | rotate | một phần tử | index chẵn | dump |
| IQ block | reverse | một cửa sổ | energy > T | correlation |
| symbol | remap | mỗi N phần tử | label = 3 | round trip |
| bin | move | một band | trừ DC | grid diff |
| event | delay | một type | generation cũ | timeline |
| buffer | duplicate | qua queue | owner = DMA | ownership trace |
| PDU field | patch | đúng một byte | length boundary | parser status |
| state event | drop | một phase | đang CONNECTED | transition log |

Ví dụ random nhưng có nghĩa:

```text
Object    = bin
Động từ   = mirror
Phạm vi   = mỗi N phần tử
Điều kiện = trừ DC
Observer  = grid diff
```

Chuyển thành contract:

```c
int grid_mirror_stride(struct grid *grid,
                       size_t stride,
                       int preserve_dc);
```

Trước khi code, phải trả lời:

1. Mirror source/destination có đè nhau không?
2. Có cần temporary grid không?
3. Bin Nyquist được xử lý ra sao?
4. Fail có giữ nguyên grid không?
5. Property nào đúng khi gọi helper hai lần?

Một property rất mạnh:

```text
mirror(mirror(grid)) == grid
```

Đây là cách tự sinh test từ bản chất của phép biến đổi.

---

## W.10. Sáu cấp luyện “ý nghĩ → helper”

### Cấp 0 — Sửa literal

Bạn chỉ đổi:

```c
bins[3] = value;
```

Chưa có helper, chưa reusable.

### Cấp 1 — Helper một case

```c
void move_bin_3_to_minus_3(struct grid *grid);
```

Vụng nhưng có ranh giới.

### Cấp 2 — Parameter hóa

```c
int grid_move_bin(struct grid *grid, int from, int to);
```

### Cấp 3 — Contract + failure atomicity

Invalid bin không sửa grid. Destination policy được định nghĩa.

### Cấp 4 — Observer + property

```text
changed_bins == 2
energy preserved nếu destination ban đầu zero
```

### Cấp 5 — Composition

```text
shift → notch → move → dump
```

### Cấp 6 — Experiment engine

Mutation được truyền như callback, baseline tự clone, report tự sinh.

Không cần ở Cấp 6 cho mọi helper. Phù thủy giỏi biết lúc nào một helper nhỏ đã
đủ và lúc nào abstraction giúp mở ra nhiều experiment mới.

---

## W.11. Nhật ký bắt buộc của một trò nghịch

```markdown
## EXP-NNN — tên experiment

### Ý nghĩ ban đầu
Tôi muốn thay đổi gì?

### Representation
Byte, bit, symbol, bin, IQ sample, event, handle hay state?

### Baseline
Input nhỏ nhất và expected ban đầu.

### Dự đoán trước khi chạy
Những field/bin/sample nào đổi?
Những gì bắt buộc không đổi?

### Helper mới
Prototype + contract + owner.

### Observer
Dump/metric/digest/property nào được dùng?

### Actual
Output/trace thật.

### First divergence
Stage đầu tiên khác baseline.

### Giải thích
Vì sao actual khớp/không khớp prediction?

### Spell mới mở khóa
Từ kết quả này, helper hoặc experiment kế tiếp là gì?
```

Mục tiêu không phải có nhiều file. Mục tiêu là mỗi file ghi lại một lần bạn biến
suy nghĩ thành evidence.

---

## W.12. Capstone sáng tạo — Tự chế một Baseband Workbench

Không nhìn reference để chọn feature. Tự chọn ba nhóm:

### Nhóm A — Signal spell

Chọn ít nhất ba:

```text
IQ conjugate/range
phase jump
gain ramp
sample dropout
burst noise
constellation remap
```

### Nhóm B — Grid spell

Chọn ít nhất bốn:

```text
set/get logical bin
move/copy/swap
notch/band gain
shift/mirror
comb/checkerboard painter
guard/pilot validator
```

### Nhóm C — Runtime/protocol spell

Chọn ít nhất ba:

```text
delay/drop/duplicate event
queue pressure pattern
stale timer
stale handle
truncate PDU
sequence wrap script
invalid state sequence
```

Workbench CLI local có thể có dạng:

```text
./bbwizard grid set +3 1200 -400
./bbwizard grid move +3 -3
./bbwizard grid notch -1 +1
./bbwizard observe grid
./bbwizard run experiments/move_and_notch.txt
```

Không cần viết parser CLI phức tạp ngay. Phiên bản đầu có thể dùng array command
hard-code trong test. Thứ tự trưởng thành:

```text
hard-coded experiment
→ helper calls
→ command struct
→ script text local
→ reusable workbench
```

Definition of Done:

```text
[ ] Mọi mutation chạy trên baseline clone.
[ ] Có ít nhất 12 helper do chính bạn thiết kế.
[ ] Có dump + metric + digest + property oracle.
[ ] Invalid input không làm object bị sửa nửa chừng.
[ ] Fixed seed cho mọi noise/random pattern.
[ ] Có ít nhất 20 experiment logs.
[ ] Có một experiment mà kết quả ban đầu trái dự đoán.
[ ] Bạn giải thích được first divergence.
[ ] Bạn tự nghĩ được experiment thứ 21 mà tài liệu không gợi ý.
```

---

## W.13. Rubric “phù thủy baseband” 100 điểm

| Năng lực | Điểm |
|---|---:|
| Dịch câu hỏi mơ hồ thành representation + contract | 15 |
| Tự thiết kế helper nhỏ, rõ ownership và failure | 15 |
| Tạo baseline/tiny vector tính tay được | 10 |
| Viết observer/digest/diff/property | 15 |
| Thao tác IQ/constellation/grid bin theo ý | 15 |
| Tạo mutation cho channel/runtime/protocol | 10 |
| Compose helper mà vẫn debug được first divergence | 10 |
| Tự sinh experiment mới và ghi explanation | 10 |

Mốc:

```text
0–39   biết chạy code
40–59  biết sửa code có sẵn
60–74  tự viết helper từ yêu cầu rõ
75–89  tự biến ý nghĩ thành experiment có evidence
90–100 tự xây vocabulary/tooling để khám phá câu hỏi chưa có sẵn
```

Điểm 100 không có nghĩa biết hết 5G. Nó có nghĩa bạn không còn bị khóa vào những
bài đã có lời giải: gặp một câu hỏi mới trong phạm vi modem/baseband, bạn biết
cách chia nhỏ, code helper, quan sát và tự tiến về phía câu trả lời.

<!-- BASEBAND_WIZARD_LAB_END -->

---

# PHẦN XVIII — LỘ TRÌNH CAPSTONE TỪ 0 ĐẾN MODEM

## Capstone A — `iq101`

Mục tiêu: chứng minh đã chắc C cơ bản.

Feature:

```text
- struct cpx16
- IQ array
- saturation
- little-endian helpers
- IQ16 writer/reader
- defensive length checks
```

Done khi:

```text
write file → read file → samples identical
```

---

<!-- MODEM_CONCEPT_CAPSTONE_A -->
### Ý nghĩa modem/baseband của Capstone A

**Concept trung tâm:** IQ representation end-to-end.

**Bạn đang ghép cái gì?** gom parsing/printing/measurement của IQ thành một tool nhỏ.

**Vì sao capstone này tồn tại?** Bạn chứng minh mình hiểu sample, block, sample rate và file IQ như một data source gần với ADC/DAC ảo.

Checkpoint bắt buộc trước khi đi tiếp:

```text
Tôi phải vẽ được data flow của capstone này từ input đến output
và chỉ ra mỗi module đang sở hữu representation nào.
```


### Forge checkpoint A — IQ representation không được học thuộc một fixture

Tự code ba biến thể trước khi đi tiếp:

1. Round-trip block `count=0,1,8192` với I/Q chứa `0,±1,INT16_MIN,INT16_MAX`.
2. Cắt file tại mọi offset `0..16+count*4-1`; reader phải reject sạch.
3. Thêm trailing byte và sample-rate zero; out-view phải giữ nguyên khi fail.

<details>
<summary>Đáp án/harness checkpoint A</summary>

```c
static void verify_iq_case(const struct cpx16 *input,size_t count,uint32_t rate)
{
    uint8_t file[16U+8192U*4U];
    struct iq_view view={(uint32_t)77,UINT64_C(88),(const uint8_t*)1};
    size_t bytes=0U,k;
    assert(iq16_write_memory(file,sizeof file,rate,input,count,&bytes)==0);
    assert(iq16_view(file,bytes,&view)==0);
    assert(view.rate==rate&&view.count==count);
    for(k=0U;k<count;++k){int16_t i=0,q=0;assert(iq16_sample(&view,k,&i,&q)==0);
        assert(i==input[k].i&&q==input[k].q);}
    for(k=0U;k<bytes;++k)assert(iq16_view(file,k,&view)!=0);
    file[bytes]=0U;assert(iq16_view(file,bytes+1U,&view)!=0);
}
```

Đáp án đạt khi sanitizer sạch, reader không allocation theo count chưa validate,
và encode/decode không dùng `memcpy(struct)`.

</details>

## Capstone B — `modulation101`

Feature:

```text
QPSK
16-QAM
CRC24A
noise-free round trip
```

Done khi toàn bộ symbol round-trip pass.

---

<!-- MODEM_CONCEPT_CAPSTONE_B -->
### Ý nghĩa modem/baseband của Capstone B

**Concept trung tâm:** modulation chain.

**Bạn đang ghép cái gì?** bits ↔ QPSK/16-QAM symbols.

**Vì sao capstone này tồn tại?** Bạn kiểm tra round-trip mapping độc lập trước khi nhét modulation vào OFDM, nhờ đó lỗi constellation không bị lẫn với lỗi FFT.

Checkpoint bắt buộc trước khi đi tiếp:

```text
Tôi phải vẽ được data flow của capstone này từ input đến output
và chỉ ra mỗi module đang sở hữu representation nào.
```


### Forge checkpoint B — constellation phải chịu sweep và fault

Tự code:

1. QPSK round-trip mọi byte `0..255` ở cả MSB-first và LSB-first contract.
2. 16-QAM round-trip đủ 16 nibble ở ba amplitude hợp lệ.
3. Flip từng bit payload; CRC-24A phải reject, demapper vẫn không OOB.

<details>
<summary>Đáp án/harness checkpoint B</summary>

```c
for(unsigned value=0U;value<256U;++value){
    struct cpx16 s[4];uint8_t decoded=0U;
    assert(qpsk_byte((uint8_t)value,1,INT16_C(16384),s)==0);
    for(size_t k=0U;k<4U;++k){decoded|=(uint8_t)(hard_bit(s[k].i)<<(7U-2U*k));
        decoded|=(uint8_t)(hard_bit(s[k].q)<<(6U-2U*k));}
    assert(decoded==(uint8_t)value);
}
for(unsigned nibble=0U;nibble<16U;++nibble)
    assert(qam16_demap(qam16_map((uint8_t)nibble,INT16_C(4096)),
                        INT16_C(4096))==(uint8_t)nibble);
```

Nếu QPSK fixture không khớp, sửa mapper/demapper theo **một** bit-order contract;
không đảo expected riêng từng test.

</details>

## Capstone C — `ofdm101`

Feature:

```text
NFFT nhỏ trước: 16/32
DFT reference
FFT implementation
resource mapping
CP
IFFT → FFT round trip
```

Done khi constellation recover đúng trong channel lý tưởng.

---

<!-- MODEM_CONCEPT_CAPSTONE_C -->
### Ý nghĩa modem/baseband của Capstone C

**Concept trung tâm:** OFDM transceiver.

**Bạn đang ghép cái gì?** resource bins → IFFT → CP → FFT → bins.

**Vì sao capstone này tồn tại?** Đây là xương sống waveform. Sau capstone này bạn phải nhìn được ranh giới frequency-domain và time-domain.

Checkpoint bắt buộc trước khi đi tiếp:

```text
Tôi phải vẽ được data flow của capstone này từ input đến output
và chỉ ra mỗi module đang sở hữu representation nào.
```


### Forge checkpoint C — OFDM không được chỉ pass một NFFT

Tự code ba matrix:

1. `NFFT=16,32,64`, impulse/DC/tone/random deterministic.
2. CP length `0,1,N/4,N/2`, add→remove phải identical.
3. Bins âm/DC/dương/guard; IFFT→FFT phục hồi occupied bins trong tolerance.

<details>
<summary>Đáp án/harness checkpoint C</summary>

```c
static const size_t nffts[]={16U,32U,64U};
for(size_t ni=0U;ni<sizeof nffts/sizeof nffts[0];++ni){
    size_t n=nffts[ni];
    assert(is_pow2(n));
    for(size_t cp=0U;cp<=n/2U;cp+=(cp==0U?1U:n/4U)){
        /* fill deterministic bins, IFFT, add_cp, remove_cp, FFT */
        /* compare occupied bins with abs+relative tolerance and first_bad */
        assert(ofdm_round_trip_case(n,cp,UINT32_C(0xC017)+ni)==0);
        if(cp==1U)cp=n/4U-1U;
    }
}
```

`ofdm_round_trip_case` phải trả stage + first bad bin khi fail. Đáp án không phải
một assert khổng lồ chỉ báo “OFDM failed”.

</details>

## Capstone D — `channel101`

Feature:

```text
delay
noise
CFO
fixed seed
saturation
```

Done khi cùng seed tạo byte-identical waveform.

---

<!-- MODEM_CONCEPT_CAPSTONE_D -->
### Ý nghĩa modem/baseband của Capstone D

**Concept trung tâm:** radio impairment.

**Bạn đang ghép cái gì?** đưa delay/noise/CFO vào giữa TX và RX.

**Vì sao capstone này tồn tại?** Receiver chỉ có ý nghĩa khi channel có thể làm hỏng tín hiệu. Capstone này biến demo perfect-loopback thành communication problem thật sự.

Checkpoint bắt buộc trước khi đi tiếp:

```text
Tôi phải vẽ được data flow của capstone này từ input đến output
và chỉ ra mỗi module đang sở hữu representation nào.
```


### Forge checkpoint D — channel phải deterministic cả khi chia block

Tự code:

1. Cùng seed/config chạy hai lần cho IQ byte-identical.
2. Xử lý 1024 sample một block và 4×256 sample phải identical khi state liên tục.
3. Đổi thứ tự delay→CFO→noise thành noise→CFO→delay; test phải thấy digest khác.

<details>
<summary>Đáp án/harness checkpoint D</summary>

```c
struct channel_config cfg={.seed=UINT32_C(0x5400),.delay=17U,.cfo_hz=1800,.noise=7U};
struct channel_state a,b,c;
channel_init(&a,&cfg);channel_init(&b,&cfg);channel_init(&c,&cfg);
assert(channel_apply(&a,input,whole,1024U)==0);
assert(channel_apply(&b,input,chunked+0U,256U)==0);
assert(channel_apply(&b,input+256U,chunked+256U,256U)==0);
assert(channel_apply(&b,input+512U,chunked+512U,256U)==0);
assert(channel_apply(&b,input+768U,chunked+768U,256U)==0);
assert(memcmp(whole,chunked,sizeof whole)==0);
channel_set_order(&c,CHANNEL_NOISE_CFO_DELAY);
assert(channel_apply(&c,input,reordered,1024U)==0);
assert(memcmp(whole,reordered,sizeof whole)!=0);
```

PRNG, oscillator phase và delay history đều thuộc `channel_state`, không global.

</details>

## Capstone E — `sync101`

Feature:

```text
known sync sequence
correlation
timing estimate
identity search
```

Done khi tìm đúng offset trong nhiều test delay.

---

<!-- MODEM_CONCEPT_CAPSTONE_E -->
### Ý nghĩa modem/baseband của Capstone E

**Concept trung tâm:** cell/signal synchronization.

**Bạn đang ghép cái gì?** reference sequence + correlation + timing/CFO estimate.

**Vì sao capstone này tồn tại?** UE mới bật không biết waveform bắt đầu ở đâu. Đây là khối giúp nó tìm được tín hiệu trước khi decode payload.

Checkpoint bắt buộc trước khi đi tiếp:

```text
Tôi phải vẽ được data flow của capstone này từ input đến output
và chỉ ra mỗi module đang sở hữu representation nào.
```


### Forge checkpoint E — sync phải có success-rate và false-peak evidence

Tự code:

1. Sweep delay `0..64` cho cả ba `N_ID2` trong channel sạch.
2. Chạy 100 seed ở ba noise level, in success rate bằng `%zu/%u` đúng type.
3. Chèn hai PSS peak bằng nhau; tie policy first-win phải deterministic.

<details>
<summary>Đáp án/harness checkpoint E</summary>

```c
for(unsigned id=0U;id<3U;++id)
    for(size_t delay=0U;delay<=64U;++delay){
        size_t got_delay=SIZE_MAX;unsigned got_id=99U;
        make_pss_trial(id,delay,0U,UINT32_C(1),waveform,WAVE_CAP);
        assert(pss_detect(waveform,WAVE_CAP,64U,&got_id,&got_delay)==0);
        assert(got_id==id&&got_delay==delay);
    }
for(unsigned level=0U;level<3U;++level){
    uint32_t ok=0U,histogram[65];
    assert(pss_sweep(run_pss_trial,&fixture,(int)level,&ok,histogram)==0);
    printf("noise=%u success=%" PRIu32 "/100\n",level,ok);
}
```

Fixture literal độc lập là bắt buộc; generator và detector dùng chung bug có thể
tự đồng ý với nhau 100/100.

</details>

## Capstone F — `runtime101`

Feature:

```text
SPSC ring
buffer pool + generation
virtual tick
timer generation
IRQ → deferred task
```

Done khi queue full/stale handle/late timer đều bị reject sạch.

---

<!-- MODEM_CONCEPT_CAPSTONE_F -->
### Ý nghĩa modem/baseband của Capstone F

**Concept trung tâm:** firmware execution model.

**Bạn đang ghép cái gì?** event/ring/pool/timer/scheduler.

**Vì sao capstone này tồn tại?** Bạn chuyển từ DSP offline sang firmware modem: data đến bất đồng bộ, resource bounded, công việc phải hoàn tất trước deadline.

Checkpoint bắt buộc trước khi đi tiếp:

```text
Tôi phải vẽ được data flow của capstone này từ input đến output
và chỉ ra mỗi module đang sở hữu representation nào.
```


### Forge checkpoint F — runtime phải chứng minh recovery sau pressure

Tự code ba scenario:

1. Fill queue tới full, một push fail, drain hai, push lại và giữ FIFO.
2. Release/reacquire slot rồi deliver stale handle từ event cũ; resolver reject.
3. Arm timer A rồi B; callback A đến cùng tick với IRQ, state chỉ chịu B theo order.

<details>
<summary>Đáp án scenario checkpoint F</summary>

```c
static const struct runtime_step script[]={
    {0U,STEP_FILL_QUEUE,QUEUE_CAPACITY},
    {0U,STEP_EXPECT_PUSH_FAIL,1U},
    {1U,STEP_DRAIN,2U},
    {1U,STEP_EXPECT_PUSH_OK,2U},
    {2U,STEP_RELEASE_REACQUIRE,0U},
    {3U,STEP_DELIVER_STALE,0U},
    {4U,STEP_ARM_TIMER_A,10U},
    {5U,STEP_ARM_TIMER_B,10U},
    {10U,STEP_POST_IRQ,8U},
    {10U,STEP_DELIVER_TIMER_A,0U},
    {10U,STEP_DELIVER_TIMER_B,0U}
};
assert(runtime_run_script(&rt,script,sizeof script/sizeof script[0])==0);
assert(rt.dropped==1U&&rt.stale_rejected==1U&&rt.timer_fired==1U);
assert(!rt.supervisor_fault&&pool_all_returned(&rt.pool));
```

Đáp án phải kèm trace order, không chỉ counters cuối.

</details>

## Capstone G — `l2_101`

Feature:

```text
MAC mux/demux
HARQ process table
RLC reorder
PDCP anti-replay/reorder
```

Done khi duplicate/reorder/wraparound tests pass.

---

<!-- MODEM_CONCEPT_CAPSTONE_G -->
### Ý nghĩa modem/baseband của Capstone G

**Concept trung tâm:** Layer-2 data plane.

**Bạn đang ghép cái gì?** MAC/RLC/PDCP stateful processing.

**Vì sao capstone này tồn tại?** Bạn đưa payload từ transport block thành bearer data có sequence/reorder/reliability semantics, thay vì coi PHY block là application packet.

Checkpoint bắt buộc trước khi đi tiếp:

```text
Tôi phải vẽ được data flow của capstone này từ input đến output
và chỉ ra mỗi module đang sở hữu representation nào.
```


### Forge checkpoint G — L2 phải đi qua wrap, reorder và interleaved HARQ

Tự code:

1. RLC receive `4094,0,4095,1`: release phải `4094,4095,0,1`.
2. HARQ id 2 và 5 retransmit xen kẽ; soft buffer không lẫn.
3. PDCP accept ahead, delayed-in-window, duplicate và too-old với reason riêng.

<details>
<summary>Đáp án scenario checkpoint G</summary>

```c
static const uint16_t arrival[]={4094U,0U,4095U,1U};
static const uint16_t expected[]={4094U,4095U,0U,1U};
struct release_log log={0};
rlc_set_next(&rlc,4094U);
for(size_t k=0U;k<4U;++k)assert(rlc_receive(&rlc,arrival[k],payload[k],&log)>=0);
assert(log.count==4U&&memcmp(log.sn,expected,sizeof expected)==0);

assert(harq_begin(&harq,2U,100U,NLLR)==0);
assert(harq_begin(&harq,5U,200U,NLLR)==0);
assert(harq_combine(&harq,2U,100U,llr_a,NLLR)==0);
assert(harq_combine(&harq,5U,200U,llr_b,NLLR)==0);
assert(memcmp(harq.p[2].llr,harq.p[5].llr,sizeof harq.p[2].llr)!=0);
```

Parser/integrity phải validate trước mọi mutation của ba context trên.

</details>

## Capstone H — `control101`

Feature:

```text
RRC state machine
NAS state machine
security-job async stub
link-loss/recovery
```

Done khi invalid events không mutate state.

---

<!-- MODEM_CONCEPT_CAPSTONE_H -->
### Ý nghĩa modem/baseband của Capstone H

**Concept trung tâm:** control plane.

**Bạn đang ghép cái gì?** RRC/NAS state + timer + security event.

**Vì sao capstone này tồn tại?** Bạn mô hình hóa việc UE thực sự search/camp/connect/register/session thay vì đặt thẳng một cờ `connected=true`.

Checkpoint bắt buộc trước khi đi tiếp:

```text
Tôi phải vẽ được data flow của capstone này từ input đến output
và chỉ ra mỗi module đang sở hữu representation nào.
```


### Forge checkpoint H — control plane không được “set cờ cho xong”

Tự code:

1. Mọi event của valid attach path, rồi permute từng event sai state; context giữ nguyên.
2. Security completion sai cookie/generation không được chuyển RRC/NAS.
3. Link-loss ở từng state phải về policy recovery được công bố, timer cũ vô hiệu.

<details>
<summary>Đáp án scenario checkpoint H</summary>

```c
static const enum event attach[]={EV_POWER,EV_FOUND,EV_CAMP_OK,EV_SETUP,
                                  EV_SECURITY_OK,EV_REGISTER_OK,EV_SESSION_OK};
for(size_t cut=0U;cut<sizeof attach/sizeof attach[0];++cut){
    struct control c=CONTROL_INITIALIZER;
    for(size_t k=0U;k<cut;++k)assert(control_post(&c,attach[k])==0);
    struct control before=c;
    assert(control_post(&c,EV_SESSION_OK)!=0);
    assert(control_equal(&c,&before));
}
assert(control_attach_path(attach,sizeof attach/sizeof attach[0])==STATE_DATA);
assert(control_security_done(&ctl,wrong_cookie,old_generation,tag)!=0);
assert(ctl.rrc==before_rrc&&ctl.nas==before_nas);
```

`control_equal` so sánh field logic canonical, không `memcmp` padding/raw pointer.

</details>

## Capstone I — `modem101`

Ghép toàn bộ:

```text
NetworkPeer control/data
      ↓
virtual RF waveform
      ↓
SoC simulated RX DMA
      ↓
IRQ
      ↓
PHY receive
      ↓
MAC
      ↓
RLC
      ↓
PDCP
      ↓
RRC/NAS/session
```

Scenario cuối:

```text
boot
→ cell search
→ camp
→ RRC connect
→ security
→ registration
→ PDU session
→ transfer 4096-byte SDU
→ force one failed codeword / HARQ retry
→ replay packet must be rejected
→ link loss
→ recamp/reconnect
→ DATA again
```

Đây mới là thời điểm hợp lý để đọc project solution lớn.

---

<!-- MODEM_CONCEPT_CAPSTONE_I -->
### Ý nghĩa modem/baseband của Capstone I

**Concept trung tâm:** full modem integration.

**Bạn đang ghép cái gì?** NetworkPeer ↔ waveform ↔ UE full stack.

**Vì sao capstone này tồn tại?** Đây là bài chứng minh toàn hệ thống: payload và control phải đi qua waveform và các layer, recovery/HARQ/replay/link-loss đều thể hiện bằng state/trace chứ không bằng shortcut.

Checkpoint bắt buộc trước khi đi tiếp:

```text
Tôi phải vẽ được data flow của capstone này từ input đến output
và chỉ ra mỗi module đang sở hữu representation nào.
```





### Forge checkpoint I — ba biến thể E2E trước khi được mở ZIP reference

Tự code:

1. Baseline đúng đề: CFO `+1,8 kHz`, PCI 42, 4096 byte, một erased codeword.
2. Đổi modulation/data segmentation nhưng vẫn buộc control và payload đi qua IQ.
3. Inject từng fault riêng: truncated IQ, malformed PDU, late timer, queue full,
   restart PHY/L2; mọi run phải deterministic và không crash/hang/UAF/OOB.

<details>
<summary>Đáp án checkpoint I — acceptance harness bám lời giải 100/100</summary>

```c
struct acceptance {
    int registered,session,payload_equal,harq_retx,llr_combined;
    int replay_drop,rlc_reordered,pdcp_reordered,recovered;
    uint64_t delivered,crc_fail;
};
static int pass(const struct acceptance*a)
{
    return a->registered&&a->session&&a->payload_equal&&a->harq_retx&&
           a->llr_combined&&a->replay_drop&&a->rlc_reordered&&
           a->pdcp_reordered&&a->recovered&&a->delivered==4096U&&
           a->crc_fail>=1U;
}

for(size_t scenario=0U;scenario<SCENARIO_COUNT;++scenario){
    struct run_result first=run_modem(scenario,UINT32_C(0xC017));
    struct run_result second=run_modem(scenario,UINT32_C(0xC017));
    assert(pass(&first.acceptance));
    assert(runs_identical(&first.evidence,&second.evidence));
    assert(trace_ticks_monotonic(first.trace,first.trace_len));
}
```

Đích baseline phải tương đương evidence của ZIP 100/100:

```text
MODEM_RESULT status=PASS state=DATA tx_bytes=4096 rx_bytes=4096 crc_fail=1 recoveries=2
Release + ASan/UBSan + -fanalyzer: toàn bộ test pass
Cortex-R5 ELF: audit ABI/undefined/W^X/.ARM.exidx + QEMU boot-smoke pass
```

Không hard-code stdout/trace, không side-channel payload, không để NetworkPeer
sửa thẳng state UE. Evidence phải phát sinh từ execution thật.

</details>

## Capstone J — `rebuild_modem101_from_blank`: tự code lại toàn bộ đáp án

Đây là **bài tốt nghiệp thật sự của khóa**.

Capstone I chứng minh rằng bạn ghép được các block thành modem end-to-end. Capstone J buộc bạn chứng minh một chuyện khó hơn:

> **Đóng ZIP đáp án. Tạo repo trắng. Tự dựng lại project tương đương từ contract và mental model đã học.**

Không yêu cầu source giống từng dòng reference. Yêu cầu là bạn tự tái tạo được toàn bộ hệ thống với cùng lớp functionality và các invariant quan trọng.

### Forge checkpoint J — ba lần rebuild, ba mức tự do

1. **Rebuild A:** được nhìn public contract + danh sách test, không nhìn source.
2. **Rebuild B:** chỉ nhìn đề `01_mini_ue_end_to_end.md` và artifact của chính A;
   đổi module boundary ít nhất một chỗ nhưng giữ behavior/evidence.
3. **Rebuild C:** thêm một feature tự nghĩ xuyên PHY/runtime/L2/control/trace,
   kèm fault injection; sau đó chạy lại toàn verification matrix baseline.

<details>
<summary>Đáp án/rubric kề bên checkpoint J</summary>

Không có một source text duy nhất cho B/C. Đáp án là bộ bằng chứng executable:

```text
strict C17 host build                       PASS
ASan + UBSan                               PASS
GCC -fanalyzer                             PASS
unit_q1/q2/runtime/l2/defensive/mac         PASS
integration_full/truncated/determinism/IQ  PASS
Arm ELF audit + Cortex-R5 boot-smoke        PASS
payload 4096 byte byte-for-byte             PASS
HARQ RETX/combine + replay drop             PASS
link-loss → recamp/reconnect → DATA         PASS
same seed → stdout/IQ/JSONL identical       PASS
```

Rebuild A có thể đối chiếu ZIP 100/100 theo file map ở các milestone ngay dưới.
Rebuild B/C đúng khi test và invariant tương đương dù source/kiến trúc chi tiết
khác reference; hard-code output hay side-channel là fail tuyệt đối.

</details>

### 1. Definition of Graduation

Bạn chỉ tốt nghiệp khóa khi có thể nói thật:

```text
Tôi có thể xóa project vừa làm,
mở một thư mục trắng,
và tự dựng lại một mini UE end-to-end tương đương reference.
```

Không đủ nếu chỉ:

```text
đọc hiểu source reference
copy lại source reference
nhớ tên hàm và thứ tự file
```

Phải đạt:

```text
HIỂU CONTRACT
→ TỰ THIẾT KẾ
→ TỰ CODE
→ TỰ TEST
→ TỰ DEBUG
→ TỰ TÍCH HỢP
→ TỰ REBUILD TOÀN BỘ
```

### 2. Repo trắng phải tự tạo được

Từ một thư mục trống, tự dựng cấu trúc tương đương:

```text
CMakeLists.txt
cmake/
    arm-none-eabi-toolchain.cmake

arch/
    armv7r/
        startup_armv7r.S
        linker.ld

include/
    bb/
        public API / contracts

src/
    fw/
        codec/
        channel/
        phy/
        memory/
        runtime/
        l2/
        control/
        security/
        ipc/
        trace/
    hal/
    soc/
    host/

tests/
```

Tên file và tên hàm có thể khác đáp án. Những thứ không được khác về bản chất:

```text
payload đi thật qua waveform
không side-channel
memory/resource bounded
virtual time deterministic
ownership rõ
state transition có guard
malformed input không crash
host + Arm dùng chung firmware core
```

### 3. Phase A — build system + public API trước

Đầu tiên chỉ cần:

```text
project compile/link sạch
host executable chạy được
public types/API được định nghĩa
```

Tự định nghĩa lại các concept:

```text
complex IQ sample
IQ block
buffer handle
event
stats
platform ops
RF backend
modem init/post/irq/run
SoC init/step
```

Với mỗi type/API phải trả lời:

```text
input là gì?
output là gì?
owner là ai?
lifetime bao lâu?
wire hay local-only?
endianness?
valid range?
```

### 4. Phase B — codec/channel/PHY từ thấp lên cao

Không code `phy_receive()` khổng lồ ngay.

Đi theo dependency:

```text
sat16
→ complex arithmetic
→ CRC-24A
→ QPSK
→ 16-QAM
→ FFT/IFFT
→ CP
→ resource grid
→ virtual channel
→ PSS
→ timing/CFO
→ equalization
→ demap/decode
```

Gate cuối Phase B:

```text
bytes
→ CRC
→ modulation
→ OFDM waveform
→ channel
→ sync
→ FFT/equalize/demap
→ CRC
→ same bytes
```

Không được dùng biến chung để đưa payload từ TX sang RX.

### 5. Phase C — runtime/memory

Tự viết và test:

```text
arena
buffer pool
generation-tagged handle
SPSC ring
event queue
virtual timer
scheduler
IRQ top-half contract
trace/counter
```

Malformed/edge cases bắt buộc:

```text
queue empty
queue full
wraparound
stale handle
stale timer generation
tick wrap
deadline miss
```

### 6. Phase D — HAL/SoC/MMIO/DMA

Tự tái tạo hardware-like boundary:

```text
MMIO read/write
DMA descriptor
descriptor validation
DMA ownership
cache hooks
IRQ status/ack
RF RX/TX completion
CRYPTO completion
IPC doorbell
```

Mental model:

```text
CPU fill descriptor
→ publish OWN
→ doorbell
→ simulated engine
→ DONE/IRQ
→ deferred firmware work
```

### 7. Phase E — MAC → RLC → PDCP

MAC:

```text
multiplex/demultiplex
LCID validation
BSR
HARQ processes
HARQ timer
soft-combine model
```

RLC-UM:

```text
12-bit SN
modular comparison
window
reorder
duplicate/out-of-window
timeout
```

PDCP:

```text
18-bit SN
HFN/COUNT
reorder
anti-replay
duplicate rejection
```

Gate:

```text
payload
→ MAC
→ RLC
→ PDCP
→ reorder/replay tests
→ exact payload
```

### 8. Phase F — RRC/NAS + security

RRC phải tự đi qua state hợp lệ:

```text
OFF
→ SEARCHING
→ CAMPED
→ CONNECTING
→ CONNECTED
```

NAS:

```text
DEREGISTERED
→ REGISTERING
→ REGISTERED
→ SESSION_ACTIVE
```

Mỗi transition:

```text
FROM
EVENT
GUARD
ACTION
TO
```

Security phải đi qua async hardware-like path:

```text
submit crypto job
→ descriptor/MMIO
→ engine
→ IRQ
→ continuation
```

Bad integrity, replay, stale timer và wrong-state event không được mutate state sai.

### 9. Phase G — tự viết `NetworkPeer`

Peer chỉ được giao tiếp qua protocol/waveform:

```text
NetworkPeer
→ encode/modulate
→ IQ
→ RF backend
→ SoC/DMA/IRQ
→ PHY
→ MAC/RLC/PDCP
→ RRC/NAS hoặc user data
```

Cấm:

```text
set thẳng UE state
chọc pointer vào UE
copy payload qua global variable
hard-code delivered bytes
```

### 10. Phase H — scenario cuối tương đương đáp án

Final lifecycle:

```text
BOOT
→ cell search
→ camp
→ RRC setup
→ security
→ registration
→ PDU session
→ transfer SDU 4096 byte
→ force một codeword fail
→ HARQ RETX
→ reorder case
→ replay rejection
→ link loss
→ recamp/reconnect
→ DATA
```

Definition of Done:

```text
SDU byte-for-byte đúng
có CRC fail có chủ đích
có HARQ retransmission
replay bị drop
link-loss recovery thành công
final state trở lại DATA
```

Reference hiện tại kết thúc bằng dạng:

```text
MODEM_RESULT status=PASS state=DATA tx_bytes=4096 rx_bytes=4096 crc_fail=1 recoveries=2
```

Không hard-code output này. Counters phải sinh ra từ execution thật.

### 11. Phase I — Armv7-R target

Sau khi host core pass mới tự dựng:

```text
vector table
reset entry
mode stacks
.data copy
.bss zero
linker memory regions
freestanding link
```

Phải tự build được target và đọc được:

```text
ELF size
map file
undefined symbols
segment permissions
```

### 12. Phase J — verification gate

Rebuild chỉ hoàn thành khi có test tương đương các nhóm của reference:

```text
unit PHY/codec
unit security/protocol
unit runtime
unit L2
unit MAC
defensive malformed-input tests
full integration
truncated IQ rejection
deterministic replay
IQ file round-trip
Arm ELF audit
Arm smoke boot
```

Ít nhất phải chạy:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Sau đó chạy sanitizer và deterministic replay.

Cùng config/seed/events phải cho:

```text
same final state
same counters
same IQ output
same trace
```

### 13. Closed-book rebuild lần hai

Đây là bài kiểm tra cuối cùng.

Tạo repo mới lần nữa:

```text
modem101_rebuild_closed_book/
```

Không copy source từ lần rebuild đầu.

Chỉ được dùng:

```text
đề bài
notes do chính mình viết
spec được phép
compiler/debugger
test vectors
```

Không mở ZIP đáp án và không mở full solution của khóa.

Nếu bí, quay lại **concept**, không quay lại source solution ngay.

Mục tiêu là chuyển từ:

```text
"tôi nhớ code trông như thế nào"
```

sang:

```text
"tôi hiểu contract nên tự suy ra code"
```

### 14. Khi nào mới mở lại đáp án?

Chỉ sau khi rebuild của bạn đã:

```text
build sạch
unit tests chính pass
E2E đi qua waveform
malformed input không crash
deterministic
```

Lúc đó mới mở reference để review kiến trúc.

Mỗi module viết bảng tự đánh giá:

```text
Cách mình thiết kế:
...

Cách reference thiết kế:
...

Invariant giống nhau:
...

Điểm mình thiếu:
...

Điểm mình khác nhưng vẫn đúng:
...
```

Reference lúc này là **review material**, không còn là source để copy.

---

<!-- SOURCE_READING_MAP_V2 -->
# PHẦN XVIII-B — ĐỌC PROJECT MINI UE MÀ KHÔNG COPY MÙ

File ZIP đi kèm có các module gần đúng với mental model của khóa. Không đọc `main.c` rồi cố nuốt toàn bộ. Đọc theo dependency và luôn trả lời “representation hiện tại là gì?”.

```text
include/bb/bb.h
    ↓ public types / API / contracts
src/fw/codec/codec.c
    ↓ CRC / FFT-adjacent primitives
src/fw/channel/channel.c
    ↓ virtual RF impairment
src/fw/phy/phy.c
    ↓ IQ ↔ transport block
src/fw/memory/memory.c
    ↓ pool / arena concepts
src/fw/runtime/runtime.c
    ↓ event / ring / timer / scheduler
src/fw/l2/mac.c
src/fw/l2/rlc_um.c
src/fw/l2/pdcp.c
src/fw/l2/protocol.c
    ↓ L2 data semantics
src/fw/control/state_machine.c
src/fw/control/transitions.c
    ↓ RRC/NAS state semantics
src/fw/security/security.c
    ↓ crypto job / security context
src/hal/mmio.c
src/soc/dma_descriptor.c
src/soc/soc.c
    ↓ CPU ↔ simulated hardware
src/host/network_peer.c
src/host/main.c
    ↓ integration / scenario / virtual RF
arch/armv7r/startup_armv7r.S
arch/armv7r/linker.ld
    ↓ target boot / memory image
```

Với mỗi file, không hỏi ngay “code này làm gì từng dòng?”. Hỏi theo thứ tự:

```text
1. File này thuộc layer nào?
2. Input representation là gì?
3. Output representation là gì?
4. Ai gọi nó?
5. Nó gọi/đẩy event sang ai?
6. Nó sở hữu buffer nào?
7. Nó phụ thuộc clock/state nào?
8. Failure được báo bằng return code, counter hay event nào?
```

Ví dụ khi đọc `phy.c`, trước khi đọc implementation hãy vẽ:

```text
TX TB → bits → symbols → OFDM → IQ
RX IQ → sync/FFT → symbols → bits → TB + CRC status
```

Sau đó mới tìm từng hàm tương ứng trong source. Làm như vậy sẽ biến source thành **bản implementation của một concept đã biết**, thay vì biến source thành thứ bạn phải học thuộc.

---

# PHẦN XVIII-C — BẢN ĐỒ “BÀI NÀY CODE ĐỂ LÀM GÌ?”

Phần này là bản đồ đối chiếu, không phải lời giải. Chỉ mở nó **sau khi đã đọc
phần Ý nghĩa modem/baseband ngay dưới bài**. Cột cuối giúp nối bài toy với source
đáp án; nó không có nghĩa là copy hàm đó về làm bài.

| Bài | Bạn đang luyện C | Bạn thực sự đang xây trong modem | Output được dùng tiếp bởi | Đối chiếu trong reference |
|---:|---|---|---|---|
| 1 | `struct`, initializer, `printf` | một complex IQ sample | IQ buffer, DSP primitive | `include/bb/bb.h`: `bb_cpx16` |
| 2 | array, `for`, hàm | một block waveform và phép đo thô | AGC/sync/channel | `bb_iq_block`; DMA sample arrays trong `internal.h` |
| 3 | pointer, in-place update | DSP sửa sample tại chỗ | gain/CFO/equalization | pattern xử lý sample trong `channel.c`, `phy.c` |
| 4 | pointer + `count`, bounds | contract của mọi sample buffer | mọi fast path PHY | các API `(samples,count)` và `bb_iq_block.count` |
| 5 | integer width, clamp | saturation fixed-point | FFT/NCO/QAM/channel | `codec.c`: `bb_sat16` |
| 6 | shift, mask, endian | wire representation tường minh | PDU/IPC/DMA/file parser | `protocol.c`, `ipc.c`, `dma_descriptor.c` |
| 7 | arithmetic an toàn | quan hệ sample count–sample rate–time | scheduler/deadline | `bb_iq_block`, virtual tick trong `runtime.c` |
| 8 | hàm mapper | 2 bit → QPSK symbol | resource mapper | `codec.c`: `bb_qpsk_map` |
| 9 | out-pointer, round trip | QPSK symbol → 2 bit | decoder/CRC | `codec.c`: `bb_qpsk_demap` |
| 10 | lookup/branch/test | 4 bit ↔ 16-QAM symbol | PHY modulation profile | `bb_qam16_map`, `bb_qam16_demap` |
| 11 | byte loop, bit polynomial | kiểm tra transport block bị lỗi | HARQ ACK/NACK | `codec.c`: `bb_crc24a` |
| 12 | nested loop, complex math | hiểu frequency bins ↔ time samples | FFT/IFFT | bài cầu nối; reference dùng FFT thay DFT trực tiếp |
| 13 | widen, multiply, shift | complex multiply Q15 | twiddle/NCO/equalizer | `codec.c`: `complex_q15` |
| 14 | array transform, fixture test | FFT/IFFT fixed-point | OFDM TX/RX | `codec.c`: `bb_fft_fixed` |
| 15 | copy theo vùng, bounds | add/remove cyclic prefix | virtual channel / FFT RX | `phy.c`: phát symbol và `extract_fft_symbol` |
| 16 | index mapping | data/pilot/PSS → resource bins | IFFT ở TX, equalizer ở RX | `map_data`, `map_pilot`, `carrier_bin` |
| 17 | offset, zero-fill | propagation/timing delay | synchronizer | `channel.c`: `bb_channel_apply` |
| 18 | trạng thái lặp, complex rotation | oscillator cho CFO và bù CFO | channel/sync | `bb_oscillator_init`, `bb_oscillator_step` |
| 19 | sliding window, score | correlation tìm mốc thời gian | PSS detector | `phy.c`: `find_pss` |
| 20 | sequence generator | ba PSS dài 127 và cell identity | cell search | `phy.c`: `make_pss`, `find_pss` |
| 21 | ghép pipeline | bytes → OFDM IQ → bytes | PHY boundary với runtime | `bb_phy_transmit`, `bb_phy_receive_soft_meta` |
| 22 | `enum`, tagged message | event mà runtime có thể xếp lịch | IRQ/task dispatcher | `bb_event`, `bb_internal_event_type` |
| 23 | circular indices | queue bounded single-thread | atomic SPSC | `runtime.c`: `bb_ring_push/pop/peek` |
| 24 | C11 atomics | publish/consume giữa execution contexts | IRQ top-half/deferred task | `bb_event_ring` + acquire/release trong `runtime.c` |
| 25 | slot table, generation | buffer identity chống stale handle | task/DMA/AP ownership | `memory.c`: `bb_pool_*` |
| 26 | alignment, offset | init toàn modem từ arena cố định | no-heap firmware init | `bb_modem_required_memory`, `bb_modem_init` |
| 27 | accessor boundary | MMIO register access có kiểm tra | SoC accelerator model | `hal/mmio.c` |
| 28 | enum state + validator | DMA ownership và descriptor lifecycle | IRQ completion/deferred PHY | `runtime.c`, `dma_descriptor.c`, `soc.c` |
| 29 | serialize nhiều SDU | MAC multiplex theo LCID | PHY transport block | `mac.c`: `bb_mac_multiplex` |
| 30 | defensive parser | MAC demultiplex atomic/canonical | RLC/control queues | `mac.c`: `bb_mac_demultiplex` |
| 31 | signed LLR + saturation | HARQ Chase combining | lần decode retransmission | `bb_harq_process`, `harq_store_soft`, `bb_mac_harq_accept` |
| 32 | modular arithmetic | RLC-UM reorder theo SN 12-bit | PDCP in-order input | `rlc_um.c` |
| 33 | bitmap/window | PDCP COUNT/HFN/reorder/anti-replay | application/control delivery | `pdcp.c` |
| 34 | `enum`, transition table | RRC state machine có guard | NAS/session/recovery | `transitions.c`, `state_machine.c` |
| 35 | generation + virtual tick | timer cũ không phá context mới | HARQ/RLC/PDCP/recovery | `bb_tick_before/due`; timer generation trong L2/runtime |
| 36 | async job state | key handle + crypto accelerator/IRQ | integrity-before-state | `security.c`, CRYPTO path trong `soc.c` |
| 37 | function pointer interface | platform clock/MMIO/cache shim | cùng core chạy host và Arm | `bb_platform_ops` |
| 38 | stable ordering | deterministic scheduler theo virtual time | mọi task/state transition | `runtime.c`: `bb_modem_run_until`, task queues |
| 39 | file I/O + parser bounds | container IQ16 ở host boundary | virtual RF input | `src/host/main.c`: IQ reader/writer |
| 40 | đọc ELF/map | memory image firmware Armv7-R | boot/audit | `startup_armv7r.S`, `linker.ld`, `BUILD_REPORT.md` |
| 41 | header/source/test/CMake | module contract thay cho spaghetti | toàn capstone | toàn cây project và `CMakeLists.txt` |

## Cách dùng bảng mà không biến nó thành danh sách để học thuộc

Ví dụ đang làm Bài 15:

```text
Input của mình là gì?       frequency-domain OFDM symbol
Code đổi nó thành gì?       time-domain symbol có cyclic prefix
Tại sao cần?                cho receiver chịu được delay/multipath trong giới hạn
Ai dùng output?             virtual channel / RF backend
Chiều ngược ở RX là gì?     bỏ CP rồi FFT
Reference hiện thực ở đâu?  emit/extract symbol trong phy.c
```

Đó là cách “đọc source bằng concept”. Không làm theo kiểu:

```text
thấy hàm reference
→ copy tên biến
→ sửa đến khi compile
→ không biết invariant nào vừa được bảo vệ
```

## Ba flow phải đánh dấu khi rebuild đáp án

Trong note của mỗi module, dùng ba dòng riêng:

```text
DATA FLOW      representation nào đi vào/đi ra?
STATE FLOW     state/timer/counter nào được phép đổi?
OWNERSHIP FLOW ai đang giữ buffer/descriptor, khi nào phải release?
```

Ví dụ một RX block hoàn chỉnh:

```text
DATA:
IQ → sync → FFT → equalize → LLR → TB → MAC → RLC → PDCP → SDU

STATE:
SEARCHING → CAMPED → CONNECTING → CONNECTED

OWNERSHIP:
RF backend → DMA_OWNED → DONE → CPU_OWNED → task handle → FREE
```

Nếu debug mà không biết lỗi thuộc flow nào, hãy dừng và vẽ lại ba dòng này
trước khi thêm `printf`.

---

# PHẦN XIX — KẾ HOẠCH HỌC 12 TUẦN

## Tuần 1 — C nền tảng

Làm:

```text
Bài 1–6
```

Phải tự viết được:

```text
struct
array
pointer
function
size_t
stdint
bit operations
serialization
```

Không qua tuần 2 nếu vẫn nhầm `p`, `&p`, `*p`.

---

## Tuần 2 — IQ và fixed point

Làm:

```text
Bài 7–10
sat16
complex multiply
Q15
```

Tự giải thích được:

```text
complex IQ không phải hai channel audio
```

---

## Tuần 3 — CRC + DFT/FFT

Làm:

```text
Bài 11–14
```

Mỗi ngày trace một FFT nhỏ bằng tay.

---

## Tuần 4 — OFDM

Làm:

```text
Bài 15–16
Capstone C
```

Có thể dùng floating-point reference test để kiểm fixed-point implementation.

---

## Tuần 5 — Channel + sync

Làm:

```text
Bài 17–20
Capstone D/E
```

Quan sát failure khi tăng noise/CFO.

---

## Tuần 6 — PHY end-to-end

Làm:

```text
Bài 21
```

Không chuyển sang L2 trước khi payload qua waveform thật và CRC pass.

---

## Tuần 7 — Runtime firmware

Làm:

```text
Bài 22–26
```

Tập trung:

```text
ownership
bounded queue
stale handle
virtual time
```

---

## Tuần 8 — MMIO/DMA

Làm:

```text
Bài 27–28
```

Vẽ ownership state trên giấy trước khi code.

---

## Tuần 9 — MAC/HARQ

Làm:

```text
Bài 29–31
```

Fault inject malformed length.

---

## Tuần 10 — RLC/PDCP

Làm:

```text
Bài 32–33
```

Test wraparound trước khi tin code.

---

## Tuần 11 — RRC/NAS/timer/security architecture

Làm:

```text
Bài 34–38
```

Phải có transition tests.

---

## Tuần 12 — Integration + ARM

Làm:

```text
Bài 39–41
Capstone I
```

Cuối tuần build cả host và target.

---

# PHẦN XX — CÁCH ĐỌC SOURCE MODEM MÀ KHÔNG BỊ NGỘP

Đừng mở `main.c` 800 dòng rồi kéo xuống đọc tuần tự.

Dùng phương pháp call/data-path tracing.

## Pass 1 — Public API

Đọc header trước.

Gạch ra:

```text
config
IQ block
event
stats
RF backend
platform ops
modem init/run/post
SoC step
PHY TX/RX
```

Chỉ cần hiểu contract.

## Pass 2 — One packet journey

Chọn **một transport block** rồi trace:

```text
where created?
where CRC added?
where mapped?
where IFFT?
where channel applied?
where RX submit?
where IRQ generated?
where deferred task runs?
where CRC checked?
where MAC parses?
where PDCP validates?
```

## Pass 3 — Ownership

Mỗi buffer hỏi:

```text
Ai tạo?
Ai sở hữu hiện tại?
Ai release?
Có generation không?
Có DMA state không?
```

## Pass 4 — Time

Mỗi timer hỏi:

```text
deadline ở unit nào?
virtual hay wall clock?
wraparound thế nào?
generation thế nào?
```

## Pass 5 — Fault

Mỗi parser/API hỏi:

```text
NULL?
zero length?
max length?
overflow?
truncated?
unknown type?
invalid state?
queue full?
```

Đọc source theo năm pass này hiệu quả hơn đọc từ dòng 1 đến dòng cuối.

---

# PHẦN XXI — DEBUGGING PLAYBOOK

## 1. PHY lỗi

Không in toàn buffer 8000 sample.

In/checkpoint:

```text
TX tb length
TX CRC
number OFDM symbols
PSS identity
RX best correlation metric
RX timing offset
estimated CFO
FFT pilot bins
first 8 demapped bits
received CRC / computed CRC
```

## 2. Runtime lỗi

In:

```text
current tick
event tick
queue id
read/write index
handle slot/generation
DMA state transition
```

## 3. Protocol lỗi

In:

```text
layer
SN/COUNT/HARQ ID
from state
event
guard result
to state
reject reason
```

## 4. Determinism lỗi

So sánh:

```text
same config
same seed
same events
same IQ input
```

Hash:

```text
trace
IQ output
final counters
```

Nếu khác, tìm nguồn nondeterminism:

```text
wall-clock
uninitialized memory
unordered processing
random seed
pointer value leaked into output
race
```

---

# PHẦN XXII — CÁC SAI LẦM CẦN TRÁNH

## Sai 1 — Học 3GPP trước khi hiểu buffer

Bạn sẽ biết tên PDSCH nhưng vẫn không code được parser 20 byte.

Fix: học C/data representation trước.

## Sai 2 — Copy FFT

Bạn có function chạy nhưng không biết:

```text
input đang ở time hay frequency domain?
forward hay inverse?
scale ở đâu?
```

Fix: DFT N=4 trước.

## Sai 3 — Nghĩ modem = protocol

Không. Modem còn timing, DSP, queues, DMA, hardware state.

## Sai 4 — Dùng `malloc` cho mọi thứ

Dễ học host app nhưng làm mất tư duy deterministic firmware.

## Sai 5 — Parser tin length

Đây là bug class nghiêm trọng trong low-level communication software.

## Sai 6 — State transition ở bất kỳ đâu

Sau một thời gian sẽ không biết ai đổi state.

Fix: một transition contract rõ ràng.

## Sai 7 — Debug bằng `printf("here")`

Dùng structured trace có tick/domain/event/value.

---

# PHẦN XXIII — CHECKLIST “TÔI ĐÃ THẬT SỰ HIỂU CHƯA?”

Bạn chỉ nên tự coi là hiểu một module khi làm được cả bốn:

### Cấp 1 — Explain

Giải thích không nhìn code.

### Cấp 2 — Trace

Cho input nhỏ, trace bằng tay.

### Cấp 3 — Implement

Viết lại từ header/API.

### Cấp 4 — Break

Tự tạo malformed/fault case làm code fail sạch.

Ví dụ QPSK:

```text
Explain: 2 bit → complex symbol.
Trace: 11 → (-A,-A).
Implement: map + demap.
Break: noisy sample gần decision boundary.
```

---



<!-- REFLECTION_TEMPLATE_V2 -->
# PHẦN XXIII-B — MẪU NOTE SAU MỖI BÀI

Copy template này vào notebook sau mỗi bài. Không cần viết văn dài; 5–10 dòng chính xác tốt hơn một trang mơ hồ.

```text
Bài số:
Khối modem:
Layer:

Input:
Output:
Representation trước:
Representation sau:

State bị đọc/thay đổi:
Ai sở hữu buffer trước:
Ai sở hữu buffer sau:

Lỗi nào bài này có thể phát hiện:
Nếu implementation sai, symptom sẽ xuất hiện ở đâu:

Bài kế tiếp dùng output này để làm gì:
Tự giải thích lại bằng 3 câu:
1.
2.
3.
```

Một bài chỉ thực sự hoàn thành khi bạn có thể viết phần này **không nhìn lời giải**.

---

# PHẦN XXIV — BẢN ĐỒ LỜI GIẢI TẠI CHỖ

Toàn bộ lời giải Bài 2–41 và Bài kiểu dữ liệu 2–8 đã được chuyển vào **Bước 7
ngay dưới chính bài tương ứng**. Mỗi lời giải nằm trong khối `<details>` đóng mặc
định: không vô tình nhìn thấy, nhưng cũng không phải lật hàng nghìn dòng để đối
chiếu.

Quy tắc duy nhất:

```text
tự code → test → debug ít nhất một lần → mở lời giải tại chỗ
        → đóng lời giải → viết lại từ file trắng
```

Nếu đang ở Bài N, không cần quay về phần này và cũng không cần tìm “Solution Bài
N” ở cuối file nữa.

---

# PHẦN XXV — BÀI TẬP NÂNG CAO KHÔNG CÓ LỜI GIẢI NGAY

Đây là phần chuyển từ “học theo bài” sang “tự engineering”.

## Challenge 1 — IQ statistics

Viết:

```c
struct iq_stats {
    int16_t min_i;
    int16_t max_i;
    int16_t min_q;
    int16_t max_q;
    uint64_t clipping_count;
};
```

Không overflow khi tính statistic.

### Tự code thêm trước khi mở đáp án

1. Viết `iq_stats_scan` cho block rỗng, một sample và block bình thường.
2. Biến thể A: đếm clip khi **một trong hai** thành phần chạm `INT16_MIN/MAX`.
3. Biến thể B: thêm `peak_abs` nhưng không được negate `INT16_MIN` trong 16 bit.

<details>
<summary>Đáp án kề bên — IQ statistics an toàn</summary>

```c
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

struct cpx16 { int16_t i, q; };
struct iq_stats {
    int16_t min_i, max_i, min_q, max_q;
    uint64_t clipping_count;
    uint32_t peak_abs;
};

static uint32_t abs_i16(int16_t value)
{
    int32_t wide = value;
    return (uint32_t)((wide < 0) ? -wide : wide);
}

int iq_stats_scan(const struct cpx16 *samples, size_t count,
                  struct iq_stats *out)
{
    size_t k;
    if ((samples == NULL) || (out == NULL) || (count == 0U)) return -1;
    out->min_i = out->max_i = samples[0].i;
    out->min_q = out->max_q = samples[0].q;
    out->clipping_count = 0U;
    out->peak_abs = 0U;
    for (k = 0U; k < count; ++k) {
        uint32_t ai = abs_i16(samples[k].i);
        uint32_t aq = abs_i16(samples[k].q);
        if (samples[k].i < out->min_i) out->min_i = samples[k].i;
        if (samples[k].i > out->max_i) out->max_i = samples[k].i;
        if (samples[k].q < out->min_q) out->min_q = samples[k].q;
        if (samples[k].q > out->max_q) out->max_q = samples[k].q;
        if (ai > out->peak_abs) out->peak_abs = ai;
        if (aq > out->peak_abs) out->peak_abs = aq;
        if ((samples[k].i == INT16_MIN) || (samples[k].i == INT16_MAX) ||
            (samples[k].q == INT16_MIN) || (samples[k].q == INT16_MAX)) {
            out->clipping_count++;
        }
    }
    return 0;
}
```

Test bắt buộc: `{INT16_MIN,0}` phải cho `peak_abs == 32768`, input rỗng phải
reject và không được đọc `samples[0]`.

</details>

---

## Challenge 2 — AGC toy

Input amplitude quá lớn/nhỏ.

Tính peak rồi scale block về target peak cố định bằng integer arithmetic.

Check zero block.

### Tự code thêm trước khi mở đáp án

Code ba policy: scale xuống-only, scale cả lên/xuống, và giữ nguyên zero block.
Mỗi policy phải test `target_peak=1`, `32767`, input có `INT16_MIN` và `count=0`.

<details>
<summary>Đáp án kề bên — AGC integer có saturation</summary>

```c
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

struct cpx16 { int16_t i, q; };

static int16_t sat16_agc(int64_t x)
{
    if (x > INT16_MAX) return INT16_MAX;
    if (x < INT16_MIN) return INT16_MIN;
    return (int16_t)x;
}

int agc_to_peak(struct cpx16 *x, size_t n, uint32_t target,
                int allow_gain_up)
{
    uint32_t peak = 0U;
    uint64_t gain_q20;
    size_t k;
    if ((x == NULL) || (n == 0U) || (target == 0U) ||
        (target > (uint32_t)INT16_MAX)) return -1;
    for (k = 0U; k < n; ++k) {
        int32_t wi = x[k].i, wq = x[k].q;
        uint32_t ai = (uint32_t)((wi < 0) ? -wi : wi);
        uint32_t aq = (uint32_t)((wq < 0) ? -wq : wq);
        if (ai > peak) peak = ai;
        if (aq > peak) peak = aq;
    }
    if (peak == 0U) return 1; /* zero block: hợp lệ nhưng không scale */
    if (!allow_gain_up && (peak <= target)) return 0;
    gain_q20 = ((uint64_t)target << 20U) / peak;
    for (k = 0U; k < n; ++k) {
        int64_t i = (int64_t)x[k].i * (int64_t)gain_q20;
        int64_t q = (int64_t)x[k].q * (int64_t)gain_q20;
        x[k].i = sat16_agc((i + (INT64_C(1) << 19U)) >> 20U);
        x[k].q = sat16_agc((q + (INT64_C(1) << 19U)) >> 20U);
    }
    return 0;
}
```

Không tính `target / peak` trước khi chuyển sang fixed-point: với signal nhỏ,
integer division sẽ rơi về zero hoặc mất gần hết độ chính xác.

</details>

---

## Challenge 3 — CFO compensation

Channel xoay `+Δf`.

Receiver xoay `-Δf`.

Test waveform error trước/sau compensation.

### Tự code thêm trước khi mở đáp án

Biến thể A xoay từng sample bằng phase tuyệt đối. Biến thể B giữ phase liên tục
qua nhiều block. Biến thể C cố ý dùng sai dấu CFO và viết test chứng minh error
tăng thay vì giảm.

<details>
<summary>Đáp án kề bên — CFO compensator host reference</summary>

```c
#include <math.h>
#include <stddef.h>
#include <stdint.h>

struct cpx16 { int16_t i, q; };
struct cfo_state { double phase; };

static int16_t round_sat(double x)
{
    long v = lround(x);
    if (v > 32767L) v = 32767L;
    if (v < -32768L) v = -32768L;
    return (int16_t)v;
}

int cfo_compensate(struct cpx16 *x, size_t n, double cfo_hz,
                   double sample_rate_hz, struct cfo_state *state)
{
    const double tau = 6.28318530717958647692;
    double step;
    size_t k;
    if ((x == NULL) || (state == NULL) || !(sample_rate_hz > 0.0)) return -1;
    step = -tau * cfo_hz / sample_rate_hz;
    for (k = 0U; k < n; ++k) {
        double c = cos(state->phase), s = sin(state->phase);
        double i = x[k].i, q = x[k].q;
        x[k].i = round_sat(i * c - q * s);
        x[k].q = round_sat(i * s + q * c);
        state->phase += step;
        if (state->phase > tau || state->phase < -tau) {
            state->phase = fmod(state->phase, tau);
        }
    }
    return 0;
}
```

Đây là oracle host, không phải fast path firmware. Test hai block liên tiếp phải
cho cùng output như ghép chúng thành một block; reset `state.phase` giữa hai
block phải làm test đỏ.

</details>

---

## Challenge 4 — FFT reference testing

Tạo DFT floating reference và FFT fixed implementation.

So sánh với tolerance.

Không chỉ test một vector.

### Tự code thêm trước khi mở đáp án

Chạy ít nhất năm họ vector: impulse, DC, một tone đúng bin, alternating sign và
random deterministic. Với mỗi họ, sweep `N=8,16,32,64` và báo first-divergence.

<details>
<summary>Đáp án kề bên — DFT oracle và comparator</summary>

```c
#include <math.h>
#include <stddef.h>

struct cpx64 { double i, q; };

void dft_ref(const struct cpx64 *in, struct cpx64 *out, size_t n)
{
    const double tau = 6.28318530717958647692;
    size_t k, t;
    for (k = 0U; k < n; ++k) {
        double si = 0.0, sq = 0.0;
        for (t = 0U; t < n; ++t) {
            double a = -tau * (double)k * (double)t / (double)n;
            si += in[t].i * cos(a) - in[t].q * sin(a);
            sq += in[t].i * sin(a) + in[t].q * cos(a);
        }
        out[k].i = si; out[k].q = sq;
    }
}

int spectrum_close(const struct cpx64 *want, const struct cpx64 *got,
                   size_t n, double abs_tol, size_t *first_bad)
{
    size_t k;
    if ((want == NULL) || (got == NULL) || (first_bad == NULL) ||
        !(abs_tol >= 0.0)) return -1;
    for (k = 0U; k < n; ++k) {
        if ((fabs(want[k].i - got[k].i) > abs_tol) ||
            (fabs(want[k].q - got[k].q) > abs_tol)) {
            *first_bad = k; return 0;
        }
    }
    return 1;
}
```

Khi so fixed-point FFT, tolerance phải xuất phát từ scaling schedule và số
stage, không chọn một số lớn chỉ để test xanh.

</details>

---

## Challenge 5 — PSS timing under noise

Random delay trong range 0..64.

Chạy 100 seeds.

Tính success rate.

### Tự code thêm trước khi mở đáp án

Viết runner nhận `inject(seed,delay)` và `detect()` qua callback; chạy lại hai
lần cùng seed phải có cùng histogram. Thêm sweep noise thấp, vừa và cao.

<details>
<summary>Đáp án kề bên — harness 100 seed có tiêu chí rõ</summary>

```c
#include <stddef.h>
#include <stdint.h>

typedef int (*pss_trial_fn)(uint32_t seed, uint32_t delay,
                            int noise_level, void *ctx);

int pss_sweep(pss_trial_fn trial, void *ctx, int noise_level,
              uint32_t *successes, uint32_t histogram[65])
{
    uint32_t seed;
    if ((trial == NULL) || (successes == NULL) || (histogram == NULL)) return -1;
    *successes = 0U;
    for (seed = 0U; seed < 65U; ++seed) histogram[seed] = 0U;
    for (seed = 0U; seed < 100U; ++seed) {
        uint32_t delay = (seed * 37U + 11U) % 65U;
        int detected = trial(UINT32_C(0xC0170000) + seed,
                             delay, noise_level, ctx);
        if (detected < 0) return -1;
        if ((uint32_t)detected == delay) {
            (*successes)++;
            histogram[delay]++;
        }
    }
    return 0;
}
```

Gate gợi ý: noise thấp `100/100`; noise vừa đạt ngưỡng do bạn công bố; noise
cao có thể giảm nhưng không crash, không trả delay ngoài `0..64`.

</details>

---

## Challenge 6 — Queue pressure

Fill task queue tới full.

System phải:

```text
increment dropped counter
trace reason
not corrupt indices
recover after consumer drains
```

### Tự code thêm trước khi mở đáp án

Thử ba policy: reject-new, drop-oldest và high-priority-reserved-slot. Mỗi policy
phải công bố object nào còn thuộc producer khi push thất bại.

<details>
<summary>Đáp án kề bên — queue reject-new không phá index</summary>

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define QCAP 4U
struct queue { unsigned v[QCAP]; size_t r, w, count; uint64_t dropped; };

bool queue_push(struct queue *q, unsigned value)
{
    if ((q == NULL) || (q->r >= QCAP) || (q->w >= QCAP) ||
        (q->count > QCAP)) return false;
    if (q->count == QCAP) { q->dropped++; return false; }
    q->v[q->w] = value;
    q->w = (q->w + 1U) % QCAP;
    q->count++;
    return true;
}

bool queue_pop(struct queue *q, unsigned *value)
{
    if ((q == NULL) || (value == NULL) || (q->count == 0U)) return false;
    *value = q->v[q->r];
    q->r = (q->r + 1U) % QCAP;
    q->count--;
    return true;
}
```

Test: push `1..4`, push `5` fail và `dropped==1`; pop `1,2`; push `6,7`; pop
phải ra `3,4,6,7`. Nếu `5` xuất hiện, queue đã overwrite unread work.

</details>

---

## Challenge 7 — Stale buffer handle fuzz

Generate random slot/generation combinations.

`pool_get` không bao giờ trả pointer cho stale/invalid handle.

### Tự code thêm trước khi mở đáp án

Fuzz riêng ba trục: slot ngoài range, generation lệch và length lớn hơn object.
Sau mỗi release/acquire lại cùng slot, handle cũ phải fail 100%.

<details>
<summary>Đáp án kề bên — validation trước khi lộ pointer</summary>

```c
#include <stddef.h>
#include <stdint.h>

#define SLOTS 4U
#define BYTES 64U
struct handle { uint16_t slot, generation; uint32_t length; };
struct slot { uint16_t generation; int in_use; uint8_t bytes[BYTES]; };
struct pool { struct slot slots[SLOTS]; };

void *pool_get(struct pool *p, struct handle h)
{
    struct slot *s;
    if ((p == NULL) || ((size_t)h.slot >= SLOTS) ||
        (h.length > BYTES) || (h.generation == 0U)) return NULL;
    s = &p->slots[h.slot];
    if (!s->in_use || (s->generation != h.generation)) return NULL;
    return s->bytes;
}

int pool_release(struct pool *p, struct handle h)
{
    struct slot *s = (struct slot *)0;
    if (pool_get(p, h) == NULL) return -1;
    s = &p->slots[h.slot];
    s->in_use = 0;
    s->generation++;
    if (s->generation == 0U) s->generation = 1U;
    return 0;
}
```

Fuzzer chỉ được so `pool_get(...) == NULL/non-NULL`; không dereference random
pointer. Fuzz harness không được tự tạo UB để “kiểm tra” code phòng thủ.

</details>

---

## Challenge 8 — Parser budget

Parser nested TLV có giới hạn:

```text
max total bytes
max IE count
max depth
max work operations
```

Malformed input phải bounded runtime.

### Tự code thêm trước khi mở đáp án

Tạo input: header thiếu một byte, length vượt tail, 65 IE, depth 5 và chuỗi IE
zero-length. Với mọi reject, output context không được commit nửa chừng.

<details>
<summary>Đáp án kề bên — iterative TLV parser có work budget</summary>

```c
#include <stddef.h>
#include <stdint.h>

struct limits { size_t bytes, ies, depth, work; };
struct frame { const uint8_t *p; size_t n, off, depth; };

int parse_bounded(const uint8_t *p, size_t n, struct limits lim)
{
    struct frame stack[5];
    size_t sp = 1U, ies = 0U, work = 0U;
    if ((p == NULL) || (n > lim.bytes) || (lim.depth > 4U)) return -1;
    stack[0] = (struct frame){p, n, 0U, 0U};
    while (sp != 0U) {
        struct frame *f = &stack[sp - 1U];
        uint8_t type, len;
        if (++work > lim.work) return -1;
        if (f->off == f->n) { sp--; continue; }
        if ((f->n - f->off) < 2U) return -1;
        type = f->p[f->off++]; len = f->p[f->off++];
        if ((size_t)len > (f->n - f->off) || (++ies > lim.ies)) return -1;
        if ((type & 0x80U) != 0U) {
            if ((f->depth + 1U > lim.depth) || (sp >= 5U)) return -1;
            stack[sp++] = (struct frame){&f->p[f->off], len, 0U,
                                         f->depth + 1U};
        }
        f->off += len;
    }
    return 0;
}
```

Parser thật nên parse vào temporary context rồi commit một lần sau khi toàn
frame hợp lệ. Hàm trên chỉ minh họa bounded traversal, chưa mutate protocol.

</details>

---

## Challenge 9 — RRC transition table

Thay `switch` bằng table:

```c
struct transition {
    enum state from;
    enum event event;
    bool (*guard)(...);
    int (*action)(...);
    enum state to;
};
```

Không để input kiểm soát function pointer tùy ý; table là compile-time internal data.

### Tự code thêm trước khi mở đáp án

Thêm guard fail, action fail và duplicate `(from,event)`. Chứng minh cả ba trường
hợp không đổi state; action chỉ chạy sau guard.

<details>
<summary>Đáp án kề bên — transition table transactional</summary>

```c
#include <stdbool.h>
#include <stddef.h>

enum state { OFF, SEARCHING, CAMPED, CONNECTED, STATE_COUNT };
enum event { POWER_ON, FOUND, SETUP, EVENT_COUNT };
struct ctx;
typedef bool (*guard_fn)(const struct ctx *);
typedef int (*action_fn)(struct ctx *);
struct transition { enum state from; enum event event;
                    guard_fn guard; action_fn action; enum state to; };
struct ctx { enum state state; unsigned actions; };

int dispatch(struct ctx *c, enum event e,
             const struct transition *table, size_t count)
{
    size_t k;
    if ((c == NULL) || (table == NULL) || (c->state >= STATE_COUNT) ||
        (e >= EVENT_COUNT)) return -1;
    for (k = 0U; k < count; ++k) {
        const struct transition *t = &table[k];
        if ((t->from == c->state) && (t->event == e)) {
            if ((t->guard != NULL) && !t->guard(c)) return 1;
            if ((t->action != NULL) && (t->action(c) != 0)) return -1;
            c->state = t->to;
            return 0;
        }
    }
    return 1; /* wrong-state/unknown transition: context unchanged */
}
```

Trong project thật, validate table lúc init để reject duplicate `(from,event)`;
table là `static const`, parser chỉ tạo enum event đã range-check.

</details>

---

## Challenge 10 — Deterministic replay

Run cùng scenario hai lần.

So sánh:

```text
final state
counters
trace
IQ output
```

Phải identical.

### Tự code thêm trước khi mở đáp án

Biến thể A so byte-for-byte. Biến thể B bỏ duy nhất field wall-clock được công
bố. Biến thể C cố ý để global PRNG không reset và viết test bắt nondeterminism.

<details>
<summary>Đáp án kề bên — evidence bundle để so replay</summary>

```c
#include <stddef.h>
#include <stdint.h>
#include <string.h>

struct final_state { uint32_t state, tx, rx, crc_fail, recoveries; };
struct run_result {
    struct final_state final;
    const uint8_t *trace; size_t trace_len;
    const uint8_t *iq; size_t iq_len;
};

int runs_identical(const struct run_result *a, const struct run_result *b)
{
    if ((a == NULL) || (b == NULL)) return 0;
    if (memcmp(&a->final, &b->final, sizeof(a->final)) != 0) return 0;
    if ((a->trace_len != b->trace_len) || (a->iq_len != b->iq_len)) return 0;
    if ((a->trace_len != 0U) &&
        (memcmp(a->trace, b->trace, a->trace_len) != 0)) return 0;
    if ((a->iq_len != 0U) &&
        (memcmp(a->iq, b->iq, a->iq_len) != 0)) return 0;
    return 1;
}
```

Không `memcmp` struct có padding trong wire/test liên tiến trình; ở đây
`final_state` phải được zero-init và chỉ dùng trong cùng executable. Bản audit
chắc hơn encode từng field rồi hash/cmp canonical bytes như lời giải 100/100.

</details>

---

# PHẦN XXVI — TỪ MINI MODEM ĐẾN NR THẬT: HỌC GÌ TIẾP?

Khi đã làm được toàn bộ khóa này, mới nên đào sâu spec theo nhóm.

## PHY

Đọc theo thứ tự:

```text
38.211 → physical channels/signals, OFDM, modulation, sequence
38.212 → channel coding, rate matching
38.213 → physical layer procedures/control
38.214 → data procedures / MCS / related behavior
```

Đừng cố đọc từ đầu đến cuối.

Chọn một đường dữ liệu:

```text
PDSCH receive
```

rồi tra section cần thiết.

## L2/L3

```text
38.321 MAC
38.322 RLC
38.323 PDCP
38.331 RRC
24.501 5GS NAS
33.501 security architecture
```

Luôn phân biệt:

```text
mini teaching implementation
vs
3GPP-conformant implementation
```

Khóa này giúp bạn có mental model và kỹ năng code để **đọc spec**, không thay thế spec.

---

# PHẦN XXVII — BẢN ĐỒ LIÊN HỆ VỚI PROJECT THAM CHIẾU

Khi đã học xong phần nền, mở project reference theo thứ tự này:

```text
1. include/bb/bb.h
2. src/fw/codec/codec.c
3. src/fw/channel/channel.c
4. src/fw/phy/phy.c
5. src/fw/memory/memory.c
6. src/fw/runtime/runtime.c
7. src/fw/l2/mac.c
8. src/fw/l2/rlc_um.c
9. src/fw/l2/pdcp.c
10. src/fw/control/state_machine.c
11. src/fw/security/security.c
12. src/soc/soc.c
13. src/host/network_peer.c
14. src/host/main.c
15. arch/armv7r/startup_armv7r.S
16. arch/armv7r/linker.ld
```

Lý do không bắt đầu từ `main.c`:

```text
main = orchestration lớn
```

Nếu chưa biết primitive mà đọc orchestration, bạn chỉ nhớ tên function.

Lý do `codec.c` trước `phy.c`:

```text
PHY dùng QPSK/QAM/FFT/CRC
```

Lý do `memory/runtime` trước protocol:

```text
protocol code chạy bên trong ownership + queue + scheduler model đó
```

---

# PHẦN XXVIII — MỘT BUỔI HỌC MẪU 90 PHÚT

Ví dụ hôm nay học ring buffer.

## 0–10 phút

Không code.

Vẽ:

```text
producer → ring → consumer
```

Giải thích full/empty.

## 10–25 phút

Viết struct + `push`.

## 25–35 phút

Test full.

## 35–50 phút

Viết `pop`.

## 50–60 phút

Test wraparound.

## 60–70 phút

Cố tình tạo bug:

```text
capacity=7 nhưng vẫn dùng & (capacity-1)
```

Quan sát sai.

## 70–80 phút

Học tại sao power-of-two invariant tồn tại.

## 80–90 phút

Đóng code, viết lại `push/pop` từ đầu.

Đó là cách biến code từ “thứ đã nhìn thấy” thành “thứ mình sở hữu”.

---

# PHẦN XXIX — CHECKPOINT CUỐI

Nếu có thể trả lời các câu dưới đây mà không tra, bạn đã qua giai đoạn beginner modem/baseband:

1. IQ sample là gì?
2. Tại sao cần complex representation?
3. Sample rate nói điều gì?
4. QPSK map bao nhiêu bit/symbol?
5. IFFT nằm ở TX hay RX OFDM?
6. CP nằm ở đâu?
7. CFO biểu hiện như thế nào trên complex samples?
8. Correlation giúp sync ra sao?
9. CRC khác FEC thế nào?
10. IRQ top-half nên làm gì?
11. Vì sao FFT không nên chạy trong ISR?
12. SPSC ring full được nhận biết thế nào?
13. Generation-tagged handle chặn loại lỗi gì?
14. DMA ownership state là gì?
15. Tại sao `volatile` không thay atomic synchronization?
16. MAC/RLC/PDCP khác nhau ở mental model nào?
17. HARQ soft combining là gì?
18. Vì sao SN wraparound không compare bằng `>` naïve?
19. Vì sao state machine cần guard?
20. Timer generation giải quyết callback stale thế nào?
21. Vì sao host simulator dùng virtual time?
22. Tại sao deterministic replay quan trọng?
23. Tại sao cùng firmware source nên build host + target?
24. MMIO abstraction giúp test thế nào?
25. Tại sao parser phải check multiplication overflow?

Nếu còn câu nào trả lời mơ hồ, quay lại đúng level tương ứng thay vì tiếp tục nhồi thêm kiến thức.

---

# PHỤ LỤC A — Compiler flags nên dùng sớm

Host learning:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -fsanitize=address,undefined -g task.c -o task
```

Freestanding-oriented source discipline:

```text
-std=c17
-ffreestanding
-fno-builtin
-fno-common
-Wall
-Wextra
-Wconversion
-Wshadow
-Werror
```

Đừng bật `-Werror` rồi chữa warning bằng cast bừa. Hiểu warning trước.

---

# PHỤ LỤC B — GDB tối thiểu cho người học C firmware

Compile:

```bash
gcc -g -O0 task.c -o task
```

Start:

```bash
gdb ./task
```

Các lệnh nên thuộc:

```gdb
break main
run
next
step
print x
print/x x
print *p
x/16bx buffer
x/8hx samples
backtrace
continue
```

Đặc biệt với buffer:

```gdb
x/32bx ptr
```

sẽ giúp nối mental model C object với bytes trong memory.

---

# PHỤ LỤC C — Quy tắc coding firmware cá nhân

Mỗi function bạn tự viết nên check checklist:

```text
[ ] pointer input có thể NULL không?
[ ] length/capacity có được check không?
[ ] phép cộng/nhân size có overflow không?
[ ] loop có bounded không?
[ ] ownership sau return thuộc ai?
[ ] state transition có hợp lệ không?
[ ] counter/SN/tick có wrap không?
[ ] output có deterministic không?
[ ] error có làm partial mutation nguy hiểm không?
[ ] có unit test malformed case chưa?
```

---

# PHỤ LỤC D — Đích kỹ thuật cụ thể của đề tham chiếu

Sau khi làm xong lộ trình, project cuối hướng tới các primitive và subsystem kiểu:

```text
C17 freestanding firmware core
Armv7-R startup + linker + vectors
host simulator
virtual IQ RF backend
fixed-point radix-2 FFT/IFFT
CP + resource grid
QPSK/16-QAM
PSS detection
CFO/timing estimation
one-tap equalization
CRC-24A
SPSC rings
buffer pool with generation handles
DMA ownership
MMIO abstraction
virtual scheduler/timers
MAC multiplexing
HARQ processes
RLC-UM reorder
PDCP COUNT/reorder/anti-replay
RRC state machine
NAS registration/session state
async crypto job interface
fault injection
structured deterministic trace
```

Đừng biến danh sách này thành checklist copy source. Mỗi mục chỉ được đánh dấu xong khi tự viết được phiên bản nhỏ độc lập và test được nó.

---

# KẾT LUẬN

Con đường hợp lý không phải:

```text
“học 5G” → copy modem source
```

mà là:

```text
C chắc
→ bytes/bits/memory
→ IQ
→ modulation
→ transform/OFDM
→ channel/sync
→ PHY link
→ runtime/ownership
→ MMIO/DMA
→ L2
→ state machines
→ integration
→ đọc spec sâu
```

Khi đi theo thứ tự này, source modem lớn sẽ dần biến từ một đống function lạ thành những block đã quen:

```text
“à, đây là ring”
“đây là buffer ownership”
“đây là QPSK mapper”
“đây là FFT stage”
“đây là correlation sync”
“đây là RLC window”
“đây là RRC transition”
```

Đó mới là thời điểm bạn ngừng “biết copy code” và bắt đầu thực sự đọc, debug và tự engineering modem/baseband.

**Checkpoint cao nhất của khóa:** đóng/xóa reference và tự dựng lại `modem101` từ repo trắng. Khi bạn tự tái tạo được toàn bộ pipeline, runtime, protocol, target build và E2E tests mà không cần nhìn source đáp án, mục tiêu mới thực sự hoàn thành.

---

# PHẦN XXX — BÀI TỐT NGHIỆP: TỰ CODE LẠI NGUYÊN PROJECT `01_mini_ue_end_to_end`

Đây là phần cuối và cũng là mục tiêu thật sự của toàn bộ tài liệu.

Các bài 1–41 giúp bạn học từng primitive. Các capstone trước giúp bạn nối những
primitive đó. Phần này yêu cầu cao hơn:

> Đóng ZIP lời giải, tạo một repo trắng và tự code lại toàn bộ project mini UE
> end-to-end, từ build system đến host simulator, firmware core, virtual RF,
> PHY, runtime, L2/L3, security, SoC/MMIO/DMA, Armv7-R và tests.

Bạn không cần tạo source giống từng dòng reference. Bạn phải tái tạo được:

```text
cùng lớp chức năng
+ cùng public contract
+ cùng data flow
+ cùng state flow
+ cùng ownership flow
+ cùng invariant phòng thủ
+ cùng hành vi end-to-end có thể kiểm chứng
```

Nếu chương trình chỉ in đúng `MODEM_RESULT` nhưng payload không đi qua IQ và
toàn stack, đó không phải rebuild. Nếu copy source rồi đổi tên biến, đó cũng
không phải rebuild.

## 30.1. Definition of Done tuyệt đối

Project tự viết chỉ được coi là hoàn thành khi đồng thời đạt các điều kiện sau:

```text
[ ] Tự tạo được toàn bộ source tree từ thư mục trắng.
[ ] Firmware core là ISO C17 freestanding và không heap sau init.
[ ] Cùng firmware source build được cho host và target Armv7-R.
[ ] Payload/control data đi thật qua waveform IQ, không side-channel.
[ ] PHY TX/RX round trip được QPSK và 16-QAM.
[ ] Dò được PSS, timing và CFO trong virtual channel.
[ ] Runtime có ring, pool, scheduler, timer và IRQ deferred work.
[ ] DMA/MMIO/cache ownership có transition hợp lệ.
[ ] MAC/RLC/PDCP đi qua queue/handle, không gọi tắt layer.
[ ] RRC/NAS chỉ đổi state qua transition hợp lệ.
[ ] Integrity được kiểm trước khi mutate protocol state.
[ ] SDU 4096 byte được khôi phục byte-for-byte.
[ ] Có một CRC fail được inject có chủ đích.
[ ] Có HARQ retransmission và soft combine.
[ ] Có reorder và replay rejection thật.
[ ] Link loss dẫn tới search/camp/connect lại và trở về DATA.
[ ] Malformed input không crash, OOB, UAF, deadlock hay allocation vô hạn.
[ ] Cùng seed/config/events cho output và trace deterministic.
[ ] Host unit/integration tests pass.
[ ] ARM ELF link sạch, không W+X và boot tới firmware entry.
```

Thiếu một gate quan trọng thì chưa được nói “đã code lại nguyên đáp án”.

## 30.2. Quy tắc đóng lời giải

Tạo ba thư mục riêng:

```text
01_reference_locked/       # ZIP đáp án, không mở trong lúc tự làm
02_rebuild_guided/         # lần rebuild có dùng giáo trình và notes
03_rebuild_closed_book/    # lần rebuild cuối, không nhìn source lần trước
```

Trong lần `02_rebuild_guided`, được dùng:

```text
đề bài
giáo trình này
notes do chính bạn viết
3GPP/tài liệu được đề cho phép
compiler, debugger, sanitizer
test vectors do bạn tự tạo
```

Không được dùng:

```text
copy/paste source reference
diff liên tục với reference
mở main.c của đáp án để bắt chước orchestration
truyền payload bằng global/shared variable ngoài waveform
hard-code final counter hoặc final stdout
```

Chỉ mở reference sau khi một milestone đã có implementation và test của riêng
bạn. Khi mở, chỉ review theo bảng:

```text
Contract mình thiết kế:
Implementation mình viết:
Invariant mình bảo vệ:
Test mình đã pass:
Reference khác ở đâu:
Khác biệt đó là bug hay chỉ là cách thiết kế khác:
```

Sau review, đóng reference và sửa bằng hiểu biết của mình, không paste đoạn code
vừa nhìn thấy.

## 30.3. Repo trắng phải tự dựng

Bắt đầu bằng cây thư mục sau. Chưa viết logic lớn trong ngày đầu.

```text
modem101_rebuild/
├── CMakeLists.txt
├── scenario.cfg
├── events.txt
├── cmake/
│   └── arm-none-eabi-toolchain.cmake
├── arch/
│   ├── host/
│   │   └── task_stack_x86_64.S
│   └── armv7r/
│       ├── startup_armv7r.S
│       ├── linker.ld
│       └── test_key_arm.S
├── include/
│   └── bb/
│       ├── bb.h
│       ├── protocol.h
│       ├── mac.h
│       ├── security.h
│       ├── ipc.h
│       └── soc.h
├── src/
│   ├── fw/
│   │   ├── internal.h
│   │   ├── arm_entry.c
│   │   ├── codec/codec.c
│   │   ├── channel/channel.c
│   │   ├── phy/phy.c
│   │   ├── memory/memory.c
│   │   ├── runtime/runtime.c
│   │   ├── l2/protocol.c
│   │   ├── l2/stack.c
│   │   ├── l2/mac.c
│   │   ├── l2/rlc_um.c
│   │   ├── l2/pdcp.c
│   │   ├── control/transitions.c
│   │   ├── control/state_machine.c
│   │   ├── security/security.c
│   │   ├── ipc/ipc.c
│   │   └── trace/trace.c
│   ├── hal/mmio.c
│   ├── soc/dma_descriptor.c
│   ├── soc/soc.c
│   ├── soc/test_key_host.c
│   └── host/
│       ├── network_peer.h
│       ├── network_peer.c
│       └── main.c
└── tests/
    ├── unit_q1.c
    ├── unit_q2.c
    ├── unit_runtime.c
    ├── unit_l2.c
    ├── unit_mac.c
    ├── unit_defensive.c
    ├── deterministic.cmake
    ├── bad_iq.cmake
    ├── iq_input.cmake
    ├── arm_elf_audit.cmake
    └── arm_qemu_smoke.cmake
```

Bạn có thể đổi tên file, nhưng không được gộp toàn bộ vào một `main.c` khổng
lồ. Boundary giữa các module là một phần của bài.

## 30.3A. Track cầm tay chỉ việc — từ code cực nhỏ tới đúng project

Phần primer ở đầu tài liệu cố ý dùng những ví dụ rất nhỏ như một byte `0x41`,
`struct cpx16`, mảng IQ, QPSK toy, event toy và ownership toy. Bây giờ ta dùng
lại đúng các mảnh dễ hiểu đó để **bắt đầu repo thật**.

Nguyên tắc:

```text
không bắt đầu bằng phy.c 26 nghìn byte
không bắt đầu bằng runtime.c hơn 40 nghìn byte

bắt đầu bằng:
một type
→ một hàm
→ một test
→ một module
→ một boundary
→ một pipeline
```

Mỗi buổi code lại một phần lời giải đều theo chín bước:

```text
1. Nói bằng lời module nhận gì và trả gì.
2. Tạo header/prototype trước.
3. Tạo source với implementation nhỏ nhất.
4. Tạo test cho happy path.
5. Compile ngay.
6. Đọc warning/error, không copy lỗi lên mạng rồi paste bừa.
7. Thêm edge case và invalid input.
8. Nối sang đúng consumer kế tiếp.
9. Ghi concept + invariant vào REBUILD_LOG.md.
```

### Buổi 0 — Tạo repo đầu tiên mà chưa cần biết modem

Mở terminal:

```bash
mkdir -p modem101_rebuild/include/bb
mkdir -p modem101_rebuild/src/fw/codec
mkdir -p modem101_rebuild/tests
cd modem101_rebuild
```

Đừng tạo hết 30 file rỗng ngay. Ta chỉ tạo file khi hiểu trách nhiệm của nó.

Tạo `include/bb/bb.h`:

```c
#ifndef BB_BB_H
#define BB_BB_H

#include <stdint.h>

struct bb_cpx16 {
    int16_t i;
    int16_t q;
};

#endif
```

Tạo `tests/smoke_types.c`:

```c
#include "bb/bb.h"

#include <stdio.h>

int main(void)
{
    struct bb_cpx16 sample = {1200, -450};

    printf("I=%d Q=%d\n", (int)sample.i, (int)sample.q);
    return 0;
}
```

Compile trực tiếp trước, chưa cần CMake:

```bash
gcc -std=c17 -Wall -Wextra -Werror \
    -Iinclude tests/smoke_types.c -o smoke_types
./smoke_types
```

Output:

```text
I=1200 Q=-450
```

#### Bạn vừa làm gì?

```text
C concept:       struct + fixed-width integer + header
Modem concept:   một complex baseband sample
Input:           hai giá trị I/Q
Output:          object có representation rõ
Dùng tiếp bởi:   IQ block, QAM mapper, FFT, channel, DMA
```

Nếu chưa tự giải thích được vì sao dùng `int16_t` thay vì `int`, chưa sang bước
tiếp theo.

### Buổi 1 — Biến một sample thành một block waveform

Mở lại `bb.h`, thêm:

```c
#include <stddef.h>

struct bb_iq_block {
    uint64_t t0;
    uint32_t sample_rate_hz;
    uint32_t count;
    struct bb_cpx16 *samples;
};
```

Tạo test:

```c
#include "bb/bb.h"

#include <stdio.h>

int main(void)
{
    struct bb_cpx16 samples[4] = {
        {100, 0},
        {200, 10},
        {300, 20},
        {400, 30}
    };

    struct bb_iq_block block = {
        .t0 = 500U,
        .sample_rate_hz = 1000U,
        .count = 4U,
        .samples = samples
    };

    for (uint32_t n = 0U; n < block.count; ++n) {
        printf("%u: (%d,%d)\n",
               (unsigned)n,
               (int)block.samples[n].i,
               (int)block.samples[n].q);
    }

    return 0;
}
```

Đừng chỉ nhìn output. Hãy vẽ:

```text
block.samples ───────┐
                     ▼
              samples[0..3]

block.count = 4      giới hạn vùng hợp lệ
block.sample_rate    khoảng cách thời gian giữa sample
block.t0             thời điểm sample đầu tiên
```

#### Tự phá code

Đổi:

```c
.count = 40U
```

Chạy dưới AddressSanitizer:

```bash
gcc -std=c17 -Wall -Wextra -Werror \
    -fsanitize=address,undefined -g \
    -Iinclude tests/smoke_block.c -o smoke_block
./smoke_block
```

Mục đích không phải giữ bug. Mục đích là thấy rõ: pointer không mang theo kích
thước; `count` là một phần của security/correctness contract.

### Buổi 2 — Module thật đầu tiên: `codec.c`

Ta chọn `sat16` vì nó nhỏ, dễ test và thật sự có trong lời giải.

Thêm prototype vào `bb.h`:

```c
int16_t bb_sat16(int32_t value);
```

Tạo `src/fw/codec/codec.c`:

```c
#include "bb/bb.h"

#include <stdint.h>

int16_t bb_sat16(int32_t value)
{
    if (value > INT16_MAX) {
        return INT16_MAX;
    }

    if (value < INT16_MIN) {
        return INT16_MIN;
    }

    return (int16_t)value;
}
```

Tạo `tests/unit_codec_beginner.c`:

```c
#include "bb/bb.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    assert(bb_sat16(100) == 100);
    assert(bb_sat16(32767) == 32767);
    assert(bb_sat16(32768) == 32767);
    assert(bb_sat16(-32768) == -32768);
    assert(bb_sat16(-32769) == -32768);

    puts("sat16 passed");
    return 0;
}
```

Compile:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -Iinclude src/fw/codec/codec.c tests/unit_codec_beginner.c \
    -o unit_codec_beginner
./unit_codec_beginner
```

Expected:

```text
sat16 passed
```

#### Giải thích từng nhánh

```text
value > 32767   → không thể chứa trong int16_t → clamp 32767
value < -32768  → không thể chứa trong int16_t → clamp -32768
còn lại         → cast an toàn vì đã chứng minh nằm trong range
```

#### Nó được dùng ở đâu trong modem?

```text
complex multiply
FFT butterfly
oscillator
channel noise/CFO
equalization
soft LLR combining
```

Nếu để arithmetic wrap, một số dương lớn có thể biến thành số âm và làm
constellation nhảy sang quadrant khác.

### Buổi 3 — QPSK mapper/demapper thật

Thêm prototype:

```c
void bb_qpsk_map(uint8_t first, uint8_t second,
                 struct bb_cpx16 *symbol);
void bb_qpsk_demap(struct bb_cpx16 symbol,
                   uint8_t *first, uint8_t *second);
```

Thêm implementation vào `codec.c`:

```c
void bb_qpsk_map(uint8_t first, uint8_t second,
                 struct bb_cpx16 *symbol)
{
    const int16_t amplitude = 11585;

    if (symbol == NULL) {
        return;
    }

    symbol->i = (first & 1U) != 0U ?
                    (int16_t)-amplitude : amplitude;
    symbol->q = (second & 1U) != 0U ?
                    (int16_t)-amplitude : amplitude;
}

void bb_qpsk_demap(struct bb_cpx16 symbol,
                   uint8_t *first, uint8_t *second)
{
    if (first == NULL || second == NULL) {
        return;
    }

    *first = symbol.i < 0 ? 1U : 0U;
    *second = symbol.q < 0 ? 1U : 0U;
}
```

Thêm test:

```c
for (uint8_t bits = 0U; bits < 4U; ++bits) {
    uint8_t b0 = (uint8_t)((bits >> 1U) & 1U);
    uint8_t b1 = (uint8_t)(bits & 1U);
    uint8_t out0 = 9U;
    uint8_t out1 = 9U;
    struct bb_cpx16 symbol = {0, 0};

    bb_qpsk_map(b0, b1, &symbol);
    bb_qpsk_demap(symbol, &out0, &out1);

    assert(out0 == b0);
    assert(out1 == b1);
}
```

#### Đọc data flow

```text
(b0,b1)
   ↓ map
(I,Q)
   ↓ channel lý tưởng
(I,Q)
   ↓ demap
(b0,b1)
```

Đây là round-trip đầu tiên của PHY. Nó chưa phải waveform vì chưa có resource
grid, IFFT hay CP.

#### Bài tự làm ngay sau đó

Không xem lời giải `bb_qam16_map` trong ZIP. Tự viết 16-QAM theo quy tắc:

```text
4 bit → hai cặp 2 bit
mỗi cặp chọn một mức trục trong {-3A,-A,+A,+3A}
Gray mapping
```

Test đủ 16 input:

```c
for (uint8_t x = 0U; x < 16U; ++x) {
    struct bb_cpx16 symbol;
    bb_qam16_map(x, &symbol);
    assert(bb_qam16_demap(symbol) == x);
}
```

Nếu fail, in từng `x`, `I`, `Q`, `decoded`; đừng sửa mapping bằng đoán mò.

### Buổi 4 — Little-endian trước khi đụng descriptor/PDU

Tạo file học nhỏ `tests/unit_endian.c`:

```c
#include <assert.h>
#include <stdint.h>

static void put_u32_le(uint8_t out[4], uint32_t value)
{
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8U);
    out[2] = (uint8_t)(value >> 16U);
    out[3] = (uint8_t)(value >> 24U);
}

static uint32_t get_u32_le(const uint8_t in[4])
{
    return ((uint32_t)in[0]) |
           ((uint32_t)in[1] << 8U) |
           ((uint32_t)in[2] << 16U) |
           ((uint32_t)in[3] << 24U);
}

int main(void)
{
    uint8_t bytes[4] = {0U, 0U, 0U, 0U};

    put_u32_le(bytes, UINT32_C(0x12345678));

    assert(bytes[0] == 0x78U);
    assert(bytes[1] == 0x56U);
    assert(bytes[2] == 0x34U);
    assert(bytes[3] == 0x12U);
    assert(get_u32_le(bytes) == UINT32_C(0x12345678));

    return 0;
}
```

Sau khi hiểu, bạn sẽ dùng cùng pattern trong:

```text
protocol.c
ipc.c
dma_descriptor.c
IQ16 reader/writer
```

Không chuyển sang `memcpy(output, &struct_value, sizeof struct_value)`. Struct
trong RAM có thể có padding và ABI khác wire format.

### Buổi 5 — Ring toy trước atomic SPSC

Tạo phiên bản single-thread để hiểu index trước:

```c
#include <stdbool.h>
#include <stddef.h>

#define TOY_RING_CAPACITY 4U

struct toy_ring {
    size_t read_index;
    size_t write_index;
    size_t count;
    int entries[TOY_RING_CAPACITY];
};

static bool toy_push(struct toy_ring *ring, int value)
{
    if (ring == NULL || ring->count == TOY_RING_CAPACITY) {
        return false;
    }

    ring->entries[ring->write_index] = value;
    ring->write_index =
        (ring->write_index + 1U) % TOY_RING_CAPACITY;
    ++ring->count;
    return true;
}

static bool toy_pop(struct toy_ring *ring, int *value)
{
    if (ring == NULL || value == NULL || ring->count == 0U) {
        return false;
    }

    *value = ring->entries[ring->read_index];
    ring->read_index =
        (ring->read_index + 1U) % TOY_RING_CAPACITY;
    --ring->count;
    return true;
}
```

Test bằng giấy trước:

```text
push 10 → push 20 → pop → push 30 → push 40 → push 50
```

Ghi `read_index`, `write_index`, `count` sau mỗi bước.

Sau khi toy pass, chuyển sang ring thật:

```text
entry type: struct bb_event
index type: _Atomic uint32_t
producer publish: release
consumer observe: acquire
capacity bounded
không mutex/block trong IRQ
```

Đừng thêm atomic trước khi hiểu ring full/empty/wrap. Atomic giải quyết
visibility/order giữa contexts; nó không sửa một thuật toán ring sai.

### Buổi 6 — Buffer handle toy trước pool thật

Vẽ hai object khác nhau:

```text
raw pointer: địa chỉ tạm thời trong owner hiện tại
handle:      {slot,generation,length} dùng để nhận diện object qua queue
```

Tự code pool hai slot trước:

```c
#include <stdbool.h>
#include <stdint.h>

#define TOY_POOL_SLOTS 2U
#define TOY_POOL_BYTES 16U

struct toy_handle {
    uint16_t slot;
    uint16_t generation;
    uint32_t length;
};

struct toy_slot {
    bool used;
    uint16_t generation;
    uint32_t length;
    uint8_t data[TOY_POOL_BYTES];
};

struct toy_pool {
    struct toy_slot slots[TOY_POOL_SLOTS];
};
```

Tự viết ba hàm, chưa xem solution:

```c
int toy_alloc(struct toy_pool *pool, uint32_t length,
              struct toy_handle *handle, uint8_t **data);
int toy_get(struct toy_pool *pool, struct toy_handle handle,
            uint8_t **data);
int toy_release(struct toy_pool *pool, struct toy_handle handle);
```

Hint theo tầng:

```text
Hint 1: alloc tìm slot used=false.
Hint 2: get check slot bound, used, generation và length.
Hint 3: release chỉ hợp lệ nếu handle vẫn match slot hiện tại.
Hint 4: khi tái sử dụng slot, generation phải đổi.
```

Test quan trọng nhất:

```text
h1 = alloc slot 0 generation 1
release h1
h2 = alloc lại slot 0 generation 2
get(h1) phải fail
get(h2) phải success
```

Đó là bản cực nhỏ của `memory.c` trong lời giải.

### Buổi 7 — State machine toy trước RRC/NAS

Đừng viết `if` rải rác khắp parser. Tạo state rõ:

```c
enum toy_rrc_state {
    TOY_OFF = 0,
    TOY_SEARCHING,
    TOY_CAMPED,
    TOY_CONNECTED
};

enum toy_event {
    TOY_BOOT = 1,
    TOY_CELL_FOUND,
    TOY_SETUP_OK,
    TOY_LINK_LOSS
};
```

Viết hàm đầu tiên bằng `switch` để hiểu:

```c
static bool toy_transition(enum toy_rrc_state *state,
                           enum toy_event event)
{
    if (state == NULL) {
        return false;
    }

    switch (*state) {
    case TOY_OFF:
        if (event == TOY_BOOT) {
            *state = TOY_SEARCHING;
            return true;
        }
        break;

    case TOY_SEARCHING:
        if (event == TOY_CELL_FOUND) {
            *state = TOY_CAMPED;
            return true;
        }
        break;

    case TOY_CAMPED:
        if (event == TOY_SETUP_OK) {
            *state = TOY_CONNECTED;
            return true;
        }
        break;

    case TOY_CONNECTED:
        if (event == TOY_LINK_LOSS) {
            *state = TOY_SEARCHING;
            return true;
        }
        break;

    default:
        break;
    }

    return false;
}
```

Test sai state:

```text
state=OFF, event=SETUP_OK
→ return false
→ state vẫn OFF
```

Sau khi hiểu, refactor thành transition table `{from,event,guard,action,to}` và
tách RRC/NAS như reference.

### Buổi 8 — Pipeline toy đầu tiên: `0x41 → QPSK → 0x41`

Trước OFDM, hãy chứng minh bạn hiểu đổi representation.

Pseudo-code cần tự biến thành C:

```text
input byte = 0x41

for bit_pair = 0..3:
    lấy hai bit MSB-first
    qpsk_map → symbol[bit_pair]

for mỗi symbol:
    qpsk_demap → hai bit
    set lại vào output byte

assert output byte == input byte
```

Trace mong đợi:

```text
input byte:  01000001
pair 0:      01 → symbol
pair 1:      00 → symbol
pair 2:      00 → symbol
pair 3:      01 → symbol
output byte: 01000001
```

#### Bạn vừa code gì?

```text
application representation: byte
PHY logical representation: bits
modulation representation:  QPSK complex symbols
demodulation:               symbol signs trở lại bits
application output:         byte
```

Đây là mini bridge giữa phần giải thích rất dễ ở đầu tài liệu và `phy.c` thật.

### Buổi 9 — Từ QPSK symbols thành OFDM toy

Không nhảy thẳng NFFT=256. Dùng NFFT=8:

```text
1. Tạo bins[8] = 0.
2. Đặt bốn QPSK symbol vào bốn bin đã chọn.
3. Chạy IFFT reference bằng double hoặc DFT chậm.
4. Copy hai sample cuối lên đầu làm CP.
5. Receiver bỏ hai CP sample.
6. Chạy FFT/DFT.
7. Lấy lại bốn bin.
8. Demap và so byte.
```

Test không impairment trước. Chỉ khi round trip pass mới tăng lên:

```text
NFFT=16
→ NFFT=256
→ fixed-point Q15
→ pilot
→ PSS
→ timing/CFO/noise
```

Nếu bắt đầu bằng NFFT=256, một lỗi index sẽ tạo hàng trăm số sai và người mới
không biết nhìn từ đâu.

## 30.3B. CMake từng nấc, không viết build system khổng lồ ngay

Sau Buổi 2, tạo CMake nhỏ:

```cmake
cmake_minimum_required(VERSION 3.16)
project(modem101_rebuild C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(modem_fw_host STATIC
    src/fw/codec/codec.c)

target_include_directories(modem_fw_host PUBLIC include)

target_compile_options(modem_fw_host PRIVATE
    -Wall -Wextra -Wconversion -Wshadow -Werror)

add_executable(unit_codec_beginner
    tests/unit_codec_beginner.c)

target_link_libraries(unit_codec_beginner PRIVATE modem_fw_host)

enable_testing()
add_test(NAME unit_codec_beginner COMMAND unit_codec_beginner)
```

Chạy:

```bash
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Mỗi khi thêm module:

```text
1. Thêm đúng một source vào library.
2. Build ngay.
3. Thêm đúng một unit test.
4. Chạy lại toàn test cũ.
5. Chỉ sau khi xanh mới thêm module tiếp.
```

Đến cuối mới bổ sung ARM target, sanitizer option và integration scripts.

## 30.3C. Bảng cầm tay cho từng phần của lời giải

| Khi code lại phần này | Việc đầu tiên phải làm | Hàm nhỏ đầu tiên | Test đầu tiên | Sau đó mới mở rộng |
|---|---|---|---|---|
| Public API | vẽ object/lifetime | `bb_cpx16`, `bb_iq_block` | header smoke | toàn modem/SoC API |
| Codec | test integer boundary | `bb_sat16` | saturation edges | CRC/QAM/FFT/NCO |
| Channel | identity copy | delay=0 path | input == output | delay, CFO, noise |
| PHY TX | một byte QPSK | `get_bit` | known symbol | grid/IFFT/CP/PSS |
| PHY RX | ideal symbol | hard demap | TX→RX byte | sync/CFO/equalize/LLR |
| Memory | pool hai slot | alloc/get/release | stale handle | 16 slot + owned events |
| Runtime | ring bốn entry | push/pop | full/empty/wrap | atomic/IRQ/tasks/timers |
| MMIO | fake array register | aligned read/write | bad alignment | platform ops + SoC |
| DMA | một descriptor | put/get fields | 32-byte fixture | CRC/ring/cycle/ownership |
| Protocol | header không payload | encode/decode | round trip | IE/depth/budget/integrity |
| IPC | one TLV | CRC32C | bad CRC | rings/doorbell/epoch-seq |
| MAC | one SDU | multiplex | one-LCID round trip | multi-LCID/BSR/HARQ |
| RLC | SN in-order | modular distance | 0,1,2 | gap/window/timer/wrap |
| PDCP | COUNT in-order | reconstruct count | near SN wrap | replay/reorder/reassembly |
| Control | bốn state toy | one transition | wrong state | full RRC + NAS tables |
| Security | known CMAC vector | block encrypt/CMAC | official vector | descriptor/MMIO/IRQ async |
| Stack | hai fake stages | enqueue copy | owner transfers | PHY→MAC→RLC→PDCP→control |
| Peer | one cell beacon | peer TX IQ | UE sees TB | full scenario lifecycle |
| Host | parse one config key | integer parser | invalid key | events/IQ/trace/E2E |
| ARM | reset→entry only | zero `.bss` | QEMU marker | vectors/MPU/IRQ/audit |

## 30.3D. Mẫu hướng dẫn khi đang code một file thật

Ví dụ tới `src/fw/l2/rlc_um.c`, đừng mở reference. Làm đúng checklist:

### Bước 1 — Viết contract bằng lời

```text
Input:  payload + SN 12-bit + tick
State:  rx_next + 64 reorder slots + timer generation
Output: PDU contiguous được đẩy sang PDCP
Reject: duplicate/out-of-window/collision
```

### Bước 2 — Viết helper nhỏ nhất

```c
static uint16_t sn12_distance(uint16_t from, uint16_t to)
{
    return (uint16_t)((to - from) & 0x0FFFU);
}
```

### Bước 3 — Test helper quanh wrap

```text
distance(0,1)       = 1
distance(4095,0)    = 1
distance(4094,1)    = 3
```

### Bước 4 — Chỉ hỗ trợ in-order trước

```text
if SN == rx_next:
    deliver
    rx_next = rx_next + 1 modulo 4096
else:
    trả lỗi tạm thời
```

### Bước 5 — Thêm đúng một feature

```text
out-of-order slot
```

Test xong mới thêm:

```text
contiguous drain
→ duplicate bitmap
→ window
→ timer
→ generation
→ restart cleanup
```

Đây là cách code từng phần mà vẫn hiểu. Không viết full RLC trong một lần rồi
nhìn 20 test fail cùng lúc.

Lặp đúng phương pháp đó với PDCP, HARQ, runtime và PHY:

```text
happy path nhỏ
→ test
→ một edge case
→ test
→ một state/ownership rule
→ test
→ integration
```

## 30.3E. Quy tắc nhận hỗ trợ trong lúc tự rebuild

Khi bí, hãy mô tả theo mẫu này thay vì gửi nguyên file và hỏi “sửa hộ”:

```text
Module tôi đang làm:
Contract tôi hiểu:
Input test:
Expected:
Actual:
Checkpoint cuối còn đúng:
Invariant nghi bị sai:
Code nhỏ nhất tái hiện lỗi:
```

Thứ tự trợ giúp nên là:

```text
Giải thích concept
→ hỏi dẫn dắt
→ Hint 1
→ Hint 2
→ pseudo-code
→ skeleton có TODO
→ review code bạn tự viết
→ full solution chỉ khi đã thử thật
```

Mục tiêu là giúp bạn tự code tiếp, không biến trợ giúp thành một nguồn copy mới.

## 30.4. Dependency graph phải thuộc trước khi code

```text
public types/contracts
        ↓
codec primitives ─── channel model
        ↓                 ↓
        └────── PHY TX/RX ┘
                  ↓
          memory + runtime
                  ↓
          HAL/MMIO + SoC/DMA
                  ↓
       MAC → RLC → PDCP → control
                  ↓          ↓
                 IPC      security
                  └────┬─────┘
                       ↓
             NetworkPeer + host CLI
                       ↓
                 E2E scenario
                       ↓
              Arm startup/linker
```

Không code theo thứ tự tên file. Code theo dependency. Ví dụ `phy.c` phụ thuộc
FFT/QAM/CRC, nên không hợp lý khi viết `phy_receive()` trước `codec.c`.

## 30.5. Milestone 0 — Viết contract trước implementation

Tạo `include/bb/bb.h` trước. Tự khai báo lại các nhóm type:

```text
bb_cpx16
bb_iq_block
bb_buf_handle
bb_event
bb_stats
bb_rf_backend
bb_platform_ops
bb_config
bb_rrc_state
bb_nas_state
bb_session_status
```

Các constant profile cần chốt tường minh:

```text
NFFT                 256
CP length            18
max IQ samples       8192
max TB bytes         512
max SDU bytes        4096
slot duration        500 virtual ticks
modulation           QPSK=2 bits, 16-QAM=4 bits
```

Tự viết lại public API theo nhóm trách nhiệm:

```c
size_t bb_modem_required_memory(const struct bb_config *cfg);
int bb_modem_init(/* arena, config, platform ops, out modem */);
int bb_modem_post(/* modem, event */);
int bb_modem_irq(/* modem, irq */);
int bb_modem_run_until(/* modem, virtual tick */);

size_t bb_soc_required_memory(const struct bb_config *cfg);
int bb_soc_init(/* arena, config, RF backend, out SoC */);
int bb_soc_step(/* SoC, firmware, virtual tick */);
```

Không copy prototype rồi bỏ đó. Với mỗi API, viết comment contract của riêng
bạn:

```text
valid input
invalid input
owner trước call
owner sau call thành công
owner sau call thất bại
bounded work
observable result
```

### Gate Milestone 0

```text
[ ] Header tự compile độc lập bằng C17.
[ ] Không public type nào phụ thuộc kích thước pointer host.
[ ] Wire field dùng fixed-width integer.
[ ] Không C bit-field cho wire/MMIO.
[ ] CMake tạo được target host rỗng nhưng link sạch.
```

### Variant gate M0 — contract đổi nhưng implementation chưa được phép xuất hiện

Tự code ba bản header: baseline API đề bài; bản thêm observer callback + `ctx`;
bản thêm cancellation bằng generation token. Viết compile-only test chứng minh
opaque struct không thể `sizeof` ở caller và mọi size dùng `size_t/%zu`.

<details><summary>Đáp án đối chiếu M0 — ngay tại milestone</summary>

Shape đúng nằm ở `include/bb/bb.h`, `protocol.h`, `ipc.h`, `mac.h`,
`security.h`, `soc.h` trong ZIP 100/100. Public header chỉ lộ fixed-width wire
field, pointer + length, ops + context, opaque `bb_modem/bb_soc`; private layout
nằm ở `src/fw/internal.h`. Gate pass khi một TU caller chỉ include public header
vẫn compile strict C17 và không truy cập internal field.

</details>

## 30.6. Milestone 1 — Build system và hai target dùng chung core

`CMakeLists.txt` phải tạo tối thiểu:

```text
modem_fw_host     static library chứa firmware core
modem101_host     executable host, OUTPUT_NAME là modem101
bb_fw_arm         executable ELF cho Cortex-R5 khi cross-compile
```

Firmware source dùng flags:

```text
-std=c17
-ffreestanding
-fno-builtin
-fno-common
-Wall
-Wextra
-Wconversion
-Wshadow
-Werror
-Wstrict-prototypes
-Wmissing-prototypes
-Wformat=2
```

Đích của milestone này chưa phải modem chạy. Đích là chứng minh kiến trúc build:

```text
cùng source firmware
      ├── build thành static library cho host test
      └── link vào ELF freestanding cho Arm
```

### Gate Milestone 1

```bash
cmake -S . -B build-host -DCMAKE_BUILD_TYPE=Debug
cmake --build build-host --parallel
```

Phải không có warning. Không chữa warning bằng cast bừa.

### Variant gate M1 — build graph cũng phải bị mutation

Tự code ba cấu hình: Release strict; ASan+UBSan host; `-fanalyzer`. Sau đó
cross-build cùng firmware sources cho Cortex-R5 và cố thêm một host syscall vào
`src/fw` để target/link audit bắt lỗi.

<details><summary>Đáp án đối chiếu M1</summary>

ZIP 100/100 dùng `modem_fw_host`→`libmodem_fw_host.a`, `modem101_host`, và
`bb_fw_arm.elf` chung core trong `CMakeLists.txt`; toolchain ở
`cmake/arm-none-eabi-toolchain.cmake`. Đáp án phải cho 10/10 test ở Release,
sanitizer và analyzer; Arm target pass `arm_elf_audit` + `arm_qemu_smoke`.

</details>

## 30.7. Milestone 2 — `codec.c`: primitive toán học và bit

Viết theo thứ tự nhỏ đến lớn:

```text
bb_sat16
bb_crc24a
bb_qpsk_map / bb_qpsk_demap
bb_qam16_map / bb_qam16_demap
complex Q15 multiply
oscillator init / oscillator step
radix-2 fixed-point FFT/IFFT
```

Invariant bắt buộc:

```text
bit order của CRC là MSB-first
CRC-24A polynomial là 0x1864CFB, init zero
QAM mapping/demapping round trip ở điểm lý tưởng
nhân int16_t phải widen trước
kết quả thu hẹp phải saturation
IFFT scale một bit mỗi radix-2 stage
FFT input count phải power of two và bounded
```

Test nhỏ phải viết trước khi sang PHY:

```text
sat16 ở 32767, 32768, -32768, -32769
CRC empty/one-byte/multi-byte fixture
QPSK đủ bốn constellation points
16-QAM đủ 16 input nibble
FFT impulse
FFT single tone
FFT rồi IFFT round trip có tolerance fixed-point
oscillator zero CFO và CFO khác zero
```

### Ý nghĩa modem/baseband

Đây là alphabet số học mà PHY dùng. Nếu milestone này sai, waveform vẫn có thể
“trông giống sóng” nhưng receiver không thể khôi phục bit ổn định.

### Gate Milestone 2

Tạo `tests/unit_codec.c` của riêng bạn. Không cần chờ `unit_q1` lớn mới debug.

### Variant gate M2 — codec không được chỉ có happy path

Tự code thêm: CRC incremental split mọi offset; bit-order MSB/LSB fixture cùng
byte `0x81`; Q15 saturation exhaustive quanh biên. Cố inject bốn bug CRC ở F19.

<details><summary>Đáp án đối chiếu M2</summary>

Đối chiếu `src/fw/codec/codec.c`; test độc lập ở `tests/unit_q1.c` phải phủ CRC,
QPSK/16-QAM, fixed-point/FFT primitive và literal fixture. Đáp án đúng giữ CRC-24A
poly `0x1864CFB`, init zero, MSB-first; storage `int16_t`, intermediate rộng rồi
scale/saturation.

</details>

## 30.8. Milestone 3 — `channel.c`: virtual RF có impairment

Tự viết:

```c
int bb_channel_apply(const struct bb_config *cfg,
                     const struct bb_iq_block *in,
                     struct bb_iq_block *out);
```

Pipeline channel:

```text
TX IQ
→ timing delay
→ CFO rotation
→ deterministic AWGN-like noise
→ RX IQ
```

Điều kiện:

```text
PRNG phụ thuộc seed, không phụ thuộc wall-clock
output capacity được kiểm trước khi ghi
delay không đọc ngoài input
CFO zero cho phase không đổi
sample arithmetic dùng widen + saturation
cùng seed/input cho byte-identical output
```

### Gate Milestone 3

Chạy ba test độc lập:

```text
identity channel: delay=0, CFO=0, noise=0
delay-only: đầu output là zero rồi mới tới input
determinism: chạy hai lần và cmp toàn bộ IQ
```

### Variant gate M3 — channel streaming và order

Tự code một-block vs chunked equivalence; ba impairment order; seed/config
mutation. Mỗi run ghi digest IQ, phase cuối, PRNG state và delay history.

<details><summary>Đáp án đối chiếu M3</summary>

Đối chiếu `src/fw/channel/channel.c` và channel tests trong `unit_q1.c`.
Deterministic state nằm trong context; delay/CFO/AWGN không dùng global; cùng
seed/config cho byte-identical waveform, đổi order phải có test phát hiện.

</details>

## 30.9. Milestone 4 — `phy.c`: TX trước, RX sau

Đừng viết một hàm 2.000 dòng. Chia TX thành các stage:

```text
TB bytes
→ append CRC-24A
→ bit extraction
→ QPSK hoặc 16-QAM mapping
→ resource data bins
→ pilot/PSS bins
→ IFFT
→ add cyclic prefix
→ IQ block
```

Các helper concept tương ứng trong reference:

```text
get_bit / set_bit
make_pss
carrier_bin
map_pss
map_pilot
map_data
emit_symbol
```

Sau khi TX pass fixture, mới code RX:

```text
IQ block
→ validate count/rate
→ AGC estimate
→ PSS correlation / timing
→ CFO estimate và correction
→ remove CP
→ FFT
→ pilot channel estimate
→ one-tap equalization
→ common-phase tracking
→ hard/soft demap
→ reconstruct bytes
→ CRC-24A decision
```

Các checkpoint trace phải nhìn thấy:

```text
SYNC
FFT
CHANNEL_EST
DEMAP
DECODE
TB_CRC
```

PSS milestone phải sinh và dò ba sequence dài 127. Không được “sync” bằng cách
truyền sẵn timing index qua side-channel.

### Gate Milestone 4

Thứ tự test:

```text
1. QPSK, no noise, no CFO, no delay.
2. 16-QAM, no impairment.
3. Thêm timing offset.
4. Thêm CFO +1.8 kHz.
5. Thêm noise nhỏ.
6. Max TB 512 byte.
7. Corrupt waveform để CRC fail.
```

Gate mạnh nhất:

```text
RX(channel(TX(payload))) == payload
```

Không debug bằng mỗi dòng `PHY failed`. In checkpoint theo từng stage.

### Variant gate M4 — PHY phải fail ở đúng stage

Tự code QPSK/16-QAM ở min/max TB; sweep CFO/noise/delay; corrupt PSS, pilot, data
và CRC riêng. Trace phải chỉ first-divergence trong `SYNC/FFT/CHANNEL_EST/DEMAP/DECODE/TB_CRC`.

<details><summary>Đáp án đối chiếu M4</summary>

Đối chiếu `src/fw/phy/phy.c`, `tests/unit_q1.c`, `unit_defensive.c`. Reference
100/100 có đủ ba PSS literal-checked, timing/CFO/AGC, fixed FFT/IFFT+CP/grid,
one-tap equalization/common phase, soft demap, CRC và max TB 512 byte; empty TB
reject. Không có đường bit tắt giữa TX/RX.

</details>

## 30.10. Milestone 5 — `memory.c`: pool và handle generation-tagged

Tự định nghĩa pool cố định. Reference dùng 16 slot, mỗi slot 1024 byte:

```text
FREE slot
→ alloc(length)
→ trả handle {slot,generation,length}
→ get(handle)
→ release(handle)
→ generation tăng trước vòng sử dụng tiếp
```

Public/raw pointer không được chạy xuyên task, DMA hoặc IPC. Pointer chỉ có giá
trị trong owner domain hiện tại; object identity liên-domain là handle.

Hàm cần tự viết:

```text
bb_pool_init
bb_pool_alloc
bb_pool_get
bb_pool_release
```

### Gate Milestone 5

```text
[ ] alloc tới khi full
[ ] release rồi alloc lại cùng slot với generation mới
[ ] old handle bị reject
[ ] length zero/quá lớn bị reject
[ ] double release bị reject
[ ] failure không làm hỏng slot khác
```

### Variant gate M5 — churn pool tới khi generation wrap/reuse

Tự code acquire/release 100.000 lượt; stale handle từ mọi generation cũ; wrong
slot/length; pool full rồi recover. Mọi failure giữ ownership và free-count.

<details><summary>Đáp án đối chiếu M5</summary>

Đối chiếu `src/fw/memory/memory.c`, identity contract trong `INVARIANTS.md` và
`tests/unit_runtime.c`. Resolver check `(slot,generation,length)` trước pointer;
release/restart invalidate generation; no heap sau init.

</details>

## 30.11. Milestone 6 — `runtime.c`: ring, IRQ, task và virtual time

Runtime cần tự code các mảnh:

```text
C11 atomic SPSC ring
8 task queues
priority/deadline dispatch
virtual slot 500 tick
bounded work budget
IRQ top-half
deferred RX/TX work
timer generation
task stack guard/high-water
stats/trace/fault state
```

Ring API:

```text
bb_ring_push
bb_ring_pop
bb_ring_peek
```

Modem API chính:

```text
bb_modem_required_memory
bb_modem_init
bb_modem_post
bb_modem_irq
bb_modem_run_until
bb_modem_send_tb
bb_modem_rx_iq_submit
bb_modem_tx_acquire
bb_modem_tx_complete
bb_modem_read_tb
bb_modem_restart_domain
```

Quy tắc IRQ:

```text
IRQ top-half:
ACK + chụp status/timestamp/handle + enqueue

Deferred task:
FFT + decode + parser + protocol + trace nặng
```

Không chạy FFT, CMAC, PDU parser hoặc file I/O trong ISR.

### Gate Milestone 6

`unit_runtime` của bạn phải kiểm:

```text
empty/full/wrap ring
same-tick stable ordering
queue pressure/drop
IRQ chỉ enqueue
deferred work mới decode
cache clean/invalidate đúng ownership boundary
stale timer generation
tick modular comparison
stack guard fault
domain restart dọn handle/queue
```

### Variant gate M6 — runtime schedule phải bị stress theo time và pressure

Tự code same-tick stable order, tick wrap, queue full/recovery, timer stale và
stack guard fault. ISR test phải chứng minh chỉ ACK+enqueue, decoder không chạy trong IRQ.

<details><summary>Đáp án đối chiếu M6</summary>

Đối chiếu `src/fw/runtime/runtime.c`, `arch/host/task_stack_x86_64.S`,
`tests/unit_runtime.c`. Reference có slot 0,5 ms, modular tick, bounded dispatcher,
10 task stack thật guard/high-water, SPSC acquire/release và deferred RF path.

</details>

## 30.12. Milestone 7 — `dma_descriptor.c`, `mmio.c`, `soc.c`

Descriptor wire có đúng 32 byte:

```text
src:u32
dst:u32
len:u32
flags:u32
cookie:u16
generation:u16
next:u32
reserved:u32
crc32c:u32
```

Tự viết:

```text
bb_dma_descriptor_encode
bb_dma_descriptor_decode
bb_dma_ring_validate
bb_mmio_read32
bb_mmio_write32
bb_soc_required_memory
bb_soc_init
bb_soc_step
bb_soc_mmio_read32
bb_soc_mmio_write32
```

DMA state machine:

```text
FREE → CPU_OWNED → DMA_OWNED → DONE → CPU_OWNED/FREE
```

Publish order:

```text
fill descriptor
→ cache clean
→ release fence
→ set OWN
→ clean OWN/cache line
→ release fence
→ doorbell
```

Completion order:

```text
device DONE
→ IRQ
→ acquire fence
→ cache invalidate
→ CPU đọc data
```

Validation phải bao gồm alignment, region, length, flags, CRC, `next`, bounded
ring count và cycle detection.

### Gate Milestone 7

Descriptor encode/decode phải byte-exact, không `memcpy(struct)` và không phụ
thuộc padding ABI.

### Variant gate M7 — descriptor/MMIO phải chịu input đối nghịch

Tự code aligned/misaligned address; aperture boundary exact/overflow; `next`
cycle/self-loop/too-long; bad CRC/flags/generation. Assert cache/fence/OWN/doorbell order.

<details><summary>Đáp án đối chiếu M7</summary>

Đối chiếu `src/soc/dma_descriptor.c`, `src/hal/mmio.c`, `src/soc/soc.c`,
`tests/unit_q2.c`. Descriptor canonical đúng 32 byte, CRC32C phủ byte 0–27,
physical resolver bounded, publish fill→clean→release→OWN→doorbell; MMIO aligned.

</details>

## 30.13. Milestone 8 — `protocol.c` và `ipc.c`: wire parser phòng thủ

Protocol PDU phải encode/decode từng byte. Tự dựng các type cần cho scenario:

```text
CELL_BEACON
RRC_SETUP
SECURITY_MODE
REG_ACCEPT
SESSION_ACCEPT
DATA_GRANT
DATA
LINK_LOSS
và các response phía UE
```

Contract parser:

```text
PDU tối đa 65,535 byte
IE tối đa 64
depth tối đa 4
work budget tối đa 1,000 operation/PDU
reserved/padding phải canonical
không partial state mutation nếu frame cuối cùng invalid
```

BBIP frame có header 28 byte:

```text
magic 'BBIP':u32
version:u16
type:u16
total_len:u32
seq:u32
epoch:u32
flags:u32
crc32c:u32
```

IPC API:

```text
bb_crc32c
bb_ipc_encode
bb_ipc_decode
bb_modem_ipc_submit
bb_modem_ipc_receive
```

Response phải giữ đúng `(epoch,seq)`. Unknown/duplicate TLV, bad length, bad CRC
và non-zero padding phải bị reject có kiểm soát.

### Gate Milestone 8

`unit_q2` và malformed sweep phải chạy parser trên truncated input ở mọi byte
boundary, không chỉ vài case đẹp.

### Variant gate M8 — parser phải transactional dưới mutation

Tự code unknown TLV skip, duplicate singleton reject, truncated ở mọi offset,
IE 64/65, depth 4/5, budget 1000/1001; IPC response giữ đúng `(epoch,seq)`.

<details><summary>Đáp án đối chiếu M8</summary>

Đối chiếu `src/fw/l2/protocol.c`, `src/fw/ipc/ipc.c`, `tests/unit_q2.c` và
`unit_defensive.c`. BBIP header 28 byte + TLV padded 4; validate version/length/
padding/CRC/count/depth/budget vào temporary context rồi commit một lần.

</details>

## 30.14. Milestone 9 — `mac.c`: multiplex, BSR và tám HARQ process

Đầu tiên tự viết MAC multiplex/demultiplex độc lập:

```c
int bb_mac_multiplex(/* SDU list → BMAC TB */);
int bb_mac_demultiplex(/* BMAC TB → SDU views */);
```

BMAC profile:

```text
magic/version/count/reserved
1..8 sub-SDU
mỗi subheader: LCID, reserved, length
LCID control hoặc data
tổng length phải khớp chính xác
```

Toàn frame phải validate xong trước khi enqueue bất kỳ child event nào.

Sau parser mới thêm:

```text
BSR tăng khi enqueue data
BSR giảm khi data được transmit
8 HARQ process độc lập
state/transmissions/generation/expiry/PDCP COUNT riêng
soft LLR buffer riêng
LLR dương = bit 0, âm = bit 1
Chase combine bằng saturated add
```

### Gate Milestone 9

`unit_mac` phải kiểm multi-LCID round trip và canonical errors. `unit_l2` phải
kiểm hai HARQ process xen kẽ không ghi nhầm soft buffer.

### Variant gate M9 — MAC/HARQ interleave chứ không chạy từng process riêng

Tự code 1..8 LCID canonical; BSR enqueue/TX/fail; HARQ ids `2,5,2,5` với erased
codeword, duplicate retransmission và timeout. Wrong id/count không mutate soft buffer.

<details><summary>Đáp án đối chiếu M9</summary>

Đối chiếu `src/fw/l2/mac.c`, `tests/unit_mac.c`, `unit_l2.c`. Reference có BMAC
validate toàn TB trước dispatch, 8 context độc lập, signed-LLR Chase saturation,
active-grant/count association, timer/generation và retry bounded.

</details>

## 30.15. Milestone 10 — `rlc_um.c`: sequence number và reorder

RLC-UM profile:

```text
SN 12-bit
modulo 4096
window 64
fixed reorder slots
virtual reorder timer
```

Tự code:

```text
bb_rlc_process
bb_rlc_drain
bb_rlc_check_timer
```

Các case bắt buộc:

```text
in-order PDU
SN tới trước gây gap
SN bị thiếu tới sau và release contiguous
duplicate
out-of-window
slot collision
SN wrap 4095 → 0
timer expiry
stale timer callback
```

Không compare sequence bằng `a > b` naïve. Viết helper modular distance và test
ngay quanh điểm wrap.

### Variant gate M10 — RLC reorder phải chạy xuyên wrap và timeout

Tự code arrival `4094,0,4095,1`, duplicate, out-of-window, collision cùng slot và
missing SN timeout. Release order/counter/reason phải deterministic.

<details><summary>Đáp án đối chiếu M10</summary>

Đối chiếu `src/fw/l2/rlc_um.c`, `tests/unit_l2.c`. Window 64 modulo 4096, fixed
reorder slots, contiguous release, duplicate/out-of-window/collision reason và
timer generation; không allocation theo sequence gap.

</details>

## 30.16. Milestone 11 — `pdcp.c`: COUNT, HFN và anti-replay

PDCP profile:

```text
SN 18-bit
HFN là phần cao của COUNT
COUNT modulo 2^32
window/reorder tối đa 64
anti-replay bitmap
```

Tự code:

```text
bb_pdcp_reconstruct_count
bb_pdcp_decode_and_process
bb_pdcp_check_timer
```

Gate bắt buộc:

```text
in-order delivery
out-of-order giữ lại
release contiguous
duplicate/replay drop có counter/reason
18-bit SN wrap làm HFN tăng đúng
overlap fragment không ghi đè sai
timer generation invalidation
```

Output của PDCP mới được đi tới reassembly/application hoặc control. Không cho
MAC nhảy thẳng tới delivered SDU.

### Variant gate M11 — PDCP phải phân biệt SN18 và COUNT32

Tự code SN wrap làm HFN tăng; ahead/delayed/duplicate/too-old; reorder timeout;
bad integrity trước state mutation. Wire chỉ mang SN18, context suy COUNT.

<details><summary>Đáp án đối chiếu M11</summary>

Đối chiếu `src/fw/l2/pdcp.c`, `tests/unit_l2.c`. Reference dùng fixed 64-slot
reorder, COUNT modulo `2^32`, SN18 thấp + HFN cao, anti-replay và verify integrity
trước HARQ/RLC/PDCP/control mutation.

</details>

## 30.17. Milestone 12 — `transitions.c` và `state_machine.c`

Viết transition table trước action code.

RRC:

```text
OFF → SEARCHING → CAMPED → CONNECTING → CONNECTED
```

NAS:

```text
DEREGISTERED → REGISTERING → REGISTERED → SESSION_ACTIVE
```

Mỗi row có:

```text
from
event
guard
action
to
```

Tự code validator kiểu:

```text
bb_control_rrc_allowed
bb_control_nas_allowed
bb_control_process
```

Sai state hoặc guard false phải:

```text
không đổi context
không tăng delivered giả
không bật security/session giả
ghi trace/drop reason phù hợp
```

Test transition table độc lập trước khi nối PDU parser.

### Variant gate M12 — control table phải chịu event permutation

Tự code valid attach/recovery path; chèn mỗi event ở mọi wrong state; guard fail;
action fail; link-loss ở từng state; stale timer. Context chỉ commit sau success.

<details><summary>Đáp án đối chiếu M12</summary>

Đối chiếu `src/fw/control/transitions.c`, `state_machine.c`, `tests/unit_l2.c`.
Mỗi row có `{from,event,guard,action,to}`, một PDU tối đa một transition; recovery
đi `SEARCHING→CAMPED→CONNECTING→CONNECTED`, NAS tách state RRC.

</details>

## 30.18. Milestone 13 — `security.c`: async crypto, không phải helper gọi tắt

Firmware context chỉ giữ `key_slot` handle. Key test nằm sau hardware-like
boundary.

Primitive cần có:

```text
AES block operation nội bộ
CMAC
NIA2(COUNT, BEARER, DIRECTION, message)
constant-result verification contract
```

Nhưng protocol path không được gọi CMAC rồi tiếp tục ngay. Phải đi qua:

```text
submit crypto job
→ fill descriptor
→ publish OWN
→ MMIO doorbell
→ accelerator step
→ completion IRQ 13
→ deferred continuation
```

Các function boundary cần tái tạo:

```text
bb_security_cmac
bb_security_nia2
bb_security_verify
bb_crypto_submit_mac
bb_crypto_submit_mac_sign
bb_crypto_accelerator_step
bb_crypto_complete_irq
bb_crypto_reset
```

Integrity phải được xác minh trước HARQ/RLC/PDCP/control mutation. Tag sai không
được làm state tiến lên rồi mới rollback.

### Variant gate M13 — async crypto completion phải chống stale và bad tag

Tự code known-answer CMAC/NIA2; wrong key slot/count/bearer/direction; descriptor
queue full; wrong cookie/generation completion; IRQ đến sau domain restart.

<details><summary>Đáp án đối chiếu M13</summary>

Đối chiếu `src/fw/security/security.c`, `include/bb/security.h`,
`tests/unit_q2.c`, `unit_l2.c`. Firmware chỉ giữ key-slot handle; NIA2 message là
`COUNT||BEARER||DIRECTION||0^26||message`; continuation chỉ sau IRQ13 hợp lệ.

</details>

## 30.19. Milestone 14 — `stack.c`: nối layer bằng event và owned handle

Đây là milestone dễ sinh shortcut nhất.

Đường đúng:

```text
PHY decoded TB
→ enqueue MAC event + owned handle
→ MAC validate/demux
→ enqueue RLC
→ RLC reorder/release
→ enqueue PDCP
→ PDCP verify/reorder/replay
→ enqueue CONTROL/NAS hoặc deliver SDU
```

Đường sai:

```text
PHY gọi thẳng bb_control_process()
MAC copy payload vào global delivered buffer
NetworkPeer set thẳng RRC/NAS state
```

Tự code boundary kiểu:

```text
bb_stack_init
bb_stack_process_stage
bb_stack_process_pdu
bb_stack_check_timers
bb_modem_enqueue_stage_copy
```

Mỗi enqueue phải xác định owner mới và đường release khi success/failure.

### Variant gate M14 — ownership ledger cho mọi edge trong stack

Tự code success, queue-full, parser-reject, integrity-fail và domain-restart cho
mỗi edge PHY→MAC→RLC→PDCP→control. Sau mỗi scenario mọi handle phải về pool đúng một lần.

<details><summary>Đáp án đối chiếu M14</summary>

Đối chiếu `src/fw/l2/stack.c`, `src/fw/internal.h`, `tests/unit_mac.c`,
`unit_l2.c`, `unit_runtime.c`. Layer truyền owned handle qua task queue; không
application→decoder call tắt; restart dọn queue/handle/context theo domain.

</details>

## 30.20. Milestone 15 — `network_peer.c`: đối tác kiểm thử thật qua waveform

`NetworkPeer` phải có state riêng, không được là oracle chọc vào context UE.

Peer cần làm:

```text
tạo control/data PDU
MAC multiplex
protocol/security encode
PHY transmit
channel apply
đưa IQ vào RF backend
nhận uplink IQ
PHY receive
parse response của UE
quyết định bước scenario tiếp theo
```

Scenario bắt buộc:

```text
BOOT
→ phát cell/PSS cho PCI 42
→ UE search và camp ở CFO +1.8 kHz
→ RRC Setup
→ security completion
→ registration
→ PDU session
→ gửi SDU 4096 byte
→ làm hỏng một codeword để tạo CRC fail
→ nhận NACK và phát HARQ RETX
→ gửi reorder case
→ phát lại PDU cũ để test anti-replay
→ inject link loss
→ UE search/camp/connect lại
→ trở về DATA
```

Không được giữ pointer tới `struct bb_modem` trong peer để set state hoặc copy
delivered bytes.

### Variant gate M15 — peer không được trở thành oracle/side-channel

Tự code control PDU và data segmentation qua cùng waveform; erase codeword ở ba
vị trí; thay payload ngẫu nhiên deterministic. Search source/test để chứng minh
peer không cầm pointer vào UE state/payload destination.

<details><summary>Đáp án đối chiếu M15</summary>

Đối chiếu `src/host/network_peer.c/.h`, `tests/integration_full` qua CMake.
Reference peer đi BMAC→CRC/modulation/grid/IFFT/CP→channel→RF backend; không sửa
trực tiếp RRC/NAS/session hay copy SDU vào UE.

</details>

## 30.21. Milestone 16 — `main.c`: host runner và file format

CLI cuối phải nhận:

```bash
./modem101 \
  --config scenario.cfg \
  --events events.txt \
  --iq-in downlink.iq \
  --iq-out uplink.iq \
  --trace trace.jsonl
```

`--iq-in` có thể optional cho scenario tự sinh bởi peer, nhưng reader IQ16 phải
được implement và test.

IQ16:

```text
magic[4] = "IQ16"
sample_rate:u32 little-endian
count:u64 little-endian
count cặp I/Q int16 little-endian
```

Reader phải reject:

```text
bad magic
zero/invalid sample rate
count vượt bound
size multiplication overflow
truncated sample data
trailing byte khi format cấm
```

`scenario.cfg`:

```text
UTF-8 key=value
unknown key là lỗi
range từng field được check
```

`events.txt`:

```text
tick|source|hex_payload
stable sort theo tick
trùng tick giữ thứ tự file
```

`trace.jsonl` mỗi record có ít nhất:

```text
tick
domain
event
```

`stdout` cuối chỉ có đúng một dòng:

```text
MODEM_RESULT status=<PASS|FAIL> state=<STATE> tx_bytes=<N> rx_bytes=<N> crc_fail=<N> recoveries=<N>
```

Không in PASS chỉ vì vòng lặp chạy hết. PASS phải phụ thuộc toàn bộ invariant
scenario.

### Variant gate M16 — host runner phải bị fuzz như parser

Tự code unknown/missing/duplicate config key; same-tick event stable order;
odd hex/trailing input; IQ16 truncation/trailing/count overflow; output path fail.

<details><summary>Đáp án đối chiếu M16</summary>

Đối chiếu `src/host/main.c`, `scenario.cfg`, `events.txt`,
`tests/bad_iq.cmake`, `iq_input.cmake`, `deterministic.cmake`. CLI schema đúng đề;
stdout đúng một dòng result; JSONL canonical/monotonic; hai run IQ/trace identical.

</details>

## 30.22. Milestone 17 — Armv7-R startup và linker

Chỉ làm sau khi host core pass.

`startup_armv7r.S` phải tự thực hiện:

```text
vector table
reset entry
mode stack setup
.data copy từ load address
.bss zero
MPU/cache policy tối thiểu
IRQ entry/return giữ AAPCS alignment
gọi bb_firmware_main
```

`linker.ld` phải mô hình hóa:

```text
ROM
TCM_RX
TCM_RW
SRAM
DDR / DMA_NC
IPC_SHM
KEY
```

Không để W+X sau boot. Đặt `.ARM.extab/.ARM.exidx` hợp lệ cạnh code để tránh
relocation PREL31 vượt tầm.

Build:

```bash
cmake -S . -B build-arm \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-toolchain.cmake

cmake --build build-arm --target bb_fw_arm --parallel
arm-none-eabi-size -A build-arm/bb_fw_arm.elf
arm-none-eabi-nm -u build-arm/bb_fw_arm.elf
arm-none-eabi-readelf -h -l -S build-arm/bb_fw_arm.elf
```

Gate:

```text
ELF32 little-endian ARM EABI5 soft-float
không undefined symbol
không LOAD segment W+X
GNU_STACK không executable
section không vượt memory region
QEMU Cortex-R5 boot tới bb_firmware_main
```

### Variant gate M17 — startup/linker phải có test phá có chủ đích

Tự code rồi cố phá từng thứ: bỏ `.bss` zero; copy `.data` sai end; làm stack lệch
AAPCS; đặt `.ARM.exidx` sai vùng; tạo W+X; để undefined symbol. Audit phải bắt từng bug.

<details><summary>Đáp án đối chiếu M17</summary>

Đối chiếu `arch/armv7r/startup_armv7r.S`, `linker.ld`, `test_key_arm.S`,
`src/fw/arm_entry.c`, `tests/arm_elf_audit.cmake`, `arm_qemu_smoke.cmake`.
Reference dựng vector/mode stacks, copy data/zero bss, MPU/cache, ELF32 EABI5
soft-float, `.ARM.exidx` cạnh text TCM_RX, no W+X và boot tới `bb_firmware_main`.

</details>

## 30.23. Verification matrix phải tự dựng lại

Không chỉ copy tên test. Mỗi test phải chứng minh một contract cụ thể.

| Test | Điều phải chứng minh |
|---|---|
| `unit_q1` | CRC, QAM, PSS, FFT, OFDM, channel và max TB |
| `unit_q2` | CMAC/NIA2, protocol/IE, IPC và DMA descriptor |
| `unit_runtime` | deferred IRQ, queue, pool, ownership, cache và virtual time |
| `unit_l2` | HARQ, RLC/PDCP reorder/wrap/replay, state guard, restart |
| `unit_mac` | multi-LCID round trip và canonical parser rejection |
| `unit_defensive` | hàng nghìn malformed input deterministic dưới sanitizer |
| `integration_full` | lifecycle end-to-end qua waveform và toàn stack |
| `reject_truncated_iq` | IQ16 reader không đọc thiếu/tràn |
| `deterministic_replay` | hai lượt cho stdout/IQ/trace giống nhau |
| `integration_iq_input` | waveform ghi file rồi đọc lại vẫn chạy đúng |
| `arm_elf_audit` | ABI, undefined symbols, section/segment permissions |
| `arm_qemu_smoke` | target thực sự boot tới firmware entry |

Chạy host:

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --parallel
ctest --test-dir build-release --output-on-failure
```

Chạy sanitizer:

```bash
cmake -S . -B build-san \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBB_ENABLE_SANITIZERS=ON
cmake --build build-san --parallel
ctest --test-dir build-san --output-on-failure
```

Chạy static analyzer nếu GCC hỗ trợ:

```bash
cmake -S . -B build-analyzer \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS=-fanalyzer
cmake --build build-analyzer --parallel
ctest --test-dir build-analyzer --output-on-failure
```

## 30.24. File-by-file rebuild checklist

Không sang file tiếp theo chỉ vì file hiện tại compile. Phải pass gate của nó.

| File/nhóm | Bạn phải tự code được | Gate trước khi đi tiếp |
|---|---|---|
| `bb.h` | public types, constants, modem/SoC/PHY API | header compile + contract notes |
| `protocol.h` | PDU type/layout/API | encode/decode plan trên giấy |
| `mac.h` | SDU/LCID/API | canonical format fixture |
| `soc.h` | descriptor flags/layout/API | exact 32-byte diagram |
| `codec.c` | saturation, CRC, QAM, FFT, oscillator | primitive unit tests |
| `channel.c` | delay/CFO/noise deterministic | identity + impairment tests |
| `phy.c` | TX/RX waveform pipeline | ideal rồi impaired round trip |
| `memory.c` | pool/handle/generation | stale/double/full tests |
| `runtime.c` | ring/task/IRQ/timer/DMA orchestration | `unit_runtime` |
| `dma_descriptor.c` | byte codec + validation | malformed descriptor sweep |
| `mmio.c` | aligned platform accessor | invalid address/alignment tests |
| `soc.c` | register model, accelerator/DMA/IRQ step | device completion tests |
| `protocol.c` | PDU wire codec + IE budget | truncated/canonical tests |
| `ipc.c` | BBIP/TLV/CRC/ring/doorbell | `(epoch,seq)` tests |
| `mac.c` | multiplex/demux/BSR/HARQ | `unit_mac` + HARQ tests |
| `rlc_um.c` | SN12 reorder/timer | gap/wrap/duplicate tests |
| `pdcp.c` | COUNT/SN18/replay/reorder | replay/wrap/reassembly tests |
| `transitions.c` | legal RRC/NAS transitions | transition table test |
| `state_machine.c` | guarded actions | wrong-state no-mutation test |
| `security.c` | CMAC/NIA2 + async job path | vectors + bad tag before state |
| `stack.c` | layer queue/handle data flow | no shortcut audit |
| `trace.c` | deterministic checkpoint hook | monotonic tick/schema test |
| `network_peer.c` | peer protocol/waveform state | không truy cập UE internals |
| `main.c` | parser, runner, RF backend, final gate | full integration |
| startup/linker | boot image và memory contract | ELF audit + QEMU smoke |

## 30.25. Cách debug rebuild mà không mở lời giải

Khi test fail, đi theo thứ tự:

```text
1. Viết lại expected transformation bằng lời.
2. Xác định representation tại điểm fail.
3. Xác định data/state/ownership flow nào sai.
4. Thu nhỏ input tới case tính tay được.
5. Đặt invariant assert/checkpoint trước và sau stage.
6. So sánh intermediate output, không chỉ final output.
7. Viết regression test cho bug.
8. Sửa implementation.
9. Chạy lại toàn gate trước đó.
```

Ví dụ CRC fail:

```text
Không kết luận ngay “channel noise quá lớn”.

Kiểm:
TX bit order
→ symbol mapping
→ resource bin index
→ IFFT/CP
→ timing/CFO correction
→ FFT bin index
→ equalization
→ LLR sign
→ byte reconstruction
→ CRC coverage
```

Ví dụ wrong RRC state:

```text
Kiểm:
PDU integrity
→ parser result
→ event type
→ current state
→ guard
→ action
→ next state
→ timer generation
```

## 30.26. Nhật ký bắt buộc cho mỗi milestone

Tạo `REBUILD_LOG.md`. Mỗi milestone ghi:

```markdown
## Milestone N — tên

### Contract tôi hiểu

### Input/output representation

### Invariant

### Data flow

### State flow

### Ownership flow

### Tests tôi tự viết

### Bug tôi gặp và nguyên nhân thật

### Điều tôi có thể viết lại không nhìn code

### Điều còn chưa chắc
```

Nếu note chỉ ghi “copy code xong, test pass” thì milestone không có giá trị học.

## 30.27. Ba lần rebuild

### Lần 1 — Rebuild có hướng dẫn

Được dùng toàn bộ giáo trình. Không mở source reference cho tới khi module tự
viết đã có test.

Mục tiêu:

```text
hiểu dependency
viết được contract
biết chia module
debug được từng stage
```

### Lần 2 — Rebuild chỉ dùng đề và notes

Xóa hoặc chuyển repo lần 1 ra khỏi workspace. Tạo repo mới.

Được dùng:

```text
đề bài
REBUILD_LOG.md
test vectors
spec
```

Không dùng full solution của các bài và không mở repo lần 1.

Mục tiêu:

```text
tự suy ra implementation từ contract
```

### Lần 3 — Closed-book timed rebuild

Đây là bài thi cuối. Chia thành các phiên có thời gian:

```text
Phiên 1: public API + build + codec
Phiên 2: channel + PHY ideal
Phiên 3: impaired RX + sync
Phiên 4: memory/runtime/SoC
Phiên 5: protocol/MAC/RLC/PDCP
Phiên 6: control/security/IPC
Phiên 7: peer/host/E2E
Phiên 8: ARM + defensive audit
```

Không cần nhanh ngay lần đầu. Timed rebuild dùng để phát hiện module nào bạn
vẫn chỉ nhớ hình dạng code mà chưa sở hữu mental model.

## 30.28. Rubric tự chấm 100 điểm

| Nhóm | Điểm |
|---|---:|
| C17/build/public contract | 8 |
| codec/channel primitive | 10 |
| PHY TX waveform | 8 |
| PHY RX sync/equalize/decode | 12 |
| memory/runtime/IRQ/virtual time | 12 |
| MMIO/DMA/SoC/IPC | 10 |
| MAC/HARQ | 8 |
| RLC/PDCP/replay | 8 |
| RRC/NAS/security | 8 |
| NetworkPeer + host parser | 5 |
| E2E scenario thật qua IQ | 7 |
| defensive/determinism/ARM audit | 4 |
| **Tổng** | **100** |

Điều kiện liệt dù tổng điểm cao:

```text
side-channel payload
hard-code PASS/final counters
NetworkPeer sửa thẳng state UE
malformed input gây memory corruption
copy nguyên reference rồi đổi tên
```

## 30.29. Bằng chứng tốt nghiệp phải giữ lại

Khi hoàn thành, tạo thư mục:

```text
graduation_evidence/
├── build_release.log
├── ctest_release.log
├── ctest_sanitizer.log
├── static_analyzer.log
├── arm_size.txt
├── arm_undefined_symbols.txt
├── arm_readelf.txt
├── qemu_smoke.log
├── run1.stdout
├── run2.stdout
├── run1.iq
├── run2.iq
├── run1.trace.jsonl
├── run2.trace.jsonl
├── hashes.txt
└── REBUILD_LOG.md
```

Phải chứng minh:

```text
cmp run1.stdout run2.stdout
cmp run1.iq run2.iq
cmp run1.trace.jsonl run2.trace.jsonl
```

Kết quả E2E mục tiêu có dạng:

```text
MODEM_RESULT status=PASS state=DATA tx_bytes=4096 rx_bytes=4096 crc_fail=1 recoveries=2
```

Con số không phải thứ để hard-code. Nó là bằng chứng cuối của cả chuỗi
execution thật.

## 30.30. Câu hỏi cuối trước khi tự nhận “đã code lại được lời giải”

Không nhìn source và tự trả lời:

1. Một byte SDU biến đổi qua những representation nào trước khi thành IQ?
2. RX tìm đầu frame mà không được truyền timing index bằng cách nào?
3. Vì sao FFT/decoder không chạy trong IRQ top-half?
4. Handle generation ngăn stale reference theo cơ chế nào?
5. Descriptor được publish cho DMA theo thứ tự nào?
6. Vì sao parser phải validate toàn frame trước khi commit child event?
7. HARQ soft buffer gắn với process/COUNT nào?
8. RLC SN12 và PDCP SN18 xử lý wrap khác compare integer thường ra sao?
9. Integrity failure bị chặn trước state mutation ở boundary nào?
10. RRC và NAS có state machine riêng nhưng phối hợp thế nào?
11. Link loss làm sạch timer/queue/handle nào rồi khởi động recovery ra sao?
12. NetworkPeer chứng minh không có side-channel bằng kiến trúc nào?
13. Vì sao virtual time và seed cố định làm test reproducible?
14. Host và Arm dùng chung core nhưng khác platform shim ở đâu?
15. Test nào sẽ bắt bug nếu bạn vô tình serialize padding của struct?

Nếu trả lời được, xóa một module bất kỳ và viết lại nó. Nếu vẫn làm được mà
không nhìn reference, module đó mới thực sự thuộc về bạn.

## 30.31. Checkpoint cuối cùng của toàn khóa

```text
Đóng reference.
Tạo repo trắng.
Tự viết build system.
Tự viết từng module theo dependency.
Tự viết test trước khi tích hợp.
Cho payload đi thật qua IQ và toàn stack.
Inject lỗi và quan sát recovery.
Build host + Arm.
Chạy sanitizer + deterministic replay.
Xóa repo và làm lại lần hai.
```

Khi hoàn thành chuỗi này, mục tiêu không còn là:

```text
“tôi đọc hiểu lời giải”
```

mà là:

```text
“tôi có thể tự engineering lại lời giải từ contract và mental model của mình.”
```
# PHẦN XXXI — MASTERY FORGE: BIẾN Ý NGHĨ MODEM/BASEBAND THÀNH CODE

> Phần này **chỉ bổ sung**, không thay thế và không rút gọn bất kỳ phần nào ở
> trên. Toàn bộ bài cũ, lời giải, Wizard Lab, phao source và lộ trình rebuild
> vẫn còn nguyên. Đây là cây cầu còn thiếu giữa “làm được từng bài” và “tự nghĩ
> ra một thay đổi rồi tự đưa nó vào project”.

## 31.0. Kết luận của lần duyệt cuối

Tài liệu cũ đã đủ rộng để học modem/baseband và đã có source cho các bài. Tuy
nhiên, chỉ đọc rồi gõ lại từng đáp án vẫn có thể tạo ra một người **nhớ code**
chứ chưa chắc tạo ra một người **sinh được code**. Vì vậy phần bổ sung này ép
người học lặp thêm bốn năng lực:

| Năng lực | Bằng chứng bắt buộc |
|---|---|
| Nói ý tưởng thành contract | Viết được input, output, state, owner, lỗi và invariant |
| Chẻ contract thành helper | Mỗi helper làm một việc, có prototype và test riêng |
| Nối helper thành feature | Feature đi qua API, implementation, test, scenario và trace |
| Tự biến thể ngoài đáp án | Thêm một hành vi reference không có mà test cũ vẫn xanh |

Định nghĩa “thành thạo code” của khóa này không phải là gõ nhanh. Bạn phải làm
được vòng sau từ một file trắng:

```text
ý nghĩ bằng tiếng Việt
→ ví dụ cụ thể bằng giấy
→ representation C
→ prototype
→ implementation nhỏ nhất
→ test quan sát được
→ edge case và invariant
→ nối vào project
→ tự tạo biến thể khác
→ xóa code và viết lại
```

Nếu một bước còn phải đoán, dừng ngay ở bước đó và viết ra điều chưa rõ. Không
che một lỗ hổng concept bằng cách paste thêm code.

## 31.1. Giao thức học bắt buộc — không cần lật lên xuống

Mỗi lab mới trong phần này tự chứa đủ đề, kiến thức, lệnh build, output, debug
và phao. Làm đúng thứ tự tại chỗ:

1. Đọc đề và **đóng khối phao**.
2. Viết ba ví dụ input/output bằng tay.
3. Gạch chân danh từ để tìm type; gạch chân động từ để tìm function.
4. Viết prototype trước thân hàm.
5. Viết test fail hoặc ít nhất viết `assert` mong đợi trước.
6. Code happy path nhỏ nhất.
7. Compile với warning nghiêm ngặt.
8. Thêm `NULL`, biên mảng, overflow, wrap hoặc stale state phù hợp.
9. Chạy sanitizer.
10. Chỉ lúc đó mở phao ngay dưới bài.
11. Đóng phao, tạo file mới và viết lại không nhìn.
12. Thay một yêu cầu để buộc bản thân sửa code bằng hiểu biết.

Một bài chỉ được đánh dấu xong khi có đủ bốn lượt:

| Lượt | Được nhìn gì? | Phải tạo ra gì? |
|---|---|---|
| Guided | Được đọc hướng dẫn, chưa mở phao | File compile được |
| Closed-book | Không nhìn lời giải | Viết lại từ trắng và pass test |
| Mutation | Tự đổi một yêu cầu | Test mới + implementation mới |
| Explain | Không nhìn code | Giải thích type, lifetime, owner, state và invariant |

## 31.2. “Compiler trong đầu” — biến một câu quái dị thành source

Ví dụ ý nghĩ:

> “Tao muốn hút năng lượng khỏi subcarrier `-3`, đẩy sang subcarrier `+5`, rồi
> đo xem peak đã chạy thật chưa.”

Đừng code ngay. Biên dịch ý nghĩ đó qua worksheet:

| Câu hỏi | Câu trả lời của ví dụ |
|---|---|
| Object nào tồn tại? | Mảng resource-grid gồm complex IQ bins |
| Một object biểu diễn thế nào? | `struct wh_cpx16 { int16_t i; int16_t q; }` |
| `-3` có phải array index không? | Không; cần helper logical-bin → array-index |
| Động tác là gì? | Lấy source, cộng có saturation vào destination, zero source |
| Ai sở hữu buffer? | Caller; helper chỉ mượn và sửa trong thời gian call |
| Lỗi nào phải chặn? | `NULL`, NFFT lẻ/zero, bin ngoài miền, source=destination |
| Quan sát thành công bằng gì? | Energy bảo toàn gần đúng, source zero, peak đổi vị trí |
| Invariant modem là gì? | Không truy cập ngoài grid; fixed-point không wrap |

Từ bảng đó mới sinh prototype:

```c
int wh_grid_move(struct wh_cpx16 *grid, size_t nfft,
                 int32_t source_bin, int32_t destination_bin);
```

Quy tắc tổng quát:

- Danh từ có state/lifetime riêng thường thành `struct`.
- Một tập trạng thái loại trừ nhau thường thành `enum`.
- Động từ nhỏ, có input/output rõ thành function.
- Thứ thay đổi theo platform thành ops table + `void *context`.
- Identity đi qua queue/domain thành handle, không thành raw pointer.
- Dữ liệu đi qua wire/file/MMIO thành byte codec fixed-width, không serialize
  nguyên `struct`.
- Điều cần chứng minh thành `assert`, observer hoặc trace checkpoint.

## 31.3. Bộ helper nền — 20 viên gạch có source đầy đủ

Đây không phải library để phụ thuộc mãi mãi. Nó là **phao luyện cơ tay**. Làm
theo ba vòng:

1. Tự viết từng prototype và test trước khi nhìn source.
2. Đối chiếu source, sửa cho warning sạch.
3. Xóa `wizard_helpers.c`, chỉ giữ header + test rồi viết lại toàn bộ.

### 31.3.1. Đề bài nhỏ

Tạo ba file:

```text
practice/mastery/wizard_helpers.h
practice/mastery/wizard_helpers.c
practice/mastery/test_wizard_helpers.c
```

Bộ helper phải làm được:

| Nhóm | Helper | Ý nghĩa baseband |
|---|---|---|
| Fixed-point | saturation, Q15 scale | Chặn wrap khi xử lý IQ/LLR |
| Bits/wire | get/set bit, LE16/LE32 | CRC, PDU, descriptor, IQ container |
| OFDM grid | đổi logical bin, set/move/notch | Chủ động thao tác resource grid |
| Observer | energy, peak bin, digest | Biết code đã làm gì thay vì chỉ thấy PASS |
| Time/state | modular tick, SN12 distance | Timer/RLC đúng khi counter wrap |
| Identity | generation handle compare | Phát hiện stale ownership |
| Mutation | deterministic PRNG, flip bit | Fault injection có thể replay |

### 31.3.2. Tự code trước

Chỉ mở editor và tự khai báo API từ bảng trên. Với mỗi helper, viết ít nhất:

- một happy-path `assert`;
- một boundary `assert`;
- một invalid-input `assert` nếu function có thể nhận input sai.

Không viết 20 thân hàm rồi mới compile. Nhịp đúng là:

```text
prototype 1 → test 1 → implementation 1 → compile
prototype 2 → test 2 → implementation 2 → compile
...
```

### 31.3.3. Phao cứu sinh — header đầy đủ

<details>
<summary>Mở source <code>wizard_helpers.h</code> sau khi đã tự thử</summary>

```c
#ifndef WIZARD_HELPERS_H
#define WIZARD_HELPERS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct wh_cpx16 {
    int16_t i;
    int16_t q;
};

struct wh_handle {
    uint16_t slot;
    uint16_t generation;
    uint32_t length;
};

int16_t wh_sat16(int32_t value);
int16_t wh_scale_q15(int16_t value, int16_t gain_q15);

int wh_get_bit_msb(const uint8_t *bytes, size_t length,
                   size_t bit_index, uint8_t *bit);
int wh_set_bit_msb(uint8_t *bytes, size_t length,
                   size_t bit_index, uint8_t bit);

int wh_put_u16_le(uint8_t out[2], uint16_t value);
int wh_get_u16_le(const uint8_t in[2], uint16_t *value);
int wh_put_u32_le(uint8_t out[4], uint32_t value);
int wh_get_u32_le(const uint8_t in[4], uint32_t *value);

int wh_logical_bin_to_index(int32_t logical_bin, size_t nfft,
                            size_t *index);
int wh_grid_set(struct wh_cpx16 *grid, size_t nfft,
                int32_t logical_bin, struct wh_cpx16 value);
int wh_grid_move(struct wh_cpx16 *grid, size_t nfft,
                 int32_t source_bin, int32_t destination_bin);
int wh_grid_notch(struct wh_cpx16 *grid, size_t nfft,
                  int32_t first_bin, size_t width);
uint64_t wh_grid_energy(const struct wh_cpx16 *grid, size_t count);
int wh_peak_logical_bin(const struct wh_cpx16 *grid, size_t nfft,
                        int32_t *logical_bin, uint64_t *energy);

bool wh_tick_after_or_equal_u32(uint32_t now, uint32_t deadline);
uint16_t wh_sn12_distance(uint16_t from, uint16_t to);
bool wh_handle_equal(struct wh_handle left, struct wh_handle right);

uint32_t wh_xorshift32(uint32_t *state);
int wh_flip_bit(uint8_t *bytes, size_t length, size_t bit_index);
uint32_t wh_digest32(const uint8_t *bytes, size_t length);

#endif
```

</details>

### 31.3.4. Phao cứu sinh — implementation đầy đủ

<details>
<summary>Mở source <code>wizard_helpers.c</code> sau khi test của bạn đã fail đúng lý do</summary>

```c
#include "wizard_helpers.h"

#include <limits.h>

int16_t wh_sat16(int32_t value)
{
    if (value > INT16_MAX) {
        return INT16_MAX;
    }
    if (value < INT16_MIN) {
        return INT16_MIN;
    }
    return (int16_t)value;
}

int16_t wh_scale_q15(int16_t value, int16_t gain_q15)
{
    int32_t product = (int32_t)value * (int32_t)gain_q15;
    if (product >= 0) {
        product += INT32_C(16384);
    } else {
        product -= INT32_C(16384);
    }
    return wh_sat16(product / INT32_C(32768));
}

static bool bit_position_valid(size_t length, size_t bit_index)
{
    if (length > (SIZE_MAX / 8U)) {
        return false;
    }
    return bit_index < (length * 8U);
}

int wh_get_bit_msb(const uint8_t *bytes, size_t length,
                   size_t bit_index, uint8_t *bit)
{
    size_t byte_index;
    uint8_t shift;
    if ((bytes == NULL) || (bit == NULL) ||
        !bit_position_valid(length, bit_index)) {
        return -1;
    }
    byte_index = bit_index / 8U;
    shift = (uint8_t)(7U - (uint8_t)(bit_index % 8U));
    *bit = (uint8_t)((bytes[byte_index] >> shift) & 1U);
    return 0;
}

int wh_set_bit_msb(uint8_t *bytes, size_t length,
                   size_t bit_index, uint8_t bit)
{
    size_t byte_index;
    uint8_t shift;
    uint8_t mask;
    if ((bytes == NULL) || (bit > 1U) ||
        !bit_position_valid(length, bit_index)) {
        return -1;
    }
    byte_index = bit_index / 8U;
    shift = (uint8_t)(7U - (uint8_t)(bit_index % 8U));
    mask = (uint8_t)(1U << shift);
    if (bit != 0U) {
        bytes[byte_index] |= mask;
    } else {
        bytes[byte_index] &= (uint8_t)~mask;
    }
    return 0;
}

int wh_put_u16_le(uint8_t out[2], uint16_t value)
{
    if (out == NULL) {
        return -1;
    }
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8U);
    return 0;
}

int wh_get_u16_le(const uint8_t in[2], uint16_t *value)
{
    if ((in == NULL) || (value == NULL)) {
        return -1;
    }
    *value = (uint16_t)((uint16_t)in[0] |
                        (uint16_t)((uint16_t)in[1] << 8U));
    return 0;
}

int wh_put_u32_le(uint8_t out[4], uint32_t value)
{
    if (out == NULL) {
        return -1;
    }
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8U);
    out[2] = (uint8_t)(value >> 16U);
    out[3] = (uint8_t)(value >> 24U);
    return 0;
}

int wh_get_u32_le(const uint8_t in[4], uint32_t *value)
{
    if ((in == NULL) || (value == NULL)) {
        return -1;
    }
    *value = (uint32_t)in[0] |
             ((uint32_t)in[1] << 8U) |
             ((uint32_t)in[2] << 16U) |
             ((uint32_t)in[3] << 24U);
    return 0;
}

int wh_logical_bin_to_index(int32_t logical_bin, size_t nfft,
                            size_t *index)
{
    int64_t half;
    int64_t logical = logical_bin;
    if ((index == NULL) || (nfft == 0U) || ((nfft & 1U) != 0U) ||
        (nfft > (size_t)INT32_MAX)) {
        return -1;
    }
    half = (int64_t)(nfft / 2U);
    if ((logical < -half) || (logical >= half)) {
        return -1;
    }
    if (logical >= 0) {
        *index = (size_t)logical;
    } else {
        *index = nfft - (size_t)(-logical);
    }
    return 0;
}

int wh_grid_set(struct wh_cpx16 *grid, size_t nfft,
                int32_t logical_bin, struct wh_cpx16 value)
{
    size_t index;
    if ((grid == NULL) ||
        (wh_logical_bin_to_index(logical_bin, nfft, &index) != 0)) {
        return -1;
    }
    grid[index] = value;
    return 0;
}

int wh_grid_move(struct wh_cpx16 *grid, size_t nfft,
                 int32_t source_bin, int32_t destination_bin)
{
    size_t source_index;
    size_t destination_index;
    int32_t sum_i;
    int32_t sum_q;
    if ((grid == NULL) || (source_bin == destination_bin) ||
        (wh_logical_bin_to_index(source_bin, nfft, &source_index) != 0) ||
        (wh_logical_bin_to_index(destination_bin, nfft,
                                 &destination_index) != 0)) {
        return -1;
    }
    sum_i = (int32_t)grid[destination_index].i +
            (int32_t)grid[source_index].i;
    sum_q = (int32_t)grid[destination_index].q +
            (int32_t)grid[source_index].q;
    grid[destination_index].i = wh_sat16(sum_i);
    grid[destination_index].q = wh_sat16(sum_q);
    grid[source_index].i = 0;
    grid[source_index].q = 0;
    return 0;
}

int wh_grid_notch(struct wh_cpx16 *grid, size_t nfft,
                  int32_t first_bin, size_t width)
{
    size_t offset;
    if ((grid == NULL) || (width == 0U) ||
        (width > (size_t)INT32_MAX)) {
        return -1;
    }
    for (offset = 0U; offset < width; ++offset) {
        int64_t logical = (int64_t)first_bin + (int64_t)offset;
        size_t index;
        if ((logical < INT32_MIN) || (logical > INT32_MAX) ||
            (wh_logical_bin_to_index((int32_t)logical, nfft, &index) != 0)) {
            return -1;
        }
    }
    for (offset = 0U; offset < width; ++offset) {
        size_t index;
        (void)wh_logical_bin_to_index(
            (int32_t)((int64_t)first_bin + (int64_t)offset), nfft, &index);
        grid[index].i = 0;
        grid[index].q = 0;
    }
    return 0;
}

static uint64_t sample_energy(struct wh_cpx16 sample)
{
    int64_t i = sample.i;
    int64_t q = sample.q;
    return (uint64_t)((i * i) + (q * q));
}

uint64_t wh_grid_energy(const struct wh_cpx16 *grid, size_t count)
{
    uint64_t total = 0U;
    size_t index;
    if (grid == NULL) {
        return 0U;
    }
    for (index = 0U; index < count; ++index) {
        uint64_t term = sample_energy(grid[index]);
        if (term > (UINT64_MAX - total)) {
            return UINT64_MAX;
        }
        total += term;
    }
    return total;
}

int wh_peak_logical_bin(const struct wh_cpx16 *grid, size_t nfft,
                        int32_t *logical_bin, uint64_t *energy)
{
    size_t index;
    size_t peak_index = 0U;
    uint64_t peak_energy;
    if ((grid == NULL) || (logical_bin == NULL) || (energy == NULL) ||
        (nfft == 0U) || ((nfft & 1U) != 0U) ||
        (nfft > (size_t)INT32_MAX)) {
        return -1;
    }
    peak_energy = sample_energy(grid[0]);
    for (index = 1U; index < nfft; ++index) {
        uint64_t current = sample_energy(grid[index]);
        if (current > peak_energy) {
            peak_energy = current;
            peak_index = index;
        }
    }
    if (peak_index < (nfft / 2U)) {
        *logical_bin = (int32_t)peak_index;
    } else {
        *logical_bin = (int32_t)peak_index - (int32_t)nfft;
    }
    *energy = peak_energy;
    return 0;
}

bool wh_tick_after_or_equal_u32(uint32_t now, uint32_t deadline)
{
    return (uint32_t)(now - deadline) < UINT32_C(0x80000000);
}

uint16_t wh_sn12_distance(uint16_t from, uint16_t to)
{
    return (uint16_t)((to - from) & UINT16_C(0x0FFF));
}

bool wh_handle_equal(struct wh_handle left, struct wh_handle right)
{
    return (left.slot == right.slot) &&
           (left.generation == right.generation) &&
           (left.length == right.length);
}

uint32_t wh_xorshift32(uint32_t *state)
{
    uint32_t value;
    if (state == NULL) {
        return 0U;
    }
    value = (*state == 0U) ? UINT32_C(0x6D2B79F5) : *state;
    value ^= value << 13U;
    value ^= value >> 17U;
    value ^= value << 5U;
    *state = value;
    return value;
}

int wh_flip_bit(uint8_t *bytes, size_t length, size_t bit_index)
{
    size_t byte_index;
    uint8_t shift;
    if ((bytes == NULL) || !bit_position_valid(length, bit_index)) {
        return -1;
    }
    byte_index = bit_index / 8U;
    shift = (uint8_t)(7U - (uint8_t)(bit_index % 8U));
    bytes[byte_index] ^= (uint8_t)(1U << shift);
    return 0;
}

uint32_t wh_digest32(const uint8_t *bytes, size_t length)
{
    uint32_t digest = UINT32_C(2166136261);
    size_t index;
    if (bytes == NULL) {
        return 0U;
    }
    for (index = 0U; index < length; ++index) {
        digest ^= bytes[index];
        digest *= UINT32_C(16777619);
    }
    return digest;
}
```

</details>

### 31.3.5. Phao cứu sinh — test đầy đủ

<details>
<summary>Mở source <code>test_wizard_helpers.c</code> sau khi đã tự viết test</summary>

```c
#include "wizard_helpers.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static void test_fixed_point(void)
{
    assert(wh_sat16(INT32_C(40000)) == INT16_MAX);
    assert(wh_sat16(INT32_C(-40000)) == INT16_MIN);
    assert(wh_sat16(INT32_C(1234)) == INT16_C(1234));
    assert(wh_scale_q15(INT16_C(20000), INT16_C(16384)) ==
           INT16_C(10000));
    assert(wh_scale_q15(INT16_C(-20000), INT16_C(16384)) ==
           INT16_C(-10000));
}

static void test_bits_and_wire(void)
{
    uint8_t bytes[4] = {0U, 0U, 0U, 0U};
    uint8_t bit = 9U;
    uint16_t value16 = 0U;
    uint32_t value32 = 0U;

    assert(wh_set_bit_msb(bytes, sizeof(bytes), 0U, 1U) == 0);
    assert(wh_set_bit_msb(bytes, sizeof(bytes), 9U, 1U) == 0);
    assert(bytes[0] == UINT8_C(0x80));
    assert(bytes[1] == UINT8_C(0x40));
    assert(wh_get_bit_msb(bytes, sizeof(bytes), 9U, &bit) == 0);
    assert(bit == 1U);
    assert(wh_get_bit_msb(bytes, sizeof(bytes), 32U, &bit) != 0);
    assert(wh_set_bit_msb(bytes, sizeof(bytes), 0U, 2U) != 0);

    assert(wh_put_u16_le(bytes, UINT16_C(0x1234)) == 0);
    assert(bytes[0] == UINT8_C(0x34));
    assert(bytes[1] == UINT8_C(0x12));
    assert(wh_get_u16_le(bytes, &value16) == 0);
    assert(value16 == UINT16_C(0x1234));

    assert(wh_put_u32_le(bytes, UINT32_C(0x78563412)) == 0);
    assert(wh_get_u32_le(bytes, &value32) == 0);
    assert(value32 == UINT32_C(0x78563412));
}

static void test_grid(void)
{
    struct wh_cpx16 grid[16];
    struct wh_cpx16 source = {INT16_C(1000), INT16_C(-2000)};
    struct wh_cpx16 destination = {INT16_C(100), INT16_C(200)};
    size_t source_index = 0U;
    size_t destination_index = 0U;
    int32_t peak = 99;
    uint64_t peak_energy = 0U;

    (void)memset(grid, 0, sizeof(grid));
    assert(wh_logical_bin_to_index(-3, 16U, &source_index) == 0);
    assert(source_index == 13U);
    assert(wh_logical_bin_to_index(5, 16U, &destination_index) == 0);
    assert(destination_index == 5U);
    assert(wh_logical_bin_to_index(8, 16U, &destination_index) != 0);

    assert(wh_grid_set(grid, 16U, -3, source) == 0);
    assert(wh_grid_set(grid, 16U, 5, destination) == 0);
    assert(wh_grid_energy(grid, 16U) == UINT64_C(5050000));
    assert(wh_peak_logical_bin(grid, 16U, &peak, &peak_energy) == 0);
    assert(peak == -3);

    assert(wh_grid_move(grid, 16U, -3, 5) == 0);
    assert(grid[source_index].i == 0);
    assert(grid[source_index].q == 0);
    assert(grid[destination_index].i == INT16_C(1100));
    assert(grid[destination_index].q == INT16_C(-1800));
    assert(wh_peak_logical_bin(grid, 16U, &peak, &peak_energy) == 0);
    assert(peak == 5);

    assert(wh_grid_notch(grid, 16U, 4, 3U) == 0);
    assert(grid[destination_index].i == 0);
    assert(grid[destination_index].q == 0);
    assert(wh_grid_notch(grid, 16U, 7, 2U) != 0);
}

static void test_time_identity_and_mutation(void)
{
    struct wh_handle first = {1U, 7U, 32U};
    struct wh_handle same = {1U, 7U, 32U};
    struct wh_handle reused = {1U, 8U, 32U};
    uint8_t payload[3] = {UINT8_C(0xAA), UINT8_C(0x55), UINT8_C(0xF0)};
    uint32_t seed_a = UINT32_C(1234);
    uint32_t seed_b = UINT32_C(1234);
    uint32_t before;

    assert(wh_tick_after_or_equal_u32(100U, 100U));
    assert(wh_tick_after_or_equal_u32(2U, UINT32_MAX - 1U));
    assert(!wh_tick_after_or_equal_u32(UINT32_MAX, 5U));
    assert(wh_sn12_distance(UINT16_C(4095), 0U) == 1U);
    assert(wh_sn12_distance(UINT16_C(4094), 1U) == 3U);
    assert(wh_handle_equal(first, same));
    assert(!wh_handle_equal(first, reused));

    assert(wh_xorshift32(&seed_a) == wh_xorshift32(&seed_b));
    assert(wh_xorshift32(&seed_a) == wh_xorshift32(&seed_b));
    before = wh_digest32(payload, sizeof(payload));
    assert(wh_flip_bit(payload, sizeof(payload), 9U) == 0);
    assert(wh_digest32(payload, sizeof(payload)) != before);
    assert(wh_flip_bit(payload, sizeof(payload), 9U) == 0);
    assert(wh_digest32(payload, sizeof(payload)) == before);
    assert(wh_flip_bit(payload, sizeof(payload), 24U) != 0);
}

int main(void)
{
    test_fixed_point();
    test_bits_and_wire();
    test_grid();
    test_time_identity_and_mutation();
    puts("wizard_helpers passed");
    return 0;
}
```

</details>

Build và chạy:

```bash
mkdir -p build/practice
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -Wstrict-prototypes -Wmissing-prototypes \
    -fsanitize=address,undefined -g \
    -Ipractice/mastery \
    practice/mastery/wizard_helpers.c \
    practice/mastery/test_wizard_helpers.c \
    -o build/practice/test_wizard_helpers
./build/practice/test_wizard_helpers
```

Output:

```text
wizard_helpers passed
```

### 31.3.6. Debug có chủ đích

Tự cài lần lượt bốn bug, nhìn test bắt chúng rồi hoàn tác:

1. Đổi MSB-first thành LSB-first trong `wh_get_bit_msb`.
2. Bỏ saturation trong `wh_grid_move`.
3. So sánh tick bằng `now >= deadline`.
4. Bỏ `generation` khỏi `wh_handle_equal`.

Sau mỗi bug, ghi vào `REBUILD_LOG.md`:

```text
Bug tôi cài:
Test nào đỏ:
Vì sao test đó nhìn thấy bug:
Nếu vào modem thật, symptom sẽ xuất hiện ở layer nào:
Invariant sửa lại là gì:
```

### Sau khi code: ý nghĩa modem/baseband

Bộ helper này là các “động từ” cơ bản của modem số: đọc bit, bảo toàn
representation, thao tác bin, giữ arithmetic trong miền biểu diễn, so counter
quanh wrap, xác minh identity và tạo lỗi deterministic. Khi nghĩ ra một trò
mới, bạn không chờ ai trao API; bạn ghép hoặc viết thêm những động từ nhỏ như
vậy rồi buộc chúng chứng minh hành vi bằng test.

### 31.3.7. Viết lại không nhìn

Xóa **chỉ file luyện của bạn** `wizard_helpers.c`, không xóa tài liệu hay source
reference. Viết lại theo nhóm:

```text
Ngày 1: fixed-point + bits
Ngày 2: endian + grid indexing
Ngày 3: move/notch + observers
Ngày 4: tick/SN/handle + mutation
Ngày 5: viết lại cả file trong một phiên
```

Gate: test cũ phải pass mà không đổi một dòng test nào.

## 31.4. Wizard Lab tích hợp — dùng C như một ngôn ngữ kịch bản baseband

Mục tiêu của lab này không phải viết parser text. Mục tiêu là học cách biến
một chuỗi ý tưởng thành **data**, rồi có một engine C thực thi data đó. Cách này
được dùng trong event script, test vector, register sequence, fault schedule và
state-machine table.

### 31.4.1. Đề bài nhỏ

Viết một runner hiểu năm command:

| Command | Ý nghĩa |
|---|---|
| `SET_BIN` | Đặt complex value vào một logical subcarrier |
| `MOVE_BIN` | Dồn source sang destination |
| `NOTCH` | Zero một dải subcarrier |
| `EXPECT_PEAK` | Assert peak đang ở bin mong đợi |
| `EXPECT_ZERO` | Assert một bin đã về zero |

Scenario đầu tiên:

```text
SET_BIN -3 = (12000,1000)
SET_BIN +5 = (100,200)
EXPECT_PEAK -3
MOVE_BIN -3 → +5
EXPECT_PEAK +5
NOTCH từ +4, rộng 3 bin
EXPECT_ZERO +5
```

### 31.4.2. Tự đoán trước khi code

Trả lời bằng giấy:

1. Command là code hay data?
2. `enum` biểu diễn cái gì, `struct` biểu diễn cái gì?
3. Runner sở hữu grid hay chỉ mượn grid?
4. Mutation và assertion có nên dùng chung return code không?
5. Khi command thứ tư fail, làm sao biết command nào gây lỗi?

### 31.4.3. Học đúng lượng C cần dùng

- `enum` để tag loại command;
- `struct` để chứa operand;
- mảng `const struct wiz_command[]` làm script;
- `switch` dispatch command;
- function composition: runner gọi helper nhỏ;
- observer tách khỏi mutation;
- fail-fast: dừng đúng command đầu tiên sai.

### 31.4.4. Tự code

Tạo `practice/mastery/grid_script.c`. Bạn được dùng
`wizard_helpers.h/.c` vừa tự viết. Tự code trước, chưa mở phao.

### 31.4.5. Phao cứu sinh — source đầy đủ

<details>
<summary>Mở <code>grid_script.c</code> sau khi đã tự chạy ít nhất một lần</summary>

```c
#include "wizard_helpers.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define WIZ_NFFT 16U

enum wiz_opcode {
    WIZ_SET_BIN = 1,
    WIZ_MOVE_BIN,
    WIZ_NOTCH,
    WIZ_EXPECT_PEAK,
    WIZ_EXPECT_ZERO
};

struct wiz_command {
    enum wiz_opcode opcode;
    int32_t first;
    int32_t second;
    int16_t i;
    int16_t q;
    size_t width;
};

struct wiz_runner {
    struct wh_cpx16 grid[WIZ_NFFT];
    size_t executed;
};

static int expect_peak(const struct wiz_runner *runner, int32_t expected)
{
    int32_t actual = 0;
    uint64_t energy = 0U;
    if (wh_peak_logical_bin(runner->grid, WIZ_NFFT,
                            &actual, &energy) != 0) {
        return -1;
    }
    (void)printf("observer peak=%" PRId32 " energy=%" PRIu64 "\n",
                 actual, energy);
    return (actual == expected) ? 0 : -1;
}

static int expect_zero(const struct wiz_runner *runner, int32_t logical_bin)
{
    size_t index = 0U;
    if (wh_logical_bin_to_index(logical_bin, WIZ_NFFT, &index) != 0) {
        return -1;
    }
    return ((runner->grid[index].i == 0) &&
            (runner->grid[index].q == 0)) ? 0 : -1;
}

static int execute_one(struct wiz_runner *runner,
                       const struct wiz_command *command)
{
    struct wh_cpx16 value;
    if ((runner == NULL) || (command == NULL)) {
        return -1;
    }
    value.i = command->i;
    value.q = command->q;
    switch (command->opcode) {
    case WIZ_SET_BIN:
        return wh_grid_set(runner->grid, WIZ_NFFT,
                           command->first, value);
    case WIZ_MOVE_BIN:
        return wh_grid_move(runner->grid, WIZ_NFFT,
                            command->first, command->second);
    case WIZ_NOTCH:
        return wh_grid_notch(runner->grid, WIZ_NFFT,
                             command->first, command->width);
    case WIZ_EXPECT_PEAK:
        return expect_peak(runner, command->first);
    case WIZ_EXPECT_ZERO:
        return expect_zero(runner, command->first);
    default:
        return -1;
    }
}

static int run_script(struct wiz_runner *runner,
                      const struct wiz_command *commands,
                      size_t command_count)
{
    size_t index;
    if ((runner == NULL) ||
        ((commands == NULL) && (command_count != 0U))) {
        return -1;
    }
    for (index = 0U; index < command_count; ++index) {
        if (execute_one(runner, &commands[index]) != 0) {
            (void)fprintf(stderr, "command %zu failed\n", index);
            return -1;
        }
        runner->executed++;
    }
    return 0;
}

int main(void)
{
    static const struct wiz_command scenario[] = {
        {WIZ_SET_BIN, -3, 0, INT16_C(12000), INT16_C(1000), 0U},
        {WIZ_SET_BIN,  5, 0, INT16_C(100), INT16_C(200), 0U},
        {WIZ_EXPECT_PEAK, -3, 0, 0, 0, 0U},
        {WIZ_MOVE_BIN, -3, 5, 0, 0, 0U},
        {WIZ_EXPECT_PEAK, 5, 0, 0, 0, 0U},
        {WIZ_NOTCH, 4, 0, 0, 0, 3U},
        {WIZ_EXPECT_ZERO, 5, 0, 0, 0, 0U}
    };
    struct wiz_runner runner;

    (void)memset(&runner, 0, sizeof(runner));
    assert(run_script(&runner, scenario,
                      sizeof(scenario) / sizeof(scenario[0])) == 0);
    assert(runner.executed ==
           sizeof(scenario) / sizeof(scenario[0]));
    puts("grid_script passed");
    return 0;
}
```

</details>

Build:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -Wstrict-prototypes -Wmissing-prototypes \
    -fsanitize=address,undefined -g \
    -Ipractice/mastery \
    practice/mastery/wizard_helpers.c \
    practice/mastery/grid_script.c \
    -o build/practice/grid_script
./build/practice/grid_script
```

Output tối thiểu:

```text
observer peak=-3 energy=145000000
observer peak=5 energy=147850000
grid_script passed
```

### 31.4.6. Debug

- Nếu peak vẫn là `-3` sau `MOVE_BIN`, kiểm tra mapping logical bin âm.
- Nếu energy thành số cực lớn bất ngờ, kiểm tra signed widening trước phép
  nhân.
- Nếu `NOTCH` sửa một phần grid rồi mới báo lỗi, bạn đã validate trong lúc
  commit. Hãy validate toàn range trước rồi mới mutate.
- Nếu script fail mà không biết lệnh nào, in index command; đừng thêm `printf`
  rải rác trong helper toán học.

### 31.4.7. Wizard mutation — tự nối, nhưng vẫn có phao

Tự thêm command `WIZ_SCALE_RANGE`:

```text
operand: first_bin, width, gain_q15
effect:  scale I và Q của cả range
rule:    validate toàn range trước khi sửa
test:    gain 0.5 làm (12000,1000) thành xấp xỉ (6000,500)
```

Hint 1: dùng `wh_logical_bin_to_index` và `wh_scale_q15`.

Hint 2: thêm `int16_t gain_q15` vào command hoặc tái dùng field `i` với comment
rõ; cách sạch hơn là đổi operand thành `union`, nhưng chỉ làm sau khi bản đơn
giản pass.

<details>
<summary>Phao source cho nhánh <code>WIZ_SCALE_RANGE</code></summary>

```c
static int scale_range(struct wiz_runner *runner, int32_t first_bin,
                       size_t width, int16_t gain_q15)
{
    size_t offset;
    if ((runner == NULL) || (width == 0U) ||
        (width > (size_t)INT32_MAX)) {
        return -1;
    }
    for (offset = 0U; offset < width; ++offset) {
        int64_t logical = (int64_t)first_bin + (int64_t)offset;
        size_t index;
        if ((logical < INT32_MIN) || (logical > INT32_MAX) ||
            (wh_logical_bin_to_index((int32_t)logical, WIZ_NFFT,
                                     &index) != 0)) {
            return -1;
        }
    }
    for (offset = 0U; offset < width; ++offset) {
        size_t index;
        (void)wh_logical_bin_to_index(
            (int32_t)((int64_t)first_bin + (int64_t)offset),
            WIZ_NFFT, &index);
        runner->grid[index].i =
            wh_scale_q15(runner->grid[index].i, gain_q15);
        runner->grid[index].q =
            wh_scale_q15(runner->grid[index].q, gain_q15);
    }
    return 0;
}
```

Thêm enum:

```c
WIZ_SCALE_RANGE
```

Thêm nhánh dispatch:

```c
case WIZ_SCALE_RANGE:
    return scale_range(runner, command->first,
                       command->width, command->i);
```

Thêm test command trước `MOVE_BIN`:

```c
{WIZ_SCALE_RANGE, -3, 0, INT16_C(16384), 0, 1U}
```

</details>

### Sau khi code: ý nghĩa modem/baseband

Bạn vừa tạo một mini experiment engine. Cùng pattern này có thể điều khiển
channel impairment, register write, fault injection, timer event, DMA completion
và protocol mutation. “Biết nghịch” ở đây là biến thử nghiệm thành data có thể
replay, không phải sửa bừa vài byte rồi hy vọng thấy điều thú vị.

### 31.4.8. Viết lại không nhìn

Viết một scenario khác chỉ từ câu sau:

> “Tạo hai tone cân nhau, attenuate tone âm còn 1/2, chuyển tone dương sang bin
> kế bên, rồi chứng minh peak và digest thay đổi đúng hai checkpoint.”

Không được sửa helper để hard-code scenario.

## 31.5. Đọc project như bản đồ, không như một cuốn tiểu thuyết

Khi code project lớn, không đọc từ dòng 1 của file đầu tới dòng cuối của file
cuối. Hãy lần theo một representation hoặc một hành vi.

### 31.5.1. Bốn câu lệnh điều tra bắt buộc

Đứng ở root của `01_mini_ue_end_to_end`:

```bash
# 1. Contract được public ở đâu?
rg -n "bb_crc24a|bb_modem_rx_iq_submit|bb_dma_descriptor_encode" include

# 2. Implementation nằm ở đâu?
rg -n "^[[:space:]]*(int|void|uint32_t|size_t).*bb_crc24a" src

# 3. Ai gọi nó?
rg -n "bb_crc24a\(" src tests

# 4. Test nào định nghĩa hành vi?
rg -n "crc24|rx_iq_submit|descriptor_encode" tests
```

Với state/field không phải function:

```bash
rg -n "generation|rx_next|replay|harq_combines" include src tests
```

Với checkpoint chạy thật:

```bash
rg -n '"SYNC"|"FFT"|"TB_CRC"|"FINAL"' src tests
```

Ghi kết quả vào bảng trước khi sửa:

| Câu hỏi | File/dòng bạn tìm được |
|---|---|
| Type/contract |  |
| Producer |  |
| Consumer |  |
| Owner chuyển ở đâu |  |
| Test happy path |  |
| Test failure path |  |
| Trace/observable |  |

Nếu chưa điền được bảng, bạn chưa đủ thông tin để sửa feature.

### 31.5.2. Quy tắc `static` hay public

| Tình huống | Chọn gì? | Lý do |
|---|---|---|
| Chỉ một `.c` dùng | `static` function | Thu hẹp contract và namespace |
| Nhiều module firmware dùng | Prototype trong internal header | Không phơi ra host/API nếu không cần |
| Host/test/integration cần gọi như API | Public header `include/bb/...` | Có contract ổn định |
| Chỉ test muốn gọi một helper `static` | Test qua public behavior trước | Đừng phá encapsulation chỉ để test dễ |
| Logic đủ phức tạp và có invariant riêng | Tách module nhỏ | Cho phép test độc lập |

Một helper không trở thành “kiến trúc” chỉ vì bạn cho nó vào header. Mức
visibility là một quyết định ownership/dependency.

## 31.6. Vertical Slice Lab — thêm observer IQ thật vào cấu trúc project

Lab này luyện đúng chuỗi:

```text
ý nghĩ → public type/API → source → unit test → CMake → integration point
```

### 31.6.1. Đề bài nhỏ

Ý nghĩ:

> “Sau mỗi stage PHY, tao muốn biết block IQ còn bao nhiêu sample khác zero,
> tổng energy và peak absolute component là bao nhiêu.”

Yêu cầu:

- không heap;
- không `float`;
- không sửa input;
- `NULL` hoặc `count == 0` bị reject;
- energy cộng saturation nếu `uint64_t` sắp overflow;
- `INT16_MIN` có magnitude 32768, không gọi `abs(int16_t)`;
- output chỉ commit sau khi input được validate;
- source dùng được cho host lẫn Arm core.

### 31.6.2. Tự thiết kế trước

Điền worksheet:

```text
Input representation:
Output representation:
Borrowed hay owned:
Miền của peak:
Overflow policy:
Return code:
Test vector nhỏ nhất:
Integration checkpoint:
```

Sau đó tự tạo ba file. Chưa mở phao.

### 31.6.3. Phao source — public contract

<details>
<summary>Mở <code>include/bb/iq_observer.h</code></summary>

```c
#ifndef BB_IQ_OBSERVER_H
#define BB_IQ_OBSERVER_H

#include "bb/bb.h"

#include <stddef.h>
#include <stdint.h>

struct bb_iq_metrics {
    uint64_t energy;
    uint32_t nonzero_samples;
    uint16_t peak_component;
};

int bb_iq_measure(const struct bb_cpx16 *samples, size_t count,
                  struct bb_iq_metrics *metrics);

#endif
```

</details>

### 31.6.4. Phao source — implementation

<details>
<summary>Mở <code>src/fw/trace/iq_observer.c</code></summary>

```c
#include "bb/iq_observer.h"

#include <limits.h>
#include <stdbool.h>

static uint16_t component_magnitude(int16_t value)
{
    if (value >= 0) {
        return (uint16_t)value;
    }
    return (uint16_t)(-(int32_t)value);
}

static uint64_t iq_energy(struct bb_cpx16 sample)
{
    int64_t i = sample.i;
    int64_t q = sample.q;
    return (uint64_t)((i * i) + (q * q));
}

int bb_iq_measure(const struct bb_cpx16 *samples, size_t count,
                  struct bb_iq_metrics *metrics)
{
    struct bb_iq_metrics result = {0U, 0U, 0U};
    size_t index;
    if ((samples == NULL) || (metrics == NULL) || (count == 0U) ||
        (count > UINT32_MAX)) {
        return -1;
    }
    for (index = 0U; index < count; ++index) {
        uint16_t magnitude_i = component_magnitude(samples[index].i);
        uint16_t magnitude_q = component_magnitude(samples[index].q);
        uint64_t term = iq_energy(samples[index]);
        if ((samples[index].i != 0) || (samples[index].q != 0)) {
            result.nonzero_samples++;
        }
        if (magnitude_i > result.peak_component) {
            result.peak_component = magnitude_i;
        }
        if (magnitude_q > result.peak_component) {
            result.peak_component = magnitude_q;
        }
        if (term > (UINT64_MAX - result.energy)) {
            result.energy = UINT64_MAX;
        } else {
            result.energy += term;
        }
    }
    *metrics = result;
    return 0;
}
```

</details>

### 31.6.5. Phao source — unit test

<details>
<summary>Mở <code>tests/unit_iq_observer.c</code></summary>

```c
#include "bb/iq_observer.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>

static void test_known_block(void)
{
    static const struct bb_cpx16 samples[] = {
        {0, 0},
        {3, 4},
        {-12, 5},
        {INT16_MIN, INT16_MAX}
    };
    struct bb_iq_metrics metrics = {99U, 99U, 99U};
    uint64_t expected = UINT64_C(25) + UINT64_C(169) +
                        UINT64_C(1073741824) + UINT64_C(1073676289);

    assert(bb_iq_measure(samples,
                         sizeof(samples) / sizeof(samples[0]),
                         &metrics) == 0);
    assert(metrics.energy == expected);
    assert(metrics.nonzero_samples == 3U);
    assert(metrics.peak_component == UINT16_C(32768));
}

static void test_reject_without_partial_commit(void)
{
    static const struct bb_cpx16 one[] = {{1, 2}};
    struct bb_iq_metrics unchanged = {7U, 8U, 9U};

    assert(bb_iq_measure(NULL, 1U, &unchanged) != 0);
    assert(unchanged.energy == 7U);
    assert(unchanged.nonzero_samples == 8U);
    assert(unchanged.peak_component == 9U);
    assert(bb_iq_measure(one, 0U, &unchanged) != 0);
    assert(bb_iq_measure(one, 1U, NULL) != 0);
}

int main(void)
{
    test_known_block();
    test_reject_without_partial_commit();
    puts("unit_iq_observer passed");
    return 0;
}
```

</details>

### 31.6.6. Nối build từng dòng

Thêm source vào `modem_fw_host`:

```cmake
add_library(modem_fw_host STATIC
    # các source cũ giữ nguyên
    src/fw/trace/iq_observer.c)
```

Thêm test, không xóa test cũ:

```cmake
add_executable(unit_iq_observer tests/unit_iq_observer.c)
target_link_libraries(unit_iq_observer PRIVATE modem_fw_host)
target_compile_options(unit_iq_observer PRIVATE ${BB_WARNINGS})
add_test(NAME unit_iq_observer COMMAND unit_iq_observer)
```

Chạy:

```bash
cmake --build build --parallel
ctest --test-dir build -R unit_iq_observer --output-on-failure
ctest --test-dir build --output-on-failure
```

Không chỉ chạy test mới. Test cũ là bằng chứng feature mới không phá contract
cũ.

### 31.6.7. Nối observer vào PHY mà không làm bẩn DSP

Không `printf` trong FFT loop. Chọn boundary sau khi tạo xong một IQ block:

```c
struct bb_iq_metrics metrics;

if (bb_iq_measure(block->samples, block->count, &metrics) == 0) {
    trace(context, tick, "IQ_NONZERO",
          (int32_t)metrics.nonzero_samples);
    trace(context, tick, "IQ_PEAK",
          (int32_t)metrics.peak_component);
}
```

Nếu `energy` vượt `int32_t`, đừng cast bừa vào trace API `int32_t`. Có ba lựa
chọn phải nói rõ:

1. trace energy đã scale/log;
2. mở rộng trace schema thành `uint64_t`;
3. chỉ trace peak/nonzero, giữ energy cho unit test.

Trong lab này chọn phương án 3 để thay đổi ít boundary nhất.

### 31.6.8. Debug

| Symptom | Nghi ở đâu? | Probe nhỏ nhất |
|---|---|---|
| Peak của `INT16_MIN` là 0 hoặc 32767 | `abs`/narrowing | Test riêng `-32768` |
| Energy âm sau khi in | Format/cast observer, không phải energy | In bằng `PRIu64` |
| Test invalid làm output đổi | Commit quá sớm | Dùng local `result`, assign cuối |
| Host pass, Arm warning | Type/format phụ thuộc ABI | Fixed-width + đúng format macro |
| PHY chậm mạnh | Observer nằm trong inner loop | Đo một lần tại stage boundary |

### Sau khi code: ý nghĩa modem/baseband

Observability không phải trang trí. Baseband có hàng nghìn sample; nếu không có
observer định lượng, người học chỉ thấy “decode fail”. Helper này cho biết
waveform biến mất, saturate hay còn năng lượng trước khi phải nghi ngờ CRC/MAC.

### 31.6.9. Viết lại và tự biến thể

Đóng phao, xóa ba file luyện và viết lại. Sau đó tự thêm **một** metric:

- số component bị saturation;
- mean absolute component bằng integer;
- peak sample index;
- energy riêng I và Q.

Mỗi metric mới bắt buộc đi đủ `type → implementation → fixture → invalid test →
integration decision`. Không chỉ thêm field rồi quên producer/consumer.

## 31.7. Script Lab — dùng Python để tự động nghịch modem, không thay C core

Firmware core vẫn là ISO C17. Python ở đây là host-side experiment helper: tạo
config, chạy binary nhiều lần, parse machine-readable result và giữ thí nghiệm
reproducible. Đây là kỹ năng script đúng phạm vi modem/baseband.

### 31.7.1. Đề bài nhỏ

Tự viết `tools/run_modem_sweep.py` để chạy bốn case:

| Case | Thay đổi |
|---|---|
| `baseline` | Giữ config gốc |
| `clean_channel` | CFO=0, noise=0, timing=0 |
| `half_cfo` | CFO=900 Hz |
| `qpsk` | modulation=2 |

Script phải:

- không sửa `scenario.cfg` gốc;
- reject key không tồn tại;
- dùng temporary directory;
- capture stdout/stderr;
- yêu cầu đúng một dòng `MODEM_RESULT`;
- parse field theo tên, không dựa vào vị trí cứng;
- fail nếu process return nonzero hoặc `status != PASS`;
- đếm byte IQ và số dòng trace để có observable.

### 31.7.2. Tự code trước

Viết pseudo-code năm function:

```text
render_config
parse_result
run_case
print_summary
main
```

Chỉ sau khi tự viết và chạy baseline mới mở phao.

### 31.7.3. Phao source Python đầy đủ

<details>
<summary>Mở <code>tools/run_modem_sweep.py</code></summary>

```python
#!/usr/bin/env python3
from __future__ import annotations

import argparse
import subprocess
import tempfile
from pathlib import Path


CASES = (
    ("baseline", {}),
    (
        "clean_channel",
        {
            "cfo_hz": "0",
            "noise_amplitude": "0",
            "timing_offset_samples": "0",
        },
    ),
    ("half_cfo", {"cfo_hz": "900"}),
    ("qpsk", {"modulation": "2"}),
)


def render_config(base_text: str, overrides: dict[str, str]) -> str:
    output: list[str] = []
    seen: set[str] = set()
    for raw_line in base_text.splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            output.append(raw_line)
            continue
        key, separator, value = line.partition("=")
        if not separator:
            raise ValueError(f"invalid config line: {raw_line!r}")
        if key in overrides:
            value = overrides[key]
            seen.add(key)
        output.append(f"{key}={value}")
    missing = set(overrides) - seen
    if missing:
        raise ValueError(f"unknown config keys: {sorted(missing)}")
    return "\n".join(output) + "\n"


def parse_result(stdout: str) -> dict[str, str]:
    lines = [
        line for line in stdout.splitlines()
        if line.startswith("MODEM_RESULT ")
    ]
    if len(lines) != 1:
        raise ValueError(
            f"expected one MODEM_RESULT line, got {len(lines)}"
        )
    result: dict[str, str] = {}
    for token in lines[0].split()[1:]:
        key, separator, value = token.partition("=")
        if not separator or not key or not value:
            raise ValueError(f"bad result token: {token!r}")
        result[key] = value
    required = {
        "status", "state", "tx_bytes", "rx_bytes",
        "crc_fail", "recoveries",
    }
    if not required.issubset(result):
        missing = sorted(required - set(result))
        raise ValueError(f"missing result fields: {missing}")
    return result


def run_case(
    project: Path,
    binary: Path,
    name: str,
    overrides: dict[str, str],
    output_dir: Path,
) -> dict[str, str]:
    base_path = project / "scenario.cfg"
    config_text = render_config(
        base_path.read_text(encoding="utf-8"), overrides
    )
    config_path = output_dir / f"{name}.cfg"
    iq_path = output_dir / f"{name}.iq"
    trace_path = output_dir / f"{name}.jsonl"
    config_path.write_text(config_text, encoding="utf-8")

    completed = subprocess.run(
        [
            str(binary),
            "--config", str(config_path),
            "--events", str(project / "events.txt"),
            "--iq-out", str(iq_path),
            "--trace", str(trace_path),
        ],
        cwd=project,
        text=True,
        capture_output=True,
        check=False,
    )
    result = parse_result(completed.stdout)
    if completed.returncode != 0 or result["status"] != "PASS":
        raise RuntimeError(
            f"{name} failed: rc={completed.returncode}\n"
            f"stdout={completed.stdout}\n"
            f"stderr={completed.stderr}"
        )
    result["iq_bytes"] = str(iq_path.stat().st_size)
    with trace_path.open(encoding="utf-8") as trace_file:
        result["trace_lines"] = str(sum(1 for _ in trace_file))
    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--project", type=Path, default=Path("."))
    parser.add_argument(
        "--binary", type=Path, default=Path("build/modem101")
    )
    args = parser.parse_args()
    project = args.project.resolve()
    binary = args.binary
    if not binary.is_absolute():
        binary = (project / binary).resolve()

    with tempfile.TemporaryDirectory(prefix="modem-sweep-") as temporary:
        output_dir = Path(temporary)
        for name, overrides in CASES:
            result = run_case(
                project, binary, name, overrides, output_dir
            )
            print(
                f"{name:13s} status={result['status']} "
                f"state={result['state']} "
                f"iq_bytes={result['iq_bytes']} "
                f"trace_lines={result['trace_lines']}"
            )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
```

</details>

Chạy từ root project:

```bash
python3 tools/run_modem_sweep.py \
    --project . --binary build/modem101
```

Output có dạng:

```text
baseline      status=PASS state=DATA iq_bytes=3452 trace_lines=987
clean_channel status=PASS state=DATA iq_bytes=3304 trace_lines=987
half_cfo      status=PASS state=DATA iq_bytes=3452 trace_lines=987
qpsk          status=PASS state=DATA iq_bytes=4548 trace_lines=987
```

Kích thước cụ thể có thể đổi nếu bạn thay implementation; contract quan trọng
là các case PASS, file được tạo và parser không nuốt output lỗi.

### 31.7.4. Test chính script, không tin nó vô điều kiện

Tạo ba lỗi có chủ đích:

1. Override key `cfoo_hz`: script phải reject trước khi chạy modem.
2. Đổi binary thành `/bin/false`: script phải fail rõ.
3. Tạo fake executable in hai dòng `MODEM_RESULT`: parser phải reject ambiguity.

Sau đó thêm một case **được kỳ vọng FAIL** do config parser reject. Refactor
`run_case` để case có `expected_status`/`expected_returncode`; không biến mọi
failure thành exception giống nhau.

### Sau khi code: ý nghĩa modem/baseband

Bạn đã biến việc “nghịch CFO/noise/modulation” thành experiment có thể lặp lại,
so sánh và audit. C vẫn làm modem; script giúp sinh scenario và thu bằng chứng.
Đó là khác biệt giữa nghịch có phương pháp và click/chỉnh số ngẫu nhiên.

### 31.7.5. Viết lại không nhìn

Không nhìn phao, viết script thứ hai chỉ làm nhiệm vụ:

```text
seed = 1..20
→ chạy cùng config
→ ghi CSV: seed,status,crc_fail,recoveries,iq_bytes
→ dừng ngay nếu một seed không deterministic khi chạy lại hai lần
```

Không parse trace bằng regex tùy tiện; mỗi JSONL line dùng `json.loads`.

## 31.8. Phao cho các bài tự biến thể ở trên

Phần này đặt phao **sau** các bài biến thể để bạn không bị nhìn thấy đáp án khi
đang làm. Tất cả helper/biến thể đã yêu cầu đều có source tiếp tục được ở đây.

### 31.8.1. Phao biến thể hai tone, scale, move và digest

Thêm helper digest grid vào `grid_script.c`. Helper encode từng component theo
little-endian tường minh; không hash raw `struct` vì representation/padding phụ
thuộc ABI.

<details>
<summary>Mở source digest + scenario biến thể</summary>

```c
static uint32_t digest_update(uint32_t digest,
                              const uint8_t *bytes, size_t length)
{
    size_t index;
    for (index = 0U; index < length; ++index) {
        digest ^= bytes[index];
        digest *= UINT32_C(16777619);
    }
    return digest;
}

static uint32_t grid_digest(const struct wh_cpx16 *grid, size_t count)
{
    uint32_t digest = UINT32_C(2166136261);
    size_t index;
    for (index = 0U; index < count; ++index) {
        uint8_t encoded[4];
        (void)wh_put_u16_le(&encoded[0], (uint16_t)grid[index].i);
        (void)wh_put_u16_le(&encoded[2], (uint16_t)grid[index].q);
        digest = digest_update(digest, encoded, sizeof(encoded));
    }
    return digest;
}
```

Scenario và checkpoint cho `main()`:

```c
static const struct wiz_command setup[] = {
    {WIZ_SET_BIN, -2, 0, INT16_C(12000), 0, 0U},
    {WIZ_SET_BIN,  2, 0, INT16_C(12000), 0, 0U}
};
static const struct wiz_command mutate[] = {
    {WIZ_SCALE_RANGE, -2, 0, INT16_C(16384), 0, 1U},
    {WIZ_MOVE_BIN, 2, 3, 0, 0, 0U},
    {WIZ_EXPECT_PEAK, 3, 0, 0, 0, 0U}
};
struct wiz_runner runner;
uint32_t before;
uint32_t after;

(void)memset(&runner, 0, sizeof(runner));
assert(run_script(&runner, setup,
                  sizeof(setup) / sizeof(setup[0])) == 0);
before = grid_digest(runner.grid, WIZ_NFFT);
assert(run_script(&runner, mutate,
                  sizeof(mutate) / sizeof(mutate[0])) == 0);
after = grid_digest(runner.grid, WIZ_NFFT);
assert(before != after);
```

Đoạn này dùng nhánh `WIZ_SCALE_RANGE` và `scale_range` đã có phao đầy đủ ở
31.4.7.

</details>

### 31.8.2. Phao metric `saturated_components`

Thêm field vào public type:

```c
struct bb_iq_metrics {
    uint64_t energy;
    uint32_t nonzero_samples;
    uint32_t saturated_components;
    uint16_t peak_component;
};
```

Trong loop `bb_iq_measure`, trước phần peak:

```c
if ((samples[index].i == INT16_MIN) ||
    (samples[index].i == INT16_MAX)) {
    result.saturated_components++;
}
if ((samples[index].q == INT16_MIN) ||
    (samples[index].q == INT16_MAX)) {
    result.saturated_components++;
}
```

Sửa initializer local:

```c
struct bb_iq_metrics result = {0U, 0U, 0U, 0U};
```

Test đầy đủ cho field mới với fixture ở 31.6:

```c
assert(metrics.saturated_components == 2U);
```

Test commit failure phải thêm field canary:

```c
struct bb_iq_metrics unchanged = {7U, 8U, 9U, 10U};
assert(bb_iq_measure(NULL, 1U, &unchanged) != 0);
assert(unchanged.energy == 7U);
assert(unchanged.nonzero_samples == 8U);
assert(unchanged.saturated_components == 9U);
assert(unchanged.peak_component == 10U);
```

### 31.8.3. Phao hỗ trợ expected-failure case cho sweep

Không ép mọi case phải PASS. Tạo contract rõ:

```python
def run_expected(
    command: list[str],
    cwd: Path,
    expected_returncode: int,
    expected_stderr_fragment: str,
) -> None:
    completed = subprocess.run(
        command,
        cwd=cwd,
        text=True,
        capture_output=True,
        check=False,
    )
    if completed.returncode != expected_returncode:
        raise AssertionError(
            f"returncode={completed.returncode}, "
            f"expected={expected_returncode}\n{completed.stderr}"
        )
    if expected_stderr_fragment not in completed.stderr:
        raise AssertionError(
            f"missing stderr fragment {expected_stderr_fragment!r}: "
            f"{completed.stderr!r}"
        )
```

Tạo config malformed bằng temporary file rồi gọi:

```python
bad_config = output_dir / "unknown-key.cfg"
bad_config.write_text(
    (project / "scenario.cfg").read_text(encoding="utf-8")
    + "cfoo_hz=900\n",
    encoding="utf-8",
)
run_expected(
    [
        str(binary),
        "--config", str(bad_config),
        "--events", str(project / "events.txt"),
    ],
    cwd=project,
    expected_returncode=1,
    expected_stderr_fragment="invalid config or events file",
)
```

### 31.8.4. Phao full script sweep seed + deterministic replay

File này import hai helper đã có source ở 31.7, không giấu logic modem trong
script thứ hai.

<details>
<summary>Mở <code>tools/seed_replay.py</code></summary>

```python
#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import subprocess
import tempfile
from pathlib import Path

from run_modem_sweep import parse_result, render_config


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        while chunk := stream.read(65536):
            digest.update(chunk)
    return digest.hexdigest()


def validate_jsonl(path: Path) -> int:
    count = 0
    with path.open(encoding="utf-8") as stream:
        for line_number, line in enumerate(stream, start=1):
            try:
                value = json.loads(line)
            except json.JSONDecodeError as error:
                raise ValueError(
                    f"{path}:{line_number}: invalid JSON: {error}"
                ) from error
            if not isinstance(value, dict):
                raise ValueError(
                    f"{path}:{line_number}: expected JSON object"
                )
            count += 1
    return count


def run_once(
    project: Path,
    binary: Path,
    config: Path,
    prefix: Path,
) -> tuple[dict[str, str], str, str, int]:
    iq_path = prefix.with_suffix(".iq")
    trace_path = prefix.with_suffix(".jsonl")
    completed = subprocess.run(
        [
            str(binary),
            "--config", str(config),
            "--events", str(project / "events.txt"),
            "--iq-out", str(iq_path),
            "--trace", str(trace_path),
        ],
        cwd=project,
        text=True,
        capture_output=True,
        check=False,
    )
    result = parse_result(completed.stdout)
    if completed.returncode != 0 or result["status"] != "PASS":
        raise RuntimeError(
            f"run failed rc={completed.returncode}\n"
            f"{completed.stdout}\n{completed.stderr}"
        )
    return (
        result,
        sha256(iq_path),
        sha256(trace_path),
        validate_jsonl(trace_path),
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--project", type=Path, default=Path("."))
    parser.add_argument(
        "--binary", type=Path, default=Path("build/modem101")
    )
    parser.add_argument("--output", type=Path, default=Path("seed-sweep.csv"))
    args = parser.parse_args()
    project = args.project.resolve()
    binary = args.binary
    if not binary.is_absolute():
        binary = (project / binary).resolve()
    base_text = (project / "scenario.cfg").read_text(encoding="utf-8")

    with tempfile.TemporaryDirectory(prefix="seed-replay-") as temporary:
        temporary_path = Path(temporary)
        with args.output.open("w", newline="", encoding="utf-8") as stream:
            writer = csv.DictWriter(
                stream,
                fieldnames=[
                    "seed", "status", "crc_fail", "recoveries",
                    "iq_sha256", "trace_sha256", "trace_lines",
                ],
            )
            writer.writeheader()
            for seed in range(1, 21):
                config = temporary_path / f"seed-{seed}.cfg"
                config.write_text(
                    render_config(base_text, {"seed": str(seed)}),
                    encoding="utf-8",
                )
                first = run_once(
                    project, binary, config,
                    temporary_path / f"seed-{seed}-a",
                )
                second = run_once(
                    project, binary, config,
                    temporary_path / f"seed-{seed}-b",
                )
                if first != second:
                    raise RuntimeError(
                        f"seed {seed} is not deterministic"
                    )
                result, iq_hash, trace_hash, trace_lines = first
                writer.writerow(
                    {
                        "seed": seed,
                        "status": result["status"],
                        "crc_fail": result["crc_fail"],
                        "recoveries": result["recoveries"],
                        "iq_sha256": iq_hash,
                        "trace_sha256": trace_hash,
                        "trace_lines": trace_lines,
                    }
                )
                stream.flush()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
```

</details>

Chạy:

```bash
python3 tools/seed_replay.py --project . \
    --binary build/modem101 --output seed-sweep.csv
```

Phao không có nghĩa được copy ngay. Nó đảm bảo người mới không bị kẹt vĩnh
viễn sau khi đã tự thử, đồng thời cho phép đối chiếu một implementation hoàn
chỉnh.

## 31.9. Kỹ năng tạo helper — khi nào tách, tách ở đâu, test cái gì

### 31.9.1. Dấu hiệu cần tách helper

Tách khi một đoạn code có ít nhất một trong các dấu hiệu:

- cùng phép biến đổi xuất hiện từ hai nơi;
- có invariant riêng đáng đặt tên;
- chứa indexing/overflow/wrap dễ sai;
- có thể test bằng input nhỏ hơn nhiều so với cả pipeline;
- tên helper giúp câu caller đọc giống data flow;
- muốn inject fake implementation trên host.

Không tách chỉ vì “hàm dài 20 dòng”. Đừng tạo helper `do_stuff()` hoặc
`process_data()`; tên phải nói representation và action, ví dụ
`sn12_distance`, `logical_bin_to_index`, `descriptor_validate_region`.

### 31.9.2. Năm câu hỏi trước mỗi prototype

```text
1. Caller truyền object gì và object đó đã được validate tới đâu?
2. Helper mượn, sửa hay nhận ownership?
3. Capacity/length nằm ở đâu?
4. Failure có để output nguyên vẹn không?
5. Caller quan sát thành công bằng return, out-param, state hay trace?
```

Prototype xấu:

```c
int process(void *data);
```

Prototype tốt hơn:

```c
int bb_grid_move(struct bb_cpx16 *bins, size_t bin_count,
                 int32_t source_bin, int32_t destination_bin);
```

Prototype thứ hai chưa hoàn hảo cho mọi project, nhưng nó buộc representation,
capacity, mutability và operand xuất hiện trong contract.

### 31.9.3. Helper ladder bắt buộc cho mọi feature mới

| Bậc | Bạn code gì? | Gate |
|---|---|---|
| 0 | Ví dụ bằng giấy | Tự tính được expected |
| 1 | Pure helper | Không global, unit test nhỏ |
| 2 | Stateful wrapper | State transition rõ |
| 3 | Queue/owner boundary | Success/failure ownership đều rõ |
| 4 | Scenario composition | Nhiều helper chạy theo data |
| 5 | Observer | Biết checkpoint nào sai |
| 6 | Mutation | Một biến thể ngoài happy path |
| 7 | Rebuild | Viết lại không nhìn |

Không nhảy từ bậc 0 lên bậc 4. Khi whole-project test fail, hạ xuống helper
nhỏ nhất còn tái hiện được lỗi.

## 31.10. Đọc compiler như feedback, không như kẻ thù

Với mỗi lỗi, chỉ sửa **lỗi đầu tiên có nguyên nhân thật**, build lại rồi mới
xem tiếp. Một lỗi syntax sớm có thể tạo 30 diagnostic giả phía sau.

| Diagnostic thường gặp | Nó đang nói gì? | Cách điều tra |
|---|---|---|
| `implicit declaration` | Chưa thấy prototype | Header đúng chưa, include đúng layer chưa |
| `conflicting types` | Declaration và definition lệch | So từng parameter, `const`, width, return |
| `conversion ... may change value` | Miền đích hẹp hơn | Chứng minh range rồi mới cast hoặc đổi type |
| `comparison of signed and unsigned` | Hai miền số khác nhau | Chọn một representation đúng domain |
| `shadowed declaration` | Tên local che object ngoài | Đổi tên theo vai trò, đừng tắt warning |
| `missing prototype` | Function global vô tình | Cho `static` hoặc thêm contract header |
| ASan out-of-bounds | Index/capacity contract sai | In index, count, capacity tại boundary |
| UBSan signed overflow | Arithmetic chưa widen | Cast operand trước phép toán |
| Leak không xuất hiện nhưng pool cạn | Ownership logical bị mất | Trace alloc/release handle generation |
| Test treo | Loop/timer không bounded | Thêm budget và progress invariant |

Mẫu notebook debug:

```text
Expected representation:
Actual representation:
Checkpoint cuối đúng:
Helper đầu tiên sai:
Input nhỏ nhất gây sai:
Invariant bị vi phạm:
Một dòng sửa:
Regression test tôi thêm:
```

## 31.11. 36 buổi rèn “từ ý nghĩ tới code” — không chỉ đọc

Các phần cũ vẫn học theo thứ tự của tài liệu. Bảng này là lớp luyện cơ tay chạy
song song. Mỗi buổi 45–90 phút, phải có commit hoặc thư mục evidence riêng.

| Buổi | Việc phải tự code từ trắng | Evidence |
|---:|---|---|
| 1 | `sat16`, test bốn biên | binary + output |
| 2 | MSB bit get/set | round-trip 32 bit |
| 3 | LE16/LE32 codec | byte fixture |
| 4 | IQ struct + energy observer | known vector |
| 5 | logical bin mapping | đủ 16 bin |
| 6 | grid set/move/notch | before/after dump |
| 7 | Q15 scale | positive/negative/edge |
| 8 | QPSK map/demap | đủ 4 points |
| 9 | 16-QAM map/demap | đủ 16 nibbles |
| 10 | CRC bit loop | known vector |
| 11 | ring single-thread | full/empty/wrap |
| 12 | pool hai slot | stale handle bị reject |
| 13 | fake clock ops | hai context độc lập |
| 14 | timer modular tick | wrap fixture |
| 15 | state machine switch | illegal event giữ state |
| 16 | transition table | trace from/event/to |
| 17 | PDU encoder byte-by-byte | golden bytes |
| 18 | defensive decoder | truncated/trailing/length |
| 19 | scenario command runner | fail đúng command index |
| 20 | deterministic PRNG/noise | hai hash giống nhau |
| 21 | DFT N=8 | impulse + tone |
| 22 | IFFT + CP N=8 | round trip |
| 23 | byte→QPSK→OFDM→byte | payload `0x41` |
| 24 | timing delay helper | zero prefix đúng count |
| 25 | CFO rotator | zero CFO identity |
| 26 | channel composition | same seed byte-identical |
| 27 | public IQ observer module | unit + old tests |
| 28 | ops-table RF fake | host backend swap |
| 29 | IRQ top-half toy | chỉ enqueue |
| 30 | deferred decoder toy | task mới xử lý |
| 31 | DMA descriptor codec | đúng 32 bytes |
| 32 | RLC SN12 reorder toy | wrap + duplicate |
| 33 | PDCP replay bitmap toy | old/replay/new |
| 34 | Python config sweep | bốn case có summary |
| 35 | seed deterministic replay | CSV + hashes |
| 36 | feature tự nghĩ | full vertical slice |

Quy tắc evidence mỗi buổi:

```text
practice/session_XX/
├── README.md          # ý nghĩ, contract, invariant
├── source/            # code tự viết
├── tests/             # test tự viết
├── build.log
├── run.log
├── mutation.log       # bug/variant đã thử
└── rewrite/           # bản viết lại không nhìn
```

Không cần Git commit nếu chưa dùng Git, nhưng không được chỉ nói “đã hiểu”.
Phải có artifact chạy được.

## 31.12. Gate chống ảo tưởng thành thạo

### Gate A — Function fluency

Trong 20 phút, từ file trắng:

```text
sat16 + bit get/set + LE32 + 12 assert
```

Không warning, không nhìn phao.

### Gate B — Module fluency

Trong 60 phút:

```text
header guard
public type/prototype
source có static helper
unit test happy + edge + invalid
strict compile
sanitizer
```

### Gate C — Composition fluency

Trong 120 phút, tự code một command runner hoặc pipeline ít nhất năm stage,
có observer chỉ đúng stage đầu tiên hỏng.

### Gate D — Project fluency

Nhận một yêu cầu mới, điền đủ:

```text
contract
files touched
new helper
state/owner impact
unit test
integration scenario
trace
failure injection
rebuild evidence
```

### Gate E — Wizard fluency

Tự nghĩ ra một phép nghịch **không có trong đáp án**, ví dụ:

```text
notch một dải bin theo lịch virtual tick
đổi CFO giữa frame nhưng replay được
drop đúng descriptor generation N
delay một RLC SN cụ thể rồi thả lại
đổi modulation theo command script
đo IQ saturation tại mỗi PHY stage
```

Sau đó code nó mà không sửa đường tắt vào final state, không trao payload ngoài
IQ và không phá test cũ.

## 31.13. Capstone cuối — feature tự nghĩ phải đi xuyên project

Chọn **một** feature của Gate E. Tạo `FEATURE_CONTRACT.md` theo mẫu:

```markdown
# Tên feature

## Một câu mô tả

## Ví dụ input/output bằng tay

## Representation mới hoặc thay đổi

## Public/internal API

## Ownership và lifetime

## State transition

## Invariant và bound

## Unit tests

## Integration scenario

## Trace/observer

## Fault/mutation

## Host/Arm impact

## Definition of done
```

Thứ tự triển khai:

1. Viết contract và ba test name, chưa code.
2. Dùng `rg` điền producer/consumer/file map.
3. Code pure helper nhỏ nhất.
4. Unit test helper.
5. Thêm state nếu thật sự cần; không nhét global.
6. Nối owner boundary.
7. Thêm trace checkpoint.
8. Viết scenario chạy feature.
9. Inject một failure.
10. Chạy toàn test host + sanitizer.
11. Build/audit Arm nếu toolchain có sẵn.
12. Đóng code, tự trình bày data flow.
13. Xóa bản luyện và rebuild feature lần hai.

### Phao kiến trúc cho capstone

Capstone không có một đáp án duy nhất, nhưng không bị bỏ mặc. Dùng skeleton
vertical slice này:

```c
/* public/internal contract */
struct feature_config {
    uint32_t trigger_tick;
    int32_t parameter;
    bool enabled;
};

struct feature_state {
    struct feature_config config;
    uint32_t generation;
    bool applied;
};

int feature_validate(const struct feature_config *config);
int feature_arm(struct feature_state *state,
                const struct feature_config *config);
int feature_step(struct feature_state *state, uint32_t now,
                 struct wh_cpx16 *samples, size_t count);
```

Implementation pattern đầy đủ để khởi động:

```c
int feature_validate(const struct feature_config *config)
{
    if (config == NULL) {
        return -1;
    }
    if (config->enabled &&
        ((config->parameter < -32768) ||
         (config->parameter > 32767))) {
        return -1;
    }
    return 0;
}

int feature_arm(struct feature_state *state,
                const struct feature_config *config)
{
    if ((state == NULL) || (feature_validate(config) != 0)) {
        return -1;
    }
    state->config = *config;
    state->generation++;
    if (state->generation == 0U) {
        state->generation = 1U;
    }
    state->applied = false;
    return 0;
}

int feature_step(struct feature_state *state, uint32_t now,
                 struct wh_cpx16 *samples, size_t count)
{
    size_t index;
    if ((state == NULL) || (samples == NULL) || (count == 0U)) {
        return -1;
    }
    if (!state->config.enabled || state->applied ||
        !wh_tick_after_or_equal_u32(now,
                                    state->config.trigger_tick)) {
        return 0;
    }
    for (index = 0U; index < count; ++index) {
        samples[index].i = wh_sat16(
            (int32_t)samples[index].i + state->config.parameter);
    }
    state->applied = true;
    return 1;
}
```

Đây chỉ là phao cho lifecycle `validate → arm → trigger once → observe`; bạn
phải thay phép cộng I bằng operation baseband của feature mình chọn. Test bắt
buộc:

```text
invalid config không commit state
trước trigger không mutate
đúng trigger mutate một lần
gọi lại không mutate lần hai
re-arm tăng generation và cho phép trigger mới
tick wrap vẫn đúng trong half-range contract
NULL/count zero bị reject
```

## 31.14. Khi nào được nói “tôi tự code được cái tôi nghĩ”?

Chỉ khi cả bảy câu đều là “có”:

1. Tôi có thể viết ví dụ expected trước implementation không?
2. Tôi tự chọn được type theo miền giá trị và lifetime không?
3. Tôi tự chẻ được helper và đặt tên theo invariant không?
4. Tôi tự viết test khiến bug cụ thể lộ ra không?
5. Tôi lần được producer/consumer/owner trong project không?
6. Tôi biến feature thành scenario replay được và observer đo được không?
7. Tôi đóng đáp án rồi viết lại từ file trắng được không?

Nếu câu 7 chưa đạt, lặp lại bài. Nếu câu 1–6 chưa đạt, quay về bậc helper tương
ứng; đừng bù bằng copy.

## 31.15. Checkpoint bảo toàn tài liệu và tiêu chí hoàn tất lần duyệt này

Lần bổ sung này tuân thủ các ràng buộc:

- giữ nguyên toàn bộ nội dung trước `PHẦN XXXI`;
- không xóa bài, lời giải, Wizard Lab, phao, phần concept hay rebuild guide;
- thêm source đầy đủ cho helper nền, C command runner, IQ observer và host-side
  experiment scripts;
- mọi bài mới đều đặt đề/hướng dẫn trước, phao ở sau;
- mọi source C hoàn chỉnh đều có lệnh strict build/test;
- kỹ năng script chỉ phục vụ config/IQ/trace/scenario modem;
- thao tác vẫn nằm trong virtual RF/file IQ/local simulator;
- mục tiêu cuối vẫn là tự rebuild lời giải và tự thêm feature của riêng mình.

Lệnh tự kiểm nhanh tài liệu:

```bash
rg -n '^# PHẦN|Wizard Lab|Phao cứu sinh|Viết lại không nhìn' \
    PHU_THUY_BASEBAND_C17_MASTERY_FORGE_KHONG_CON_COPY_CODE.md

rg -n '```c|```python|gcc -std=c17|ctest' \
    PHU_THUY_BASEBAND_C17_MASTERY_FORGE_KHONG_CON_COPY_CODE.md
```

Đích cuối không còn là “biết lời giải trông như thế nào”, mà là:

```text
thấy một hành vi baseband trong đầu
→ tự chọn representation
→ tự đúc helper
→ tự nối thành pipeline
→ tự viết script thí nghiệm
→ tự quan sát, phá, debug và chứng minh
→ tự rebuild từ trắng
```

# PHẦN XXXII — C17 IRONMAN: BẮT BUỘC CODE C VỚI KHỐI LƯỢNG LỚN

> Mục tiêu mới được chốt rất rõ: người đọc phải có thể **thành thạo C chỉ bằng
> việc làm đúng những gì tài liệu yêu cầu**, không chỉ hiểu modem. Vì vậy phần
> này biến toàn bộ khóa học thành một chế độ luyện bắt buộc có số lượng
> artifact, số lần viết lại và số lần debug cụ thể.

## 32.0. Quota tối thiểu — đọc không được tính

Mỗi bài trong toàn tài liệu phải tạo tám artifact. Không có artifact thì bài
chưa hoàn thành dù bạn trả lời miệng đúng.

| Artifact | Tên gợi ý | Việc phải làm |
|---|---|---|
| 1 | `attempt_01.c` | Tự code trước phao |
| 2 | `guided_fix.c` | Sửa attempt sau khi đọc compiler/hint |
| 3 | `retype_solution.c` | Tự tay gõ lại phao, không copy/paste |
| 4 | `closed_book.c` | File trắng, không nhìn phao |
| 5 | `boundary_mutation.c` | Đổi biên/range/capacity và sửa code |
| 6 | `behavior_mutation.c` | Thêm một hành vi mới |
| 7 | `bug_injection.c` | Cố cài bug rồi viết regression test bắt nó |
| 8 | `composition.c` | Nối helper với ít nhất một helper/module khác |

Mỗi artifact phải có executable test hoặc được link vào một test target. Chỉ
đổi tên file hoặc đổi literal không được tính là một artifact mới.

Khối lượng tối thiểu của toàn khóa:

| Nhóm | Số bài/lượt | Artifact code tối thiểu |
|---|---:|---:|
| 8 bài kiểu dữ liệu khó | 8 × 8 | 64 |
| 41 bài core + Wizard Lab | 41 × 8 | 328 |
| 36 buổi Mastery Forge | 36 × 3 lượt | 108 |
| Variant Deck ở phần này | 20 helper × 3 lượt | 60 |
| Rebuild project | 2 lần từ repo trắng | toàn bộ source hai lần |
| **Tổng trước rebuild** |  | **ít nhất 560 artifact/lượt code** |

Không chạy 560 bài trong một tuần. Đây là lượng deliberate practice của cả lộ
trình. Bạn không cần đếm số dòng để khoe; bạn cần đếm số lần tự tạo contract,
compile, debug và viết lại.

## 32.1. Một phiên code bắt buộc trông như thế nào

Mỗi phiên 60–90 phút:

| Phút | Việc |
|---:|---|
| 0–5 | Viết input/output và invariant trên giấy |
| 5–15 | Khai báo type/prototype + test đầu tiên |
| 15–35 | Code attempt, compile liên tục |
| 35–45 | Chạy edge case/sanitizer, debug |
| 45–55 | Mở hint/phao, ghi khác biệt |
| 55–70 | Đóng phao, viết lại từ trắng |
| 70–85 | Thêm mutation hoặc composition |
| 85–90 | Ghi log và lệnh tái hiện |

Mỗi phiên kết thúc bằng:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -Wstrict-prototypes -Wmissing-prototypes \
    -fsanitize=address,undefined -g \
    your_source.c your_test.c -o your_test
ASAN_OPTIONS=detect_leaks=0 ./your_test
```

`ASAN_OPTIONS=detect_leaks=0` chỉ dùng nếu môi trường container/debugger không
cho LeakSanitizer đọc `/proc`; trên máy bình thường hãy để leak detection bật.

## 32.2. Luật ép cơ tay C

1. Bài 1 được gõ theo hướng dẫn từng dòng.
2. Từ bài 2, tự viết attempt trước khi mở source.
3. Không copy/paste phao vào `retype_solution.c`; tự gõ để gặp lỗi syntax/type.
4. Mọi warning là lỗi build; không tắt warning để qua bài.
5. Mỗi ngày ít nhất một file C mới và một file C viết lại.
6. Mỗi tuần có một ngày chỉ debug code đã cố làm hỏng.
7. Mỗi 10 bài phải có một composition nối ba helper.
8. Mỗi 20 bài phải xóa một module luyện và rebuild không nhìn.
9. Mỗi helper public phải có invalid-input contract.
10. Mỗi stateful helper phải có test gọi hai lần, wrap/reuse hoặc stale event.

Không được dùng AI/answer để sinh attempt đầu. Sau attempt, có thể dùng phao để
đối chiếu và hỏi tại sao.

## 32.3. Variant Deck — 20 helper C phải tự code ba lần

Deck này cố tình dùng lại các concept modem quen thuộc nhưng đổi yêu cầu để
người học không thể chỉ nhớ source cũ.

### 32.3.1. Hai mươi đề nhỏ

| # | Helper phải tự code | Test tối thiểu | Concept modem |
|---:|---|---|---|
| 1 | Get bit LSB-first | `0x81`, bit 0 và 7 đều 1 | Bit-order contract |
| 2 | Put `uint64_t` little-endian | fixture `0x0123456789ABCDEF` | IQ/file/descriptor codec |
| 3 | Get `uint64_t` little-endian | round trip + NULL | Parser phòng thủ |
| 4 | IQ conjugate | `(3,-4) → (3,4)`; `INT16_MIN` | Complex math + saturation |
| 5 | IQ rotate +90° | `(3,4) → (-4,3)` | Phase manipulation |
| 6 | Inject tone vào logical bin | add + saturation | Resource grid |
| 7 | Swap hai logical bin | âm/dương + invalid | Mapping FFT storage |
| 8 | Zero guard bands | hai mép phổ, DC không đổi | OFDM allocation |
| 9 | Ring push | full reject, không overwrite | Runtime queue |
| 10 | Ring peek | không advance owner/index | Observability |
| 11 | Ring drop oldest | empty reject + wrap | Backpressure policy |
| 12 | Next generation | skip zero khi wrap | Stale handle |
| 13 | Cancel timer | inactive + generation tăng | Stale timeout |
| 14 | State name | mọi enum + unknown | Trace/debug |
| 15 | Validate transition table | duplicate `(from,event)` reject | Deterministic control plane |
| 16 | TLV find | skip unknown, reject truncated | Defensive protocol parser |
| 17 | Flip every k-th bit | deterministic + bounds | Fault injection |
| 18 | Count trace value | borrowed read-only array | Observer helper |
| 19 | Clock ops call | hai context độc lập | Host/Arm abstraction |
| 20 | Stable event insert | equal tick giữ thứ tự | Deterministic runtime |

Ba lần bắt buộc:

```text
Lần A: nhìn prototype + test spec
Lần B: nhìn test source, không nhìn implementation
Lần C: chỉ nhìn tên helper, tự dựng lại type + test + implementation
```

### 32.3.2. Header được phép nhìn ngay

Tạo `practice/ironman/variant_deck.h`:

```c
#ifndef VARIANT_DECK_H
#define VARIANT_DECK_H

#include "wizard_helpers.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define VD_RING_CAPACITY 4U
#define VD_EVENT_CAPACITY 8U

struct vd_ring {
    size_t read_index;
    size_t write_index;
    size_t count;
    int entries[VD_RING_CAPACITY];
};

struct vd_timer {
    uint32_t deadline;
    uint32_t generation;
    bool active;
};

enum vd_state {
    VD_OFF = 0,
    VD_SEARCHING,
    VD_CAMPED,
    VD_CONNECTED,
    VD_STATE_COUNT
};

struct vd_transition {
    enum vd_state from;
    uint16_t event;
    enum vd_state to;
};

struct vd_trace_record {
    uint64_t tick;
    int32_t value;
};

struct vd_clock_ops {
    uint64_t (*ticks)(void *context);
};

struct vd_event {
    uint64_t tick;
    uint16_t source;
    uint16_t sequence;
};

struct vd_event_list {
    struct vd_event events[VD_EVENT_CAPACITY];
    size_t count;
};

int vd_get_bit_lsb(const uint8_t *bytes, size_t length,
                   size_t bit_index, uint8_t *bit);
int vd_put_u64_le(uint8_t out[8], uint64_t value);
int vd_get_u64_le(const uint8_t in[8], uint64_t *value);

struct wh_cpx16 vd_iq_conjugate(struct wh_cpx16 sample);
struct wh_cpx16 vd_iq_rotate90(struct wh_cpx16 sample);
int vd_inject_tone(struct wh_cpx16 *grid, size_t nfft,
                   int32_t logical_bin, struct wh_cpx16 tone);
int vd_grid_swap(struct wh_cpx16 *grid, size_t nfft,
                 int32_t first_bin, int32_t second_bin);
int vd_zero_guard_bands(struct wh_cpx16 *grid, size_t nfft,
                        size_t guard_width);

bool vd_ring_push(struct vd_ring *ring, int value);
bool vd_ring_peek(const struct vd_ring *ring, int *value);
bool vd_ring_drop_oldest(struct vd_ring *ring);

uint32_t vd_next_generation(uint32_t current);
int vd_timer_cancel(struct vd_timer *timer);
const char *vd_state_name(enum vd_state state);
int vd_transition_table_validate(const struct vd_transition *table,
                                 size_t count);

int vd_tlv_find(const uint8_t *frame, size_t frame_length,
                uint8_t wanted_type, const uint8_t **payload,
                uint8_t *payload_length);
int vd_flip_every_kth_bit(uint8_t *bytes, size_t length,
                          size_t first_bit, size_t step);
size_t vd_trace_count_value(const struct vd_trace_record *records,
                            size_t count, int32_t wanted);
int vd_clock_read(const struct vd_clock_ops *ops, void *context,
                  uint64_t *tick);
int vd_event_insert_stable(struct vd_event_list *list,
                           struct vd_event event);

#endif
```

### 32.3.3. Cách làm không lật tài liệu

Làm helper #1, compile; helper #2, compile. Không chờ đủ 20 helper. Với mỗi
helper, tạo ba dòng log:

```text
representation:
failure leaves what unchanged:
modem invariant:
```

Khi hết helper #20, mở test phao bên dưới. Implementation phao đặt **sau test**
để bạn có thể dùng test làm spec mà chưa thấy lời giải.

### 32.3.4. Test phao — được mở sau attempt lần A

<details>
<summary>Mở <code>test_variant_deck.c</code>, vẫn chưa phải implementation</summary>

```c
#include "variant_deck.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static void test_bits_and_u64(void)
{
    static const uint8_t bit_fixture[] = {UINT8_C(0x81)};
    uint8_t bit = 9U;
    uint8_t encoded[8] = {0U};
    uint64_t decoded = 0U;

    assert(vd_get_bit_lsb(bit_fixture, sizeof(bit_fixture), 0U, &bit) == 0);
    assert(bit == 1U);
    assert(vd_get_bit_lsb(bit_fixture, sizeof(bit_fixture), 7U, &bit) == 0);
    assert(bit == 1U);
    assert(vd_get_bit_lsb(bit_fixture, sizeof(bit_fixture), 8U, &bit) != 0);

    assert(vd_put_u64_le(encoded, UINT64_C(0x0123456789ABCDEF)) == 0);
    assert(encoded[0] == UINT8_C(0xEF));
    assert(encoded[7] == UINT8_C(0x01));
    assert(vd_get_u64_le(encoded, &decoded) == 0);
    assert(decoded == UINT64_C(0x0123456789ABCDEF));
    assert(vd_get_u64_le(NULL, &decoded) != 0);
}

static void test_iq_and_grid(void)
{
    struct wh_cpx16 sample = {INT16_C(3), INT16_C(-4)};
    struct wh_cpx16 edge = {INT16_MIN, INT16_MIN};
    struct wh_cpx16 grid[8];
    struct wh_cpx16 tone = {INT16_C(30000), INT16_C(-30000)};
    struct wh_cpx16 extra = {INT16_C(10000), INT16_C(-10000)};
    size_t index = 0U;

    sample = vd_iq_conjugate(sample);
    assert(sample.i == 3 && sample.q == 4);
    edge = vd_iq_conjugate(edge);
    assert(edge.i == INT16_MIN && edge.q == INT16_MAX);
    sample.i = 3;
    sample.q = 4;
    sample = vd_iq_rotate90(sample);
    assert(sample.i == -4 && sample.q == 3);

    (void)memset(grid, 0, sizeof(grid));
    assert(vd_inject_tone(grid, 8U, -1, tone) == 0);
    assert(vd_inject_tone(grid, 8U, -1, extra) == 0);
    assert(wh_logical_bin_to_index(-1, 8U, &index) == 0);
    assert(grid[index].i == INT16_MAX);
    assert(grid[index].q == INT16_MIN);
    assert(vd_grid_swap(grid, 8U, -1, 2) == 0);
    assert(wh_logical_bin_to_index(2, 8U, &index) == 0);
    assert(grid[index].i == INT16_MAX);

    for (index = 0U; index < 8U; ++index) {
        grid[index].i = 10;
        grid[index].q = -10;
    }
    assert(vd_zero_guard_bands(grid, 8U, 2U) == 0);
    assert(wh_logical_bin_to_index(-4, 8U, &index) == 0);
    assert(grid[index].i == 0);
    assert(wh_logical_bin_to_index(-3, 8U, &index) == 0);
    assert(grid[index].i == 0);
    assert(wh_logical_bin_to_index(2, 8U, &index) == 0);
    assert(grid[index].i == 0);
    assert(wh_logical_bin_to_index(3, 8U, &index) == 0);
    assert(grid[index].i == 0);
    assert(wh_logical_bin_to_index(0, 8U, &index) == 0);
    assert(grid[index].i == 10);
    assert(vd_zero_guard_bands(grid, 8U, 5U) != 0);
}

static void test_ring_generation_and_timer(void)
{
    struct vd_ring ring = {0U, 0U, 0U, {0, 0, 0, 0}};
    struct vd_timer timer = {100U, 7U, true};
    int value = 0;

    assert(vd_ring_push(&ring, 10));
    assert(vd_ring_push(&ring, 20));
    assert(vd_ring_push(&ring, 30));
    assert(vd_ring_push(&ring, 40));
    assert(!vd_ring_push(&ring, 50));
    assert(vd_ring_peek(&ring, &value) && value == 10);
    assert(ring.count == 4U);
    assert(vd_ring_drop_oldest(&ring));
    assert(vd_ring_peek(&ring, &value) && value == 20);
    assert(vd_ring_push(&ring, 50));
    assert(ring.count == 4U);

    assert(vd_next_generation(0U) == 1U);
    assert(vd_next_generation(7U) == 8U);
    assert(vd_next_generation(UINT32_MAX) == 1U);
    assert(vd_timer_cancel(&timer) == 0);
    assert(!timer.active);
    assert(timer.generation == 8U);
    assert(timer.deadline == 0U);
    assert(vd_timer_cancel(NULL) != 0);
}

static void test_state_and_tlv(void)
{
    static const struct vd_transition valid[] = {
        {VD_OFF, 1U, VD_SEARCHING},
        {VD_SEARCHING, 2U, VD_CAMPED},
        {VD_CAMPED, 3U, VD_CONNECTED}
    };
    static const struct vd_transition duplicate[] = {
        {VD_OFF, 1U, VD_SEARCHING},
        {VD_OFF, 1U, VD_CONNECTED}
    };
    static const uint8_t frame[] = {
        1U, 2U, UINT8_C(0xAA), UINT8_C(0xBB),
        9U, 1U, UINT8_C(0xCC)
    };
    static const uint8_t truncated[] = {1U, 3U, UINT8_C(0xAA)};
    const uint8_t *payload = NULL;
    uint8_t payload_length = 0U;

    assert(strcmp(vd_state_name(VD_OFF), "OFF") == 0);
    assert(strcmp(vd_state_name(VD_CONNECTED), "CONNECTED") == 0);
    assert(strcmp(vd_state_name((enum vd_state)99), "UNKNOWN") == 0);
    assert(vd_transition_table_validate(
               valid, sizeof(valid) / sizeof(valid[0])) == 0);
    assert(vd_transition_table_validate(
               duplicate, sizeof(duplicate) / sizeof(duplicate[0])) != 0);

    assert(vd_tlv_find(frame, sizeof(frame), 9U,
                       &payload, &payload_length) == 1);
    assert(payload_length == 1U && payload[0] == UINT8_C(0xCC));
    payload = (const uint8_t *)(uintptr_t)1U;
    payload_length = 99U;
    assert(vd_tlv_find(frame, sizeof(frame), 7U,
                       &payload, &payload_length) == 0);
    assert(payload == (const uint8_t *)(uintptr_t)1U);
    assert(payload_length == 99U);
    assert(vd_tlv_find(truncated, sizeof(truncated), 1U,
                       &payload, &payload_length) < 0);
}

struct fake_clock {
    uint64_t now;
};

static uint64_t fake_ticks(void *context)
{
    return ((const struct fake_clock *)context)->now;
}

static void test_mutation_observer_and_ops(void)
{
    uint8_t bytes[2] = {0U, 0U};
    static const struct vd_trace_record records[] = {
        {1U, 7}, {2U, 9}, {3U, 7}, {4U, -1}
    };
    const struct vd_clock_ops ops = {fake_ticks};
    struct fake_clock first = {100U};
    struct fake_clock second = {900U};
    uint64_t tick = 0U;

    assert(vd_flip_every_kth_bit(bytes, sizeof(bytes), 0U, 3U) == 0);
    assert(bytes[0] == UINT8_C(0x92));
    assert(bytes[1] == UINT8_C(0x49));
    assert(vd_flip_every_kth_bit(bytes, sizeof(bytes), 16U, 1U) != 0);
    assert(vd_trace_count_value(
               records, sizeof(records) / sizeof(records[0]), 7) == 2U);
    assert(vd_clock_read(&ops, &first, &tick) == 0 && tick == 100U);
    assert(vd_clock_read(&ops, &second, &tick) == 0 && tick == 900U);
}

static void test_stable_event_insert(void)
{
    struct vd_event_list list = {{{0U, 0U, 0U}}, 0U};

    assert(vd_event_insert_stable(&list, (struct vd_event){10U, 1U, 0U}) == 0);
    assert(vd_event_insert_stable(&list, (struct vd_event){5U, 1U, 1U}) == 0);
    assert(vd_event_insert_stable(&list, (struct vd_event){10U, 1U, 2U}) == 0);
    assert(vd_event_insert_stable(&list, (struct vd_event){5U, 1U, 3U}) == 0);
    assert(list.count == 4U);
    assert(list.events[0].tick == 5U && list.events[0].sequence == 1U);
    assert(list.events[1].tick == 5U && list.events[1].sequence == 3U);
    assert(list.events[2].tick == 10U && list.events[2].sequence == 0U);
    assert(list.events[3].tick == 10U && list.events[3].sequence == 2U);
}

int main(void)
{
    test_bits_and_u64();
    test_iq_and_grid();
    test_ring_generation_and_timer();
    test_state_and_tlv();
    test_mutation_observer_and_ops();
    test_stable_event_insert();
    puts("variant_deck passed");
    return 0;
}
```

</details>

### 32.3.5. Debug trước khi xem implementation

Nếu test không compile, sửa declaration/include/type trước. Nếu compile nhưng
assert fail, chỉ chạy một nhóm test bằng cách tạm thời comment **các lời gọi test
khác trong `main` của file luyện**, không sửa expected để khớp code sai.

Thứ tự điều tra:

```text
bit/u64
→ IQ/grid
→ ring/generation/timer
→ state/TLV
→ mutation/ops
→ stable event
```

Chỉ mở implementation sau khi bạn đã tự hoàn thành ít nhất 10/20 helper hoặc
đã ghi rõ lý do bị kẹt ở từng helper.

### 32.3.6. Phao implementation đầy đủ

<details>
<summary>Mở <code>variant_deck.c</code> — full source để đối chiếu</summary>

```c
#include "variant_deck.h"

#include <limits.h>

static bool bit_index_valid(size_t length, size_t bit_index)
{
    return (length <= (SIZE_MAX / 8U)) &&
           (bit_index < (length * 8U));
}

int vd_get_bit_lsb(const uint8_t *bytes, size_t length,
                   size_t bit_index, uint8_t *bit)
{
    size_t byte_index;
    uint8_t shift;
    if ((bytes == NULL) || (bit == NULL) ||
        !bit_index_valid(length, bit_index)) {
        return -1;
    }
    byte_index = bit_index / 8U;
    shift = (uint8_t)(bit_index % 8U);
    *bit = (uint8_t)((bytes[byte_index] >> shift) & 1U);
    return 0;
}

int vd_put_u64_le(uint8_t out[8], uint64_t value)
{
    size_t index;
    if (out == NULL) {
        return -1;
    }
    for (index = 0U; index < 8U; ++index) {
        out[index] = (uint8_t)(value >> (index * 8U));
    }
    return 0;
}

int vd_get_u64_le(const uint8_t in[8], uint64_t *value)
{
    uint64_t result = 0U;
    size_t index;
    if ((in == NULL) || (value == NULL)) {
        return -1;
    }
    for (index = 0U; index < 8U; ++index) {
        result |= (uint64_t)in[index] << (index * 8U);
    }
    *value = result;
    return 0;
}

struct wh_cpx16 vd_iq_conjugate(struct wh_cpx16 sample)
{
    sample.q = wh_sat16(-(int32_t)sample.q);
    return sample;
}

struct wh_cpx16 vd_iq_rotate90(struct wh_cpx16 sample)
{
    struct wh_cpx16 rotated;
    rotated.i = wh_sat16(-(int32_t)sample.q);
    rotated.q = sample.i;
    return rotated;
}

int vd_inject_tone(struct wh_cpx16 *grid, size_t nfft,
                   int32_t logical_bin, struct wh_cpx16 tone)
{
    size_t index;
    if ((grid == NULL) ||
        (wh_logical_bin_to_index(logical_bin, nfft, &index) != 0)) {
        return -1;
    }
    grid[index].i = wh_sat16((int32_t)grid[index].i + tone.i);
    grid[index].q = wh_sat16((int32_t)grid[index].q + tone.q);
    return 0;
}

int vd_grid_swap(struct wh_cpx16 *grid, size_t nfft,
                 int32_t first_bin, int32_t second_bin)
{
    size_t first_index;
    size_t second_index;
    struct wh_cpx16 temporary;
    if ((grid == NULL) ||
        (wh_logical_bin_to_index(first_bin, nfft, &first_index) != 0) ||
        (wh_logical_bin_to_index(second_bin, nfft, &second_index) != 0)) {
        return -1;
    }
    temporary = grid[first_index];
    grid[first_index] = grid[second_index];
    grid[second_index] = temporary;
    return 0;
}

int vd_zero_guard_bands(struct wh_cpx16 *grid, size_t nfft,
                        size_t guard_width)
{
    size_t offset;
    int64_t half;
    if ((grid == NULL) || (nfft == 0U) || ((nfft & 1U) != 0U) ||
        (nfft > (size_t)INT32_MAX) || (guard_width > (nfft / 2U))) {
        return -1;
    }
    half = (int64_t)(nfft / 2U);
    for (offset = 0U; offset < guard_width; ++offset) {
        int32_t lower = (int32_t)(-half + (int64_t)offset);
        int32_t upper = (int32_t)(half - (int64_t)guard_width +
                                  (int64_t)offset);
        size_t lower_index;
        size_t upper_index;
        (void)wh_logical_bin_to_index(lower, nfft, &lower_index);
        (void)wh_logical_bin_to_index(upper, nfft, &upper_index);
        grid[lower_index].i = 0;
        grid[lower_index].q = 0;
        grid[upper_index].i = 0;
        grid[upper_index].q = 0;
    }
    return 0;
}

static bool ring_valid(const struct vd_ring *ring)
{
    return (ring != NULL) &&
           (ring->read_index < VD_RING_CAPACITY) &&
           (ring->write_index < VD_RING_CAPACITY) &&
           (ring->count <= VD_RING_CAPACITY);
}

bool vd_ring_push(struct vd_ring *ring, int value)
{
    if (!ring_valid(ring) || (ring->count == VD_RING_CAPACITY)) {
        return false;
    }
    ring->entries[ring->write_index] = value;
    ring->write_index =
        (ring->write_index + 1U) % VD_RING_CAPACITY;
    ring->count++;
    return true;
}

bool vd_ring_peek(const struct vd_ring *ring, int *value)
{
    if (!ring_valid(ring) || (value == NULL) || (ring->count == 0U)) {
        return false;
    }
    *value = ring->entries[ring->read_index];
    return true;
}

bool vd_ring_drop_oldest(struct vd_ring *ring)
{
    if (!ring_valid(ring) || (ring->count == 0U)) {
        return false;
    }
    ring->read_index =
        (ring->read_index + 1U) % VD_RING_CAPACITY;
    ring->count--;
    return true;
}

uint32_t vd_next_generation(uint32_t current)
{
    uint32_t next = current + 1U;
    return (next == 0U) ? 1U : next;
}

int vd_timer_cancel(struct vd_timer *timer)
{
    if (timer == NULL) {
        return -1;
    }
    timer->active = false;
    timer->deadline = 0U;
    timer->generation = vd_next_generation(timer->generation);
    return 0;
}

const char *vd_state_name(enum vd_state state)
{
    switch (state) {
    case VD_OFF:
        return "OFF";
    case VD_SEARCHING:
        return "SEARCHING";
    case VD_CAMPED:
        return "CAMPED";
    case VD_CONNECTED:
        return "CONNECTED";
    default:
        return "UNKNOWN";
    }
}

static bool state_valid(enum vd_state state)
{
    return (state >= VD_OFF) && (state < VD_STATE_COUNT);
}

int vd_transition_table_validate(const struct vd_transition *table,
                                 size_t count)
{
    size_t outer;
    if ((table == NULL) && (count != 0U)) {
        return -1;
    }
    for (outer = 0U; outer < count; ++outer) {
        size_t inner;
        if (!state_valid(table[outer].from) ||
            !state_valid(table[outer].to) ||
            (table[outer].event == 0U)) {
            return -1;
        }
        for (inner = 0U; inner < outer; ++inner) {
            if ((table[inner].from == table[outer].from) &&
                (table[inner].event == table[outer].event)) {
                return -1;
            }
        }
    }
    return 0;
}

int vd_tlv_find(const uint8_t *frame, size_t frame_length,
                uint8_t wanted_type, const uint8_t **payload,
                uint8_t *payload_length)
{
    const uint8_t *found = NULL;
    uint8_t found_length = 0U;
    size_t offset = 0U;
    if ((payload == NULL) || (payload_length == NULL) ||
        ((frame == NULL) && (frame_length != 0U))) {
        return -1;
    }
    while (offset < frame_length) {
        uint8_t type;
        uint8_t length;
        if ((frame_length - offset) < 2U) {
            return -1;
        }
        type = frame[offset];
        length = frame[offset + 1U];
        offset += 2U;
        if ((size_t)length > (frame_length - offset)) {
            return -1;
        }
        if (type == wanted_type) {
            if (found != NULL) {
                return -1;
            }
            found = &frame[offset];
            found_length = length;
        }
        offset += length;
    }
    if (found == NULL) {
        return 0;
    }
    *payload = found;
    *payload_length = found_length;
    return 1;
}

int vd_flip_every_kth_bit(uint8_t *bytes, size_t length,
                          size_t first_bit, size_t step)
{
    size_t total_bits;
    size_t position;
    if ((bytes == NULL) || (step == 0U) ||
        (length > (SIZE_MAX / 8U))) {
        return -1;
    }
    total_bits = length * 8U;
    if (first_bit >= total_bits) {
        return -1;
    }
    position = first_bit;
    for (;;) {
        (void)wh_flip_bit(bytes, length, position);
        if ((step > (SIZE_MAX - position)) ||
            ((position + step) >= total_bits)) {
            break;
        }
        position += step;
    }
    return 0;
}

size_t vd_trace_count_value(const struct vd_trace_record *records,
                            size_t count, int32_t wanted)
{
    size_t matches = 0U;
    size_t index;
    if ((records == NULL) && (count != 0U)) {
        return 0U;
    }
    for (index = 0U; index < count; ++index) {
        if (records[index].value == wanted) {
            matches++;
        }
    }
    return matches;
}

int vd_clock_read(const struct vd_clock_ops *ops, void *context,
                  uint64_t *tick)
{
    if ((ops == NULL) || (ops->ticks == NULL) || (tick == NULL)) {
        return -1;
    }
    *tick = ops->ticks(context);
    return 0;
}

int vd_event_insert_stable(struct vd_event_list *list,
                           struct vd_event event)
{
    size_t index;
    if ((list == NULL) || (list->count > VD_EVENT_CAPACITY) ||
        (list->count == VD_EVENT_CAPACITY)) {
        return -1;
    }
    index = list->count;
    while ((index > 0U) &&
           (list->events[index - 1U].tick > event.tick)) {
        list->events[index] = list->events[index - 1U];
        index--;
    }
    list->events[index] = event;
    list->count++;
    return 0;
}
```

</details>

Build:

```bash
gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Werror \
    -Wstrict-prototypes -Wmissing-prototypes \
    -fsanitize=address,undefined -g \
    -Ipractice/mastery -Ipractice/ironman \
    practice/mastery/wizard_helpers.c \
    practice/ironman/variant_deck.c \
    practice/ironman/test_variant_deck.c \
    -o build/practice/test_variant_deck
ASAN_OPTIONS=detect_leaks=0 ./build/practice/test_variant_deck
```

Output:

```text
variant_deck passed
```

### 32.3.7. Hai mươi bug bắt buộc tự cài

Mỗi helper có một bug injection tương ứng. Cài một bug, chạy test đỏ, viết tên
regression test, rồi sửa:

| # | Bug phải cài |
|---:|---|
| 1 | Dùng MSB shift trong LSB helper |
| 2 | Encode byte cao trước |
| 3 | Shift trên `uint8_t` trước khi cast `uint64_t` |
| 4 | Negate `int16_t` trước khi widen |
| 5 | Xoay `(q,-i)` thay vì `(-q,i)` |
| 6 | Add không saturation |
| 7 | Dùng logical bin âm làm array index |
| 8 | Zero nhầm DC |
| 9 | Push khi full và overwrite unread entry |
| 10 | Peek làm tăng `read_index` |
| 11 | Drop quên giảm count |
| 12 | Cho generation wrap về zero |
| 13 | Cancel không tăng generation |
| 14 | Trả `NULL` cho unknown state |
| 15 | Chỉ check duplicate transition kề nhau |
| 16 | Return match trước khi validate trailing TLV |
| 17 | `position += step` overflow tạo loop vô hạn |
| 18 | Observer sửa record khi đếm |
| 19 | Fake clock dùng global thay context |
| 20 | Insert dùng `>=`, phá stable order khi cùng tick |

Sau 20 bug, xóa implementation và làm lần C. Đây là phần “bắt code cực nhiều”:
không chỉ viết happy path mà còn phải biết code sai trông như thế nào và test
nào giết được nó.

## 32.4. Bộ nhân khối lượng cho 49 bài đã có

Không viết lại nội dung 49 bài vì chúng đã có đầy đủ đề/hướng dẫn/phao. Thay
vào đó, sau **Bước 9 — Wizard Lab** của mỗi bài, bắt buộc áp dụng ma trận sau:

| Vòng | Mutation bắt buộc | Ví dụ với PHY/runtime/L2 |
|---|---|---|
| Type | Đổi width/signedness có chủ đích | `uint16_t` SN, `uint32_t` tick |
| Capacity | Test 0, 1, max, max+1 | TB length, ring count, TLV length |
| Ownership | Success/failure/retry | handle, DMA buffer, event payload |
| Time | before/equal/after/wrap | timer, HARQ timeout, reorder timer |
| Representation | encode/decode/round-trip | IQ16, descriptor, PDU |
| Fault | một bit/truncate/duplicate/reorder | CRC, replay, RLC gap |
| Observer | metric/checkpoint mới | energy, queue depth, state trace |
| Composition | nối producer→consumer | TX→channel→RX, IRQ→task |

Như vậy mỗi bài không còn là “một đáp án”. Nó trở thành ít nhất tám lần sửa C
có lý do. Phao source gốc của bài là baseline; source trong PHẦN XXXI/XXXII là
mẫu cho mutation, observer, script và composition.

## 32.5. Checkpoint C17 riêng, độc lập với kiến thức modem

Trước khi sang mỗi milestone project, phải vượt gate C tương ứng:

| Milestone | C gate phải làm không nhìn |
|---|---|
| Codec/PHY | integer promotion, saturation, array+count, bit codec |
| Channel | struct config, deterministic state, pure composition |
| Runtime | pointer lifetime, ring indices, atomic contract, callback+ctx |
| DMA/MMIO | volatile boundary, endian, alignment, state enum |
| L2/control | parser transaction, modular counters, transition table |
| Host/E2E | file parser, CLI args, subprocess/script, deterministic evidence |

Nếu modem concept đúng nhưng C gate fail, không sang milestone. Quay về Variant
Deck và viết lại nhóm helper liên quan.

## 32.6. Thành thạo không được đo bằng “đọc hết”

Tốt nghiệp C của tài liệu khi bạn có thể:

```text
nhìn một test → tự dựng prototype và implementation
nhìn một contract → tự dựng type và test
nhìn một bug → thu nhỏ thành helper tái hiện
nhìn một pipeline → chẻ thành stage có observer
nhìn một ý tưởng lạ → tạo representation + command + scenario
đóng toàn bộ phao → viết lại module
```

Nếu chỉ hoàn tất một lượt source đáp án, bạn mới ở mức “làm theo”. Nếu hoàn tất
quota 560 artifact, 20 bug injection, 36 buổi forge và hai lần rebuild project,
bạn đã luyện C bằng chính domain modem/baseband với khối lượng đủ lớn để tạo
phản xạ thực chiến.

<!-- ORIGINAL_TRAILING_BLANK_PRESERVED -->

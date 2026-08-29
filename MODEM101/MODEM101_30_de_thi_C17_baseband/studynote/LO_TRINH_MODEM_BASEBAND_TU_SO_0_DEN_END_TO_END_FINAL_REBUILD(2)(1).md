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

---

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

---

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

---

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

---

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

---

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

---

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

---

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

### 2. Tự đoán declaration/chương trình cần biểu diễn gì

Mỗi field cần type thể hiện đúng signedness, width, lifetime hoặc platform role; không có một type dùng cho tất cả.

### 3. Học đúng lượng kiến thức cần cho bài

- wire width khác memory size
- signedness theo miền giá trị
- enum/bool/uintptr_t/ssize_t có vai trò riêng

### 4. Tự làm / tự code

Chưa mở Bước 7. Viết câu trả lời hoặc file `practice/type_02.c`; với bài code, thêm test `main()` nhỏ nhất.

### 5. Chạy test / quan sát compiler và output

Điền đủ 10 dòng rồi thêm cột miền giá trị và lý do.

Với bài có C source, dùng warning nghiêm ngặt và sanitizer như hướng dẫn ở mục 1B.33A.

### 6. Debug

Nếu chỉ trả lời theo số lớn/nhỏ, kiểm tra lại field đó nằm trên wire, trong RAM, là address, state hay status.

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

</details>

### 8. Viết lại không nhìn lời giải

Đóng khối lời giải, viết lại câu trả lời/code từ trắng, rồi tự giải thích **type → miền giá trị → ownership/lifetime → representation modem**.

---

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

---

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

---

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

---

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

---

## 1B.40. Bài kiểu dữ liệu 7 — Pointer hay handle?

### 1. Đề bài nhỏ

Với từng tình huống, chọn raw pointer hay handle và giải thích:

1. Hàm FFT sửa buffer trong cùng task.
2. IRQ enqueue RX buffer cho deferred task.
3. Encode descriptor lên wire.
4. Local parser nhìn payload trong thời gian function call.
5. AP gửi buffer identity cho CP.

### 2. Tự đoán declaration/chương trình cần biểu diễn gì

Pointer phù hợp khi cùng lifetime/context; handle phù hợp khi vượt task/IRQ/IPC hoặc cần phát hiện stale identity.

### 3. Học đúng lượng kiến thức cần cho bài

- lifetime
- ownership transfer
- serialization và generation

### 4. Tự làm / tự code

Chưa mở Bước 7. Viết câu trả lời hoặc file `practice/type_07.c`; với bài code, thêm test `main()` nhỏ nhất.

### 5. Chạy test / quan sát compiler và output

Lập bảng 5 tình huống: lựa chọn, owner hiện tại, lifetime và lỗi cần tránh.

Với bài có C source, dùng warning nghiêm ngặt và sanitizer như hướng dẫn ở mục 1B.33A.

### 6. Debug

Nếu định encode raw pointer lên wire, dừng lại: address chỉ có nghĩa trong address space hiện tại.

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

</details>

### 8. Viết lại không nhìn lời giải

Đóng khối lời giải, viết lại câu trả lời/code từ trắng, rồi tự giải thích **type → miền giá trị → ownership/lifetime → representation modem**.

---

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

---

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

---

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

---

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

---

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

---

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

---

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

---

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

---

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

---

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

---

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

---

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

---

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

---

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

---

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

---

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

---

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

---

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

---

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

---

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

---

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

---

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

---

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

---

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

---

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

---

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

---

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

---

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

---

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

---

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

---

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

### Bài 40 — Đọc linker map, chưa cần viết

#### 1. Đề bài nhỏ

Build một chương trình bare-metal mẫu nhỏ rồi dùng:

```bash
arm-none-eabi-size -A firmware.elf
arm-none-eabi-nm firmware.elf
arm-none-eabi-objdump -h firmware.elf
```

Trả lời:

- `.text` size bao nhiêu?
- `.bss` có chiếm bytes trong ELF file giống `.data` không?
- reset symbol ở address nào?

Chỉ sau đó mới tự viết linker script.

#### 2. Tự đoán chương trình cần làm gì

Các tool ELF trả lời code/data nằm đâu, tốn bao nhiêu và reset symbol ở địa chỉ nào; bài này tạo báo cáo quan sát thay vì một thuật toán C.

**Viết dự đoán trước khi code:** input là gì, output là gì, trường hợp nào phải fail?

#### 3. Học đúng lượng kiến thức cần cho bài

- ELF sections/symbols
- linker map
- `size`, `nm`, `objdump`

Chỉ cần các ý trên để bắt đầu; chưa cần đọc trước module lớn hay 3GPP.

#### 4. Tự code

Làm trong note + `build/task40.elf`. Chưa mở Bước 7.

1. Chép lại **type/prototype từ đề**, không chép implementation.
2. Viết happy path nhỏ nhất.
3. Thêm validation/boundary theo contract.
4. Viết `main()` hoặc test gọi API bằng vector nhỏ tính tay được.
5. Compile ngay; không đợi viết “xong hết” mới compile.

#### 5. Chạy test / quan sát output

Lưu output ba tool vào note; tự trả lời địa chỉ reset, size `.text/.data/.bss` và kiểm tra câu trả lời bằng map file.

```bash
arm-none-eabi-gcc ... -Wl,-Map=build/task40.map -o build/task40.elf
arm-none-eabi-size -A build/task40.elf
arm-none-eabi-nm -n build/task40.elf
arm-none-eabi-objdump -h build/task40.elf
```

Ghi actual output cạnh expected output. “Không crash” chưa đủ; giá trị và state phải đúng.

#### 6. Debug

Nếu không thấy reset symbol, kiểm tra linker script `ENTRY`, symbol visibility và đúng ELF đang quan sát.

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

Bài này không có một “full C source” duy nhất vì mục tiêu là quan sát ELF. Checklist lời giải:

```bash
arm-none-eabi-size -A firmware.elf
arm-none-eabi-nm -n firmware.elf
arm-none-eabi-objdump -h firmware.elf
arm-none-eabi-objdump -t firmware.elf
```

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

---

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





## Capstone J — `rebuild_modem101_from_blank`: tự code lại toàn bộ đáp án

Đây là **bài tốt nghiệp thật sự của khóa**.

Capstone I chứng minh rằng bạn ghép được các block thành modem end-to-end. Capstone J buộc bạn chứng minh một chuyện khó hơn:

> **Đóng ZIP đáp án. Tạo repo trắng. Tự dựng lại project tương đương từ contract và mental model đã học.**

Không yêu cầu source giống từng dòng reference. Yêu cầu là bạn tự tái tạo được toàn bộ hệ thống với cùng lớp functionality và các invariant quan trọng.

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

---

## Challenge 2 — AGC toy

Input amplitude quá lớn/nhỏ.

Tính peak rồi scale block về target peak cố định bằng integer arithmetic.

Check zero block.

---

## Challenge 3 — CFO compensation

Channel xoay `+Δf`.

Receiver xoay `-Δf`.

Test waveform error trước/sau compensation.

---

## Challenge 4 — FFT reference testing

Tạo DFT floating reference và FFT fixed implementation.

So sánh với tolerance.

Không chỉ test một vector.

---

## Challenge 5 — PSS timing under noise

Random delay trong range 0..64.

Chạy 100 seeds.

Tính success rate.

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

---

## Challenge 7 — Stale buffer handle fuzz

Generate random slot/generation combinations.

`pool_get` không bao giờ trả pointer cho stale/invalid handle.

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

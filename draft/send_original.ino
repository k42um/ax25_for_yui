/*#include <SPI.h>

SPISettings mySetting = SPISettings(1000000, MSBFIRST, SPI_MODE0);

byte data = B10100010;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  SPI.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  SPI.beginTransaction(mySetting);
  SPI.transfer(data);
  SPI.endTransaction();
  delay(100);
}*/
/*
#include <Arduino.h>
#include <avr/interrupt.h>

volatile uint8_t sendData = B11111111;  // 出したいデータ（1バイト）
volatile uint8_t bitIndex = 7;

void setup() {
  // SPIスレーブ設定
  pinMode(MISO, OUTPUT);   // 出力
  pinMode(MOSI, INPUT);    // 未使用（でも設定は必要）
  pinMode(SCK, INPUT);     // マスターからのクロック
  pinMode(SS, INPUT_PULLUP); // 外部でLowに固定（SPI有効化のため）

  // SPIコントローラ設定
  SPCR = _BV(SPE) | _BV(SPIE);  // SPI有効 + 割り込み許可 (MODE0)
  SPSR = 0;                     // ダブルスピードなし

  // 最初の送信バイトをセット
  SPDR = sendData;

  sei();  // 全割り込み許可
}

ISR(SPI_STC_vect) {
  // マスターがクロック8回出すごとに呼ばれる（1バイト完了）
  // 今回は送信専用なので、SPDRを更新して次の送信データを準備
  SPDR = sendData;  // 同じデータを繰り返し出す
}

void loop() {
  // 必要があれば動的に送信データを更新できる
  // 例: sendData = 0b01010101;
}
*/


// === Arduino Mega 2560: SPI Slave (MODE0), 固定1バイト送信・CSなし(53はGND) 最終形 ===
// マスターは CPOL=0/CPHA=0（立ち上がりでサンプル）で動かしてください。
/*
#include <Arduino.h>
#include <avr/interrupt.h>

#define dataCount 64

// 送りたい1バイト。必要に応じて書き換え可（例: 0xFF, 0xA2 など）
volatile uint8_t tx_next = 0b00000000;  // 初期値
volatile uint8_t tx_data[dataCount] = {
  0x7E, 0x39, 0x29, 0x76, 0x05, 0x02, 0x02, 0x07, 0x39, 0x76, 0x19, 0x51, 0x59, 0x02, 0x86, 0xC0, 
  0x0F, 0x2A, 0x16, 0xA6, 0x04, 0x8E, 0xAE, 0x96, 0xC6, 0xD6, 0x04, 0x46, 0x4E, 0xF6, 0xEE, 0x76, 
  0x04, 0x66, 0xF6, 0x1E, 0x04, 0x56, 0xAE, 0xB6, 0x0E, 0xCE, 0x04, 0xF6, 0x6E, 0xA6, 0x4E, 0x04, 
  0x2E, 0x16, 0xA6, 0x04, 0x36, 0x86, 0x5E, 0x9E, 0x04, 0x26, 0xF6, 0xE6, 0x75, 0xF3, 0x3F, 0x00
};
//volatile uint8_t tx_next = 0b111111111;
//volatile int16_t count = 0;
volatile uint16_t count = 0;
void setup() {
  // 配線:
  //   SCK(52)  ← マスターSCK
  //   MISO(50) → マスターMISO（Arduinoから送信）
  //   MOSI(51) ← 未使用でも入力に
  //   SS(53)   ← GNDに直結（常時Low）※重要
  //   GND共通

  //Serial.begin(9600);

  pinMode(MISO, OUTPUT);
  pinMode(MOSI, INPUT);
  pinMode(SCK,  INPUT);
  pinMode(SS,   INPUT_PULLUP);  // 外部で必ずGND固定しておくこと

  // 不要な割り込みを止めてISR遅延を最小化（Timer0停止→millis()/delay()不可）
  TIMSK0 = 0;

  // SPI スレーブ有効化（MODE0: CPOL=0, CPHA=0）
  // SPE=1: SPI enable, SPIE=1: SPI割り込み許可, MSTR=0: スレーブ
  SPCR = _BV(SPE) | _BV(SPIE);  // MODE0なのでCPOL/CPHAは0のまま
  SPSR = 0;

  // 最初の送信バイトを必ずプリロード
  SPDR = tx_next;

  sei(); // 全割り込み許可
}

// 1バイト(8クロック)完了のたびに呼ばれる
ISR(SPI_STC_vect) {
  // 次バイトを即プリロード（最短処理）
  //if((count / 10000) % 2 == 0) SPDR = tx_next;
  /*if(count < dataCount){
    SPDR = tx_data[count];
    //Serial.println(tx_data[count], HEX);
  }
  //Serial.println(count);
  //Serial.println(count);
  count++;*//*
  uint8_t set = count % dataCount;
  SPDR = tx_data[set];
}

void loop() {
  // 送信データを動的に変えたい場合は、原子更新で上書きしてOK
  // 例：
  // cli(); tx_next = 0xA2; sei();
  // 以降のフレームで確実にその値が送られます
}*/

// === Arduino Mega 2560: SPI Slave (MODE0), インターバル付きループ送信 ===
// マスターは CPOL=0/CPHA=0 で、クロックを連続供給し続ける必要があります。

#include <Arduino.h>
#include <avr/interrupt.h>

// --- 設定値 ---
const uint16_t DATA_BYTES = 65;     // 送信したいデータのバイト数
const uint16_t INTERVAL_BYTES = 65; // インターバルの長さ（バイト単位）
const uint8_t  INTERVAL_DATA = 0x00; // インターバル中に送信し続けるデータ
// ---

volatile uint8_t tx_data[DATA_BYTES] = {
  0x7E, 0x39, 0x29, 0x76, 0x05, 0x02, 0x02, 0x07, 0x39, 0x76, 0x19, 0x51, 0x59, 0x02, 0x86, 0xC0, 
  0x0F, 0x2A, 0x16, 0xA6, 0x04, 0x8E, 0xAE, 0x96, 0xC6, 0xD6, 0x04, 0x46, 0x4E, 0xF6, 0xEE, 0x76, 
  0x04, 0x66, 0xF6, 0x1E, 0x04, 0x56, 0xAE, 0xB6, 0x0E, 0xCE, 0x04, 0xF6, 0x6E, 0xA6, 0x4E, 0x04, 
  0x2E, 0x16, 0xA6, 0x04, 0x36, 0x86, 0x5E, 0x9E, 0x04, 0x26, 0xF6, 0xE6, 0x75, 0xF3, 0x3F, 0x00, 0x7E
};

volatile uint16_t byte_counter = 0;   // 現在の期間でのバイトカウンタ
volatile bool sending_data = true;    // true: データ送信期間, false: インターバル期間

void setup() {
  pinMode(MISO, OUTPUT);
  pinMode(MOSI, INPUT);
  pinMode(SCK,  INPUT);
  pinMode(SS,   INPUT_PULLUP); // 外部で必ずGND固定

  // Timer0停止 (millis()/delay()不可)
  TIMSK0 = 0;

  // SPI スレーブ有効化 (MODE0), 割り込み許可
  SPCR = _BV(SPE) | _BV(SPIE);  
  SPSR = 0;

  // 最初のバイトをプリロード (データ送信期間から開始)
  SPDR = tx_data[0];
  byte_counter = 1; // 1バイト目をロード済みのためカウンタは1から

  sei(); // 全割り込み許可
}

// SPI 1バイト(8クロック)転送完了ごと
ISR(SPI_STC_vect) {

  if (sending_data) {
    // --- データ送信期間 ---
    if (byte_counter < DATA_BYTES) {
      // 次のデータをプリロード
      SPDR = tx_data[byte_counter];
      byte_counter++;
    } else {
      // データ送信完了 -> インターバル期間へ移行
      sending_data = false;
      byte_counter = 0;
      SPDR = INTERVAL_DATA; // インターバルの最初のバイトをプリロード
    }
  } 
  else {
    // --- インターバル期間 ---
    if (byte_counter < INTERVAL_BYTES) {
      // インターバルデータをプリロード (同じデータをセットし続ける)
      SPDR = INTERVAL_DATA; 
      byte_counter++;
    } else {
      // インターバル完了 -> データ送信期間へ移行
      sending_data = true;
      byte_counter = 0;
      SPDR = tx_data[0]; // データの最初のバイトをプリロード
    }
  }
  
  // ISRの最後で byte_counter をインクリメントするのではなく、
  // 次にロードするインデックスとして管理したほうがロジックが簡潔になるため、
  // 上記のように変更しました。
  // (SPDRにtx_data[0]をセットした時点で、byte_counterは次のインデックス1を指すべき)
  
  // --- 以下、修正版ロジック ---
  /*
  if (sending_data) {
    // --- データ送信期間 ---
    SPDR = tx_data[byte_counter];
    byte_counter++;
    if (byte_counter >= DATA_BYTES) {
      sending_data = false; // 次はインターバル
      byte_counter = 0;     // カウンタリセット
    }
  } 
  else {
    // --- インターバル期間 ---
    SPDR = INTERVAL_DATA;
    byte_counter++;
    if (byte_counter >= INTERVAL_BYTES) {
      sending_data = true;  // 次はデータ送信
      byte_counter = 0;     // カウンタリセット
    }
  }
  */
  // ※上記のコメントアウトしたロジックの方がシンプルで、
  //   setup()のプリロードも SPDR = tx_data[0]; byte_counter = 1; ではなく
  //   SPDR = tx_data[0]; byte_counter = 0; とし、
  //   ISRの先頭で byte_counter++; (データ期間) または byte_counter=0; (期間移行)
  //   としてからSPDRにセットする方が直感的かもしれません。
  //   混乱を避けるため、元のロジック（コメントアウトしていない方）を推奨します。
}

void loop() {
  // loop()は空でOK。すべて割り込みで処理されます。
}
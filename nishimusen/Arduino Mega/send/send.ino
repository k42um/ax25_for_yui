#include <Arduino.h>
#include <avr/interrupt.h>
#include "ax25.h"

// Constructor
ax25 AX25;

// Data Sets
volatile uint8_t tx_data[dataCount] = {
    0x7E, 0x39, 0x29, 0x76, 0x05, 0x02, 0x02, 0x07, 0x39, 0x76, 0x19, 0x51, 0x59, 0x02, 0x86, 0xC0, 
    0x0F, 0x2A, 0x16, 0xA6, 0x04, 0x8E, 0xAE, 0x96, 0xC6, 0xD6, 0x04, 0x46, 0x4E, 0xF6, 0xEE, 0x76, 
    0x04, 0x66, 0xF6, 0x1E, 0x04, 0x56, 0xAE, 0xB6, 0x0E, 0xCE, 0x04, 0xF6, 0x6E, 0xA6, 0x4E, 0x04, 
    0x2E, 0x16, 0xA6, 0x04, 0x36, 0x86, 0x5E, 0x9E, 0x04, 0x26, 0xF6, 0xE6, 0x75, 0xF3, 0x3F, 0x00
};

const char sendMessage[] = ""Hello, Yui Project. This is Nishi Musen. Sorry for being a pain."";
const size_t sendMessageLen = strlen(sendMessage);

// Flags
volatile bool isTxDone = false;

// Arduino Formula
void setup() {
    // 配線:
    //   Spresense  ←    西無線
    //   SCK(52)    ←    TRCLK
    //   MISO(50)   →    TRDAT
    //   MOSI(51)                 Not used in TX mode 
    //   SS(53)     ←    GND      Must keep connection to GND because Arduino Mega is SPI slave
    //   GND        ←    GND

    // Serial.begin(9600); // for debug

    pinMode(MISO, OUTPUT);
    pinMode(MOSI, INPUT); // not required
    pinMode(SCK,  INPUT);
    pinMode(SS,   INPUT_PULLUP);

    TIMSK0 = 0;

    // SPI スレーブ有効化（MODE0: CPOL=0, CPHA=0）
    // SPE=1: SPI enable, SPIE=1: SPI割り込み許可, MSTR=0: スレーブ
    SPCR = _BV(SPE) | _BV(SPIE);  // MODE0なのでCPOL/CPHAは0のまま
    SPSR = 0;

    cli(); // 割り込み禁止
}

// NOTE : sei()：割り込み許可／cli():割り込み禁止

// 1バイト(8クロック)完了のたびに呼ばれる
ISR(SPI_STC_vect) {

    SPDR = "HI!" // TODO : データ読み出し指示を書く

    // TODO : 送信データがnullなら（送信が完了したら）、isTxDoneフラグを立てる処理を追加
}

void loop() {

    // TODO : この辺もライブラリ編集し、一つの関数にまとめたい！
    AX25.arrayInit();
    AX25.setCallsignAndSsid();

    strcpy(message, sendMessage);

    AX25.formatPacket(strlen(sendMessage));

    // ここまでで、AX.25のパケット構築は完了している
    

}
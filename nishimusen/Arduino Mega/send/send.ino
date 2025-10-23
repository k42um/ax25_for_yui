#include <Arduino.h>
#include <avr/interrupt.h>
#include "ax25.h"

// Constructor
AX25 AX25;

// Data Sets
const char sendMessage[] = "Hello, Yui Project. This is Nishi Musen. Sorry for being a pain.";

// Flags
volatile bool isTxDone = false;

// Counter
volatile int sendCount = 0;
volatile int finalNum = 0;

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

    finalNum = AX25.formatPacket(strlen(sendMessage), sendMessage);

    // TIMSK0 = 0; // タイマ割り込みを無効化

    // SPI スレーブ有効化（MODE0: CPOL=0, CPHA=0）
    // SPE=1: SPI enable, SPIE=1: SPI割り込み許可, MSTR=0: スレーブ
    SPCR = _BV(SPE) | _BV(SPIE);  // MODE0なのでCPOL/CPHAは0のまま
    SPSR = 0;

    cli(); // 割り込み禁止

    delay(1000);
}

// NOTE : sei()：割り込み許可／cli():割り込み禁止
// SPCR |= _BV(SPIE); : SPI割り込み許可
// SPCR &= ~_BV(SPIE); : SPI割り込み禁止

// 1バイト(8クロック)完了のたびに呼ばれる
ISR(SPI_STC_vect) {
    if(sendCount < finalNum){
        SPDR = AX25.getPacketByte(sendCount++);
    }else{
        isTxDone = true;
    }
}

void loop() {
    finalNum = AX25.formatPacket(strlen(sendMessage), sendMessage);

    // ここまでで、AX.25のパケット構築は完了している

    delay(10);

    SPDR = AX25.getPacketByte(sendCount++);

    delay(10);

    sei();

    /*

    // for debug

    for(int i = 0; i < MAX_MESSAGE_SIZE; i++){
        uint8_t temp = message[i];
        if(temp = NULL_ASCII) break;
        Serial.print(temp, HEX);
        Serial.print(" ");
    }
    */

    while(1){ // 一回送信が完了するまで待機
        if(isTxDone == true){
            cli();
            break;
        }
        //delay(1);
    }

    // delay(2000);

    isTxDone = false;
    sendCount = 0;
    finalNum = 0;
}
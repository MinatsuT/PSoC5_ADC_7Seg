/* ========================================
 *
 * Copyright Minatsu, 2018
 * All Rights Reserved
 *
 * ========================================
 */
#include "project.h"
#include "stdio.h"

// 小さいほうの値を返すマクロ
#define MIN(x, y) ((x < y) ? x : y)

// 大きいほうの値を返すマクロ
#define MAX(x, y) ((x > y) ? x : y)

// UARTへの出力に使う文字列バッファのサイズ。
#define TRANSMIT_BUFFER_SIZE 80

// UARTの文字列バッファ。
char buf[TRANSMIT_BUFFER_SIZE];

// ADCの出力値（電圧）を格納する変数。小数値を格納したいのでfloat型(浮動小数点型)
float volt;

// 気圧の値。小数値を格納したいのでfloat型(浮動小数点型)
float pascal;

// 平均値（移動平均）。updateAvg()関数で更新する。
float avg = 0.0;

// 移動平均を更新するときの、瞬時値に対するの重み。
// これを小さくすると、滑らかに平均化されるが追従性が悪くなる。大きくすると、追従性はよくなるが瞬時値のブレの影響を受けやすくなる。
// UARTのデバッグ出力を見て、計測間隔に合わせて調整するとよい。
#define AVG_WEIGHT (0.5)

// このファイル内で定義している自前の関数のプロトタイプ宣言
float updateAvg(float val);
void setAvg(float val);
float lerp(float inMin, float inMax, float outMin, float outMax, float in);

int main(void) {
    CyGlobalIntEnable; /* Enable global interrupts. */

    /* Place your initialization/startup code here (e.g. MyInst_Start()) */
    UART_1_Start();
    LED_Driver_1_Start();
    ADC_DelSig_1_Start();
    ADC_DelSig_1_StartConvert();

    UART_1_PutString("ADC start\n");
    uint8_t initialFlag = 1;

    for (;;) {
        if (ADC_DelSig_1_IsEndConversion(ADC_DelSig_1_RETURN_STATUS)) {
            // ADCから電圧を取得。単位がmVなので、Vに変換するため1000.0で割る。その結果を変数voltに代入。
            volt = ADC_DelSig_1_CountsTo_mVolts(ADC_DelSig_1_GetResult16()) / 1000.0;
            
            // 0.8～4.0の値を、0.0～5.0の範囲に線形変換する。その結果を変数pascalに代入。
            pascal = lerp(0.8, 4.0, 0.0, 5.0, volt);

            if (initialFlag) {
                // 初回のみ、平均値（移動平均）を直接セットする。そうしないと、起動直後に0.0からゆっくり変化してしまってよくない。
                setAvg(pascal);
                initialFlag = 0;
            }
            
            // 平均値（移動平均）を更新。平均値は、上のほうで定義している変数avgとに代入される。
            updateAvg(pascal);
            
            // 7セグLEDへ出力
            //  気圧の値を1000倍して、小数点第３位が１の位に来るようにする：0.123 -> 123
            //  LEDの出力開始位置：一番左(0)
            //  LEDの桁数：4
            //  表示方法：右寄せ、かつ、数字が４桁未満なら未使用桁に0を表示 例１：123->0123　例２：12 -> 0012
            LED_Driver_1_Write7SegNumberDec(avg * 1000, 0, 4, LED_Driver_1_ZERO_PAD);

            // 小数点を一番左の桁(0)に表示
            LED_Driver_1_PutDecimalPoint(1, 0);
            
            // UARTへデバッグ出力
            sprintf(buf, "ADC=%5.3f[V]  Raw=%5.3f[MPa] Average=%5.3f[MPa]\r\n", volt, pascal,avg);
            UART_1_PutString(buf);
        }
        CyDelay(100);
    }
}

// 平均値を更新
float updateAvg(float val) {
    avg = avg * (1 - AVG_WEIGHT) + val * AVG_WEIGHT;
    return avg;
}

// 平均値を直接セット
void setAvg(float val) { avg = val; }

// 線形補間（上限、下限制限付き）
float lerp(float inMin, float inMax, float outMin, float outMax, float in) {
    float out = (in - inMin) * (outMax - outMin) / (inMax - inMin);
    return MIN(outMax, MAX(outMin, out));
}
/* [] END OF FILE */

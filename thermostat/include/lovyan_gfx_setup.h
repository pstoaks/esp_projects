/////////////////////////////////////////////////////////////////
/*
  ESP32 | LVGL8 | Ep 0. GFX Setup (ft. LovyanGFX)
  Video Tutorial: https://youtu.be/IPCvQ4o_WP8
  Created by Eric N. (ThatProject)
*/
/////////////////////////////////////////////////////////////////
#pragma once

#include <lvgl.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

static const unsigned TFT_HEIGHT = 320U;
static const unsigned TFT_WIDTH = 240U;

class LGFX : public lgfx::LGFX_Device
{
lgfx::Panel_ILI9341 _panel_instance;
lgfx::Bus_SPI       _bus_instance;   // SPIバスのインスタンス
lgfx::Light_PWM     _light_instance;
lgfx::Touch_XPT2046 _touch_instance;

public:
  LGFX(void)
  {
    { // バス制御の設定を行います。
      auto cfg = _bus_instance.config();    // バス設定用の構造体を取得します。
      cfg.spi_host = VSPI_HOST;     // 使用するSPIを選択  (VSPI_HOST or HSPI_HOST)
      cfg.spi_mode = 0;             // SPI通信モードを設定 (0 ~ 3)
      cfg.freq_write = 40000000;    // 送信時のSPIクロック (最大80MHz, 80MHzを整数で割った値に丸められます)
      cfg.freq_read  = 16000000;    // 受信時のSPIクロック
      cfg.spi_3wire  = false;        // 受信をMOSIピンで行う場合はtrueを設定
      cfg.use_lock   = true;        // トランザクションロックを使用する場合はtrueを設定
      cfg.dma_channel = 1;          // Set the DMA channel (1 or 2. 0=disable)   使用するDMAチャンネルを設定 (0=DMA不使用)
      cfg.pin_sclk = 18;            // SPIのSCLKピン番号を設定
      cfg.pin_mosi = 23;            // SPIのMOSIピン番号を設定
      cfg.pin_miso = 19;            // SPIのMISOピン番号を設定 (-1 = disable)
      cfg.pin_dc   = 21;            // SPIのD/Cピン番号を設定  (-1 = disable)
      _bus_instance.config(cfg);    // 設定値をバスに反映します。
      _panel_instance.setBus(&_bus_instance);      // バスをパネルにセットします。
    }

    { // 表示パネル制御の設定を行います。
      auto cfg = _panel_instance.config();    // 表示パネル設定用の構造体を取得します。
      cfg.pin_cs           =    15;  // CSが接続されているピン番号   (-1 = disable)
      cfg.pin_rst          =     4;  // RSTが接続されているピン番号  (-1 = disable)
      cfg.pin_busy         =    -1;  // BUSYが接続されているピン番号 (-1 = disable)
      cfg.memory_width     =   TFT_WIDTH;  // ドライバICがサポートしている最大の幅
      cfg.memory_height    =   TFT_HEIGHT;  // ドライバICがサポートしている最大の高さ
      cfg.panel_width      =   TFT_WIDTH;  // 実際に表示可能な幅
      cfg.panel_height     =   TFT_HEIGHT;  // 実際に表示可能な高さ
      cfg.offset_x         =     0;  // パネルのX方向オフセット量
      cfg.offset_y         =     0;  // パネルのY方向オフセット量
      cfg.offset_rotation  =     0;  // 回転方向の値のオフセット 0~7 (4~7は上下反転) Landscape, SD card up.
      cfg.dummy_read_pixel =     8;  // ピクセル読出し前のダミーリードのビット数
      cfg.dummy_read_bits  =     1;  // ピクセル以外のデータ読出し前のダミーリードのビット数
      cfg.readable         =  true;  // データ読出しが可能な場合 trueに設定
      cfg.invert           = false;  // Set to true if the light and darkness of the panel is reversed
      cfg.rgb_order        = false;  // パネルの赤と青が入れ替わってしまう場合 trueに設定
      cfg.dlen_16bit       = false;  // データ長を16bit単位で送信するパネルの場合 trueに設定
      cfg.bus_shared       =  true;  // SDカードとバスを共有している場合 trueに設定(drawJpgFile等でバス制御を行います)

      _panel_instance.config(cfg);
    }

    { // バックライト制御の設定を行います。（必要なければ削除）
      // Backlight LED control
      auto cfg = _light_instance.config();    // バックライト設定用の構造体を取得します。

      cfg.pin_bl = 5;              // バックライトが接続されているピン番号
      cfg.invert = false;           // バックライトの輝度を反転させる場合 true
      cfg.freq   = 44100;           // バックライトのPWM周波数
      cfg.pwm_channel = 7;          // 使用するPWMのチャンネル番号

      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);  // バックライトをパネルにセットします。
    }

    { // タッチスクリーン制御の設定を行います。（必要なければ削除）
      auto cfg = _touch_instance.config();
      cfg.x_min      = 0;    // タッチスクリーンから得られる最小のX値(生の値)
      cfg.x_max      = TFT_WIDTH-1;  // タッチスクリーンから得られる最大のX値(生の値)
      cfg.y_min      = 0;    // タッチスクリーンから得られる最小のY値(生の値)
      cfg.y_max      = TFT_HEIGHT-1;  // タッチスクリーンから得られる最大のY値(生の値)
      cfg.pin_int    = -1;   // INTが接続されているピン番号
      cfg.bus_shared = true; // 画面と共通のバスを使用している場合 trueを設定
      cfg.offset_rotation = 0;// 表示とタッチの向きのが一致しない場合の調整 0~7の値で設定
      cfg.spi_host = VSPI_HOST;// 使用するSPIを選択 (HSPI_HOST or VSPI_HOST)
      cfg.freq = 1000000;     // SPIクロックを設定
      cfg.pin_sclk = 18;     // SCLKが接続されているピン番号
      cfg.pin_mosi = 23;     // MOSIが接続されているピン番号
      cfg.pin_miso = 19;     // MISOが接続されているピン番号
      cfg.pin_cs   = 22;     //   CSが接続されているピン番号
      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);  // タッチスクリーンをパネルにセットします。
    }
    setPanel(&_panel_instance); // 使用するパネルをセットします。
  }
}; // class LGFX

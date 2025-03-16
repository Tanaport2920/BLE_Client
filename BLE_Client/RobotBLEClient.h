#ifndef ROBOT_BLE_CLIENT_H
#define ROBOT_BLE_CLIENT_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEClient.h>
#include <BLEScan.h>
#include <BLERemoteService.h>
#include <BLERemoteCharacteristic.h>

// コントローラ側と同じ ControllerData 構造を使い、受信する
// (ボタンのステータスやアナログスティックなど、必要に応じて拡張する)
struct ControllerData {
  uint8_t buttons;
  int16_t x;
  int16_t y;
};

class RobotBLEClient {
public:
  RobotBLEClient();
  ~RobotBLEClient();

  // BLEクライアント初期化
  void begin(const char* deviceName = "ESP32C6 Client");

  // コントローラ(サーバ)へ接続
  // serviceUUID, characteristicUUID: コントローラ側と一致するUUID
  bool connectToController(BLEUUID serviceUUID, BLEUUID characteristicUUID);

  // Notifyで受け取ったデータを取得
  ControllerData getControllerData();

  // Arduinoのloopで呼ぶ（切断が起きたときに再接続する）
  void update();

  // 接続状態を返す
  bool isConnected() const { return m_connected; }

private:
  // Notifyコールバックを設定するための静的関数
  static void notifyCallback(
    BLERemoteCharacteristic* pBLERemoteCharacteristic,
    uint8_t* pData,
    size_t length,
    bool isNotify);

  // 内部で再接続を試みる関数
  bool reconnectIfNeeded();

private:
  bool m_connected;
  BLEClient* m_pClient;
  BLERemoteCharacteristic* m_pRemoteCharacteristic;

  // コントローラから受信した最新データ (静的メンバ)
  static ControllerData s_controllerData;

  // 「再接続用」に保存しておくUUID文字列
  // connectToController() で代入し、update() で使い回す
  BLEUUID m_serviceUUIDstr;
  BLEUUID m_charUUIDstr;
};

#endif
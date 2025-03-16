/*
  ESP32C6 BLE Client Example
  ・ロボット本体側
  ・BLEサーバーをスキャン
  ・目的のUUIDを持つサーバーを発見して接続
  ・コントローラからのデータ送信ごとに応答（Notify）
*/

#include <Arduino.h>
// BLE関連（クライアント）のラッピング
#include <RobotBLEClient.h>

// サーバー側と合わせたUUIDを設定
static BLEUUID MY_SERVICE_UUID("12345678-1234-5678-1234-56789abcdef0");        
static BLEUUID MY_CHARACTERISTIC_UUID("abcdef01-1234-5678-1234-56789abcdef0");

// ロボット(BLEクライアント)
RobotBLEClient myRobot;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Robot (Client) starting...");

  // BLEクライアント初期化
  myRobot.begin("MyRobotClient");

  if(!myRobot.connectToController(MY_SERVICE_UUID, MY_CHARACTERISTIC_UUID)) {
      Serial.println("コントローラへの接続失敗。コントローラの電源は入っていますか？？");
    } else {
      Serial.println("コントローラへの接続成功");
    }
}

void loop() {
  // 受信したデータを1秒ごとに表示
  static unsigned long prevMs = 0;
  if (millis() - prevMs > 30) {
    prevMs = millis();

    if (myRobot.isConnected()) {
      // 最新の入力情報を取得
      ControllerData data = myRobot.getControllerData();

      // ボタン判定
      bool a = (data.buttons & 0x01);
      bool b = (data.buttons & 0x02);
      bool x = (data.buttons & 0x04);
      bool y = (data.buttons & 0x08);

      Serial.print("ABXY = ");
      Serial.print(a ? "A " : "- ");
      Serial.print(b ? "B " : "- ");
      Serial.print(x ? "X " : "- ");
      Serial.print(y ? "Y " : "- ");
      Serial.print(" | X=");
      Serial.print(data.x);
      Serial.print(", Y=");
      Serial.println(data.y);
    }
    else {
      Serial.println("コントローラ接続なし");
    }
  }

  // ライブラリのupdate
  myRobot.update();
}

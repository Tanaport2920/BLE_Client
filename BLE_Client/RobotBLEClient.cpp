#include "RobotBLEClient.h"

// 静的メンバの実体を定義
ControllerData RobotBLEClient::s_controllerData = {0};

// スキャンで見つかったデバイスを保存するためのグローバル変数
static BLEAdvertisedDevice* g_foundDevice = nullptr;

// ----- スキャンコールバック -----
static BLEUUID serviceUUID("12345678-1234-5678-1234-56789abcdef0"); // サンプル
class RobotAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    // サーバーがアドバタイズしているサービスUUIDをチェック
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      // サービスUUIDが一致したらスキャンを停止して接続へ進む
      BLEDevice::getScan()->stop();
      g_foundDevice = new BLEAdvertisedDevice(advertisedDevice);
    }
  }
};

// ----- Notifyコールバック (静的関数) -----
void RobotBLEClient::notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify)
{
  if (length == sizeof(ControllerData)) {
    // ControllerDataをそのまま構造体にコピー
    memcpy(&s_controllerData, pData, length);
    // デバッグ出力など
    /*
    Serial.println("[RobotBLEClient] Notified new controller data!");
    Serial.print(" buttons=0x"); Serial.println(s_controllerData.buttons, HEX);
    Serial.print(" x="); Serial.println(s_controllerData.x);
    Serial.print(" y="); Serial.println(s_controllerData.y);
    */
  }
  else {
    Serial.print("[RobotBLEClient] BrokenData Get, data length: ");
    Serial.println(length);
  }
}

// ----- コンストラクタ・デストラクタ -----
RobotBLEClient::RobotBLEClient() {
  m_connected = false;
  m_pClient   = nullptr;
  m_pRemoteCharacteristic = nullptr;
}

RobotBLEClient::~RobotBLEClient() {
}

// ----- 初期化 -----
void RobotBLEClient::begin(const char* deviceName) {
  BLEDevice::init(deviceName);

  // スキャン設定
  BLEScan* pScan = BLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(new RobotAdvertisedDeviceCallbacks());
  // アクティブスキャン(ScanReqを投げてScanRspを受け取る)
  pScan->setActiveScan(true);
  // Interval, Window
  pScan->setInterval(100);
  pScan->setWindow(99);
}

// ----- コントローラへ接続 -----
bool RobotBLEClient::connectToController(BLEUUID serviceUUIDIn, BLEUUID characteristicUUIDIn) {
  // 再接続用に保存 (update()で再利用)
  m_serviceUUIDstr = serviceUUIDIn;
  m_charUUIDstr    = characteristicUUIDIn;

  // まずスキャン (5秒間)
  g_foundDevice = nullptr;
  BLEDevice::getScan()->start(5, false);
  if (g_foundDevice == nullptr) {
    Serial.println("[RobotBLEClient] デバイスが見つかりませんでした");
    return false;
  }

  // 見つかったデバイスに接続
  m_pClient = BLEDevice::createClient();
  if(!m_pClient->connect(g_foundDevice)) {
    Serial.println("[RobotBLEClient] デバイスへ接続ができませんでした。");
    return false;
  }
  Serial.println("[RobotBLEClient] デバイスへ接続成功");

  // サービスを取得
  BLERemoteService* pService = m_pClient->getService(m_serviceUUIDstr);
  if (pService == nullptr) {
    Serial.println("[RobotBLEClient] 指定サービスが見つかりません");
    m_pClient->disconnect();
    return false;
  }

  // キャラクタリスティックを取得
  m_pRemoteCharacteristic = pService->getCharacteristic(m_charUUIDstr);
  if (m_pRemoteCharacteristic == nullptr) {
    Serial.println("[RobotBLEClient] キャラクタリスティックが見つかりません");
    m_pClient->disconnect();
    return false;
  }

  // Notifyを受け取るよう登録
  if (m_pRemoteCharacteristic->canNotify()) {
    m_pRemoteCharacteristic->registerForNotify(notifyCallback);
    Serial.println("[RobotBLEClient] Notify受信登録成功");
  } else {
    Serial.println("[RobotBLEClient] Notify受信登録失敗(CharacteristicにNOTIFYがありません)");
  }

  m_connected = true;
  return true;
}

// ----- 受信データ取得 -----
ControllerData RobotBLEClient::getControllerData() {
  return s_controllerData;
}

// ----- update()で接続を監視・再接続 -----
void RobotBLEClient::update() {
  // すでに接続中の場合、実際にBLEClientが切断されてないかチェック
  if (m_connected && m_pClient) {
    if (!m_pClient->isConnected()) {
      Serial.println("[RobotBLEClient] 切断を検知しました。再接続を試みます。");
      m_connected = false;
    }
  }

  // 接続されていない場合、一定時間おきに再接続を試みる
  if (!m_connected) {
    reconnectIfNeeded();
  }
}

// ----- 再接続を試みる実装 -----
bool RobotBLEClient::reconnectIfNeeded() {
  static unsigned long lastAttempt = 0;
  const unsigned long RECONNECT_INTERVAL = 10000; // 10秒ごとに再試行

  // まだインターバルに達していなければ何もしない
  if (millis() - lastAttempt < RECONNECT_INTERVAL) {
    return false;
  }
  lastAttempt = millis();

  Serial.println("[RobotBLEClient] 再接続スキャンを開始します...");
  // 先ほど保存したUUIDを使ってconnectToController
  bool result = connectToController(m_serviceUUIDstr, m_charUUIDstr);
  if (result) {
    Serial.println("[RobotBLEClient] 再接続成功!");
  } else {
    Serial.println("[RobotBLEClient] 再接続失敗...");
  }
  return result;
}

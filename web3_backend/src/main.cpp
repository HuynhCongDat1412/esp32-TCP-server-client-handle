#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>


const char *ssid = "I-Soft";          // Replace with your network SSID
const char *password = "i-soft@2023"; // Replace with your network password
WiFiServer* tcpServer = nullptr;

// put function declarations here:
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>ESP32 Web Server</title>
  <meta charset="UTF-8">
  <link rel="icon" href="data:,">
  <script>
  var ws;
  window.onload = function() {
    ws = new WebSocket('ws://' + location.host + '/ws');
    ws.onmessage = onMessage;
    ws.onopen = onOpen;
    ws.onclose = onClose;
    ws.onerror = onError;
  };
  function onMessage(event) {
    var resp = document.getElementById('server_response');
    if (resp.value.length > 0) resp.value += '\n';
    resp.value += event.data;
    resp.scrollTop = resp.scrollHeight;
  }
  function onOpen() {
    console.log('WebSocket connection opened');
  }
  function onClose() {
    console.log('WebSocket connection closed');
  }
  function onError(error) {
    console.error('WebSocket error:', error);
  }
  
  function sendCommand() {
    var cmd = document.getElementById('client_cmd').value;
    if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(cmd);
    } else {
    alert('WebSocket is not connected');
    }
  }
  function sendLogin() {
    var username = document.getElementById('username').value;
    var password = document.getElementById('password').value;
    var msg = JSON.stringify({username: username, password: password});
    document.getElementById('client_cmd').value = msg;
    ws.send(msg);
    return false; // Không submit form truyền thống
  }
  </script>
</head>
<body>
  <h1>Hello from ESP32!</h1>
  <p>Trang web này chạy trên ESP32.</p>
  <div class="container">
    <div class="row pt-5">
      <form onsubmit="return sendLogin();">
        <label for="username">Tên đăng nhập:</label>
        <input type="text" id="username" name="username"><br><br>
        <label for="password">Mật khẩu:</label>
        <input type="password" id="password" name="password"><br><br>
        <button type="submit">Đăng nhập</button>
      </form>
    </div>
    <hr>
    <div>
      <label for="client_cmd">Lệnh gửi từ client:</label><br>
      <textarea id="client_cmd" rows="8" cols="50"></textarea><br>
      <button type="button" onclick="sendCommand()">Gửi lệnh</button>
    </div>
    <hr>
    <div>
      <label for="server_response">Tin nhắn từ server:</label><br>
      <textarea id="server_response" rows="8" cols="50" readonly></textarea>
    </div>
  </div>
</body>
</html>
)rawliteral";

AsyncWebServer server(80);
// AsyncWebSocket ws("/ws"); nếu dùng WebSocket

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                      void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_DATA) {
    String msg = "";
    for (size_t i = 0; i < len; i++) msg += (char)data[i];
    Serial.println("Client gửi: " + msg);
    client->text("Đã nhận: " + msg);
    // Đăng nhập: "LOGIN username password"
    // Xử lý nếu client gửi JSON dạng {username: "...", password: "..."}
    if (msg.startsWith("{") && msg.endsWith("}")) {
      // Đơn giản dùng ArduinoJson để parse (nên thêm thư viện ArduinoJson vào project)
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, msg);
      if (!error) {
      String username = doc["username"] | "";
      String password = doc["password"] | "";
      if (username == "admin" && password == "admin") {
        client->text("Đăng nhập thành công!");
      } else {
        client->text("MInh gaf!");
      }
      } else {
      client->text("Lỗi định dạng JSON!");
      }
    }
    // Đăng nhập: "LOGIN username password"
    else if (msg.startsWith("LOGIN ")) {
      int firstSpace = msg.indexOf(' ', 6);
      String username = msg.substring(6, firstSpace);
      String password = msg.substring(firstSpace + 1);
      if (username == "admin" && password == "admin") {
      client->text("Đăng nhập thành công!");
      } else {
      client->text("Sai tên đăng nhập hoặc mật khẩu!");
      }
    }
    // Gửi lệnh: "CMD <lệnh>"
    else if (msg.startsWith("CMD ")) {
      String cmd = msg.substring(4);
      String response = "Server đã nhận lệnh: " + cmd;
      client->text(response);
    }
    // Trường hợp khác
    else {
      client->text("Lệnh không hợp lệ!");
    }
  }
}


StaticJsonDocument<200> parseClientJson(const String& msg) {
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, msg);
  StaticJsonDocument<200> result;
  if (!error) {
    result["role"] = doc["role"] | "";
    result["status"] = doc["status"] | "";
    result["data"] = doc["data"] | "";
    result["ip"] = doc["ip"] | "";
    result["port"] = doc["port"] | "";
  } else {
    result["role"] = "";
    result["status"] = "";
    result["data"] = "";
    result["ip"] = "";
    result["port"] = "";
  }
  return result;
}


void handleTCPclient() {
    if (!tcpServer) return; // Nếu server chưa được khởi tạo
    WiFiClient client = tcpServer->available();
    if (client) {
        Serial.println("New client connected");
        // Xử lý client kết nối
        while (client.connected()) {
            if (client.available()) {
              String msg = client.readStringUntil('\n');
              StaticJsonDocument<200> doc1 = parseClientJson(msg);

              String role = doc1["role"] | "";
              String status = doc1["status"] | "";
              String data = doc1["data"] | "";

              if (role == "client") {
                if (status == "disconnect")
                {
                  Serial.println("Client requested disconnect");
                  client.println("Disconnected by server");
                  break; // Thoát khỏi vòng lặp để ngắt kết nối client
                }
                else if (data.length() > 0)
                {
                  Serial.println("Client data: " + data);
                  client.println("Server received: " + data);
                }
                else {
                  client.println("No data received");
                }
              }
              else {
                client.println("Invalid role");
              }
          }
        }
        if(!client) return; 
        
        Serial.println("Client disconnected");
        client.stop();     
    }
  }
void stopAndDeleteTCPServer() {
  if (tcpServer) {
    tcpServer->stop();
    delete tcpServer;
    tcpServer = nullptr;
    Serial.println("TCP Server stopped and deleted");
  }
}

String msgSerial;
String currentRole;

// Nhận trạng thái từ Serial để xử lý role của tcp 
void serialHandler() {
  if (Serial.available()) {
    msgSerial = Serial.readStringUntil('\n');
    Serial.println("Received from Serial: " + msgSerial);
    // Xử lý lệnh từ Serial nếu cần
  

    if (msgSerial.startsWith("{") && msgSerial.endsWith("}")) {
        // Đơn giản dùng ArduinoJson để parse (nên thêm thư viện ArduinoJson vào project)
        StaticJsonDocument<200> obj = parseClientJson(msgSerial);       
          String ip = obj["ip"] | "";
          String role = obj["role"] | "";
          String status = obj["status"] | "";
          int port = String(obj["port"]| "").toInt() ;
          if (role == "server") {
            currentRole = "server";
            Serial.println("Server IP: " + ip);
            Serial.println("Server Port: " + port);
            stopAndDeleteTCPServer(); // Dừng và xóa server cũ nếu có
            if(status == "connect"){
              tcpServer = new WiFiServer(port != 0 ? port : 80);
              tcpServer->begin();
              Serial.println("i am TCP Server with port " + String(port != 0 ? port : 80));
              Serial.println("Server started");
            }
            if(status == "disconnect") {
              stopAndDeleteTCPServer(); // Dừng và xóa server cũ nếu có
            }
          }
          
          else if (role == "client") {
          if (status == "connect") {
            currentRole = "client";
            Serial.println("Client IP: " + ip);
            Serial.println("Client Port: " + port);
            // Thiết lập client TCP
            WiFiClient client;
            if (client.connect(ip.c_str(), port)) {
              Serial.println("Connected to server at " + ip + ":" + String(port));
              client.println("i am TCP client");
            } else {
              Serial.println("Connection failed");
            }
          } 
          else if (status == "disconnect") {
            WiFiClient client;
            client.stop();
            Serial.println("Disconnecting from server");
          }      
    }
  }
}
}



void setup()
{
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println(WiFi.localIP());
  tcpServer = new WiFiServer(8080);

  // ws.onEvent(onWebSocketEvent); nếu dùng WebSocket
  // server.addHandler(&ws); nếu dùng WebSocket

  // ------------------------------------------------------------------------gửi web cho client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });

  server.begin();
}



void loop()
{
  // put your main code here, to run repeatedly:
  // WiFiClient client = tcpServer.available();
  // if (client) {
  // }
  handleTCPclient();
  serialHandler(); 

}


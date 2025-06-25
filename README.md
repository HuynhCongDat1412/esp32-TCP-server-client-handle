# ESP32 TCP Server/Client Example

## Giới thiệu

Dự án này minh họa cách ESP32 có thể hoạt động **linh hoạt** ở cả hai vai trò:
- **TCP Server:** Nhận kết nối từ các client (ví dụ: Hercules TCP Client trên PC).
- **TCP Client:** Chủ động kết nối tới một TCP server khác (ví dụ: Hercules TCP Server trên PC).

Việc chuyển đổi vai trò, thiết lập kết nối, gửi/nhận dữ liệu đều được điều khiển qua lệnh gửi từ Serial (theo định dạng JSON).

---

## Cấu trúc chính của mã nguồn

### 1. **Khai báo và khởi tạo**

- **WiFi:** Kết nối vào mạng WiFi với SSID và password cấu hình sẵn.
- **tcpServer:** Con trỏ tới đối tượng `WiFiServer` (dùng khi ESP32 làm server).
- **tcpClient:** Đối tượng `WiFiClient` (dùng khi ESP32 làm client).
- **isTCPClientConnected:** Biến trạng thái kết nối client.
- **currentRole:** Lưu vai trò hiện tại (`"server"` hoặc `"client"`).

---

### 2. **Xử lý lệnh từ Serial (`serialHandler`)**

- Đọc lệnh từ Serial (theo định dạng JSON).
- Parse lệnh để lấy các trường: `role`, `status`, `ip`, `port`.
- Nếu `role` là `"server"`:
  - Nếu `status` là `"connect"`: Khởi tạo lại TCP server với port mới (nếu có).
  - Nếu `status` là `"disconnect"`: Dừng và xóa server.
- Nếu `role` là `"client"`:
  - Nếu `status` là `"connect"`: Kết nối tới server với IP/port chỉ định.
  - Nếu `status` là `"disconnect"`: Ngắt kết nối client.

---

### 3. **Xử lý khi ESP32 là TCP Server (`handleTCPclient`)**

- Kiểm tra có client nào kết nối tới ESP32 không.
- Khi có client:
  - Đọc từng dòng dữ liệu từ client (theo định dạng JSON).
  - Parse và kiểm tra các trường `role`, `status`, `data`.
  - Nếu client gửi yêu cầu ngắt kết nối (`status == "disconnect"`), server sẽ phản hồi và đóng kết nối.
  - Nếu có dữ liệu, server sẽ in ra Serial và phản hồi lại client.
  - Nếu không hợp lệ, gửi thông báo lỗi về client.

---

### 4. **Xử lý khi ESP32 là TCP Client (`handleTCPServerResponse`)**

- Nếu đang kết nối tới server:
  - Đọc dữ liệu từ server (theo từng dòng).
  - Parse và kiểm tra các trường `role`, `status`, `data`.
  - Nếu server yêu cầu ngắt kết nối (`status == "disconnect"`), client sẽ đóng kết nối.
  - Nếu có dữ liệu, in ra Serial.
  - Nếu không hợp lệ, in thông báo lỗi.

---

### 5. **Hàm tiện ích**

- **`parseClientJson`**: Parse chuỗi JSON thành đối tượng `StaticJsonDocument` để dễ dàng truy xuất các trường.
- **`stopAndDeleteTCPServer`**: Dừng và giải phóng bộ nhớ cho TCP server khi cần thiết.

---

### 6. **setup()**

- Kết nối WiFi.
- Khởi tạo TCP server mặc định ở port 8080.
- Khởi tạo web server để phục vụ giao diện web (không liên quan đến websocket trong scope này).

---

### 7. **loop()**

- Luôn gọi:
  - `handleTCPclient()` để xử lý khi là server.
  - `serialHandler()` để nhận lệnh chuyển vai trò/kết nối từ Serial.
  - `handleTCPServerResponse()` để xử lý khi là client.

---

## **Luồng hoạt động tổng quát**

1. **Khởi động:** ESP32 kết nối WiFi, khởi tạo TCP server mặc định.
2. **Nhận lệnh Serial:** Người dùng gửi lệnh JSON qua Serial để chuyển vai trò hoặc thiết lập kết nối.
3. **Xử lý TCP:**
   - Nếu là server: Chờ client kết nối, nhận/gửi dữ liệu.
   - Nếu là client: Chủ động kết nối tới server, nhận/gửi dữ liệu.
4. **Chuyển đổi vai trò linh hoạt:** Có thể chuyển đổi giữa server/client bất cứ lúc nào qua lệnh Serial.

---

## **Ví dụ lệnh Serial**

- **Chuyển sang TCP server (port 9000):**
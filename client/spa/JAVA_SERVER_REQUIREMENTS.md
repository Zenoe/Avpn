# Java UDP SPA 服务端对接规格 v1

本文档以当前 C++ 客户端实现为准。Java 服务端必须使用国密算法，并与以下字节级约定完全一致。

## 1. 固定配置

```text
UDP 监听端口: 16888
magic: ASCII "ASPA"
version: 1
套件: GM-SPA-SM2-ECDH-SM4GCM-SM3-V1
SM2 ID: 1be19cff-fd14-48fd-9a67-5d54b0442d58（UTF-8）
加密密钥 encryptionKeyId: 1
签名密钥 signingKeyId: 1
SM4-GCM nonce: 12 bytes
SM4-GCM tag: 16 bytes
时间允许偏差: 60000 ms
ticket 建议有效期: 60000 ms，单次使用
所有整数: unsigned、big-endian
所有字符串: UTF-8
```

客户端内置公钥：

```text
SM2 加密/ECDH 公钥:
04e3960abf643ac6c8f0fcb07544d724cf1159b04fcfb09c0c5aa4e7ec72bdfface3d1d85c0821cb9b806933fa85377e135d94c08c828abeb688f865000f9d465d

SM2 响应签名公钥:
04ad3c2e69f985138e5d7270f52a8164b10452fa70a226651603987f22323ba3ff9bf51be930fb0cf9a0f695112edd57d666f30b1b8025991fe4483656b37c4cd4
```

两者均为 65 字节、未压缩 SM2 公钥，格式为 `04 || X(32) || Y(32)`。服务端必须持有与其对应的私钥，私钥不得进入仓库、日志或响应。

## 2. 密钥协商与派生

客户端每次请求生成临时 SM2 密钥对。服务端使用静态加密私钥与客户端临时公钥执行 SM2 椭圆曲线点乘：

```text
sharedPoint  = SM2-ECDH(serverStaticPrivateKey, clientEphemeralPublicKey)
sharedSecret = sharedPoint.x(32-byte big-endian) || sharedPoint.y(32-byte big-endian)

requestKey  = first16(SM3(
    UTF8("ASPA-REQUEST-KEY-v1") || sharedSecret || requestId))

responseKey = first16(SM3(
    UTF8("ASPA-RESPONSE-KEY-v1") || sharedSecret || requestId))
```

`requestKey` 和 `responseKey` 均为 16 字节 SM4 密钥，禁止互换或复用。

## 3. 请求报文

```text
magic                     4 bytes   ASCII "ASPA"
version                   1 byte    0x01
messageType               1 byte    0x01
encryptionKeyId           2 bytes   0x0001
requestId                32 bytes   安全随机数
clientEphemeralPublicKey 65 bytes   04 || X || Y
gcmNonce                 12 bytes   安全随机数
ciphertextLength           2 bytes   含 16-byte GCM tag
ciphertext                 N bytes   JSON 密文 || tag
```

SM4-GCM 的 AAD 是从 `magic` 到 `ciphertextLength` 的完整请求前缀头。

请求密文 JSON：

```json
{
  "requestId": "base64url-32-bytes-no-padding",
  "clientTime": 1784236800000,
  "nonce": "base64url-32-random-bytes-no-padding",
  "requestedService": "login",
  "installationId": "stable-sm3-hex",
  "platform": "windows|macos|linux",
  "appVersion": "4.9.0.3",
  "authMode": "DISCOVERY_ONLY",
  "deviceId": "stable-sm3-hex"
}
```

校验要求：

- JSON `requestId` 解码后必须与头部 32 字节一致；
- `requestedService` 只接受 `login`；
- `clientTime` 与服务端当前时间差不得超过 60000 ms；
- 对 `requestId` 和 JSON `nonce` 做防重放；
- 同一 `requestId` 的 UDP 重试必须幂等，返回首次生成的相同响应；
- 未知 encryptionKeyId、非法长度、解密失败或 GCM tag 错误应静默丢弃。

## 4. 设备标识

客户端从 Qt `QSysInfo::machineUniqueId()` 获取平台稳定机器标识，规范化为去除首尾空白的小写字节，再计算：

```text
deviceId = lowerHex(SM3(
    UTF8("ASPA-DEVICE-v1") || 0x00 || UTF8(platform) || 0x00 || normalizedMachineId))

installationId = lowerHex(SM3(
    UTF8("ASPA-INSTALLATION-v1") || 0x00 || UTF8(platform) || 0x00 || normalizedMachineId))
```

机器标识不可用时客户端拒绝发包，不使用随机值降级。服务端可将 `deviceId` 用于设备绑定、准入、限流和审计，但不得记录原始机器标识。

## 5. 响应报文

```text
magic              4 bytes   ASCII "ASPA"
version            1 byte    0x01
messageType        1 byte    0x02
encryptionKeyId     2 bytes   加密/ECDH keyId，当前 0x0001
signingKeyId        2 bytes   响应签名 keyId，当前 0x0001
requestId          32 bytes
gcmNonce           12 bytes
ciphertextLength     2 bytes   含 16-byte GCM tag
ciphertext           N bytes   JSON 密文 || tag
signatureLength      2 bytes   DER 签名实际长度
signature            M bytes   SM2-with-SM3 DER 签名
```

SM4-GCM 的 AAD 是从 `magic` 到 `ciphertextLength` 的完整响应前缀头。

签名算法为 SM2-with-SM3，SM2 ID 使用本文固定值，签名输入严格为：

```text
responsePrefixHeader || ciphertext
```

签名格式必须是 ASN.1 DER，不是固定 64 字节的 `r || s`；`signatureLength` 必须写入实际 DER 长度。

客户端处理顺序：校验报文结构和 requestId，验证 SM2 签名，验证 SM4-GCM tag 并解密，最后验证 JSON、时间和端点策略。

## 6. 响应 JSON

成功响应：

```json
{
  "status": "OK",
  "requestId": "base64url-32-bytes-no-padding",
  "loginScheme": "https",
  "loginHost": "login-17.example.com",
  "loginPort": 18443,
  "spaTicket": "base64url-random-ticket-no-padding",
  "issuedAt": 1784236800000,
  "expiresAt": 1784236860000
}
```

业务拒绝响应：

```json
{
  "status": "REJECTED",
  "requestId": "base64url-32-bytes-no-padding",
  "errorCode": "RATE_LIMITED",
  "retryAfterMs": 10000,
  "issuedAt": 1784236800000
}
```

成功响应要求：

- `loginScheme` 必须为 `https`；
- `loginHost` 为服务端选择的域名或地址；
- `loginPort` 可为 1–65535 中的任意动态端口，包括 443；
- `expiresAt > issuedAt`；
- 客户端收到时剩余有效期至少 1000 ms；
- `issuedAt` 与客户端当前时间差不得超过 60000 ms。

## 7. Ticket 与动态端点

- ticket 至少使用 32 字节安全随机数并采用 Base64url 无 padding 编码；
- ticket 必须绑定返回的 `scheme + host + port`；
- ticket 默认 60 秒有效且只能原子消费一次；
- 登录请求通过 HTTP Header `X-SPA-Ticket` 携带 ticket；
- 443 和其他动态端口无特殊分支，均按返回值连接；
- 服务端不得接受客户端指定登录端口；
- 多实例部署时，防重放、幂等响应和 ticket 状态应放入共享存储。

## 8. Java 实现注意事项

- JCA/JCE 提供方必须明确支持 SM2、SM3、SM4-GCM；建议统一使用同一版本的 Bouncy Castle 国密实现并锁定版本；
- 不得使用 SM2 加密 `C1C3C2` 代替本文的 SM2-ECDH 点乘；当前协议不传输 SM2 `C1C3C2` 密文；
- Java `ByteBuffer` 默认 big-endian，与协议一致；
- EC 坐标必须补齐为固定 32 字节 big-endian，不能保留 `BigInteger.toByteArray()` 的符号前导字节；
- UDP 发送与接收按原始字节处理，不得把二进制报文转为字符串；
- 日志禁止输出私钥、sharedSecret、SM4 key、ticket、完整 deviceId 或完整报文。

## 9. 联调必测项

- 固定私钥与客户端内置公钥匹配；
- SM2-ECDH 得到相同的 64 字节 `x || y`；
- requestKey/responseKey 派生结果一致；
- 请求和响应的 SM4-GCM AAD、密文及 tag 互通；
- SM2 ID 和 DER 签名互通；
- 返回 443 与非 443 动态端口均可解析；
- GCM tag、签名、requestId、时间、长度任一被篡改时客户端拒绝；
- 重复 UDP 请求不产生多个 ticket；
- ticket 首次使用成功，重复或过期使用失败。

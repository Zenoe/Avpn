# Java UDP SPA 服务端需求规格 v1

## 1. 目标

实现一个独立的 Java UDP SPA（Single Packet Authorization）服务。客户端在用户登录前发送一个加密 UDP 报文；服务端校验成功后，选择一个当前可用的动态登录端点，并返回：

- HTTPS 登录域名或地址；
- 动态登录端口（可能为 443，也可能为其他端口）；
- 短期、单次使用的 `spaTicket`；
- 端点和 ticket 的过期时间。

客户端随后使用返回的 `host + port` 发起 HTTPS 登录，并在请求头中携带 `spaTicket`。登录网关必须校验 ticket 后才允许请求进入真正的登录服务。

本服务只负责 SPA、动态端点发现和临时访问授权，不负责校验用户名密码，也不签发用户登录 token。

## 2. 技术约束

- Java 17 或更高版本；
- UDP 服务建议使用 Netty，也可以使用 Java NIO；
- 不能为每个 UDP 报文创建一个线程；
- 单个请求和响应均不得超过 1200 字节，避免 UDP IP 分片；
- 所有多字节整数使用网络字节序（big-endian）；
- 所有服务端时间使用 UTC Unix epoch milliseconds；
- 服务应支持 Windows/Linux 开发测试，生产环境目标为 Linux；
- 多实例部署时，防重放、ticket 和幂等数据必须存储在共享 Redis 中；
- 日志中禁止记录完整报文、密钥、ticket、用户凭据或完整 installationId。

## 3. 安全边界

SPA 发生在用户登录之前，因此服务端此时没有用户身份。仅使用服务端公钥加密，可以证明响应来自服务端并隐藏报文内容，但不能证明请求方是某个合法用户，因为服务端公钥本来就是公开的。

服务端必须支持以下两种客户端授权模式：

1. `DISCOVERY_ONLY`：只校验加密完整性、时间戳、nonce、防重放和限流。该模式提供隐藏登录入口和抗扫描能力，但不提供强客户端身份认证。
2. `DEVICE_CREDENTIAL`：请求携带已预置或已注册设备凭据生成的 `authProof`。服务端校验成功后才签发 ticket。该模式用于后续具备设备注册条件的生产环境。

禁止把所有安装包共享的固定 HMAC 密钥当作强身份凭据；桌面安装包中的共享密钥可以被提取。若暂时使用共享密钥，只能把它视为额外的滥用门槛，并且必须支持密钥轮换。

`DEVICE_CREDENTIAL` 模式下，服务端通过 `credentialId` 查找已经登记的设备 Ed25519 公钥。`authProof` 是设备私钥对以下确定性字节串的 Ed25519 签名：

```text
UTF8("ASPA-AUTH-v1")
|| requestId(raw 32 bytes)
|| clientTime(int64 big-endian)
|| nonce(raw bytes)
|| UTF8(requestedService)
|| UTF8(installationId)
```

设备私钥必须在 SPA 之前通过独立的设备注册或预置流程得到。如果当前系统还没有该流程，首版使用 `DISCOVERY_ONLY`，不能伪造一个“安全的固定客户端密钥”。

## 4. 密码算法

优先使用 JDK 17 原生支持的标准算法：

- 密钥交换：X25519；
- 密钥派生：HKDF-SHA-256；
- 对称加密：AES-256-GCM；
- 响应签名：Ed25519；
- 请求 ID：32 字节随机数；
- AES-GCM nonce：12 字节安全随机数；
- ticket：至少 32 字节安全随机数，使用 Base64 URL-safe、无 padding 编码；
- ticket 存储：Redis 中只保存 `SHA-256(ticket)`，不保存 ticket 明文。

服务端配置两类长期密钥：

- X25519 私钥：用于解密客户端请求和派生响应密钥；
- Ed25519 私钥：用于签名响应。

客户端内置相应的 X25519 和 Ed25519 公钥。协议必须支持 `keyId`，以便灰度轮换密钥。

请求密钥派生：

```text
sharedSecret = X25519(serverStaticPrivateKey, clientEphemeralPublicKey)
requestKey   = HKDF-SHA256(
    ikm  = sharedSecret,
    salt = requestId,
    info = "amnezia-spa-request-v1",
    len  = 32)
```

响应使用独立密钥，禁止复用请求密钥：

```text
responseKey = HKDF-SHA256(
    ikm  = sharedSecret,
    salt = requestId,
    info = "amnezia-spa-response-v1",
    len  = 32)
```

AES-GCM 的 AAD 为完整明文报文头（不包含密文本身和签名）。

## 5. UDP 二进制报文格式

### 5.1 公共请求头

```text
magic                    4 bytes   ASCII "ASPA"
version                  1 byte    0x01
messageType              1 byte    0x01 = REQUEST
keyId                    2 bytes   unsigned
requestId               32 bytes
clientEphemeralPublicKey 32 bytes  X25519 raw public key
gcmNonce                 12 bytes
ciphertextLength          2 bytes   unsigned
ciphertext                N bytes   AES-256-GCM ciphertext including 16-byte tag
```

服务端必须先校验固定头、版本和长度，再进行任何昂贵的密码运算。错误 magic、未知版本、未知 keyId、非法长度或超过 1200 字节的报文直接静默丢弃，不返回错误，避免形成 UDP 放大器和协议探测 oracle。

### 5.2 请求密文 JSON

```json
{
  "requestId": "base64url-32-bytes",
  "clientTime": 1784236800000,
  "nonce": "base64url-32-random-bytes",
  "requestedService": "login",
  "installationId": "sha256-or-random-installation-id",
  "platform": "windows|macos|linux",
  "appVersion": "4.9.0.3",
  "authMode": "DISCOVERY_ONLY|DEVICE_CREDENTIAL",
  "credentialId": "optional-key-id",
  "authProof": "optional-base64url-proof"
}
```

要求：

- JSON 中的 `requestId` 必须与头部 requestId 一致；
- `requestedService` v1 只接受 `login`；
- `clientTime` 与服务端时间差默认不得超过 60 秒，允许通过配置调整；
- `nonce` 必须至少 16 字节，推荐 32 字节；
- Redis 中按 `requestId` 和 `nonce` 做防重放；
- `installationId` 只用于限流和审计，不等同于可信设备身份；
- `DEVICE_CREDENTIAL` 模式下必须校验 `credentialId + authProof`；
- 用户名和密码绝不能出现在 SPA 请求中。

### 5.3 公共响应头

```text
magic             4 bytes   ASCII "ASPA"
version           1 byte    0x01
messageType       1 byte    0x02 = RESPONSE
keyId             2 bytes   unsigned
requestId        32 bytes
gcmNonce         12 bytes
ciphertextLength   2 bytes   unsigned
ciphertext         N bytes   AES-256-GCM ciphertext including 16-byte tag
signatureLength    2 bytes   v1 固定为 64
signature         64 bytes   Ed25519 signature
```

响应前缀头指从 `magic` 到 `ciphertextLength` 的所有字节。AES-GCM AAD 使用该前缀头，签名输入为：

```text
responsePrefixHeader || ciphertext
```

响应使用与请求相同的 `requestId`。客户端只有在 AES-GCM 校验、Ed25519 签名、requestId、有效期和端点策略全部通过后，才接受动态登录端点。

### 5.4 响应密文 JSON

成功响应：

```json
{
  "status": "OK",
  "requestId": "base64url-32-bytes",
  "loginScheme": "https",
  "loginHost": "login-17.example.com",
  "loginPort": 18443,
  "spaTicket": "base64url-random-ticket",
  "issuedAt": 1784236800000,
  "expiresAt": 1784236860000
}
```

已成功解密但业务拒绝的响应：

```json
{
  "status": "REJECTED",
  "requestId": "base64url-32-bytes",
  "errorCode": "RATE_LIMITED",
  "retryAfterMs": 10000,
  "issuedAt": 1784236800000
}
```

支持的错误码至少包括：

- `CLOCK_SKEW`；
- `REPLAY_DETECTED`；
- `UNAUTHORIZED_DEVICE`；
- `RATE_LIMITED`；
- `NO_LOGIN_ENDPOINT`；
- `UNSUPPORTED_CLIENT_VERSION`；
- `INTERNAL_ERROR`。

只有已经通过 GCM 解密和基本格式校验的请求才允许收到加密错误响应。无法认证的报文静默丢弃。

## 6. 动态登录端点选择

定义接口：

```java
public interface LoginEndpointSelector {
    Optional<LoginEndpoint> select(SpaRequestContext context);
}
```

`LoginEndpoint` 至少包含：

```java
record LoginEndpoint(String scheme, String host, int port, String endpointId) {}
```

要求：

- `scheme` v1 固定为 `https`；
- `host` 优先使用有有效 TLS 证书的域名，不推荐返回裸 IP；
- `port` 支持 1–65535，但生产环境应配置允许范围；
- 443 与其他动态端口走完全相同的协议；
- 只选择健康检查通过且有剩余容量的登录节点；
- ticket 必须绑定 `endpointId + host + port`，不能用于其他端点；
- 如果没有可用节点，返回 `NO_LOGIN_ENDPOINT`；
- 服务端不得接受客户端指定登录地址或登录端口。

## 7. SPA ticket

成功 SPA 后创建一次性 ticket，推荐有效期 60 秒，可配置为 30–120 秒。

Redis 数据示例：

```text
key: spa:ticket:{sha256(ticket)}
value:
  requestId
  installationIdHash
  sourceIp
  endpointId
  loginHost
  loginPort
  issuedAt
  expiresAt
  state = UNUSED
TTL: expiresAt - now
```

登录网关收到请求：

```http
POST /v1/auth/login
X-SPA-Ticket: <ticket>
Content-Type: application/json
```

登录网关必须原子执行：

1. 计算 ticket 的 SHA-256；
2. 查询 Redis；
3. 校验未过期、状态为 `UNUSED`、host/port/endpoint 匹配；
4. 根据配置校验源 IP；
5. 原子标记为 `USED` 或直接删除；
6. 通过后才转发用户名密码登录请求。

必须使用 Redis Lua 脚本、事务或等价原子操作，防止同一个 ticket 并发使用两次。

源 IP 绑定应可配置：

- 桌面稳定网络可启用严格 IP 绑定；
- NAT、代理或网络切换场景可关闭严格绑定；
- 即使绑定 IP，ticket 仍必须单次、短期有效。

## 8. UDP 重试与幂等

客户端会在超时后重发完全相同的 UDP 报文，默认最多发送 3 次。因此服务端必须对同一 `requestId` 幂等。

Redis 幂等数据：

```text
key: spa:request:{requestId}
value: 已生成的完整加密响应字节
TTL: 120 seconds
```

重复请求处理：

- requestId、请求摘要和来源符合预期：直接返回第一次生成的相同响应；
- requestId 相同但请求摘要不同：视为冲突或攻击，静默丢弃；
- 不得为每次 UDP 重试生成新的 ticket；
- 对同一请求重复返回相同响应不会延长 ticket 有效期。

## 9. 限流和抗 UDP 放大

至少实现以下限制：

- 单 IP 每秒请求数；
- 单 IP 每分钟请求数；
- 单 installationId 每分钟请求数；
- 全局并发密码运算上限；
- 单个 UDP 报文最大 1200 字节；
- 响应不得明显大于请求；
- 无法解密、未知 keyId、错误 authProof 或错误格式的请求不返回任何数据；
- Redis 或端点选择器不可用时快速失败，不允许请求无限堆积。

服务应记录不含敏感内容的指标：接收数、格式拒绝数、解密失败数、重放数、限流数、成功数、无端点数、处理时延和 Redis 错误数。

## 10. 推荐模块结构

```text
spa-server/
  src/main/java/.../spa/
    SpaServerApplication.java
    config/
      SpaServerProperties.java
      CryptoKeyProperties.java
    transport/
      UdpSpaServer.java
      SpaPacketDecoder.java
      SpaPacketEncoder.java
    crypto/
      SpaCryptoService.java
      HkdfSha256.java
      KeyRegistry.java
    protocol/
      SpaRequest.java
      SpaResponse.java
      PacketHeader.java
      ErrorCode.java
    security/
      ReplayGuard.java
      ClientAuthorizer.java
      RateLimiter.java
    endpoint/
      LoginEndpoint.java
      LoginEndpointSelector.java
      HealthAwareEndpointSelector.java
    ticket/
      SpaTicketService.java
      RedisSpaTicketService.java
    login/
      SpaTicketValidationFilter.java
    observability/
      SpaMetrics.java
```

协议解析、密码处理、端点选择、ticket 存储和 UDP 传输必须通过接口解耦，便于单元测试和替换实现。

## 11. 配置项

至少提供：

```yaml
spa:
  udp-host: 0.0.0.0
  udp-port: 32000
  max-packet-bytes: 1200
  allowed-clock-skew-ms: 60000
  ticket-ttl-ms: 60000
  request-cache-ttl-ms: 120000
  auth-mode: DISCOVERY_ONLY
  bind-ticket-to-source-ip: true
  minimum-client-version: "4.9.0"
  allowed-login-port-min: 10000
  allowed-login-port-max: 65535
  keys:
    active-key-id: 1
    x25519-private-key: "${SPA_X25519_PRIVATE_KEY}"
    ed25519-private-key: "${SPA_ED25519_PRIVATE_KEY}"
  redis:
    uri: "${SPA_REDIS_URI}"
```

私钥、Redis 密码和设备凭据只能从密钥管理系统或环境注入，不能提交到源码仓库或配置样例。

## 12. 与 C++ 客户端的对应关系

服务端成功响应最终映射为：

```cpp
spa::LoginEndpoint endpoint;
endpoint.scheme = loginScheme;
endpoint.host = loginHost;
endpoint.port = loginPort;
endpoint.ticket = decodedTicketBytes;
endpoint.expiresAtUtc = decodedExpiresAt;
```

C++ 侧 `spa::Codec` 的 Java 协议实现负责：

- 生成 X25519 临时密钥；
- 构造请求报文；
- 派生 request/response key；
- AES-GCM 加解密；
- 验证 Ed25519 响应签名；
- 验证 requestId；
- 将成功 JSON 转成 `spa::LoginEndpoint`；
- 对不属于当前请求的 UDP 数据返回 `Ignore`；
- 对已认证但业务失败的响应返回 `Rejected`。

## 13. 必须提供的测试

单元测试至少覆盖：

- 正常请求得到动态端点和 ticket；
- 返回 443 和返回非 443 端口；
- 错误 magic、版本、keyId 和长度；
- GCM tag 错误；
- 请求时间过期和未来时间；
- requestId/nonce 重放；
- 同一请求重试返回完全相同响应；
- 同 requestId 不同请求内容被拒绝；
- 无健康登录节点；
- ticket 过期；
- ticket 被使用两次；
- ticket 用于错误 host/port；
- Redis 超时或不可用；
- 单 IP 和单 installationId 限流；
- 报文大小边界和随机畸形报文 fuzz 测试。

集成测试至少包含：

1. C++ 客户端生成 UDP 请求；
2. Java 服务端成功解密并返回动态端点；
3. C++ 客户端验证和解密响应；
4. 客户端使用 `host + dynamicPort + spaTicket` 访问 HTTPS 登录接口；
5. 首次访问通过，重复使用 ticket 被拒绝；
6. SPA 过期后登录被拒绝。

## 14. 完成标准

- Java 服务能在配置端口接收 UDP SPA 请求；
- 所有无效或无法认证的 UDP 报文静默丢弃；
- 有效请求在正常负载下快速返回签名加密响应；
- 返回端口不写死，443 和动态端口均可工作；
- UDP 重试不会产生多个 ticket；
- ticket 短期、单次使用并绑定返回端点；
- 未携带有效 ticket 的登录请求不能进入登录服务；
- Java 和 C++ 使用固定协议测试向量验证完全兼容；
- 私钥和 ticket 不出现在日志、异常栈或持久化明文中；
- 核心协议、密码学、Redis 原子消费和端点选择均有自动化测试。

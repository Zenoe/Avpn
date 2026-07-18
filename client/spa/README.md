# SPA client module

`AmneziaSpa` is a standalone Qt 6/C++17 UDP discovery module. It depends only on
`Qt6::Core` and `Qt6::Network`; it does not include or call application controllers,
repositories, QML, VPN code, or platform-specific APIs.

## Responsibilities

- send one encoded SPA request to a configured UDP gateway;
- retry the exact same datagram using a bounded timeout policy;
- pass responses to an injected protocol codec;
- enforce basic policy on the returned dynamic login host, port, scheme, and expiry;
- emit a validated `spa::LoginEndpoint` containing the one-time login ticket.

The module deliberately does not provide a plaintext JSON codec. `spa::Codec` is the
security boundary and must be implemented after the Java server protocol is fixed.
Its production implementation must encrypt/authenticate requests, verify response
authenticity, correlate the response request ID/nonce, and reject replays before it
returns `DecodeDisposition::Accepted`.

## Build and consumption

The desktop build creates the static target `AmneziaSpa` and alias `Amnezia::Spa`.
The application does not link or instantiate it yet. A future authentication adapter
can consume it explicitly:

```cmake
target_link_libraries(AuthenticationAdapter PRIVATE Amnezia::Spa)
```

```cpp
auto codec = std::make_shared<JavaSpaCodec>(serverPublicKey);
auto client = new spa::Client(codec, parent);

spa::ClientConfig config;
config.gatewayHost = QStringLiteral("spa.example.com");
config.gatewayPort = 32000;
config.endpointPolicy.allowedSchemes = { QStringLiteral("https") };
config.endpointPolicy.allowedHostSuffixes = { QStringLiteral("example.com") };
config.endpointPolicy.minimumPort = 10000;
config.endpointPolicy.maximumPort = 65535;
client->setConfig(config);
```

The returned port is never fixed by the module. Port `443` and arbitrary server-approved
dynamic ports follow the same path.

The matching Java server contract is documented in `JAVA_SERVER_REQUIREMENTS.md`.

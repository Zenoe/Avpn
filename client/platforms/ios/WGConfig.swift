import Foundation

struct WGConfig: Decodable {
  let dns1: String
  let dns2: String
  let mtu: String
  let hostName: String
  let port: Int
  let clientIP: String
  let clientPrivateKey: String
  let serverPublicKey: String
  let presharedKey: String?
  var allowedIPs: [String]
  var persistentKeepAlive: String
  let splitTunnelType: Int
  let splitTunnelSites: [String]

  enum CodingKeys: String, CodingKey {
    case dns1
    case dns2
    case mtu
    case hostName
    case port
    case clientIP = "client_ip"
    case clientPrivateKey = "client_priv_key"
    case serverPublicKey = "server_pub_key"
    case presharedKey = "psk_key"
    case allowedIPs = "allowed_ips"
    case persistentKeepAlive = "persistent_keep_alive"
    case splitTunnelType
    case splitTunnelSites
  }

  var str: String {
    """
    [Interface]
    Address = \(clientIP)
    DNS = \(dns1), \(dns2)
    MTU = \(mtu)
    PrivateKey = \(clientPrivateKey)
    [Peer]
    PublicKey = \(serverPublicKey)
    \(presharedKey == nil ? "" : "PresharedKey = \(presharedKey!)")
    AllowedIPs = \(allowedIPs.joined(separator: ", "))
    Endpoint = \(hostName):\(port)
    PersistentKeepalive = \(persistentKeepAlive)
    """
  }

  var redux: String {
    """
    [Interface]
    Address = \(clientIP)
    DNS = \(dns1), \(dns2)
    MTU = \(mtu)
    PrivateKey = ***
    [Peer]
    PublicKey = ***
    PresharedKey = ***
    AllowedIPs = \(allowedIPs.joined(separator: ", "))
    Endpoint = \(hostName):\(port)
    PersistentKeepalive = \(persistentKeepAlive)

    SplitTunnelType = \(splitTunnelType)
    SplitTunnelSites = \(splitTunnelSites.joined(separator: ", "))
    """
  }
}

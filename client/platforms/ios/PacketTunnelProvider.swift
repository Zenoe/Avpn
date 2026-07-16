import Foundation
import NetworkExtension
import os

struct Constants {
  static let kActivationAttemptId = "activationAttemptId"
  static let wireGuardConfigKey = "wireguard"
  static let kActionStatus = "status"
  static let kMessageKeyAction = "action"
}

class PacketTunnelProvider: NEPacketTunnelProvider {
  var wgAdapter: WireGuardAdapter?

  override func handleAppMessage(_ messageData: Data,
                                 completionHandler: ((Data?) -> Void)? = nil) {
    if messageData.count == 1 && messageData[0] == 0 {
      handleWireguardAppMessage(messageData, completionHandler: completionHandler)
      return
    }

    guard let message = try? JSONSerialization.jsonObject(with: messageData) as? [String: Any],
          let action = message[Constants.kMessageKeyAction] as? String else {
      handleWireguardAppMessage(messageData, completionHandler: completionHandler)
      return
    }

    if action == Constants.kActionStatus {
      handleWireguardStatusMessage(messageData, completionHandler: completionHandler)
    } else {
      completionHandler?(nil)
    }
  }

  override func startTunnel(options: [String: NSObject]? = nil,
                            completionHandler: @escaping (Error?) -> Void) {
    let activationAttemptId = options?[Constants.kActivationAttemptId] as? String
    let errorNotifier = ErrorNotifier(activationAttemptId: activationAttemptId)
    startWireguard(activationAttemptId: activationAttemptId,
                   errorNotifier: errorNotifier,
                   completionHandler: completionHandler)
  }

  override func stopTunnel(with reason: NEProviderStopReason,
                           completionHandler: @escaping () -> Void) {
    stopWireguard(with: reason, completionHandler: completionHandler)
  }
}

extension WireGuardLogLevel {
  var osLogLevel: OSLogType {
    switch self {
    case .verbose: return .debug
    case .error: return .error
    }
  }
}

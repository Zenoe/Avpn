package org.caelispect.vpn

import org.caelispect.vpn.protocol.Protocol
import org.caelispect.vpn.protocol.wireguard.Wireguard

enum class VpnProto(
    val label: String,
    val processName: String,
    val serviceClass: Class<out CaelispectVpnService>
) {
    WIREGUARD(
        "WireGuard",
        "org.caelispect.vpn:wireguardService",
        WireguardService::class.java
    ) {
        override fun createProtocol(): Protocol = Wireguard()
    };

    private var _protocol: Protocol? = null
    val protocol: Protocol
        get() {
            if (_protocol == null) _protocol = createProtocol()
            return _protocol ?: throw AssertionError("Set to null by another thread")
        }

    protected abstract fun createProtocol(): Protocol

    companion object {
        fun get(protocolName: String): VpnProto = VpnProto.valueOf(protocolName.uppercase())
    }
}

#ifndef CONFIGKEYS_H
#define CONFIGKEYS_H

#include <QLatin1String>

namespace caelispect
{
    namespace configKey
    {
        constexpr QLatin1String hostName("hostName");
        constexpr QLatin1String userName("userName");
        constexpr QLatin1String password("password");
        constexpr QLatin1String port("port");
        constexpr QLatin1String localPort("local_port");

        constexpr QLatin1String dns1("dns1");
        constexpr QLatin1String dns2("dns2");

        constexpr QLatin1String serverIndex("serverIndex");
        constexpr QLatin1String description("description");
        constexpr QLatin1String displayName("displayName");
        constexpr QLatin1String name("name");
        constexpr QLatin1String cert("cert");
        constexpr QLatin1String accessToken("api_key");
        constexpr QLatin1String config("config");
        constexpr QLatin1String configVersion("config_version");


        constexpr QLatin1String vpnProto("protocol");
        constexpr QLatin1String protocol("protocol");
        constexpr QLatin1String protocols("protocols");

        constexpr QLatin1String remote("remote");
        constexpr QLatin1String transportProto("transport_proto");
        constexpr QLatin1String cipher("cipher");
        constexpr QLatin1String hash("hash");
        constexpr QLatin1String ncpDisable("ncp_disable");
        constexpr QLatin1String tlsAuth("tls_auth");

        constexpr QLatin1String clientPrivKey("client_priv_key");
        constexpr QLatin1String clientPubKey("client_pub_key");
        constexpr QLatin1String serverPrivKey("server_priv_key");
        constexpr QLatin1String serverPubKey("server_pub_key");
        constexpr QLatin1String pskKey("psk_key");
        constexpr QLatin1String mtu("mtu");
        constexpr QLatin1String allowedIps("allowed_ips");
        constexpr QLatin1String persistentKeepAlive("persistent_keep_alive");

        constexpr QLatin1String clientIp("client_ip");

        constexpr QLatin1String site("site");
        constexpr QLatin1String blockOutsideDns("block_outside_dns");

        constexpr QLatin1String subnetAddress("subnet_address");
        constexpr QLatin1String subnetMask("subnet_mask");
        constexpr QLatin1String subnetCidr("subnet_cidr");

        constexpr QLatin1String additionalClientConfig("additional_client_config");
        constexpr QLatin1String additionalServerConfig("additional_server_config");

        constexpr QLatin1String wireguard("wireguard");

        constexpr QLatin1String splitTunnelSites("splitTunnelSites");
        constexpr QLatin1String splitTunnelType("splitTunnelType");

        constexpr QLatin1String splitTunnelApps("splitTunnelApps");
        constexpr QLatin1String appSplitTunnelType("appSplitTunnelType");

        constexpr QLatin1String allowedDnsServers("allowedDnsServers");

        constexpr QLatin1String killSwitchOption("killSwitchOption");

        constexpr QLatin1String crc("crc");

        constexpr QLatin1String clientId("clientId");

        constexpr QLatin1String nameOverriddenByUser("nameOverriddenByUser");

        constexpr QLatin1String clientName("clientName");
        constexpr QLatin1String userData("userData");
        constexpr QLatin1String creationDate("creationDate");
        constexpr QLatin1String latestHandshake("latestHandshake");
        constexpr QLatin1String dataReceived("dataReceived");
        constexpr QLatin1String dataSent("dataSent");

        constexpr QLatin1String storageServerId("storageServerId");

    }
}

#endif

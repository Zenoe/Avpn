#ifndef PROTOCOLCONSTANTS_H
#define PROTOCOLCONSTANTS_H

namespace caelispect
{

    namespace protocols
    {

        namespace dns
        {
            constexpr char caelispectDnsIp[] = "172.29.172.254";
        }





        namespace wireguard
        {
            // Config file keys ([Interface] / [Peer] sections) - case-sensitive
            constexpr char PrivateKey[] = "PrivateKey";
            constexpr char Address[] = "Address";
            constexpr char PublicKey[] = "PublicKey";
            constexpr char PresharedKey[] = "PresharedKey";
            constexpr char PreSharedKey[] = "PreSharedKey";
            constexpr char AllowedIPs[] = "AllowedIPs";
            constexpr char Endpoint[] = "Endpoint";
            constexpr char PersistentKeepalive[] = "PersistentKeepalive";
            constexpr char MTU[] = "MTU";

            constexpr char defaultSubnetAddress[] = "10.8.1.0";
            constexpr char defaultSubnetMask[] = "255.255.255.0";
            constexpr char defaultSubnetCidr[] = "24";

            constexpr char defaultPort[] = "51820";

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS) || defined(MACOS_NE)
            constexpr char defaultMtu[] = "1280";
#else
            constexpr char defaultMtu[] = "1376";
#endif
            constexpr char serverConfigPath[] = "/opt/caelispect/wireguard/wg0.conf";
            constexpr char serverPublicKeyPath[] = "/opt/caelispect/wireguard/wireguard_server_public_key.key";
            constexpr char serverPskKeyPath[] = "/opt/caelispect/wireguard/wireguard_psk.key";

        }

        namespace sftp
        {
            constexpr char defaultUserName[] = "sftp_user";

        } // namespace sftp

        namespace socks5Proxy
        {
            constexpr char defaultUserName[] = "proxy_user";
            constexpr char defaultPort[] = "38080";

            constexpr char proxyConfigPath[] = "/usr/local/3proxy/conf/3proxy.cfg";
        }

        namespace mtProxy
        {
            constexpr char secretKey[]            = "mtproxy_secret";
            constexpr char tagKey[]               = "mtproxy_tag";
            constexpr char tgLinkKey[]            = "mtproxy_tg_link";
            constexpr char tmeLinkKey[]           = "mtproxy_tme_link";
            constexpr char isEnabledKey[]         = "mtproxy_is_enabled";
            constexpr char publicHostKey[]        = "mtproxy_public_host";
            constexpr char transportModeKey[]     = "mtproxy_transport_mode";
            constexpr char tlsDomainKey[]         = "mtproxy_tls_domain";
            constexpr char additionalSecretsKey[] = "mtproxy_additional_secrets";
            constexpr char workersKey[]           = "mtproxy_workers";
            constexpr char workersModeKey[]       = "mtproxy_workers_mode";
            constexpr char natEnabledKey[]        = "mtproxy_nat_enabled";
            constexpr char natInternalIpKey[]     = "mtproxy_nat_internal_ip";
            constexpr char natExternalIpKey[]     = "mtproxy_nat_external_ip";

            constexpr char transportModeStandard[] = "standard";
            constexpr char transportModeFakeTLS[]  = "faketls";

            constexpr char workersModeAuto[]       = "auto";
            constexpr char workersModeManual[]     = "manual";

            constexpr char defaultPort[]           = "443";
            constexpr char defaultWorkers[]        = "2";
            constexpr int  maxWorkers              = 32;
            constexpr int  botTagHexLength         = 32;
            constexpr char defaultTlsDomain[]      = "googletagmanager.com";
        }

        namespace telemt
        {
            constexpr char secretKey[]            = "telemt_secret";
            constexpr char tagKey[]               = "telemt_tag";
            constexpr char tgLinkKey[]            = "telemt_tg_link";
            constexpr char tmeLinkKey[]           = "telemt_tme_link";
            constexpr char isEnabledKey[]         = "telemt_is_enabled";
            constexpr char publicHostKey[]        = "telemt_public_host";
            constexpr char transportModeKey[]     = "telemt_transport_mode";
            constexpr char tlsDomainKey[]         = "telemt_tls_domain";
            constexpr char maskEnabledKey[]       = "telemt_mask_enabled";
            constexpr char tlsEmulationKey[]      = "telemt_tls_emulation";
            constexpr char useMiddleProxyKey[]    = "telemt_use_middle_proxy";
            constexpr char userNameKey[]          = "telemt_user_name";
            // Stored for UI only (Telemt server ignores these; same controls as MTProxy page)
            constexpr char additionalSecretsKey[] = "telemt_additional_secrets";
            constexpr char workersKey[]           = "telemt_workers";
            constexpr char workersModeKey[]       = "telemt_workers_mode";
            constexpr char natEnabledKey[]        = "telemt_nat_enabled";
            constexpr char natInternalIpKey[]     = "telemt_nat_internal_ip";
            constexpr char natExternalIpKey[]     = "telemt_nat_external_ip";

            constexpr char transportModeStandard[] = "standard";
            constexpr char transportModeFakeTLS[]  = "faketls";

            constexpr char defaultPort[]           = "443";
            constexpr char defaultTlsDomain[]      = "googletagmanager.com";
            constexpr char defaultUserName[]       = "caelispect";
            constexpr char defaultWorkers[]        = "2";
            constexpr char workersModeAuto[]       = "auto";
            constexpr char workersModeManual[]     = "manual";
            constexpr int  maxWorkers              = 32;
            constexpr int  botTagHexLength         = 32;
        }

    } // namespace protocols
}

#endif // PROTOCOLCONSTANTS_H

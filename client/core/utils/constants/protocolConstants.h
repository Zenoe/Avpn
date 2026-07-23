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

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS) || defined(MACOS_NE)
            constexpr char defaultMtu[] = "1280";
#else
            constexpr char defaultMtu[] = "1376";
#endif
        }

    } // namespace protocols
}

#endif // PROTOCOLCONSTANTS_H

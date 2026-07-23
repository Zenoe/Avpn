#ifndef APIKEYS_H
#define APIKEYS_H

#include <QLatin1String>

namespace apiDefs
{
    namespace key
    {
        constexpr QLatin1String apiEndpoint("api_endpoint");
        constexpr QLatin1String apiKey("api_key");
        constexpr QLatin1String protocol("protocol");
        constexpr QLatin1String apiConfig("api_config");
        constexpr QLatin1String serviceType("service_type");
        constexpr QLatin1String serviceInfo("service_info");
        constexpr QLatin1String serviceProtocol("service_protocol");
        constexpr QLatin1String vpnKey("vpn_key");
        constexpr QLatin1String stackType("stack_type");
        constexpr QLatin1String cliVersion("cli_version");
        constexpr QLatin1String cliName("cli_name");
        constexpr QLatin1String availableCountries("available_countries");
        constexpr QLatin1String availableProtocols("available_protocols");
        constexpr QLatin1String installationUuid("installation_uuid");
        constexpr QLatin1String uuid("installation_uuid");
        constexpr QLatin1String osVersion("os_version");
        constexpr QLatin1String userCountryCode("user_country_code");
        constexpr QLatin1String serverCountryCode("server_country_code");
        constexpr QLatin1String serverCountryName("server_country_name");
        constexpr QLatin1String appVersion("app_version");
        constexpr QLatin1String authData("auth_data");

        constexpr QLatin1String aesKey("aes_key");
        constexpr QLatin1String aesIv("aes_iv");
        constexpr QLatin1String aesSalt("aes_salt");
        constexpr QLatin1String apiPayload("api_payload");
        constexpr QLatin1String keyPayload("key_payload");

        constexpr QLatin1String services("services");
        constexpr QLatin1String workerLastUpdated("worker_last_updated");
        constexpr QLatin1String lastDownloaded("last_downloaded");
        constexpr QLatin1String sourceType("source_type");
        constexpr QLatin1String appLanguage("app_language");

        constexpr QLatin1String supportInfo("support_info");
        constexpr QLatin1String email("email");
        constexpr QLatin1String website("website");
        constexpr QLatin1String websiteName("website_name");
        constexpr QLatin1String telegram("telegram");

        constexpr QLatin1String id("id");
        constexpr QLatin1String config("config");

        constexpr QLatin1String configs("configs");

        constexpr QLatin1String publicKeyInfo("public_key");
        constexpr QLatin1String publicKey("public_key");
        constexpr QLatin1String expiresAt("expires_at");
        constexpr QLatin1String isConnectEvent("is_connect_event");
        constexpr QLatin1String certificate("certificate");
    } // namespace key
} // namespace apiDefs

#endif // APIKEYS_H

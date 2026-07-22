rem Remove transient Caelispect tunnel and split-tunnel drivers.
sc stop CaelispectWGTunnel$Caelispect
sc delete CaelispectWGTunnel$Caelispect
sc stop Caelispect-service
taskkill /IM "Caelispect-service.exe" /F
taskkill /IM "Caelispect.exe" /F
sc stop CaelispectSplitTunnel
sc delete CaelispectSplitTunnel
exit /b 0

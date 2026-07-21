sc stop CaelispectWGTunnel$Caelispect
sc delete CaelispectWGTunnel$Caelispect
taskkill /IM "Caelispect-service.exe" /F
taskkill /IM "Caelispect.exe" /F
exit /b 0

# w25q-spi-driver
W25Qxx SPI NOR flash client driver for Raspberry Pi 3 (Linux 4.14)

## Device Tree Overlay
使用device tree compiler將 overlay `w25q-overlay.dts`編譯成`w25q.dtbo`：
```
dtc -@ -I dts -O dtb -o w25q.dtbo w25q-overlay.dts
```
將dtbo複製到overlays目錄
```
sudo cp w25q.dtbo /boot/overlays/
```
在/boot/config.txt新增:
```
dtoverlay=w25q
```
重新開機後生效
```
sudo reboot
```
## match device via of_match_table
driver透過`of_match_table`比對device tree節點的compatible
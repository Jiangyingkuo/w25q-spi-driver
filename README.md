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

## Read JEDEC ID
參考華邦 W25Q64 datasheet 8.2.29 Read JEDEC ID(9Fh)：送出command`0x9F` 後，晶片回傳3個byte

在probe中以`spi_sync()`實作，一個`spi_message`包含兩段`spi_transfer`：
1. 送出1 byte command`0x9F`
2. 接收3 bytes回傳值

>[!NOTE]
>CS(chip select)是master用來選擇要跟哪一顆device通訊的訊號線，拉低表示選中，拉高表示通訊結束。
>SPI core是以message為單位控制CS的，當message開始時拉低，整個傳完才拉高。
>所以把送指令和讀資料放在同一個message裡，CS中間才不會被放開，W25Q才會把它們當成同一個指令。

>[!WARNING]
>目前狀態：尚需在硬體上驗證（模組排針焊點需重焊）

##references
W25Q64 datasheet
https://docs.rs-online.com/9bfc/0900766b81704060.pdf

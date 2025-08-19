Development Repo for video player using
STM32F411 with sd card, and ST7735 display

Hardware
Using STM32F411 (Blackpill development board)
Generic ST7735 1.8inch LCD Display from amazon
Micro SD  breakout - Uses SDIO so those need to be broken out.

Program flow
On Bootup
- Index .raw videos in cycle folder
- Open a .raw file, which will be the current video to play

Main Loop
- Read a frame from the current open file (blocking)
- Check for user button press (if button pressed, close current video file, and open the next one)
- Check if we are currently writing to the display (if we are, block here until finished)
- Write frame to display using SPI with DMA (non bocking)
- Repeat Loop

Debug Prints
- Prints done with Segger RTT. Requires a RTT supported debug probe

Currently 46 FPS Video playback can be achieved, this is with a blocking FatFS read from the micro SD and DMA frame display. Currently the frame display takes about 20ms and read from the frame read from the micro sd take about 22-23ms, so the 46FPS number makes sense.

Things to try to improve fps
- Larger SDIO transfer could be faster. Could try larger SDIO trasnfer with DMA and have a larger buffer in RAM to store multiple frames.
- Try increasing SPI Clock to display, currently set to 50MBit/s, may not be possible with this MCU


Usage
Expects videos to be in a folder named "cycle" at root.
Will index these on bootup and the key button on the dev board is used to cycle through the videos

Video should be in raw format, see the following for converting a 360p video called myvideo.mp4
ffmpeg  -i "myvideo.mp4" -an -filter:v:0 "crop=450:360:100:0" -s 160x128 -f rawvideo -pix_fmt rgb565be -y myvideo.raw


ST7735 driver from https://github.com/afiskon/stm32-st7735



Wiring Diagram
<img width="926" height="919" alt="image" src="https://github.com/user-attachments/assets/24b4b9d6-cb15-43bd-924a-32722e4bcbbf" />

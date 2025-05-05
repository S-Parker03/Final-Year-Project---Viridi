# Viridi

## Description

**Viridi** is a mobile application designed to work alongside a sensor system that sends environmental data to a ThingSpeak channel. The app allows users to:

- View live environmental data
- Understand the significance of each metric
- Explore historical trends through visualizations

All of this is presented in a clear and engaging interface.

## Requirements
The code for the ESP32 and sensor setup is in ESP32_Code.ino
Follow the setup instructions for your ESP32, instructions for the one detailed in my report can be found here: 
https://wiki.dfrobot.com/FireBeetle_ESP32_IOT_Microcontroller(V3.0)__Supports_Wi-Fi_&_Bluetooth__SKU__DFR0478

Simply upload the code to the ESP32 from the arduino IDE and then connect the esp32 to your sensors. Make sure to upload the code before connecting the sensors.

The code for the mobile application is contained in the zip file.
When starting the mobile app do the following:
- Run `npm install` to install dependencies
- *(Optional)* Download and set up **Expo Go** on your Android device
- Run `npx expo start` to begin development
  - Scan the QR code with Expo Go
  - Or press `a` to launch an Android emulator (if available)

## Usage

Once the app is running:

- Navigate between the **Environment** and **Data** screens
- Tap a metric on the **Environment** screen to learn what it means
- Use the **Data** screen to view historical trends by selecting a metric and a time range

## Author

- Stanley Parker ([GitHub](https://github.com/S-Parker03))

## License

This project is licensed under the [GNU General Public License v3.0](./LICENSE.txt).

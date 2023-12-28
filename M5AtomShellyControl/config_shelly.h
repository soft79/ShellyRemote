#define DIM_STEPS 3
#define DIM_MINIMUM 1
#define DIM_MIDDLE 50
#define DIM_MAXIMUM 100

#define N_Devices 6
const String DeviceUrls[N_Devices] = {
  "http://0:0@192.168.2.113/light/0/", //Woonkamer lamp
  "http://0:0@192.168.2.112/light/0/", //Eetkamer lamp
  "http://0:0@192.168.2.72/light/0/", //Keuken lamp
  "http://0:0@192.168.2.73/white/3/", //Keuken RGB
  "http://0:0@192.168.2.81/light/0/", //Buitenlamp steen
  "http://0:0@192.168.2.104/relay/0/" //Tuin LEDs
};

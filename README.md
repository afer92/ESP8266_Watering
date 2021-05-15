# ESP8266_Watering
Watering control with ESP-12E NodeMCU board
##Features
- Web parameters update
- OTA update
- Web log  viewwer
## Get data
```
http://sensorIp/
```

```
{"minindex":418,"maxindex":700,"seuilindex":630,"sensorvalue":465,"moisture":83,"pumpon":0,"epoch": 2085999012,"sequence": 303010,"wateringsleep": 10,"pgmrunning": 0,"wateringsleeping": 1,"sequencepgm": 0,"wateringsleeppgm": 0,"pgmversion": "1.0.1","timedata": "12:10:12"}
```
- minindex : sensor value at moisture 100%
- maxindex : sensor value at moisture 0%
- seuilindex : if sensor value < seuilindex, watering start
- wateringsleep : watering pause (seconds)
- sequence : 303010 - watering 10s, pause, watering 30s, pause watering 30s, end
- moisture : moisture %
- pumpon : 1 if pump on, else 0
- epoch : data timestamp (epoch)
## Set parameter
```
http://sensorIp/seuilindex=600
```

```
{"minindex":418,"maxindex":700,"seuilindex":600,"sensorvalue":465,"moisture":83,"pumpon":0,"epoch": 2085999012,"sequence": 303010,"wateringsleep": 10,"pgmrunning": 0,"wateringsleeping": 1,"sequencepgm": 0,"wateringsleeppgm": 0,"pgmversion": "1.0.1","timedata": "12:10:12"}
```
## Get log
```
http://sensorIp/log.html
```

```
15:01:01 ## Start of Program v: 1.0.1 ##
15:01:01 Connecting to WirelessAF
15:01:01 Program start : 303010
15:01:02 Moisture : 5
15:01:02 Set Pump On
15:01:12 Set Pump Off
15:01:42 Set Pump On
15:02:12 Set Pump Off
15:02:43 Set Pump On
15:03:13 Set Pump Off
15:03:13 Program stop
15:03:13 Moisture : 5
15:03:14 Program start : 303010
```
## Schema
NodeMCU Lolin V3 Module ESP8266 ESP-12F
![](https://github.com/afer92/ESP8266_Watering/blob/main/images/20210428_PasseBas.jpg?raw=true)
## Sensor
[https://www.instructables.com/Waterproofing-a-Capacitance-Soil-Moisture-Sensor/](https://www.instructables.com/Waterproofing-a-Capacitance-Soil-Moisture-Sensor/)
![](https://github.com/afer92/ESP8266_Watering/blob/main/images/20210515_212744.jpg?raw=true)
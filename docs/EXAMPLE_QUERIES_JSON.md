# Misc operations
## Get Version
### Request
```http
GET /req/version
```

### Response

```json
{
  "success": "true",
  "esp_chip": "ESP32",
  "esp_version": "0.1",
  "esp_revision": 4,
  "esp_cores": 2,
  "esp_flash": "Unknown",
  "software_version": "v1.0 2020/03/30",
  "fpga_version": "a5.10 r3"
}
```
# SD card operation
## List files
### Request
```http
GET /req/list?path=pathrelativetosdroot
```
Argument _path_ should be relative to the SD card root.
## Response
```json
{
  "path":  "/",
  "success":  true,
        "error": "errorstring in case of error",
  "entries":  [{
      "type":  "file",
      "name":  "README.md",
      "size":  33
    }, {
      "type":  "dir",
      "name":  "DirNo1"
    }]
}
```
## Delete file
### Request
```http
GET /req/delete?path=pathrelativetosdroot/filename.ext
```

### Response
```json
{
  "path" : "pathrelativetosdroot/filename.ext",
  "success": "true",
  "error": "errorstring in case of error"
}
```

## Rename/move file
### Request (rename)
```http
GET /req/rename?path=pathrelativetosdroot/filename.ext&newpath=newfilename.ext
```
### Request (move)
```http
GET /req/rename?path=pathrelativetosdroot/filename.ext&newpath=pathrelativetosdroot/newfilename.ext
````
### Response
```json
{
  "path" : "pathrelativetosdroot/newfilename.ext",
  "success":  "true",
  "error": "errorstring in case of error"
}
```

## Create directory
### Request
```http
GET /req/mkdir?path=pathrelativetosdroot/newdir
```
### Response
```json
{
  "path" : "pathrelativetosdroot/newdir",
  "success": "true",
  "error": "errorstring in case of error"
}
```

## Delete directory
### Request
```
GET /req/rmdir?path=pathrelativetosdroot/newdir
```
### Response
```json
{
  "path" : "pathrelativetosdroot/newdir",
  "success": "true",
  "error": "errorstring in case of error"
}
```

## Upload file
### Request
```http
POST /upload?path=pathrelativetosdroot/filename.ext
```

### Response
_note that filename might differ due to size limitations_
```json
{
  "success":  "true",
  "error": "errorstring in case of error",
  "path" : "pathrelativetosdroot/filename.ext",
  "size" : "filesizebytes",
}
```

## Upload file data
### Request
```http
POST /upload/fileupload?token=sometokenhere
```
The token should be obtained first by calling [Upload file] request
### Response
```json
{
  "success":  "true",
  "error": "errorstring in case of error"
}
```
# Device configuration
The following operations allow one to configure USB/Bluetooth devices.

## List devices
List all configured and unconfigured devices
### Request
```http
GET /req/devlist
```
### Response

```json
{
  "devices": [
  	{ "bus" : "usb",
          "driver": "usb_mouse",
          "id": "0adf:2342", 
          "vendor": "Vendor name here",
          "product": "Product name here",
          "serial" : "Serial here",
          "class": "mouse,pointer",
          "connected": "true",
          "use": "mouse"
        }
  ]
}
```
"class" is a list of supported classes/uses for this device.
"use" is the current use (configuration) for the device, or null if not configured.

## Get configuraion
### Request
```http
GET /req/devconf?usbid=0adf:2342?serial=xpto
```
### Response
```json
{
	"bus": "usb",
	"id": "0adf:2342",
        "serial": "Serial here",
        "use" : "mouse", 
        "controls" : [
        ]
        "mappings" : {
        	"default" : [
                       { "index" : 0,
                         "map": "keyboard", 
                         "value", "enter"
                       },
                       { "index" : 1,
                         "map": "keyboard",
                         "threshold": -128
                         "value": "q"
                       }
                ]
        }
}
```

## Configure input
```http
POST /req/devconf
```
POST contents:
```json
{
	"bus": "usb",
	"id": "0adf:2342",
        "serial": "Serial here",
        "use" : "mouse", 
        "mapping": "default",
        
        
}
```

# WiFi operations
## Get configuration
### Request
```http
GET /req/wifi
```
### Response

```json
{
  "mode" : "ap", 
  "sta": {
   	"ssid": "ssid",
        "hostname": "interfacez", 
        "ip": "dhcp", 
        "netmask": "255.255.255.0", 
        "gw": "127.0.0.1", 
  },
  "ap" : {
   	"ssid": "ssid",
        "hostname": "interfacez",
        "channel": 1,
        "ip" : "127.0.0.1",
        "netmask" : "255.255.255.0",
        "gw" : "127.0.0.1"
  }
}
```
## Start scan
### Request
```http
GET /req/startscan
```
### Response

```json
{
  "success": "true",
  "error": "errorstring in case of error"
}
```
## Get scan
### Request
```http
GET /req/scan
```
### Response

```json
{
  "success": "true",
  "error": "errorstring in case of error",
  "aplist" : [
  {
  	"ssid": "ssid",
        "channel": 4,
        "auth": 3 
  }
  ]
}
```
For *auth* values see ESP32 *wifi_auth_mode_t*.

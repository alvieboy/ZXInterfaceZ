# Misc operations
## Get Version
### Request
```http
GET /req/version```

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
GET /req/move?path=pathrelativetosdroot/filename.ext&newpath=newfilename.ext
```
### Request (move)
```http
GET /req/move?path=pathrelativetosdroot/filename.ext&newpath=pathrelativetosdroot/newfilename.ext
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
GET /req/upload?path=pathrelativetosdroot/filename.ext&size=filesizeinbytes
```

### Response
_note that filename might differ due to size limitations_
```json
{
  "path" : "pathrelativetosdroot/filename.ext",
  "size" : "filesizebytes",
  "success":  "true",
  "error": "errorstring in case of error",
  "uploadtoken":  "sometokenhere"
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

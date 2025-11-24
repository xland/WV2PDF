# WV2PDF

Use WebView2 to Create PDF

# Download

 [Releases](https://github.com/xland/WV2PDF/releases)

# Usage

- Download [Service.exe](https://github.com/xland/WV2PDF/releases)
- Start Service.exe
- Then connect Service

```javascript
const WebSocket = require("ws");
const ws = new WebSocket(`ws://127.0.0.1:8080`);
let objs = [
    { url: "https://www.baidu.com" }, 
    { url: "https://www.cnblogs.com" }, 
    { url: "https://www.csdn.net" }, 
    { url: "https://www.oschina.net" }
];
ws.on("open", () => {
  console.log("connected");
  for (let obj of objs) {
    ws.send(JSON.stringify(obj));
  }
});
ws.on("message", (data) => {
  let obj = JSON.parse(data.toString());
  console.log("receive msg:", obj);
});

ws.on("close", () => console.log("conn closed"));
ws.on("error", (err) => console.error("error:", err));
```
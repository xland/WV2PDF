const WebSocket = require("ws");
const ws = new WebSocket(`ws://127.0.0.1:8080`);

let objs = [{ url: "https://www.baidu.com" }, { url: "https://www.cnblogs.com" }, { url: "https://www.csdn.net" }, { url: "https://www.oschina.net" }];

ws.on("open", () => {
  console.log("已连接到服务器");
  for (let obj of objs) {
    ws.send(JSON.stringify(obj));
  }
});

ws.on("message", (data) => {
  let obj = JSON.parse(data.toString());
  console.log("收到服务器消息:", obj);
});

ws.on("close", () => console.log("连接已关闭"));
ws.on("error", (err) => console.error("错误:", err));

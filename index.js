const WebSocket = require("ws");
const ws = new WebSocket("ws://127.0.0.1:8080");

ws.on("open", () => {
  console.log("已连接到服务器");
  let msg = { action: "url2pdf", url: "https://www.baidu.com" };
  ws.send(JSON.stringify(msg));
});

ws.on("message", (data) => {
  console.log("收到服务器消息:", data.toString());
});

ws.on("close", () => console.log("连接已关闭"));
ws.on("error", (err) => console.error("错误:", err));

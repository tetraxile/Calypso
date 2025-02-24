var ws;

function init() {
  ws = new WebSocket("ws://localhost:8173/");

  ws.onopen = function() {
    output("onopen");
  };
  
  ws.onmessage = function(e) {
    output("onmessage: " + e.data);
  };
  
  ws.onclose = function() {
    output("onclose");
  };

  ws.onerror = function(e) {
    output("onerror");
    console.log(e)
  };
}

function onCloseClick() {
  ws.close();
}

function onButtonClick() {
  ws.send("hi :3");
}

function onChooseFile(e) {
  if (!e || !e.target || !e.target.files || event.target.files.length === 0) {
    return;
  }

  const file = e.target.files[0];
  ws.send(file.name);
  let reader = new FileReader();
  reader.readAsArrayBuffer(file);

  reader.onload = readerEvent => {
    let content = readerEvent.target.result;
    ws.send(content);
  }
}

function output(str) {
  var log = document.getElementById("log");
  var escaped = str.replace(/&/, "&amp;").replace(/</, "&lt;").
    replace(/>/, "&gt;").replace(/"/, "&quot;"); // "
  log.innerHTML = escaped + "<br>" + log.innerHTML;
}

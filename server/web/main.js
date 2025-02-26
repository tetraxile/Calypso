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
  ws.send("type: close");
  ws.close();
}

function onButtonClick() {
  ws.send("type: button")
  ws.send("hi :3");
}

function onSettingsClick() {
  moonrefresh = document.getElementById("MoonRefresh").checked;
  cutscenes = document.getElementById("Cutscenes").checked;
  moonlock = document.getElementById("MoonLock").checked;
  checkpoints = document.getElementById("Checkpoints").checked;
  autosave = document.getElementById("Autosave").checked;
  hud = document.getElementById("HUD").checked;
  music = document.getElementById("Music").checked;
  ws.send("type: settings");
  ws.send(moonrefresh);
  ws.send(cutscenes);
  ws.send(moonlock);
  ws.send(checkpoints);
  ws.send(autosave);
  ws.send(hud);
  ws.send(music);
}

function onChooseFile(e) {
  if (!e || !e.target || !e.target.files || event.target.files.length === 0) {
    return;
  }

  const file = e.target.files[0];
  let reader = new FileReader();
  reader.readAsArrayBuffer(file);

  reader.onload = readerEvent => {
    let content = readerEvent.target.result;

    ws.send("type: script");
    ws.send(file.name);
    ws.send(content.byteLength.toString());
    ws.send(content);
  }
}

function output(str) {
  var log = document.getElementById("log");
  var escaped = str.replace(/&/, "&amp;").replace(/</, "&lt;").
    replace(/>/, "&gt;").replace(/"/, "&quot;"); // "
  log.innerHTML = escaped + "<br>" + log.innerHTML;
}

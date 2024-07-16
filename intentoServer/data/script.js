var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

// Init web socket when the page loads
window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
}

function getReadings(){
    websocket.send("getReadings");
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

// When websocket is established, call the getReadings() function
function onOpen(event) {
    console.log('Connection opened');
    getReadings();
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

// Function that receives the message from the ESP32 with the readings
function onMessage(event) {
    console.log(event.data);
    var myObj = JSON.parse(event.data);
    var keys = Object.keys(myObj);

    for (var i = 0; i < keys.length; i++){
        var key = keys[i];
        document.getElementById(key).innerHTML = myObj[key];
    }

    // Obtener los valores de suciedad y CO2
    var suciedad = parseInt(myObj.suciedad);
    var co2 = parseFloat(myObj.co2);

    // Mostrar asterisco rojo si se superan los umbrales
    if (suciedad > 1000 || co2 > 0.04) {  // Ajusta los umbrales según tus necesidades
        document.getElementById('alert').innerHTML = "<span style='color: red;'>*</span>";
    } else {
        document.getElementById('alert').innerHTML = "Normal";
    }
}

function funcionBoton() {
    websocket.send("activarServo");
}

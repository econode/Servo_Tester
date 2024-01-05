window.addEventListener('load', handleOnLoad);

var websocket;

function handleOnLoad(event) {
    updateFromConfigValues();
    document.getElementById("buttonUpdate").addEventListener("pointerup",handleBtnUpdate);
    document.getElementById("buttonSweepStart").addEventListener("pointerup",handleBtnSweepStart);
    document.getElementById("enableServo").addEventListener("change",handleEnableServo);
    let wsGatewayAddr = servoConfig.wsGatewayAddr == "%wsGatewayAddr%" ? "ws://192.168.4.1/ws":servoConfig.wsGatewayAddr;
    initWebSocket( wsGatewayAddr );
    updateFromConfigValues();
}

function updateFromConfigValues(){
    document.getElementById("enableServo").checked = servoValues.servoEnabled==1;
    document.getElementById("servoFreq").value = servoValues.servoFreq;
    document.getElementById("servoMinPulse").value = servoValues.servoMinPulse;
    document.getElementById("servoMaxPulse").value = servoValues.servoMaxPulse;
    document.getElementById("servoSettleTime").value = servoValues.servoSettleTime;
}

function updateMessage(){
    const oUpdatedMessage = document.getElementById("updatedMessage");
    oUpdatedMessage.style.display = "block";
    setTimeout( function(){oUpdatedMessage.style.display = "none";},1500);
}

function handleBtnUpdate(event){
    var ServoFrequency = parseInt( document.getElementById("servoFreq").value );
    var ServoMinPulse = parseInt( document.getElementById("servoMinPulse").value );
    var ServoMaxPulse = parseInt( document.getElementById("servoMaxPulse").value );
    var ServoSettleTime = parseInt( document.getElementById("servoSettleTime").value );

    var ServoEnabled = document.getElementById("enableServo").checked ? 1 : 0;
    var jsonData = JSON.stringify({
        message:'updateServo',
        ServoFrequency:ServoFrequency,
        ServoMinPulse:ServoMinPulse,
        ServoMaxPulse:ServoMaxPulse,
        ServoSettleTime:ServoSettleTime,
        ServoEnabled:ServoEnabled
    });
    console.log(jsonData);
    websocket.send(jsonData);
}

function handleBtnSweepStart(event){
    var SweepStart = parseInt( document.getElementById("sweepStart").value );
    var SweepEnd = parseInt( document.getElementById("sweepEnd").value );
    var SweepDelay = parseInt( document.getElementById("sweepDelay").value );
    var ServoEnabled = document.getElementById("enableServo").checked ? 1 : 0;
    var jsonData = JSON.stringify({
        message:'sweepStart',
        SweepStart:SweepStart,
        SweepEnd:SweepEnd,
        SweepDelay:SweepDelay,
        ServoEnabled:ServoEnabled
    });
    console.log(jsonData);
    websocket.send(jsonData);
}

function handleEnableServo(event){
    var ServoEnabled = document.getElementById("enableServo").checked ? 1 : 0;
    var jsonData = JSON.stringify({
        message:'servoEnable',
        ServoEnabled:ServoEnabled
    });
    console.log(jsonData);
    websocket.send(jsonData);
}

function confirmUpdate(jsonData){
    var ServoFrequency = parseInt(jsonData.ServoFrequency);
    var ServoMinPulse = parseInt(jsonData.ServoMinPulse);
    var ServoMaxPulse = parseInt(jsonData.ServoMaxPulse);
    var ServoEnabled = parseInt(jsonData.ServoEnabled);
    var ServoSettleTime = parseInt(jsonData.ServoSettleTime);
    document.getElementById("servoFreq").value = ServoFrequency;
    document.getElementById("servoMinPulse").value = ServoMinPulse;
    document.getElementById("servoMaxPulse").value = ServoMaxPulse;
    document.getElementById("servoSettleTime").value = ServoSettleTime;
    document.getElementById("enableServo").checked = ServoEnabled==1;
    updateMessage();
}

function confirmSweep(jsonData){
    var SweepRun = parseInt(jsonData.SweepRun);
    var SweepDelay = parseInt(jsonData.SweepDelay);
    var SweepEnd = parseInt(jsonData.SweepEnd);
    var SweepStart = parseInt(jsonData.SweepStart);
    document.getElementById("sweepStart").value = SweepStart;
    document.getElementById("sweepEnd").value = SweepEnd;
    document.getElementById("sweepDelay").value = SweepDelay;
    console.log("do sweep start / or stop run:"+SweepRun);
}

function initWebSocket(wsGatewayAddr){
    console.log('WS: Trying to open a WebSocket connection to '+ wsGatewayAddr );
    websocket = new WebSocket(wsGatewayAddr);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage;
}
function onOpen(event) {
    console.log('WS: Connection opened');
}

function onClose(event) {
    console.log('WS: Connection closed');
    setTimeout(initWebSocket, 2000);
}
function onMessage(event) {
    var messageType = false
    var jsonData = false
    // Make sure we have valid JSON
    try {
        jsonData = JSON.parse(event.data)
    } catch (e) {
    console.log("Websocket JSON parse error")
    console.log(e)
    }
    console.log(jsonData);
    if( jsonData.message == "confirmUpdate" ){
        confirmUpdate(jsonData);
    }
    if( jsonData.message == "confirmSweep" ){
        confirmSweep(jsonData);
    }
}

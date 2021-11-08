function poll() {
    var sleep = time => new Promise(resolve => setTimeout(resolve, time))
    var poll = (promiseFn, time) => promiseFn().then(sleep(time).then(() => poll(promiseFn, time)))
    var received = false;
    poll(() => new Promise(() => {
        if (!received) {
            received = true;
            const http = new XMLHttpRequest();
            http.open('GET', 'getsettings');
            http.send();
            http.onload = () => {
                console.log(http.responseText);
                var prefs = JSON.parse(http.responseText);
                console.log(prefs);
                $('#deviceName').val(prefs.deviceName);
                $('#microcontrollerIP').val(prefs.ip);
                if (prefs.mqttIp > 0) {
                    $("#mqttCheckbox").prop('checked', true);
                }
                $('#deviceName').val(prefs.deviceName);
                $('#inputMqttIp').val(prefs.mqttIp);
                $('#mqttPort').val(prefs.mqttPort);
                $('#mqttuser').val(prefs.mqttuser);
                $('#mqttpass').val(prefs.mqttpass);
                $('#additionalParam').val(prefs.gpio);
            }
        }
    }), 5000);
}
function callDevice(payload) {
    console.log(payload);
    const http = new XMLHttpRequest();
    http.open('GET', 'lights/glowwormluciferin/set?payload=' + payload);
    http.send();
    http.onload = () => console.log(http.responseText);
}
function createPayload() {
    var toggleLED = $('#toggleLED')[0];
    var payload = {
        state: toggleLED.classList.contains('active') ? 'ON' : 'OFF',
        effect: $('#effectSelect').val(),
        color: colorPicker.color.rgb,
        whitetemp: $('#whiteTempSelect').val()
    }
    return payload;
}
const sleep = (s) => {
  return new Promise(resolve => setTimeout(resolve, (s*1000)));
};
sleep(2).then(() => {
  poll();
});

var received = false;

function poll() {
    var sleep = time => new Promise(resolve => setTimeout(resolve, time))
    var poll = (promiseFn, time) => promiseFn().then(sleep(time).then(() => poll(promiseFn, time)))
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
                if (prefs.mqttIp.length > 0) {
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
function callDevice() {
    let payload = createPayload();
    console.log(payload);
    const http = new XMLHttpRequest();

    http://192.168.4.1/setting?
        // deviceName=GLOW_WORM_ESP8266
        // &microcontrollerIP=
        // &ssid=NetStar
        // &pass=Reda7983bfg9000dare83799000bfgreda7983bfg9000dare83799000bfg%25%25
        // &OTApass=123StellaStella
        // &mqttCheckbox=on
        // &mqttIP=192.168.1.3
        // &mqttPort=1883
        // &mqttuser=dpsoftwaremqtt
        // &mqttpass=123StellaStella
        // &additionalParam=2

    http.open('GET', 'setting?payload=' + payload);
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

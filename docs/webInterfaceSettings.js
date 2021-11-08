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
                if (prefs.mqttIp.length == 0) {
                    $("#mqttCheckbox").click();
                }
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
    let payload = "deviceName=" + $('#deviceName').val();
    payload += "&microcontrollerIP=" + $('#microcontrollerIP').val();
    payload += "&mqttCheckbox=" + $("#mqttCheckbox").prop("checked");
    payload += "&mqttIP=" + $('#inputMqttIp').val();
    payload += "&mqttPort=" + $('#mqttPort').val();
    payload += "&mqttuser=" + $('#mqttuser').val();
    payload += "&mqttpass=" + $('#mqttpass').val();
    payload += "&additionalParam=" + $('#additionalParam').val();
    console.log(payload);
    const http = new XMLHttpRequest();
    http.open('GET', 'setting?' + payload);
    http.send();
    http.onload = () => {
        console.log(http.responseText);
        alert("Success: rebooting the microcontroller");        
        return false;
    };
}
const sleep = (s) => {
  return new Promise(resolve => setTimeout(resolve, (s)));
};
sleep(500).then(() => {
  poll();
});

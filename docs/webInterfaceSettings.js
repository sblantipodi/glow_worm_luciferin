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
                if (prefs.dhcp == '0') {
                    $("#dhcpCheckbox").click();
                }
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
function mqttCheckboxAction(cbMqtt, cbDhcp) {
    if (cbMqtt.checked) {
        document.getElementById('inputMqttIp').setAttribute('required', '');
        document.getElementById('mqttPort').setAttribute('required', '');
        document.getElementById('inputMqttIp').disabled = false;
        document.getElementById('mqttPort').disabled = false;
        document.getElementById('mqttPort').disabled = false;
        document.getElementById('mqttuser').disabled = false;
        document.getElementById('mqttpass').disabled = false;
        document.getElementById('inputMqttIp').disabled = false;
    } else {
        document.getElementById('inputMqttIp').removeAttribute('required');
        document.getElementById('inputMqttIp').disabled = true;
        document.getElementById('mqttPort').removeAttribute('required');
        document.getElementById('mqttPort').disabled = true;
        document.getElementById('mqttPort').value = "";
        document.getElementById('mqttPort').disabled = true;
        document.getElementById('mqttuser').value = "";
        document.getElementById('mqttuser').disabled = true;
        document.getElementById('mqttpass').value = "";
        document.getElementById('mqttpass').disabled = true;
        document.getElementById('inputMqttIp').value = "";
        document.getElementById('inputMqttIp').disabled = true;
    }
    if (cbDhcp.checked) {
        document.getElementById('microcontrollerIP').disabled = false;
    } else {
        document.getElementById('microcontrollerIP').disabled = true;
        document.getElementById('microcontrollerIP').value = "";
    }
}
const sleep = (s) => {
  return new Promise(resolve => setTimeout(resolve, (s)));
};
sleep(100).then(() => {
  poll();
});

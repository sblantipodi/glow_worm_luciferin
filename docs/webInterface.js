function poll() {
    var sleep = time => new Promise(resolve => setTimeout(resolve, time))
    var poll = (promiseFn, time) => promiseFn().then(sleep(time).then(() => poll(promiseFn, time)))
    poll(() => new Promise(() => {
      const http = new XMLHttpRequest();
      http.open('GET', 'prefs');
      http.send();
      http.onload = () => {
        console.log(http.responseText);
        var prefs = JSON.parse(http.responseText);
        console.log(prefs);
        if (prefs.toggle == '0') {
          $('#toggleLED').css('background-color','lightgrey');
          $('#toggleLED')[0].textContent = "Turn ON";
          $('#toggleLED').removeClass("active");
        } else if (prefs.toggle == '1') {
          $('#toggleLED').css('background-color','orange');
          $('#toggleLED')[0].textContent = "Turn OFF";
          $('#toggleLED').addClass("active");
        }
        $('#effectSelect').val(prefs.effect.length == 0 ? 'solid' : prefs.effect);
        $('#whiteTempSelect').val(prefs.whiteTemp == 0 ? 1 : prefs.whiteTemp);
        if (prefs.cp.length > 0) {
          colorPicker.color.rgb = { r: prefs.cp.split(',')[0], g: prefs.cp.split(',')[1], b: prefs.cp.split(',')[2] }
        }
        $('#gitlink').text("Glow Worm Luciferin (V"+prefs.VERSION+')');
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
var colorPicker = new iro.ColorPicker('#picker', {
    width: 320,
    color: '#0091ff'
});
$('#effectSelect').change(function () {
    callDevice(JSON.stringify(createPayload()));
});
$('#whiteTempSelect').change(function () {
    callDevice(JSON.stringify(createPayload()));
});
$('#toggleLED').click(function () {
    callDevice(JSON.stringify(createPayload()));
    var toggleLED = $('#toggleLED')[0];
    if (toggleLED.classList.contains('active')) {
        $('#toggleLED').css('background-color','orange');
        $('#toggleLED')[0].textContent = "Turn OFF";
    } else {
        $('#toggleLED').css('background-color','lightgrey');
        $('#toggleLED')[0].textContent = "Turn ON";
    }
});
colorPicker.on(['input:end'], function(color) {
    callDevice(JSON.stringify(createPayload()));
});
const sleep = (s) => {
  return new Promise(resolve => setTimeout(resolve, (s*1000)));
};
$('#effectSelect').append(new Option("solid", "Solid"));
$('#effectSelect').append(new Option("fire", "Fire"));
$('#effectSelect').append(new Option("twinkle", "Twinkle"));
$('#effectSelect').append(new Option("bpm", "Bpm"));
$('#effectSelect').append(new Option("rainbow", "Rainbow"));
$('#effectSelect').append(new Option("mixed rainbow", "Mixed rainbow"));
$('#effectSelect').append(new Option("chase rainbow", "Chase rainbow"));
$('#effectSelect').append(new Option("solid rainbow", "Solid rainbow"));

sleep(2).then(() => {
  poll();
});

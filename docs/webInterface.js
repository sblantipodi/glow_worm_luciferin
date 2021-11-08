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
        if (prefs.framerate > 0) {
            $('#effectSelect').val("GlowWorm");
        } else {
            $('#effectSelect').val(prefs.effect.length == 0 ? 'solid' : prefs.effect);
        }
        $('#whiteTempSelect').val(prefs.whiteTemp == 0 ? 1 : prefs.whiteTemp);
        if (prefs.cp.length > 0) {
          colorPicker.color.rgb = { r: prefs.cp.split(',')[0], g: prefs.cp.split(',')[1], b: prefs.cp.split(',')[2] }
        }
        $('#gitlink').text("Glow Worm Luciferin (V"+prefs.VERSION+')');
        $('#wifi').text(prefs.wifi + '% ');
        $('#fps').text(prefs.framerate + 'FPS');
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
sleep(2).then(() => {
  $('#effectSelect').append(new Option("Solid", "solid"));
  $('#effectSelect').append(new Option("Fire", "fire"));
  $('#effectSelect').append(new Option("Twinkle", "twinkle"));
  $('#effectSelect').append(new Option("Bpm", "bpm"));
  $('#effectSelect').append(new Option("Rainbow", "rainbow"));
  $('#effectSelect').append(new Option("Mixed rainbow", "mixed rainbow"));
  $('#effectSelect').append(new Option("Chase rainbow", "chase rainbow"));
  $('#effectSelect').append(new Option("Solid rainbow", "solid rainbow"));
  $('#effectSelect').append(new Option("BIAS LIGHT", "GlowWorm"));
  $('#whiteTempSelect').append(new Option("Uncorrected temperature", "1"));
  $('#whiteTempSelect').append(new Option("1900 Kelvin", "2"));
  $('#whiteTempSelect').append(new Option("2600 Kelvin", "3"));
  $('#whiteTempSelect').append(new Option("2850 Kelvin", "4"));
  $('#whiteTempSelect').append(new Option("3200 Kelvin", "5"));
  $('#whiteTempSelect').append(new Option("5200 Kelvin", "6"));
  $('#whiteTempSelect').append(new Option("5400 Kelvin", "7"));
  $('#whiteTempSelect').append(new Option("6000 Kelvin", "8"));
  $('#whiteTempSelect').append(new Option("7000 Kelvin", "9"));
  $('#whiteTempSelect').append(new Option("20000 Kelvin", "10"));
  $('#whiteTempSelect').append(new Option("Warm Fluorescent", "11"));
  $('#whiteTempSelect').append(new Option("Standard Fluorescent", "12"));
  $('#whiteTempSelect').append(new Option("Cool White Fluorescent", "13"));
  $('#whiteTempSelect').append(new Option("Full Spectrum Fluorescent", "14"));
  $('#whiteTempSelect').append(new Option("Grow Light Fluorescent", "15"));
  $('#whiteTempSelect').append(new Option("Black Light Fluorescent", "16"));
  $('#whiteTempSelect').append(new Option("Mercury Vapor", "17"));
  $('#whiteTempSelect').append(new Option("Sodium Vapor", "18"));
  $('#whiteTempSelect').append(new Option("Metal Halide", "19"));
  $('#whiteTempSelect').append(new Option("High Pressure Sodium", "20"));    
  poll();
});

// DuoStats - Pebble JS config bridge (no external dependencies)

function sendSettings(settings) {
  Pebble.sendAppMessage(
    {
      'DARK_MODE': settings.darkMode ? 1 : 0,
      'BOX_A':     parseInt(settings.boxA, 10),
      'BOX_B':     parseInt(settings.boxB, 10),
      'BOX_C':     parseInt(settings.boxC, 10),
      'BOX_D':     parseInt(settings.boxD, 10)
    },
    function() { console.log('[DuoStats] Settings sent OK'); },
    function(e) { console.log('[DuoStats] Settings send failed: ' + JSON.stringify(e)); }
  );
}

Pebble.addEventListener('ready', function() {
  console.log('[DuoStats] PebbleKit JS ready');
});

Pebble.addEventListener('showConfiguration', function() {
  var darkMode = localStorage.getItem('darkMode') || '1';
  var boxA     = localStorage.getItem('boxA')     || '0';
  var boxB     = localStorage.getItem('boxB')     || '1';
  var boxC     = localStorage.getItem('boxC')     || '2';
  var boxD     = localStorage.getItem('boxD')     || '3';

  // Host config.html on a web server and set this URL.
  // e.g. GitHub Pages: https://yourusername.github.io/duostats/config.html
  var configUrl = 'https://your-host/duostats/config.html' +
    '?darkMode=' + darkMode +
    '&boxA='     + boxA +
    '&boxB='     + boxB +
    '&boxC='     + boxC +
    '&boxD='     + boxD;

  Pebble.openURL(configUrl);
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (!e.response) { return; }
  try {
    var settings = JSON.parse(decodeURIComponent(e.response));
    localStorage.setItem('darkMode', settings.darkMode ? '1' : '0');
    localStorage.setItem('boxA', String(settings.boxA));
    localStorage.setItem('boxB', String(settings.boxB));
    localStorage.setItem('boxC', String(settings.boxC));
    localStorage.setItem('boxD', String(settings.boxD));
    sendSettings(settings);
  } catch (err) {
    console.log('[DuoStats] Config parse error: ' + err);
  }
});

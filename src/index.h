const char INDEX[] PROGMEM = R"=====(
<!DOCTYPE html><html lang="en"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0"><title>Spotify API for ESP32</title></head><body><div id="login"><h1 id="title">Spotify API for ESP32</h1><h2 id="subtitle">get current track and put in esp32 screen</h2><button id="login-button"> Log in with Spotify </button></div><script src="https://cdn.jsdelivr.net/npm/js-sha256@0.9.0/build/sha256.min.js"></script><script type="module">
//change this value with the current clientID
const clientID = 'f8c8cb559b5245328a674b2e1771478e';

const redirectUrl = 'http://'+window.location.hostname+'/';  
const websocket = new WebSocket('ws://'+window.location.hostname+':81/');
const authorizationEndpoint = "https://accounts.spotify.com/authorize";
const scope = 'user-read-currently-playing';
const args = new URLSearchParams(window.location.search);
const code = args.get('code');
if (code) {
    websocket.onopen = function(){
      websocket.send('0'+code);
      websocket.send(localStorage.getItem('code_verifier'));
    }
    document.getElementById('title').innerText = "check esp32 screen";
    document.getElementById('subtitle').innerText = "can take some time, be patient";
    //refreshing
    const url = new URL(window.location.href);
    url.searchParams.delete("code");
    const updatedURL = url.search ? url.href : url.href.replace('?', '');
  window.history.replaceState({}, document.title, updatedURL);
}
async function loginWithSpotifyClick() {
  await redirectToSpotifyAuthorize();
}
document.getElementById('login-button').addEventListener('click', function() {
    loginWithSpotifyClick();
});
async function redirectToSpotifyAuthorize() {
  const possible = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
  const randomValues = crypto.getRandomValues(new Uint8Array(64));
  const randomString = randomValues.reduce((acc, x) => acc + possible[x % possible.length], "");
  const code_verifier = randomString;
  const data = new TextEncoder().encode(code_verifier);
  const hash = sha256.create();
  hash.update(data);
  const hashed = hash.digest();
  const code_challenge_base64 = btoa(String.fromCharCode(...new Uint8Array(hashed)))
    .replace(/=/g, '')
    .replace(/\+/g, '-')
    .replace(/\//g, '_');
  window.localStorage.setItem('code_verifier', code_verifier);
  const authUrl = new URL(authorizationEndpoint)
  const params = {
    response_type: 'code',
    client_id: clientID,
    scope: scope,
    code_challenge_method: 'S256',
    code_challenge: code_challenge_base64,
    redirect_uri: redirectUrl,
  };
  authUrl.search = new URLSearchParams(params).toString();
  window.location.href = authUrl.toString(); // Redirect the user to the authorization server for login
}
    </script>
</body>
</html>
)=====";

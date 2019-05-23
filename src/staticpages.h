#include <Arduino.h>

const char index_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en-US">

<head>
   <title>Switch</title>
   <meta charset="UTF-8">
   <meta name="viewport" content="width=device-width, initial-scale=1">
   <link rel="stylesheet" href="https://www.w3schools.com/w3css/4/w3.css">
   <link rel="stylesheet" href="https://www.w3schools.com/lib/w3-theme-black.css">
   <link href="https://fonts.googleapis.com/css?family=Roboto:100" rel="stylesheet" type="text/css">
   <link href="https://fonts.googleapis.com/css?family=Roboto+Mono:100" rel="stylesheet" type="text/css">
   <link href="favicon.ico" rel="icon" type="image/x-icon" />
   <script src="https://code.jquery.com/jquery-3.3.1.slim.min.js"
      integrity="sha384-q8i/X+965DzO0rT7abK41JStQIAqVgRVzpbzo5smXKp4YfRvH+8abtTE1Pi6jizo"
      crossorigin="anonymous"></script>
   <script>
      $(document).ready(function () {
         getStartData();
      });

      function getStartData() {
         var xhttp = new XMLHttpRequest();
         xhttp.onreadystatechange = function () {
            if (this.readyState == 4 && this.status == 200) {
               var data = this.responseText;
               var json = JSON.parse(data);
               document.title = json.title;
               document.getElementById("BoxID").innerHTML = json.title;
               document.getElementById("BoxVersion").innerHTML = json.version;
            }
         };
         xhttp.open("GET", "/JSONSTART", true);
         xhttp.send();
      }

      $("btnClear").click(function () {
         $('#SwitchResponse').empty();
      });

      $("btnRestart").click(function () {
         populatePre('/RESTART');
      });

      $("btnSwitch20").click(function () {
         populatePre('/switch20');
      });

      $("btnSwitch21").click(function () {
         populatePre('/btnSwitch21');
      });

      $("btnSwitch22").click(function () {
         populatePre('/btnSwitch22');
      });

      $("btnSwitch2A").click(function () {
         populatePre('/btnSwitch2A');
      });

      function populatePre(url) {
         var xhttp = new XMLHttpRequest();
         xhttp.onreadystatechange = function () {
            if (this.readyState == 4 && this.status == 200) {
               document.getElementById("SwitchResponse").textContent = this.responseText;
            }
         };
         xhttp.open("GET", url, true);
         xhttp.send();
      }
   </script>
   <style>
      body,
      a,
      button,
      pre,
      h1,
      h2,
      h3,
      h4,
      h5,
      h6 {
         font-family: "Roboto", sans-serif;
         margin: 0px 0px 0px 0px;
         padding: 0px 0px 0px 0px;
      }

      body,
      html {
         height: 100%;
         width: 100%;
      }

      hr,
      .div {
         margin: 1vh;
         padding: 0vh;
      }
   </style>
</head>

<body class="w3-theme-dark w3-mobile" style="min-width: 30%;">
   <div>
      <div class="w3-center">
         <h1><b><span id="BoxID">BoxID</span></b></h1>
      </div>
   </div>
   <hr class="w3-border-grey">
   <div>
      <div class="w3-bar">
         <a href="/switchu?c=0&i=0&s=1"><button class="w3-bar-item" style="width:25%">A1 ON</button></a>
         <a href="/switchu?c=0&i=1&s=1"><button class="w3-bar-item" style="width:25%">A2 ON</button></a>
         <a href="/switchu?c=0&i=2&s=1"><button class="w3-bar-item" style="width:25%">A3 ON</button></a>
         <a href="/switchu?c=0&i=3&s=1"><button class="w3-bar-item" style="width:25%">A All ON</button></a>
      </div>
      <div class="w3-bar">
         <a href="/switchu?c=0&i=0&s=0"><button class="w3-bar-item" style="width:25%">A1 OFF</button></a>
         <a href="/switchu?c=0&i=1&s=0"><button class="w3-bar-item" style="width:25%">A2 OFF</button></a>
         <a href="/switchu?c=0&i=2&s=0"><button class="w3-bar-item" style="width:25%">A3 OFF</button></a>
         <a href="/switchu?c=0&i=3&s=0"><button class="w3-bar-item" style="width:25%">A All OFF</button></a>
      </div>
      <hr class="w3-border-grey">
      <div class="w3-bar">
         <a href="/switchu?c=2&i=0&s=1"><button class="w3-bar-item" style="width:25%">B1 ON</button></a>
         <a href="/switchu?c=2&i=1&s=1"><button class="w3-bar-item" style="width:25%">B2 ON</button></a>
         <a href="/switchu?c=2&i=2&s=1"><button class="w3-bar-item" style="width:25%">B3 ON</button></a>
         <a href="/switchu?c=2&i=3&s=1"><button class="w3-bar-item" style="width:25%">B All ON</button></a>
      </div>
      <div class="w3-bar">
         <a href="/switchu?c=2&i=0&s=0"><button class="w3-bar-item" style="width:25%">B1 OFF</button></a>
         <a href="/switchu?c=2&i=1&s=0"><button class="w3-bar-item" style="width:25%">B2 OFF</button></a>
         <a href="/switchu?c=2&i=2&s=0"><button class="w3-bar-item" style="width:25%">B3 OFF</button></a>
         <a href="/switchu?c=2&i=3&s=0"><button class="w3-bar-item" style="width:25%">B All OFF</button></a>
      </div>
      <hr class="w3-border-grey">
      <div class="w3-bar">
         <a href="/switchu?c=1&i=0&s=1"><button class="w3-bar-item" style="width:25%">C1 ON</button></a>
         <a href="/switchu?c=1&i=1&s=1"><button class="w3-bar-item" style="width:25%">C2 ON</button></a>
         <a href="/switchu?c=1&i=2&s=1"><button class="w3-bar-item" style="width:25%">C3 ON</button></a>
         <a href="/switchu?c=1&i=3&s=1"><button class="w3-bar-item" style="width:25%">C All ON</button></a>
      </div>
      <div class="w3-bar">
         <a href="/switchu?c=1&i=0&s=0"><button class="w3-bar-item" style="width:25%">C1 OFF</button></a>
         <a href="/switchu?c=1&i=1&s=0"><button class="w3-bar-item" style="width:25%">C2 OFF</button></a>
         <a href="/switchu?c=1&i=2&s=0"><button class="w3-bar-item" style="width:25%">C3 OFF</button></a>
         <a href="/switchu?c=1&i=3&s=0"><button class="w3-bar-item" style="width:25%">C All OFF</button></a>
      </div>
   </div>
   <hr class="w3-border-grey">
   <div class="w3-center">
      <div>
         <button id="btnClear" class="w3-button w3-round-xlarge w3-padding-small">Clear</button>
         <a id="btnSettings" href="/settings.html" class="w3-button w3-round-xlarge w3-padding-small">Settings</a>
         <button id="btnRestart" class="w3-button w3-round-xlarge w3-padding-small">Clear</button>
      </div>
      <div>
         <span id="BoxVersion">Version</span>
      </div>
      <div>
         <pre id="SwitchResponse"></pre>
      </div>
   </div>
</body>

</html>
)=====";
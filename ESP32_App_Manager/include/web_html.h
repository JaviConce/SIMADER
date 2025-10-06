#pragma once

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>Control MQTT ESP32</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            text-align: center;
            margin: 50px;
            background-color: #f0f0f0;
        }
        h1 {
            color: #333;
        }
        h2 {
            color: #555;
            margin-top: 30px;
        }
        .info {
            background-color: #fff;
            padding: 15px;
            border-radius: 8px;
            margin: 20px auto;
            max-width: 600px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        button {
            background-color: #4CAF50;
            color: white;
            padding: 15px 32px;
            font-size: 16px;
            margin: 10px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            transition: background-color 0.3s;
        }
        button:hover {
            background-color: #45a049;
        }
        button:active {
            background-color: #3d8b40;
        }
        #response {
            margin-top: 20px;
            padding: 10px;
            background-color: #fff;
            border-radius: 4px;
            min-height: 30px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        .success {
            color: #4CAF50;
        }
        .error {
            color: #f44336;
        }
        .clients-container {
            background-color: #fff;
            padding: 20px;
            border-radius: 8px;
            margin: 20px auto;
            max-width: 600px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        .client-item {
            background-color: #f9f9f9;
            padding: 10px;
            margin: 10px 0;
            border-radius: 4px;
            border-left: 4px solid #4CAF50;
            text-align: left;
        }
        .client-id {
            font-weight: bold;
            color: #333;
        }
        .client-ip {
            font-size: 0.9em;
            color: #2196F3;
            margin-top: 5px;
        }
        .client-uptime {
            font-size: 0.9em;
            color: #666;
        }
        .no-clients {
            color: #999;
            font-style: italic;
        }
        .total-count {
            margin-top: 15px;
            font-weight: bold;
            color: #555;
        }
    </style>
</head>
<body>
    <h1>Control MQTT - ESP32 Manager</h1>
    <div class="info">
        <p><strong>Server IP:</strong> <span id="serverIP">Loading...</span></p>
    </div>

    <h2>Commands</h2>
    <div class="info">
        <p>Select a device and send commands:</p>
        <select id="deviceSelect" style="padding: 10px; margin: 10px; width: 300px; font-size: 14px;">
            <option value="">Select a device.</option>
        </select>
        <br>
        <button onclick="sendCommand('SendData')">Send Data</button>
        <button onclick="sendCommand('Check')">Check</button>
    </div>
    <div id="response"></div>

    <h2>Connected Devices</h2>
    <div class="clients-container">
        <div id="clientsList">Loading clients...</div>
        <div class="total-count" id="totalClients">Total: 0 device</div>
    </div>

    <script>
        document.getElementById('serverIP').textContent = window.location.hostname;

        function sendCommand(command) {
            const deviceSelect = document.getElementById('deviceSelect');
            const responseDiv = document.getElementById('response');

            const selectedIP = deviceSelect.value;

            if(!selectedIP) {
                responseDiv.innerHTML = '<span class="error">Please select a device</span>';
                setTimeout(() => {
                    responseDiv.innerHTML = '';
                }, 3000);
                return;
            }

            responseDiv.innerHTML = 'Sending command ' + command + '...';

            fetch('/send?ip=' + encodeURIComponent(selectedIP) + '&cmd=' + encodeURIComponent(command))
                .then(response => response.text())
                .then(data => {
                    responseDiv.innerHTML = '<span class="success">' + data + '</span>';
                    setTimeout(() => {
                        responseDiv.innerHTML = '';
                    }, 5000);
                })
                .catch(error => {
                    responseDiv.innerHTML = '<span class="error">Error: ' + error + '</span>';
                    setTimeout(() => {
                        responseDiv.innerHTML = '';
                    }, 3000);
                });
        }

        function updateClients() {
            fetch('/api/clients')
                .then(response => response.json())
                .then(data => {
                    const clientsList = document.getElementById('clientsList');
                    const totalClients = document.getElementById('totalClients');
                    const deviceSelect = document.getElementById('deviceSelect');

                    // Actualizar selector de dispositivos
                    const currentSelected = deviceSelect.value;
                    deviceSelect.innerHTML = '<option value="">Select a device...</option>';
                    data.clients.forEach(client => {
                        const option = document.createElement('option');
                        option.value = client.ip;
                        option.textContent = client.id + ' (' + client.ip + ')';
                        if(client.ip === currentSelected) {
                            option.selected = true;
                        }
                        deviceSelect.appendChild(option);
                    });

                    if(data.clients.length === 0) {
                        clientsList.innerHTML = '<div class="no-clients">No devices connected</div>';
                    } else {
                        let html = '';
                        data.clients.forEach(client => {
                            html += '<div class="client-item">';
                            html += '<div class="client-id">' + client.id + '</div>';
                            html += '<div class="client-ip">IP: ' + client.ip + '</div>';
                            html += '<div class="client-uptime">Time connected: ' + client.uptime + ' segundos</div>';
                            html += '</div>';
                        });
                        clientsList.innerHTML = html;
                    }

                    totalClients.textContent = 'Total: ' + data.total + ' devices' + (data.total !== 1 ? 's' : '');
                })
                .catch(error => {
                    document.getElementById('clientsList').innerHTML = '<div class="error">Error loading customers</div>';
                });
        }
        updateClients();
        setInterval(updateClients, 2000);
    </script>
</body>
</html>
)rawliteral";

const app = {
    sesionActiva: null,
    personas: [],
    sesionesFinalizadas: [],
    sensores: [],
    chartEMG: null,
    chartModalRealTime: null,
    connected: false,
    sesionesEnGrafico: [],
    modoGrafico: 'medias',
    modalSesion: null,
    modalCalibracion: null,
    modalRealtimeData: [],
    maxRealtimePoints: 100,
    connectionCheckInterval: null,
    lastMessageTime: null,
    calibracionActiva: false,
    calibracionInterval: null,
    sensorActual: null,
    fatigaData: {
        baseline: [],
        baselineSet: false,
        rmsBaseline: 0,
        windowSize: 20,
        currentWindow: [],
        fatigaIndex: 0,
        trend: 'estable'
    }
};

document.addEventListener('DOMContentLoaded', () => {
    initNavigation();
    initEventListeners();
    loadLocalData();
    initUIBuilder();
    initModal();
    initModoEjercicio();
    initWebcam();
    initCV();

    if (typeof Chart !== 'undefined') {
        initChart();
        initModalChart();
    } else {
        console.error('Chart.js no está cargado');
    }
});

function initUIBuilder() {
    if (typeof uibuilder !== 'undefined') {
        uibuilder.start();

        setTimeout(() => { checkConnection(); }, 500);

        app.connectionCheckInterval = setInterval(() => { checkConnection(); }, 3000);

        uibuilder.onChange('isConnected', (connected) => {
            app.connected = connected;
            updateConnectionStatus(connected);
        });

        uibuilder.onChange('msg', (msg) => {
            handleMessage(msg);
        });
    } else {
        loadTestData();
    }
}

function checkConnection() {
    if (typeof uibuilder !== 'undefined') {
        const isConnected = uibuilder.get('isConnected');
        const hasRecentMessages = app.lastMessageTime && (Date.now() - app.lastMessageTime < 10000);
        const shouldBeConnected = isConnected || hasRecentMessages;
        if (shouldBeConnected !== app.connected) {
            app.connected = shouldBeConnected;
            updateConnectionStatus(shouldBeConnected);
        }
    }
}

function initNavigation() {
    document.querySelectorAll('[data-page]').forEach(link => {
        link.addEventListener('click', (e) => {
            e.preventDefault();
            const page = e.currentTarget.dataset.page;
            showPage(page);

            document.querySelectorAll('.nav-link').forEach(l => l.classList.remove('active'));
            e.currentTarget.classList.add('active');
        });
    });
}

function showPage(pageName) {
    document.querySelectorAll('.page-content').forEach(page => {
        page.classList.add('d-none');
    });
    document.getElementById(`page-${pageName}`).classList.remove('d-none');

    if (pageName === 'historial') {
        renderHistorial();
        renderPersonasHistorialSelect();
    } else if (pageName === 'configuracion') {
        renderPersonas();
    }
}

function updateConnectionStatus(connected) {
    const statusEl = document.getElementById('connectionStatus');
    if (connected) {
        statusEl.innerHTML = '<i class="bi bi-circle-fill text-success me-1"></i> Conectado';
        statusEl.classList.remove('text-danger');
        statusEl.classList.add('text-success');
    } else {
        statusEl.innerHTML = '<i class="bi bi-circle-fill text-danger me-1"></i> Desconectado';
    }
}

function handleMessage(msg) {
    if (!msg) return;

    app.lastMessageTime = Date.now();

    if (!app.connected) {
        app.connected = true;
        updateConnectionStatus(true);
    }

    switch (msg.topic) {
        case 'personas':
            app.personas = msg.payload;
            renderPersonasSelect();
            renderPersonas();
            renderPersonasHistorialSelect();
            updatePersonasGraficoSelect();
            break;

        case 'sensores':
            if (msg.payload && msg.payload.clients) {
                app.sensores = msg.payload.clients.map(c => ({
                    id: c.id,
                    ip: c.ip,
                    nombre: c.id,
                    estado: c.connected ? 'conectado' : 'desconectado',
                    uptime: c.uptime
                }));
            } else if (Array.isArray(msg.payload)) {
                app.sensores = msg.payload;
            }
            renderSensoresSelect();
            break;

        case 'emg_data':
            if (app.sesionActiva) {
                updateStats(msg.payload);
                updateModalChart(msg.payload);
            }
            break;

        case 'sesiones':
            app.sesionesFinalizadas = msg.payload;
            renderHistorial();
            updateSesionesSelect();
            break;

        case 'notification':
            showToast(msg.payload.title, msg.payload.message, msg.payload.type);
            break;

        case 'esp32/responses':
            if (typeof msg.payload === 'string') {
                const parts = msg.payload.split(':');
                if (parts.length >= 2) {
                    const responseType = parts[1];

                    if (responseType.startsWith('Calibration_Status_')) {
                        const jsonStr = msg.payload.substring(msg.payload.indexOf('Calibration_Status_') + 19);
                        try {
                            const status = JSON.parse(jsonStr);
                            if (app.calibracionActiva) {
                                updateCalibrationUI(status);
                            }
                        } catch (e) {
                            console.error('Error parsing calibration status:', e);
                        }
                    }
                }
            }
            break;
    }
}

function initEventListeners() {
    document.getElementById('btnAbrirModal').addEventListener('click', abrirModalSesion);
    document.getElementById('btnCheck').addEventListener('click', checkSensor);
    document.getElementById('btnCalibrar').addEventListener('click', abrirModalCalibracion);
    document.getElementById('btnModalIniciar').addEventListener('click', iniciarSesion);
    document.getElementById('btnModalDetener').addEventListener('click', detenerSesion);
    document.getElementById('btnStartCalibration').addEventListener('click', iniciarCalibracion);
    document.getElementById('btnCancelCalibration').addEventListener('click', cancelarCalibracion);
    document.getElementById('formPersona').addEventListener('submit', agregarPersona);

    document.getElementById('selectPersonaGrafico').addEventListener('change', (e) => {
        const personaId = e.target.value;
        const selectSesion = document.getElementById('selectSesionGrafico');

        if (personaId) {
            selectSesion.disabled = false;
            updateSesionesSelectByPersona(personaId);
            updateChart('', personaId);
        } else {
            selectSesion.disabled = true;
            selectSesion.innerHTML = '<option value="">-- Primero selecciona persona --</option>';
            updateChart('', '');
        }
    });

    document.getElementById('selectSesionGrafico').addEventListener('change', (e) => {
        const personaId = document.getElementById('selectPersonaGrafico').value;
        if (!personaId) return;
        updateChart(e.target.value, personaId);
    });

    document.getElementById('btnRefreshChart').addEventListener('click', () => {
        const personaId = document.getElementById('selectPersonaGrafico').value;
        const sesionId = document.getElementById('selectSesionGrafico').value;
        updateChart(sesionId, personaId);
    });

    document.getElementById('btnExportCSV').addEventListener('click', exportarCSV);

    document.getElementById('btnRefreshHistory').addEventListener('click', () => renderHistorial());

    document.getElementById('selectPersonaHistorial').addEventListener('change', (e) => {
        renderHistorial(e.target.value);
    });
}

function checkSensor() {
    const sensorId = document.getElementById('selectSensor').value;

    if (!sensorId) {
        showToast('Error', 'Selecciona un sensor', 'danger');
        return;
    }

    const sensor = app.sensores.find(s => (s.id || s.ip) === sensorId);

    if (!sensor || !sensor.ip) {
        showToast('Error', 'Sensor sin IP válida', 'danger');
        return;
    }

    const sensorName = sensor.nombre || sensor.id || sensor.ip;
    const message = `${sensor.ip}:Check`;

    if (typeof uibuilder !== 'undefined') {
        uibuilder.send({ topic: 'esp32/commands', payload: message });
        showToast('Comando Enviado', `Comprobando sensor: ${sensorName}`, 'info');
    }
}

function iniciarSesion() {
    const selectPersona = document.getElementById('selectPersona');
    const selectSensor = document.getElementById('selectSensor');
    const sensorId = document.getElementById('selectSensor').value;
    const personaId = selectPersona.value;
    const descripcion = document.getElementById('inputDescripcionSesion').value.trim();

    if (!personaId) {
        showToast('Error', 'Selecciona una persona', 'danger');
        return;
    }
    if (!sensorId) {
        showToast('Error', 'Selecciona un sensor', 'danger');
        return;
    }

    const sensor = app.sensores.find(s => (s.id || s.ip) === sensorId);

    const targetIP = sensor.ip;

    const persona = app.personas.find(p => p.id == personaId);
    const nombreSesion = descripcion ||
        `${persona ? persona.nombre : 'Desconocido'} - ${new Date().toLocaleDateString('es-ES')}`;

    app.sesionActiva = {
        id: Date.now(),
        persona_id: parseInt(personaId),
        persona_nombre: persona ? persona.nombre : 'Desconocido',
        nombre: nombreSesion,
        fecha_inicio: new Date().toISOString(),
        estado: 'activa',
        datos: [],
        muestras: 0,
        suma: 0,
        max: 0,
        min: 999
    };

    const alertEl = document.getElementById('alertEstado');
    alertEl.className = 'alert mt-3 mb-0 alert-success';
    document.getElementById('estadoTexto').innerHTML =
        `<strong>CAPTURANDO</strong> - ${app.sesionActiva.persona_nombre}`;

    document.getElementById('modalAlertEstado').className = 'alert alert-success';
    document.getElementById('modalEstadoTexto').innerHTML = '<strong>SESIÓN ACTIVA</strong> - Capturando datos';
    document.getElementById('btnModalIniciar').disabled = true;
    document.getElementById('btnModalDetener').disabled = false;

    if (typeof uibuilder !== 'undefined') {
        uibuilder.send({ topic: 'session/start', payload: `${targetIP}:StartSession` });
    }

    showToast('Sesión Iniciada', `Capturando datos de ${app.sesionActiva.persona_nombre}`, 'success');

    startSessionTimer();
    resetAdaptaciones();
    ejercicioMode.effectiveReps = parseInt(document.getElementById('targetReps')?.value || '0');
    ejercicioMode.effectiveTime = parseInt(document.getElementById('targetTime')?.value || '0');
    resetStats();
    clearModalChart();
    resetFatigaDetection();
}
function detenerSesion() {
    if (!app.sesionActiva) return;

    const sensorId = document.getElementById('selectSensor').value;
    const sensor = app.sensores.find(s => (s.id || s.ip) === sensorId);
    const targetIP = sensor ? sensor.ip : null;

    app.sesionActiva.fecha_fin = new Date().toISOString();
    app.sesionActiva.estado = 'finalizada';
    app.sesionActiva.total_muestras = app.sesionActiva.muestras;

    if (app.sesionActiva.muestras > 0) {
        app.sesionActiva.emg_promedio = app.sesionActiva.suma / app.sesionActiva.muestras;
        app.sesionActiva.emg_max = app.sesionActiva.max;
        app.sesionActiva.emg_min = app.sesionActiva.min;
    } else {
        app.sesionActiva.emg_promedio = 0;
        app.sesionActiva.emg_max = 0;
        app.sesionActiva.emg_min = 0;
    }

    if (targetIP && typeof uibuilder !== 'undefined') {
        uibuilder.send({ topic: 'esp32/commands', payload: `${targetIP}:StopSession` });
    }

    app.sesionesFinalizadas.push({ ...app.sesionActiva });
    localStorage.setItem('sesionesFinalizadas', JSON.stringify(app.sesionesFinalizadas));
    localStorage.setItem('emgData_' + app.sesionActiva.id, JSON.stringify(app.sesionActiva.datos));

    if (typeof uibuilder !== 'undefined') {
        uibuilder.send({ topic: 'session/stop', payload: app.sesionActiva });
    }

    stopSessionTimer();

    showToast('Sesión Finalizada',
        `${app.sesionActiva.total_muestras} muestras - Promedio: ${app.sesionActiva.emg_promedio.toFixed(4)}`,
        'info');

    document.getElementById('inputDescripcionSesion').value = '';

    const alertEl = document.getElementById('alertEstado');
    alertEl.className = 'alert mt-3 mb-0';
    document.getElementById('estadoTexto').textContent = 'Sin sesión activa';

    document.getElementById('modalAlertEstado').className = 'alert alert-warning';
    document.getElementById('modalEstadoTexto').textContent = 'Sin sesión activa';
    document.getElementById('btnModalIniciar').disabled = false;
    document.getElementById('btnModalDetener').disabled = true;

    if (app.modalSesion) app.modalSesion.hide();

    app.sesionActiva = null;
    clearModalChart();
    updateSesionesSelect();
}

function updateStats(emgValue) {
    if (app.sesionActiva && emgValue !== undefined) {
        app.sesionActiva.muestras++;
        app.sesionActiva.suma += emgValue;
        app.sesionActiva.max = Math.max(app.sesionActiva.max, emgValue);
        app.sesionActiva.min = Math.min(app.sesionActiva.min, emgValue);
        app.sesionActiva.datos.push(emgValue);

        const promedio = app.sesionActiva.suma / app.sesionActiva.muestras;

        document.getElementById('statMuestras').textContent = app.sesionActiva.muestras;
        document.getElementById('statPromedio').textContent = promedio.toFixed(4);
        document.getElementById('statMax').textContent = app.sesionActiva.max.toFixed(4);
        document.getElementById('statUltimo').textContent = app.sesionActiva.min.toFixed(4);
        return;
    }

    const selectSesionGrafico = document.getElementById('selectSesionGrafico');
    const sessionId = selectSesionGrafico ? selectSesionGrafico.value : null;

    if (sessionId) {
        const session = app.sesionesFinalizadas.find(s => s.id == sessionId);
        if (session) {
            document.getElementById('statMuestras').textContent = session.total_muestras || 0;
            document.getElementById('statPromedio').textContent = (session.emg_promedio || 0).toFixed(4);
            document.getElementById('statMax').textContent = (session.emg_max || 0).toFixed(4);
            document.getElementById('statUltimo').textContent = (session.emg_min || 0).toFixed(4);
        }
    } else if (!app.sesionActiva) {
        resetStats();
    }
}

function resetStats() {
    document.getElementById('statMuestras').textContent = '0';
    document.getElementById('statPromedio').textContent = '0.0000';
    document.getElementById('statMax').textContent = '0.0000';
    document.getElementById('statUltimo').textContent = '0.0000';
}

function initChart() {
    const ctx = document.getElementById('chartEMG').getContext('2d');

    app.chartEMG = new Chart(ctx, {
        type: 'scatter',
        data: {
            labels: [],
            datasets: [{
                label: 'Valores EMG',
                data: [],
                borderColor: '#0d6efd',
                backgroundColor: 'rgba(13, 110, 253, 0.7)',
                borderWidth: 2,
                pointRadius: 6,
                pointHoverRadius: 10,
                pointBackgroundColor: '#0d6efd',
                pointBorderColor: '#fff',
                pointBorderWidth: 2,
                showLine: true,
                tension: 0.2
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
                legend: {
                    display: true,
                    position: 'top'
                },
                title: {
                    display: true,
                    text: 'Selecciona una persona para ver sus medias',
                    font: { size: 16, weight: 'bold' }
                },
                tooltip: {
                    callbacks: {
                        title: function (context) {
                            if (app.modoGrafico === 'medias' && app.sesionesEnGrafico.length > 0) {
                                const index = context[0].dataIndex;
                                const sesion = app.sesionesEnGrafico[index];
                                if (sesion) {
                                    const fecha = new Date(sesion.fecha_inicio).toLocaleString('es-ES');
                                    return `Sesión: ${fecha}`;
                                }
                            }
                            return '';
                        },
                        label: function (context) {
                            const point = context.parsed;
                            if (app.modoGrafico === 'medias' && app.sesionesEnGrafico.length > 0) {
                                const index = context.dataIndex;
                                const sesion = app.sesionesEnGrafico[index];
                                if (sesion) {
                                    return [
                                        `Media: ${point.y.toFixed(4)}`,
                                        `Muestras: ${sesion.total_muestras || 0}`,
                                        `Máx: ${(sesion.emg_max || 0).toFixed(4)}`,
                                        `Mín: ${(sesion.emg_min || 0).toFixed(4)}`,
                                        '',
                                        'Clic para ver todos los valores'
                                    ];
                                }
                            }
                            return `EMG: ${point.y.toFixed(4)}`;
                        }
                    }
                }
            },
            scales: {
                y: {
                    beginAtZero: true,
                    title: {
                        display: true,
                        text: 'Valor EMG'
                    },
                    grid: {
                        color: 'rgba(0, 0, 0, 0.05)'
                    }
                },
                x: {
                    type: 'linear',
                    position: 'bottom',
                    title: {
                        display: true,
                        text: 'Sesión'
                    },
                    grid: {
                        color: 'rgba(0, 0, 0, 0.05)'
                    }
                }
            },
            interaction: {
                intersect: false,
                mode: 'nearest'
            },
            onClick: (event, elements) => {
                if (app.modoGrafico !== 'medias' || elements.length === 0) return;

                const index = elements[0].index;
                const sesion = app.sesionesEnGrafico[index];

                if (sesion && sesion.id) {
                    const personaId = document.getElementById('selectPersonaGrafico').value;
                    document.getElementById('selectSesionGrafico').value = sesion.id;
                    updateChart(sesion.id, personaId);
                }
            },
            onHover: (event, elements) => {
                const canvas = event.native.target;
                if (app.modoGrafico === 'medias' && elements.length > 0) {
                    canvas.style.cursor = 'pointer';
                } else {
                    canvas.style.cursor = 'default';
                }
            }
        }
    });
}

async function updateChart(sessionId, personaId) {
    if (!app.chartEMG) return;

    const persona = personaId ? app.personas.find(p => p.id == personaId) : null;

    if (!personaId) {
        app.chartEMG.data.labels = [];
        app.chartEMG.data.datasets[0].data = [];
        app.chartEMG.options.plugins.title.text = 'Selecciona una persona para ver sus datos EMG';
        app.chartEMG.options.scales.x.title.text = '';
        app.chartEMG.update();
        return;
    }

    // Filtrar sesiones por persona
    const sesionesFiltradas = app.sesionesFinalizadas.filter(s => s.persona_id == personaId);

    if (!sessionId || sessionId === '') {
        app.modoGrafico = 'medias';
        app.sesionesEnGrafico = sesionesFiltradas;

        if (sesionesFiltradas.length === 0) {
            app.chartEMG.data.labels = [];
            app.chartEMG.data.datasets[0].data = [];
            app.chartEMG.options.plugins.title.text = `${persona.nombre}: Sin sesiones registradas`;
            app.chartEMG.update();
            return;
        }

        const scatterData = sesionesFiltradas.map((s, i) => ({
            x: i + 1,
            y: s.emg_promedio || 0
        }));

        const labels = sesionesFiltradas.map((s) => {
            return new Date(s.fecha_inicio).toLocaleDateString('es-ES', {
                day: '2-digit',
                month: '2-digit'
            });
        });

        app.chartEMG.data.labels = labels;
        app.chartEMG.data.datasets[0].data = scatterData;
        app.chartEMG.data.datasets[0].label = 'Media EMG por sesión (clic para ver detalle)';
        app.chartEMG.data.datasets[0].pointRadius = 12;
        app.chartEMG.data.datasets[0].pointHoverRadius = 16;
        app.chartEMG.data.datasets[0].showLine = true;
        app.chartEMG.data.datasets[0].borderColor = '#0d6efd';
        app.chartEMG.data.datasets[0].backgroundColor = 'rgba(13, 110, 253, 0.7)';
        app.chartEMG.options.plugins.title.text = `${persona.nombre} - Medias EMG (${sesionesFiltradas.length} sesiones) - Haz clic en un punto para ver detalles`;

        app.chartEMG.options.scales.x = {
            type: 'linear',
            position: 'bottom',
            title: { display: true, text: 'Sesión' },
            ticks: {
                callback: function (value) {
                    return labels[value - 1] || '';
                }
            }
        };
        app.chartEMG.options.scales.y.title.text = 'Media EMG';

    } else {
        app.modoGrafico = 'detalle';
        app.sesionesEnGrafico = [];

        const session = sesionesFiltradas.find(s => s.id == sessionId);
        let emgData = [];

        try {
            const res = await fetch(`/api/sesion/${sessionId}/datos`);
            if (res.ok) {
                const result = await res.json();
                emgData = result.emg_values || [];
            } else {
                console.error('Error API datos EMG:', res.status);
            }
        } catch (e) {
            console.error('Error cargando datos EMG:', e);
            emgData = JSON.parse(localStorage.getItem('emgData_' + sessionId)) || [];
        }

        if (emgData.length === 0) {
            app.chartEMG.data.labels = [];
            app.chartEMG.data.datasets[0].data = [];
            app.chartEMG.options.plugins.title.text = `Sin datos EMG para esta sesión (ID: ${sessionId})`;
            app.chartEMG.update();
            return;
        }

        const labels = emgData.map((_, i) => i + 1);
        const fecha = session ? new Date(session.fecha_inicio).toLocaleString('es-ES') : '';

        app.chartEMG.data.labels = labels;
        app.chartEMG.data.datasets[0].data = emgData;
        app.chartEMG.data.datasets[0].label = `Valores EMG (${emgData.length} muestras)`;
        app.chartEMG.data.datasets[0].pointRadius = emgData.length > 100 ? 1 : 3;
        app.chartEMG.data.datasets[0].pointHoverRadius = 5;
        app.chartEMG.data.datasets[0].showLine = true;
        app.chartEMG.data.datasets[0].fill = false;
        app.chartEMG.data.datasets[0].borderColor = '#198754';
        app.chartEMG.data.datasets[0].backgroundColor = 'rgba(25, 135, 84, 0.6)';
        app.chartEMG.data.datasets[0].tension = 0.1;
        app.chartEMG.options.plugins.title.text = `${persona.nombre} - ${fecha} (${emgData.length} valores, Media: ${(session?.emg_promedio || 0).toFixed(4)})`;

        app.chartEMG.options.scales.x = {
            type: 'linear',
            position: 'bottom',
            title: { display: true, text: 'Muestra #' },
            min: 1,
            max: emgData.length
        };
        app.chartEMG.options.scales.y.title.text = 'Valor EMG';
    }

    updateStats();
    app.chartEMG.update();
}

function updateSesionesSelect() {
    const select = document.getElementById('selectSesionGrafico');
    select.innerHTML = '<option value="">-- Primero selecciona persona --</option>';
    select.disabled = true;

    document.getElementById('selectPersonaGrafico').value = '';

    updatePersonasGraficoSelect();
}

function updatePersonasGraficoSelect() {
    const select = document.getElementById('selectPersonaGrafico');
    select.innerHTML = '<option value="">-- Seleccionar Persona --</option>';

    app.personas.forEach(p => {
        const option = document.createElement('option');
        option.value = p.id;
        option.textContent = p.nombre;
        select.appendChild(option);
    });
}

function updateSesionesSelectByPersona(personaId) {
    const select = document.getElementById('selectSesionGrafico');

    if (!personaId) {
        select.innerHTML = '<option value="">-- Primero selecciona persona --</option>';
        select.disabled = true;
        return;
    }

    select.disabled = false;
    select.innerHTML = '<option value="">Ver medias (o haz clic en el gráfico)</option>';

    const sesionesFiltradas = app.sesionesFinalizadas.filter(s => s.persona_id == personaId);

    if (sesionesFiltradas.length === 0) {
        select.innerHTML = '<option value="">Sin sesiones registradas</option>';
        return;
    }

    sesionesFiltradas.forEach(s => {
        const fecha = new Date(s.fecha_inicio).toLocaleDateString('es-ES', {
            day: '2-digit',
            month: '2-digit',
            year: 'numeric',
            hour: '2-digit',
            minute: '2-digit'
        });
        const option = document.createElement('option');
        option.value = s.id;
        option.textContent = `${fecha} - ${s.total_muestras || 0} muestras (Media: ${(s.emg_promedio || 0).toFixed(4)})`;
        select.appendChild(option);
    });
}

function agregarPersona(e) {
    e.preventDefault();

    const nombre = document.getElementById('inputNombre').value.trim();
    const email = document.getElementById('inputEmail').value.trim();
    const telefono = document.getElementById('inputTelefono').value.trim();
    const descripcion = document.getElementById('inputDescripcion').value.trim();

    if (!nombre) {
        showToast('Error', 'El nombre es requerido', 'danger');
        return;
    }

    if (typeof uibuilder !== 'undefined') {
        uibuilder.send({ topic: 'persona/new', payload: { nombre, email: email || null, telefono: telefono || null, descripcion: descripcion || null } });
    }

    e.target.reset();

    showToast('Persona Agregada', `${nombre} ha sido registrado`, 'success');
}

function renderPersonasSelect() {
    const select = document.getElementById('selectPersona');
    if (!select) {
        console.error('Element selectPersona not found');
        return;
    }

    select.innerHTML = '<option value="">-- Seleccionar --</option>';

    app.personas.forEach(p => {
        const option = document.createElement('option');
        option.value = p.id;
        option.textContent = p.nombre;
        select.appendChild(option);
    });
}

function renderSensoresSelect() {
    const select = document.getElementById('selectSensor');
    if (!select) {
        console.error('Element selectSensor not found');
        return;
    }

    const currentValue = select.value;

    select.innerHTML = '<option value="">-- Seleccionar --</option>';

    app.sensores.forEach(s => {
        const option = document.createElement('option');
        option.value = s.id || s.ip;

        let text = s.nombre || s.id || s.ip;
        if (s.ip && s.ip !== s.id) text += ` [${s.ip}]`;
        if (s.estado) text += ` - ${s.estado}`;
        option.textContent = text;

        if (s.estado === 'desconectado') {
            option.disabled = true;
        }

        select.appendChild(option);
    });

    if (currentValue && select.querySelector(`option[value="${currentValue}"]`)) {
        select.value = currentValue;
    }
}

function renderPersonas() {
    const container = document.getElementById('listaPersonas');
    const totalEl = document.getElementById('totalPersonas');

    totalEl.textContent = app.personas.length;

    if (app.personas.length === 0) {
        container.innerHTML = `
            <div class="list-group-item text-center text-muted py-4">
                <i class="bi bi-person-x fs-1 d-block mb-2"></i>
                No hay personas registradas
            </div>`;
        return;
    }

    container.innerHTML = app.personas.map(p => `
        <div class="list-group-item d-flex justify-content-between align-items-center">
            <div>
                <h6 class="mb-1">${p.nombre}</h6>
                <small class="text-muted">
                    ${p.email ? `<i class="bi bi-envelope me-1"></i>${p.email}` : ''}
                    ${p.telefono ? `<i class="bi bi-phone ms-2 me-1"></i>${p.telefono}` : ''}
                </small>
            </div>
            <button class="btn btn-outline-danger btn-sm" onclick="eliminarPersona(${p.id})">
                <i class="bi bi-trash"></i>
            </button>
        </div>
    `).join('');
}

function renderPersonasHistorialSelect() {
    const select = document.getElementById('selectPersonaHistorial');
    select.innerHTML = '<option value="">Todas las personas</option>';

    app.personas.forEach(p => {
        const option = document.createElement('option');
        option.value = p.id;
        option.textContent = p.nombre;
        select.appendChild(option);
    });
}

function eliminarPersona(id) {
    if (!confirm('¿Eliminar esta persona?')) return;

    if (typeof uibuilder !== 'undefined') {
        uibuilder.send({ topic: 'persona/delete', payload: { id: id } });
    }

    showToast('Persona Eliminada', 'La persona ha sido eliminada', 'warning');
}

function renderHistorial(personaId = '') {
    const tbody = document.getElementById('historialBody');

    let sesiones = app.sesionesFinalizadas;
    if (personaId) {
        sesiones = sesiones.filter(s => s.persona_id == personaId);
    }

    if (sesiones.length === 0) {
        tbody.innerHTML = `
            <tr>
                <td colspan="7" class="text-center text-muted py-4">
                    <i class="bi bi-inbox fs-1 d-block mb-2"></i>
                    ${personaId ? 'No hay sesiones para esta persona' : 'No hay sesiones registradas'}
                </td>
            </tr>`;
        return;
    }

    tbody.innerHTML = [...sesiones].reverse().map(s => `
        <tr>
            <td>${new Date(s.fecha_inicio).toLocaleString('es-ES')}</td>
            <td><strong>${s.persona_nombre}</strong></td>
            <td class="text-end">${s.total_muestras || 0}</td>
            <td class="text-end"><span class="badge bg-primary">${(s.emg_promedio || 0).toFixed(4)}</span></td>
            <td class="text-end"><span class="badge bg-success">${(s.emg_max || 0).toFixed(4)}</span></td>
            <td class="text-end"><span class="badge bg-warning text-dark">${(s.emg_min || 0).toFixed(4)}</span></td>
            <td class="text-center">
                <button class="btn btn-sm btn-outline-primary" onclick="verSesion(${s.id})">
                    <i class="bi bi-eye"></i>
                </button>
            </td>
        </tr>
    `).join('');
}

function verSesion(id) {
    const sesion = app.sesionesFinalizadas.find(s => s.id == id);
    if (!sesion) return;

    const personaId = sesion.persona_id;

    document.getElementById('selectPersonaGrafico').value = personaId;
    updateSesionesSelectByPersona(personaId);
    document.getElementById('selectSesionGrafico').value = id;
    updateChart(id, personaId);
    showPage('sesiones');
    document.querySelectorAll('.nav-link').forEach(l => l.classList.remove('active'));
    document.querySelector('[data-page="sesiones"]').classList.add('active');
}

function exportarCSV() {
    const personaId = document.getElementById('selectPersonaHistorial').value;

    let sesiones = app.sesionesFinalizadas;
    if (personaId) {
        sesiones = sesiones.filter(s => s.persona_id == personaId);
    }

    if (sesiones.length === 0) {
        showToast('Error', 'No hay datos para exportar', 'warning');
        return;
    }

    let csv = 'Fecha,Persona,Muestras,Promedio,Maximo,Minimo\n';
    sesiones.forEach(s => {
        csv += `"${s.fecha_inicio}","${s.persona_nombre}",${s.total_muestras || 0},${s.emg_promedio || 0},${s.emg_max || 0},${s.emg_min || 0}\n`;
    });

    const blob = new Blob([csv], { type: 'text/csv' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;

    const fecha = new Date().toISOString().slice(0, 10);
    const personaNombre = personaId ? app.personas.find(p => p.id == personaId)?.nombre.replace(/\s+/g, '_') : '';
    a.download = `EMG_Sesions${personaNombre}_${fecha}.csv`;

    a.click();
    URL.revokeObjectURL(url);

    const mensaje = personaId ?
        `${sesiones.length} sesiones de ${personaNombre} exportadas` :
        `${sesiones.length} sesiones exportadas`;
    showToast('Exportación Completa', mensaje, 'success');
}

async function loadLocalData() {
    try {
        const personasRes = await fetch('/api/personas');
        if (personasRes.ok) {
            app.personas = await personasRes.json();
            renderPersonasSelect();
            renderPersonas();
            renderPersonasHistorialSelect();
            updatePersonasGraficoSelect();
        }
    } catch (e) {}

    try {
        const sesionesRes = await fetch('/api/sesiones');
        if (sesionesRes.ok) {
            app.sesionesFinalizadas = await sesionesRes.json();
            updateSesionesSelect();
            renderHistorial();
            updateChart('');
        }
    } catch (e) {
        const sesiones = localStorage.getItem('sesionesFinalizadas');
        if (sesiones) {
            app.sesionesFinalizadas = JSON.parse(sesiones);
            updateSesionesSelect();
        }
    }
}

async function loadTestData() {
    try {
        const res = await fetch('/api/personas');
        if (res.ok) {
            await loadLocalData();
            updateConnectionStatus(true);
            return;
        }
    } catch (e) {}

    if (app.personas.length === 0) {
        app.personas = [
            { id: 1, nombre: 'Juan García', email: 'juan@email.com', telefono: '+34 612345678' },
            { id: 2, nombre: 'María López', email: 'maria@email.com', telefono: '+34 623456789' },
            { id: 3, nombre: 'Carlos Rodríguez', email: 'carlos@email.com', telefono: '+34 634567890' }
        ];
        localStorage.setItem('personas', JSON.stringify(app.personas));
    }

    if (app.sensores.length === 0) {
        app.sensores = [
            { id: 'ESP32_001', ip: '192.168.4.100', nombre: 'Sensor EMG Brazo', estado: 'conectado' },
            { id: 'ESP32_002', ip: '192.168.4.101', nombre: 'Sensor EMG Pierna', estado: 'conectado' },
            { id: 'ESP32_003', ip: '192.168.4.102', nombre: 'Sensor EMG Espalda', estado: 'desconectado' }
        ];
    }

    if (app.sesionesFinalizadas.length === 0) {
        app.sesionesFinalizadas = [
            { id: 1001, persona_nombre: 'Juan García', fecha_inicio: '2026-01-28T09:00:00Z', total_muestras: 150, emg_promedio: 0.1947, emg_max: 0.34, emg_min: 0.05 },
            { id: 1002, persona_nombre: 'Juan García', fecha_inicio: '2026-01-28T15:00:00Z', total_muestras: 120, emg_promedio: 0.2411, emg_max: 0.41, emg_min: 0.08 },
            { id: 1003, persona_nombre: 'María López', fecha_inicio: '2026-01-29T10:00:00Z', total_muestras: 100, emg_promedio: 0.1528, emg_max: 0.27, emg_min: 0.03 },
            { id: 1004, persona_nombre: 'María López', fecha_inicio: '2026-01-29T16:00:00Z', total_muestras: 180, emg_promedio: 0.3224, emg_max: 0.54, emg_min: 0.10 },
            { id: 1005, persona_nombre: 'Carlos Rodríguez', fecha_inicio: '2026-01-30T11:00:00Z', total_muestras: 80, emg_promedio: 0.1168, emg_max: 0.21, emg_min: 0.02 }
        ];
        localStorage.setItem('sesionesFinalizadas', JSON.stringify(app.sesionesFinalizadas));

        app.sesionesFinalizadas.forEach(s => {
            const datos = [];
            for (let i = 0; i < s.total_muestras; i++) {
                datos.push(s.emg_min + Math.random() * (s.emg_max - s.emg_min));
            }
            localStorage.setItem('emgData_' + s.id, JSON.stringify(datos));
        });
    }

    renderPersonasSelect();
    renderPersonas();
    renderPersonasHistorialSelect();
    renderSensoresSelect();
    renderHistorial();
    updateSesionesSelect();
    updateChart('');

    updateConnectionStatus(true);
}

function showToast(title, message, type = 'info') {
    const toast = document.getElementById('toastNotification');
    const titleEl = document.getElementById('toastTitle');
    const messageEl = document.getElementById('toastMessage');

    titleEl.textContent = title;
    messageEl.textContent = message;

    toast.className = 'toast';
    if (type === 'success') toast.classList.add('bg-success', 'text-white');
    else if (type === 'danger') toast.classList.add('bg-danger', 'text-white');
    else if (type === 'warning') toast.classList.add('bg-warning');
    else toast.classList.add('bg-info', 'text-white');

    const bsToast = new bootstrap.Toast(toast);
    bsToast.show();
}

function initModal() {
    app.modalSesion = new bootstrap.Modal(document.getElementById('modalSesion'));
    app.modalCalibracion = new bootstrap.Modal(document.getElementById('modalCalibracion'));

    document.getElementById('modalCalibracion').addEventListener('hidden.bs.modal', function () {
        stopCalibrationPolling();
    });
}

function abrirModalSesion() {
    const personaId = document.getElementById('selectPersona').value;
    const sensorId = document.getElementById('selectSensor').value;

    if (!personaId || !sensorId) {
        showToast('Error', 'Selecciona una persona y un sensor antes de abrir el control', 'danger');
        return;
    }

    if (app.sesionActiva) {
        document.getElementById('modalAlertEstado').className = 'alert alert-success';
        document.getElementById('modalEstadoTexto').innerHTML = '<strong>SESIÓN ACTIVA</strong> - Capturando datos';

        document.getElementById('btnModalIniciar').disabled = true;
        document.getElementById('btnModalDetener').disabled = false;
    } else {
        document.getElementById('modalAlertEstado').className = 'alert alert-warning';
        document.getElementById('modalEstadoTexto').textContent = 'Sin sesión activa';

        document.getElementById('btnModalIniciar').disabled = false;
        document.getElementById('btnModalDetener').disabled = true;

        clearModalChart();
    }

    app.modalSesion.show();
}

function initModalChart() {
    const ctx = document.getElementById('chartModalRealTime').getContext('2d');

    app.chartModalRealTime = new Chart(ctx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: 'Valor EMG',
                data: [],
                borderColor: '#38a169',
                backgroundColor: 'rgba(56, 161, 105, 0.2)',
                borderWidth: 3,
                pointRadius: 0,
                pointHoverRadius: 5,
                tension: 0.4,
                fill: true
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            animation: {
                duration: 0
            },
            plugins: {
                legend: {
                    display: true,
                    position: 'top',
                    labels: {
                        color: '#cbd5e0',
                        font: {
                            size: 12,
                            weight: 'bold'
                        }
                    }
                },
                title: {
                    display: true,
                    text: 'Esperando sesión activa...',
                    color: '#e2e8f0',
                    font: { size: 14, weight: 'bold' }
                }
            },
            scales: {
                y: {
                    beginAtZero: true,
                    title: {
                        display: true,
                        text: 'Valor EMG',
                        color: '#cbd5e0',
                        font: { size: 12, weight: 'bold' }
                    },
                    ticks: {
                        color: '#a0aec0'
                    },
                    grid: {
                        color: 'rgba(255, 255, 255, 0.1)'
                    }
                },
                x: {
                    display: false,
                    grid: {
                        display: false
                    }
                }
            }
        }
    });
}

function calculateRMS(values) {
    if (values.length === 0) return 0;
    const sum = values.reduce((acc, val) => acc + (val * val), 0);
    return Math.sqrt(sum / values.length);
}

function calculateStdDev(values) {
    if (values.length === 0) return 0;
    const mean = values.reduce((acc, val) => acc + val, 0) / values.length;
    const variance = values.reduce((acc, val) => acc + Math.pow(val - mean, 2), 0) / values.length;
    return Math.sqrt(variance);
}

function analyzeFatigue(emgValue) {
    const fatiga = app.fatigaData;

    if (!fatiga.baselineSet) {
        fatiga.baseline.push(emgValue);

        if (fatiga.baseline.length >= fatiga.windowSize) {
            fatiga.rmsBaseline = calculateRMS(fatiga.baseline);
            fatiga.baselineSet = true;
        }

        fatiga.fatigaIndex = 0;
        fatiga.trend = 'estable';
        fatiga.debugInfo = {
            emgValue: emgValue,
            rmsActual: 0,
            stdDevActual: 0,
            rmsIncrease: 0,
            variabilityFactor: 0
        };
        updateFatigaUI();
        return;
    }

    fatiga.currentWindow.push(emgValue);

    if (fatiga.currentWindow.length > fatiga.windowSize) {
        fatiga.currentWindow.shift();
    }

    const rmsActual = calculateRMS(fatiga.currentWindow);
    const stdDevActual = calculateStdDev(fatiga.currentWindow);

    const rmsChange = Math.abs(rmsActual - fatiga.rmsBaseline);
    const rmsChangePercent = (rmsChange / fatiga.rmsBaseline) * 100;
    const variabilityFactor = stdDevActual / (fatiga.rmsBaseline + 1);

    let fatigaIndex = 0;
    fatigaIndex += Math.min(rmsChangePercent * 1.5, 50);
    fatigaIndex += Math.min(variabilityFactor * 60, 50);

    const stdDevPercent = (stdDevActual / fatiga.rmsBaseline) * 100;
    if (stdDevPercent > 30) {
        fatigaIndex += 20;
    }

    fatigaIndex = Math.max(0, Math.min(100, fatigaIndex));

    let trend = 'estable';
    if (fatigaIndex >= 60) trend = 'alta';
    else if (fatigaIndex >= 30) trend = 'incrementando';

    fatiga.fatigaIndex = fatigaIndex;
    fatiga.trend = trend;
    fatiga.debugInfo = {
        emgValue: emgValue,
        rmsActual: rmsActual,
        stdDevActual: stdDevActual,
        rmsIncrease: rmsChangePercent,
        variabilityFactor: variabilityFactor,
        stdDevPercent: stdDevPercent
    };

    updateFatigaUI();
}

function updateFatigaUI() {
    const fatiga = app.fatigaData;
    const indicator = document.getElementById('fatigaIndicator');
    const bar = document.getElementById('fatigaBar');
    const text = document.getElementById('fatigaText');
    const status = document.getElementById('fatigaStatus');

    if (!indicator || !bar || !text || !status) return;

    if (!fatiga.baselineSet) {
        indicator.style.display = 'none';
        return;
    }

    indicator.style.display = 'block';

    bar.style.width = `${fatiga.fatigaIndex}%`;

    if (fatiga.fatigaIndex < 30) {
        bar.className = 'progress-bar bg-success';
        status.textContent = '✓ Señal estable';
        status.className = 'badge bg-success';
    } else if (fatiga.fatigaIndex < 60) {
        bar.className = 'progress-bar bg-warning';
        status.textContent = '⚠ Señal inestable';
        status.className = 'badge bg-warning';
    } else {
        bar.className = 'progress-bar bg-danger';
        status.textContent = '⚡ Señal muy inestable';
        status.className = 'badge bg-danger';
    }

    text.textContent = `${fatiga.fatigaIndex.toFixed(0)}%`;

    checkAdaptation(fatiga.fatigaIndex);

    if (fatiga.debugInfo) {
        const debug = fatiga.debugInfo;

        const baselineEl = document.getElementById('debugBaseline');
        if (baselineEl) baselineEl.textContent = fatiga.rmsBaseline.toFixed(2);

        const rmsActualEl = document.getElementById('debugRMSActual');
        if (rmsActualEl) rmsActualEl.textContent = debug.rmsActual.toFixed(2);

        const rmsIncreaseEl = document.getElementById('debugRMSIncrease');
        if (rmsIncreaseEl) {
            rmsIncreaseEl.textContent = `${debug.rmsIncrease.toFixed(1)}%`;
            rmsIncreaseEl.className = debug.rmsIncrease > 10 ? 'text-warning' : 'text-success';
        }

        const stdDevEl = document.getElementById('debugStdDev');
        if (stdDevEl) stdDevEl.textContent = debug.stdDevActual.toFixed(2);

        const emgValueEl = document.getElementById('debugEMGValue');
        if (emgValueEl) emgValueEl.textContent = debug.emgValue.toFixed(0);

        const variabilityEl = document.getElementById('debugVariability');
        if (variabilityEl) {
            variabilityEl.textContent = debug.variabilityFactor.toFixed(3);
            variabilityEl.className = debug.variabilityFactor > 0.5 ? 'text-warning' : 'text-success';
        }
    }
}

function resetFatigaDetection() {
    app.fatigaData = {
        baseline: [],
        baselineSet: false,
        rmsBaseline: 0,
        windowSize: 20,
        currentWindow: [],
        fatigaIndex: 0,
        trend: 'estable'
    };

    const indicator = document.getElementById('fatigaIndicator');
    if (indicator) {
        indicator.style.display = 'none';
    }
}

function updateModalChart(emgValue) {
    if (!app.chartModalRealTime) return;

    analyzeFatigue(emgValue);

    app.modalRealtimeData.push(emgValue);

    if (app.modalRealtimeData.length > app.maxRealtimePoints) {
        app.modalRealtimeData.shift();
    }

    app.chartModalRealTime.data.labels = app.modalRealtimeData.map((_, i) => i + 1);
    app.chartModalRealTime.data.datasets[0].data = app.modalRealtimeData;
    app.chartModalRealTime.options.plugins.title.text = `Capturando (${app.sesionActiva ? app.sesionActiva.muestras : 0} muestras)`;
    app.chartModalRealTime.update('none');
}

function clearModalChart() {
    if (!app.chartModalRealTime) return;

    app.modalRealtimeData = [];
    app.chartModalRealTime.data.labels = [];
    app.chartModalRealTime.data.datasets[0].data = [];
    app.chartModalRealTime.options.plugins.title.text = 'Esperando sesión activa...';
    app.chartModalRealTime.update();
}

function abrirModalCalibracion() {
    const sensorId = document.getElementById('selectSensor').value;

    if (!sensorId) {
        showToast('Error', 'Selecciona un sensor antes de calibrar', 'danger');
        return;
    }

    app.sensorActual = app.sensores.find(s => (s.id || s.ip) === sensorId);

    if (!app.sensorActual || !app.sensorActual.ip) {
        showToast('Error', 'Sensor sin IP válida', 'danger');
        return;
    }

    resetCalibrationUI();
    app.modalCalibracion.show();
}

function resetCalibrationUI() {
    document.getElementById('calibrationAlert').className = 'alert alert-info mb-4';
    document.getElementById('calibrationPhaseTitle').textContent = 'Preparando calibración...';
    document.getElementById('calibrationPhaseDescription').textContent = 'Siga las instrucciones cuidadosamente para una calibración precisa.';
    document.getElementById('calibrationTimer').textContent = '--';
    document.getElementById('calibrationProgress').style.width = '0%';
    document.getElementById('calibrationProgressText').textContent = '0%';
    document.getElementById('calibrationIcon').className = 'bi bi-person-arms-up display-1 mb-3';
    document.getElementById('calibrationInstruction').textContent = 'Esperando inicio...';
    document.getElementById('calibrationResults').classList.add('d-none');
    document.getElementById('btnStartCalibration').classList.remove('d-none');
    document.getElementById('btnCancelCalibration').classList.add('d-none');
    document.getElementById('btnCloseCalibration').disabled = false;
}

function iniciarCalibracion() {
    if (!app.sensorActual) return;

    const targetIp = app.sensorActual.ip;
    const message = `${targetIp}:Calibrate`;

    if (typeof uibuilder !== 'undefined') {
        uibuilder.send({ topic: 'esp32/commands', payload: message });
    }

    app.calibracionActiva = true;

    document.getElementById('btnStartCalibration').classList.add('d-none');
    document.getElementById('btnCancelCalibration').classList.remove('d-none');
    document.getElementById('btnCloseCalibration').disabled = true;

    startCalibrationPolling();

    showToast('Calibración Iniciada', 'Siga las instrucciones en pantalla', 'info');
}

function startCalibrationPolling() {
    app.calibracionInterval = setInterval(() => {
        if (!app.calibracionActiva || !app.sensorActual) {
            stopCalibrationPolling();
            return;
        }

        const targetIp = app.sensorActual.ip;
        const message = `${targetIp}:CalibrationStatus`;

        if (typeof uibuilder !== 'undefined') {
            uibuilder.send({
                topic: 'esp32/commands',
                payload: message
            });
        }
    }, 500);
}

function stopCalibrationPolling() {
    if (app.calibracionInterval) {
        clearInterval(app.calibracionInterval);
        app.calibracionInterval = null;
    }
    app.calibracionActiva = false;
}

function updateCalibrationUI(status) {
    if (!status || !app.calibracionActiva) return;

    const { calibrated, calibrating, phase, baseline, maxValue, remainingMs, phaseDescription } = status;

    // Actualizar fase
    if (calibrating) {
        if (phase === 0) {
            // Fase de reposo
            document.getElementById('calibrationAlert').className = 'alert alert-warning mb-4';
            document.getElementById('calibrationPhaseTitle').textContent = 'FASE 1: REPOSO';
            document.getElementById('calibrationPhaseDescription').textContent = phaseDescription || 'Relaje completamente el músculo';
            document.getElementById('calibrationIcon').className = 'bi bi-moon-stars display-1 mb-3';
            document.getElementById('calibrationInstruction').textContent = 'Relájese completamente';

            // Progreso: 0-67% (10 segundos de 15 totales)
            const progress = ((10000 - remainingMs) / 10000) * 67;
            document.getElementById('calibrationProgress').style.width = `${progress}%`;
            document.getElementById('calibrationProgressText').textContent = `${Math.floor(progress)}%`;
            document.getElementById('calibrationProgress').className = 'progress-bar progress-bar-striped progress-bar-animated bg-warning';
        } else if (phase === 1) {
            // Fase de contracción máxima
            document.getElementById('calibrationAlert').className = 'alert alert-danger mb-4';
            document.getElementById('calibrationPhaseTitle').textContent = 'FASE 2: CONTRACCIÓN MÁXIMA';
            document.getElementById('calibrationPhaseDescription').textContent = phaseDescription || 'Contraiga el músculo al MÁXIMO';
            document.getElementById('calibrationIcon').className = 'bi bi-lightning-charge display-1 mb-3';
            document.getElementById('calibrationInstruction').textContent = '¡CONTRAIGA AL MÁXIMO!';

            // Progreso: 67-100% (5 segundos finales)
            const progress = 67 + ((5000 - remainingMs) / 5000) * 33;
            document.getElementById('calibrationProgress').style.width = `${progress}%`;
            document.getElementById('calibrationProgressText').textContent = `${Math.floor(progress)}%`;
            document.getElementById('calibrationProgress').className = 'progress-bar progress-bar-striped progress-bar-animated bg-danger';
        }

        // Actualizar timer
        const seconds = Math.ceil(remainingMs / 1000);
        document.getElementById('calibrationTimer').textContent = `${seconds}s`;
    } else if (calibrated) {
        // Calibración completada
        stopCalibrationPolling();

        document.getElementById('calibrationAlert').className = 'alert alert-success mb-4';
        document.getElementById('calibrationPhaseTitle').textContent = '✅ CALIBRACIÓN EXITOSA';
        document.getElementById('calibrationPhaseDescription').textContent = 'El sensor ha sido calibrado correctamente.';
        document.getElementById('calibrationIcon').className = 'bi bi-check-circle display-1 mb-3 text-success';
        document.getElementById('calibrationInstruction').textContent = '✓ Calibración completada';
        document.getElementById('calibrationProgress').style.width = '100%';
        document.getElementById('calibrationProgressText').textContent = '100%';
        document.getElementById('calibrationProgress').className = 'progress-bar bg-success';
        document.getElementById('calibrationTimer').textContent = 'Completo';

        // Mostrar resultados
        document.getElementById('calibrationBaseline').textContent = baseline.toFixed(2);
        document.getElementById('calibrationMax').textContent = maxValue.toFixed(2);
        document.getElementById('calibrationResults').classList.remove('d-none');

        // Habilitar cierre
        document.getElementById('btnCancelCalibration').classList.add('d-none');
        document.getElementById('btnCloseCalibration').disabled = false;

        showToast('Calibración Exitosa', 'Puede cerrar el modal y comenzar a capturar datos', 'success');
    } else if (!calibrating && !calibrated) {
        // Calibración fallida
        stopCalibrationPolling();

        document.getElementById('calibrationAlert').className = 'alert alert-danger mb-4';
        document.getElementById('calibrationPhaseTitle').textContent = '❌ CALIBRACIÓN FALLIDA';
        document.getElementById('calibrationPhaseDescription').textContent = 'No se detectó suficiente diferencia entre reposo y contracción.';
        document.getElementById('calibrationIcon').className = 'bi bi-x-circle display-1 mb-3 text-danger';
        document.getElementById('calibrationInstruction').textContent = 'Intente nuevamente';

        document.getElementById('btnCancelCalibration').classList.add('d-none');
        document.getElementById('btnStartCalibration').classList.remove('d-none');
        document.getElementById('btnCloseCalibration').disabled = false;

        showToast('Calibración Fallida', 'Asegúrese de contraer al máximo en la fase 2', 'danger');
    }
}

function cancelarCalibracion() {
    stopCalibrationPolling();

    if (app.sensorActual) {
        const message = `${app.sensorActual.ip}:CancelCalibration`;
        if (typeof uibuilder !== 'undefined') {
            uibuilder.send({ topic: 'esp32/commands', payload: message });
        }
    }

    app.modalCalibracion.hide();
    showToast('Calibración Cancelada', '', 'warning');
}

const ejercicioMode = {
    mode: 'adaptativo',
    lastAdaptation: 0,
    COOLDOWN: 15000,
    HIGH_THRESHOLD: 65,
    adaptationCount: 0,
    effectiveReps: 0,
    effectiveTime: 0,
    REPS_STEP: 1,
    TIME_STEP: 15,
    MIN_REPS: 3,
};

function initModoEjercicio() {
    document.querySelectorAll('input[name="modoEjercicio"]').forEach(radio => {
        radio.addEventListener('change', e => {
            ejercicioMode.mode = e.target.value;
            if (ejercicioMode.mode === 'estricto') {
                showToast('Modo Estricto', 'Reps y tiempo fijos durante toda la sesión', 'info');
            } else {
                showToast('Modo Adaptativo', 'Los objetivos se ajustarán según la fatiga muscular detectada', 'info');
            }
        });
    });
}

function checkAdaptation(fatigaIndex) {
    if (ejercicioMode.mode !== 'adaptativo') return;
    if (!app.sesionActiva) return;

    const now = Date.now();
    if (now - ejercicioMode.lastAdaptation < ejercicioMode.COOLDOWN) return;
    if (fatigaIndex < ejercicioMode.HIGH_THRESHOLD) return;

    ejercicioMode.lastAdaptation = now;
    ejercicioMode.adaptationCount++;

    const cambios = [];

    if (ejercicioMode.effectiveReps > 0) {
        const prev = ejercicioMode.effectiveReps;
        ejercicioMode.effectiveReps = Math.max(ejercicioMode.MIN_REPS, prev - ejercicioMode.REPS_STEP);
        if (ejercicioMode.effectiveReps !== prev) {
            cambios.push(`Reps ${prev} → ${ejercicioMode.effectiveReps}`);
            syncSelectToValue(document.getElementById('targetReps'), ejercicioMode.effectiveReps, 'down');
        }
    }

    if (ejercicioMode.effectiveTime > 0) {
        const prev = ejercicioMode.effectiveTime;
        ejercicioMode.effectiveTime = prev + ejercicioMode.TIME_STEP;
        cambios.push(`Tiempo ${formatTime(prev)} → ${formatTime(ejercicioMode.effectiveTime)}`);
        syncSelectToValue(document.getElementById('targetTime'), ejercicioMode.effectiveTime, 'up');
    }

    if (cambios.length === 0) return;

    updateRepsGoalDisplay();
    flashTimerValues();

    const logEl  = document.getElementById('adaptacionLog');
    const textEl = document.getElementById('adaptacionLogText');
    const badge  = document.getElementById('adaptacionBadge');
    if (logEl && textEl && badge) {
        logEl.classList.remove('d-none');
        textEl.textContent = `#${ejercicioMode.adaptationCount} — Fatiga ${fatigaIndex.toFixed(0)}%: ${cambios.join(' | ')}`;
        badge.textContent  = ejercicioMode.adaptationCount;
    }

    showToast(`Adaptación #${ejercicioMode.adaptationCount}`,
        `Fatiga alta (${fatigaIndex.toFixed(0)}%). ${cambios.join(' | ')}`, 'warning');
}

function syncSelectToValue(selectEl, targetValue, direction) {
    if (!selectEl) return;
    const values = Array.from(selectEl.options)
        .map(o => parseInt(o.value))
        .filter(v => v > 0)
        .sort((a, b) => a - b);

    let chosen;
    if (direction === 'down') {
        chosen = [...values].reverse().find(v => v <= targetValue) ?? values[0];
    } else {
        chosen = values.find(v => v >= targetValue) ?? values[values.length - 1];
    }
    selectEl.value = chosen;
}

function resetAdaptaciones() {
    ejercicioMode.lastAdaptation = 0;
    ejercicioMode.adaptationCount = 0;
    ejercicioMode.effectiveReps = 0;
    ejercicioMode.effectiveTime = 0;
    const logEl = document.getElementById('adaptacionLog');
    if (logEl) logEl.classList.add('d-none');
}

const sessionTimer = {
    interval: null,
    startTime: null
};

function startSessionTimer() {
    sessionTimer.startTime = Date.now();

    const timerRow = document.getElementById('sessionTimerRow');
    timerRow.classList.remove('d-none');

    updateCountdownDisplay();
    updateRepsGoalDisplay();

    if (sessionTimer.interval) clearInterval(sessionTimer.interval);
    sessionTimer.interval = setInterval(tickTimer, 1000);
    tickTimer();
}

function stopSessionTimer() {
    if (sessionTimer.interval) {
        clearInterval(sessionTimer.interval);
        sessionTimer.interval = null;
    }
    document.getElementById('sessionTimerRow').classList.add('d-none');
    document.getElementById('sessionTimerDisplay').textContent = '00:00';
    document.getElementById('sessionCountdown').textContent = '--:--';
    document.getElementById('sessionCountdown').className = 'session-timer font-monospace';
    document.getElementById('sessionCountdown').classList.remove('timer-warning');
}

function tickTimer() {
    const elapsed = Math.floor((Date.now() - sessionTimer.startTime) / 1000);
    document.getElementById('sessionTimerDisplay').textContent = formatTime(elapsed);

    const targetSecs  = parseInt(document.getElementById('targetTime')?.value || '0');
    const countdownEl = document.getElementById('sessionCountdown');

    if (targetSecs > 0) {
        const remaining = targetSecs - elapsed;
        if (remaining <= 0) {
            countdownEl.textContent = '00:00';
            setCountdownColor(countdownEl, 'danger');
            if (remaining === 0) {
                showToast('¡Tiempo completado!', `Objetivo de ${formatTime(targetSecs)} alcanzado`, 'success');
            }
        } else {
            countdownEl.textContent = formatTime(remaining);
            if (remaining <= 10) {
                setCountdownColor(countdownEl, 'danger');
            } else if (remaining <= 30) {
                setCountdownColor(countdownEl, 'warning');
            } else {
                setCountdownColor(countdownEl, 'normal');
            }
        }
    } else {
        countdownEl.textContent = '--:--';
        setCountdownColor(countdownEl, 'none');
    }
}

function setCountdownColor(el, state) {
    // Mientras el flash esté activo no tocamos los colores
    if (el.dataset.flashing) return;

    el.classList.remove('timer-normal', 'timer-caution', 'timer-urgent', 'timer-warning');
    el.style.color = ''; // limpiar cualquier inline residual

    const map = { danger: 'timer-urgent', warning: 'timer-caution', normal: 'timer-normal', none: 'timer-normal' };
    el.classList.add(map[state] ?? 'timer-normal');
    if (state === 'danger') el.classList.add('timer-warning'); // mantiene el pulso al llegar a 0
}

function updateCountdownDisplay() {
    // Si hay sesión activa, muestra el tiempo restante real; si no, el objetivo completo
    const targetSecs  = parseInt(document.getElementById('targetTime')?.value || '0');
    const countdownEl = document.getElementById('sessionCountdown');
    if (targetSecs <= 0) {
        countdownEl.textContent = '--:--';
        countdownEl.className = 'fw-bold font-monospace';
        return;
    }
    if (sessionTimer.startTime) {
        const elapsed   = Math.floor((Date.now() - sessionTimer.startTime) / 1000);
        const remaining = Math.max(0, targetSecs - elapsed);
        countdownEl.textContent = formatTime(remaining);
    } else {
        countdownEl.textContent = formatTime(targetSecs);
    }
    countdownEl.className = 'fw-bold font-monospace text-light';
}

function flashTimerValues() {
    ['sessionCountdown', 'repsGoalDisplay'].forEach(id => {
        const el = document.getElementById(id);
        if (!el) return;

        el.dataset.flashing = '1';

        anime({
            targets: el,
            color: ['#e2e8f0', '#f59e0b', '#fbbf24', '#e2e8f0'],
            duration: 700,
            easing: 'easeInOutSine',
            complete: () => {
                el.style.color = ''; // limpiar inline para que las clases CSS tomen el control
                delete el.dataset.flashing;
            }
        });
    });
}

function updateRepsGoalDisplay() {
    const targetReps = parseInt(document.getElementById('targetReps')?.value || '0');
    const container  = document.getElementById('repsGoalContainer');
    const divider    = document.getElementById('repsGoalDivider');
    if (targetReps > 0) {
        container.classList.remove('d-none');
        if (divider) divider.classList.remove('d-none');
        document.getElementById('repsGoalDisplay').textContent = `${cvState.repCount}/${targetReps}`;
    } else {
        container.classList.add('d-none');
        if (divider) divider.classList.add('d-none');
    }
}

function formatTime(seconds) {
    const m = Math.floor(seconds / 60).toString().padStart(2, '0');
    const s = (seconds % 60).toString().padStart(2, '0');
    return `${m}:${s}`;
}

const webcam = {
    stream: null,
    flipped: false
};

function initWebcam() {
    document.getElementById('btnActivarWebcam').addEventListener('click', startWebcam);
    document.getElementById('btnDesactivarWebcam').addEventListener('click', stopWebcam);
    document.getElementById('btnFlipWebcam').addEventListener('click', flipWebcam);
    document.getElementById('modalSesion').addEventListener('hidden.bs.modal', stopWebcam);
}

async function startWebcam() {
    if (!navigator.mediaDevices || !navigator.mediaDevices.getUserMedia) {
        showToast('Webcam no disponible', 'Tu navegador no soporta acceso a la cámara', 'danger');
        return;
    }
    try {
        webcam.stream = await navigator.mediaDevices.getUserMedia({ video: true, audio: false });
        const video = document.getElementById('webcamVideo');
        video.srcObject = webcam.stream;

        document.getElementById('webcamPlaceholder').classList.add('d-none');
        document.getElementById('webcamWrapper').classList.remove('d-none');
        const controls = document.getElementById('webcamControls');
        controls.classList.remove('d-none');
        controls.classList.add('d-flex');
    } catch (err) {
        const msg = err.name === 'NotAllowedError'
            ? 'Permiso denegado. Permite el acceso a la cámara en el navegador.'
            : 'No se pudo acceder a la cámara: ' + err.message;
        showToast('Error de cámara', msg, 'danger');
    }
}

function stopWebcam() {
    stopTracking();

    if (webcam.stream) {
        webcam.stream.getTracks().forEach(t => t.stop());
        webcam.stream = null;
    }

    const video = document.getElementById('webcamVideo');
    if (video) { video.srcObject = null; }

    webcam.flipped = false;
    document.getElementById('webcamWrapper').classList.add('d-none');
    document.getElementById('webcamPlaceholder').classList.remove('d-none');

    const controls = document.getElementById('webcamControls');
    controls.classList.add('d-none');
    controls.classList.remove('d-flex');
}

function flipWebcam() {
    webcam.flipped = !webcam.flipped;
    document.getElementById('webcamVideo').classList.toggle('flipped', webcam.flipped);
    document.getElementById('cvCanvas').classList.toggle('flipped', webcam.flipped);
}

const cvState = {
    pose: null,
    tracking: false,
    animFrame: null,
    targetY: null,       // 0-1 normalizado (null = no fijado)
    repCount: 0,
    repState: 'below',   // 'above' | 'below'
    landmarkIdx: 16,     // índice MediaPipe Pose
    lastY: null,
    HYSTERESIS: 0.04     // margen para evitar doble conteo
};

function initCV() {
    document.getElementById('btnIniciarTracking').addEventListener('click', startTracking);
    document.getElementById('btnPararTracking').addEventListener('click', stopTracking);
    document.getElementById('btnFijarObjetivo').addEventListener('click', setTargetFromLandmark);
    document.getElementById('btnResetReps').addEventListener('click', resetReps);
    document.getElementById('selectLandmark').addEventListener('change', e => {
        cvState.landmarkIdx = parseInt(e.target.value);
    });
    document.getElementById('cvCanvas').addEventListener('click', setTargetFromClick);
}

async function startTracking() {
    if (!webcam.stream) {
        showToast('Cámara requerida', 'Activa la cámara antes de iniciar el tracking', 'warning');
        return;
    }

    if (!cvState.pose) {
        showToast('Cargando modelo...', 'Inicializando MediaPipe Pose...', 'info');
        try {
            await loadPoseModel();
        } catch (e) {
            console.error('[CV] Error cargando modelo:', e);
            showToast('Error al cargar modelo', String(e.message || e), 'danger');
            return;
        }
    }

    cvState.tracking = true;
    cvState.repCount = 0;
    cvState.repState = 'below';
    cvState.targetY = null;
    cvState.lastY = null;
    updateRepDisplay();

    document.getElementById('cvPanel').classList.remove('d-none');
    document.getElementById('btnIniciarTracking').classList.add('d-none');

    runTrackingLoop();
}

function stopTracking() {
    if (!cvState.tracking && cvState.animFrame === null) return;

    cvState.tracking = false;
    if (cvState.animFrame) {
        cancelAnimationFrame(cvState.animFrame);
        cvState.animFrame = null;
    }

    const canvas = document.getElementById('cvCanvas');
    if (canvas) {
        canvas.getContext('2d').clearRect(0, 0, canvas.width, canvas.height);
    }

    document.getElementById('cvPanel').classList.add('d-none');
    document.getElementById('btnIniciarTracking').classList.remove('d-none');
}

function loadPoseModel() {
    return new Promise((resolve, reject) => {
        if (typeof Pose === 'undefined') {
            reject(new Error('El script pose.js no está cargado'));
            return;
        }
        const base = window.location.href.replace(/\/[^/]*$/, '/');
        const pose = new Pose({
            locateFile: file => {
                const safeFile = file.replace('simd_wasm_bin', 'wasm_bin');
                return `${base}mediapipe/${safeFile}`;
            }
        });
        pose.setOptions({
            modelComplexity: 1,
            smoothLandmarks: true,
            enableSegmentation: false,
            minDetectionConfidence: 0.5,
            minTrackingConfidence: 0.5
        });
        pose.onResults(processResults);
        pose.initialize().then(() => {
            cvState.pose = pose;
            resolve();
        }).catch(reject);
    });
}

function runTrackingLoop() {
    if (!cvState.tracking) return;

    const video = document.getElementById('webcamVideo');
    const canvas = document.getElementById('cvCanvas');

    if (video && canvas && cvState.pose && video.readyState >= 2) {
        canvas.width = video.videoWidth;
        canvas.height = video.videoHeight;
        cvState.pose.send({ image: video });
    }

    cvState.animFrame = requestAnimationFrame(runTrackingLoop);
}

function processResults(results) {
    const canvas = document.getElementById('cvCanvas');
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    if (!results.poseLandmarks) return;

    const lm = results.poseLandmarks[cvState.landmarkIdx];
    if (!lm || lm.visibility < 0.4) return;

    const x = lm.x * canvas.width;
    const y = lm.y * canvas.height;

    ctx.beginPath();
    ctx.arc(x, y, 12, 0, 2 * Math.PI);
    ctx.fillStyle = 'rgba(59, 130, 246, 0.85)';
    ctx.fill();
    ctx.strokeStyle = '#fff';
    ctx.lineWidth = 2.5;
    ctx.stroke();

    if (cvState.targetY !== null) {
        const targetPx = cvState.targetY * canvas.height;
        const reached = lm.y < cvState.targetY;

        ctx.setLineDash([12, 6]);
        ctx.beginPath();
        ctx.moveTo(0, targetPx);
        ctx.lineTo(canvas.width, targetPx);
        ctx.strokeStyle = reached ? 'rgba(74, 222, 128, 0.9)' : 'rgba(250, 204, 21, 0.9)';
        ctx.lineWidth = 3;
        ctx.stroke();
        ctx.setLineDash([]);

        ctx.fillStyle = reached ? 'rgba(74, 222, 128, 0.9)' : 'rgba(250, 204, 21, 0.9)';
        ctx.font = 'bold 13px Inter, sans-serif';
        ctx.fillText('OBJETIVO', 8, targetPx - 7);

        countRep(lm.y);
    }

    cvState.lastY = lm.y;
}

function countRep(currentY) {
    const { targetY, repState, HYSTERESIS } = cvState;
    if (targetY === null) return;

    if (repState === 'below' && currentY < targetY) {
        cvState.repCount++;
        cvState.repState = 'above';
        updateRepDisplay();
        flashRepCounter();

        const targetReps = parseInt(document.getElementById('targetReps')?.value || '0');
        if (targetReps > 0) {
            // Actualizar display reps en el timer
            document.getElementById('repsGoalDisplay').textContent = `${cvState.repCount}/${targetReps}`;

            // Actualizar barra de progreso
            const pct = Math.min(100, (cvState.repCount / targetReps) * 100);
            document.getElementById('repsProgressBar').classList.remove('d-none');
            document.getElementById('repsProgressFill').style.width = `${pct}%`;

            // Objetivo alcanzado
            if (cvState.repCount >= targetReps) {
                showToast('¡Objetivo alcanzado!', `Has completado ${cvState.repCount} repeticiones`, 'success');
                document.getElementById('repsProgressFill').classList.remove('bg-info');
                document.getElementById('repsProgressFill').classList.add('bg-success');
            }
        }
    } else if (repState === 'above' && currentY > targetY + HYSTERESIS) {
        cvState.repState = 'below';
    }
}

function setTargetFromLandmark() {
    if (cvState.lastY === null) {
        showToast('Sin detección', 'El cuerpo no es visible en la cámara', 'warning');
        return;
    }
    cvState.targetY = cvState.lastY;
    cvState.repState = 'below';
    showToast('Objetivo fijado', 'Sube el punto por encima de la línea amarilla para contar repeticiones', 'success');
}

function setTargetFromClick(e) {
    if (!cvState.tracking) return;
    const canvas = document.getElementById('cvCanvas');
    const rect = canvas.getBoundingClientRect();
    cvState.targetY = (e.clientY - rect.top) / rect.height;
    cvState.repState = 'below';
    showToast('Objetivo fijado', 'Sube el punto por encima de la línea amarilla para contar', 'success');
}

function resetReps() {
    cvState.repCount = 0;
    cvState.repState = 'below';
    cvState.targetY = null;
    updateRepDisplay();

    const bar = document.getElementById('repsProgressBar');
    if (bar) bar.classList.add('d-none');
    const fill = document.getElementById('repsProgressFill');
    if (fill) { fill.style.width = '0%'; fill.className = 'progress-bar bg-info'; }
    const goal = document.getElementById('repsGoalDisplay');
    const targetReps = parseInt(document.getElementById('targetReps')?.value || '0');
    if (goal && targetReps > 0) goal.textContent = `0/${targetReps}`;
}

function updateRepDisplay() {
    const el = document.getElementById('repCount');
    if (el) el.textContent = cvState.repCount;
}

function flashRepCounter() {
    const el = document.getElementById('repCount');
    if (!el) return;
    el.classList.remove('rep-flash');
    void el.offsetWidth; // reflow para reiniciar animación
    el.classList.add('rep-flash');
    setTimeout(() => el.classList.remove('rep-flash'), 400);
}

// Exponer funciones globales
window.eliminarPersona = eliminarPersona;
window.verSesion = verSesion;

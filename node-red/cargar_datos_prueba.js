// =====================================================
// DATOS DE PRUEBA PARA SIMADER
// =====================================================
// Ejecutar en la consola del navegador (F12 → Console)
// URL: http://localhost:1880/ui o http://localhost:1880/simader
// =====================================================

// Personas
const personas = [
    { id: 1, nombre: 'Juan García López', email: 'juan.garcia@email.com', telefono: '+34 612 345 678', descripcion: 'Deportista amateur - Running' },
    { id: 2, nombre: 'María Fernández Ruiz', email: 'maria.fernandez@email.com', telefono: '+34 623 456 789', descripcion: 'Fisioterapeuta - Rehabilitación' },
    { id: 3, nombre: 'Carlos Martínez Sánchez', email: 'carlos.martinez@email.com', telefono: '+34 634 567 890', descripcion: 'Ciclista profesional' },
    { id: 4, nombre: 'Ana López Díaz', email: 'ana.lopez@email.com', telefono: '+34 645 678 901', descripcion: 'Paciente - Lesión muscular' },
    { id: 5, nombre: 'Pedro Sánchez García', email: 'pedro.sanchez@email.com', telefono: '+34 656 789 012', descripcion: 'Entrenador personal' }
];

// Función para generar datos EMG simulados
function generarDatosEMG(numMuestras, minVal, maxVal, patron = 'normal') {
    const datos = [];
    for (let i = 0; i < numMuestras; i++) {
        let valor;
        switch (patron) {
            case 'creciente':
                valor = minVal + (i / numMuestras) * (maxVal - minVal) + (Math.random() * 0.02 - 0.01);
                break;
            case 'decreciente':
                valor = maxVal - (i / numMuestras) * (maxVal - minVal) + (Math.random() * 0.02 - 0.01);
                break;
            case 'picos':
                valor = minVal + Math.random() * (maxVal - minVal);
                if (i % 20 < 5) valor = maxVal - Math.random() * 0.05;
                break;
            default:
                valor = minVal + Math.random() * (maxVal - minVal);
        }
        datos.push(Math.max(0, Math.min(1, valor)));
    }
    return datos;
}

// Sesiones con estadísticas
const sesionesFinalizadas = [
    // Juan García López
    { id: 1, persona_id: 1, persona_nombre: 'Juan García López', fecha_inicio: new Date(Date.now() - 6*24*60*60*1000 + 9*60*60*1000).toISOString(), total_muestras: 150, emg_promedio: 0.1761, emg_max: 0.285, emg_min: 0.065 },
    { id: 2, persona_id: 1, persona_nombre: 'Juan García López', fecha_inicio: new Date(Date.now() - 6*24*60*60*1000 + 18*60*60*1000).toISOString(), total_muestras: 180, emg_promedio: 0.3531, emg_max: 0.449, emg_min: 0.25 },
    { id: 3, persona_id: 1, persona_nombre: 'Juan García López', fecha_inicio: new Date(Date.now() - 2*24*60*60*1000 + 10*60*60*1000).toISOString(), total_muestras: 270, emg_promedio: 0.1896, emg_max: 0.229, emg_min: 0.15 },

    // María Fernández Ruiz
    { id: 4, persona_id: 2, persona_nombre: 'María Fernández Ruiz', fecha_inicio: new Date(Date.now() - 5*24*60*60*1000 + 11*60*60*1000).toISOString(), total_muestras: 120, emg_promedio: 0.2121, emg_max: 0.328, emg_min: 0.081 },
    { id: 5, persona_id: 2, persona_nombre: 'María Fernández Ruiz', fecha_inicio: new Date(Date.now() - 1*24*60*60*1000 + 16*60*60*1000).toISOString(), total_muestras: 210, emg_promedio: 0.1875, emg_max: 0.268, emg_min: 0.121 },

    // Carlos Martínez Sánchez
    { id: 6, persona_id: 3, persona_nombre: 'Carlos Martínez Sánchez', fecha_inicio: new Date(Date.now() - 4*24*60*60*1000 + 7*60*60*1000).toISOString(), total_muestras: 240, emg_promedio: 0.4415, emg_max: 0.598, emg_min: 0.30 },
    { id: 7, persona_id: 3, persona_nombre: 'Carlos Martínez Sánchez', fecha_inicio: new Date(Date.now() - 4*24*60*60*1000 + 19*60*60*1000).toISOString(), total_muestras: 180, emg_promedio: 0.5521, emg_max: 0.647, emg_min: 0.453 },
    { id: 8, persona_id: 3, persona_nombre: 'Carlos Martínez Sánchez', fecha_inicio: new Date(Date.now() - 3*24*60*60*1000 + 10*60*60*1000).toISOString(), total_muestras: 150, emg_promedio: 0.3204, emg_max: 0.443, emg_min: 0.206 },
    { id: 9, persona_id: 3, persona_nombre: 'Carlos Martínez Sánchez', fecha_inicio: new Date(Date.now() - 2*24*60*60*1000 + 10*60*60*1000).toISOString(), total_muestras: 120, emg_promedio: 0.2558, emg_max: 0.298, emg_min: 0.20 },

    // Ana López Díaz
    { id: 10, persona_id: 4, persona_nombre: 'Ana López Díaz', fecha_inicio: new Date(Date.now() - 5*24*60*60*1000 + 9*60*60*1000).toISOString(), total_muestras: 90, emg_promedio: 0.0712, emg_max: 0.109, emg_min: 0.03 },
    { id: 11, persona_id: 4, persona_nombre: 'Ana López Díaz', fecha_inicio: new Date(Date.now() - 3*24*60*60*1000 + 9*60*60*1000).toISOString(), total_muestras: 180, emg_promedio: 0.114, emg_max: 0.172, emg_min: 0.062 },
    { id: 12, persona_id: 4, persona_nombre: 'Ana López Díaz', fecha_inicio: new Date(Date.now() - 1*24*60*60*1000 + 9*60*60*1000).toISOString(), total_muestras: 180, emg_promedio: 0.1379, emg_max: 0.179, emg_min: 0.10 },

    // Pedro Sánchez García
    { id: 13, persona_id: 5, persona_nombre: 'Pedro Sánchez García', fecha_inicio: new Date(Date.now() - 4*24*60*60*1000 + 14*60*60*1000).toISOString(), total_muestras: 90, emg_promedio: 0.242, emg_max: 0.347, emg_min: 0.15 },
    { id: 14, persona_id: 5, persona_nombre: 'Pedro Sánchez García', fecha_inicio: new Date(Date.now() - 2*60*60*1000).toISOString(), total_muestras: 60, emg_promedio: 0.2419, emg_max: 0.299, emg_min: 0.18 }
];

// Guardar personas
localStorage.setItem('personas', JSON.stringify(personas));
console.log('✅ Personas guardadas:', personas.length);

// Guardar sesiones
localStorage.setItem('sesionesFinalizadas', JSON.stringify(sesionesFinalizadas));
console.log('✅ Sesiones guardadas:', sesionesFinalizadas.length);

// Generar y guardar datos EMG para cada sesión
const patrones = {
    1: 'creciente',   // Calentamiento
    2: 'normal',      // Post-entrenamiento
    3: 'normal',      // Evaluación semanal
    4: 'picos',       // Diagnóstico inicial
    5: 'normal',      // Seguimiento
    6: 'picos',       // Pre-competición
    7: 'normal',      // Post-competición (fatiga alta)
    8: 'decreciente', // Recuperación día 1
    9: 'normal',      // Recuperación día 2
    10: 'normal',     // Evaluación lesión
    11: 'creciente',  // Rehabilitación 1
    12: 'creciente',  // Rehabilitación 2
    13: 'picos',      // Demo
    14: 'normal'      // Prueba cliente
};

sesionesFinalizadas.forEach(s => {
    const patron = patrones[s.id] || 'normal';
    const datos = generarDatosEMG(s.total_muestras, s.emg_min, s.emg_max, patron);
    localStorage.setItem('emgData_' + s.id, JSON.stringify(datos));
    console.log(`✅ Datos EMG sesión ${s.id} (${s.persona_nombre}): ${datos.length} muestras [${patron}]`);
});

console.log('\n════════════════════════════════════════════');
console.log('   ✅ DATOS DE PRUEBA CARGADOS CORRECTAMENTE');
console.log('════════════════════════════════════════════');
console.log('📊 Total personas:', personas.length);
console.log('📈 Total sesiones:', sesionesFinalizadas.length);
console.log('🔢 Total muestras EMG:', sesionesFinalizadas.reduce((sum, s) => sum + s.total_muestras, 0));
console.log('\n🔄 Recarga la página (F5) para ver los datos');

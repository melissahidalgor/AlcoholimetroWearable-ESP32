// Espera a que el DOM (Document Object Model) esté completamente cargado
document.addEventListener('DOMContentLoaded', () => {

    // --- Obtener referencias a elementos del HTML ---
    // Botones
    const btnIniciar = document.getElementById('iniciar-medicion');
    const btnVolver = document.getElementById('volver');

    // Elemento para la tabla de mediciones
    const tablaMedicionesBody = document.getElementById('tabla-mediciones');

    // Contenedores de las pantallas
    const pantallaPrincipal = document.getElementById('pantalla-principal');
    const pantallaMedicion = document.getElementById('pantalla-medicion');

    // Spans donde se mostrarán las mediciones actuales
    const oxigenacionSpan = document.getElementById('oxigenacion');
    const pulsoSpan = document.getElementById('pulso');
    const alcoholSpan = document.getElementById('alcohol');

    // --- Variable para almacenar las mediciones ---
    // Se carga desde localStorage al inicio o se inicializa como un array vacío
    let mediciones = cargarMedicionesLocales();
    let pollingIntervalId = null; 

    // --- Funciones principales ---

    /**
     * Inicia el proceso de medición solicitando al ESP32 que comience.
     */
    function iniciarMedicionEnESP() {
        // Clear previous values and set to blank indicating measurement is starting
        oxigenacionSpan.textContent = '--';
        pulsoSpan.textContent = '--';
        alcoholSpan.textContent = '--';

        fetch('/iniciar_medicion')
            .then(response => {
                if (!response.ok) {
                    throw new Error(`Error HTTP! estado: ${response.status}`);
                }
                return response.json();
            })
            .then(data => {
                if (data.status === "iniciando" || data.status === "en_curso") {
                    console.log("Medición solicitada, iniciando sondeo...");
                    // Start polling immediately after initiating the measurement
                    startPollingForData();
                } else {
                    console.error("El ESP32 no indicó un estado de inicio o en curso:", data.status);
                    // Optionally, you could set spans to an error message or keep them blank
                }
            })
            .catch(error => {
                console.error('Error al solicitar iniciar medición:', error);
                // Optionally, you could set spans to an error message or keep them blank
            });
    }

    /**
     * Obtiene los datos de medición (oxigenación, pulso, alcohol) desde el ESP32.
     * Actualiza los spans en la pantalla de medición y retorna el valor de alcohol.
     * @returns {Promise<object>} Una promesa que resuelve con los datos completos de la medición.
     */
    function obtenerDatosMedicion() {
        return fetch('/obtener_datos')
            .then(response => {
                if (!response.ok) {
                    throw new Error(`Error HTTP! estado: ${response.status}`);
                }
                return response.json();
            })
            .then(data => {
                // If measurement is still in progress, keep values blank
                if (data.status === "midiendo") {
                    oxigenacionSpan.textContent = '--';
                    pulsoSpan.textContent = '--';
                    alcoholSpan.textContent = '--';
                } else if (data.status === "completado") {
                    // If measurement is complete, stop polling and display values
                    clearInterval(pollingIntervalId); 
                    pollingIntervalId = null; // Reset the ID
                    
                    oxigenacionSpan.textContent = data.oxigenacion !== undefined ? data.oxigenacion.toFixed(1) : '--';
                    pulsoSpan.textContent = data.pulso !== undefined ? data.pulso : '--';
                    alcoholSpan.textContent = data.alcohol !== undefined ? data.alcohol.toFixed(2) : '--';
                } else {
                    console.error('Estado de medición desconocido:', data.status);
                    clearInterval(pollingIntervalId); // Stop polling on unknown status
                    pollingIntervalId = null;
                    // Optionally, display an error or keep blank
                    oxigenacionSpan.textContent = 'Error';
                    pulsoSpan.textContent = 'Error';
                    alcoholSpan.textContent = 'Error';
                }
                return data; // Return the entire data object
            })
            .catch(error => {
                console.error('Error al obtener los datos de medición:', error);
                clearInterval(pollingIntervalId); // Stop polling on error
                pollingIntervalId = null;
                // Optionally, display an error or keep blank
                oxigenacionSpan.textContent = 'Error';
                pulsoSpan.textContent = 'Error';
                alcoholSpan.textContent = 'Error';
                throw error;
            });
    }

    /**
     * Inicia el sondeo repetitivo para obtener datos de medición.
     */
    function startPollingForData() {
        // Clear any existing interval to prevent multiple intervals running
        if (pollingIntervalId) {
            clearInterval(pollingIntervalId);
        }
        // Poll every 1 second (adjust as needed)
        pollingIntervalId = setInterval(() => {
            obtenerDatosMedicion().then(data => {
                // The interval will be cleared in obtenerDatosMedicion() when status is "completado"
            }).catch(error => {
                // Error handling is already in obtenerDatosMedicion()
            });
        }, 1000); 
    }

    // --- Manejadores de eventos para los botones ---

    // Cuando se hace clic en el botón "Iniciar Medición"
    btnIniciar.addEventListener('click', () => {
        pantallaPrincipal.style.display = 'none';    // Oculta la pantalla principal
        pantallaMedicion.style.display = 'block';    // Muestra la pantalla de medición
        iniciarMedicionEnESP(); // Inicia la obtención y visualización de datos
    });

    // Cuando se hace clic en el botón "Volver"
    btnVolver.addEventListener('click', () => {
    obtenerDatosMedicion()
        .then(data => {
            if (data.status === "completado") {
                const alcohol = data.alcohol;
                if (!isNaN(parseFloat(alcohol))) {
                    agregarMedicion(parseFloat(alcohol));
                } else {
                    console.warn("No se recibió un valor de alcohol válido para guardar.");
                }
            } else {
                console.warn("Medición aún no completada, no se guardará.");
            }

            pantallaMedicion.style.display = 'none';
            pantallaPrincipal.style.display = 'block';
        })
        .catch(error => {
            console.error("Error al obtener o guardar los datos al volver:", error);
            pantallaMedicion.style.display = 'none';
            pantallaPrincipal.style.display = 'block';
        });
    });

    // --- Funciones para la gestión de mediciones (localStorage) ---

    /**
     * Carga las mediciones guardadas en el almacenamiento local (localStorage).
     * @returns {Array} Un array con las mediciones.
     */
    function cargarMedicionesLocales() {
        const medicionesGuardadas = localStorage.getItem('mediciones');
        return medicionesGuardadas ? JSON.parse(medicionesGuardadas) : [];
    }

    /**
     * Guarda el array de mediciones en el almacenamiento local (localStorage).
     */
    function guardarMedicionesLocales() {
        localStorage.setItem('mediciones', JSON.stringify(mediciones));
    }

    /**
     * Agrega una nueva medición al array y la guarda en localStorage.
     * Determina el nivel de riesgo basado en el valor de alcohol.
     * @param {number} alcohol El valor de alcohol medido.
     */
    function agregarMedicion(alcohol) {
        const ahora = new Date();
        const fecha = ahora.toLocaleDateString(); // Formato de fecha local
        const hora = ahora.toLocaleTimeString(); // Formato de hora local

        let riesgo = "INDETERMINADO";

        // Clasifica el riesgo de alcohol
        if (alcohol < 0.1) {
            riesgo = "Bajo";
        } else if (alcohol >= 0.1 && alcohol < 0.24) {
            riesgo = "Medio";
        } else if (alcohol >= 0.24) {
            riesgo = "Alto";
        }

        mediciones.push({
            fecha,
            hora,
            alcohol,
            riesgo
        });
        guardarMedicionesLocales();   // Guarda las mediciones actualizadas
        renderizarMediciones();       // Actualiza la tabla en el HTML
    }

    /**
     * Renderiza (muestra) todas las mediciones en la tabla HTML.
     */
    function renderizarMediciones() {
        tablaMedicionesBody.innerHTML = ''; // Limpia la tabla antes de renderizar de nuevo
        mediciones.forEach((medicion, index) => {
            const fila = tablaMedicionesBody.insertRow(); // Inserta una nueva fila
            fila.insertCell().textContent = medicion.fecha;
            fila.insertCell().textContent = medicion.hora;
            fila.insertCell().textContent = medicion.alcohol.toFixed(2); // Muestra alcohol con 2 decimales

            const celdaRiesgo = fila.insertCell();
            celdaRiesgo.textContent = medicion.riesgo;
            // Añade una clase CSS dinámica para estilizar según el riesgo
            celdaRiesgo.classList.add('riesgo-' + medicion.riesgo.toLowerCase());

            const celdaAcciones = fila.insertCell();
            const botonEliminar = document.createElement('button');
            botonEliminar.classList.add('eliminar-medicion');
            botonEliminar.textContent = 'Eliminar';
            // Asigna un evento para eliminar la medición al hacer clic
            botonEliminar.addEventListener('click', () => eliminarMediciones(index));
            celdaAcciones.appendChild(botonEliminar);
        });
    }

    /**
     * Elimina una medición del array por su índice y actualiza la tabla.
     * @param {number} index El índice de la medición a eliminar.
     */
    function eliminarMediciones(index) {
        if (confirm('¿Estás seguro de que quieres eliminar esta medición?')) {
            mediciones.splice(index, 1); // Elimina la medición del array
            guardarMedicionesLocales();  // Guarda los cambios en localStorage
            renderizarMediciones();      // Vuelve a renderizar la tabla
        }
    }

    // --- Inicialización ---
    // Renderiza las mediciones existentes al cargar la página por primera vez
    renderizarMediciones();
});
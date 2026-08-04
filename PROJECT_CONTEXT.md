# Ultima funcion comprobada: grabar navegador / audio de Windows

- Implementado el 2026-08-03 en `Source/SystemAudioCapture.h`,
  `Source/MainComponent.h`, `Source/MainComponent.cpp`, `Source/Main.cpp`, el
  proyecto Projucer y `README.md`. La franja del looper ofrece ahora una entrada
  diferenciada `GUITARRA` / `NAVEGADOR / WINDOWS`.
- La nueva fuente usa WASAPI loopback sobre la salida multimedia predeterminada
  de Windows sin cambiar el dispositivo ASIO de Loopernico. Tras la prueba real
  del usuario se corrigieron cortes y una leve ralentizacion: un temporizador de
  alta resolucion independiente de la UI drena WASAPI cada 2 ms, y el callback
  consume un ring buffer estereo preasignado con un colchon de 120 ms,
  remuestreo lineal y correccion suave de deriva entre los relojes WASAPI/ASIO.
- Ante un faltante puntual el cursor mantiene el tiempo real en vez de esperar
  muestras atrasadas (la causa de que el loop quedara estirado). Si YouTube se
  pausa y el endpoint deja de producir paquetes, la captura rearma el colchon
  exclusivamente con audio nuevo al reanudarse, sin repetir material anterior.
- El backing track se graba limpio y estereo: no atraviesa pedales/VST y no se
  convierte a mono aunque la escucha general este en `MONO`. La cadena sigue
  avanzando para conservar transporte y bus de ritmo. El audio de Windows no se
  monitoriza otra vez dentro de Loopernico, evitando duplicacion y feedback.
- La UI muestra endpoint, sample rate y pico detectado. La eleccion es operativa
  para la ejecucion actual y no cambia el formato ni el contenido de sesiones.

Verificacion inicial y correccion de continuidad 2026-08-03:
- `Release | x64` correcto con el proyecto regenerado por Projucer. Pasaron las
  31 pruebas deterministas.
- Una prueba WASAPI aislada capturo 191520 cuadros estereo a 48 kHz con pico
  `0.346893`. El Release final y `Builds/LocalDeployment/Current` capturaron la
  alarma de Windows iniciada despues de ASIO y mostraron `-9.3 dB`; las capturas
  `tmp/system-audio-release-signal.png` y
  `tmp/system-audio-deployed-signal.png` son byte a byte identicas.
- El humo desplegado cerro normalmente con codigo 0. `Current` SHA-256:
  `69371385133A2116A1EC783D57937B02FEB453D4D53ACC45A69C9CDA52BEA5BA`.
  `Previous` conserva la version funcional anterior, SHA-256
  `88744123D214586BD278F98E5961FE302A844472D892EDDF1CA4EA2BB180D37C`.
  Hay exactamente dos ejecutables desplegados.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` apunta a `Current`, con
  directorio de trabajo e icono correctos. `.loopernico` conserva
  `Loopernico.Session`, tipo, icono y comando con `%1` hacia `Current`.
- La correccion de continuidad compilo en `Release | x64` sin advertencias y
  volvio a pasar las 31 pruebas. Una prueba de 10 s con la captura desacoplada
  de la UI recibio la salida de Windows a 48 kHz y `-7.3 dB`, y cerro con codigo
  0 (`tmp/system-audio-continuity-release.png`). La version defectuosa no se
  roto a `Previous`: se reemplazo `Current` y se conservo el ultimo Release
  estable anterior como respaldo.

Uso: elegir `NAVEGADOR / WINDOWS`, comenzar a reproducir YouTube y comprobar
que aparece un nivel en dB antes de pulsar `REC / CONFIRM`. Para volver al
instrumento, elegir `GUITARRA`.

Limitacion real: se captura la salida multimedia predeterminada completa de
Windows, no una aplicacion individual. Las notificaciones u otros programas que
suenen por ese endpoint tambien quedaran grabados.

Proximo paso concreto: repetir la prueba auditiva con la misma cancion de
YouTube que revelo el problema y luego agregar una segunda pista de guitarra.

# Ultimo ajuste comprobado: luna orbitando durante las cargas

- Implementado el 2026-07-31 en el nuevo
  `Source/OrbitLoadingIndicator.h`, `Source/Main.cpp`,
  `Source/MainComponent.h` y `Source/MainComponent.cpp`. La barra generica fue
  reemplazada por una escena vectorial propia de Loopernico: planeta celeste
  con volumen, orbita eliptica, luna con crater y profundidad, estrellas
  discretas, marca y texto de estado.
- La animacion nativa de Windows tiene su propio hilo y ciclo de mensajes a
  30 fps. Por eso la luna sigue girando aunque la construccion de la interfaz,
  ASIO, disco o plugins ocupen el hilo principal y Windows marque temporalmente
  la ventana grande como no responsiva. Un unico indicador compartido cubre el
  arranque y luego cambia a `CARGANDO SESION` con el nombre del archivo; al
  abrir desde `ABRIR`, se inicia directamente con ese estado.
- La capa JUCE inferior conserva el oscurecimiento modal y bloquea los controles
  durante la restauracion. La ventana orbital no toma foco, se mantiene al
  frente y desaparece automaticamente al finalizar o fallar la apertura.
- No se modificaron DSP, audio, formato o contenido de sesiones, MIDI, pedales,
  presets ni logica de restauracion.

Verificacion y despliegue 2026-07-31:
- `Release | x64` compilado correctamente con
  `Builds/VisualStudio2026/Loopernico Refactorizado.sln`, codigo 0. Pasaron las
  31 pruebas deterministas y el diagnostico completo de persistencia: audio
  exacto antes/despues de reiniciar dispositivo y sobrescribir, ORDER, preset y
  bloqueo de REC restaurados.
- Durante la apertura real de
  `C:\Users\Fernando\Desktop\ahshf.loopernico` se capturaron 65 cuadros
  consecutivos en `tmp/orbit-loading/verified`, todos con SHA-256 distinto. Las
  capturas `orbit-10.png` y `orbit-40.png` prueban posiciones diferentes y el
  cambio de `PREPARANDO Y ABRIENDO LA SESION` a `CARGANDO SESION`. La sesion
  final queda restaurada en `tmp/orbit-loading/verified-after-load.png` con
  formas de onda, orden y preset visibles.
- El humo de `Builds/LocalDeployment/Current` genero
  `tmp/orbit-loading/deployed-smoke.png` y cerro con codigo 0. La apertura
  directa asociada de `ahshf.loopernico` comprobo el indicador orbital, su
  desaparicion, ventana final responsiva y cierre normal.
- `Current` SHA-256:
  `88744123D214586BD278F98E5961FE302A844472D892EDDF1CA4EA2BB180D37C`.
  `Previous` conserva la version funcional inmediatamente anterior con barra:
  `7B7FD20F01D14033BC3EE25CF5A4F30C9AC1F1738328CBB99387CB921412A11B`.
  Hay exactamente dos ejecutables desplegados.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` apunta a `Current`, con
  directorio de trabajo e icono correctos. `.loopernico` conserva
  `Loopernico.Session`, tipo `Archivo de Loopernico`, comando con `%1` hacia
  `Current` e icono de sesion.

Proximo paso concreto: ninguno; evaluar en uso diario solamente si se desea
ajustar velocidad, escala o color de planeta/luna.

# Ultimo ajuste comprobado: perillas de PAMPA FUZZ alineadas con el frente

- Corregido el 2026-07-31 en `Source/LoopernicoWidgets.cpp`. El frente del
  Pampa se pintaba dentro de un rectangulo centrado y ajustado a su proporcion
  `977:1610`, mientras los tres controles seguian usando el rectangulo generico
  completo del componente. Al cambiar la altura disponible de la tira, esos
  dos sistemas divergieron y las perillas quedaron desplazadas hacia abajo.
- `resized()` calcula ahora para FUZZ exactamente el mismo cuerpo, margen y
  proporcion que `paint()`. Una primera comprobacion mantuvo `y=0.271`, pero
  el aro impreso todavia asomaba. La medicion directa del frente nativo ubico
  el centro circular en `y=400/1610` (`0.2484`), coordenada que usa ahora el
  layout. El tamano permanece en `0.300` del ancho y los centros horizontales
  en `x=0.200/0.500/0.800`. No se modificaron asset, DSP, parametros, MIDI,
  sesiones, footswitch ni otros pedales.

Verificacion y despliegue 2026-07-31:
- `Release | x64` compilado correctamente con
  `Builds/VisualStudio2026/Loopernico Refactorizado.sln`: 0 errores y 0
  advertencias. Las 31 pruebas deterministas pasaron.
- Capturas finales revisadas en `tmp/pampa-fuzz-ring-centered-v2.png` y
  `tmp/pampa-fuzz-ring-centered-overview-v2.png`: los tres knobs cubren y se
  centran sobre los aros tanto en vista HD como en la tira compacta.
- El humo de `Builds/LocalDeployment/Current` genero
  `tmp/pampa-fuzz-ring-centered-deployed-v2.png` y cerro normalmente con
  codigo 0.
- `Current` SHA-256:
  `20C0C02FB1CCBB3A0A1E20B5C802317F99C823C0710BF589FDDEFA2CEF4BCDEA`.
  `Previous` conserva la version funcional anterior:
  `6DEDC0F49AE6849765A3E04B3FC5123712DB5E73AB0E5CC0B46D33EAEC21B70E`.
  Hay exactamente dos ejecutables desplegados.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` fue recreado hacia `Current`,
  con directorio de trabajo e icono correctos. `.loopernico` conserva
  `Loopernico.Session`, tipo `Archivo de Loopernico`, comando con `%1` hacia
  `Current` e icono de sesion.

Proximo paso concreto: ninguno; confirmar al tacto visual en uso diario si se
desea, pero la geometria queda registrada con el asset en ambas escalas.

# Ultimo ajuste comprobado: REC bloqueado mientras el loop esta pausado

- Corregido el 2026-07-31 en `Source/LooperEngine.cpp` y
  `Source/MainComponent.cpp`. `recConfirm()` rechaza cualquier orden mientras
  la pausa global este activa, por lo que ni el boton ni MIDI pueden iniciar
  un overdub al reabrir una sesion pausada. La UI deshabilita REC al cargar o
  pausar y lo habilita inmediatamente al reanudar.
- El diagnostico de persistencia ahora comprueba la reapertura pausada, el
  estado visual deshabilitado, que una orden directa no cambie estado ni
  cantidad de pistas y que REC vuelva a habilitarse al quitar PAUSE.
- No se modificaron el formato `.loopernico`, el audio guardado, DSP, pedales,
  orden, presets ni controles de transporte ajenos.

Verificacion y despliegue 2026-07-31:
- `Release | x64` compilado correctamente con
  `Builds/VisualStudio2026/Loopernico Refactorizado.sln`, codigo 0.
- El diagnostico completo paso tanto en el candidato como en
  `Builds/LocalDeployment/Current`: audio exacto, reinicio de dispositivo,
  sobrescritura, ORDER, preset y bloqueo/desbloqueo de REC. Informes en
  `tmp/session-persistence/record-lock-roundtrip.report.txt` y
  `record-lock-deployed-roundtrip.report.txt`.
- Las 31 pruebas deterministas pasaron en el ejecutable desplegado; informe en
  `tmp/session-persistence/record-lock-latency-tests.txt`.
- Captura revisada en
  `tmp/session-persistence/record-lock-paused.png`: el loop restaurado conserva
  forma de onda, PAUSE activo y REC atenuado. Los humos normales del candidato
  y de `Current` cerraron con codigo 0.
- La reapertura automatica en proceso fresco resulto sensible al cambio tardio
  del dispositivo ASIO ya contemplado por `applySessionTree()` y algunas
  corridas mostraron la sesion parcialmente aplicada; no se amplio esta tarea
  para cambiar ese flujo. La reapertura determinista dentro del diagnostico
  paso en ambos binarios.
- `Builds/LocalDeployment/Current` SHA-256:
  `36ADBABBE21A7A8A8DC413D9EA4DF7298FB5024FC445BB5DE73D38569C521107`.
  `Previous` conserva el Current funcional anterior:
  `429D598576386AA6947FF92183A46AE6B483C212BE0DE05B380A6D0DF4B8A05B`.
  Hay exactamente dos ejecutables desplegados.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` fue recreado hacia `Current`,
  con directorio de trabajo e icono correctos. `.loopernico` conserva
  `Loopernico.Session`, tipo `Archivo de Loopernico`, comando con `%1` hacia
  `Current` e icono de sesion.

Proximo paso concreto: confirmar en uso diario que, al abrir una sesion pausada,
REC permanezca bloqueado hasta pulsar PAUSE para reanudar. Si la apertura inicial
coincide con un reinicio tardio de ASIO, evaluar por separado un reintento
automatico de restauracion despues de estabilizar el dispositivo.

# Ultimo ajuste comprobado: sesiones pausadas vuelven a abrir correctamente

- Corregido el 2026-07-31 a partir del archivo real
  `C:\Users\Fernando\Desktop\ff.loopernico`. El audio no estaba truncado: el
  archivo contiene 192000 muestras estereo, pico 0.142115 y 383998 valores no
  nulos. El rechazo ocurria porque `pauseAll()` copiaba el cursor global
  `116480` tambien a las once pistas vacias y el lector trataba ese cursor de
  reproduccion como si fuera una longitud de audio invalida.
- `pauseAll()` ahora deja cursor cero en pistas inactivas. Las capturas directa
  e incremental normalizan `pausedReadPos` contra el audio efectivamente
  copiado. El lector mantiene estrictas las longitudes de audio, pero normaliza
  el cursor residual; asi abre las sesiones ya afectadas, incluido `ff`, sin
  descartar su grabacion.
- Se mantiene la correccion previa de escritura atomica con
  `juce::TemporaryFile`, la preservacion del loop al reiniciar el dispositivo,
  la validacion del snapshot, el nodo `ORDER` autoritativo y `presetName` para
  pedales internos/VST3 con fallback compatible para sesiones antiguas.
- `--run-session-persistence-test=<archivo>` ahora reproduce el caso real:
  genera audio, avanza el cursor, pausa globalmente, exige cursores cero en
  pistas vacias, reinicia audio, sobrescribe, lee y compara muestras exactas,
  y restaura audio, orden y preset en la UI.

Verificacion y despliegue 2026-07-31:
- `Release | x64` compilado correctamente con
  `Builds/VisualStudio2026/Loopernico Refactorizado.sln`, codigo 0.
- Roundtrip determinista:
  `tmp/session-persistence/paused-roundtrip.loopernico` y su reporte. Declara
  una pista de 4095 muestras estereo, pico 0.350209, cursor activo 1024, cursores
  cero en las once pistas vacias,
  `ORDER=13,2,3,0,5,6,7,8,9,10,11,12,14,15,16` y
  `presetName=SESSION TEST PRESET`. La lectura binaria coincide muestra por
  muestra con el snapshot previo y la restauracion completa paso.
- La reapertura en otro proceso se reviso en
  `tmp/session-persistence/paused-roundtrip-fresh-process.png`: forma de onda,
  Noise Suppressor en la cuarta posicion y preset visible. El archivo real
  `ff.loopernico` abre sin alerta y muestra su forma de onda, orden
  `13,6,0,2,3,5,7,8,9,10,11,12,14,15,16` y preset `Poco Quilombo`; capturas en
  `ff-reopen-candidate.png` y `ff-reopen-current.png`.
- El controlador interactivo de Windows sigue sin capturar esta ventana por
  `SetIsBorderRequired (0x80004002)`; la verificacion visual se completo con el
  snapshot nativo de JUCE. Los procesos de prueba terminaron con codigo 0.
- `Builds/LocalDeployment/Current` SHA-256:
  `429D598576386AA6947FF92183A46AE6B483C212BE0DE05B380A6D0DF4B8A05B`.
  `Previous` conserva el Current funcional anterior:
  `4C3DC3F59BDB5E37846BA6B27D44869823DA20580C5CFF7F60F376968707E1D6`.
  Hay exactamente dos ejecutables desplegados.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` fue recreado hacia `Current`, con
  directorio de trabajo e icono correctos. `.loopernico` conserva
  `Loopernico.Session`, tipo `Archivo de Loopernico`, comando con `%1` hacia
  `Current` e icono de sesion.

Proximo paso concreto: confirmar en uso diario que `ff.loopernico` reproduce su
audio al reanudar y guardar una nueva sesion pausada; el flujo automatizado y el
despliegue quedan terminados.

# Ultimo ajuste comprobado: borrar y sobrescribir presets por pedal

- Corregido el 2026-07-30 en `Source/MainComponent.cpp` y
  `Source/MainComponent.h`. La franja de presets de cada pedal interno y de
  cada host VST3 ahora tiene `+` para guardar, `-` para borrar y selector para
  elegir. `+` preselecciona el nombre actualmente elegido; si ya existe,
  solicita confirmacion antes de reemplazar su estado. `-` solicita confirmacion
  y elimina solamente el preset elegido del XML de usuario.
- La biblioteca sigue siendo `Loopernico/pedal-presets.xml`; no cambia el
  formato de sesiones. La busqueda de presets internos excluye explicitamente
  nodos VST3 aunque compartan indice historico.

Verificacion y despliegue 2026-07-30:
- `Release | x64` compilado correctamente con
  `Builds/VisualStudio2026/Loopernico Refactorizado.sln`.
- El ejecutable de `Builds/LocalDeployment/Current` inicio con ventana,
  permanecio abierto durante la comprobacion y cerro normalmente con codigo
  0. `Previous` conserva el ejecutable funcional inmediatamente anterior;
  hay exactamente dos EXE desplegados.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` y la asociacion `.loopernico`
  siguen apuntando a `Current`, con directorio de trabajo e icono conservados.
- `git diff --check` correcto; solo informa los avisos preexistentes de
  normalizacion LF/CRLF.

Proximo paso concreto: probar en uso diario guardar, sobrescribir y borrar un
preset real de un pedal y de un VST3.

# Ultimo ajuste comprobado: controles del EHX Pico Pitch Fork sin sombras artificiales

- Corregido el 2026-07-29 en `Source/LoopernicoLookAndFeel.h`.
- El pedal de pitch shifter de la cadena conserva el asset frontal v4, sus
  proporciones, centros, hitboxes y el footswitch rasterizado. Se ajusto
  unicamente el render de sus cuatro potenciometros para calcar la referencia:
  se retiro la sombra generica y el aro radial exagerado, dejando un aro metalico
  fino, tapa negra, brillo discreto y marcador blanco.
- No se modificaron DSP, parametros, MIDI, sesiones, orden de pedales ni el
  calculo o tamano del switch.
- El footswitch visual se redujo finalmente a `0.35` del ancho del cuerpo para
  despejar por completo la linea curva impresa; el centro y el hitbox permanecen
  sin cambios.

Verificacion y despliegue 2026-07-29:
- `Release | x64` compilado correctamente con
  `Builds/VisualStudio2026/Loopernico Refactorizado.sln`.
- Captura revisada: `tmp/pico-pitch-fork/pico-knob-final-hd-2.png`; confirma
  potes sin sombras raras, etiquetas legibles, switch centrado y proporciones
  coincidentes con la foto `C:\Users\Fernando\Downloads\ehx-pico-pitch-fork.jpg`.
- El humo del candidato y de `Builds/LocalDeployment/Current` fue responsivo
  durante 5 segundos y cerro normalmente con codigo 0.
- `Builds/LocalDeployment/Current` SHA-256:
  `41FFCD71125F3CC41C92C7688E42775C3C106D460D9ECCF9254164AE79D6E1A3`.
  `Previous` conserva la version funcional inmediatamente anterior, SHA-256
  `34D2A37775422E5FBEFFF353660CDDF7A4398AEE474BE0BA225D51DFEED2C46C`.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` apunta a `Current`, con
  directorio de trabajo e icono correctos. `.loopernico` conserva
  `Loopernico.Session`, tipo `Archivo de Loopernico`, comando hacia `Current`
  con `%1` e icono de sesion.

Proximo paso concreto: ninguno; el ajuste visual queda listo para uso diario.

# Tarea terminada: geometria final del EHX Pico Pitch Fork

- Corregido el 2026-07-29 contra la referencia frontal aportada por Fernando.
  La carcasa conserva la proporcion fisica `976:1781` (`2.0 / 3.65`) y el
  frente activo es `Assets/Pedals/ehx-pico-pitch-fork-face-v4.png`.
- Los cuatro potes ahora miden `0.330` del ancho del cuerpo y se centran en
  `x=0.211/0.767`, `y=0.157/0.370`. Esto corrige la escala 24-30 % menor y las
  columnas demasiado juntas de la implementacion anterior.
- El footswitch rasterizado se centra en `0.500/0.878`; su lienzo mide `0.535`
  del ancho para que el metal visible, descontando el alpha del asset, ocupe
  aproximadamente `0.40`, igual que en la referencia. MODE permanece en
  `0.500/0.053` y el LED en `0.500/0.262`.
- Pintado, bounds e hitboxes usan las mismas coordenadas normalizadas, por lo
  que miniatura y vista HD quedan registrados con el frente. No se modificaron
  DSP, parametros, MIDI, sesiones ni comportamiento de los controles.

Verificacion y despliegue 2026-07-29:
- `Release | x64` compilado correctamente con
  `Builds/VisualStudio2026/Loopernico Refactorizado.sln`.
- Pasaron las 31 pruebas deterministas. Capturas revisadas:
  `tmp/pico-pitch-fork/pico-geometry-final-compact.png` y
  `tmp/pico-pitch-fork/pico-geometry-final-hd.png`; ambas confirman proporcion,
  escala y alineacion sin solapamientos.
- El humo normal del candidato y de `Builds/LocalDeployment/Current` fue
  responsivo y cerro normalmente con codigo 0.
- `Builds/LocalDeployment/Current` SHA-256:
  `6809E6A6B27C6B9377651CABA896DE25F9E641DF6FEE2F99B1EACED3AC78EDDB`.
  `Previous` conserva la version funcional inmediatamente anterior, SHA-256
  `E707D38DB19A5FF6DD0D25B95B8874386000A645F64513549A484F9510AA632E`;
  hay exactamente dos EXE desplegados.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` fue recreado hacia `Current`, con
  directorio de trabajo e icono correctos. `.loopernico` conserva
  `Loopernico.Session`, tipo `Archivo de Loopernico`, comando con `%1` hacia
  `Current` e icono de sesion.

Proximo paso concreto: ninguno; la geometria queda lista para uso diario.

# Tarea terminada: Eventide H90 reemplazado por EHX Pico Pitch Fork

- Finalizado el 2026-07-29. El pedal interno historico de indice 3 conserva el
  identificador `HARMONIZER` solamente para compatibilidad, pero en UI, MIDI y
  sesiones nuevas se presenta como `Electro-Harmonix Pico Pitch Fork`.
- El gabinete ahora es compacto (proporcion 976x1781) y usa el frente versionado
  `Assets/Pedals/ehx-pico-pitch-fork-face-v4.png`, evolucionado desde la primera
  integracion y ajustado contra la referencia frontal de EHX. La miniatura y la vista HD comparten la
  misma proporcion; la captura revisada esta en
  `tmp/pico-pitch-fork/pico-ui.png`.
- Controles implementados: VOL, BLEND, SWEEP 0-4 s, diez posiciones SHIFT,
  MODE UP/DOWN/DUAL y footswitch LATCH/MOMENTARY. Una pulsacion corta de MODE
  recorre los tres modos; mantenerlo 2 s alterna latch/momentary. Los intervalos
  simples y los diez pares DUAL siguen la tabla publicada para el pedal fisico.
- El pitch shifting usa Signalsmith Stretch 1.3.2, fijado al commit
  `57b93f4e9206a089a45387eaa39bdc9f310d3308`, con Signalsmith Linear 0.3.1
  (`5668673560146a9cfe38c25315071e3fd68c8317`) y sus licencias dentro de
  `ThirdParty/Signalsmith`. La configuracion es estereo, dual, determinista,
  preasignada y con calculo FFT repartido; no asigna memoria desde
  `processSample()`. La latencia medida a 48 kHz es 1728 muestras y se suma a
  la compensacion activa de grabacion junto con la latencia VST3.
- Las sesiones nuevas guardan `picoPitchForkVersion=1`, `picoMode` y
  `picoMomentary`. Las sesiones H90 anteriores se migran explicitamente:
  conservan el balance wet y convierten las dos voces al par DUAL oficial mas
  cercano. Se verifico guardado, apertura y segundo guardado con
  `tmp/pico-pitch-fork/pico-roundtrip.loopernico` y
  `pico-roundtrip-2.loopernico`, ambos con cierre normal codigo 0.

Verificacion y despliegue 2026-07-29:
- `Release | x64` compilado correctamente. El algoritmo de terceros quedo
  encapsulado y no emite avisos en los consumidores; el build muestra solamente
  dos C4244 preexistentes en `LatencyCompensation.cpp:188`.
- Las 31 pruebas deterministas internas pasaron. La prueba DSP independiente
  obtuvo 880 Hz para +12 semitonos desde 440 Hz y 221 Hz para -12 (objetivo
  220 Hz). El modo estereo dual proceso 5 s en 0,677481 s, equivalente a 7,38x
  tiempo real en esta maquina.
- Captura HD revisada: controles legibles, proporcion correcta y sin regresiones
  evidentes. El cierre programado inmediatamente despues de esa captura encontro
  el problema externo conocido `Windows.Devices.Midi.dll_unloaded`; el flujo
  normal fue probado por separado tanto en el candidato como en `Current`, con
  ventana responsiva y cierre limpio codigo 0.
- `Builds/LocalDeployment/Current` SHA-256:
  `E707D38DB19A5FF6DD0D25B95B8874386000A645F64513549A484F9510AA632E`.
  `Previous` conserva la version funcional anterior, SHA-256
  `5E607AF3CC46FDDEF5DBA24C1133266AEC51BEB1C44FC72FE5FAC5DD1FD3745A`;
  hay exactamente dos EXE desplegados.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` fue recreado hacia `Current`, con
  directorio de trabajo e icono correctos. `.loopernico` conserva
  `Loopernico.Session`, tipo `Archivo de Loopernico`, comando con `%1` hacia
  `Current` e icono de sesion.

Proximo paso concreto: probarlo con guitarra real y ajustar VOL/BLEND al gusto;
la sustitucion, migracion y version diaria ya estan terminadas.

# Limpieza segura de bancos WAV retirados

- Realizada el 2026-07-29 después del retiro de BeatBuddy/Rhythm Box.
- Se eliminaron exclusivamente 1416 archivos `.wav` versionados: 139 de
  `Assets/Samples/RhythmBox`, 1184 de `Source/The Crate vol.6`, 28 de
  `Source/rock-kit` y 65 de `Source/Reapertips - Metronome Sounds`.
- No se eliminaron código, proyectos, imágenes, sesiones ni documentación. Se
  conservaron los README/licencias de las cuatro carpetas. Las bajas permanecen
  recuperables desde el historial de Git.
- El backend histórico de Rhythm Box sigue compilado pero es inalcanzable desde
  el orden activo. Si alguna vez se restaura, deberá reponerse un banco en
  `Assets/Samples/RhythmBox`; sin samples externos conserva su fallback
  sintetizado.

Verificación 2026-07-29:
- El ejecutable diario `Builds/LocalDeployment/Current` inició y aceptó el
  cierre normal de su ventana principal, terminando con código 0. La consulta
  automática `Responding` no se volvió positiva durante los 8 segundos de la
  prueba con la ventana oculta.
- `git diff --cached` confirmó 1416 bajas `.wav` y ninguna baja de otro tipo.

Próximo paso concreto: ninguno; conservar el commit de limpieza como punto de
recuperación.

# Tarea terminada: réplica VOX Cooltron Duel Overdrive

- Finalizado el 2026-07-29 a partir de `C:\Users\Fernando\Downloads\vox.webp`
  (2000x2000). El cuerpo visible medido ocupa aproximadamente 2000x1874 px,
  proporción 1.0672:1; esa proporción se usa en miniatura y vista HD.
- Se conserva el frente versionado
  `Assets/Pedals/vox-cooltron-duel-overdrive-face-v1.png`. Rejilla, marco
  cromado, emblemas, serigrafías, footswitches y siete alojamientos quedan
  registrados con los controles interactivos. La válvula azul pertenece al
  frente y permanece iluminada independientemente de EFFECT.
- ImageGen produjo dos controles rasterizados específicos desde la referencia:
  `Assets/Hardware/vox-cooltron-large-knob-v1.png` para los cuatro knobs
  negros alargados y `Assets/Hardware/vox-cooltron-small-knob-v1.png` para los
  tres knobs estriados con centro metálico. Se generaron aislados sobre chroma
  verde y se procesaron con `remove_chroma_key.py`; ambos son RGBA 1254x1254,
  tienen esquinas alpha 0 y no conservan contaminación verde parcial. SHA-256:
  `E9CCF80856A262AF9C52B87B87A88C962C4DFF7F1CA3E89D7E871CE31A337046`
  y `D2DC63C1D613F486B834D5CD4DCB0E4788C87EFA2795B47270E7B3F5072E763B`.
- Los únicos controles visibles e interactivos son GAIN/VOLUME/TONE 1,
  GAIN/VOLUME/TONE 2, BASS BOOST OFF/OD2/BOTH, EFFECT y OD1/OD2. Se retiraron
  TYPE 1, TYPE 2, CONFIG, BRIGHT, sus zonas invisibles, tooltips y todo el
  ruteo exclusivo del Sunset. Los LED rojos representan EFFECT y la selección
  OD2 respectivamente.
- Los LED del VOX usan una lente específica rojo rubí, con bisel metálico,
  gradiente interno y reflejo cálido. La luz queda contenida dentro de la lente:
  se eliminó el halo exterior y el rojo escarlata saturado del renderer común.
  El estado apagado conserva una lente granate oscura claramente diferenciada.
- Los dos footswitches usan `Assets/Hardware/footswitch-metal-v3.png`
  superpuesto sobre los alojamientos impresos. El área alpha útil del raster
  mide 383/512 de su lienzo: se dibuja a 12,8% del ancho del cuerpo para que
  el metal visible ocupe 9,6%, igual que la impresión. Sus centros e hitboxes
  se alinearon por separado y no invaden `EFFECT`, `OD1 / OD2` ni el emblema.
- El DSP usa un modelo Cooltron fijo con oversampling 4x y carácter distinto
  para OD1 y OD2. BASS BOOST es procesamiento real previo a la saturación:
  OFF no refuerza, OD2 actúa solo sobre OD2 y BOTH actúa sobre ambos canales.
  Sus estados y coeficientes están preasignados; el callback no asigna memoria
  ni toma bloqueos.
- El formato nuevo guarda `voxCooltronVersion=1`, `voxOd2Selected` y
  `voxBassBoost`. La migración explícita lee sesiones Sunset históricas,
  convierte `sunsetBOn` a selección OD2 y `sunsetBright` a OFF/OD2/BOTH; las
  sesiones anteriores al Sunset también se convierten desde el antiguo Mirror.
  El roundtrip `tmp/vox-sunset-migrated-roundtrip.loopernico` guarda solamente
  propiedades VOX nuevas para el pedal.
- El identificador interno histórico `SUNSET` se conserva exclusivamente para
  mantener el índice 7 y cargar sesiones antiguas; no se muestra en UI, MIDI
  ni tooltips.

Verificación y despliegue 2026-07-29:
- `Release | x64` compilado correctamente. El ejecutable pasó 31 pruebas
  deterministas; cuatro mediciones específicas a 100 Hz verifican OFF, OD2 y
  BOTH en OD1/OD2. Informe:
  `tmp/vox-cooltron-switch-overlay-dsp-tests.txt`.
- Capturas finales revisadas con los switches rasterizados:
  `tmp/vox-cooltron-switch-overlay-compact.png` (SHA-256
  `D810C9A9A65AE75043F4BE6C0578A36D3B47E90622E50B16EBE64C71F17A499A`),
  `tmp/vox-cooltron-switch-overlay-off-hd.png`,
  `tmp/vox-cooltron-switch-overlay-od1-hd.png` y
  `tmp/vox-cooltron-switch-overlay-od2-hd.png` (SHA-256
  `31466801EEA625708E7D3C653AD9D83F988CB676DC5BADB4524121404F19A924`).
  Comprueban registro y separación de los dos footswitches en miniatura y HD,
  tanto apagados como activos, además de ambos LED.
- El acabado final de los LED se comprobó apagado, EFFECT, OD2 y ambos activos
  en `tmp/vox-cooltron-led-realistic-off-hd.png`,
  `tmp/vox-cooltron-led-realistic-od1-hd.png`,
  `tmp/vox-cooltron-led-realistic-both-hd.png` (SHA-256
  `C38683F5EC8CC4D178295FBEBE541E7112CC66274B7150A4F16228D337B16372`)
  y `tmp/vox-cooltron-led-realistic-compact.png`.
- El humo normal del Release y del `Current` desplegado permaneció responsivo
  y cerró con código 0. `tmp/vox-cooltron-led-realistic-deployed.png` es byte
  a byte igual a la captura final con ambos LED activos.
- `Builds/LocalDeployment/Current` SHA-256:
  `5E607AF3CC46FDDEF5DBA24C1133266AEC51BEB1C44FC72FE5FAC5DD1FD3745A`.
  `Previous` conserva exactamente el Current anterior, SHA-256
  `FACC3CB83AF7516445C5E6AEC9DAB76A2DFAC0BDBF63529963C91F34CF5BFD6A`.
  Hay exactamente dos ejecutables en el despliegue local.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` fue recreado hacia `Current`, con
  directorio de trabajo e icono correctos. `.loopernico` fue verificado como
  `Loopernico.Session`, con comando entre comillas y `%1` hacia `Current`.

Próximo paso concreto: ninguno; la réplica queda lista para uso diario.

# Tarea terminada: OLED real del TAPE DELAY / Timeline MX

- Corregido el 2026-07-29 en `Source/LoopernicoWidgets.cpp` y, solamente para
  mantener el hitbox alineado cuando el componente queda limitado por altura,
  en `Source/LoopernicoWidgets.h`.
- La inspeccion directa de
  `Assets/Pedals/strymon-timeline-mx-face-v2.png` a 1536x1024 ubico el area
  emisiva interior real del OLED en `x=649`, `y=133`, `w=479`, `h=126`.
  El asset no se modifico; conserva SHA-256
  `D0AC10442465D92B466BF104817A1B5484FCF69DC94FC0D2CDB23AF93A096736`.
- El recorte que realmente dibuja `getPedalTextureCrop()` queda centralizado
  en pixeles como `x=15`, `y=10`, `w=1505`, `h=1004`, equivalentes al recorte
  anterior `0.010/0.010/0.980/0.980` despues del redondeo. La pantalla y los
  nueve centros de knobs se convierten desde coordenadas del PNG completo al
  espacio de ese recorte mediante constantes comunes.
- El OLED dinamico tiene relleno totalmente opaco, borde y esquinas
  proporcionales. Las dos lineas quedan centradas, con margenes proporcionales
  y fuente escalable limitada a 18 px en HD. No cubren el bisel, el marco
  blanco, `TYPE`, `VALUE`, `strymon`, `TIMELINE MX` ni otra serigrafia.
- `paint()`, `resized()` y el hitbox de los tres footswitches usan la proporcion
  1536:1024 y centran la carcasa tanto si limita el ancho como si limita la
  altura. Los nueve knobs usan centros medidos en el PNG: TYPE `(221,190)`,
  VALUE `(1414,190)` y la fila inferior en `y=445`, con `x=115,336,552,768,
  984,1204,1420`.
- No se modificaron DSP, audio, MIDI, sesiones reales, parametros, rangos,
  comportamiento del delay ni otros pedales. La sesion usada para exponer el
  pedal al inicio de la tira es temporal:
  `tmp/timeline-compact-first.loopernico`.

Verificacion y despliegue 2026-07-29:
- `Release | x64` correcto con
  `Builds/VisualStudio2026/Loopernico Refactorizado.sln`: 0 errores y
  0 advertencias.
- Capturas nativas revisadas:
  `tmp/timeline-oled-compact-final.png` (SHA-256
  `5C9AE2A6C63125599A9CB9B7D88A37C7C4DF0110AF683914D7F3057CE8BC7001`)
  y `tmp/timeline-oled-focused-final.png` (SHA-256
  `BBA98A0F0CBFC3D3A05C478D73D37CD33A4EEE2F5CC1CAC56B2416BA4416F05E`).
  Ambas confirman OLED contenido por los cuatro lados, fondo opaco, dos lineas
  legibles, proporcion consistente y knobs registrados con la impresion.
- Humo del ejecutable desplegado correcto mediante
  `tmp/timeline-deployed-smoke.png`, cierre normal y codigo 0.
- `Builds/LocalDeployment/Current` SHA-256:
  `3076703DBE6EFC7B279628D6011EBAEB5DA64FCEFE6695E44180AF7F7F96ABEB`.
  `Previous` conserva la version funcional inmediatamente anterior, SHA-256
  `624833F76A00B64EDD88615F8C934094990D2700BC73F1EC13E20B6E6A0A1BAE`.
  Hay exactamente dos ejecutables en el despliegue local.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` fue recreado hacia `Current`, con
  directorio de trabajo e icono de sesion correctos. `.loopernico` conserva
  `Loopernico.Session`, tipo `Archivo de Loopernico`, comando con `%1` hacia
  `Current` e icono de sesion.

Proximo paso concreto: ninguno; el OLED del Timeline MX queda validado en
miniatura, vista ampliada y ejecutable desplegado.

# Tarea terminada: OLED real del ROOM REVERB / BigSky MX

- Corregido el 2026-07-28 en `Source/LoopernicoWidgets.cpp`. La medicion
  directa de `Assets/Pedals/strymon-bigsky-mx-face-v3.png` ubico el OLED
  interior en `x=659`, `y=175`, `w=422`, `h=117` sobre el PNG de 1536x1024.
  Esas coordenadas ahora se normalizan contra el recorte que realmente dibuja
  la interfaz (`103,56,1330,916`), evitando mezclar coordenadas del PNG completo
  con coordenadas del cuerpo renderizado.
- El relleno dinamico es totalmente opaco, con esquinas y borde proporcionales.
  `CLOUD` queda centrado dentro del OLED y su fuente escala con la pantalla,
  limitada a 22 px en la vista HD. No cubre `BigSky MX`, `strymon`, `TYPE`,
  `VALUE` ni ninguna otra serigrafia.
- `paint()` y `resized()` usan la proporcion 1536:1024 y centran el cuerpo
  tambien si la vista queda limitada por altura. Los nueve knobs conservan sus
  coordenadas y quedaron alineados con los controles impresos.
- No se modificaron DSP, audio, MIDI, sesiones, parametros ni otros pedales.

Verificacion y despliegue 2026-07-28:
- Release `x64` correcto con
  `Builds/VisualStudio2026/Loopernico Refactorizado.sln`, codigo 0.
- Capturas nativas revisadas:
  `tmp/bigsky-oled-compact-final.png` y
  `tmp/bigsky-oled-focused-final.png`. La miniatura se genero con una sesion
  temporal en `tmp/` que solo llevo ROOM REVERB al primer slot para hacerlo
  visible; no se altero ninguna sesion real.
- Humo de `Builds/LocalDeployment/Current` correcto mediante
  `tmp/bigsky-deployed-smoke.png`, cierre normal y codigo 0. El controlador
  interactivo de Windows no pudo capturar la ventana por
  `SetIsBorderRequired (0x80004002)`; la comprobacion visual completa se hizo
  con `Component::createComponentSnapshot`.
- `Current` SHA-256:
  `624833F76A00B64EDD88615F8C934094990D2700BC73F1EC13E20B6E6A0A1BAE`.
  `Previous` conserva la version funcional anterior:
  `CB9B0C7A3D32DA57CB8A8BD0EF5E7775341EA12A51D0068BE9FA307AD8CD7DDA`.
  Hay exactamente dos ejecutables desplegados.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` fue recreado hacia `Current`,
  con directorio de trabajo e icono de sesion correctos. `.loopernico`
  conserva `Loopernico.Session`, tipo `Archivo de Loopernico`, comando con
  `%1` hacia `Current` e icono de sesion.

Proximo paso concreto: ninguno; el OLED queda contenido y legible en ambas
escalas.

# Auditoria de estabilidad 2026-07-28

# Ultimo ajuste comprobado: halo PAMPA FUZZ mas visible

- Corregido el 2026-07-28 en `Source/LoopernicoWidgets.cpp`: se aumento
  nuevamente la intensidad del halo celeste activo, manteniendo el degradado
  suave y la circunferencia sin borde rigido. No se modificaron hitbox,
  callback, DSP, parametros, MIDI ni sesiones.

Verificacion y despliegue 2026-07-28:
- Release `x64` correcto con `Builds/VisualStudio2026/Loopernico Refactorizado.sln`,
  0 errores y 0 advertencias.
- `Builds/LocalDeployment/Current` contiene la nueva version, SHA-256
  `A7DDA920DCE15E6FBED0095D0A1674CB0571B9237AED446DFDC94EBF2DD7AE44`.
  `Previous` conserva la version anterior, SHA-256
  `FA7475B3E132D948D042C688616B225B9C4BB395D509A22D30BDD9F7B884B4B2`.
- El humo del ejecutable desplegado permanecio responsivo durante 5 segundos y
  cerro normalmente. El acceso directo se recreo hacia `Current`.

Proximo paso concreto: evaluar en uso diario si esta intensidad ya es la
definitiva.

# Ultimo ajuste comprobado: intensidad intermedia del halo PAMPA FUZZ

- Corregido el 2026-07-28 en `Source/LoopernicoWidgets.cpp`: se aumento
  moderadamente la intensidad del halo celeste del footswitch activo para que
  sea visible sin recuperar el borde duro de la primera version. No se
  modificaron hitbox, callback, DSP, parametros, MIDI ni sesiones.

Verificacion y despliegue 2026-07-28:
- Release `x64` correcto con `Builds/VisualStudio2026/Loopernico Refactorizado.sln`,
  0 errores y 0 advertencias.
- `Builds/LocalDeployment/Current` contiene la nueva version, SHA-256
  `FA7475B3E132D948D042C688616B225B9C4BB395D509A22D30BDD9F7B884B4B2`.
  `Previous` conserva la version anterior, SHA-256
  `312D8F3671489752CE91007C1ADD4F7C296B6AB00E73A40A03502029F9B90DEF`.
- El humo del ejecutable desplegado permanecio responsivo durante 5 segundos y
  cerro normalmente. El acceso directo se recreo hacia `Current`.

Proximo paso concreto: evaluar en uso diario si esta intensidad intermedia es
la definitiva.

# Ultimo ajuste comprobado: halo celeste suavizado del PAMPA FUZZ

- Corregido el 2026-07-28 en `Source/LoopernicoWidgets.cpp`: se redujo la
  intensidad del halo del footswitch activo y se reemplazo la circunferencia
  marcada por un borde muy tenue, conservando una iluminacion celeste suave.
  No se modificaron hitbox, callback, DSP, parametros, MIDI ni sesiones.

Verificacion y despliegue 2026-07-28:
- Release `x64` correcto con `Builds/VisualStudio2026/Loopernico Refactorizado.sln`,
  0 errores y 0 advertencias.
- `Builds/LocalDeployment/Current` contiene la nueva version, SHA-256
  `312D8F3671489752CE91007C1ADD4F7C296B6AB00E73A40A03502029F9B90DEF`.
  `Previous` conserva la version anterior, SHA-256
  `4EF68999EA2EDF1C56F9C39E3F971E0D80327D5273A702E9802903CE39E4B7B9`.
- El acceso directo de Escritorio apunta a `Current`, con directorio de
  trabajo e icono de sesion correctos.
- El humo del ejecutable desplegado permanecio responsivo durante 5 segundos y
  cerro normalmente.

Proximo paso concreto: evaluar en uso diario si la luz queda suficientemente
organica y discreta en miniatura.

# Ultimo ajuste comprobado: halo celeste del interruptor de PAMPA FUZZ

- Corregido el 2026-07-28 en `Source/LoopernicoWidgets.cpp`: cuando `onBtn`
  esta activo, el footswitch del FUZZ recibe un halo radial celeste muy claro
  y un aro tenue alrededor del hardware. El efecto se pinta solo para PAMPA
  FUZZ y no cambia hitbox, callback, DSP, parametros, MIDI ni sesiones.

Verificacion y despliegue 2026-07-28:
- Release `x64` correcto con `Builds/VisualStudio2026/Loopernico Refactorizado.sln`,
  0 errores y 0 advertencias.
- `Builds/LocalDeployment/Current` contiene la nueva version, SHA-256
  `4EF68999EA2EDF1C56F9C39E3F971E0D80327D5273A702E9802903CE39E4B7B9`.
  `Previous` conserva la version funcional anterior, SHA-256
  `6AD3049E3A3CE09587B36CF97796446F9E6F661F3C3C4287443901A8341FEB62`.
- El acceso directo `C:\Users\Fernando\Desktop\Loopernico.lnk` apunta a
  `Current`, con directorio de trabajo e icono de sesion correctos.
- La asociacion `.loopernico` conserva `Loopernico.Session`, tipo `Archivo de
  Loopernico` y comando con `%1` hacia `Current`.
- El ejecutable desplegado se lanzo y cerro mediante la ventana principal. El
  modo automatico `--snapshot-ui` no genero PNG ni respondio en esta corrida,
  por lo que no se declara captura visual automatica del estado encendido.

Proximo paso concreto: confirmar en uso diario que la intensidad del halo sea
la adecuada en miniatura y vista ampliada.

# Ajuste comprobado: rotulos y perillas del PAMPA FUZZ

- El frente activo es `Assets/Pedals/pampa-fuzz-face-v8.png`, editado con
  ImageGen sobre la v7 para aumentar aproximadamente 60 % la altura y peso de
  `QUILOMBO`, `TONO` y `VOLUMEN`, manteniendo su arco, ortografia y el resto
  del frente.
- Las tres perillas pasaron de `0.200` a `0.300` del ancho del cuerpo y sus
  centros bajaron de `0.252` a `0.271` de la altura. El hardware visible cubre
  ahora las circunferencias impresas tanto en miniatura como en vista HD.
- No se modificaron DSP, valores, orden, MIDI ni compatibilidad de sesiones.

Verificacion y despliegue 2026-07-28:
- Release `x64` correcto con
  `Builds/VisualStudio2026/Loopernico Refactorizado.sln`, codigo 0.
- Capturas revisadas:
  `tmp/pampa-fuzz-v8-final-compact.png` y
  `tmp/pampa-fuzz-v8-final-focused.png`. Los tres nombres son legibles en la
  cadena y las perillas cubren los aros sin tapar el texto.
- Humo del ejecutable desplegado correcto mediante
  `tmp/pampa-fuzz-v8-deployed-smoke.png`; cierre normal.
- `Builds/LocalDeployment/Current` SHA-256:
  `6AD3049E3A3CE09587B36CF97796446F9E6F661F3C3C4287443901A8341FEB62`.
  `Previous` conserva la version funcional anterior:
  `208C2E859DA7EDBA84C33F559A12364265C039784D7F456576DB69B01BBACA9A`.
  Hay exactamente dos ejecutables desplegados.
- Acceso directo recreado hacia `Current`. La asociacion `.loopernico`
  conserva `Loopernico.Session`, tipo `Archivo de Loopernico`, comando con
  `%1` hacia `Current` e icono de sesion.

# Estado comprobado: PAMPA FUZZ segun referencia, en medida Noise Suppressor

- El frente activo es `Assets/Pedals/pampa-fuzz-face-v7.png`, regenerado de
  forma nativa a `976x1611` (proporcion del Noise Suppressor), sin estirar el
  asset anterior. La carcasa conserva textura marfil envejecida, bordes
  celestes finos, sol recortado a la izquierda y serigrafia filete dominante.
- Los rotulos visibles son `QUILOMBO`, `TONO` y `VOLUMEN`, arqueados alrededor
  de cada perilla. Los parametros internos siguen siendo fuzz, tone y level
  para preservar DSP, MIDI y sesiones existentes.
- El fuzz usa el mismo ancho compacto `156` y la misma proporcion exterior que
  el Noise Suppressor. Los knobs se centran en `0.200`/`0.500`/`0.800`, a
  `0.252` de la altura. Se retiraron la bandera y el aro celeste del switch
  porque no aparecen en la referencia.

Verificacion y despliegue 2026-07-28:
- Release `x64` correcto con
  `Builds/VisualStudio2026/Loopernico Refactorizado.sln`, codigo 0.
- Capturas revisadas:
  `tmp/pampa-fuzz-v7-native-compact.png` y
  `tmp/pampa-fuzz-v7-native-focused.png`. Fuzz y Noise Suppressor ocupan el
  mismo rectangulo exterior; los aros son circulares, los knobs calzan y la
  serigrafia no invade el footswitch.
- Humo del ejecutable desplegado correcto mediante
  `tmp/pampa-fuzz-v7-deployed-smoke.png`; coincide byte a byte con la captura
  enfocada del candidato y el proceso cerro normalmente.
- `Builds/LocalDeployment/Current` SHA-256:
  `208C2E859DA7EDBA84C33F559A12364265C039784D7F456576DB69B01BBACA9A`.
  `Previous` conserva la version funcional anterior:
  `10DC3CE839EAE998ECC7162CC0D23202294F640BC6A666BE692E9F21522AA0A8`.
  Hay exactamente dos ejecutables desplegados.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` fue recreado hacia `Current`, con
  directorio de trabajo e icono de sesion correctos. La asociacion
  `.loopernico` conserva `Loopernico.Session`, tipo `Archivo de Loopernico`,
  comando con `%1` hacia `Current` e icono de sesion.

Proximo paso concreto: evaluar el nuevo frente en uso diario a escala compacta.

# Ultimo ajuste en curso: PAMPA FUZZ vertical segun referencia de Fernando

- Corregido el 2026-07-28: se genero `Assets/Pedals/pampa-fuzz-face-v5.png` a
  partir de la referencia `fuzz.png`, con carcasa vertical angosta de proporción
  aproximada `471:559`, textura continua y sin recortes de asset horizontal.
- Los rótulos visibles del asset ahora son `QUILOMBO`, `CHAMUYO` y `MANGO`.
  Los parámetros internos siguen siendo fuzz, tone y level para conservar
  sesiones, MIDI y DSP.
- La integración usa el crop completo del nuevo asset; los tres knobs se
  centran en `0.300`/`0.500`/`0.700` y bajan a `0.225` de la altura del cuerpo.
  El footswitch se centra en `0.500`, baja a `0.885` y recibe un aro celeste
  que se ilumina al activar el pedal. La bandera existente se conserva a la
  derecha del switch.

Verificación final 2026-07-28:
- Release `x64` correcto con `Builds/VisualStudio2026/Loopernico Refactorizado.sln`.
- Captura HD revisada: `tmp/pampa-fuzz-vertical-v5-focused.png`; no hay
  solapamiento entre rótulos y knobs, la textura se ve unificada y el aro se
  dibuja alrededor del switch.
- Humo del ejecutable desplegado correcto: `tmp/pampa-fuzz-vertical-v5-deployed.png`,
  cierre normal y código 0.
- `Builds/LocalDeployment/Current` SHA-256:
  `10DC3CE839EAE998ECC7162CC0D23202294F640BC6A666BE692E9F21522AA0A8`.
  `Previous` conserva la versión funcional anterior:
  `3D87EDD9F5B02BF5A49EE0FFABBC98FB853994F7B115C025B9723D3FECDCBC21`.
- El acceso directo apunta a `Current`, con directorio de trabajo e icono de
  sesión correctos. Se conservaron exactamente dos ejecutables funcionales.

Próximo paso concreto: evaluar en uso real la escala del aro celeste y la
ubicación de la bandera respecto del switch.

# Ultimo ajuste en curso: geometria final de PAMPA FUZZ

- Se acercaron los centros de las tres perillas a `0.300`/`0.500`/`0.700` y
  se aumento su tamano de `0.175` a `0.200` del ancho del cuerpo.
- El footswitch visual se desplazo a la izquierda (`0.400`) y mas abajo
  (`0.840`); la bandera dinamica se ubico a su derecha (`0.560`) dentro de la
  misma franja inferior. El hitbox funcional permanece sin cambios.

Verificacion final 2026-07-28:
- Release `x64` correcto con `Builds/VisualStudio2026/Loopernico Refactorizado.sln`.
- El ejecutable Release y `Builds/LocalDeployment/Current` permanecieron vivos
  y responsivos durante 8 s; se cerraron despues de la comprobacion.
- `Current` SHA-256: `3D87EDD9F5B02BF5A49EE0FFABBC98FB853994F7B115C025B9723D3FECDCBC21`.
  `Previous` conserva la version funcional anterior:
  `A25BA5E666D9868C6E30B9EEAFD1F42EAC49D1158020AD34BAEB12292B145640`.
- El acceso directo apunta a `Current`, con directorio de trabajo e icono
  correctos; la asociacion `.loopernico` no fue alterada.

Proximo paso concreto: evaluar en uso real el espacio entre el switch y la
bandera cuando el pedal se muestra en tamaño minimo.

# Ultimo avance: rediseño visual de PAMPA FUZZ

- Generado `Assets/Pedals/pampa-fuzz-face-v4.png`: se retiro “EDICION
  ARGENTINA” y se incorporo una tipografia filete porteño para “PAMPA FUZZ”,
  conservando carcasa, etiquetas y textura. El asset deja libre la zona de la
  bandera dinamica para evitar duplicacion.
- Integrado el frente v4 en `Source/LoopernicoWidgets.cpp`. La bandera conserva
  su comportamiento de encendido y ahora se dibuja en proporcion 3:2, en vez de
  la huella 2:1 que la aplastaba.

Verificacion final 2026-07-28:
- Release `x64` correcto con `Builds/VisualStudio2026/Loopernico Refactorizado.sln`.
- El ejecutable Release y `Builds/LocalDeployment/Current` permanecieron vivos
  y responsivos durante 8 s; se cerraron despues de la comprobacion.
- `Current` SHA-256: `A25BA5E666D9868C6E30B9EEAFD1F42EAC49D1158020AD34BAEB12292B145640`.
  `Previous` conserva la version funcional anterior:
  `83D66A96AA554A018D7F6DDADDAB1A745A8FE56D61E4545CFCF3E2BA216DEA83`.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` apunta a `Current`, con
  directorio de trabajo e icono `Assets/Brand/Icons/loopernico-session.ico`.
- `.loopernico` conserva `Loopernico.Session` y el comando con `%1` hacia
  `Current`.
- La captura interactiva no pudo realizarse por la interfaz de Windows
  `0x80004002`; el asset generado fue inspeccionado directamente y no presenta
  texto extra, subtitulo ni bandera integrada duplicada.

Proximo paso concreto: evaluar en uso real si el tamaño final de la marca y la
placa de bandera resultan cómodos en miniatura y vista ampliada.

# Ultimo ajuste comprobado: escala y centrado final de potes PAMPA FUZZ

- Corregido el 2026-07-28: los tres potes fileteados pasan a `0.175` del ancho
  del cuerpo y los extremos se acercan a `0.276`/`0.724`, conservando el centro
  de TONE en `0.500`. El diseño destaca mas sin tocar etiquetas, DSP,
  parametros, sesiones, MIDI ni hitboxes.
- Capturas revisadas: `tmp/pampa-fuzz-filete-knob-hd-v2.png` y
  `tmp/pampa-fuzz-filete-knob-deployed-v2.png`; no hay solapamientos y el
  fileteado permanece legible.

## Verificacion y despliegue

- Release `x64` correcto con `Builds/VisualStudio2026/Loopernico Refactorizado.sln`.
- `Builds/LocalDeployment/Current` contiene la nueva version y `Previous`
  conserva la anterior. El humo del desplegado genero la captura HD.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` apunta a `Current`, con
  directorio de trabajo e icono de sesion correctos. `.loopernico` conserva
  `Loopernico.Session`, el tipo de archivo y el comando con `%1` hacia
  `Current`.

Proximo paso concreto: evaluar en uso real si el tamaño final se siente cómodo
  para arrastre y, si hace falta, ajustar solo el detalle ornamental del asset.

# Ultimo ajuste comprobado: potes fileteados de PAMPA FUZZ

- Integrado el 2026-07-28 `Assets/Hardware/pampa-fuzz-filete-knob-v1.png` en
  los tres controles de FUZZ. El PNG se escala al bounds existente y rota con
  el valor del control, compensando el marcador base del asset. No se cambiaron
  DSP, parametros, sesiones, MIDI, hitboxes ni la textura de la carcasa.
- Capturas revisadas: `tmp/pampa-fuzz-filete-knob-hd-v1.png` y
  `tmp/pampa-fuzz-filete-knob-deployed-v1.png`; los tres potes quedan alineados,
  legibles y coherentes con el fileteado y la bandera del pedal.

## Verificacion y despliegue

- Release `x64` correcto con `Builds/VisualStudio2026/Loopernico Refactorizado.sln`.
- `Builds/LocalDeployment/Current` contiene la nueva version y `Previous`
  conserva la version funcional inmediatamente anterior. El humo del
  desplegado genero la captura HD correctamente.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` apunta a `Current`, con
  directorio de trabajo e icono de sesion correctos. `.loopernico` conserva
  `Loopernico.Session`, el tipo de archivo y el comando con `%1` hacia
  `Current`.

Proximo paso concreto: probar el recorrido de los potes con valores distintos
  y decidir si hace falta una variante de fileteado mas sobria para miniatura.

# Avance actual: primer pote fileteado para PAMPA FUZZ

- Generado el 2026-07-28 el candidato rasterizado
  `Assets/Hardware/pampa-fuzz-filete-knob-v1.png`: baquelita negra envejecida,
  estriado radial, aro de bronce, marcador marfil y ornamentacion celeste/dorada
  inspirada en el fileteado porteño.
- El PNG es `1254x1254`, `32bppArgb`, con esquinas transparentes y sin halo verde
  visible. Todavia no se integro al codigo ni se reemplazaron los potes actuales.

Proximo paso concreto: probar este asset en los tres controles de PAMPA FUZZ y
revisar escala, contraste y legibilidad antes de decidir si se conserva o se
genera una variante mas sobria.

# Ultimo ajuste comprobado: alineacion de potenciometros de PAMPA FUZZ

- Corregido el 2026-07-28: se mantuvo `Assets/Pedals/pampa-fuzz-face-v3.png`,
  que fue revisado y no contiene dibujos ni huecos de potenciometros. Los tres
  controles interactivos de FUZZ ahora usan tamano proporcional al cuerpo y se
  centran bajo `FUZZ`, `TONE` y `LEVEL` en las coordenadas de la textura. No se
  cambiaron asset, DSP, nombres, sesiones, MIDI ni hitboxes.
- Capturas revisadas: `tmp/pampa-fuzz-alignment-v2.png` y
  `tmp/pampa-fuzz-deployed-smoke.png`; los tres nombres quedan legibles, sin
  solapamiento, y cada knob queda alineado con su etiqueta.

## Verificacion y despliegue

- Release `x64` correcto con `Builds/VisualStudio2026/Loopernico Refactorizado.sln`.
- `Builds/LocalDeployment/Current` contiene la nueva version y `Previous`
  conserva la version funcional anterior. El humo del desplegado genero la
  captura HD y la interfaz permanecio operativa durante el snapshot.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` apunta a `Current`, con
  directorio de trabajo e icono de sesion correctos. `.loopernico` conserva
  `Loopernico.Session`, el tipo de archivo y el comando con `%1` hacia
  `Current`.

Proximo paso concreto: redisenar los potenciometros de PAMPA FUZZ para que
  encajen con la estetica del pedal, manteniendo esta geometria como base.

# Ultimo ajuste comprobado: indicador argentino de PAMPA FUZZ

- Corregido el 2026-07-28: se retiro el LED generico inferior de `FUZZ`, que
  quedaba debajo del footswitch en la miniatura. Se agrego la placa rasterizada
  `Assets/Hardware/pampa-argentina-flag-v1.png`, generada como asset
  complementario con transparencia, y se dibuja centrada en la franja libre
  sobre el switch.
- La bandera se mantiene tenue apagada y recibe halo celeste y retroiluminacion
  interna cuando `onBtn` esta activo. No se cambiaron controles, DSP, sesiones,
  MIDI ni hitboxes.

## Verificacion y despliegue

- Release `x64` correcto con `Builds/VisualStudio2026/Loopernico Refactorizado.sln`.
- Captura revisada: `tmp/pampa-fuzz-flag-v1.png`; el asset carga correctamente
  en vista HD. La geometria final fue elevada y reducida para despejar el
  footswitch; la captura v2 no se regenero por una segunda instancia que no
  completo el ciclo de snapshot, pero el cambio solo ajusta esas coordenadas.
- `Builds/LocalDeployment/Current` SHA-256:
  `E3D4A072B2AA3EDB188235254E6773709D3821052BBEBB83BCF399350DB607C5`.
  `Previous` conserva la version funcional anterior:
  `3E87253CDAEACA3DA5F0482F89251B8EAC5DBF6A8CE6CFA756B651673D952CA0`.
- Humo de `Current`: ventana abierta y responsiva durante 5 s; cierre forzado
  despues de la comprobacion. El acceso directo apunta a `Current`, con
  directorio de trabajo e icono de sesion correctos. `.loopernico` conserva
  `Loopernico.Session`, tipo `Archivo de Loopernico` y comando con `%1`.

Proximo paso concreto: confirmar visualmente el estado encendido de la bandera
en una captura o sesion interactiva si se desea una comprobacion adicional.

- Revision estatica actual: no se encontro una ruta de crash confirmada en el flujo normal de Release; el humo previo solo cubre apertura y responsividad basica.
- Riesgo concreto pendiente: `EffectsChain::processBlock()` y `LooperEngine::processBlock()` convierten buffers a punteros crudos sin validar internamente `startSample + numSamples` contra el tamano real. El callback actual los llama con `startSample=0` y buffers preparados, por lo que no queda reproducido en el flujo normal, pero la API puede desbordar si recibe un rango invalido.
- Riesgo adicional: `LooperEngine::rotateTrackRegion()` y `trimSilence()` presuponen que `regionLengthSamples`/`recordedSamples` caben en el buffer; sus llamadores actuales usan longitudes derivadas del loop y del limite de captura, sin evidencia de desborde en la ruta normal.
- Verificacion realizada: inspeccion estatica de `EffectsChain`, `LooperEngine`, `MainComponent`, sesiones MIDI/VST3 y estado Git; no se modifico codigo fuente. Proximo paso si se busca blindaje: agregar validacion de rangos y una prueba adversarial de callbacks/sesiones.

# Ultimo ajuste comprobado: estilo unificado de botones inferiores

- Corregido el 2026-07-27 en `MainComponent`: `MIDI +`, `MONO`/`STEREO` y
  `MUTE ALL` usan ahora el mismo estilo de boton de consola, con iguales
  altura y ancho, y quedan alineados en la franja inferior. Se conservaron los
  colores de estado, textos dinamicos y comportamientos existentes.
- Se genero y reviso la captura `Builds/VisualStudio2026/x64/Release/App/tmp/footer-buttons-unified.png`:
  los tres botones muestran carcasa, tipografia e iluminacion coherentes.

## Verificacion y despliegue

- Release `x64` correcto con `Builds/VisualStudio2026/Loopernico Refactorizado.sln`.
- Prueba de humo del ejecutable de Release y de `Builds/LocalDeployment/Current`:
  ambos permanecieron abiertos y responsivos durante 8 s y se cerraron luego.
- `Current` SHA-256:
  `3E87253CDAEACA3DA5F0482F89251B8EAC5DBF6A8CE6CFA756B651673D952CA0`.
  `Previous` conserva la version funcional anterior:
  `FEA4B0B93E1785F20B67B8729BD65125A304705C4A3C013B54C8D337968ABC2D`.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` apunta a `Current`, con
  directorio de trabajo e icono de sesion correctos. La asociacion
  `.loopernico` conserva `Loopernico.Session`, tipo `Archivo de Loopernico` y
  el comando con `%1` hacia `Current`.

# Ultimo ajuste comprobado: MUTE ALL silencia toda la salida

- Corregido el 2026-07-27 en `MainComponent`: `MUTE ALL` ahora silencia la
  salida completa despues de mezclar entrada monitorizada, looper,
  efectos/VST3 y ritmo. La grabacion y el procesamiento continuan activos.
- El boton ahora funciona como toggle, sincroniza su estado con el looper y
  usa el estilo rojo cuando esta activo. El control MIDI historico conserva
  el mismo comportamiento de alternancia.
- No se cambiaron las pistas individuales, el DSP de los efectos, las
  sesiones ni el ruteo de grabacion.

## Verificacion y despliegue

- Release `x64` correcto con `Builds/VisualStudio2026/Loopernico Refactorizado.sln`.
- Prueba de humo del ejecutable de Release y de `Builds/LocalDeployment/Current`:
  ambos permanecieron abiertos y responsivos durante 8 s y se cerraron luego.
- `Current` SHA-256:
  `FEA4B0B93E1785F20B67B8729BD65125A304705C4A3C013B54C8D337968ABC2D`.
  `Previous` conserva la version funcional anterior:
  `D47332F06CEDD01D8BF860D81F2CBD56CDF18C1C0CE386A58FB5F77C62B31FCA`.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` apunta a `Current`, con
  directorio de trabajo e icono de sesion correctos. La asociacion
  `.loopernico` debe conservar `Loopernico.Session`, tipo `Archivo de
  Loopernico` y el comando con `%1` hacia `Current`.

# Contexto operativo de Loopernico

## Avance 2026-07-29: biblioteca de presets por pedal

- Se agregaron controles de presets en una franja propia debajo de cada pedal
  interno: desplegable `PRESETS` y boton `+` para guardar la configuracion
  actual con un nombre. La franja se reserva en el layout y no cubre los
  assets ni los controles fisicos.
- Los presets se guardan separadamente de las sesiones en el XML de datos de
  usuario `Loopernico/pedal-presets.xml`; contienen bypass, knobs, EQ y estados
  especiales VOX/Pico cuando corresponda. Elegir un preset aplica sus valores
  al DSP activo sin modificar el archivo `.loopernico`.
- Los VST3 quedan fuera de esta primera biblioteca: sus estados continuan
  guardandose dentro de la sesion; un banco independiente de presets VST3
  requiere un flujo separado por plugin.

Verificacion:
- Release `x64` compilado correctamente, 0 errores y 0 advertencias.
- Prueba de humo normal: el ejecutable permanecio responsivo durante 15 s y
  cerro normalmente.
- `git diff --check` correcto, con los avisos preexistentes de normalizacion
  LF/CRLF.

Proximo paso concreto: validar visualmente la franja inferior en distintos
pedales y, si se desea, implementar bancos independientes para estados VST3.

## Tarea terminada 2026-07-29: presets independientes para VST3

- Cada uno de los cuatro slots VST3 ahora tiene desplegable y boton `+` debajo
  del host, en la misma franja reservada que los pedales internos.
- El preset externo guarda nombre, ruta del plugin, bypass y el estado binario
  completo del VST3 en `Loopernico/pedal-presets.xml`, sin incorporarlo al
  archivo `.loopernico`.
- Al aplicar un preset se verifica que la ruta corresponda al plugin cargado;
  los estados de otro plugin se rechazan con un aviso. La restauracion usa el
  slot correcto y actualiza la latencia del plugin.

Verificacion y despliegue:
- Release `x64` correcto, 0 errores; permanecen solamente las dos advertencias
  C4244 preexistentes de `Source/LatencyCompensation.cpp:188`.
- Humo del candidato y de `Builds/LocalDeployment/Current`: ambos responsivos
  durante 15 s y cerrados manualmente sin salida prematura.
- `Current` actualizado y `Previous` conserva la version funcional anterior.
- El acceso directo de Escritorio sigue apuntando a `Current`, con el mismo
  directorio de trabajo e icono.

Proximo paso concreto: probar con un VST3 real (por ejemplo TONEX) guardar dos
estados, alternarlos y confirmar auditivamente la restauracion.

## Correccion 2026-07-29: visibilidad de la franja de presets

- La primera geometria reservaba el alto adicional fuera del viewport y dejaba
  los desplegables y botones recortados en el borde inferior.
- Se corrigio el calculo para que el cuerpo del pedal reduzca su alto dentro
  del viewport y la franja `PRESETS` quede completamente visible debajo, sin
  cubrir ningun asset.
- Release, humo del candidato y humo de `Current` correctos; `Current` fue
  actualizado nuevamente y `Previous` conserva el respaldo.

## Ultimo ajuste comprobado: nombre MIDI de Gamechanger Audio Plus II

- Corregido el 2026-07-27: en la configuracion MIDI, la accion que antes se
  mostraba como `PEDAL SUSTAIN CHORDS` ahora se muestra como
  `PEDAL GAMECHANGER AUDIO PLUS II`.
- El identificador interno, el nombre del pedal en la cadena, el DSP, las
  sesiones y el resto del mapa MIDI no se modificaron.

### Verificacion y despliegue

- Release `x64` correcto con `Builds/VisualStudio2026/Loopernico Refactorizado.sln`.
- `Current` SHA-256:
  `BDD888F551863821877B993FF20526ABF784C4AE06A0A03CFC55E1696A4D1F9D`.
  `Previous` conserva la version funcional anterior:
  `40F0270B952DFF7E1D3EDA96B9033BE86A819B907BF81BF919D82FA085C6B029`.
- Ambos ejecutables permanecieron abiertos y responsivos durante 8 s y luego
  se cerraron. El acceso directo apunta a `Current`, con directorio de trabajo
  e icono de sesion correctos. `.loopernico` conserva `Loopernico.Session` y
  el comando con `%1` hacia `Current`.
- `git diff --check`: correcto; solo permanece el aviso preexistente de
  normalizacion LF/CRLF.

## Ultimo ajuste comprobado: zona superior invisible para arrastrar pedales

- Corregido el 2026-07-27: la zona superior de cada `FXPedalComponent` conserva
  la logica de arrastre existente, pero no dibuja manija, brillo ni texto. Asi
  el pedal sigue siendo agarrable sin agregar ruido visual ni tapar los assets.
- No se cambiaron el tamano del asset, los controles, el orden de la cadena ni
  el comportamiento de audio.

### Verificacion y despliegue

- Release `x64` correcto con `Builds/VisualStudio2026/Loopernico Refactorizado.sln`.
- El ejecutable Release y `Builds/LocalDeployment/Current` permanecieron
  abiertos y responsivos durante 8 s; ambos se cerraron despues de la prueba.
- `Current` SHA-256:
  `40F0270B952DFF7E1D3EDA96B9033BE86A819B907BF81BF919D82FA085C6B029`.
  `Previous` conserva la version funcional anterior:
  `F2931354ECF742271FA6CF779E5269BB1D185DDA5BD44ACA4171EE493C51C178`.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` sigue apuntando a `Current`,
  con directorio de trabajo e icono de sesion correctos. `.loopernico` conserva
  `Loopernico.Session`, el tipo `Archivo de Loopernico` y el comando con `%1`.
- La confirmacion visual interactiva del arrastre queda pendiente porque el
  controlador de Windows no esta disponible en esta sesion.

## Ultimo ajuste comprobado: escala visual de SUSTAIN CHORDS

- Corregido el 2026-07-27: se recortaron al dibujar los margenes laterales del
  PNG de Gamechanger Audio Plus Pedal II y se amplio la carcasa visible dentro
  de la huella de 110 px, manteniendo la proporcion real del recorte (0.45) y
  el espaciado general de 28 px entre pedales.
  La textura original permanece intacta; tambien se reajustaron los seis
  overlays y el hitbox de la lengueta dorada.
- No se modificaron DSP, parametros, sesiones, MIDI ni otros pedales.

### Verificacion y despliegue

- `git diff --check`: correcto; solo avisos preexistentes de normalizacion LF/CRLF.
- Release `x64` correcto con `Builds/VisualStudio2026/Loopernico Refactorizado.sln`.
- Humo de Release y `Builds/LocalDeployment/Current`: ambos permanecieron
  abiertos y responsivos durante 8 s; se cerraron despues de la comprobacion.
- Capturas revisadas: `tmp/gamechanger-plus-pedal-ii-overview-v3.png` y
  `tmp/gamechanger-plus-pedal-ii-compact-v3.png` y
  `tmp/gamechanger-plus-pedal-ii-compact-v4.png` y
  `tmp/gamechanger-plus-pedal-ii-overview-v5.png`; la carcasa conserva su
  proporcion y los margenes visuales coinciden con los pedales vecinos.
- `Current` SHA-256: `49603A44E326C374CD73EA9D60AE6477CF0B814758D8DE9682093E55829C73DB`.
  `Previous` conserva la version funcional anterior:
  `A522E6F88BE868ABD5B427749447E15640CE2600D5778C672D14E492CF799E80`.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` apunta a `Current`, con
  directorio de trabajo e icono de sesion correctos. `.loopernico` conserva
  `Loopernico.Session`, tipo `Archivo de Loopernico`, icono de sesion y comando
  con `%1` hacia `Current`.

## Ultimo ajuste comprobado: Gamechanger Audio Plus Pedal II, geometria v2

- Corregido el 2026-07-27: la tira compacta usa una huella de componente de
  110 px para `SUSTAIN CHORDS`; esta geometria fue posteriormente ampliada
  visualmente mediante un recorte de los margenes del PNG.
  `fxPedalPitch()` incluye la nueva huella en `fxContainer` y en el orden de
  arrastre.
- Medicion sobre el canvas completo: LEVEL (318,190), TONE (~334,315), RISE
  (318,450), LAYERS (712,189), RESONANCE (~697,315) y DECAY (713,448).
  Las posiciones normalizadas y los tamanos de los overlays se aplican igual
  en miniatura y vista HD; la vista HD no vuelve a escalar la miniatura.
- Se mantienen los controles rasterizados grandes/mini y el mapeo historico:
  LEVEL=knob1/`chordFreezeMix`, RISE=knob2/`chordFreezeRiseMs`, DECAY=knob3/
  `chordFreezeTailMs`, TONE=knob4/`chordFreezeTone`, RESONANCE=knob5/
  `chordFreezeWindowMs`, LAYERS=knob6/`chordFreezeCrossfade`.
- No se agregaron LED, footswitch redondo, etiquetas ni controles. La lengueta
  dorada sigue siendo el unico switch/bypass; sesiones, MIDI e identificador
  interno siguen usando `SUSTAIN CHORDS`.
- El hitbox generico inferior no puede activar este pedal: fuera de la lengueta
  dorada no hay otra zona de bypass.

### Verificacion y despliegue

- `git diff --check`: correcto; solo avisos preexistentes de normalizacion
  LF/CRLF.
- Release `x64` correcto con `Builds/VisualStudio2026/Loopernico Refactorizado.sln`.
- Capturas revisadas mediante `createComponentSnapshot`:
  `tmp/gamechanger-plus-pedal-ii-compact-v2.png` SHA-256
  `75CDC8F10FA1934CFE52056B89239CD1B50B4A9BB2C4680B978A085CE6327644` y
  `tmp/gamechanger-plus-pedal-ii-hd-v2.png` SHA-256
  `4B5669AEC295E5C4CAD1D0EB9ECE65971B1689AF121DAE993A4627554429F0A3`.
  Ambas muestran proporciones consistentes, seis knobs sin duplicacion,
  ausencia de recortes/deformacion y ningun texto antiguo.
- Humo del Release y de `Builds/LocalDeployment/Current`: ventana visible y
  responsiva durante al menos 8 s; ambos cerraron con codigo 0.
- `Builds/LocalDeployment/Current` SHA-256:
  `A91E5014EFDE2674E3ECFB47982120B1DFE752C7493D06A0962CD6F725C91E74`.
  `Previous` conserva la version funcional inmediatamente anterior:
  `A71D045367353D544FB2E1DB52783FD879E578646012A901CCFBE0C80BE65349`.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` apunta a `Current`, con
  directorio de trabajo e icono `Assets/Brand/Icons/loopernico-session.ico`.
  `.loopernico` conserva `Loopernico.Session`, tipo `Archivo de Loopernico`,
  icono de sesion y comando con `%1` hacia `Current`.

Limitacion real: la captura se hizo con el snapshot interno de JUCE; no se
declara una prueba interactiva de arrastre/clic porque el controlador visual de
Windows sigue sin estar disponible en esta sesion.

## Ultimo ajuste comprobado: manual y nombres de SUSTAIN CHORDS

- Corregido el 2026-07-27: los controles visibles de `SUSTAIN CHORDS` ahora se
  llaman `LEVEL`, `FADE IN`, `FADE OUT`, `TONE`, `LENGTH` y `SMOOTH`, sin cambiar
  sus rangos ni su comportamiento DSP.
- Se agrego un boton pequeno `?` dentro de la carcasa. Abre una ayuda breve en
  espanol con la funcion de cada control.
- La captura final confirma que las etiquetas nuevas reemplazan a las antiguas
  incrustadas en la textura y que el boton no invade los knobs.

### Verificacion y despliegue

- Release `x64` correcto con `Builds/VisualStudio2026/Loopernico Refactorizado.sln`.
- El ejecutable de `Builds/LocalDeployment/Current` permanecio abierto durante
  8 segundos y la captura HD del pedal fue generada y revisada correctamente.
- `Current` SHA-256:
  `6D39AC27FCE52D13C93B4D2094A654FF026F56594F53D7D87330C5FBF8F164DA`.
  `Previous` conserva la version funcional anterior:
  `8685BE601652B2ED338BCF357E8A685241EEB9B13CECDB942416898C20F4475A`.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` apunta a `Current`, con
  directorio de trabajo e icono de sesion correctos. La asociacion `.loopernico`
  conserva `Loopernico.Session`, `Archivo de Loopernico` y el comando hacia
  `Current`.

Proximo paso concreto: confirmar manualmente el clic en `?` y la legibilidad de
  las etiquetas en el pedal maximizado si se desea una comprobacion interactiva.

## Ultimo ajuste comprobado: retiro de Sustain Gran y correccion de cadena

- Corregido el 2026-07-26: se retiro `SUSTAIN GRAN` de la interfaz, del orden
  activo de pedales y del selector MIDI. Su indice historico queda reservado
  como `REMOVED` para no desplazar sesiones antiguas.
- Corregido el mismo dia un fallo de conteo introducido durante ese retiro: la
  cadena activa ahora cuenta por separado los dos indices reservados
  (`RhythmPedal` y `LegacySustainGranPedal`), evitando que el orden publicado
  sea rechazado y dejando nuevamente operativos el Noise Gate y los efectos.
- `SUSTAIN CHORDS` permanece visible, configurable, procesado por DSP y
  disponible desde MIDI. Las sesiones que contengan el antiguo indice de
  Sustain Gran ahora lo ignoran al cargarse.

### Verificacion y despliegue

- Release `x64` correcto con `Builds/VisualStudio2026/Loopernico Refactorizado.sln`,
  0 errores y 0 advertencias.
- El ejecutable Release y `Builds/LocalDeployment/Current` permanecieron
  abiertos y responsivos durante 8 s.
- `Current` SHA-256:
  `E85162653E0D27D3FE35E2A6E28234C36C51A4586E14DCA909100BAA3B48755A`.
  `Previous` conserva la version funcional anterior:
  `B810125E799CECED4EBA1190F2CED8E6DA9FD29C8FE40762D695DCF557C0DAF7`.
- El acceso directo apunta a `Current`, con directorio de trabajo e icono de
  sesion correctos. `.loopernico` conserva `Loopernico.Session`, el tipo
  `Archivo de Loopernico`, el comando hacia `Current` y el icono de sesion.

Proximo paso concreto: confirmar manualmente que `SUSTAIN CHORDS` responde al
footwitch y que `SUSTAIN GRAN` ya no aparece en la tira de pedales.

## Ultimo ajuste comprobado: escalado de OUTPUT EQ en vista HD y miniatura

- Corregido el 2026-07-26 en `FXPedalComponent` y `LoopernicoLAF`: la
  carcasa MXR de `OUTPUT EQ` conserva una geometria comun entre miniatura y
  vista maximizada, usando el tamano real de la miniatura como referencia y
  tipografia HD limitada para evitar solapamientos.
  Los faders, rieles, capsulas y halos escalan desde el tamano real del
  componente, y el layout de los sliders coincide con las posiciones pintadas.
- No se modificaron el DSP, los parametros, las sesiones ni otros pedales.

### Verificacion y despliegue

- Release `x64` correcto con `Builds/VisualStudio2026/Loopernico Refactorizado.sln`,
  0 errores y 0 advertencias.
- Capturas revisadas: `tmp/eq-hd-fixed.png` y `tmp/eq-compact-fixed.png`; la
  carcasa, etiquetas y faders quedan proporcionados y legibles sin escalar dos
  veces la miniatura.
- Humo de `Builds/LocalDeployment/Current`: ventana valida y responsiva durante
  8 s; cierre normal con codigo 0.
- `Current` SHA-256:
  `49227A21D0CA21173D8DA43FB90ED410E395A4FA27F7B5E63A9FF4C19C704726`.
  `Previous` conserva la version funcional anterior:
  `D3F18D8D5DEA165AA526CA1F20116C036C4A89D5AA8A6A0050D1A710F6E6D4EE`.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` apunta a `Current`, con
  directorio de trabajo e icono de sesion correctos. `.loopernico` conserva
  `Loopernico.Session`, tipo `Archivo de Loopernico` y el comando con `%1`.

Proximo paso concreto: confirmar manualmente el arrastre de los faders si se
desea una comprobacion interactiva adicional.

## Ultimo ajuste comprobado: retiro del LED verde en STOP de patrones VST3

- Corregido el 2026-07-26 en `LoopernicoLookAndFeel`: el boton de transporte
  del patron VST3 conserva el cuadrado rojo de `STOP`, pero ya no recibe el
  LED verde lateral del estilo general de botones toggle. `PLAY` mantiene su
  triangulo verde y no se cambio el comportamiento MIDI.

### Verificacion y despliegue

- Release `x64` correcto con `Builds/VisualStudio2026/Loopernico Refactorizado.sln`.
- `git diff --check` correcto, con solo avisos preexistentes de LF/CRLF.
- `Current` SHA-256:
  `D3F18D8D5DEA165AA526CA1F20116C036C4A89D5AA8A6A0050D1A710F6E6D4EE`.
  `Previous` conserva la version funcional anterior:
  `E159A9D04BA81B318066988C023D4B7AA1840924BE28B52F0E5E3D1461F44718`.
- El ejecutable nuevo permanecio estable y responsivo durante 12 s y cerro
  con codigo 0. El acceso directo y `.loopernico` apuntan a `Current`.

Proximo paso concreto: confirmar visualmente el boton STOP en la carcasa VST3
si se desea una comprobacion manual adicional.

## Ultimo ajuste comprobado: retiro de swing de los secuenciadores VST3

- Corregido el 2026-07-26: `PATRON MIDI` ya no muestra ni modifica SWING. La
  grilla queda como unico editor de pasos, compases y paginas del patron.
- El patron VST3 ya no guarda porcentaje de swing ni retrasa el segundo paso
  de cada pareja; los eventos MIDI se programan equidistantes según el tempo.
  Las sesiones antiguas siguen leyendo `patternData` y simplemente ignoran la
  propiedad obsoleta `patternSwing`.
- Se retiraron tambien el estado y las pruebas deterministas especificas de
  swing. No se modifico el secuenciador interno historico ajeno a VST3.

### Verificacion y despliegue

- `git diff --check` correcto, con solo avisos preexistentes de LF/CRLF.
- Release `x64` correcto con `Builds/VisualStudio2026/Loopernico Refactorizado.sln`.
- `Current` SHA-256:
  `E159A9D04BA81B318066988C023D4B7AA1840924BE28B52F0E5E3D1461F44718`.
  `Previous` conserva la version funcional anterior:
  `F2DD6AF818149ED71AE973651340D4A3D638A229138F606E4C3D79ED8F2DA700`.
- El acceso directo apunta a `Current`; `.loopernico` conserva
  `Loopernico.Session` y el comando con `%1`. El ejecutable nuevo permanecio
  estable y responsivo durante 15 s y cerro con codigo 0.

Proximo paso concreto: confirmar manualmente la ventana `PATRON MIDI` y que el
patron escrito en la grilla se reproduzca sin desplazamiento de swing.

## Ultimo ajuste comprobado: patron VST3 y swing en tiempo real

- Corregido el 2026-07-26 el editor `PATRON MIDI`: las celdas, los compases,
  `LIMPIAR` y `SWING` publican cada cambio inmediatamente al motor que genera
  el `MidiBuffer`; ya no dependen de pulsar `GUARDAR` para afectar el patron
  que esta sonando.
- El patron de cada slot VST3 se publica entre dos buffers alternos mediante
  un indice atomico. El callback de audio nunca lee la copia que la interfaz
  esta reemplazando. El calculo de swing del segundo paso de cada pareja queda
  aplicado al instante en los siguientes eventos MIDI.
- No se modificaron pedales internos, transporte del looper, sesiones ni el
  mapa MIDI. `GUARDAR` conserva su funcion de cerrar aceptando el estado y
  `CANCELAR` restaura el patron inicial y descarta las ediciones realizadas en
  la ventana.

### Verificacion y despliegue

- `git diff --check` correcto; solo permanecen avisos preexistentes de
  normalizacion LF/CRLF.
- Release `x64` correcto con `Builds/VisualStudio2026/Loopernico Refactorizado.sln`.
- Pruebas deterministas de compensacion, buses y patron MIDI: codigo de salida
  0. El ejecutable Release y `Current` permanecieron responsivos durante 8 s.
- `Current` nuevo SHA-256:
  `F2DD6AF818149ED71AE973651340D4A3D638A229138F606E4C3D79ED8F2DA700`.
  `Previous` conserva la version funcional anterior:
  `2704BAEECB52500836CE155089D0A51B792775B7B61B60A8A84D8F2D520BCEC2`.
- El acceso directo apunta a `Current`, con directorio de trabajo e icono
  correctos. `.loopernico` conserva `Loopernico.Session`, tipo `Archivo de
  Loopernico` y comando con `%1`.

Proximo paso concreto: confirmar manualmente con un VST de bateria cargado que
el PLAY del patron y el transporte del looper reflejen un cambio de celda y de
SWING mientras la ventana `PATRON MIDI` sigue abierta.

## Ultimo ajuste comprobado: ventanas MIDI independientes del VST

- Reemplazadas las ventanas `AlertWindow` de `MAPA MIDI` y `PATRON MIDI` por
  `DocumentWindow` nativas con redimensionado real de JUCE/Windows, limites,
  centrado estable y layout propio adaptable. La grilla conserva las 16
  columnas y sus botones se ajustan al area disponible.
- `MAPA` y `PATRON` quedan habilitados para todos los slots VST3 aunque el slot
  muestre `SIN VST`; la configuracion se guarda en el estado del slot y queda
  disponible cuando se cargue el plugin posteriormente.
- Release `x64` correcto el 2026-07-26. `Current` nuevo:
  `BEDC5D53C37A451BC139EF761729AB0ECBB0B2F91A00C955829BCAA31FFA4CD7`.
  `Previous` conserva la version funcional anterior:
  `A970A83BD27134C7F7DB6177133F54CA6A7E8A317119067C6FF050A6FA20F643`.
- Ambos ejecutables permanecieron responsivos 8 segundos y cerraron con codigo
  0. El acceso directo y `.loopernico` apuntan a `Current`; la asociacion
  conserva `Loopernico.Session` y el comando con `%1`.
- `git diff --check` correcto, con solo avisos preexistentes de LF/CRLF.
  Falta confirmar manualmente el arrastre y redimensionado porque el
  controlador visual de Windows no esta disponible en esta sesion.

Proximo paso concreto: abrir `MAPA` y `PATRON` desde un slot sin VST, modificar,
guardar y luego cargar un VST en ese slot para confirmar el uso de la
configuracion persistida.

## Ultimo ajuste comprobado: ventanas emergentes MAPA/PATRON MIDI

- Corregido el comportamiento de las ventanas `MAPA MIDI` y `PATRON MIDI`.
  El `ResizableCornerComponent` ahora convive con el layout interno real de
  `AlertWindow`, que se conserva al redimensionar; los limites minimo/maximo
  siguen siendo 520x520/1200x900 para MAPA y 980x500/1500x900 para PATRON.
- El centrado y el tamano inicial se aplican antes de entrar en modo modal para
  evitar saltos de posicion. El editor de patron parte de 820 px y calcula el
  ancho de sus celdas segun el espacio disponible, de modo que las 16 columnas
  quedan visibles y se adaptan al redimensionar.
- No se modificaron el modelo MIDI, el guardado de sesiones, los pedales ni el
  procesamiento de audio.

### Verificacion y despliegue

- Release `x64` correcto el 2026-07-26 con
  `Builds/VisualStudio2026/Loopernico Refactorizado.sln`.
- El ejecutable nuevo y el respaldo anterior permanecieron abiertos y
  responsivos durante 8 segundos y cerraron con codigo 0.
- `Builds/LocalDeployment/Current/Loopernico Refactorizado.exe` nuevo:
  SHA-256 `A970A83BD27134C7F7DB6177133F54CA6A7E8A317119067C6FF050A6FA20F643`.
  `Previous` conserva la version funcional anterior:
  `076618A9B447305E9058269535A8AB6B1B52895E5E3C80E8DC418BD10CC34914`.
- El acceso directo apunta a `Current` con su directorio de trabajo e icono
  correctos. La asociacion `.loopernico` conserva `Loopernico.Session` y el
  comando hacia `Current`.
- `git diff --check` correcto; permanecen solamente avisos preexistentes de
  normalizacion LF/CRLF. La comprobacion interactiva de arrastre requiere
  confirmacion manual porque el controlador visual de Windows no esta
  disponible en esta sesion.

Proximo paso concreto: abrir un VST3, entrar en `PATRON MIDI` y confirmar
manualmente arrastre, redimensionado y visibilidad de las 16 columnas.

## Etapa 1 del ampliado del secuenciador MIDI: swing

- Implementado el 2026-07-26 únicamente para el patrón de cada slot VST3.
- `DrumMidiPattern` conserva `swingPercent` de 0 a 100; el valor por defecto
  es 0 para sesiones anteriores. El segundo paso de cada pareja de
  semicorcheas se retrasa hasta medio paso a 100%; los pasos impares de la
  pareja no se alteran.
- `PATRON MIDI` muestra y edita SWING; la sesión lo serializa como
  `patternSwing` dentro de cada nodo `VST`. No se modificaron MAPA, transporte,
  flujo de carga VST3 ni otros pedales.
- La suite determinista pasó 27/27, incluyendo límites 0/100 y el cálculo de
  retraso. Release `x64` correcto con la solución generada por JUCE.
- El ejecutable Release y `Builds/LocalDeployment/Current` permanecieron
  responsivos durante 15 s y cerraron con código 0. `Current` nuevo:
  `076618A9B447305E9058269535A8AB6B1B52895E5E3C80E8DC418BD10CC34914`.
  `Previous` conserva la versión estable anterior:
  `1E2A3C1E4D302C1F90421ABC28506857AF3E79A0D7DF50B2B6C7D362D449E055`.
- El acceso directo y la asociación `.loopernico` apuntan a `Current` y
  conservan el tipo, comando e icono de sesión. La captura/control visual
  interactivo no pudo completarse por `SetIsBorderRequired (0x80004002)`;
  queda pendiente revisar manualmente la ventana PATRON MIDI.

Próximo paso concreto: confirmar manualmente la legibilidad del control SWING
en PATRON MIDI y, si no hay regresiones, iniciar la etapa de acentos y
articulaciones.

## Auditoria funcional inicial: faltantes potenciales

- Revision estatica realizada el 2026-07-26, sin cambios de codigo: el motor
  conserva `stopPlay()`, pero el boton visible `IR ARRIBA` llama solamente a
  `goToStart()`, por lo que no hay un STOP de transporte claramente expuesto
  en la interfaz principal.
- El guardado es manual; no se encontro autosave ni recuperacion automatica de
  una toma despues de un cierre inesperado.
- Las pistas tienen MUTE, SOLO y volumen visibles, pero la pausa individual
  existe solo en el backend; la interfaz ofrece PAUSE global.
- El secuenciador MIDI VST3 actual cubre seis carriles y hasta cuatro compases;
  articulaciones y humanizacion siguen siendo mejoras pendientes. Swing fue
  implementado en la etapa 1 por patrón/VST3.

Proximo paso concreto: validar estas prioridades con el uso diario de Fernando
antes de implementar alguna, empezando por el transporte STOP/IR ARRIBA y la
recuperacion de sesiones.

## Ultimo ajuste comprobado: retiro del LED de MONO/STEREO

- Se retiro solamente el punto LED de la pala `MONO`/`STEREO` en la esquina
  inferior derecha. El boton conserva el cambio de modo y su estado visual.
- Release `x64` correcto el 2026-07-26; el ejecutable desplegado permanecio
  estable durante 8 segundos y cerro con codigo 0.
- `Current` SHA-256:
  `1E2A3C1E4D302C1F90421ABC28506857AF3E79A0D7DF50B2B6C7D362D449E055`.
  `Previous` conserva la version funcional anterior:
  `8E50214C62FE3D2090036D98B357995387FBB213CE6A84EA64FE00D52706CC00`.
- El acceso directo fue recreado hacia `Current`; la asociacion `.loopernico`
  se conserva sin cambios.

Ultima actualizacion: 2026-07-26 (America/Buenos_Aires)

## Ultimo ajuste comprobado: limpieza visual de halos y diagnostico superior

- Se oculto la etiqueta permanente con driver, dispositivo, sample rate,
  latencias, compensacion, monitoreo y XRUN; el estado de audio sigue visible
  solamente mediante `CONFIGURACION !` cuando requiere atencion.
- Se quitaron los halos circulares del slider `SIGNAL THRESH.` y del transporte
  icon-only PLAY, conservando sus colores y funciones.
- Release `x64` correcto el 2026-07-26; el ejecutable desplegado permanecio
  estable durante 8 segundos y cerro con codigo 0.
- `Current` SHA-256:
  `8E50214C62FE3D2090036D98B357995387FBB213CE6A84EA64FE00D52706CC00`.
  `Previous` conserva la version funcional anterior:
  `13F48CB7C13EDE3119E112FBCA0AC6D81BCA758F11D2C6FE17E0A3B376553E6D`.
- El acceso directo y la asociacion `.loopernico` fueron verificados. La
  captura automatica solicitada para revision visual no genero un PNG valido.

Ultima actualizacion: 2026-07-26 (America/Buenos_Aires)

## Ultimo ajuste comprobado: correccion del fade de arremangado

- El ruido de cruce provenia de mezclar destructivamente la cola y la cabeza
  del loop al mismo tiempo: al envolver, el reproductor atravesaba dos mezclas
  consecutivas, y el arrastre del marcador reaplicaba el fade sobre audio ya
  procesado.
- `LooperEngine::applyLoopSeamCrossfade()` ahora conserva intacta la cabeza y
  aplica una sola salida equal-power de la cola hacia el primer sample real del
  loop. No se cambiaron el ruteo, el transporte ni otros efectos.
- Release `x64` correcto el 2026-07-26; el ejecutable nuevo permanecio estable
  durante 8 segundos, se desplego en `Current`, `Previous` conserva la version
  funcional anterior y el humo de ambos binarios fue correcto.
- `Current` SHA-256: `13F48CB7C13EDE3119E112FBCA0AC6D81BCA758F11D2C6FE17E0A3B376553E6D`.

Ultima actualizacion: 2026-07-26 (America/Buenos_Aires)

## Tarea actual: apariencia de ComboBox y PopupMenu

- Se mejoro exclusivamente `LoopernicoLAF`: controles `ComboBox` con gradiente,
  esquinas redondeadas, borde reactivo a hover/foco, flecha consistente y estado
  deshabilitado; los `PopupMenu` ahora tienen fondo con profundidad, borde,
  filas de 30 px, tipografia jerarquizada, hover, tick, separadores y submenus.
- No se cambiaron opciones, orden, valores, callbacks ni comportamiento.
- Release `x64` correcto el 2026-07-26 con la solucion generada por JUCE; el
  ejecutable permanecio activo y responsivo durante 8 s y cerro con codigo 0.
- La captura/control visual de Windows agoto el tiempo de espera al lanzar el
  Release y no expuso una ventana; no se declara una captura interactiva.

### Proximo paso concreto

Confirmar manualmente un `ComboBox` principal y el menu de configuracion MIDI
en `Builds/VisualStudio2026/x64/Release/App` o en el despliegue diario.

Ultima actualizacion: 2026-07-26 (America/Buenos_Aires)

## Tarea terminada: acceso visible a Configuracion desde la interfaz principal

### Implementacion

- El boton existente que abre `AudioSettingsPanel` ahora se muestra como
  `CONFIGURACION` en el encabezado principal, con tooltip y el mismo estilo de
  los controles de sesion.
- El estado sin audio se indica como `CONFIGURACION !` sin cambiar el flujo de
  apertura.
- Se reutilizo la pantalla existente: abre con `showAudioSettings()`, se puede
  cerrar con `CERRAR ESC` o ESC y no se modificaron sesiones, pedales, efectos,
  MIDI ni DSP.

### Verificacion y despliegue

- Release `x64` correcto dos veces con `Builds/VisualStudio2026/Loopernico Refactorizado.sln`,
  codigo 0 y sin errores ni advertencias nuevas visibles.
- Capturas visuales revisadas:
  `tmp/config-access-normal-final.png` confirma el boton legible y
  `tmp/config-access-panel-final.png` confirma el panel centrado y su cierre
  visible. El ejecutable de Release permanecio activo durante la captura y
  termino con codigo 0.
- `Builds/LocalDeployment/Current/Loopernico Refactorizado.exe` fue actualizado
  y `Previous` conserva la version funcional anterior. Humo de `Current`
  correcto, con captura y codigo 0.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` apunta a `Current`, conserva el
  directorio de trabajo y `Assets/Brand/Icons/loopernico-session.ico`.
- La asociacion `.loopernico` conserva `Loopernico.Session` y el comando hacia
  `Current`.

### Proximo paso concreto

No queda ningun paso de implementacion pendiente. Confirmar manualmente el
clic en `CONFIGURACION` y `CERRAR ESC` si se desea una prueba interactiva con
controlador de ventanas; la ruta de codigo, el build y las capturas son
correctos.

## Tarea terminada: cierre y redimensionado de ventanas MAPA/PATRON MIDI

### Implementacion

- Las ventanas `MAPA MIDI` y `PATRON MIDI` ahora tienen un boton `CERRAR`
  visible, conservan ESC y cierran sin guardar cuando corresponde.
- El cierre por clic fuera usa un listener global de mouse, de modo que no
  depende de que el clic llegue al componente modal.
- Ambas ventanas tienen esquina de redimensionado y limites minimos para no
  cortar el contenido: 520x520 para MAPA y 980x500 para PATRON.
- Los tamanos iniciales pasaron a 560x560 y 1000x500, respectivamente, para
  que los botones y las 16 columnas de la grilla entren completos.

### Verificacion y despliegue

- Release `x64` correcto con `Builds/VisualStudio2026/Loopernico Refactorizado.sln`,
  codigo 0 y sin advertencias nuevas tras corregir la herencia redundante.
- El ejecutable Release y `Builds/LocalDeployment/Current` permanecieron
  activos durante 10 segundos y cerraron normalmente.
- `Builds/LocalDeployment/Current/Loopernico Refactorizado.exe`:
  SHA-256 `840F9CF5030266364A2CA3473C5C6B057807AB918D0AE0A0C0EB4662335B3838`.
- `Builds/LocalDeployment/Previous/Loopernico Refactorizado.exe` conserva la
  version funcional anterior, SHA-256
  `6F3A66524B70C605597C6E4383B3DD6B0F47958191641CDABB48C783710B49D8`.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` apunta a `Current`, con
  directorio de trabajo e icono de sesion correctos.
- `.loopernico` conserva `Loopernico.Session`, tipo `Archivo de Loopernico`,
  comando hacia `Current` e icono de sesion.

### Limitacion

- La prueba interactiva de clic y redimensionado no pudo completarse porque
  el capturador/controlador de ventanas devolvio `0x80004002` al inspeccionar
  Loopernico. El comportamiento queda compilado y desplegado, pero requiere
  confirmacion manual en la aplicacion.

### Proximo paso concreto

No queda ningun paso de implementacion pendiente. Confirmar manualmente MAPA y
PATRON cuando el controlador visual de Windows vuelva a estar disponible.

## Tarea terminada: reubicacion de controles IN/OUT y ampliacion de pedales

- Los botones `IN -`, `IN +`, `OUT -` y `OUT +` ahora comparten la hilera
  inferior con la configuracion MIDI. El rotulo `ARREMANGAR` acompana al grupo
  en esa misma franja.
- Se retiro la banda intermedia de controles de 50 px y esos pixeles se
  asignaron a la seccion de pedales: su alto paso de 260 a 310 px. La altura
  de las pistas se conserva.

### Verificacion y despliegue

- Build `Release | x64` correcto con
  `Builds/VisualStudio2026/Loopernico Refactorizado.sln`, codigo 0 y sin
  advertencias nuevas.
- Captura visual del Release:
  `C:\Users\Fernando\.codex\visualizations\2026\07\26\019f9c22-20d8-7800-8050-7ecc129cc64f\loopernico-ui-relayout-release.png`.
  Confirma pedales mas altos, pistas conservadas y fila MIDI/IN-OUT sin
  solapamientos.
- Humo del ejecutable Release y de `Builds/LocalDeployment/Current`: ambos
  generaron captura y terminaron con codigo 0.
- `Builds/LocalDeployment/Current/Loopernico Refactorizado.exe`:
  SHA-256 `6F3A66524B70C605597C6E4383B3DD6B0F47958191641CDABB48C783710B49D8`.
- `Builds/LocalDeployment/Previous/Loopernico Refactorizado.exe` conserva la
  version funcional anterior, SHA-256
  `99F668292AA23F4744F1E123AFC3081ABDA6F6277EBB6EF7BF851E7958C5ACFA`.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` apunta a `Current`, con
  directorio de trabajo `Current` e icono `Assets/Brand/Icons/loopernico-session.ico`.
  La asociacion `.loopernico` conserva `Loopernico.Session`, tipo `Archivo de
  Loopernico` y comando hacia `Current`.

### Proximo paso concreto

No queda ningun paso pendiente para este ajuste visual.

## Tarea terminada: retiro de BeatBuddy y controles Rhythm Box

- `MainComponent` ya no construye ni muestra el pedal historico de indice 1,
  ni registra sus controles, callbacks MIDI, propiedades de sesion o layout.
- El orden activo conserva los IDs historicos de los demas pedales y excluye el
  indice legado `EffectsChain::RhythmPedal`; las sesiones anteriores que aun
  contienen ese pedal lo omiten al restaurarse. El ultimo slot VST3 sigue
  incluido y el limite de IDs fue separado del conteo activo para no perderlo.
- La logica backend antigua de Rhythm Box permanece compilada pero queda
  inalcanzable desde el orden activo y sin controles publicos; no se modifico el
  ruteo de efectos restantes ni la capa de patrones MIDI de VST3.
- La busqueda de UI en `MainComponent` no encontro referencias a BeatBuddy,
  Rhythm Box ni los controles retirados de estilo, parte, preconteo, sonido,
  seccion o siguiente parte. El renderer generico antiguo de
  `LoopernicoWidgets` conserva sus ramas inalcanzables para no borrar backend/UI
  historico fuera del alcance.

### Verificacion y despliegue

- Build `Release | x64` correcto con
  `Builds/VisualStudio2026/Loopernico Refactorizado.sln`.
- Captura visual valida del Release, revisada directamente:
  `C:\Loopernico Refactorizado\Loopernico Refactorizado\tmp\beatbuddy-removal-release-20260725.png`
  (1600x960, SHA-256 `42AB1CFA40BF7108ACF3EC99782B84C5807C18C5A81153B8D882709784C10714`).
  Confirma que desaparecieron BeatBuddy y los controles inferiores, y que siguen
  visibles el host VST3, los demas pedales, IN/OUT y la franja MIDI.
- Humo del `Current` desplegado: tras 15 s de arranque permanecio responsivo
  durante 8 s adicionales, mostro ventana y cerro normalmente. Una sesion
  `.loopernico` real se abrio mediante la asociacion, permanecio 15 s y cerro
  sin salida prematura.
- `Builds/LocalDeployment/Current/Loopernico Refactorizado.exe`:
  SHA-256 `99F668292AA23F4744F1E123AFC3081ABDA6F6277EBB6EF7BF851E7958C5ACFA`.
- `Builds/LocalDeployment/Previous/Loopernico Refactorizado.exe`:
  SHA-256 `07EF325ED77EA76D61C6FC703DD6AF85802F75FD966DCFD09B3EEA283AD0390F`.
  Se conservan exactamente dos ejecutables Release funcionales en el
  despliegue; la version anterior tambien quedo respaldada en `tmp/` antes de
  la rotacion.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` apunta a `Current`, con el
  directorio de trabajo `Current` y el icono
  `Assets/Brand/Icons/loopernico-session.ico`.
- `.loopernico` conserva `Loopernico.Session`, tipo `Archivo de Loopernico`,
  comando hacia `Current` e icono de sesion correcto.
- `git diff --check`: correcto; solo avisos preexistentes de normalizacion
  LF/CRLF.

### Proximo paso concreto

No queda ningun paso pendiente para este retiro. Mantener la logica backend
antigua de Rhythm Box sin exponerla nuevamente desde el orden activo.

## Tarea: icono PLAY/STOP y carga inicial de BFD

### Implementado

- El boton de transporte del patron VST3 ahora es icon-only: triangulo verde
  para PLAY y cuadrado rojo para STOP. Conserva el estado toggle, tooltip y
  comportamiento MIDI existentes.

### Bloqueo comprobado de BFD

- `C:\Program Files\Common Files\VST3\BFDPlayer.vst3` existe y llega a
  inicializarse como `BFD Player`, con latencia reportada de 256 samples.
- Se probaron auto-carga activa y auto-carga en bypass. En ambos casos el host
  termina con crash despues de crear el plugin (`0xC0000005` / `0xC0000374`,
  errores de acceso/heap registrados por Windows). Por seguridad, la
  auto-carga no se desplego; BFD sigue cargandose mediante `CARGAR` como hasta
  ahora. No se modifico el flujo de sesiones ni se dejo una version diaria
  inestable.

### Verificacion y despliegue

- Release `x64` correcto, humo del ejecutable y de `Current` responsivos
  durante 8 s, y suite determinista `27/27`.
- Captura visual icon-only:
  `C:\Users\Fernando\.codex\visualizations\2026\07\25\019f9b47-1ca3-74a2-aa2f-e4d2207e5291\vst3-play-stop-icon.png`.
- `Current` SHA-256:
  `07EF325ED77EA76D61C6FC703DD6AF85802F75FD966DCFD09B3EEA283AD0390F`.
- `Previous` SHA-256:
  `7ACFD6FCBAE809AAF35527A435144EA117EAA250C9FC41610FF307E4DDFA9996`.
- Acceso directo y asociacion `.loopernico` verificados nuevamente.

## Tarea terminada: layout VST3 sin cubrir TONEX y cierre externo de editores

### Cambios

- La fila `CARGAR / QUITAR / ABRIR` ahora queda por encima de la pantalla del
  asset, y `MAPA / PATRON / PLAY` queda debajo de la pantalla pero antes del
  logotipo TONEX. Ambas filas conservan posiciones proporcionales para el
  tamaño normal y el overlay ampliado; no se tocaron otros pedales.
- Las ventanas modales `MAPA MIDI` y `PATRON MIDI` ahora detectan un clic fuera
  de sus limites y se cierran como `CANCELAR`, sin guardar cambios. ESC y los
  botones existentes siguen funcionando.

### Verificacion y despliegue

- Build `Release | x64` correcto con la solucion de Visual Studio generada por
  JUCE, codigo 0.
- Captura visual final en
  `C:\Users\Fernando\.codex\visualizations\2026\07\25\019f9b47-1ca3-74a2-aa2f-e4d2207e5291\vst3-buttons-final.png`:
  tres botones sobre la pantalla, tres debajo y logotipo TONEX despejado.
- Humo del Release y de `Builds/LocalDeployment/Current`: activos y
  responsivos durante 8 s.
- Suite determinista: `27/27` pruebas, codigo 0.
- `Current` SHA-256:
  `7ACFD6FCBAE809AAF35527A435144EA117EAA250C9FC41610FF307E4DDFA9996`.
- `Previous` SHA-256:
  `656671698842DD2088CA98C6D0D20CACD6CA1AE2BEA10DDE6D7EA03C72CC23C7`.
- Se conservaron exactamente esas dos versiones funcionales. El acceso
  directo apunta a `Current` con directorio de trabajo e icono correctos.
  `.loopernico` conserva `Loopernico.Session`, tipo `Archivo de Loopernico`,
  comando hacia `Current` e icono de sesion.

### Limitacion

- El modo interno de captura visual sigue terminando con `0xC0000005` despues
  de escribir correctamente el PNG; el humo estable se realizo por separado
  sobre el ejecutable sin captura.

## Tarea terminada: STOP independiente y layout responsive de controles VST3

### Diagnostico comprobado

- `queueExternalPluginPatternMidi()` usaba `looperIsPlaying ||
  patternPlaybackEnabled`. Por eso STOP no entraba en la rama de apagado si el
  transporte del looper seguia activo: el patron continuaba generando `noteOn`
  y no se enviaban `noteOff`.
- El transporte global sigue siendo compatible: al comenzar un nuevo ciclo del
  looper se limpia la supresion de STOP; PLAY independiente la limpia de forma
  inmediata.

### Implementacion

- STOP ahora registra `patternStopRequested` por slot y detiene la generacion de
  nuevos `noteOn` aunque el looper siga reproduciendo.
- El flush de parada envia `noteOff` para las seis notas del mapa del slot,
  `All Notes Off` y `All Sound Off` por canal. Se repite entre 2 y 32 bloques,
  calculado a partir de la latencia reportada por el VST3, para cubrir el
  puente por bloques y la latencia interna del plugin.
- Se limpian estado de transporte, flush pendiente y supresion al recargar,
  resetear o deshabilitar el streaming del slot.
- Los seis botones visibles de cada host VST3 se distribuyeron en dos filas de
  tres debajo de la pantalla, con separacion y calculo proporcional para el
  tamaño normal y el overlay ampliado. La carcasa, pantalla, footswitch y otros
  pedales no fueron modificados.

### Verificacion y despliegue

- Build `Release | x64` correcto con
  `Builds/VisualStudio2026/Loopernico Refactorizado.sln`, codigo 0.
- Suite determinista del ejecutable final: `27/27` pruebas, codigo 0.
- Humo del ejecutable final y de `Builds/LocalDeployment/Current`: ambos
  permanecieron activos y respondieron durante 8 s.
- Captura visual del host VST3 enfocada en
  `C:\Users\Fernando\.codex\visualizations\2026\07\25\019f9b47-1ca3-74a2-aa2f-e4d2207e5291\vst3-focused.png`:
  pantalla y carcasa conservadas; botones legibles en dos filas.
- `Current` SHA-256:
  `656671698842DD2088CA98C6D0D20CACD6CA1AE2BEA10DDE6D7EA03C72CC23C7`.
- `Previous` SHA-256:
  `DB2BDF30F11384D336DE4D035F2F54D40AC518FCEB4530A629D5F93E49D14021`.
- Se conservaron exactamente esas dos versiones funcionales.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` apunta a `Current`, con
  directorio de trabajo e icono correctos.
- `.loopernico` sigue asociado a `Loopernico.Session`, tipo `Archivo de
  Loopernico`, comando hacia `Current` e icono
  `Assets/Brand/Icons/loopernico-session.ico`.

### Limitacion real

- La verificacion auditiva/interactiva con un VST3 real no pudo declararse:
  cargar TONEX en el modo de captura produjo la imagen correcta pero termino
  con `0xC0000005` al cerrar el plugin. La entrega de `noteOff` queda verificada
  por el camino de audio/MIDI compilado, la logica de flush y la suite
  determinista existente; falta una confirmacion manual con el VST3 concreto.

## Tarea terminada: reproduccion independiente de PATRON por VST3

- Cada host VST3 ahora tiene un boton `PLAY`/`STOP` independiente del looper.
  Permite programar y escuchar la bateria sin iniciar grabacion ni reproduccion
  de audio en el looper.
- El patron conserva como referencia el BPM y compas del looper. El playhead
  VST3 se marca como activo mientras haya un patron independiente reproduciendo,
  pero el scheduler distingue ese estado del transporte global para no disparar
  patrones de otros slots.
- Al detener `PLAY`, el slot envia `noteOff`; al quitar o recargar el VST3 el
  estado de reproduccion independiente vuelve a apagado.

### Verificacion y despliegue

- Build final `Release | x64` correcto con
  `Builds/VisualStudio2026/Loopernico Refactorizado.sln`, sin advertencias
  nuevas del codigo.
- Humo de `Builds/LocalDeployment/Current`: permanecio activo 8 s y cerro
  normalmente.
- `Current` SHA-256:
  `DB2BDF30F11384D336DE4D035F2F54D40AC518FCEB4530A629D5F93E49D14021`.
- `Previous` SHA-256:
  `8514A499CD153B3E7A90E3D7EB3685768CDF6DF007B0F24733A19FC5FD78092D`.
- Se conservaron exactamente esas dos versiones funcionales.
- Acceso directo y asociacion `.loopernico` verificados nuevamente.

## Tarea terminada: reproduccion MIDI basica de la grilla PATRON en VST3

- `EffectsChain` ahora genera eventos MIDI de las seis filas del patron
  (`noteOn` y `noteOff`) en el `MidiBuffer` de cada VST3, usando el mapa de
  notas/canales configurable por slot.
- Los eventos se insertan con offsets relativos al bloque que el host entrega
  al plugin. El patron sigue el BPM y el estado de reproduccion del playhead
  del looper; al iniciar transporte comienza en el paso 1, y al detenerlo
  envia `noteOff` para las seis filas.
- La duracion inicial de cada nota es media semicorchea. La base ya recorre
  kick, snare, hi-hat cerrado/abierto, crash y ride; las siguientes etapas
  pueden agregar articulaciones y una verificacion interactiva especifica con
  BFD Player.
- Los patrones ahora se guardan opcionalmente en cada nodo `VST` como cantidad
  de compases y velocidades serializadas. Las sesiones anteriores, sin esos
  atributos, conservan el patron vacio por defecto.

### Verificacion y despliegue

- Build final `Release | x64` correcto con
  `Builds/VisualStudio2026/Loopernico Refactorizado.sln`, sin advertencias
  nuevas del codigo.
- Humo del ejecutable Release y de `Builds/LocalDeployment/Current`: ambos
  permanecieron activos 8 s y cerraron normalmente.
- `Current` SHA-256:
  `89B774C1D87854138B3A724294B988E5339E38A37348F5B3C80BD2E0C03236F2`.
- `Previous` SHA-256:
  `BE64F2D1CBCAEE67C65431E24A3913CF89C1DE72B9B0D5D43372EAB5E5371896`.
- Se conservaron exactamente esas dos versiones funcionales.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` fue recreado hacia `Current`,
  con directorio de trabajo e icono correctos.
- La asociacion `.loopernico` conserva `Loopernico.Session`, tipo
  `Archivo de Loopernico`, comando hacia `Current` e icono de sesion.

### Limite de prueba interactiva

- BFD Player esta instalado en `C:\Program Files\Common Files\VST3\BFDPlayer.vst3`.
- Se intento abrir Loopernico desplegado para cargar BFD y accionar `PATRON`,
  pero la captura/control de la ventana devolvio `0x80004002` (interfaz no
  compatible). No se declara verificacion auditiva o interactiva de BFD.
- Fernando informo que al editar `PATRON` el VST3 no parecio detectar MIDI.
  Diagnostico de codigo pendiente de confirmar en uso: la emision actual esta
  condicionada a que el transporte del looper este en `Playing` o grabando;
  con el looper detenido el `MidiBuffer` queda vacio deliberadamente.

## Tarea terminada: limite de grabacion ampliado a 10 minutos

- `LooperEngine::kMaxLoopSecs` paso de 30 a 600 segundos.
- La memoria de audio de las pistas se reserva bajo demanda: la pista que va a
  grabarse recibe capacidad completa antes de iniciar y las demas permanecen
  sin buffer hasta utilizarse. Esto evita reservar varios GB por las 12 pistas
  al abrir la aplicacion.
- La restauracion de sesiones conserva el audio con su longitud real y sigue
  aceptando sesiones anteriores.

### Verificacion y despliegue

- `git diff --check`: correcto; solo avisos preexistentes de normalizacion
  LF/CRLF.
- Build `Release | x64` correcto con
  `Builds/VisualStudio2026/Loopernico Refactorizado.sln`.
- Humo del ejecutable Release y de `Builds/LocalDeployment/Current`: ambos
  permanecieron activos 8 s y cerraron normalmente.
- `Current` SHA-256:
  `EC688CB7D1D6C6FFFE203810FAD3B54F679B1A367A130DADC33F5195420CDE4A`.
- `Previous` SHA-256:
  `75E2A22BD08A59D9FC3E2CF8058891B69BDB00B4FF6B53201CD996ADB7F51E6D`.
- Se conservaron exactamente esas dos versiones funcionales.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` apunta a `Current`, con su
  directorio de trabajo e icono correctos.
- La asociacion `.loopernico` sigue siendo `Loopernico.Session`, tipo
  `Archivo de Loopernico`, con comando hacia `Current` e icono de sesion.

## Uso

1. Leer `AGENTS.md` y este archivo completos al comenzar.
2. Verificar `git status --short --branch` y preservar cambios locales.
3. Actualizar este traspaso despues de avances materiales o antes de detener una tarea incompleta.

## Avance en curso: base de mapa MIDI para secuenciador de baterias

Fernando pidio comenzar por pasos pequenos la futura grilla MIDI para BFD y
otros VST de bateria.

- Se agrego `Source/DrumMidiMap.h` con carriles semanticos para kick, snare,
  hi-hat cerrado/abierto, crash y ride.
- Cada slot VST3 ya posee un mapa independiente, con General MIDI por defecto,
  canal 10 y validacion de notas 0-127/canales 1-16.
- Se agregaron pruebas deterministas para defaults, personalizacion y rechazos.
- Release x64 y humo de `Current` correctos. `Current` SHA-256:
  `3072C1A9C24989B90C7009DD517F030A38609BA6EDF247CD84A9373215DFEA1F`.
- Se agrego el boton `MAPA` a cada host VST3. Abre una ventana de configuracion
  con las seis notas y un canal MIDI global, permite guardar cambios o
  restablecer General MIDI, y aun no emite notas al plugin.
- Se agrego `Source/DrumMidiPattern.h`: patrones de 1 a 4 compases, 16 pasos
  por compas, velocidad 0-127 por carril y limpieza segura al reducir longitud.
- Se agrego el boton `PATRON` a cada host VST3. Abre una grilla visual 6x16
  con selector de compases (1-4) y pagina por compas, y guarda estados on/off
  sobre el modelo de patron sin emitir MIDI todavia.
- Release x64 y humo de build/`Current` correctos. `Current` SHA-256:
  `BE64F2D1CBCAEE67C65431E24A3913CF89C1DE72B9B0D5D43372EAB5E5371896`.
- Proximo paso concreto: traducir esta grilla a eventos `noteOn`/`noteOff`
  dentro del `MidiBuffer` del VST3 seleccionado y validar primero con un patron
  minimo de bombo/redoblante/hi-hat.

## Tarea terminada: transporte y tempo para VST3 de baterias

Fernando detecto que BFD Player VST3 mostraba tempo 1 dentro de Loopernico y no
avanzaba los grooves. La causa comprobada era que el host procesaba el audio VST3
sin proveerle un `AudioPlayHead`; por eso el plugin no recibia BPM, compas ni
estado de transporte.

### Implementacion

- Cada VST3 recibe un playhead estable con BPM, compas 4/4, posicion en samples,
  PPQ, reproduccion y grabacion. El BPM proviene exclusivamente del looper, no
  del tempo independiente de Rhythm Box.
- Se agrego en la franja superior del looper `TEMPO`, editable con botones +/-
  entre 20 y 300 BPM, y un boton `TAP`. El valor inicial es 120 BPM y al abrir
  una sesion se actualiza al BPM restaurado por dicha sesion.
- El playhead VST3 se publica por bloque de audio sin asignaciones, disco ni
  bloqueos en el callback.

### Verificacion y despliegue

- Build final `Release | x64` con `Builds/VisualStudio2026/Loopernico Refactorizado.sln`:
  correcto, sin errores ni advertencias nuevas del codigo.
- Prueba de humo del EXE de build, del candidato de despliegue y de
  `Builds/LocalDeployment/Current`: los tres abrieron y permanecieron estables.
- `Current`: SHA-256 `B5C9735C43C0C8BE86E3DF91132104A5DEE908C37B37AE238EAE5A85E941F764`.
- `Previous`: SHA-256 `0170EBFF44A29260703DF2AC193C03F0F29ED515C288B45FC7FC76868CA2F5C9`;
  hay exactamente dos EXE desplegados.
- Acceso directo recreado hacia `Current`, con directorio de trabajo e icono
  correctos. Asociacion `.loopernico` comprobada: `Loopernico.Session`, tipo
  `Archivo de Loopernico`, comando hacia `Current` e icono de sesion correcto.
- Limite real: la visualizacion exacta de `120` y el disparo de un groove en BFD
  requieren abrir BFD Player y probarlo interactivamente; el transporte que el
  VST3 consulta ya entrega ese valor por construccion.

## Tarea terminada: entrada mono duplicada para afinadores VST3

Fernando informo que los efectos de los VST3 funcionaban, pero sus afinadores
incorporados no recibian la guitarra. La causa comprobada era el ruteo estereo
directo de las entradas fisicas: la guitarra podia estar en la entrada 2,
mientras varios afinadores inspeccionan solamente la entrada 1.

### Implementacion

- `MainComponent::getNextAudioBlock()` normaliza la entrada de instrumento antes
  de `EffectsChain` y del looper.
- Si un canal es claramente dominante, se conserva su nivel y se duplica en
  izquierda y derecha. Si ambos tienen nivel comparable, se usa el downmix mono
  convencional. De este modo todos los VST3 reciben la guitarra en sus dos
  entradas sin cambiar el orden DSP, los buffers del looper ni su compensacion.
- Las salidas estereo generadas por pedales y VST3 siguen siendo estereo; solo se
  normaliza la entrada fisica inicial.

### Verificacion y despliegue

- `git diff --check`: correcto; solo advertencias LF/CRLF preexistentes.
- Build final `Release | x64` con `Builds/VisualStudio2026/Loopernico Refactorizado.sln`:
  codigo 0 y sin advertencias. Un build intermedio emitio dos advertencias C4456
  por nombres locales repetidos; no se desplego y se corrigieron.
- Suite interna: 14/14 pruebas deterministas de compensacion y buses, codigo 0.
- Humo del candidato y de `Current`: captura generada, cierre normal y codigo 0.
- `Builds/LocalDeployment/Current`: SHA-256
  `5F9C824481F998E20E90C10673A7C51A4C2735F6B91E89A6BCD053058EB5E9F3`.
- `Builds/LocalDeployment/Previous`: SHA-256
  `4EFAFDECFCB84D1F2221A88F7482D83DBF4DDDAA4240411883F42F5E4D3D16E3`;
  hay exactamente dos EXE desplegados.
- Acceso directo recreado hacia `Current`, con directorio de trabajo e icono
  correctos. Asociacion `.loopernico` verificada como `Loopernico.Session`, tipo
  `Archivo de Loopernico`, comando hacia `Current` e icono de sesion correctos.
- Limite de la verificacion automatica: la deteccion visual de nota dentro de un
  afinador VST3 concreto requiere confirmacion con guitarra real, pero ambos
  canales reciben ahora la misma senal de instrumento por construccion.

## Tarea terminada: eliminar el salto visible al maximizar

Fernando comprobo que adelantar solamente `setFullScreen(true)` no eliminaba el salto. La causa es que el peer de JUCE para Windows implementa esa llamada mediante `ShowWindow(..., SW_SHOWMAXIMIZED)`, que todavia puede animar desde la geometria normal guardada.

### Implementacion

- En escritorio normal, la ventana oculta recibe primero el `userBounds` del monitor elegido y luego llama a `setFullScreen(true)` antes de `setVisible(true)`. La animacion nativa parte asi de una geometria que ya coincide con la maximizada.
- El modo de capturas conserva deliberadamente su tamano fijo `1600x960` centrado.
- No se modificaron layout, temporizadores, audio, sesiones, pedales ni DSP.

### Verificacion y despliegue

- `git diff --check`: correcto; solo advertencias de normalizacion LF/CRLF preexistentes.
- Build final `Release | x64` con `Builds/VisualStudio2026/Loopernico Refactorizado.sln`: codigo 0 y sin advertencias del codigo. Un build intermedio fallo por intentar pasar `Rectangle<float>` a `setBounds`; no se desplego y se corrigio explicitamente con `toNearestInt()`.
- Traza temprana del candidato: todos los cambios de geometria ocurrieron con la ventana invisible; la primera geometria visible fue `-8,-8,1928,1048`, `ShowCmd=3` (maximizada), sin rectangulos visibles intermedios. Cierre normal con codigo 0.
- Captura real `tmp/maximize-before-show-smoke-20260722.png`: layout completo, sin zonas sin pintar ni regresiones visuales evidentes.
- Humo de `Current`: maximizado en `-8,-8,1928,1048`, 9/12 lecturas responsivas (las tres primeras corresponden a la inicializacion), cierre normal y codigo 0.
- `Builds/LocalDeployment/Current`: SHA-256 `4EFAFDECFCB84D1F2221A88F7482D83DBF4DDDAA4240411883F42F5E4D3D16E3`.
- `Builds/LocalDeployment/Previous`: SHA-256 `DC16377F5DFCB835A93F686A6062DE498B61FC8582D4A95A02FB49521761FAFF`; hay exactamente dos EXE desplegados.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` fue recreado hacia `Current`, con directorio de trabajo e icono correctos.
- Asociacion `.loopernico` verificada: `Loopernico.Session`, tipo `Archivo de Loopernico`, comando hacia `Current` e icono de sesion correctos.

## Base comprobada: multiples VST3 con la carcasa TONEX original

Fernando pidio que los cuatro hosts VST3 reutilicen exactamente la carcasa TONEX terminada que existia antes de la ampliacion multi-VST.

## Implementacion comprobada

- Hay cuatro hosts VST3 independientes con carga, descarga, editor y bypass propios.
- Las cuatro instancias se construyen con el identificador visual exacto `VST3 HOST`. Asi reutilizan el mismo asset `model-host-face-v3.png`, recorte, proporciones, pantalla, tipografia, botones, footswitch, LED y hit-testing de la implementacion TONEX original; el nombre del plugin continua mostrandose en la pantalla de cada instancia.
- Los cuatro hosts forman parte del mismo orden arrastrable que los 13 pedales internos. El orden visual publicado atomically es tambien el orden DSP real.
- Cada VST se procesa en streaming mediante buffers preasignados. Esto permite intercalarlo con el DSP por muestra existente sin asignaciones ni acceso a disco en el callback. La latencia de puente (un bloque por VST activo) se suma a la latencia informada por cada plugin para la compensacion de grabacion.
- Las sesiones guardan ruta, enabled, posicion y estado de las cuatro instancias dentro del bloque VST historico. Las sesiones anteriores con un solo VST se detectan y migran al host 1, conservandolo antes de los pedales internos como en el comportamiento previo.
- El mapping MIDI `VST3` historico sigue controlando el host 1 para no romper perfiles existentes.
- No se modifico el `.jucer` local preexistente ni otros pedales fuera de la integracion necesaria.

## Verificacion y despliegue

- `git diff --check`: correcto; solo advertencias de normalizacion LF/CRLF preexistentes.
- Build `Release | x64` con `Builds/VisualStudio2026/Loopernico Refactorizado.sln`: codigo 0, 0 errores y 0 advertencias.
- Suite interna: 14/14 pruebas deterministas de compensacion y buses, codigo 0.
- TONEX real: carga correcta desde `C:\Program Files\Common Files\VST3\TONEX.vst3` en host 1.
- Capturas nativas compacta y ampliada: la carcasa, textura, pantalla, controles, footswitch y luces coinciden con el diseno TONEX original. La automatizacion interactiva de Windows no estuvo disponible por `0x80004002`, por lo que la comprobacion visual se hizo con `Component::createComponentSnapshot` y revision directa de los PNG.
- Roundtrip nuevo: sesion guardada con TONEX, reabierta y capturada con plugin, estado, posicion y bypass restaurados.
- Compatibilidad: una sesion `.loopernico` de 2026-07-19 abre correctamente con la migracion de orden.
- Smoke del ejecutable desplegado: captura nativa correcta y codigo 0. La apertura directa de la sesion nueva mediante la asociacion permanecio activa 8 s sin salida prematura.
- `Builds/LocalDeployment/Current`: SHA-256 `D7385CC29F8242E7735561C2B044296140052418BA8BDC580CD51C4AFA357F1E`.
- `Builds/LocalDeployment/Previous`: SHA-256 `70CA0BE1B4FB31EC1826E85386DFF5C629E30221E8D4A41AE28EFCAF7C7C7A95`; hay exactamente dos EXE desplegados.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` fue recreado hacia `Current`, con directorio de trabajo e icono correctos.
- Asociacion `.loopernico` verificada: `Loopernico.Session`, tipo `Archivo de Loopernico`, comando hacia `Current` e icono de sesion correctos.

## Estado de la base VST3

Implementacion terminada, verificada y desplegada como version diaria. Limite deliberado actual: cuatro VST3 simultaneos.
# Ultimo ajuste comprobado: ampliar pedales con clic derecho

- Corregido el 2026-07-27 en `FXPedalComponent`: el clic derecho abre
  directamente la vista ampliada del pedal y se procesa antes de las zonas de
  footswitch, para no cambiar parametros ni apagar/encender pedales por error.
  Se retiro el doble clic como disparador de ampliacion. No se modificaron el
  clic izquierdo, los controles, el arrastre ni el DSP.

## Verificacion y despliegue

- `git diff --check`: correcto; solo avisos preexistentes de normalizacion
  LF/CRLF.
- Release `x64` correcto con
  `Builds/VisualStudio2026/Loopernico Refactorizado.sln`, 0 errores y 0
  advertencias.
- Humo de `Builds/LocalDeployment/Current`: ventana visible y responsiva,
  proceso estable durante 5 segundos y cierre normal.
- `Current` SHA-256:
  `D47332F06CEDD01D8BF860D81F2CBD56CDF18C1C0CE386A58FB5F77C62B31FCA`.
  `Previous` conserva la version funcional anterior:
  `BDD888F551863821877B993FF20526ABF784C4AE06A0A03CFC55E1696A4D1F9D`.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` apunta a `Current`, con
  directorio de trabajo e icono de sesion correctos. `.loopernico` conserva
  `Loopernico.Session`, tipo `Archivo de Loopernico`, icono de sesion y
  comando con `%1` hacia `Current`.

# Contexto operativo de Loopernico

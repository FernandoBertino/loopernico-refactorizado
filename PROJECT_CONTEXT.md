# Contexto operativo de Loopernico

Ultima actualizacion: 2026-07-22 19:13 (America/Buenos_Aires)

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
- Release x64 y humo de build/`Current` correctos. `Current` SHA-256:
  `F364E636C1C662FA89E9C939836EEC6FB610ADB6798ADAD9FAC7CFB68ECA0858`.
- Proximo paso concreto: definir el modelo de eventos de un patron y luego
  construir la grilla, manteniendo el envio MIDI desactivado hasta verificar la
  edicion.

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

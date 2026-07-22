# Contexto operativo de Loopernico

Ultima actualizacion: 2026-07-22 18:44 (America/Buenos_Aires)

## Uso

1. Leer `AGENTS.md` y este archivo completos al comenzar.
2. Verificar `git status --short --branch` y preservar cambios locales.
3. Actualizar este traspaso despues de avances materiales o antes de detener una tarea incompleta.

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

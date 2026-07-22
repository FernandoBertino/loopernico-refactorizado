# Contexto operativo de Loopernico

Ultima actualizacion: 2026-07-22 17:54 (America/Buenos_Aires)

## Uso

1. Leer `AGENTS.md` y este archivo completos al comenzar.
2. Verificar `git status --short --branch` y preservar cambios locales.
3. Actualizar este traspaso despues de avances materiales o antes de detener una tarea incompleta.

## Tarea activa: multiples VST3 con la carcasa TONEX original

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

## Estado

Implementacion terminada, verificada y desplegada como version diaria. Limite deliberado actual: cuatro VST3 simultaneos.

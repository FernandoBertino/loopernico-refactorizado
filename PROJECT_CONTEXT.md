# Contexto operativo de Loopernico

Ultima actualizacion: 2026-07-22 13:09 (America/Buenos_Aires)

## Uso

1. Leer `AGENTS.md` y este archivo completos al comenzar.
2. Verificar `git status --short --branch` y preservar cambios locales.
3. Actualizar este traspaso despues de avances materiales o antes de detener una tarea incompleta.

## Tarea activa: bateria nuevamente dentro de la cadena de efectos

Fernando informo que el audio del pedal de bateria habia quedado seco y ya no era afectado por los pedales usados para la guitarra.

## Causa y correccion comprobadas

- La separacion de buses incorporada junto con el nuevo monitoreo habia dejado vacia la etapa `RhythmPedal` de `EffectsChain`; el rhythm se sumaba en `MainComponent` despues de procesar todos los efectos.
- `processRhythm` vuelve a inyectar `drumBusSample` en la posicion real del BeatBuddy dentro del orden atomico de pedales. En modo Solo reemplaza la entrada de guitarra; en Blend/Metronome la suma.
- Los pedales ubicados despues del BeatBuddy procesan bateria y guitarra; los ubicados antes no, conforme al orden visual de la pedalera.
- Se retiro de `MainComponent` la suma seca posterior para evitar duplicacion. La señal combinada y procesada es la que recibe y graba el looper.
- El bus seco separado se conserva solamente como retorno del rhythm cuando el monitoreo de entrada esta en DIRECT. La configuracion diaria comprobada de Fernando usa monitoreo SOFTWARE (`enabled=1`), por lo que escucha y graba la bateria procesada sin duplicacion.
- No se tocaron otros pedales ni los cambios locales preexistentes en `AGENTS.md`, `.jucer`, `Screenshots/` o `tmp/`.

## Verificacion y despliegue

- `git diff --check`: correcto; solo advertencias de normalizacion LF/CRLF preexistentes.
- Build `Release | x64` con `Builds/VisualStudio2026/Loopernico Refactorizado.sln`: codigo 0.
- Suite interna: 14/14 pruebas deterministas de compensacion y buses, codigo 0 (`tmp/latency-rhythm-fx-fix-20260722.txt`).
- Smoke del candidato y del ejecutable desplegado: ambos permanecieron abiertos 8 s; AudioBox ASIO 48 kHz/256 y sin salida prematura ni XRUN informado.
- Candidato y `Builds/LocalDeployment/Current` coinciden: SHA-256 `4FF8E64EE60DC52622AC20F1ABEE704908391C9F6D772FDA958F5442DB8049E8`.
- La version estable anterior se preservo en `Builds/LocalDeployment/Previous`: SHA-256 `B15E6BAB22F0FD0D52075E92586EE5E6BBA140DB4F9E8EB3CD791703B6E61273`; hay exactamente dos EXE desplegados.
- `C:\Users\Fernando\Desktop\Loopernico.lnk` fue recreado hacia `Current`, con directorio de trabajo e icono correctos.
- Asociacion `.loopernico` verificada: `Loopernico.Session`, tipo `Archivo de Loopernico`, comando hacia `Current` e icono de sesion correctos.

## Estado

Correccion terminada, verificada, desplegada como version diaria y confirmada por Fernando con su uso habitual.

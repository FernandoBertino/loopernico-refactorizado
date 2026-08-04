# Loopernico Refactorizado

Aplicacion de escritorio para Windows construida con JUCE. Combina un looper de
hasta 12 pistas, una pedalera de efectos, Rhythm Box/metronomo, control MIDI,
hosting VST3 y exportacion de audio.

## Requisitos de desarrollo

- Windows x64.
- Visual Studio 2026 con la carga de trabajo **Desarrollo para el escritorio con
  C++** y el SDK de Windows.
- JUCE y Projucer. El proyecto espera los modulos de JUCE en `../../JUCE/modules`
  respecto de este directorio.
- Projucer debe tener configurada una ruta global de modulos valida. El archivo
  `.jucer` usa modulos globales y tiene habilitado el host VST3.

## Generar y compilar

1. Abrir `Loopernico Refactorizado.jucer` con Projucer.
2. Verificar la ruta de los modulos de JUCE.
3. Guardar el proyecto para regenerar `Builds/VisualStudio2026`.
4. Abrir `Builds/VisualStudio2026/Loopernico Refactorizado.sln`.
5. Seleccionar `Debug | x64` o `Release | x64` y compilar el proyecto App.

## Crear el instalador de Windows

1. Compilar primero `Release | x64`.
2. Instalar Inno Setup 6 y abrir `Installer/Loopernico.iss`.
3. Elegir **Build > Compile**. El instalador queda en `Installer/Output`.

El instalador crea el acceso directo **Loopernico** en el escritorio, conserva
los recursos visuales y de audio bajo la carpeta de instalacion y registra
`.loopernico` como **Archivo de Loopernico**, con su icono propio. Al hacer doble
clic en una sesion, Windows inicia Loopernico y carga directamente ese archivo.

Los archivos de `Builds` son generados por Projucer. Si se agregan o quitan
fuentes, el cambio debe hacerse en el `.jucer` y luego regenerarse la solucion.

## Arquitectura

- `Source/MainComponent.*`: interfaz, dispositivo de audio, guardado/carga de
  sesiones, importacion/exportacion y coordinacion general.
- `Source/LooperEngine.*`: grabacion y reproduccion de loops, tempo,
  cuantizacion, edicion, mezcla por pista y snapshots de sesion.
- `Source/EffectsChain.*`: pedalera DSP, Rhythm Box, carga asincrona de samples y
  alojamiento de plugins VST3.
- `Source/EQProcessor.*`: ecualizador.
- `Source/MidiController.*`: aprendizaje y asignaciones MIDI.
- `Source/LoopernicoWidgets.*` y `LoopernicoLookAndFeel.h`: controles y aspecto
  visual propios.
- `Assets/Samples/RhythmBox`: banco de samples de bateria y metronomo.

## Control MIDI

- En Windows 10 (1809 o posterior) y Windows 11, los dispositivos Bluetooth LE
  MIDI emparejados aparecen directamente como entradas MIDI. No hace falta usar
  MIDIberry ni loopMIDI; el emparejamiento inicial se realiza desde Bluetooth de
  Windows antes de abrir Loopernico.
- El perfil comienza sin asignaciones. `MAP MIDI` permite aprender CC, nota o
  Program Change para los controles del looper, los trece pedales internos y el
  pedal VST3.
- `GUARDAR MIDI` escribe explicitamente el perfil global. Los cambios sin
  guardar permanecen solo durante la ejecucion actual.
- Al iniciar se carga el ultimo perfil guardado. Abrir o guardar una sesion
  `.loopernico` no modifica las asignaciones MIDI.
- Los mappings no restringen el canal MIDI y `TODAS LAS ENTRADAS MIDI` permite
  usar varios controladores simultaneamente.

El callback de audio atraviesa la cadena de efectos y el motor del looper. Las
operaciones costosas (archivos, plugins, decodificacion y UI) deben permanecer
fuera de ese callback.

## Grabar audio del navegador o de Windows

- En la franja superior del looper, elegir `NAVEGADOR / WINDOWS` en `ENTRADA`.
- Reproducir YouTube u otra aplicacion mediante la salida predeterminada de
  Windows y usar `REC / CONFIRM` igual que con la guitarra.
- Loopernico muestra el nivel detectado en dB. La captura conserva estereo, no
  atraviesa los pedales de guitarra y no vuelve a monitorizar la fuente,
  evitando oir una copia retrasada.
- Para volver al instrumento, seleccionar `GUITARRA`. La eleccion es operativa
  y no modifica el contenido ni el formato de las sesiones.

## Samples de Rhythm Box

El banco se organiza por estilo y por instrumento, por ejemplo
`Assets/Samples/RhythmBox/Rock/Kick`. La estructura completa y los formatos
admitidos estan documentados en `Assets/Samples/RhythmBox/README.md`.

La aplicacion busca la carpeta `Assets/Samples/RhythmBox` desde su ubicacion de
ejecucion y directorios ascendentes. Para distribuir o mover el ejecutable hay
que conservar el banco en una ubicacion que esa busqueda pueda encontrar. Si no
hay un sample apropiado, el Rhythm Box usa su sonido sintetizado interno.

## Sesiones y exportacion

- **Guardar/Abrir** usa el formato propio `.loopernico` e incluye audio de las
  pistas, estado del looper, controles globales, efectos, MIDI y, cuando existe,
  el estado del VST3.
- Las sesiones se validan antes de reservar/restaurar sus datos y el audio se
  adapta a la frecuencia de muestreo actual al cargarlas.
- La mezcla completa y las pistas separadas se exportan como WAV.

El formato de sesion es interno y puede evolucionar; conviene conservar una
copia de las sesiones importantes antes de probar cambios de desarrollo.

## Capturas de interfaz

- `F12` guarda una captura del contenido actual de la aplicacion en
  `Imagenes/Loopernico` sin cerrar el programa.
- El argumento `--snapshot-ui` abre la aplicacion, espera a que termine de
  renderizar, guarda la captura en esa misma carpeta y cierra el programa.
- `--snapshot-ui=ruta/captura.png` permite indicar un archivo de salida. Las
  rutas relativas se resuelven desde el directorio de trabajo.

Este mecanismo usa `Component::createComponentSnapshot`, por lo que no depende
de la captura de ventanas de Windows y funciona tambien cuando la API de captura
del sistema no reconoce la ventana JUCE.

## Verificacion

Actualmente no existe un objetivo de pruebas automatizadas para el codigo propio
del proyecto. La solucion generada contiene solamente la aplicacion, y no se
agrego un framework ni codigo de test al ejecutable para evitar afectar Release.

Antes de publicar un cambio se recomienda:

1. Compilar `Debug | x64` y `Release | x64` sin errores.
2. Abrir la aplicacion y seleccionar entradas/salidas de audio validas.
3. Grabar el loop base y una capa; comprobar reproducir, parar, undo y redo.
4. Guardar y volver a abrir una sesion, tambien con una frecuencia de muestreo
   distinta, comprobando duracion y afinacion.
5. Activar Rhythm Box, cambiar estilo/modo durante la reproduccion y escuchar
   que no haya cortes ni clicks.
6. Cargar/bypassear un VST3 y verificar la restauracion de su estado.
7. Exportar mezcla y pistas y abrir los WAV resultantes en otro reproductor.

Una futura suite de pruebas deberia vivir en un objetivo independiente e incluir,
como minimo, remuestreo y limites de sesiones, cuantizacion, trim, rotacion del
loop y undo/redo.

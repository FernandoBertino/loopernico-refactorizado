# Contexto operativo de Loopernico

Ultima actualizacion: 2026-07-26 (America/Buenos_Aires)

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
  kick, snare, hi-hat cerrado/abierto, crash y ride; la siguiente etapa puede
  agregar articulaciones, swing y una verificacion interactiva especifica con
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

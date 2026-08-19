# Separacion de stems en Loopernico

## Estado actual

La primera etapa implementa la arquitectura offline, la interfaz, progreso,
cancelacion, temporales e importacion a pistas. `MockStemModelBackend` copia la
mezcla original a seis WAV con ganancias distintas y **no separa instrumentos**.
La segunda etapa agrega un adaptador real opcional para `demucs.cpp`, ejecutado
por CPU en un proceso hijo, sin incluir el ejecutable ni los pesos.

El modo ONNX permanece deshabilitado hasta seleccionar un grafo concreto.
Loopernico no descarga ni distribuye pesos automaticamente.

## Arquitectura

- `StemSeparationEngine`: fachada y exclusion de trabajos simultaneos.
- `StemSeparationJob`: hilo de baja prioridad, cancelable mediante atomico.
- `StemModelBackend`: contrato independiente del modelo.
- `MockStemModelBackend`: seis WAV de prueba procesados por bloques.
- `DemucsCppStemModelBackend`: adapta audio a WAV 44.1 kHz estereo, ejecuta el
  port externo por CPU, permite cancelarlo y valida sus seis salidas.
- `OnnxStemModelBackend`: configuracion y guardas para una integracion futura.
- `StemAudioPreprocessor`: validacion y lectura float estereo por bloques de
  WAV, FLAC y MP3 mediante `AudioFormatManager`.
- `StemAudioPostprocessor`: escritura WAV de 24 bits, ganancia/limitacion y
  primitivas de fade para futuras uniones overlap-add.
- `StemImportController`: lectura/remuestreo en worker y commit por movimiento
  de buffers bajo el lock del callback.

Un trabajo recibe un archivo y produce una lista extensible de `StemData`.
Cualquier stem producido puede usarse luego como entrada de otro trabajo; por
eso la API no presupone seis salidas ni una unica jerarquia.

## Hilos y tiempo real

La decodificacion, escritura, normalizacion, remuestreo, reserva de buffers e
inferencia ocurren fuera del callback de audio. El message thread recibe solo
progreso y el resultado preparado. El commit al motor valida que el dispositivo
y las pistas no hayan cambiado, y mueve los buffers durante una seccion corta
protegida por `AudioDeviceManager::getAudioCallbackLock()`.

La interfaz usa `Component::SafePointer`; cerrar `Separar cancion` la oculta y
no destruye el trabajo. Cancelar se comunica mediante `std::atomic_bool` sin
espera activa. Al cerrar la aplicacion se solicita cancelacion antes de cerrar
el dispositivo.

## Temporales

Cada trabajo usa un UUID bajo:

`%TEMP%/Loopernico/StemSeparation/<UUID>/`

Una cancelacion o error elimina solamente el directorio incompleto cuyo padre
fue validado contra esa raiz. Los trabajos correctos se conservan: el motor
actual importa audio a memoria, pero mantener los WAV evita borrar un archivo
que una futura version pudiera referenciar desde una sesion. Esta primera etapa
no realiza limpieza por antiguedad; el usuario puede borrar manualmente trabajos
completos cuando ya no los necesite.

## Importacion

Los stems se importan desde la primera pista libre y nunca sobrescriben pistas
ocupadas. Si hay menos pistas que resultados, se importan en orden solamente
los que entran y se muestra un aviso. Voz, bateria, bajo, guitarra, piano y
otros conservan inicio comun, dos canales y exactamente la misma longitud.

Si el looper esta vacio, el primer stem define la longitud base. Si ya tiene
audio, los stems se remuestrean con `juce::ResamplingAudioSource` y se
recortan/rellenan a la longitud del loop existente. El limite interno sigue
siendo 10 minutos y 12 pistas.

## Incorporar ONNX Runtime

El proyecto Projucer define `LOOPERNICO_ENABLE_ONNX=0`. Para un modelo real:

1. Elegir y documentar un modelo concreto.
2. Completar `OnnxStemModelBackend::ModelMetadata` con sample rate, canales,
   bloque, solapamiento, normalizacion, nombres de tensores, orden de salidas y
   proveedor de ejecucion. No inferir estos valores por el nombre del archivo.
3. Instalar una version x64 compatible de ONNX Runtime fuera del repositorio.
4. En el exporter VS2026 de Projucer, agregar el directorio de `include`, el
   directorio de bibliotecas y `onnxruntime.lib` (y las bibliotecas del proveedor
   elegido). No usar rutas absolutas personales en el `.jucer` compartido.
5. Cambiar la definicion a `LOOPERNICO_ENABLE_ONNX=1` para la configuracion que
   posea esas dependencias e incluir `onnxruntime_cxx_api.h` solamente dentro
   de la rama habilitada.
6. Copiar `onnxruntime.dll` y, para DirectML, las DLL requeridas por ese paquete
   junto a `Loopernico Refactorizado.exe`. CUDA exige ademas las versiones de
   CUDA/cuDNN compatibles con el paquete elegido.
7. Implementar chunking y overlap-add segun el modelo; validar forma/tipo de
   cada tensor en tiempo de ejecucion y rechazar metadatos incompatibles.

El backend debe crear la sesion ONNX y ejecutar inferencia exclusivamente en su
worker. CPU es el fallback; DirectML es la opcion natural para Windows y CUDA
queda opcional. Ningun proveedor debe incorporarse silenciosamente al callback.

## Configurar demucs.cpp por CPU

Loopernico no necesita una GPU para esta ruta. En `Separar cancion`:

1. Seleccionar `Modo 6 stems - Demucs CPU externo`.
2. Elegir el ejecutable x64 de `demucs.cpp` que acepte los argumentos
   `<modelo> <entrada.wav> <directorio-salida>`.
3. Elegir un archivo GGML correspondiente especificamente a `htdemucs_6s`.
4. Seleccionar la cancion y los stems, y pulsar `SEPARAR`.

La aplicacion decodifica WAV, FLAC o MP3 mediante JUCE y crea un WAV temporal
estereo de 44.1 kHz usando `ResamplingAudioSource`. Luego inicia el proceso sin
bloquear audio ni interfaz. `CANCELAR` termina el proceso y limpia el trabajo
incompleto. Se esperan exactamente estos nombres:

- `target_0_drums.wav`
- `target_1_bass.wav`
- `target_2_other.wav`
- `target_3_vocals.wav`
- `target_4_guitar.wav`
- `target_5_piano.wav`

Todos deben ser WAV estereo a 44.1 kHz, no vacios y con la misma longitud. Si el
contrato cambia en otra version del port, Loopernico falla con un mensaje claro
en vez de importar audio desalineado. El ejecutable y el modelo se eligen por
sesion de la aplicacion. En Windows también se detectan automaticamente estas
rutas portables por usuario:

- `%LOCALAPPDATA%/Loopernico/StemSeparation/Runtime/demucs.cpp.main.exe`
- `%LOCALAPPDATA%/Loopernico/StemSeparation/Models/ggml-model-htdemucs-6s-f16.bin`

Si ambos archivos existen, el panel abre en modo Demucs CPU. En otra PC sin esa
instalacion, vuelve de forma segura al mock y permite seleccionar rutas manuales.

### Instalacion personal comprobada

Para el uso personal autorizado por Fernando se compilo localmente el commit
actual de `demucs.cpp` con Visual Studio 2026 y OpenMP. La copia fuente y el
runtime permanecen fuera del repositorio. El peso instalado es
`ggml-model-htdemucs-6s-f16.bin`, 54.855.129 bytes, con SHA-256:

`09704F4CEAE204E56E77D5EEFD6AC71D7275BE81FD507E6913371D59ABCEE856`

Ese valor coincide con el OID LFS publicado para el artefacto. La prueba real
uso un WAV estereo de tres segundos a 48 kHz: Loopernico lo adapto a 44.1 kHz,
la inferencia produjo seis WAV float32 sincronizados y una segunda ejecucion se
cancelo limpiamente. El ciclo diagnostico completo demoro aproximadamente 90 s
en un Intel i5-10400 y el proceso de inferencia observo cerca de 468 MB de RAM.
No se uso GPU. Una cancion completa tardara considerablemente mas.

Una segunda prueba uso `gg.wav`, WAV musical estereo de 10 s, 48 kHz y 24 bits.
El ciclo de inferencia, importacion y cancelacion demoro 146 s. Los seis stems
float32 conservaron exactamente 441000 frames cada uno y fueron importados en
seis pistas libres. RMS medido: bateria 0.040144, bajo 0.028049, otros 0.063034,
voz 0.028032, guitarra 0.053051 y piano 0.028050. Las diferencias y
correlaciones confirman que no son copias identicas. Los WAV conservados para
revision auditiva estan en `tmp/stem-musical-real/results/`.

Esta instalacion local no cambia la separacion entre licencias ni concede
permiso de redistribucion. Si Loopernico se copia a otra PC, se deben instalar
alli el runtime y el modelo en las mismas rutas por usuario o seleccionarlos
manualmente.

## Licencias y distribucion

Antes de distribuir un modelo hay que verificar por separado:

- licencia del codigo o arquitectura del modelo;
- licencia de los pesos entrenados;
- permiso de redistribucion de pesos y ONNX Runtime;
- permiso para uso comercial;
- atribuciones, avisos y texto de licencia requeridos;
- restricciones de datasets de entrenamiento;
- tamano de descarga/instalacion;
- RAM, CPU/GPU, VRAM y versiones de driver requeridas.

La licencia de ONNX Runtime no concede automaticamente derechos sobre un
modelo ni sus pesos. Un modelo permitido para investigacion puede prohibir su
redistribucion o uso comercial.

## Evaluacion de modelos (2026-08-04)

### HTDemucs 6s

Es el candidato que coincide exactamente con la primera interfaz de Loopernico:
`drums`, `bass`, `other`, `vocals`, `guitar` y `piano`, a 44.1 kHz estereo. El
propio proyecto califica el modelo de seis fuentes como experimental y advierte
que piano puede presentar sangrado y artefactos.

No se encontro una exportacion ONNX oficial y verificable. La implementacion
original usa ramas de forma de onda y espectrograma, STFT/iSTFT, normalizacion
interna y Transformer; por eso no corresponde inventar una conversion generica
ni sus tensores. `demucs.cpp` ofrece un port C++17 de baja memoria y soporta el
modelo de seis fuentes con pesos convertidos a GGML, pero es un proyecto tercero
y no un backend ONNX.

La licencia MIT esta declarada para el codigo de Demucs, para `demucs.cpp` y en
la ficha de los pesos GGML convertidos. Sin embargo, el repositorio oficial no
publica una declaracion separada e inequívoca que resuelva la redistribucion de
los checkpoints preentrenados; existe una consulta abierta precisamente sobre
ese punto. Por esa razon Loopernico no descarga, incluye ni redistribuye esos
pesos en esta etapa. Antes de hacerlo se requiere confirmacion del titular o una
licencia explicita del checkpoint original, no solamente la licencia del port o
del archivo convertido.

En el equipo comprobado (GTX 1050 con 2 GB de VRAM), la ruta CUDA oficial no es
una opcion prudente: Demucs documenta un minimo normal de 3 GB y recomienda
aproximadamente 7 GB. CPU es viable pero lenta. El port `demucs.cpp` fue pensado
para memoria reducida a costa de velocidad y por eso es el candidato tecnico
mas razonable si se acepta un backend distinto de ONNX y pesos suministrados por
el usuario con licencia verificada.

### Banquet / Query-Bandit

Es una arquitectura mas extensible para consultas de instrumentos y reporta
mejoras en guitarra y piano frente a HTDemucs 6s. No obstante, su inferencia de
referencia usa lotes orientados a una RTX 4090 y no hay un adaptador ONNX oficial
verificado para Loopernico. Tampoco se incorporaran sus checkpoints hasta
comprobar directamente la licencia de cada artefacto. Queda como candidato para
hardware futuro, no para la primera integracion local en este equipo.

### Decision pendiente

Hay tres rutas posibles y no son intercambiables a nivel de dependencias:

1. **Elegida e implementada:** integrar `demucs.cpp` como backend C++/CPU
   opcional, con ejecutable y pesos externos elegidos por el usuario. Coincide
   con seis stems y evita depender de una GPU concreta.
2. Invocar Demucs Python como proceso externo opcional. Es el prototipo real mas
   corto, pero agrega Python, PyTorch y gestion de entorno/proceso al producto.
3. Mantener exclusivamente ONNX. Conserva el plan original, pero no permite
   implementar inferencia real hasta seleccionar o producir una exportacion
   concreta con metadatos, pruebas de paridad y licencia verificable.

El modo Demucs CPU queda disponible cuando el usuario configura ambos archivos;
ONNX permanece como acelerador futuro. No se incorporo ninguna descarga,
dependencia binaria ni peso durante esta implementacion.

## Compilacion y diagnostico

1. Abrir `Loopernico Refactorizado.jucer` en Projucer y guardar.
2. Compilar `Builds/VisualStudio2026/Loopernico Refactorizado.sln` en
   `Release | x64`.
3. Para el diagnostico integrado:

   `Loopernico Refactorizado.exe --run-stem-separation-test="C:\ruta\salida" --snapshot-force-fallback`

El diagnostico crea un WAV conocido, comprueba seis resultados sincronizados y
distinguibles, importa las seis pistas y valida cancelacion/limpieza. Escribe
`stem-separation-test-report.txt`.

## Limitaciones pendientes

- No hay separacion real ni modelo seleccionado.
- La normalizacion del mock es global para conservar sus ganancias relativas.
- El overlap-add dependiente del modelo no se implementa hasta conocer su
  ventana, bloque y solapamiento exactos.
- Los nombres visibles de stems no se guardan todavia en `.loopernico`.
- Los trabajos correctos no tienen limpieza automatica por antiguedad.
- MP3 depende del codec que `registerBasicFormats()` ofrezca en la compilacion
  JUCE de destino; un MP3 no decodificable produce un error visible.

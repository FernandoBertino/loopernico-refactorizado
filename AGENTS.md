# Instrucciones permanentes de Loopernico

Estas reglas complementan las preferencias globales de Fernando y se aplican a todo este repositorio.

## Contexto operativo persistente

- Al comenzar cualquier conversacion o tarea en este repositorio, leer completamente `PROJECT_CONTEXT.md` si existe antes de diagnosticar, editar, compilar o desplegar.
- Tratar `PROJECT_CONTEXT.md` como un traspaso operativo: verificar sus afirmaciones contra el codigo, `git status` y el estado real de los builds; una tarea marcada como pendiente no esta terminada.
- Despues de un avance material, una nueva medicion, un bloqueo o antes de cerrar una tarea incompleta, actualizar `PROJECT_CONTEXT.md` con fecha, estado comprobado, verificaciones realizadas y proximo paso concreto.
- Mantener el contexto compacto y actual: reemplazar informacion obsoleta en vez de acumular una cronologia indefinida. No guardar secretos, tokens, datos personales ni grandes logs.

## Aplicacion de uso diario y despliegue local

- Loopernico es una aplicacion que Fernando usa todos los dias. Mantener siempre disponible una ultima version estable y lista para abrir desde el Escritorio.
- Despues de cualquier cambio que afecte lo que ejecuta o muestra la aplicacion, por minimo que sea, compilar `Release | x64` con la solucion de Visual Studio generada por JUCE. Esto incluye cambios aislados de codigo, comportamiento, interfaz o recursos visuales, como reemplazar solamente la textura de un knob.
- Cada compilacion Release correcta debe tratarse como la nueva version de uso diario, sin reservar la actualizacion del despliegue para cambios grandes.
- Solo cuando la compilacion Release finalice correctamente, actualizar o recrear obligatoriamente `C:\Users\Fernando\Desktop\Loopernico.lnk` para que apunte al ejecutable de esa compilacion mas reciente, con el directorio de trabajo y el icono correctos.
- Conservar exactamente las dos compilaciones Release funcionales mas recientes en el despliegue local: la nueva version de uso diario y la inmediatamente anterior como respaldo manual. Antes de sustituir la version vigente, preservar una copia ejecutable de ella como respaldo; despues de validar la nueva, eliminar solamente los builds mas antiguos que esas dos versiones.
- No contar como version conservada un build fallido, incompleto o que no supere la verificacion basica. No eliminar el respaldo anterior hasta confirmar que el nuevo ejecutable abre correctamente y que el acceso directo apunta a el.
- Despues de un Release exitoso, comprobar que la asociacion `.loopernico` siga apuntando al mismo ejecutable, conserve el tipo `Archivo de Loopernico` y use el icono de sesion. Actualizarla y refrescar el Explorador solamente si fuera necesario.
- Verificar siempre el destino del acceso directo y efectuar una prueba de humo del ejecutable nuevo, incluso cuando el cambio sea minimo.
- Si el build o la prueba de humo fallan, no reemplazar ni invalidar el acceso directo o la asociacion que Fernando usa diariamente, ni rotar o borrar las dos versiones funcionales conservadas. Mantener la ultima version funcional como version de uso diario y reportar el error.

## Pedales fisicos, marcas y fidelidad

- Este proyecto es personal y no comercial. Fernando autoriza el uso dentro de Loopernico de nombres comerciales, marcas registradas, rotulos y rasgos visuales de pedales fisicos reales como referencia de implementacion.
- Cada pedal busca imitar con la mayor fidelidad practica al equipo fisico real, tanto en apariencia como en funciones y comportamiento sonoro.
- Para el aspecto visual, considerar forma y proporciones del gabinete, materiales, colores, tipografia, logotipos, rotulos, distribucion de controles, perillas, selectores, luces, conectores, tornillos y desgaste cuando corresponda.
- Para el comportamiento, reproducir controles, rangos, modos, conmutadores, interacciones y respuesta DSP relevantes del pedal real, dentro de las posibilidades tecnicas del proyecto.
- Usar referencias visuales y documentacion confiables cuando sean necesarias para lograr fidelidad. No sustituir automaticamente el diseno por uno generico cuando el pedido sea reproducir un pedal concreto.
- Tratar la fidelidad visual y funcional como un proceso de mejora continua. Dentro del alcance del pedal solicitado, corregir diferencias comprobables y conservar compatibilidad con sesiones existentes.
- La mejora continua no autoriza modificar otros pedales o subsistemas fuera del pedido actual; anotar por separado las oportunidades detectadas.

## Seguridad del codigo y alcance

- No tocar pedales, efectos, flujos de sesion, MIDI, audio o interfaz ajenos a la tarea salvo que sea estrictamente necesario para integrarla.
- Preservar cambios locales existentes y no mezclar trabajo no relacionado.
- Mantener las operaciones costosas, asignaciones, acceso a disco, plugins y trabajo de interfaz fuera del callback de audio en tiempo real.
- Evitar bloqueos, esperas y operaciones no deterministas en el hilo de audio.
- Mantener la lectura de sesiones `.loopernico` anteriores siempre que sea razonablemente posible; migrar datos antiguos de forma explicita cuando cambie un formato.
- Regenerar los archivos de proyecto mediante Projucer cuando se modifique `Loopernico Refactorizado.jucer` o sus recursos administrados.
- No agregar bancos ni archivos de audio al repositorio salvo pedido explicito. Excluir tambien builds, temporales y capturas de verificacion de los commits normales.

## Verificacion minima

- Para cambios de codigo o interfaz, compilar como minimo `Release | x64`.
- Cuando corresponda, abrir la aplicacion y verificar el flujo afectado sin depender solamente de que compile.
- Para cambios de sesiones o asociaciones, comprobar apertura directa de un `.loopernico` real.
- Para cambios visuales, generar o revisar una captura y comprobar legibilidad, proporciones y ausencia de regresiones evidentes.

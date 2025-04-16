# BetterSpray 

**BetterSpray** es un plugin para MetaHookSV que mejora el sistema de sprays de Sven Co-op con soporte para múltiples imágenes, proporciones reales y recarga dinámica.

![Vista previa de BetterSpray](https://raw.githubusercontent.com/KazamiiSC/BetterSpray---Sven-Co-op/refs/heads/main/preview/20250416020128_1.jpg)

## 🌟 Características principales

### 🖼️ Gestión de Sprays
- ✅ Soporte para múltiples sprays (PNG/JPG)
- ✅ Detección automática de imágenes en `custom_sprays`
- ✅ Recarga dinámica sin reiniciar el juego (`sprayreload`)
- ✅ Lista organizada de sprays (`spraylist`)

### 🎨 Personalización avanzada
- 🔧 Ajuste de tamaño (`sprayscale 5-60`)
- 🔄 Rotación libre (`sprayrotate 0-360`)
- 💡 Control de brillo (`spraybrightness 0.1-2.0`)
- ⏳ Duración: 60 segundos + 2 segundos de desvanecimiento
- 📐 **Mantiene la proporción original de la imagen** (720x420, etc.)

### 🚀 Rendimiento y calidad
- 🖌️ Renderizado mediante OpenGL
- 🔍 Soporte para texturas de hasta 1024x1024
- 💾 Optimización de memoria

## 📥 Instalación

Solo descarga desde la sección de Releases y colócalo en tu carpeta principal de Sven Co-op.  
O sigue estos pasos manualmente:

1. Copia `BetterSpray.dll` a:  
   `Sven Co-op/svencoop/metahook/plugins/`

2. Copia y reemplaza `SDL2.dll`, `SDL3.dll` y `svencoop.exe` en:  
   `Steam/steamapps/common/Sven Co-op`  
   *(Haz una copia de seguridad de tu `svencoop.exe` antes)*

3. Crea la carpeta:  
   `Sven Co-op/custom_sprays/`

4. Agrega tus imágenes (PNG/JPG):  
   - `spray1.png`  
   - `mi_logo.jpg`  
   - `cualquier_nombre.jpeg`

## 🕹️ Jugar y disfrutar

| Comando              | Descripción                           | Ejemplo                |
|----------------------|----------------------------------------|------------------------|
| `spray`              | Coloca el spray actual                 | -                      |
| `spraynext`          | Cambia al siguiente spray              | -                      |
| `spraynext nombre`   | Usa un spray específico                | `spraynext logo.png`   |
| `spraylist`          | Muestra todos los sprays disponibles   | -                      |
| `sprayreload`        | Recarga la carpeta de sprays           | -                      |

## ⚙️ Ajustes visuales

| Comando              | Rango        | Descripción                    | Ejemplo               |
|----------------------|--------------|--------------------------------|------------------------|
| `sprayscale`         | 5.0 - 60.0   | Tamaño vertical del spray      | `sprayscale 50`       |
| `sprayrotate`        | 0 - 360      | Rotación en grados             | `sprayrotate 45`      |
| `spraybrightness`    | 0.1 - 2.0    | Brillo o intensidad del spray  | `spraybrightness 1.5` |

## 📌 Notas importantes

- ✨ **Nuevo**: ¡Las imágenes ahora mantienen su proporción original!
- 📏 Tamaño máximo permitido: 1024x1024 píxeles
- 🔄 Usa `sprayreload` después de añadir nuevas imágenes
- 🎨 Formatos soportados: PNG, JPG, JPEG

### 🎯 Configuración recomendada

```plaintext
sprayscale 35
spraybrightness 1.2

🚀 Hoja de ruta (Roadmap)

Sistema de favoritos
Soporte multijugador
Efectos especiales (brillo, bordes, etc.)

❓ Preguntas frecuentes
P: ¿Por qué mi imagen se ve estirada?
R: ¡Ya no debería! El plugin ahora mantiene las proporciones originales.

P: ¿Cómo actualizo la lista de sprays?
R: Usa el comando sprayreload después de agregar nuevas imágenes.

P: ¿Cuál es el tamaño máximo de imagen permitido?
R: El límite es de 1024x1024 píxeles.

🛠 Versión actual: 3.1
❤️ Creado por: KZ-Sheez

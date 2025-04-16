# BetterSpray 

**BetterSpray** es un plugin para MetaHookSV que mejora el sistema de sprays de Sven Co-op con soporte para múltiples imágenes, proporciones reales y recarga dinámica.

## 🌟 Características principales

### 🖼️ Gestión de Sprays
- ✅ Soporte para múltiples sprays (PNG/JPG)
- ✅ Detección automática de imágenes en `custom_sprays`
- ✅ Recarga dinámica sin reiniciar (`sprayreload`)
- ✅ Listado organizado (`spraylist`)

### 🎨 Personalización avanzada
- 🔧 Ajuste de tamaño (`sprayscale 5-60`)
- 🔄 Rotación libre (`sprayrotate 0-360`)
- 💡 Control de brillo (`spraybrightness 0.1-2.0`)
- ⏳ Duración: 60s + 2s de desvanecimiento
- 📐 **Mantiene proporciones originales** (720x420, etc.)

### 🚀 Rendimiento y calidad
- 🖌️ Renderizado con OpenGL
- 🔍 Texturas de hasta 1024x1024
- 💾 Optimización de memoria

## 📥 Instalación

1. Copiar `BetterSpray.dll` a:  
   `Sven Co-op/svencoop/metahook/plugins/`

2. Crear carpeta:  
   `Sven Co-op/custom_sprays/`

3. Añadir imágenes (PNG/JPG):  
spray1.png
mi_logo.jpg
cualquier_nombre.jpeg

markdown
Copiar
Editar

## 🕹️ Jugar y Disfrutar

| Comando              | Descripción                          | Ejemplo               |
|----------------------|--------------------------------------|-----------------------|
| `spray`              | Coloca el spray actual               | -                     |
| `spraynext`          | Cambia al siguiente spray            | -                     |
| `spraynext nombre`   | Usa un spray específico              | `spraynext logo.png`  |
| `spraylist`          | Muestra todos los sprays disponibles | -                     |
| `sprayreload`        | Recarga la carpeta de sprays         | -                     |

## ⚙️ Ajustes visuales

| Comando              | Rango       | Descripción               | Ejemplo              |
|----------------------|-------------|---------------------------|----------------------|
| `sprayscale`         | 5.0 - 60.0  | Tamaño vertical           | `sprayscale 50`      |
| `sprayrotate`        | 0 - 360     | Rotación en grados        | `sprayrotate 45`     |
| `spraybrightness`    | 0.1 - 2.0   | Intensidad del spray      | `spraybrightness 1.5`|

## 📌 Notas importantes

- ✨ **Nuevo**: ¡Las imágenes mantienen sus proporciones originales!
- 📏 Tamaño máximo: 1024x1024 píxeles
- 🔄 Usa `sprayreload` después de añadir nuevos sprays
- 🎨 Formatos soportados: PNG, JPG, JPEG

### 🎯 Ajustes recomendados
sprayscale 35
spraybrightness 1.2

markdown
Copiar
Editar

## 🚀 Roadmap

- Soporte para proporciones reales  
- Recarga dinámica de sprays  
- Sistema de favoritos  
- Soporte multiplayer  
- Efectos especiales (brillo, bordes)

## ❓ Preguntas frecuentes

**P:** ¿Por qué mi imagen se ve estirada?  
**R:** ¡Ya no debería! El plugin ahora mantiene las proporciones originales.

**P:** ¿Cómo actualizo la lista de sprays?  
**R:** Usa el comando `sprayreload` después de añadir nuevas imágenes.

**P:** ¿Qué tamaño máximo pueden tener las imágenes?  
**R:** El límite es 1024x1024 píxeles.

---

🛠 Versión actual: **3.1**  
❤️ Creado por: **KZ-Sheez**

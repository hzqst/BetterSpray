#define M_PI 3.14159265358979323846f
#define _USE_MATH_DEFINES

#include <metahook.h>
#include <windows.h>
#include <GL/gl.h>
#include "vector_math.h"
#include <interface.h>
#include <IPlugins.h>
#include <enginefuncs.h>
#include <exportfuncs.h>
#include <pmtrace.h>
#include <pm_defs.h>
#include <pm_shared.h>
#include <string.h>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <cmath>
#include "SOIL/stb_image.h"

namespace fs = std::filesystem;

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 0x84C1
typedef void (APIENTRY* PFNGLACTIVETEXTUREPROC)(GLenum texture);
PFNGLACTIVETEXTUREPROC glActiveTexture = nullptr;

cl_enginefunc_t gEngfuncs = {};
cl_exportfuncs_t gExportFuncs = {};
cl_exportfuncs_t g_OriginalExportFuncs = {};
extern metahook_api_t* g_pMetaHookAPI;
metahook_api_t* g_pMetaHookAPI = nullptr;

std::vector<std::string> g_SprayList;
int g_CurrentSprayIndex = 0;
GLuint g_nSprayTexture = 0;
bool g_bShowSpray = false;
float g_fSprayShowTime = 0;
vec3_t g_vSprayOrigin;
vec3_t g_vSprayNormal;
float g_fSprayBrightness = 1.0f;
float g_fSprayDuration = 60.0f;
float g_fSprayFadeDuration = 2.0f;
float g_fSprayScale = 45.0f;
float g_fSprayRotation = 0.0f;

// Declaración adelantada de funciones
bool LoadSprayTexture(const char* filepath);
void LoadSprayList();
void SprayReloadCommand();

// Implementación de LoadSprayTexture primero
bool LoadSprayTexture(const char* filepath) {
    int width, height, channels;
    unsigned char* imageData = stbi_load(filepath, &width, &height, &channels, STBI_rgb_alpha);
    if (!imageData) {
        gEngfuncs.Con_Printf("BetterSpray: Failed to load spray texture: %s\n", filepath);
        return false;
    }

    if (width > 1024 || height > 1024) {
        gEngfuncs.Con_Printf("BetterSpray: Warning: Image %s exceeds maximum size (1024x1024)\n", filepath);
        stbi_image_free(imageData);
        return false;
    }

    if (g_nSprayTexture)
        glDeleteTextures(1, &g_nSprayTexture);

    glGenTextures(1, &g_nSprayTexture);
    glBindTexture(GL_TEXTURE_2D, g_nSprayTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(imageData);

    gEngfuncs.Con_Printf("BetterSpray: Loaded %s (%dx%d)\n", filepath, width, height);
    return true;
}

void LoadSprayList() {
    g_SprayList.clear();
    for (const auto& entry : fs::directory_iterator("custom_sprays")) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
                g_SprayList.push_back(entry.path().string());
            }
        }
    }
    std::sort(g_SprayList.begin(), g_SprayList.end());
    gEngfuncs.Con_Printf("BetterSpray: Found %d image(s).\n", (int)g_SprayList.size());
}

void SprayReloadCommand() {
    LoadSprayList();
    if (!g_SprayList.empty()) {
        if (g_CurrentSprayIndex >= static_cast<int>(g_SprayList.size())) {
            g_CurrentSprayIndex = 0;
        }
        LoadSprayTexture(g_SprayList[g_CurrentSprayIndex].c_str());
    }
    gEngfuncs.Con_Printf("BetterSpray: Reloaded spray list (%d sprays available)\n", (int)g_SprayList.size());
}

void NextSpray() {
    if (g_SprayList.empty()) {
        gEngfuncs.Con_Printf("BetterSpray: No sprays found.\n");
        return;
    }

    g_CurrentSprayIndex = (g_CurrentSprayIndex + 1) % g_SprayList.size();
    LoadSprayTexture(g_SprayList[g_CurrentSprayIndex].c_str());

    gEngfuncs.Con_Printf("BetterSpray: Now using [%s]\n", g_SprayList[g_CurrentSprayIndex].c_str());
}

void SprayBrightnessCommand() {
    const char* arg = gEngfuncs.Cmd_Argv(1);
    if (!arg) {
        gEngfuncs.Con_Printf("BetterSpray: Current brightness is %.2f\n", g_fSprayBrightness);
        return;
    }

    float value = static_cast<float>(atof(arg));
    if (value < 0.1f || value > 2.0f) {
        gEngfuncs.Con_Printf("BetterSpray: Brightness must be between 0.1 and 2.0\n");
        return;
    }

    g_fSprayBrightness = value;
    gEngfuncs.Con_Printf("BetterSpray: Brightness set to %.2f\n", g_fSprayBrightness);
}

void SprayScaleCommand() {
    const char* arg = gEngfuncs.Cmd_Argv(1);
    if (!arg) {
        gEngfuncs.Con_Printf("BetterSpray: Current scale is %.2f\n", g_fSprayScale);
        return;
    }

    float value = static_cast<float>(atof(arg));
    if (value < 5.0f || value > 60.0f) {
        gEngfuncs.Con_Printf("BetterSpray: Scale must be between 5.0 and 60.0\n");
        return;
    }

    g_fSprayScale = value;
    gEngfuncs.Con_Printf("BetterSpray: Scale set to %.2f\n", g_fSprayScale);
}

void SprayRotateCommand() {
    const char* arg = gEngfuncs.Cmd_Argv(1);
    if (!arg) {
        gEngfuncs.Con_Printf("BetterSpray: Current rotation is %.2f degrees\n", g_fSprayRotation);
        return;
    }

    g_fSprayRotation = static_cast<float>(atof(arg));
    gEngfuncs.Con_Printf("BetterSpray: Rotation set to %.2f degrees\n", g_fSprayRotation);
}

void SprayListCommand() {
    if (g_SprayList.empty()) {
        gEngfuncs.Con_Printf("BetterSpray: No sprays found in custom_sprays folder.\n");
        return;
    }

    gEngfuncs.Con_Printf("BetterSpray: Available sprays (%d total):\n", (int)g_SprayList.size());
    for (size_t i = 0; i < g_SprayList.size(); i++) {
        const char* indicator = (i == g_CurrentSprayIndex) ? ">>" : "  ";
        std::string filename = fs::path(g_SprayList[i]).filename().string();
        gEngfuncs.Con_Printf("%s %d. %s\n", indicator, (int)i + 1, filename.c_str());
    }
}

void SprayNextCommand() {
    const char* arg = gEngfuncs.Cmd_Argv(1);
    if (!arg || strlen(arg) == 0) {
        NextSpray();
        return;
    }

    std::string target(arg);
    std::transform(target.begin(), target.end(), target.begin(), ::tolower);

    auto it = std::find_if(g_SprayList.begin(), g_SprayList.end(), [&](const std::string& path) {
        std::string name = fs::path(path).filename().string();
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        return name == target;
        });

    if (it != g_SprayList.end()) {
        g_CurrentSprayIndex = std::distance(g_SprayList.begin(), it);
        LoadSprayTexture(it->c_str());
        gEngfuncs.Con_Printf("BetterSpray: Now using [%s]\n", it->c_str());
    }
    else {
        gEngfuncs.Con_Printf("BetterSpray: Spray [%s] not found.\n", arg);
    }
}

void PlaceSpray() {
    vec3_t viewAngles, forward, start, end;
    gEngfuncs.GetViewAngles(viewAngles);
    AngleVectors(viewAngles, forward, nullptr, nullptr);

    VectorCopy(gEngfuncs.GetLocalPlayer()->origin, start);
    start[2] += 24;

    VectorMA(start, 128.0f, forward, end);

    pmtrace_t* trace = gEngfuncs.PM_TraceLine(start, end, PM_TRACELINE_ANYVISIBLE, 2, -1);

    if (trace->fraction < 1.0f) {
        VectorCopy(trace->endpos, g_vSprayOrigin);
        VectorCopy(trace->plane.normal, g_vSprayNormal);
        g_bShowSpray = true;
        g_fSprayShowTime = gEngfuncs.GetClientTime();
    }
}

void DrawSpray() {
    if (!g_nSprayTexture || !g_bShowSpray)
        return;

    float elapsed = gEngfuncs.GetClientTime() - g_fSprayShowTime;

    if (elapsed > g_fSprayDuration + g_fSprayFadeDuration) {
        g_bShowSpray = false;
        return;
    }

    float alpha = 1.0f;
    if (elapsed > g_fSprayDuration) {
        alpha = 1.0f - ((elapsed - g_fSprayDuration) / g_fSprayFadeDuration);
    }

    vec3_t right, up, normal, tmp = { 0, 0, 1 };
    VectorCopy(g_vSprayNormal, normal);
    VectorNormalize(normal);

    if (fabs(normal[2]) > 0.99f)
        VectorSet(tmp, 1, 0, 0);

    CrossProduct(normal, tmp, right);
    VectorNormalize(right);
    CrossProduct(right, normal, up);
    VectorNormalize(up);

    if (g_fSprayRotation != 0.0f) {
        float angle = g_fSprayRotation * (M_PI / 180.0f);
        float cosAngle = cos(angle);
        float sinAngle = sin(angle);

        vec3_t newRight, newUp;
        for (int i = 0; i < 3; i++) {
            newRight[i] = right[i] * cosAngle - up[i] * sinAngle;
            newUp[i] = right[i] * sinAngle + up[i] * cosAngle;
        }
        VectorCopy(newRight, right);
        VectorCopy(newUp, up);
    }

    GLint width = 1, height = 1;
    glBindTexture(GL_TEXTURE_2D, g_nSprayTexture);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
    glBindTexture(GL_TEXTURE_2D, 0);

    float aspectRatio = (float)width / (float)height;
    float sizeH = g_fSprayScale;
    float sizeW = g_fSprayScale * aspectRatio;

    vec3_t verts[4];
    for (int i = 0; i < 3; i++) {
        verts[0][i] = g_vSprayOrigin[i] - right[i] * sizeW + up[i] * sizeH;
        verts[1][i] = g_vSprayOrigin[i] + right[i] * sizeW + up[i] * sizeH;
        verts[2][i] = g_vSprayOrigin[i] + right[i] * sizeW - up[i] * sizeH;
        verts[3][i] = g_vSprayOrigin[i] - right[i] * sizeW - up[i] * sizeH;
    }

    glPushAttrib(GL_ALL_ATTRIB_BITS);

    if (glActiveTexture) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glCullFace(GL_BACK);
    glBindTexture(GL_TEXTURE_2D, g_nSprayTexture);

    glColor4f(g_fSprayBrightness, g_fSprayBrightness, g_fSprayBrightness, alpha);

    glBegin(GL_QUADS);
    glTexCoord2f(1, 0); glVertex3fv(verts[0]);
    glTexCoord2f(0, 0); glVertex3fv(verts[1]);
    glTexCoord2f(0, 1); glVertex3fv(verts[2]);
    glTexCoord2f(1, 1); glVertex3fv(verts[3]);
    glEnd();

    glColor4f(1, 1, 1, 1);

    glBindTexture(GL_TEXTURE_2D, 0);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glPopAttrib();
}

void SprayCommand() {
    PlaceSpray();
    gEngfuncs.Con_Printf("BetterSpray: Spray placed.\n");
}

void ImpulseHandler() {
    const char* arg1 = gEngfuncs.Cmd_Argv(1);
    if (arg1 && strcmp(arg1, "201") == 0)
        SprayCommand();
}

int HUD_Redraw(float time, int intermission) {
    if (g_OriginalExportFuncs.HUD_Redraw)
        return g_OriginalExportFuncs.HUD_Redraw(time, intermission);
    return 1;
}

void HUD_DrawNormalTriangles() {
    DrawSpray();
    if (g_OriginalExportFuncs.HUD_DrawNormalTriangles)
        g_OriginalExportFuncs.HUD_DrawNormalTriangles();
}

class CBetterSpray : public IPluginsV4 {
public:
    void Init(metahook_api_t* pMetaHookAPI, mh_interface_t*, mh_enginesave_t*) override {
        g_pMetaHookAPI = pMetaHookAPI;
    }

    void LoadEngine(cl_enginefunc_t* pEngineFuncs) override {
        gEngfuncs = *pEngineFuncs;
    }

    void LoadClient(cl_exportfuncs_t* pExportFuncs) override {
        g_OriginalExportFuncs = *pExportFuncs;
        pExportFuncs->HUD_Redraw = HUD_Redraw;
        pExportFuncs->HUD_DrawNormalTriangles = HUD_DrawNormalTriangles;
        gExportFuncs = *pExportFuncs;

        gEngfuncs.pfnAddCommand("spray", SprayCommand);
        gEngfuncs.pfnAddCommand("spraynext", SprayNextCommand);
        gEngfuncs.pfnAddCommand("spraybrightness", SprayBrightnessCommand);
        gEngfuncs.pfnAddCommand("sprayscale", SprayScaleCommand);
        gEngfuncs.pfnAddCommand("sprayrotate", SprayRotateCommand);
        gEngfuncs.pfnAddCommand("spraylist", SprayListCommand);
        gEngfuncs.pfnAddCommand("sprayreload", SprayReloadCommand);

        if (g_pMetaHookAPI)
            g_pMetaHookAPI->HookCmd("impulse", ImpulseHandler);

        glActiveTexture = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");

        LoadSprayList();
        if (!g_SprayList.empty()) {
            LoadSprayTexture(g_SprayList[0].c_str());
        }
    }

    void Shutdown() override {
        if (g_nSprayTexture) {
            glDeleteTextures(1, &g_nSprayTexture);
            g_nSprayTexture = 0;
        }
    }

    void ExitGame(int) override {}

    const char* GetVersion() override {
        return "BetterSpray Plugin v3.1 (Dynamic reload + aspect ratio support)";
    }
};

CBetterSpray g_BetterSpray;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CBetterSpray, IPluginsV4, METAHOOK_PLUGIN_API_VERSION_V4, g_BetterSpray);
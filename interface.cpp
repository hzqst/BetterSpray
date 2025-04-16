#include <cstring> 
#include "interface.h"


// Inicializamos la lista global de interfaces
InterfaceReg* InterfaceReg::s_pInterfaceRegs = nullptr;

// Constructor de InterfaceReg: se auto-registra en la lista global
InterfaceReg::InterfaceReg(InstantiateInterfaceFn fn, const char* pName)
{
    m_CreateFn = fn;
    m_pName = pName;
    m_pNext = s_pInterfaceRegs;
    s_pInterfaceRegs = this;
}

// Esta función es llamada por MetaHookSV para obtener el plugin
extern "C" EXPORT_FUNCTION IBaseInterface* CreateInterface(const char* pName, int* pReturnCode)
{
    InterfaceReg* curr = InterfaceReg::s_pInterfaceRegs;

    while (curr)
    {
        if (strcmp(pName, curr->m_pName) == 0)
        {
            if (pReturnCode)
                *pReturnCode = IFACE_OK;
            return curr->m_CreateFn();
        }

        curr = curr->m_pNext;
    }

    if (pReturnCode)
        *pReturnCode = IFACE_FAILED;

    return nullptr;
}

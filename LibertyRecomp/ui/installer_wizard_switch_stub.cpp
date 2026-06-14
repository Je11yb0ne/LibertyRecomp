#include "installer_wizard.h"

#include <cstdio>

void InstallerWizard::Init()
{
    s_isVisible = false;
}

void InstallerWizard::Draw()
{
}

void InstallerWizard::Shutdown()
{
    s_isVisible = false;
}

bool InstallerWizard::Run(std::filesystem::path, bool)
{
    s_isVisible = false;
    fprintf(stderr, "[Switch] Installer wizard is not implemented in the audit NRO.\n");
    return false;
}

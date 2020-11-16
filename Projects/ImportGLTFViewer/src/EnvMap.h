#pragma once

namespace Diligent
{

enum class BackgroundMode : int
{
    None,
    EnvironmentMap,
    Irradiance,
    PrefilteredEnvMap,
    NumModes
};

}
#include "avs_ssimulacra.h"


static const char* AVSC_CC avisynth_c_plugin_init(AVS_ScriptEnvironment* env)
{
    avs_add_function(env, "ssimulacra", "cc[simple]b", Create_ssimulacra, 0);
    avs_add_function(env, "ssimulacra2", "cc", Create_ssimulacra2, 0);

    return "ssimulacra";
}

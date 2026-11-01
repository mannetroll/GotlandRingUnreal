#pragma once
#include "CoreMinimal.h"
#include "Misc/ConfigCacheIni.h"
namespace Ring {
inline FString VersionLabel(){
 FString Version;
 GConfig->GetString(TEXT("/Script/EngineSettings.GeneralProjectSettings"),TEXT("ProjectVersion"),Version,GGameIni);
 return TEXT("v")+Version;
}
}

#pragma once
#include "CoreMinimal.h"
#include "Components/SynthComponent.h"
#include <atomic>
#include <vector>
#include "RingAudio.generated.h"
struct FRingAudioState {
 std::atomic<float> Rpm{900},Load{0},Speed{0};std::atomic<bool> Muted{false};
 std::atomic<int32> Rate{0},Written{0};
 // Bounded capture used only by the standalone validation run.
 std::vector<float> Capture;
};
UCLASS(ClassGroup=Audio)
class GOTLANDRINGUNREAL_API URingAudio : public USynthComponent {
 GENERATED_BODY()
public:
 URingAudio(const FObjectInitializer& ObjectInitializer);
 TSharedPtr<FRingAudioState,ESPMode::ThreadSafe> State;
 void SetEngine(float Rpm,float Load,float Speed,bool Muted);
 void CaptureTestAudio();
 bool SaveCapture(const FString& Filename,float& Peak,int32& Samples);
protected:
 virtual ISoundGeneratorPtr CreateSoundGenerator(const FSoundGeneratorInitParams& Params) override;
};

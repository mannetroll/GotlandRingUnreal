#include "RingAudio.h"
#include "RingEngineMixer.h"
#include "Misc/FileHelper.h"
class FRingGenerator final : public ISoundGenerator {
 Ring::EngineMixer Mixer;
 TSharedPtr<FRingAudioState,ESPMode::ThreadSafe> State;
 int32 Rate,Channels;
public:
 FRingGenerator(TSharedPtr<FRingAudioState,ESPMode::ThreadSafe> InState,const FSoundGeneratorInitParams& P):State(InState),Rate(int32(P.SampleRate)),Channels(P.NumChannels){State->Rate.store(Rate);}
 int32 GetNumChannels()const override{return Channels;}
 int32 OnGenerateAudio(float* Out,int32 Count)override{
  Mixer.Render(Out,Count,Channels,Rate,State->Rpm.load(),State->Load.load(),State->Speed.load(),State->Muted.load());
  int32 Written=State->Written.load(std::memory_order_relaxed);
  int32 Copy=FMath::Min(Count,int32(State->Capture.size())-Written);
  if(Copy>0){FMemory::Memcpy(State->Capture.data()+Written,Out,Copy*sizeof(float));State->Written.store(Written+Copy,std::memory_order_release);}
  return Count;
 }
};
URingAudio::URingAudio(const FObjectInitializer& ObjectInitializer):Super(ObjectInitializer){NumChannels=2;bAllowSpatialization=false;bAutoActivate=false;State=MakeShared<FRingAudioState,ESPMode::ThreadSafe>();}
void URingAudio::CaptureTestAudio(){State->Capture.resize(96000*2*20);}
void URingAudio::SetEngine(float Rpm,float Load,float Speed,bool Muted){State->Rpm.store(Rpm);State->Load.store(Load);State->Speed.store(Speed);State->Muted.store(Muted);}
ISoundGeneratorPtr URingAudio::CreateSoundGenerator(const FSoundGeneratorInitParams& Params){return MakeShared<FRingGenerator,ESPMode::ThreadSafe>(State,Params);}
bool URingAudio::SaveCapture(const FString& Filename,float& Peak,int32& Samples){
 Samples=State->Written.load(std::memory_order_acquire);int32 Rate=State->Rate.load();Peak=0;
 if(Samples==0||Rate==0)return false;
 TArray<uint8> Wav;auto bytes=[&](const void* P,int32 N){Wav.Append(static_cast<const uint8*>(P),N);};
 auto u32=[&](uint32 V){bytes(&V,4);};auto u16=[&](uint16 V){bytes(&V,2);};
 bytes("RIFF",4);u32(36+Samples*2);bytes("WAVEfmt ",8);u32(16);u16(1);u16(2);u32(Rate);u32(Rate*4);u16(4);u16(16);bytes("data",4);u32(Samples*2);
 for(int32 I=0;I<Samples;I++){float V=State->Capture[I];Peak=FMath::Max(Peak,FMath::Abs(V));int16 S=int16(FMath::Clamp(V,-1.f,1.f)*32767);bytes(&S,2);}
 return FFileHelper::SaveArrayToFile(Wav,*Filename)&&Peak>0&&Peak<1;
}

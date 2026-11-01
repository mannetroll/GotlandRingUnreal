#pragma once
#include "CoreMinimal.h"
struct FRingRide {
 FVector Previous=FVector::ZeroVector,Acceleration=FVector::ZeroVector,Lean=FVector::ZeroVector,Tilt=FVector::ZeroVector,Contacts=FVector::ZeroVector,Offset=FVector::ZeroVector,Angles=FVector::ZeroVector,RoadOffset=FVector::ZeroVector,RoadAngles=FVector::ZeroVector;
 float Distance=0;bool Primed=false;
 void Tick(float Dt,FVector Velocity,FQuat Rotation,FVector Contact){
  if(Dt<=0)return;float Speed=Velocity.Size();FVector Change=Primed?Rotation.UnrotateVector((Velocity-Previous)/Dt):FVector::ZeroVector;Previous=Velocity;Primed=true;Change.Z=0;Change=Change.GetClampedToMaxSize(15);if(Speed<.2f)Change=FVector::ZeroVector;
  Acceleration=FMath::Lerp(Acceleration,Change,1-FMath::Exp(-Dt/.14f));Contacts=FMath::Lerp(Contacts,Contact,1-FMath::Exp(-Dt/.06f));
  const float Blend=1-FMath::Exp(-Dt/.12f);Lean=FMath::Lerp(Lean,FVector(-Acceleration.X*.0025f,-Acceleration.Y*.002f,0),Blend);Tilt=FMath::Lerp(Tilt,FVector(Acceleration.X*.09f,0,-Acceleration.Y*.07f),Blend);
  Distance=FMath::Fmod(Distance+Speed*Dt,1000.f);float Rough=(Contacts.X*.0012f+Contacts.Y*.009f+Contacts.Z*.014f)*FMath::Clamp((Speed-.5f)/11.5f,0.f,1.f);
  float Wave=FMath::Sin(Distance*2*PI/2.5f)*.65f+FMath::Sin(Distance*2*PI/.8f)*.35f;
  RoadOffset=FVector(0,FMath::Sin(Distance*2*PI/1.25f)*Rough*.22f,Wave*Rough);RoadAngles=FVector(-Wave*Rough*24,0,-FMath::Sin(Distance*2*PI/5)*Rough*32);
  Offset=(Lean+RoadOffset).GetClampedToMaxSize(.045f);Angles=Tilt+RoadAngles;
 }
};

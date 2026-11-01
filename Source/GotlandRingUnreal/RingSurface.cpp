#include "RingCar.h"
FVector ARingCar::SurfaceContacts()const{
 struct FKerb{int First,Last,Side;};static const FKerb Kerbs[]={
 #include "RingKerbs.inl"
 };
 FVector Contacts=FVector::ZeroVector;
 for(int W=0;W<4;W++){
  FVector P=GetActorTransform().TransformPosition(FVector(W<2?126:-126,W%2==0?-73:73,-48))/100;
  if(Training){if(OnDrivingRoad(P*100))Contacts.X+=.25f;else Contacts.Z+=.25f;continue;}
  float Fraction,Offset;bool Road;int I=Pilot->Nearest({float(P.X),float(P.Y)},Fraction,Offset,Road);
  if(Road){Contacts.X+=.25f;continue;}
  const float Width=Offset<0?Pilot->Rows[I].Left:Pilot->Rows[I].Right,Outside=FMath::Abs(Offset)-Width;
  bool Kerb=false;if(Outside>=.02f&&Outside<=1.17f)for(const auto& K:Kerbs)if(I>=K.First&&I<K.Last&&FMath::Sign(Offset)==K.Side){Kerb=true;break;}
  if(Kerb)Contacts.Y+=.25f;else Contacts.Z+=.25f;
 }
 return Contacts;
}

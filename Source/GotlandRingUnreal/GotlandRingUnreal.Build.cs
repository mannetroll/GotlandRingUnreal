using UnrealBuildTool;
public class GotlandRingUnreal : ModuleRules {
 public GotlandRingUnreal(ReadOnlyTargetRules Target) : base(Target) {
  PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
  PublicDependencyModuleNames.AddRange(new string[]{"Core","CoreUObject","Engine","InputCore","PhysicsCore","Json","AudioMixer"});
 }
}

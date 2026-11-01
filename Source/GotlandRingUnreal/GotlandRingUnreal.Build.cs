using UnrealBuildTool;
public class GotlandRingUnreal : ModuleRules {
 public GotlandRingUnreal(ReadOnlyTargetRules Target) : base(Target) {
  if(Target.Platform==UnrealTargetPlatform.Win64)PublicSystemLibraries.AddRange(new[]{"Shell32.lib","Ole32.lib"});
  PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
  PublicDependencyModuleNames.AddRange(new string[]{"Core","CoreUObject","Engine","InputCore","PhysicsCore","Json","AudioMixer","Slate","SlateCore","RenderCore","ProceduralMeshComponent"});
 }
}
